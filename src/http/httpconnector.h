#pragma once

#include "tnet_http.h"
#include "httpparser.h"
#include "httpresponse.h"
#include "connector.h"

namespace tnet {

class HttpConnector : public HttpParser
    , public Connector<HttpConnector> {
public:
    typedef std::function<void (const HttpConnectorPtr_t&, const HttpResponse&, ResponseEvent)> ResponseCallback_t;

    HttpConnector();
    ~HttpConnector();

    void setCallback(const ResponseCallback_t& callback) { m_callback = callback; }
    void clearCallback();

    static void setMaxHeaderSize(size_t size) { ms_maxHeaderSize = size; }
    static void setMaxBodySize(size_t size) { ms_maxBodySize = size; }

    void handleRead(const char* buffer, size_t count);
private:
    int onMessageBegin();
    int onHeader(const std::string& field, const std::string& value);
    int onHeadersComplete();
    int onBody(const char* at, size_t length);
    int onMessageComplete();
    int onError(const HttpError& error);

private:
    HttpResponse m_response;

    ResponseCallback_t m_callback;

    static size_t ms_maxHeaderSize;
    static size_t ms_maxBodySize;
};

}
