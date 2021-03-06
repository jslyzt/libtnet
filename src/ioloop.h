#pragma once

#include <vector>

#include "tnet.h"
#include "spinlock.h"

namespace tnet {
class IOEvent;
class Poller;

class IOLoop {
public:
    IOLoop();
    ~IOLoop();

    void start();
    void stop();

    int addHandler(int fd, int events, const IOHandler_t& handler);
    int updateHandler(int fd, int events);
    int removeHandler(int fd);
    int trueRemoveHandler(int fd);

    //after is milliseconds
    TimerPtr_t runAfter(int after, const Callback_t& callback);

    //this only thread safe
    void addCallback(const Callback_t& callback);

    //timeout is milliseconds
    void runInWheel(int timeout, const TimingWheelHandler_t& handler);

    bool isRunning() { return m_running; }
private:
    void run();

    void onWake(const NotifierPtr_t& notifier);

    void handleCallbacks();

    void checkDelEvents();
private:
    int m_pollFd;

    volatile bool m_running;

    std::vector<IOEvent*> m_events;
    std::vector<int> m_delevents;

    Poller* m_poller;

    std::vector<Callback_t> m_callbacks;
    SpinLock m_lock;

    NotifierPtr_t m_notifier;

    TimingWheelPtr_t m_wheel;
};

}
