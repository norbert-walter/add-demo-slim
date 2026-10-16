#pragma once
// V1.5.16 host shim of the Arduino FS File (read-only use) and an in-memory SPIFFS.
#include "Arduino.h"
#include <map>
#include <memory>
inline std::map<std::string, std::string>& shimFiles() { static std::map<std::string, std::string> f; return f; }
inline int& shimOpenFiles() { static int n = 0; return n; }
struct ShimFileHandle {                    // shared by all copies of a File, like the real FileImpl
    std::string data; size_t pos = 0; bool open = true;
    explicit ShimFileHandle(const std::string& d) : data(d) { ++shimOpenFiles(); }
    void close() { if (open) { open = false; --shimOpenFiles(); } }
    ~ShimFileHandle() { close(); }           // the last copy going away closes the file
};
class File {
    std::shared_ptr<ShimFileHandle> h_;
public:
    File() {}
    explicit File(const std::string& d) : h_(std::make_shared<ShimFileHandle>(d)) {}
    explicit operator bool() const { return h_ && h_->open; }
    size_t size() const { return h_ ? h_->data.size() : 0; }
    int available() { return (h_ && h_->open) ? (int)(h_->data.size() - h_->pos) : 0; }
    size_t read(uint8_t* buf, size_t n) {
        if (!h_ || !h_->open) return 0;
        size_t k = std::min(n, h_->data.size() - h_->pos);
        memcpy(buf, h_->data.data() + h_->pos, k); h_->pos += k; return k;
    }
    int read() { if (!h_ || !h_->open || h_->pos >= h_->data.size()) return -1; return (uint8_t)h_->data[h_->pos++]; }
    size_t readBytes(char* buf, size_t n) { return read((uint8_t*)buf, n); }
    void close() { if (h_) h_->close(); }
};
class ShimFS {
public:
    bool mount_ok = true;     ///< V1.5.20: result of begin() (set by the test)
    int  begins = 0, format_on_fail = -1;
    bool begin(bool formatOnFail = false) { ++begins; format_on_fail = formatOnFail ? 1 : 0; return mount_ok; }
    bool exists(const char* path) { return shimFiles().count(path) > 0; }
    bool exists(const String& path) { return exists(path.c_str()); }
    File open(const char* path, const char* mode = "r") {
        auto it = shimFiles().find(path); if (it == shimFiles().end()) return File(); return File(it->second);
    }
    File open(const String& path, const char* mode = "r") { return open(path.c_str(), mode); }
};
