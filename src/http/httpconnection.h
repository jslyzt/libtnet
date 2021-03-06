#pragma once

#include <map>
#include <string>

extern "C"
{
#include "http_parser.h"
}

#include "tnet_http.h"
#include "httpparser.h"
#include "httprequest.h"

namespace tnet {
class HttpServer;
class HttpRequest;
class HttpResponse;

//http server connection
class HttpConnection : public HttpParser
    , public std::enable_shared_from_this<HttpConnection> {
public:
    typedef std::function<void (HttpConnectionPtr_t&, const HttpRequest&, RequestEvent, const void*)> RequestCallback_t;

    HttpConnection(const ConnectionPtr_t& conn, const RequestCallback_t& callback);
    ~HttpConnection();

    int getSockFd() { return m_fd; }

    void send(const HttpResponse& resp);
    void send(int statusCode);
    void send(int statusCode, const std::string& body);
    void send(int statusCode, const std::string& body, const Headers_t& headers);

    //send completely callback, called when all send buffers are send.
    //If there was a previous callback, that callback will be overwritten
    void send(const HttpResponse& resp, const Callback_t& callback);
    void send(int statusCode, const Callback_t& callback);
    void send(int statusCode, const std::string& body, const Callback_t& callback);
    void send(int statusCode, const std::string& body, const Headers_t& headers, const Callback_t& callback);

    //after is milliseconds
    void shutDown(int after);

    ConnectionPtr_t lockConn() { return m_conn.lock(); }
    WeakConnectionPtr_t getConn() { return m_conn; }

    static void setMaxHeaderSize(size_t size) { ms_maxHeaderSize = size; }
    static void setMaxBodySize(size_t size) { ms_maxBodySize = size; }

    void onConnEvent(ConnectionPtr_t& conn, ConnEvent event, const void* context);
private:
    int onMessageBegin();
    int onUrl(const char* at, size_t length);
    int onHeader(const std::string& field, const std::string& value);
    int onHeadersComplete();
    int onBody(const char* at, size_t length);
    int onMessageComplete();
    int onUpgrade(const char* at, size_t length);
    int onError(const HttpError& error);

private:
    std::weak_ptr<Connection> m_conn;
    int m_fd;

    HttpRequest m_request;

    RequestCallback_t m_callback;

    Callback_t m_sendCallback;

    static size_t ms_maxHeaderSize;
    static size_t ms_maxBodySize;
};
}

