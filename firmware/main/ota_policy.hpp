#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
namespace deskduo {
// Nonblocking ownership, not a mutex: no thread waits while another does I/O.
// Every station reconfiguration and settings commit must acquire this gate.
class Maintenance {
public:
    enum Owner { Idle, Settings, Network, Update };
    bool acquire(Owner owner) { Owner empty=Idle; return owner!=Idle && owner_.compare_exchange_strong(empty,owner); }
    void release() { owner_.store(Idle); }
    bool updating() const { return owner_.load()==Update; }
private:
    std::atomic<Owner> owner_{Idle};
};
inline Maintenance maintenance;
class MaintenanceLease {
public:
    explicit MaintenanceLease(Maintenance::Owner owner): held_(maintenance.acquire(owner)) {}
    ~MaintenanceLease() { if(held_)maintenance.release(); }
    explicit operator bool() const { return held_; }
    MaintenanceLease(const MaintenanceLease&)=delete;
    MaintenanceLease& operator=(const MaintenanceLease&)=delete;
private:
    bool held_;
};
inline bool upload_origin_allowed(const std::string& origin,const std::string& host) {
    if(origin.empty())return true; // Authenticated CLI clients do not send Origin.
    if(host.empty() || host.find_first_of("/@\\ \t\r\n?#")!=std::string::npos)return false;
    return origin=="http://"+host;
}
inline int upload_policy(bool admin,const std::string& header,const std::string& media,size_t bytes,size_t capacity,const std::string& origin,const std::string& host) {
    if(!admin)return 401;
    if(header!="1")return 400;
    if(!upload_origin_allowed(origin,host))return 403;
    if(media!="application/octet-stream")return 415;
    if(bytes==0)return 400;
    if(bytes>capacity)return 413;
    return 0;
}
inline uint32_t image_u32(const unsigned char* p) { return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24; }
inline bool upload_image_allowed(const unsigned char* p,size_t length,size_t expected) {
    // ESP image header (24), first segment header (8), esp_app_desc_t (256).
    if(length<288 || expected<288 || p[0]!=0xe9 || p[1]==0 || p[1]>16 || p[12]!=0 || p[13]!=0)return false;
    const auto firstLength=image_u32(p+28);
    if(firstLength<256 || firstLength>expected-32)return false;
    return image_u32(p+32)==0xabcd5432 && std::memcmp(p+80,"desk_duo\0",9)==0;
}
// IO is the thin SDK adapter in ota.cpp. Tests exercise this entire production
// receive/validate/flash transaction with only socket/flash boundary substitutes.
// receive: positive byte count, -2 retryable socket timeout, otherwise closed.
template<class IO> int stream_update(IO& io,size_t expected) {
    std::array<unsigned char,4096> buffer{};
    size_t received=0, prefix=0;
    bool active=false;
    const uint64_t start=io.now(); uint64_t last=start;
    const auto fail=[&](int code) { if(active)io.abort();return code; };
    while(received<expected) {
        if(io.now()-start>=120000 || io.now()-last>=10000)return fail(408);
        const size_t wanted=std::min(expected-received, active?buffer.size():size_t(288)-prefix);
        if(wanted==0)return fail(400);
        const int count=io.receive(buffer.data()+(active?0:prefix),wanted);
        if(io.now()-start>=120000 || io.now()-last>=10000)return fail(408);
        if(count==-2)continue;
        if(count<=0 || size_t(count)>wanted)return fail(408);
        last=io.now(); received+=count;
        if(!active) {
            prefix+=count;
            if(prefix<288)continue;
            if(!upload_image_allowed(buffer.data(),prefix,expected))return 400;
            if(!io.begin(expected))return 500;
            active=true;
            if(!io.write(buffer.data(),prefix))return fail(500);
        } else if(!io.write(buffer.data(),count))return fail(500);
    }
    if(!active)return 400;
    // esp_ota_end consumes its handle on success AND failure. Never abort a
    // freed handle. End's image/checksum validation precedes any boot selection.
    active=false;
    if(!io.end())return 500;
    // SDK end validates the partition, including bytes retained beyond the
    // erased upload range. A valid old suffix must never complete a short body.
    // Metadata length includes image padding/checksum and appended digest.
    if(io.image_size()!=received)return 400;
    return io.select()?200:500;
}
}
