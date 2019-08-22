#pragma once

#include <vector>
#include <set>
#ifndef WIN32
#include <sys/epoll.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "tnet.h"

extern "C"
{
    struct epoll_event;
}

namespace tnet {
class IOLoop;
class IOEvent;

//A wrapper for epoll
class Poller {
public:
    Poller(IOLoop* loop);
    ~Poller();

    //timeout is milliseconds
    int poll(int timeout, const std::vector<IOEvent*>& events);

    int add(int fd, int events);
    int update(int fd, int events);
    int remove(int fd);

private:
    IOLoop* m_loop;
#ifndef WIN32
    int     m_fd;
    struct epoll_event* m_events;
    size_t m_eventSize;
#else
    fd_set  m_readfd;
    fd_set  m_writefd;
    std::set<int> m_sockets;
    static bool m_binit;
#endif
};

}
