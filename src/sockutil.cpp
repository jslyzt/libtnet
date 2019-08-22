#include "sockutil.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define snprintf _snprintf
#define close closesocket
typedef int socklen_t;
#endif

#include "address.h"
#include "log.h"

using namespace std;

namespace tnet {

int SockUtil::create() {
#ifndef WIN32
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
#else
    auto fd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
    if (fd < 0) {
        return fd;
    }
    return fd;
}

int SockUtil::bindAndListen(const Address& addr) {
    int err = 0;
    int fd = create();
    if (fd < 0) {
        err = errno;
        LOG_ERROR("create socket error %s", errorMsg(err));
        return fd;
    }

    SockUtil::setReuseable(fd, true);
    do {
        struct sockaddr_in sockAddr = addr.sockAddr();
        if (bind(fd, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) < 0) {
            err = errno;
            LOG_ERROR("bind address %s:%d error: %s", addr.ipstr().c_str(), addr.port(), errorMsg(err));
            break;
        }
        if (listen(fd, SOMAXCONN) < 0) {
            err = errno;
            LOG_ERROR("listen address %s:%d error: %s", addr.ipstr().c_str(), addr.port(), errorMsg(err));
            break;
        }
        return fd;

    } while (0);

    close(fd);
    return err;
}

int SockUtil::connect(int sockFd, const Address& addr) {
    struct sockaddr_in sockAddr = addr.sockAddr();
    auto tryCnt = 0;
    while (true) {
        tryCnt += 1;
        int ret = ::connect(sockFd, (struct sockaddr*)&sockAddr, sizeof(sockAddr));
        if (ret >= 0) {
            SockUtil::setNoDelay(sockFd, true);
            return 0;
        }
#ifdef WIN32
        auto err = WSAGetLastError();
        if (err == WSAEISCONN) {   //成功
            return 0;
        }
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == WSAEINVAL || err == WSAEALREADY) { //重连
#else
        auto err = errno;
        if (err == EISCONN) {  //成功
            return 0;
        }
        if (err == EINPROGRESS || err == EWOULDBLOCK || err == EALREADY) {    //重连
#endif
            if (tryCnt <= 5) {
                sleep();
                continue;
            }
        }
        LOG_ERROR("connect address %s:%d, fd: %d, error: %d => %s", addr.ipstr().c_str(), addr.port(), sockFd, err, errorMsg(err));
        break;
    }
    return -1;
}

int SockUtil::setNoDelay(int sockFd, bool on) {
    int opt = on ? 1 : 0;
#ifndef WIN32
    return setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt)));
#else
    return setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, static_cast<socklen_t>(sizeof(opt)));
#endif
}

int SockUtil::setReuseable(int sockFd, bool on) {
    int opt = on ? 1 : 0;
#ifndef WIN32
    return setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof(opt)));
#else
    return setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, static_cast<socklen_t>(sizeof(opt)));
#endif
}

int SockUtil::setKeepAlive(int sockFd, bool on) {
    int opt = on ? 1 : 0;
#ifndef WIN32
    return setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof(opt)));
#else
    return setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, static_cast<socklen_t>(sizeof(opt)));
#endif
}

int SockUtil::getLocalAddr(int sockFd, Address& addr) {
    struct sockaddr_in sockAddr;
    socklen_t sockLen = sizeof(sockAddr);
    if (getsockname(sockFd, (struct sockaddr*)&sockAddr, &sockLen) != 0) {
        int err = errno;
        return err;
    }
    addr = Address(sockAddr);
    return 0;
}

int SockUtil::getRemoteAddr(int sockFd, Address& addr) {
    struct sockaddr_in sockAddr;
    socklen_t sockLen = sizeof(sockAddr);
    if (getpeername(sockFd, (struct sockaddr*)&sockAddr, &sockLen) != 0) {
        int err = errno;
        return err;
    }
    addr = Address(sockAddr);
    return 0;
}

int SockUtil::getSockError(int sockFd) {
    int opt;
    socklen_t optLen = static_cast<socklen_t>(sizeof(opt));
#ifndef WIN32
    if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &opt, &optLen) < 0) {
#else
    if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, (char*)&opt, &optLen) < 0) {
#endif
        int err = errno;
        return err;
    } else {
        return opt;
    }
}

uint32_t SockUtil::getHostByName(const string& host) {
    struct addrinfo hint;
    struct addrinfo* answer;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(host.c_str(), NULL, &hint, &answer);
    if (ret != 0) {
        LOG_ERROR("getaddrinfo error %s", errorMsg(errno));
        return uint32_t(-1);
    }

    //we only use first find addr
    for (struct addrinfo* cur = answer; cur != NULL; cur = cur->ai_next) {
        return ((struct sockaddr_in*)(cur->ai_addr))->sin_addr.s_addr;
    }

    LOG_ERROR("getHostByName Error");
    return uint32_t(-1);
}

int SockUtil::bindDevice(int sockFd, const string& device) {
#ifndef WIN32
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), device.c_str());
    struct sockaddr_in* sin = (struct sockaddr_in*)&ifr.ifr_addr;

    if (ioctl(sockFd, SIOCGIFADDR, &ifr) < 0) {
        LOG_ERROR("ioctl get addr error %s", errorMsg(errno));
        return -1;
    }

    sin->sin_family = AF_INET;
    sin->sin_port = 0;

    if (bind(sockFd, (struct sockaddr*)sin, sizeof(*sin)) < 0) {
        LOG_ERROR("bind interface error %s", errorMsg(errno));
        return -1;
    }
#endif
    return 0;
}

void SockUtil::sleep(int mics) {
#ifdef WIN32
    if (mics < 1000) {
        Sleep(1);
    } else {
        Sleep((uint32_t)(mics / 1000));
    }
#else
    usleep(mics);
#endif
}
}
