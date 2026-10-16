cmake_minimum_required(VERSION 3.16)
project(native_ui LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "")
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "")
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "")
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "")
add_subdirectory("${ROOT}/firmware/managed_components/lvgl__lvgl" lvgl)
add_executable(preview preview.cpp "${ROOT}/firmware/main/ui.cpp" "${ROOT}/firmware/main/weather_view.cpp" "${ROOT}/firmware/main/assets.cpp" "${ROOT}/firmware/main/character_assets.cpp" "${ROOT}/firmware/main/fonts/ct_font_thai_12.c" "${ROOT}/firmware/main/fonts/ct_font_thai_14.c")
target_include_directories(preview PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${ROOT}/firmware/main" "${ROOT}/tests/backlight_sdk")
target_link_libraries(preview PRIVATE lvgl)

# Keep behavioral assertions enabled in every build type.
target_compile_options(preview PRIVATE -UNDEBUG)
enable_testing()
add_test(NAME ui_preview COMMAND preview)

set_tests_properties(ui_preview PROPERTIES TIMEOUT 30)
