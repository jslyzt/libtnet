#include "notifier.h"
#include "log.h"
#include "ioloop.h"

#include <assert.h>
#ifndef WIN32
#include <sys/eventfd.h>
#endif

using namespace std::placeholders;

namespace tnet {

Notifier::Notifier(const NotifierHandler_t& handler)
    : m_loop(0)
    , m_fd(0)
    , m_running(false)
    , m_handler(handler) {
#ifndef WIN32
    m_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_fd < 0) {
        LOG_ERROR("eventfd error %s", errorMsg(errno));
    }
#endif
}

Notifier::~Notifier() {
#ifndef WIN32
    if (m_fd > 0) {
        close(m_fd);
    }
#endif
}

void Notifier::start(IOLoop* loop) {
    if (m_running == true) {
        LOG_WARN("event was started");
        return;
    }

    m_running = true;
    m_loop = loop;
    if (m_fd > 0) {
        m_loop->addHandler(m_fd, TNET_READ, std::bind(&Notifier::onEvent, shared_from_this(), _1, _2));
    }
}

void Notifier::stop() {
    if (m_running == false) {
        LOG_WARN("event was stopped");
        return;
    }

    m_running = false;
    if (m_fd > 0) {
        m_loop->removeHandler(m_fd);
    }
}

void Notifier::notify() {
#ifndef WIN32
    eventfd_t value = 1;
    if (eventfd_write(m_fd, value) < 0) {
        LOG_ERROR("eventfd_write error");
    }
#endif
}

void Notifier::onEvent(IOLoop* loop, int events) {
    NotifierPtr_t notifier = shared_from_this();
#ifndef WIN32
    eventfd_t value;
    if (eventfd_read(m_fd, &value) < 0) {
        LOG_ERROR("eventfd read error");
        return;
    }
#endif
    m_handler(notifier);
}

}
