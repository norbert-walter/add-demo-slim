#pragma once
// V1.5.16 host shim of the Arduino-ESP32 WebServer (core 3.x signatures for the calls used in
// src/comm/http_server.cpp). Requests are queued by the test and dispatched in handleClient();
// each response is recorded (code, content type, headers, body incl. chunked sendContent).
#include "Arduino.h"
#include "WString.h"
#include "FS.h"
#include <functional>
#include <vector>
#include <strings.h>   // strcasecmp
typedef enum { HTTP_ANY = 0, HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS } HTTPMethod;
#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
// V1.5.25: the members of the real WebServer used by AddWebServer::dropIdleClient (protected there).
enum HTTPClientStatus { HC_NONE, HC_WAIT_READ, HC_WAIT_CLOSE };
inline int& shimClientStops() { static int n = 0; return n; }
class NetworkClient {
public:
    bool shim_avail = false;
    int available() { return shim_avail ? 1 : 0; }
    void stop() { ++shimClientStops(); }
};
struct ShimRequest { HTTPMethod method; std::string uri; std::string body; bool has_body;
                     std::vector<std::pair<std::string, std::string>> headers; };   // V1.6.5: request headers
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
    std::vector<std::string> collect_;     // V1.6.5: request headers to collect (collectHeaders)
    size_t content_len_ = 0;
protected:
    // V1.5.25: like the real server: one client at a time; a connected client that has sent nothing
    // blocks every other request until it sends, is dropped, or HTTP_MAX_DATA_WAIT (5 s) has passed.
    HTTPClientStatus _currentStatus = HC_NONE;
    unsigned long _statusChange = 0;
    NetworkClient _currentClient;
public:
    std::vector<ShimResponse> responses;
    void shimIdleClient(unsigned long since, bool has_data = false) {
        _currentStatus = HC_WAIT_READ; _statusChange = since; _currentClient = NetworkClient(); _currentClient.shim_avail = has_data;
    }
    bool shimClientWaiting() const { return _currentStatus == HC_WAIT_READ; }
    int handle_calls = 0;
    explicit WebServer(int port = 80) : port_(port) { shimServer() = this; }
    int port() const { return port_; }
    bool begun() const { return begun_; }
    void begin() { begun_ = true; }
    bool collected_before_begin = false;    // V1.6.5: collectHeaders() was called before begin()
    // V1.6.5: like the real server: only collected request headers are available (else "").
    void collectHeaders(const char* keys[], const size_t n) {
        collect_.clear(); for (size_t i = 0; i < n; ++i) collect_.push_back(keys[i]);
        collected_before_begin = !begun_;
    }
    String header(const String& name) const {
        bool known = false;
        for (auto& k : collect_) if (strcasecmp(k.c_str(), name.c_str()) == 0) known = true;
        if (!known) return String();
        for (auto& h : cur_.headers) if (strcasecmp(h.first.c_str(), name.c_str()) == 0) return String(h.second);
        return String();
    }
    bool hasHeader(const String& name) const { return header(name).length() > 0; }
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
        // V1.6.5: like WebServer::_streamFileCore of the ESP32 core: a file name ending in ".gz" gets
        // Content-Encoding: gzip automatically (unless the type is application/x-gzip).
        std::string fn = file.name();
        if (fn.size() >= 3 && fn.compare(fn.size() - 3, 3, ".gz") == 0 && type.std() != "application/x-gzip")
            hdr_.push_back({"Content-Encoding", "gzip"});
        ShimResponse r; r.code = code; r.type = type.std(); r.headers = hdr_; hdr_.clear(); r.sends = 1;
        uint8_t b[64]; size_t n, total = 0;
        while ((n = file.read(b, sizeof b)) > 0) { r.body.append((const char*)b, n); total += n; }
        responses.push_back(r); return total;
    }
    void handleClient() {
        ++handle_calls;
        if (_currentStatus == HC_WAIT_READ) {
            if (_currentClient.available()) _currentStatus = HC_NONE;          // its request: not modelled
            else if (millis() - _statusChange > 5000) _currentStatus = HC_NONE; // HTTP_MAX_DATA_WAIT
            else return;                                                       // everybody else waits
        }
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
    void shimQueue(HTTPMethod m, const std::string& uri, const std::string* body = nullptr,
                   std::vector<std::pair<std::string, std::string>> headers = {}) {
        pending_.push_back({m, uri, body ? *body : std::string(), body != nullptr, headers});
    }
private:
    void sendRaw(int code, const char* type, const std::string& content) {
        ShimResponse r; r.code = code; r.type = type ? type : ""; r.body = content; r.headers = hdr_; hdr_.clear();
        r.sends = 1; r.chunked = (content_len_ == CONTENT_LENGTH_UNKNOWN);
        responses.push_back(r);
    }
};
