#include <iostream>

#include "httpclient.h"

#include "httprequest.h"
#include "httpresponse.h"
#include "ioloop.h"
#include "../tnet_test.h"

using namespace std;
using namespace tnet;

void onResponse(IOLoop* loop, const HttpResponse& resp) {
    cout << resp.statusCode << endl;

    Headers_t::const_iterator iter = resp.headers.begin();

    while (iter != resp.headers.end()) {
        cout << iter->first << ": " << iter->second << endl;
        ++iter;
    }

    cout << resp.body.size() << endl << endl;

    loop->stop();
}

TEST_F(HttpTest, client) {
    IOLoop loop;

    HttpClientPtr_t client = std::make_shared<HttpClient>(&loop);

    client->request("http://127.0.0.1:11181/abc", std::bind(&onResponse, &loop, _1));

    loop.start();

    cout << "exit" << endl;
}
