#include "tcpserver.h"

#include <algorithm>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "address.h"
#include "sockutil.h"
#include "log.h"
#include "acceptor.h"
#include "connection.h"
#include "ioloop.h"
#include "signaler.h"
#include "timer.h"
#include "tprocess.h"
#include "timingwheel.h"

using namespace std;
using namespace std::placeholders;

namespace tnet {

const int defaultIdleTimeout = 30;  // 默认无数据读写超时时间

TcpServer::TcpServer()
    : m_loop(nullptr)
    , m_process(nullptr)
    , m_runCallback(nullptr) {
    m_running = false;
    m_maxIdleTimeout = defaultIdleTimeout;
}

TcpServer::~TcpServer() {
    if (m_loop != nullptr) {
        delete m_loop;
        m_loop = nullptr;
    }
    if (m_process != nullptr) {
        m_process = nullptr;
    }
    if (m_runCallback != nullptr) {
        m_runCallback = nullptr;
    }
}

int TcpServer::listen(const Address& addr, const ConnEventCallback_t& callback) {
    LOG_INFO("listen %s:%d", addr.ipstr().c_str(), addr.port());
    NewConnCallback_t cb = std::bind(&TcpServer::onNewConnection, this, _1, _2, callback);
    AcceptorPtr_t acceptor = std::make_shared<Acceptor>(cb);
    if (acceptor->listen(addr) < 0) {
        return -1;
    }
    m_acceptors.push_back(acceptor);
    return 0;
}

IOLoop* TcpServer::createLoop() {
    if (m_loop == nullptr) {
        m_loop = new IOLoop();
    }
    return m_loop;
}

void TcpServer::run() {
    if (m_running == true) {
        return;
    }

    if (m_loop == nullptr) {
        m_loop = new IOLoop();
    }

    m_running = true;
    m_loop->addCallback(std::bind(&TcpServer::onRun, this));
    m_loop->start();
}

void TcpServer::onRun() {
    LOG_TRACE("tcp server on run");

    for_each(m_acceptors.begin(), m_acceptors.end(), std::bind(&Acceptor::start, _1, m_loop));

    vector<int> signums{SIGINT, SIGTERM};
    m_signaler = std::make_shared<Signaler>(signums, std::bind(&TcpServer::onSignal, this, _1, _2));

    m_signaler->start(m_loop);
    m_idleWheel = std::make_shared<TimingWheel>(1000, 3600);
    m_idleWheel->start(m_loop);

    if (m_runCallback != nullptr) {
        m_runCallback(m_loop);
    }
}

void TcpServer::start(size_t maxProcess) {
#ifndef WIN32
    if (maxProcess > 1) {
        if (m_process == nullptr) {
            m_process = std::make_shared<Process>();
        }
        m_process->wait(maxProcess, std::bind(&TcpServer::run, this));
        return;
    }
#endif
     run();
}

void TcpServer::onStop() {
    if (m_running == false) {
        return;
    }
    LOG_TRACE("tcp server on stop");

    m_running = false;
    m_idleWheel->stop();
    m_signaler->stop();
    for (auto con : m_acceptors) {
        if (con != nullptr) {
            con->stop();
        }
    }
    m_loop->stop();
}

void TcpServer::stop() {
    LOG_TRACE("stop server");
    if (m_process != nullptr) {
        m_process->stop();
    }
    onStop();
}

void TcpServer::onNewConnection(IOLoop* loop, int fd, const ConnEventCallback_t& callback) {
    ConnectionPtr_t conn = std::make_shared<Connection>(loop, fd);
    conn->setEventCallback(callback);
    conn->onEstablished();

    int afterCheck = m_maxIdleTimeout / 2;
#ifndef WIN32
    afterCheck += random() % afterCheck;
#else
    afterCheck += rand() % afterCheck;
#endif
    addIdleConnCheck(conn, (uint64_t)afterCheck);
    return;
}

void TcpServer::addIdleConnCheck(ConnectionPtr_t conn, uint64_t timeout) {
    m_idleWheel->add(std::bind(&TcpServer::onIdleConnCheck, this, _1, WeakConnectionPtr_t(conn)), (int)(timeout * 1000));
}

void TcpServer::onIdleConnCheck(const TimingWheelPtr_t& wheel, const WeakConnectionPtr_t& conn) {
    ConnectionPtr_t c = conn.lock();
    if (c == nullptr || c->isClose() == true) {
        return;
    }

    struct timespec t;
    Timer::clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t now = t.tv_sec;

    uint64_t idletmt = c->getIdleTimeout();
    if (idletmt <= 0) {
        idletmt = (uint64_t)m_maxIdleTimeout;
    }

    auto diff = now - c->lastActiveTime();
    if (diff > idletmt) {
        c->shutDown();
    } else {
        addIdleConnCheck(c, idletmt - diff);
    }
}

void TcpServer::onSignal(const SignalerPtr_t& signaler, int signum) {
    switch (signum) {
        case SIGINT:
        case SIGTERM: {
            onStop();
        } break;
        default:
            LOG_ERROR("invalid signal %d", signum);
            break;
    }
}
}
