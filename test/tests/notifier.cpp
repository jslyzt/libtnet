#include <iostream>

#include "notifier.h"
#include "ioloop.h"
#include "timer.h"
#include "tnet.h"
#include "../tnet_test.h"

using namespace std;
using namespace std::placeholders;
using namespace tnet;

namespace notify {

    void onNotify(const NotifierPtr_t& notifier) {
        cout << "on notify" << endl;

        IOLoop* loop = notifier->loop();
        notifier->stop();
        loop->stop();
    }

    void onTimer(const TimerPtr_t& timer, const NotifierPtr_t& notifier) {
        cout << "begin to notify" << endl;
        notifier->notify();
    }

    void run(IOLoop* loop) {
        NotifierPtr_t notifier = std::make_shared<Notifier>(std::bind(&onNotify, _1));
        TimerPtr_t timer = std::make_shared<Timer>(std::bind(&onTimer, _1, notifier), 0, 5000);
        notifier->start(loop);
        timer->start(loop);
    }
}

TEST_F(NotifyTest, test) {
    IOLoop loop;

    notify::run(&loop);
    cout << "start" << endl;

    loop.start();

    cout << "stop" << endl;
}
