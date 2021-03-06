#include "connection.h"

#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <assert.h>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/uio.h>
#endif

#include "ioloop.h"
#include "log.h"
#include "sockutil.h"
#include "timer.h"

using namespace std;
using namespace std::placeholders;

namespace tnet {

void dummyConnEvent(ConnectionPtr_t&, ConnEvent, const void*) {
}

const int MaxReadBuffer = 4096;
Connection::Connection(IOLoop* loop, int fd)
    : m_callback(nullptr)
    , m_loop(loop)
    , m_fd(fd)
    , m_status(None)
    , m_lastActiveTime(0)
    , m_IdleTimeout(0) {
}

Connection::~Connection() {
    //LOG_INFO("connection destroyed %d", m_fd);
}

void Connection::clearEventCallback() {
    m_callback = nullptr;
}

void Connection::updateActiveTime() {
    struct timespec t;
    Timer::clock_gettime(CLOCK_MONOTONIC, &t);
    m_lastActiveTime = t.tv_sec;
}

void Connection::onEstablished() {
    if (m_status != None) {
        LOG_ERROR("invalid status %d, not None", m_status);
        return;
    }

    m_status = Connected;

    updateActiveTime();

    ConnectionPtr_t conn = shared_from_this();
    if (m_fd > 0) {
        m_loop->addHandler(m_fd, TNET_READ, std::bind(&Connection::onHandler, conn, _1, _2));
    }

    if (m_callback != nullptr) {
        m_callback(conn, Conn_ListenEvent, 0);
    }
}

void Connection::connect(const Address& addr) {
    if (m_status != None) {
        LOG_ERROR("invalid status %d, must None", m_status);
        return;
    }

    int err = SockUtil::connect(m_fd, addr);
    if (err < 0) {
        ConnectionPtr_t conn = shared_from_this();
        if (m_callback != nullptr) {
            m_callback(conn, Conn_ConnFailEvent, 0);
        }
        return;
    } else {
        m_status = Connected;
    }

    updateActiveTime();

    ConnectionPtr_t conn = shared_from_this();
    if (m_fd > 0) {
        m_loop->addHandler(m_fd, m_status == Connected ? TNET_READ : TNET_WRITE, std::bind(&Connection::onHandler, conn, _1, _2));
    }

    if (m_callback != nullptr) {
        m_callback(conn, Conn_ConnectEvent, 0);
    }
}

void Connection::onHandler(IOLoop* loop, int events) {
    ConnectionPtr_t conn = shared_from_this();
    if (events & TNET_READ) {
        handleRead();
    }

    if (events & TNET_WRITE) {
        if (m_status == Connecting) {
            handleConnect();
        } else {
            handleWrite();
        }
    }

    if (events & TNET_ERROR) {
        handleError();
    }
}

void Connection::shutDown(int after) {
    if (m_status == Disconnecting || m_status == Disconnected) {
        return;
    }
    m_status = Disconnecting;
    if (after == 0) {
        handleClose();
    } else {
        ConnectionPtr_t conn = shared_from_this();
        m_loop->runInWheel(after, std::bind(&Connection::handleClose, conn));
    }
}

void Connection::handleConnect() {
    if (m_status != Connected) {
        return;
    }
    if (SockUtil::getSockError(m_fd) != 0) {
        handleError();
        return;
    }

    m_loop->updateHandler(m_fd, TNET_READ);
    updateActiveTime();
    m_status = Connected;

    ConnectionPtr_t conn = shared_from_this();
    if (m_callback != nullptr) {
        m_callback(conn, Conn_ConnectEvent, 0);
    }
}

void Connection::handleRead() {
    if (m_status != Connected) {
        return;
    }

    char buf[MaxReadBuffer];
    int n = ::recv(m_fd, buf, sizeof(buf), 0);
    if (n > 0) {
        StackBuffer b(buf, n);
        updateActiveTime();
        ConnectionPtr_t conn = shared_from_this();
        if (m_callback != nullptr) {
            m_callback(conn, Conn_ReadEvent, &b);
        }
        return;
    } else if (n == 0) {
        handleClose();
        return;
    } else {
        int err = errno;
        LOG_ERROR("read socket %d error %d => %s", m_fd, err, errorMsg(err));
        if (err == EAGAIN || err == EWOULDBLOCK) {
            return;
        }
        handleError();
        return;
    }
}

void Connection::handleWrite() {
    handleWrite(string());
}

void Connection::handleWrite(const string& data) {
    if (m_status != Connected) {
        return;
    }

    if (m_sendBuffer.empty() == true && data.empty() == true) {
        m_loop->updateHandler(m_fd, TNET_READ);
        return;
    }

    auto totalSize = (int)(m_sendBuffer.size() + data.size());
    int n = 0;
#ifndef WIN32
    struct iovec iov[2];
    iov[0].iov_base = (void*)m_sendBuffer.data();
    iov[0].iov_len = m_sendBuffer.size();
    iov[1].iov_base = (void*)data.data();
    iov[1].iov_len = data.size();
    n = writev(m_fd, iov, 2);
#else
    if (m_sendBuffer.empty() == false) {
        n = ::send(m_fd, m_sendBuffer.c_str(), (int)(m_sendBuffer.size()), 0);
    }
    if (data.empty() == false && n >= 0) {
        n += ::send(m_fd, data.c_str(), (int)(data.size()), 0);
    }
#endif
    if (n == totalSize) {
        string().swap(m_sendBuffer);
        if (m_callback != nullptr) {
            ConnectionPtr_t conn = shared_from_this();
            m_callback(conn, Conn_WriteCompleteEvent, 0);
        }
        m_loop->updateHandler(m_fd, TNET_READ);
        updateActiveTime();
        return;
    } else if (n < 0) {
        int err = errno;
        LOG_INFO("write error %s", errorMsg(err));
        if (err == EAGAIN || err == EWOULDBLOCK) { //try write later, can enter here?
            m_sendBuffer.append(data);
            m_loop->updateHandler(m_fd, TNET_READ | TNET_WRITE);
            return;
        } else {
            string().swap(m_sendBuffer);
            handleError();
            return;
        }
    } else {
        if (m_sendBuffer.size() < n) {
            n -= (int)m_sendBuffer.size();
            m_sendBuffer = data.substr(n);
        } else {
            m_sendBuffer = m_sendBuffer.substr(n);
            m_sendBuffer += data;
        }

        updateActiveTime();
        m_loop->updateHandler(m_fd, TNET_READ | TNET_WRITE);
    }
}

bool Connection::disconnect() {
    if (m_status == Disconnected) {
        return false;
    }

    m_status = Disconnected;
    m_loop->removeHandler(m_fd);

    LOG_INFO("connect %d closed", m_fd);
#ifdef WIN32
    closesocket(m_fd);
#else
    close(m_fd);
#endif
    return true;
}

void Connection::handleError() {
    disconnect();
    ConnectionPtr_t conn = shared_from_this();
    if (m_callback != nullptr) {
        m_callback(conn, Conn_ErrorEvent, 0);
    }
}

void Connection::handleClose() {
    if (disconnect()) {
        ConnectionPtr_t conn = shared_from_this();
        if (m_callback != nullptr) {
            m_callback(conn, Conn_CloseEvent, 0);
        }
    }
}

int Connection::send(const string& data) {
    if (m_status != Connected) {
        LOG_INFO("send error");
        return -1;
    }

    handleWrite(data);
    return 0;
}
}

