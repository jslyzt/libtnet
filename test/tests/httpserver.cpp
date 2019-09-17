#include <stdio.h>
#include <string>
#include <signal.h>

#include "log.h"
#include "address.h"
#include "tcpserver.h"
#include "connection.h"
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpconnection.h"
#include "../tnet_test.h"

using namespace std;
using namespace tnet;
using namespace std::placeholders;

namespace http {
    void onHandler(HttpConnectionPtr_t& conn, const HttpRequest& request) {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.setContentType("text/html");
        resp.setKeepAlive(true);
        resp.enableDate();

        resp.body.append("first");
        //resp.body.append(1600, 'a');
        resp.body.append("Hello World");

        conn->send(resp);
    }
}


TEST_F(HttpTest, server) {
    TcpServer s;

    HttpServer httpd(&s);

    httpd.setHttpCallback("/abc", std::bind(&http::onHandler, _1, _2));

    httpd.listen(Address(11181));

    LOG_INFO("start tcp server");

    s.start();

    LOG_INFO("stop server");
}


