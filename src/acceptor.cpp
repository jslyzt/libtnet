#include "acceptor.h"

#include <sys/types.h>
#include <fcntl.h>

#include <errno.h>
#include <assert.h>

#include <memory>
#include <functional>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/socket.h>
#endif

#include "ioloop.h"
#include "address.h"
#include "sockutil.h"
#include "log.h"

using namespace std::placeholders;

namespace tnet {

int createDummyFd() {
#ifndef WIN32
    return open("/dev/null", O_RDONLY | O_CLOEXEC);
#endif
    return 0;
}

Acceptor::Acceptor(const NewConnCallback_t& callback)
    : m_loop(0)
    , m_sockFd(0)
    , m_dummyFd(createDummyFd())
    , m_running(false)
    , m_callback(callback) {

}

Acceptor::~Acceptor() {
    if (m_sockFd > 0) {
        close(m_sockFd);
    }
    if (m_dummyFd > 0) {
        close(m_dummyFd);
    }
}

int Acceptor::listen(const Address& addr) {
    int fd = SockUtil::bindAndListen(addr);
    if (fd < 0) {
        return fd;
    }
    m_sockFd = fd;
    return m_sockFd;
}

void Acceptor::start(IOLoop* loop) {
    assert(m_sockFd > 0);
    if (m_running) {
        LOG_WARN("acceptor was started");
        return;
    }

    m_loop = loop;
    m_running = true;
    m_loop->addHandler(m_sockFd, TNET_READ, std::bind(&Acceptor::onAccept, this, _1, _2));
}

void Acceptor::stop() {
    assert(m_sockFd > 0);
    if (!m_running) {
        LOG_WARN("acceptor was stoped");
        return;
    }

    m_running = false;

    LOG_TRACE("stop %d", m_sockFd);
    m_loop->removeHandler(m_sockFd);
}

void Acceptor::onAccept(IOLoop* loop, int events) {
#ifndef WIN32
    int sockFd = accept4(m_sockFd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    struct sockaddr_in sa;
    auto slen = (int)sizeof(sa);
    int sockFd = (int)accept(m_sockFd, (struct sockaddr*) &sa, &slen);
#endif
    if (sockFd < 0) {
        int err = errno;
        if (err == EMFILE || err == ENFILE) {
            LOG_ERROR("accept error %s", errorMsg(err));
            if (m_dummyFd > 0) {
                close(m_dummyFd);
            }
            sockFd = (int)accept(m_sockFd, NULL, NULL);
            close(sockFd);

            m_dummyFd = createDummyFd();
        }
        return;
    } else {
        LOG_INFO("onAccept %d", sockFd);
        SockUtil::setNoDelay(sockFd, true);
        m_callback(loop, sockFd);
    }
}
}
