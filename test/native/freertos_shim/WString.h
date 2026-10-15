#pragma once
// V1.5.16 host shim: minimal Arduino String (only what the P2 code may need).
#include <string>
#include <string.h>
#include <stdio.h>
class String {
    std::string s_;
public:
    String() {}
    String(const char* c) : s_(c ? c : "") {}
    String(const std::string& s) : s_(s) {}
    String(int v) : s_(std::to_string(v)) {}
    String(unsigned v) : s_(std::to_string(v)) {}
    String(long v) : s_(std::to_string(v)) {}
    String(unsigned long v) : s_(std::to_string(v)) {}
    const char* c_str() const { return s_.c_str(); }
    unsigned int length() const { return (unsigned)s_.size(); }
    bool isEmpty() const { return s_.empty(); }
    String& operator+=(const String& o) { s_ += o.s_; return *this; }
    String& operator+=(const char* c) { s_ += c ? c : ""; return *this; }
    String& operator+=(char c) { s_ += c; return *this; }
    friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
    bool operator==(const String& o) const { return s_ == o.s_; }
    bool operator==(const char* c) const { return s_ == (c ? c : ""); }
    bool operator!=(const String& o) const { return s_ != o.s_; }
    char operator[](unsigned i) const { return i < s_.size() ? s_[i] : 0; }
    bool startsWith(const String& p) const { return s_.rfind(p.s_, 0) == 0; }
    const std::string& std() const { return s_; }
};
