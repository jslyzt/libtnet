#include "tnet.h"

#include <stdlib.h>

#include "log.h"
#include "tcpserver.h"
#include "address.h"
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpconnection.h"
#include "timingwheel.h"
#include "../tnet_test.h"

using namespace std;
using namespace std::placeholders;
using namespace tnet;

class CometServer {
public:
    TimingWheelPtr_t wheel;
};

namespace comet {
    static CometServer comet;

    void onServerRun(IOLoop* loop) {
        comet.wheel = std::make_shared<TimingWheel>(1000, 3600);
        comet.wheel->start(loop);
    }

    void onTimeout(const TimingWheelPtr_t& wheel, const WeakHttpConnectionPtr_t& conn) {
        HttpConnectionPtr_t c = conn.lock();
        if (c) {
            c->send(200);
        }
    }

    void onHandler(HttpConnectionPtr_t& conn, const HttpRequest& request) {
        if (request.method == HTTP_GET) {
#ifndef WIN32
            int timeout = random() % 60 + 30;
#else
            int timeout = rand() % 60 + 30;
#endif
            comet.wheel->add(std::bind(&onTimeout, _1, WeakHttpConnectionPtr_t(conn)), timeout * 1000);
            //conn->send(200);
        } else {
            conn->send(405);
        }
    }
}

TEST_F(CometTest, server) {
    Log::rootLog().setLevel(Log::Error);

    TcpServer s;

    s.setRunCallback(std::bind(&comet::onServerRun, _1));

    HttpServer httpd(&s);

    httpd.setHttpCallback("/", std::bind(&comet::onHandler, _1, _2));

    httpd.listen(Address(11181));

    s.start(8);
}
