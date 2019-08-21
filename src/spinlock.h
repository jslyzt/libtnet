#pragma once

#include "tnet.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace tnet {
class SpinLock : public nocopyable {
public:
    SpinLock() {
        m_lock = 0;
    }
    ~SpinLock() {}

    void lock() {
#ifdef WIN32
        while (InterlockedExchange(&m_lock, 1)) {
        }
#else
        while (__sync_lock_test_and_set(&m_lock, 1)) {
        }
#endif
        
    }

    void unlock() {
#ifdef WIN32
        InterlockedExchange(&m_lock, 0);
#else
        __sync_lock_release(&m_lock);
#endif
    }

private:
    volatile unsigned int m_lock;
};

class SpinLockGuard : public nocopyable {
public:
    SpinLockGuard(SpinLock& lock)
        : m_lock(lock) {
        m_lock.lock();
    }

    ~SpinLockGuard() {
        m_lock.unlock();
    }
private:
    SpinLock& m_lock;
};
}
