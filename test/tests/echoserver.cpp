#include <string>

#include "tnet.h"
#include "tcpserver.h"
#include "log.h"
#include "connection.h"
#include "address.h"
#include "../tnet_test.h"

using namespace std;
using namespace std::placeholders;
using namespace tnet;

namespace echos {
    void onConnEvent(const ConnectionPtr_t& conn, ConnEvent event, const void* context) {
        switch (event) {
        case Conn_ReadEvent: {
            const StackBuffer* buffer = static_cast<const StackBuffer*>(context);
            conn->send(string(buffer->buffer, buffer->count));
        } break;
        default:
            break;
        }
    }
}

TEST_F(EchoTest, server) {
    TcpServer s;
    s.listen(Address(11181), std::bind(&echos::onConnEvent, _1, _2, _3));

    s.start(1);

    LOG_INFO("test over");
}
