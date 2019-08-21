#pragma once

#include <string>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

namespace tnet {

class Address {
public:
    Address(uint16_t port);
    Address(const std::string& ip, uint16_t port);
    Address(const struct sockaddr_in& addr);

    //host byte order
    uint32_t ip() const;

    //host byte order
    uint16_t port() const;

    const struct sockaddr_in& sockAddr() const { return m_addr; }

    std::string ipstr() const;

private:
    struct sockaddr_in m_addr;
};

}

