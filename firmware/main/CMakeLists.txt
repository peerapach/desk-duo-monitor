idf_component_register(SRCS "main.cpp" "core.cpp" "runtime.cpp" "display.cpp" "input.cpp" "network.cpp" "portal.cpp" "ui.cpp" "assets.cpp" "fonts/ct_font_thai_12.c" "fonts/ct_font_thai_14.c"
    INCLUDE_DIRS "." REQUIRES bootloader_support json nvs_flash esp_wifi esp_event esp_netif esp_timer esp_http_server esp_http_client esp-tls tcp_transport lwip esp_driver_gpio esp_driver_spi lvgl)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
