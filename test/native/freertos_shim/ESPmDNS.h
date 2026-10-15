#pragma once
// V1.5.16 host shim of ESPmDNS (MDNSResponder).
#include "Arduino.h"
struct ShimMdns { std::string host; std::string service, proto; uint16_t port = 0; int begins = 0, services = 0; };
inline ShimMdns& shimMdns() { static ShimMdns m; return m; }
class MDNSResponder {
public:
    bool begin(const char* host) { shimMdns().host = host; ++shimMdns().begins; return true; }
    bool addService(const char* service, const char* proto, uint16_t port) {
        ShimMdns& m = shimMdns(); m.service = service; m.proto = proto; m.port = port; ++m.services; return true;
    }
    void end() {}
};
inline MDNSResponder MDNS;
