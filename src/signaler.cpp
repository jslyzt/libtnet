#include "signaler.h"

#include <algorithm>

#include <signal.h>
#include <errno.h>
#include <assert.h>
#ifndef WIN32
#include <sys/signalfd.h>
#endif

#include "log.h"
#include "ioloop.h"

using namespace std;
using namespace std::placeholders;

namespace tnet {
Signaler::Signaler(int signum, const SignalHandler_t& handler)
    : m_loop(0)
    , m_fd(-1)
    , m_running(false)
    , m_handler(handler) {
    m_signums.resize(signum);
    m_fd = createSignalFd(m_signums);
}

Signaler::Signaler(const vector<int>& signums, const SignalHandler_t& handler)
    : m_loop(0)
    , m_fd(-1)
    , m_running(false)
    , m_signums(signums)
    , m_handler(handler) {
    m_fd = createSignalFd(m_signums);
}

Signaler::~Signaler() {
    //LOG_INFO("destroyed %d", m_fd);
    if (m_fd > 0) {
#ifndef WIN32
        close(m_fd);
#endif
    }
}

int Signaler::createSignalFd(const std::vector<int>& signums) {
#ifndef WIN32
    sigset_t mask;
    sigemptyset(&mask);
    for (size_t i = 0; i < signums.size(); ++i) {
        sigaddset(&mask, signums[i]);
    }
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        LOG_ERROR("sigprocmask error");
        return -1;
    }
    int fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0) {
        LOG_ERROR("signalfd error %s", errorMsg(errno));
    }
    return fd;
#else
    return 0;
#endif
}

void Signaler::start(IOLoop* loop) {
    if (m_fd > 0) {
        if (m_running) {
            LOG_WARN("signaler was started");
            return;
        }

        LOG_TRACE("start signaler %d", m_fd);
        m_running = true;
        m_loop = loop;
        m_loop->addHandler(m_fd, TNET_READ, std::bind(&Signaler::onSignal, shared_from_this(), _1, _2));
    }
}

void Signaler::stop() {
    if (m_fd > 0) {
        if (!m_running) {
            LOG_WARN("signaler was stopped");
            return;
        }

        LOG_TRACE("stop signaler %d", m_fd);
        m_running = false;
        m_loop->removeHandler(m_fd);
    }
}

void Signaler::onSignal(IOLoop* loop, int events) {
    SignalerPtr_t signaler = shared_from_this();
    int signum = 0;
#ifndef WIN32
    struct signalfd_siginfo fdsi;
    int s = read(m_fd, &fdsi, sizeof(fdsi));
    if (s != sizeof(fdsi)) {
        LOG_ERROR("onSignal read error");
        return;
    }
    signum = fdsi.ssi_signo;
    if (std::find(m_signums.begin(), m_signums.end(), signum) == m_signums.end()) {
        LOG_ERROR("uexpect signum %d", signum);
        return;
    }
#endif
    m_handler(signaler, signum);
}
}
