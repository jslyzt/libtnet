#include "httphandler.h"
#include "../log.h"

using namespace std::placeholders;

namespace tnet {

static callbackNodes* hdl_funcs = nullptr;

_hdl_reg::_hdl_reg(callbackNodes* funcs, const char* fname, const char* key, hdl_func fn) {
    if (key == nullptr || strlen(key) <= 0) {
        return;
    }
    std::string path(key);
    if (path[0] == '\"') {
        path = path.substr(1);
    }
    if (path.length() > 0 && path[path.length() - 1] == '\"') {
        path = path.substr(0, path.length() - 1);
    }
    if (path[0] != '/') {
        path = "/" + path;
    }
    if (funcs == nullptr) {
        if (hdl_funcs == nullptr) {
            hdl_funcs = new callbackNodes();
        }
        funcs = hdl_funcs;
    }
    auto iter = funcs->find(path);
    if (iter == funcs->end()) {
        (*funcs)[path] = callbackNode{
            std::bind(fn, _1, _2),
            "hdl_" + std::string(fname),
        };
    } else {
        LOG_ERROR("handl: %s regist has func", path.c_str());
    }
}

_hdl_reg::~_hdl_reg() {
}

callbackNodes* getDefcbNodes() {
    return hdl_funcs;
}

void registHandl(HttpServer& s, callbackNodes* funcs) {
    if (funcs == nullptr) {
        funcs = hdl_funcs;
    }
    if (funcs == nullptr) {
        return;
    }
    for (auto it : *funcs) {
        s.setHttpCallback(it.first, it.second.call);
        LOG_FATAL("%s => %s", it.first.c_str(), it.second.name.c_str());
    }
}

}