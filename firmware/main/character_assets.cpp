#include "character_assets.hpp"
namespace deskduo {
namespace {
#include "generated/character_pixels.inc"
constexpr lv_image_dsc_t descriptor(const uint8_t* data, unsigned size) {
    return {{LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_RGB565, 0, size, size, size * 2, 0},
            size * size * 2, data, nullptr};
}
const lv_image_dsc_t images[2][2] = {
    {descriptor(claude_64, 64), descriptor(claude_32, 32)},
    {descriptor(codex_64, 64), descriptor(codex_32, 32)}
};
}
const lv_image_dsc_t* character_image(unsigned provider, bool mini) {
    return provider < 2 ? &images[provider][mini ? 1 : 0] : nullptr;
}
}
