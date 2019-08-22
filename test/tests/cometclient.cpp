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
using namespace tnet;

string url = "http://10.20.187.120:11181/";

void onResponse(const HttpClientPtr_t& client, const HttpResponse& resp) {
    if (resp.statusCode != 200) {
        cout << "request error:" << resp.statusCode << "\t" << resp.body << endl;
        return;
    }

    client->request(url, std::bind(&onResponse, client, _1));
}

void request(const TimingWheelPtr_t& wheel, const HttpClientPtr_t& client, int num) {
    for (int i = 0; i < num; ++i) {
        client->request(url, std::bind(&onResponse, client, _1));
    }
}

TEST_F(CometTest, client) {
    Log::rootLog().setLevel(Log::Error);

    int num = 10;
    vector<HttpClientPtr_t> clients;
    IOLoop loop;
    clients.push_back(std::make_shared<HttpClient>(&loop));

    cout << "request num:" << num << endl;

    TimingWheelPtr_t wheel = std::make_shared<TimingWheel>(1000, 3600);

    int c = 60 * (int)clients.size();
    for (int i = 0; i < c; ++i) {
        int reqNum = num / c;
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            wheel->add(std::bind(&request, _1, *it, reqNum), i * 1000);
        }
    }

    wheel->start(&loop);
    loop.start();
}
