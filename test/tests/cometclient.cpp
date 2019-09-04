#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "log.h"
#include "timingwheel.h"
#include "httpclient.h"

#include "httprequest.h"
#include "httpresponse.h"
#include "ioloop.h"
#include "../tnet_test.h"

using namespace std;
using namespace std::placeholders;
using namespace tnet;

namespace comet {
    string url = "http://127.0.0.1:11181/";

    void onResponse(const HttpClientPtr_t& client, const HttpResponse& resp) {
        if (resp.statusCode != 200) {
            cout << "resp error:" << resp.statusCode << ", body: " << resp.body << endl;
            return;
        }
        cout << "resp code:" << resp.statusCode << ", body: " << resp.body << endl;
    }

    void request(const TimingWheelPtr_t& wheel, const HttpClientPtr_t& client, int num) {
        for (int i = 0; i < num; ++i) {
            client->request(url, std::bind(&onResponse, client, _1));
        }
    }
}


TEST_F(CometTest, client) {
    vector<HttpClientPtr_t> clients;
    IOLoop loop;
    clients.push_back(std::make_shared<HttpClient>(&loop));

    TimingWheelPtr_t wheel = std::make_shared<TimingWheel>(1000, 3600);

    for (int i = 0; i < 10; ++i) {
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            wheel->add(std::bind(&comet::request, _1, *it, 5), i * 10);
        }
    }

    wheel->start(&loop);
    loop.start();
}
