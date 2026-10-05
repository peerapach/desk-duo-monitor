#pragma once
#include <cstddef>
constexpr unsigned MALLOC_CAP_INTERNAL=1, MALLOC_CAP_8BIT=2;
size_t heap_caps_get_free_size(unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
