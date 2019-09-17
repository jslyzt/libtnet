#include <vector>
#include <iostream>
#include <signal.h>

#include "signaler.h"
#include "ioloop.h"
#include "tnet.h"
#include "../tnet_test.h"

using namespace tnet;
using namespace std::placeholders;
using namespace std;

namespace sig {
    void onSignaler(const SignalerPtr_t& signaler, int signum) {
        cout << "signal:" << signum << endl;
        IOLoop* loop = signaler->loop();
        switch (signum) {
        case SIGINT:
        case SIGTERM:
            loop->stop();
            break;
        default:
            break;
        }
    }
}


TEST_F(SignalTest, test) {
    IOLoop loop;

    vector<int> signums{SIGINT, SIGTERM};
    SignalerPtr_t signaler = std::make_shared<Signaler>(signums, std::bind(&sig::onSignaler, _1, _2));

    signaler->start(&loop);

    cout << "start" << endl;

    loop.start();

    cout << "end" << endl;
}
