#include "ioloop.h"

#include <stdlib.h>
#include <signal.h>
#include <algorithm>

#include "poller.h"
#include "log.h"
#include "ioevent.h"
#include "timer.h"
#include "notifier.h"
#include "timingwheel.h"

using namespace std;
using namespace std::placeholders;

namespace tnet {
class IgnoreSigPipe {
public:
    IgnoreSigPipe() {
#ifndef WIN32
        signal(SIGPIPE, SIG_IGN);
#endif
    }
};

static IgnoreSigPipe initObj;

const int DefaultEventsCapacity = 1024;
const int MaxPollWaitTime = 1 * 1000;

IOLoop::IOLoop()
    : m_running(false) {
    m_poller = new Poller(this);
    m_notifier = std::make_shared<Notifier>(std::bind(&IOLoop::onWake, this, _1));
    m_wheel = std::make_shared<TimingWheel>(1000, 3600);
    m_events.resize(DefaultEventsCapacity, 0);
}

IOLoop::~IOLoop() {
    if (m_poller != nullptr) {
        delete m_poller;
    }
    for_each(m_events.begin(), m_events.end(), default_delete<IOEvent>());
    m_delevents.clear();
}

void IOLoop::start() {
    if (m_running == true) {
        return;
    }
    m_running = true;
    m_notifier->start(this);
    m_wheel->start(this);
    run();
}

void IOLoop::stop() {
    if (m_running == false) {
        return;
    }
    m_running = false;
    m_notifier->notify();
}

void IOLoop::run() {
    while (m_running) {
        m_poller->poll(MaxPollWaitTime, m_events);
        handleCallbacks();
        checkDelEvents();
    }

    LOG_TRACE("loop stop");
    m_notifier->stop();
}

int IOLoop::addHandler(int fd, int events, const IOHandler_t& handler) {
    if (m_events.size() <= fd) {
        m_events.resize(fd + 1, 0);
    }

    if (m_events[fd] != 0) {
        LOG_ERROR("add duplicate handler %d", fd);
        return -1;
    }

    if (m_poller->add(fd, events) != 0) {
        return -1;
    }

    m_events[fd] = new IOEvent(fd, events, handler);
    return 0;
}

int IOLoop::updateHandler(int fd, int events) {
    if (m_events.size() <= fd || m_events[fd] == nullptr) {
        LOG_ERROR("invalid fd %d", fd);
        return -1;
    }
    if (m_events[fd]->events == events) {
        return 0;
    }
    if (m_poller->update(fd, events) != 0) {
        return -1;
    }
    m_events[fd]->events = events;
    return 0;
}

int IOLoop::removeHandler(int fd) {
    if (m_events.size() <= fd || m_events[fd] == 0) {
        LOG_INFO("invalid fd %d", fd);
        return -1;
    }
    m_delevents.push_back(fd);
    return 0;
}

int IOLoop::trueRemoveHandler(int fd) {
    if (m_events.size() <= fd || m_events[fd] == nullptr) {
        LOG_INFO("invalid fd %d", fd);
        return -1;
    }
    m_poller->remove(fd);
    auto env = m_events[fd];
    delete env;
    m_events[fd] = nullptr;
    return 0;
}

void onTimerHandler(const TimerPtr_t& timer, const Callback_t& callback) {
    callback();
    timer->stop();
}

TimerPtr_t IOLoop::runAfter(int after, const Callback_t& callback) {
    TimerPtr_t timer = std::make_shared<Timer>(std::bind(&onTimerHandler, _1, callback), after, 0);
    if (timer != nullptr) {
        timer->start(this);
    }
    return timer;
}

void IOLoop::addCallback(const Callback_t& callback) {
    {
        SpinLockGuard g(m_lock);
        m_callbacks.push_back(callback);
    }
    m_notifier->notify();
}

void IOLoop::checkDelEvents() {
    for (auto fd : m_delevents) {
        trueRemoveHandler(fd);
    }
    m_delevents.clear();
}

void IOLoop::handleCallbacks() {
    vector<Callback_t> callbacks;
    {
        SpinLockGuard g(m_lock);
        callbacks.swap(m_callbacks);
    }

    for (size_t i = 0; i < callbacks.size(); ++i) {
        callbacks[i]();
    }
}

void IOLoop::onWake(const NotifierPtr_t& notifier) {
    //only to wakeup poll
}

void IOLoop::runInWheel(int timeout, const TimingWheelHandler_t& handler) {
    m_wheel->add(handler, timeout);
}
}
