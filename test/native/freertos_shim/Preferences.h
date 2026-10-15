#pragma once
// V1.5.14 host shim of the Arduino-ESP32 Preferences class (same signatures and return values
// as the real one for the calls used in src/system/nvs_config.cpp). In-memory store with
// type tags; fault injection for tests.
#include "shim_core.h"
#include <map>
#include <math.h>
#include <string>

struct ShimNvsEntry { char type; std::string bytes; };   // type: 'u' uchar, 'f' float, 's' string
struct ShimNvs {
    std::map<std::string, std::map<std::string, ShimNvsEntry>> ns;
    bool fail_begin = false;          ///< begin() returns false
    std::string fail_put_key;         ///< put*() of this key returns 0
    int begins = 0, ends = 0, open = 0, misuse = 0;
    bool last_read_only = false;
};
inline ShimNvs& shimNvs() { static ShimNvs s; return s; }

class Preferences {
    std::string n_; bool ro_ = true; bool started_ = false;
    std::map<std::string, ShimNvsEntry>* t_ = nullptr;
    const ShimNvsEntry* find(const char* k, char type) const {
        if (!started_ || !t_) return nullptr;
        auto it = t_->find(k);
        if (it == t_->end() || it->second.type != type) return nullptr;
        return &it->second;
    }
    size_t put(const char* k, char type, const void* p, size_t n, size_t ret) {
        ShimNvs& s = shimNvs();
        if (!started_ || ro_) { ++s.misuse; return 0; }
        if (!k || strlen(k) > 15) return 0;
        if (s.fail_put_key == k) return 0;
        (*t_)[k] = ShimNvsEntry{type, std::string((const char*)p, n)};
        return ret;
    }
public:
    bool begin(const char* name, bool readOnly = false, const char* partition_label = nullptr) {
        ShimNvs& s = shimNvs();
        if (started_) { ++s.misuse; return false; }
        ++s.begins; s.last_read_only = readOnly;
        if (s.fail_begin) return false;
        if (readOnly && !s.ns.count(name)) return false;     // like nvs_open(NVS_READONLY)
        n_ = name; ro_ = readOnly; started_ = true; t_ = &s.ns[name]; ++s.open;
        return true;
    }
    void end() { if (started_) { started_ = false; ++shimNvs().ends; --shimNvs().open; } }
    bool clear() { if (!started_ || ro_) { ++shimNvs().misuse; return false; } t_->clear(); return true; }
    bool remove(const char* k) { if (!started_ || ro_) return false; return t_->erase(k) > 0; }
    bool isKey(const char* k) { return started_ && t_ && t_->count(k); }
    size_t putUChar(const char* k, uint8_t v) { return put(k, 'u', &v, 1, 1); }
    size_t putBool(const char* k, bool v) { return putUChar(k, v ? 1 : 0); }
    size_t putFloat(const char* k, float v) { return put(k, 'f', &v, sizeof v, sizeof v); }
    size_t putString(const char* k, const char* v) { return put(k, 's', v, strlen(v) + 1, strlen(v)); }
    uint8_t getUChar(const char* k, uint8_t d = 0) { auto e = find(k, 'u'); return e ? (uint8_t)e->bytes[0] : d; }
    bool getBool(const char* k, bool d = false) { return getUChar(k, d ? 1 : 0) == 1; }
    float getFloat(const char* k, float d = NAN) {
        auto e = find(k, 'f'); if (!e) return d; float v; memcpy(&v, e->bytes.data(), sizeof v); return v;
    }
    /// Like the real one: returns the length INCLUDING the NUL, 0 if missing or longer than maxLen
    /// (value untouched in that case).
    size_t getString(const char* k, char* value, size_t maxLen) {
        auto e = find(k, 's'); if (!e) return 0;
        size_t len = e->bytes.size(); if (len > maxLen) return 0;
        memcpy(value, e->bytes.data(), len); return len;
    }
};

// test helpers
inline void shimNvsReset() { shimNvs() = ShimNvs{}; }
inline void shimNvsPutRaw(const char* ns, const char* k, char type, const void* p, size_t n) {
    shimNvs().ns[ns][k] = ShimNvsEntry{type, std::string((const char*)p, n)};
}
