#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
// Unsigned ESP image: two segments, app descriptor in first, XOR checksum at
// the final byte. Its last sector can remain valid after a 4096-byte upload.
inline std::vector<unsigned char> ota_image_fixture() {
    std::vector<unsigned char> bytes(8192, 0);
    bytes[0]=0xe9; bytes[1]=2;
    bytes[29]=1; // First segment length 256.
    bytes[32]=0x32; bytes[33]=0x54; bytes[34]=0xcd; bytes[35]=0xab;
    std::memcpy(bytes.data()+80,"desk_duo",9);
    bytes[292]=0xc8; bytes[293]=0x1e; // Second segment length 7880.
    for(size_t i=296;i<8176;++i)bytes[i]=static_cast<unsigned char>(i);
    unsigned char checksum=0xef;
    for(size_t i=32;i<288;++i)checksum^=bytes[i];
    for(size_t i=296;i<8176;++i)checksum^=bytes[i];
    bytes.back()=checksum;
    return bytes;
}
// SDK boundary substitute parses segment lengths and validates checksum from
// the entire flash slot, independently of how many upload bytes were written.
inline size_t ota_fixture_verified_size(const std::vector<unsigned char>& flash) {
    if(flash.size()<24 || flash[0]!=0xe9 || flash[1]!=2)return 0;
    size_t position=24;
    unsigned char checksum=0xef;
    for(unsigned i=0;i<flash[1];++i) {
        if(position+8>flash.size())return 0;
        size_t length=uint32_t(flash[position+4]) | uint32_t(flash[position+5])<<8 |
                      uint32_t(flash[position+6])<<16 | uint32_t(flash[position+7])<<24;
        position+=8;
        if(length>flash.size()-position)return 0;
        for(size_t j=0;j<length;++j)checksum^=flash[position+j];
        position+=length;
    }
    const size_t end=(position+16)&~size_t(15);
    return end<=flash.size() && flash[end-1]==checksum?end:0;
}
