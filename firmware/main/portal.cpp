#include "portal.hpp"
#include "runtime.hpp"
#include "assets.hpp"
#include "ota.hpp"
#include "ota_policy.hpp"
#include "esp_http_server.h"
#include <cstring>
namespace deskduo {
namespace {
std::string header(httpd_req_t* request, const char* name, std::size_t cap) {
    const auto length = httpd_req_get_hdr_value_len(request, name);
    if (length == 0 || length > cap) return {};
    std::string value(length + 1, '\0');
    if (httpd_req_get_hdr_value_str(request, name, value.data(), value.size()) != ESP_OK) return {};
    value.resize(length); return value;
}
esp_err_t respond(httpd_req_t* request, int status, const std::string& body = "") {
    const char* text = "500 Internal Server Error";
    switch (status) {
        case 200: text = "200 OK"; break;
        case 204: text = "204 No Content"; break;
        case 400: text = "400 Bad Request"; break;
        case 401: text = "401 Unauthorized"; break;
        case 408: text = "408 Request Timeout"; break;
        case 413: text = "413 Content Too Large"; break;
        case 415: text = "415 Unsupported Media Type"; break;
        case 503: text = "503 Service Unavailable"; break;
    }
    httpd_resp_set_status(request, text);
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    if (status >= 400) httpd_resp_set_hdr(request, "Connection", "close");
    const auto result = httpd_resp_send(request, body.data(), body.size());
    return status >= 400 ? ESP_FAIL : result;
}
esp_err_t route(httpd_req_t* request) {
    const std::string path = std::string(request->uri).substr(0, std::string(request->uri).find('?'));
    const bool usage = path == "/api/usage";
    const auto config = runtime_config().config;
    if (!authorized(header(request, "Authorization", 256), usage, config.setupPassword, config.usageToken)) {
        if (!usage) httpd_resp_set_hdr(request, "WWW-Authenticate", "Basic realm=\"Desk Duo\"");
        return respond(request, 401);
    }
    if (request->method == HTTP_GET) {
        if (path == "/") {
            httpd_resp_set_type(request, "text/html; charset=utf-8");
            httpd_resp_set_hdr(request, "Content-Security-Policy", "default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
            return respond(request, 200, portal_html);
        }
        httpd_resp_set_type(request, "application/json");
        return respond(request, 200, path == "/api/firmware" ? ota_status_json() : redactedConfig(config));
    }
    if (path == "/api/firmware") return ota_upload(request);
    // Reserve settings maintenance before body receive; usage remains available.
    MaintenanceLease lease(usage ? Maintenance::Idle : Maintenance::Settings);
    if (!usage && !lease) return respond(request, 503, "Firmware maintenance busy; retry shortly");
    int policy = bodyPolicy(request->content_len, header(request, "Content-Type", 128), usage ? 4096 : 8192);
    if (policy != 0) return respond(request, policy);
    // No chunked/unbounded request bodies; HTTP server supplies content_len.
    std::string body(request->content_len, '\0');
    std::size_t offset = 0;
    const auto deadline = now_ms() + 10000;
    while (offset < body.size()) {
        if (now_ms() >= deadline) return respond(request, 408);
        int count = httpd_req_recv(request, body.data() + offset, body.size() - offset);
        if (count <= 0) return respond(request, 408);
        offset += count;
    }
    if (usage) {
        auto parsed = parseUsage(body);
        if (!parsed.value) return respond(request, 400);
        runtime_usage(*parsed.value);
        return respond(request, 204);
    }
    return respond(request, runtime_patch(body));
}
}
void portal_init() {
    httpd_config_t settings = HTTPD_DEFAULT_CONFIG();
    settings.stack_size = 12288;
    settings.max_open_sockets = 4; settings.lru_purge_enable = false;
    settings.max_uri_handlers = 8;
    settings.recv_wait_timeout = 1; settings.send_wait_timeout = 2;
    httpd_handle_t server;
    ESP_ERROR_CHECK(httpd_start(&server, &settings));
    for (const auto& path : {"/", "/api/config", "/api/firmware"}) {
        httpd_uri_t uri{}; uri.uri = path; uri.method = HTTP_GET; uri.handler = route;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));
    }
    for (const auto& path : {"/api/config", "/api/usage", "/api/firmware"}) {
        httpd_uri_t uri{}; uri.uri = path; uri.method = HTTP_POST; uri.handler = route;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));
    }
}
}
