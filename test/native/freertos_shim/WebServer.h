#pragma once
// V1.5.16 host shim of the Arduino-ESP32 WebServer (core 3.x signatures for the calls used in
// src/comm/http_server.cpp). Requests are queued by the test and dispatched in handleClient();
// each response is recorded (code, content type, headers, body incl. chunked sendContent).
#include "Arduino.h"
#include "WString.h"
#include "FS.h"
#include <functional>
#include <vector>
typedef enum { HTTP_ANY = 0, HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS } HTTPMethod;
#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
struct ShimRequest { HTTPMethod method; std::string uri; std::string body; bool has_body; };
struct ShimResponse {
    int code = 0; std::string type; std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    int sends = 0; bool chunked = false; bool chunk_end = false;
    int chunks = 0;                       // V1.5.17: non-empty sendContent calls (= HTTP chunks / TCP writes)
    std::string header(const std::string& n) const { for (auto& h : headers) if (h.first == n) return h.second; return "<none>"; }
};
class WebServer;
inline WebServer*& shimServer() { static WebServer* s = nullptr; return s; }
class WebServer {
public:
    typedef std::function<void(void)> THandlerFunction;
private:
    struct Route { std::string uri; HTTPMethod method; THandlerFunction fn; };
    std::vector<Route> routes_; THandlerFunction notFound_; int port_; bool begun_ = false;
    std::vector<ShimRequest> pending_; ShimRequest cur_{}; std::vector<std::pair<std::string,std::string>> hdr_;
    size_t content_len_ = 0;
public:
    std::vector<ShimResponse> responses;
    int handle_calls = 0;
    explicit WebServer(int port = 80) : port_(port) { shimServer() = this; }
    int port() const { return port_; }
    bool begun() const { return begun_; }
    void begin() { begun_ = true; }
    void on(const String& uri, HTTPMethod m, THandlerFunction fn) { routes_.push_back({uri.std(), m, fn}); }
    void on(const String& uri, THandlerFunction fn) { routes_.push_back({uri.std(), HTTP_ANY, fn}); }
    void onNotFound(THandlerFunction fn) { notFound_ = fn; }
    // ---- request side
    String uri() const { return String(cur_.uri); }
    HTTPMethod method() const { return cur_.method; }
    bool hasArg(const String& n) const { return n == "plain" && cur_.has_body; }
    String arg(const String& n) const { return (n == "plain" && cur_.has_body) ? String(cur_.body) : String(); }
    // ---- response side
    void sendHeader(const String& n, const String& v, bool first = false) { hdr_.push_back({n.std(), v.std()}); }
    void setContentLength(const size_t len) { content_len_ = len; }
    void send(int code, const char* type, const String& content) { sendRaw(code, type, content.std()); }
    void send(int code, const char* type, const char* content) { sendRaw(code, type, content ? content : ""); }
    void send(int code, const String& type, const String& content) { sendRaw(code, type.c_str(), content.std()); }
    void send(int code) { sendRaw(code, "", ""); }
    void sendContent(const String& c) { sendContent(c.c_str(), c.length()); }
    void sendContent(const char* c, size_t n) {
        if (responses.empty()) return;
        ShimResponse& r = responses.back();
        if (n == 0) { r.chunk_end = true; return; }
        ++r.chunks;
        r.body.append(c, n);
    }
    void sendContent(const char* c) { sendContent(c, strlen(c)); }
    template <typename T> size_t streamFile(T& file, const String& type, const int code = 200) {
        ShimResponse r; r.code = code; r.type = type.std(); r.headers = hdr_; hdr_.clear(); r.sends = 1;
        uint8_t b[64]; size_t n, total = 0;
        while ((n = file.read(b, sizeof b)) > 0) { r.body.append((const char*)b, n); total += n; }
        responses.push_back(r); return total;
    }
    void handleClient() {
        ++handle_calls;
        std::vector<ShimRequest> p; p.swap(pending_);
        for (auto& q : p) {
            cur_ = q; hdr_.clear(); content_len_ = 0;
            size_t before = responses.size();
            bool done = false;
            for (auto& rt : routes_)
                if (rt.uri == q.uri && (rt.method == HTTP_ANY || rt.method == q.method)) { rt.fn(); done = true; break; }
            if (!done && notFound_) notFound_();
            if (responses.size() == before) { ShimResponse r; r.code = -1; responses.push_back(r); }  // no answer
            else if (responses.size() > before + 1) responses.back().sends = (int)(responses.size() - before);
        }
    }
    // test helper
    void shimQueue(HTTPMethod m, const std::string& uri, const std::string* body = nullptr) {
        pending_.push_back({m, uri, body ? *body : std::string(), body != nullptr});
    }
private:
    void sendRaw(int code, const char* type, const std::string& content) {
        ShimResponse r; r.code = code; r.type = type ? type : ""; r.body = content; r.headers = hdr_; hdr_.clear();
        r.sends = 1; r.chunked = (content_len_ == CONTENT_LENGTH_UNKNOWN);
        responses.push_back(r);
    }
};
