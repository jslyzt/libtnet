#pragma once

#include "tnet_http.h"
#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpconnection.h"

namespace tnet {

struct callbackNode {
    HttpCallback_t call;
    std::string name;
};
typedef std::map<std::string, callbackNode> callbackNodes;

typedef void(*hdl_func)(HttpConnectionPtr_t& conn, const HttpRequest& request);

class _hdl_reg {
public:
    _hdl_reg(callbackNodes* funcs, const char* fname, const char* key, hdl_func fn);
    ~_hdl_reg();
};

callbackNodes* getDefcbNodes();
void registHandl(tnet::HttpServer& s, callbackNodes* funcs = nullptr);

}

#define HANDLEDEFINEWITHCB(cb, name, path) \
    void hdl_##name(tnet::HttpConnectionPtr_t& conn, const tnet::HttpRequest& request); \
    static tnet::_hdl_reg hdl_reg_##name(cb, #name, #path, hdl_##name); \
    void hdl_##name(tnet::HttpConnectionPtr_t& conn, const tnet::HttpRequest& request)

#define HANDLEDEFINE(name, path) \
    HANDLEDEFINEWITHCB(nullptr, name, path)
