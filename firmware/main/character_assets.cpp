#include "character_assets.hpp"
namespace deskduo {
namespace {
#include "generated/character_pixels.inc"
#include "generated/weather_pixels.inc"
constexpr lv_image_dsc_t descriptor(const uint8_t* data, unsigned size) {
    return {{LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_RGB565, 0, size, size, size * 2, 0},
            size * size * 2, data, nullptr};
}
const lv_image_dsc_t images[2][2] = {
    {descriptor(claude_64, 64), descriptor(claude_32, 32)},
    {descriptor(codex_64, 64), descriptor(codex_32, 32)}
};
constexpr lv_image_dsc_t navDescriptor(const uint8_t* data) {
    return {{LV_IMAGE_HEADER_MAGIC,LV_COLOR_FORMAT_RGB565,0,24,22,48,0},24*22*2,data,nullptr};
}
const lv_image_dsc_t navigationImages[4][2] = {
    {navDescriptor(navigation_0_0),navDescriptor(navigation_0_1)},
    {navDescriptor(navigation_1_0),navDescriptor(navigation_1_1)},
    {navDescriptor(navigation_2_0),navDescriptor(navigation_2_1)},
    {navDescriptor(navigation_3_0),navDescriptor(navigation_3_1)}
};
const lv_image_dsc_t weatherImages[6][2] = {
    {descriptor(weather_0_96,96),descriptor(weather_0_28,28)},
    {descriptor(weather_1_96,96),descriptor(weather_1_28,28)},
    {descriptor(weather_2_96,96),descriptor(weather_2_28,28)},
    {descriptor(weather_3_96,96),descriptor(weather_3_28,28)},
    {descriptor(weather_4_96,96),descriptor(weather_4_28,28)},
    {descriptor(weather_5_96,96),descriptor(weather_5_28,28)}
};
}
const lv_image_dsc_t* character_image(unsigned provider, bool mini) {
    return provider < 2 ? &images[provider][mini ? 1 : 0] : nullptr;
}
const lv_image_dsc_t* navigation_image(unsigned tab,bool active) {
    return tab<4?&navigationImages[tab][active?1:0]:nullptr;
}
const lv_image_dsc_t* weather_image(std::optional<int> code,std::optional<bool> day,bool mini) {
    if(!code)return nullptr;
    int tile=-1;
    switch(*code) {
        case 0:case 1:if(day)tile=*day?0:2;break;
        case 2:tile=day?(*day?1:3):4;break;
        case 3:case 45:case 48:tile=4;break;
        case 51:case 53:case 55:case 56:case 57:case 61:case 63:case 65:case 66:case 67:
        case 71:case 73:case 75:case 77:case 80:case 81:case 82:case 85:case 86:
        case 95:case 96:case 97:case 99:tile=5;break;
        default:break;
    }
    return tile<0?nullptr:&weatherImages[tile][mini?1:0];
}

}
