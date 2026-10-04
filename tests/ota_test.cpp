#include "../firmware/main/portal.hpp"
#include "../firmware/main/ota_policy.hpp"
#include "../firmware/main/core.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
using namespace deskduo;
struct Device {
    std::vector<unsigned char> image = std::vector<unsigned char>(8192, 0);
    std::vector<unsigned char> written;
    size_t position = 0, chunk = 107;
    uint64_t clock = 0, step = 1;
    int stop = -1, fail = 0, transientTimeouts = 0;
    bool started = false, aborted = false, ended = false, selected = false;
    Device() { image[0]=0xe9; image[1]=3; image[32]=0x32; image[33]=0x54; image[34]=0xcd; image[35]=0xab; image[29]=1; std::memcpy(image.data()+80,"desk_duo",9); }
    uint64_t now() { return clock; }
    int receive(unsigned char* out, size_t n) {
        clock += step;
        if(transientTimeouts>0) {--transientTimeouts;return -2;}
        if (stop >= 0 && position >= size_t(stop)) return fail == 9 ? -2 : 0;
        n=std::min({n, chunk, image.size()-position});
        std::memcpy(out,image.data()+position,n); position+=n; return int(n);
    }
    bool begin(size_t n) { started=true; return n==image.size() && fail!=1; }
    bool write(const unsigned char* p,size_t n) { if(fail==2)return false; written.insert(written.end(),p,p+n);return true; }
    bool end() { ended=true; return fail!=3; }
    void abort() { aborted=true; }
    bool select() { selected=true; return fail!=4; }
};
int main() {
    assert(upload_origin_allowed("http://192.0.2.10","192.0.2.10"));
    for(auto origin:{"http://192.0.2.10.evil.invalid","http://192.0.2.10@evil.invalid","https://192.0.2.10","http://192.0.2.10/","null"})assert(!upload_origin_allowed(origin,"192.0.2.10"));
    assert(upload_origin_allowed("","192.0.2.10"));
    assert(upload_policy(false,"1","application/octet-stream",8192,0x1e0000,"","host")==401);
    assert(!authorized("Bearer 0123456789abcdef",false,"secret-password","0123456789abcdef"));
    assert(authorized("Basic YWRtaW46c2VjcmV0LXBhc3N3b3Jk",false,"secret-password","0123456789abcdef"));
    for(auto h:{"","0","true"})assert(upload_policy(true,h,"application/octet-stream",8192,0x1e0000,"","host")==400);
    assert(upload_policy(true,"1","text/plain",8192,0x1e0000,"","host")==415);
    assert(upload_policy(true,"1","application/octet-stream",0,0x1e0000,"","host")==400);
    assert(upload_policy(true,"1","application/octet-stream",0x1e0001,0x1e0000,"","host")==413);
    assert(upload_policy(true,"1","application/octet-stream",8192,0x1e0000,"http://evil","host")==403);
    assert(upload_policy(true,"1","application/octet-stream",8192,0x1e0000,"http://host","host")==0);
    Maintenance gate; assert(gate.acquire(Maintenance::Settings)); assert(!gate.acquire(Maintenance::Update)); gate.release();
    assert(gate.acquire(Maintenance::Update)); assert(gate.updating()); assert(!gate.acquire(Maintenance::Update)); assert(!gate.acquire(Maintenance::Network)); assert(!gate.acquire(Maintenance::Settings)); gate.release();
    Device good; assert(stream_update(good,8192)==200); assert(good.written==good.image && good.ended && good.selected && !good.aborted);
    for(int bad:{0,1,12,32,80,88}) { Device d; d.image[bad]^=0x80; assert(stream_update(d,8192)==400); assert(!d.started && !d.selected); }
    Device merged; merged.image[0]=0xff; assert(stream_update(merged,8192)==400 && !merged.selected);
    Device shortHeader; shortHeader.stop=50; assert(stream_update(shortHeader,8192)==408 && !shortHeader.started);
    Device cut; cut.stop=1000; assert(stream_update(cut,8192)==408 && cut.aborted && !cut.selected);
    Device idle; idle.stop=1000; idle.fail=9; idle.step=100; assert(stream_update(idle,8192)==408 && idle.aborted && !idle.selected);
    Device total; total.step=2000; assert(stream_update(total,8192)==408 && total.aborted && !total.selected);
    for(int failure:{1,2,3,4}) { Device d; d.fail=failure; assert(stream_update(d,8192)==500); assert(!d.selected || failure==4); if(failure==2)assert(d.aborted); }
    Device retry; retry.chunk=4096; retry.transientTimeouts=3; assert(stream_update(retry,8192)==200);
    std::cout<<"OTA policy and streaming transaction tests passed\n";
}
