#pragma once

#include <string>

#include "tnet.h"

namespace tnet {
class IOLoop;
class Address;

class Connection : public nocopyable
    , public std::enable_shared_from_this<Connection> {
public:
    enum Status {
        None,
        Connecting,
        Connected,
        Disconnecting,
        Disconnected,
    };

    Connection(IOLoop* loop, int fd);
    ~Connection();

    int getSockFd() { return m_fd; }

    void setEventCallback(const ConnEventCallback_t& callback) { m_callback = callback; }
    void clearEventCallback();

    //after is milliseconds, if after is 0, close immediately
    void shutDown(int after = 0);

    //when connection is not connected
    int send(const std::string& data);

    void onEstablished();
    void connect(const Address& addr);

    uint64_t lastActiveTime() { return m_lastActiveTime; }

    bool isConnected() { return m_status == Connected; }
    bool isConnecting() { return m_status == Connecting; }
    bool isClose() { return m_status == Disconnecting || m_status == Disconnected; }

    uint64_t getIdleTimeout() { return m_IdleTimeout; }
    void setIdleTimeout(uint64_t tm) { m_IdleTimeout = tm; }

public:
    void handleRead();
    void handleError();
    void handleClose();

private:
    void onHandler(IOLoop*, int);
    void handleWrite();
    void handleWrite(const std::string& data);
    void handleConnect();
    void updateActiveTime();
    bool disconnect();

private:
    ConnEventCallback_t m_callback;

    IOLoop* m_loop;
    int m_fd;
    int m_status;

    //seconds
    uint64_t m_lastActiveTime;
    uint64_t m_IdleTimeout;
    std::string m_sendBuffer;
};

}
