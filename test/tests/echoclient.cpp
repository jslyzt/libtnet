#include "ioloop.h"
#include "log.h"
#include "connection.h"
#include "tnet.h"
#include "sockutil.h"
#include "address.h"
#include "../tnet_test.h"

using namespace std;
using namespace std::placeholders;
using namespace tnet;

namespace echoc {
    int i = 0;

    void onConnEvent(const ConnectionPtr_t& conn, ConnEvent event, const void* context) {
        switch (event) {
        case Conn_ReadEvent: {
            const StackBuffer* buffer = (const StackBuffer*)(context);
            LOG_INFO("echo %s", string(buffer->buffer, buffer->count).c_str());

            char buf[1024];
            int n = snprintf(buf, sizeof(buf), "hello world %d", ++i);
            conn->send(string(buf, n));

            //if (++i > 10) {
            //    conn->shutDown();
            //}
        } break;
        case Conn_CloseEvent:
            LOG_INFO("close");
            break;
        case Conn_ErrorEvent:
            LOG_INFO("error");
            break;
        case Conn_ConnectEvent:
            LOG_INFO("connect");
            conn->send("hello world");
            break;
        case Conn_ConnFailEvent:
            LOG_INFO("connect failed");
            exit(0);
            break;
        default:
            break;
        }
    }
}


TEST_F(EchoTest, client) {
    IOLoop loop;

    int fd = SockUtil::create();

    ConnectionPtr_t conn = std::make_shared<Connection>(&loop, fd);

    conn->setEventCallback(std::bind(&echoc::onConnEvent, _1, _2, _3));

    conn->connect(Address("127.0.0.1", 11181));

    loop.start();
}
