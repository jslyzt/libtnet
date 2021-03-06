#include "httprequest.h"

#include <string.h>
#include <stdlib.h>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

extern "C"
{
#include "http_parser.h"
}

#include "httputil.h"
#include "stringutil.h"
#include "log.h"

using namespace std;

namespace tnet {

HttpRequest::HttpRequest() {
    majorVersion = 1;
    minorVersion = 1;
    method = HTTP_GET;
    keepAlive = 0;
}

HttpRequest::~HttpRequest() {
}

void HttpRequest::clear() {
    url.clear();
    schema.clear();
    host.clear();
    path.clear();
    query.clear();
    body.clear();

    headers.clear();
    params.clear();

    majorVersion = 1;
    minorVersion = 1;
    method = HTTP_GET;
    port = 80;
    keepAlive = 0;
}

void HttpRequest::parseUrl() {
    if (!schema.empty()) {
        return;
    }

    struct http_parser_url u;
    if (http_parser_parse_url(url.c_str(), url.size(), 0, &u) != 0) {
        LOG_ERROR("parseurl error %s", url.c_str());
        return;
    }

    if (u.field_set & (1 << UF_SCHEMA)) {
        schema = url.substr(u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len);
    }

    if (u.field_set & (1 << UF_HOST)) {
        host = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
    }

    if (u.field_set & (1 << UF_PORT)) {
        port = u.port;
    } else {
        if (strcasecmp(schema.c_str(), "https") == 0 || strcasecmp(schema.c_str(), "wss") == 0) {
            port = 443;
        } else {
            port = 80;
        }
    }

    if (u.field_set & (1 << UF_PATH)) {
        path = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
    }

    if (u.field_set & (1 << UF_QUERY)) {
        query = url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
        parseQuery();
    }

}

void HttpRequest::parseQuery() {
    if (query.empty()) {
        return;
    }
    parseKeyVal(query);
}

void HttpRequest::parseKeyVal(const std::string& str) {
    if (str.empty() == true) {
        return;
    }

    static string sep1 = "&";
    static string sep2 = "=";

    vector<string> args = StringUtil::split(str, sep1);
    string key;
    string value;
    for (size_t i = 0; i < args.size(); ++i) {
        vector<string> p = StringUtil::split(args[i], sep2);
        if (p.size() == 2) {
            key = p[0];
            value = p[1];
        } else if (p.size() == 1) {
            key = p[0];
            value = "";
        } else {
            continue;
        }
        params.insert(make_pair(HttpUtil::unescape(key), HttpUtil::unescape(value)));
    }
}

void HttpRequest::parseFormData(const std::string& str, const std::string& boundary) {
    const char* ptr = str.c_str(), *p = nullptr;
    size_t size = str.length();
    string key;
    string value;

    ptr = strstr(ptr, "name=");
    while (ptr != nullptr) {
        ptr += 6;
        p = strchr(ptr, '\"');
        key = std::string(ptr, p);
        ptr = p + 1;
        while (*ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
        if (boundary.empty() == true) {
            while (*p != '\r' && *p != '\n') {
                p++;
            }
        } else {
            p = strstr(ptr, boundary.c_str());
            if (p == nullptr) {
                p = str.c_str() + size;
            } else {
                while (p > ptr && (*p == '-' || *p == '\r' || *p == '\n')) {
                    p--;
                }
                if (*p != '-' && *p == '\r' && *p == '\n') {
                    p++;
                }
            }
        }
        params.insert(make_pair(HttpUtil::unescape(key), HttpUtil::unescape(std::string(ptr, p))));
        ptr = strstr(p, "name=");
    }
}

void HttpRequest::parseMFormData(const std::string& str, const std::string& boundary) {
    parseFormData(str, boundary);
}

//--------------------------------------------------------------------------------------------
void addJsonNode(HttpRequest& req, rapidjson::Writer<rapidjson::StringBuffer>& writer,
    rapidjson::StringBuffer& buffer, const std::string& key, rapidjson::Value& val);

void addJsonMember(HttpRequest& req, rapidjson::Writer<rapidjson::StringBuffer>& writer,
    rapidjson::StringBuffer& buffer, const std::string& key, rapidjson::Value& val) {
    buffer.Clear();
    writer.Reset(buffer);
    if (val.Accept(writer) == false) {
        return;
    }
    std::string tmp(buffer.GetString(), buffer.GetSize());
    if (tmp.empty() == false) {
        if ((tmp[0]) == '\"') {
            tmp = tmp.substr(1);
        }
    }
    if (tmp.empty() == false) {
        if ((tmp[tmp.length() - 1]) == '\"') {
            tmp = tmp.substr(0, tmp.length() - 1);
        }
    }
    req.params.insert(make_pair(HttpUtil::unescape(key), HttpUtil::unescape(tmp)));
}

void addJsonArray(HttpRequest& req, rapidjson::Writer<rapidjson::StringBuffer>& writer,
                  rapidjson::StringBuffer& buffer, const std::string& key, rapidjson::Value& val) {
    std::string skey;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
        auto& node = val[i];
        if (key.empty() == true) {
            skey = std::to_string(i);
        } else {
            skey = key + "." + std::to_string(i);
        }
        addJsonNode(req, writer, buffer, skey, node);
    }
}

void addJsonObject(HttpRequest& req, rapidjson::Writer<rapidjson::StringBuffer>& writer,
                   rapidjson::StringBuffer& buffer, const std::string& key, rapidjson::Value& val) {
    std::string skey;
    for (auto iter = val.MemberBegin(); iter != val.MemberEnd(); iter++) {
        if (key.empty() == true) {
            skey = (iter->name).GetString();
        } else {
            skey = key + "." + (iter->name).GetString();
        }
        addJsonNode(req, writer, buffer, skey, iter->value);
    }
}

void addJsonNode(HttpRequest& req, rapidjson::Writer<rapidjson::StringBuffer>& writer,
                 rapidjson::StringBuffer& buffer, const std::string& key, rapidjson::Value& val) {
    if (val.IsObject() == true) {
        addJsonObject(req, writer, buffer, key, val);
    } else if (val.IsArray() == true) {
        addJsonArray(req, writer, buffer, key, val);
    } else {
        addJsonMember(req, writer, buffer, key, val);
    }
}

void HttpRequest::parseJson(const std::string& str) {
    rapidjson::Document doc;
    doc.Parse(str.c_str());
    if (doc.HasParseError() == true) {
        LOG_ERROR("parser json error: %s", doc.GetParseError());
        return;
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    addJsonNode(*this, writer, buffer, "", doc);
}
//--------------------------------------------------------------------------------------------

void HttpRequest::parseHost() {
    auto iter = headers.find("X-Real-Ip");
    if (iter == headers.end()) {
        iter = headers.find("host");
        if (iter == headers.end()) {
            return;
        }
    }
    host = iter->second;
}

void HttpRequest::parseContentType() {
    auto count = headers.count("Content-Type");
    if (count <= 0) {
        return;
    }
    auto iter = headers.find("Content-Type");
    std::string key, boundary;
    for (size_t i = 0; i < count && iter != headers.end(); i++, iter++) {
        auto pos = iter->second.find(';');
        auto size = iter->second.length();
        if (pos != std::string::npos) {
            key = iter->second.substr(0, pos);
            if (pos < size) {
                pos++;
                while (iter->second[pos] != '-' && pos < size) {
                    pos++;
                }
                boundary = iter->second.substr(pos);
            } else {
                boundary.clear();
            }
        } else {
            key = iter->second;
            boundary.clear();
        }
        if (key == "application/form-data") {  // 表单
            parseFormData(body, boundary);
        } else if (key == "application/x-www-form-urlencoded") { // 键值对
            parseKeyVal(body);
        } else if (key == "multipart/form-data") { // 支持文件上传
            parseMFormData(body, boundary);
        } else if (key == "application/json") { // json数据
            parseJson(body);
        }
    }
}

void HttpRequest::parseKeepAlive() {
    auto iter = headers.find("Connection");
    if (iter == headers.end()) {
        return;
    }
    if (iter->second == "close") {
        keepAlive = -1;
    } else if (iter->second == "keep-alive") {
        auto kiter = headers.find("Keep-Alive");
        if (kiter == headers.end()) {
            keepAlive = 0;
        } else {
            keepAlive = std::stoi(iter->second);
        }
    }
}

void HttpRequest::parseBody() {
    parseHost();
    parseContentType();
    parseKeepAlive();
}

static const string HostKey = "Host";
static const string ContentLengthKey = "Content-Length";

string HttpRequest::dump() {
    string str;

    parseUrl();

    char buf[1024];

    int n = 0;
    if (path.empty()) {
        path = "/";
    }

    if (query.empty()) {
        n = snprintf(buf, sizeof(buf), "%s %s HTTP/%d.%d\r\n",
                     http_method_str(method), path.c_str(), majorVersion, minorVersion);
    } else {
        n = snprintf(buf, sizeof(buf), "%s %s?%s HTTP/%d.%d\r\n",
                     http_method_str(method), path.c_str(), query.c_str(), majorVersion, minorVersion);
    }

    str.append(buf, n);

    headers.erase(HostKey);

    if (port == 80 || port == 443) {
        headers.insert(make_pair(HostKey, host));
    } else {
        n = snprintf(buf, sizeof(buf), "%s:%d", host.c_str(), port);
        headers.insert(make_pair(HostKey, string(buf, n)));
    }

    if (method == HTTP_POST || method == HTTP_PUT) {
        headers.erase(ContentLengthKey);

        n = snprintf(buf, sizeof(buf), "%d", int(body.size()));
        headers.insert(make_pair(ContentLengthKey, string(buf, n)));
    }

    auto iter = headers.cbegin();
    while (iter != headers.cend()) {
        int n = snprintf(buf, sizeof(buf), "%s: %s\r\n", iter->first.c_str(), iter->second.c_str());
        str.append(buf, n);
        ++iter;
    }

    str.append("\r\n");

    str.append(body);

    return str;
}
}
