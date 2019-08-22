#include "timer.h"

#include <assert.h>
#include <errno.h>
#ifndef WIN32
#include <sys/timerfd.h>
#endif
#include "ioloop.h"
#include "log.h"

#ifdef WIN32
void wonTimer(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    if (dwUser != NULL) {
        auto timer = (tnet::Timer*)dwUser;
        if (timer != nullptr) {
            timer->onTimer(timer->loop(), 0);
        }
    }
}
#endif

namespace tnet {
const uint64_t milliPerSeconds = 1000;
const uint64_t microPerSeconds = 1000000;
const uint64_t nanoPerSeconds = 1000000000;
const uint64_t nanoPerMilli = 1000000;

Timer::Timer(const TimerHandler_t& handler, int repeat, int after)
    : m_loop(0)
    , m_running(false)
    , m_handler(handler)
    , m_repeated(false) {
#ifndef WIN32
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (m_fd < 0) {
        LOG_ERROR("create timer error %s", errorMsg(errno));
        return;
    }
#endif
    reset(repeat, after);
}

Timer::~Timer() {
    LOG_INFO("destroyed %d", m_fd);
    if (m_fd > 0) {
#ifndef WIN32
        close(m_fd);
#else
        timeKillEvent(m_fd);
#endif
    }
}

void Timer::start(IOLoop* loop) {
    assert(m_fd > 0);
    if (m_running) {
        LOG_WARN("timer was started");
        return;
    }

    LOG_INFO("start timer %d", m_fd);

    m_loop = loop;
    m_running = true;
    m_loop->addHandler(m_fd, TNET_READ, std::bind(&Timer::onTimer, shared_from_this(), _1, _2));
}

void Timer::stop() {
    if (!m_running) {
        LOG_WARN("timer was stopped");
        return;
    }

    m_running = false;
    m_loop->removeHandler(m_fd);
}

void Timer::reset(int repeat, int after) {
#ifndef WIN32
    if (m_fd <= 0) {
        return;
    }
    struct itimerspec t;
    if (repeat > 0) {
        t.it_interval.tv_sec = (uint64_t)repeat / milliPerSeconds;
        t.it_interval.tv_nsec = ((uint64_t)repeat % milliPerSeconds) * nanoPerMilli;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    t.it_value.tv_sec = now.tv_sec + (uint64_t)after / milliPerSeconds;
    t.it_value.tv_nsec = now.tv_nsec + ((uint64_t)after % milliPerSeconds) * nanoPerMilli;

    if (timerfd_settime(m_fd, TFD_TIMER_ABSTIME, &t, NULL) < 0) {
        LOG_ERROR("set timer error");
    }
#else
    UINT uDelay = 0, uResolution = 1, fuEvent = TIME_ONESHOT;
    if (repeat > 0) {
        uDelay = (uint64_t)repeat / milliPerSeconds;
        fuEvent = TIME_PERIODIC;
    }
    if (after > 0) {
        uDelay = (uint64_t)after / milliPerSeconds;
    }
    if (uDelay <= 0) {
        uDelay = 1;
    }
    m_fd = timeSetEvent(uDelay, uResolution, (LPTIMECALLBACK)wonTimer, (DWORD_PTR)this, fuEvent);
#endif
    m_repeated = (repeat > 0);
}

void Timer::onTimer(IOLoop* loop, int events) {
    TimerPtr_t timer = shared_from_this();
#ifndef WIN32
    uint64_t exp;
    auto s = read(m_fd, &exp, sizeof(uint64_t));
    if (s != sizeof(exp)) {
        LOG_ERROR("onTimer read error");
        return;
    }
#endif
    m_handler(timer);
    if (!isRepeated()) {
        timer->stop();
    }
}

#ifdef WIN32

LARGE_INTEGER getFILETIMEoffset() {
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

int Timer::clock_gettime(int X, timeval* tv) {
    LARGE_INTEGER t;
    FILETIME f;
    static LARGE_INTEGER offset;
    static double frequencyToMicroseconds;
    static int initialized = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter){
        QueryPerformanceCounter(&t);
    } else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    t.QuadPart = (LONGLONG)(t.QuadPart / frequencyToMicroseconds);
    tv->tv_sec = (long)(t.QuadPart / 1000000);
    tv->tv_usec = t.QuadPart % 1000000;
    return (0);
}

#endif

}
