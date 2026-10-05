#pragma once
using esp_err_t=int;
constexpr int ESP_OK=0;
constexpr int ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET=0x8002;
constexpr int ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST=0x8004;
constexpr int ESP_ERR_ESP_TLS_SOCKET_SETOPT_FAILED=0x8005;
struct esp_tls_last_error { int esp,tls,flags; };
using esp_tls_error_handle_t=esp_tls_last_error*;
struct esp_tls_t { bool alive=true; esp_tls_last_error errors{0,0,0}; };
esp_err_t esp_tls_get_error_handle(esp_tls_t*,esp_tls_error_handle_t*);
esp_err_t esp_tls_get_and_clear_last_error(esp_tls_error_handle_t,int*,int*);

// Narrow host declarations for the production async-connect helper.
struct esp_tls_cfg_t {
    bool non_block = false;
    int timeout_ms = 0;
    bool is_plain_tcp = false;
    const char* common_name = nullptr;
    esp_err_t (*crt_bundle_attach)(void*) = nullptr;
};
int esp_tls_conn_new_async(const char*, int, int, const esp_tls_cfg_t*, esp_tls_t*);
esp_err_t esp_tls_get_conn_sockfd(esp_tls_t*, int*);
