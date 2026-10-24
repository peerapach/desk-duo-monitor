#include "network.hpp"
#include "weather_request.hpp"
#include "runtime.hpp"
#include "stock_source.hpp"
#include "ota_policy.hpp"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "weather_transport_diagnostics.hpp"
#include "weather_connect.hpp"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_tls.h"
#include "esp_transport.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <new>

namespace deskduo {
namespace {
std::atomic<bool> online{false};
WeatherDiagnostics weatherDiagnostics;
void failAttempt(WeatherAttempt* attempt, WeatherOutcome outcome, WeatherPhase phase, bool heap = false) {
    if (!attempt) return;
    attempt->fail(outcome, phase);
    if (heap) attempt->failureHeap = diagnosticHeap();
}
void wifiEvent(void*, esp_event_base_t base, int32_t id, void*) {
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) online = true;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) online = false;
}
// Callback storage outlives a timed-out caller. Two owners: network task and
// queued tcpip/DNS callback. The last owner deletes the semaphore and storage.
struct Dns {
    std::atomic<unsigned> owners{2};
    SemaphoreHandle_t ready = xSemaphoreCreateBinary();
    std::string host;
    ip_addr_t address{};
    bool ok = false;
    explicit Dns(std::string h) : host(std::move(h)) {}
    void release() { if (owners.fetch_sub(1) == 1) { if (ready) vSemaphoreDelete(ready); delete this; } }
};
void dnsDone(const char*, const ip_addr_t* address, void* arg) {
    auto* dns = static_cast<Dns*>(arg);
    if (address) { dns->address = *address; dns->ok = true; }
    xSemaphoreGive(dns->ready);
    dns->release();
}
void dnsStart(void* arg) {
    auto* dns = static_cast<Dns*>(arg);
    ip_addr_t address;
    err_t err = dns_gethostbyname(dns->host.c_str(), &address, dnsDone, dns);
    if (err != ERR_INPROGRESS) dnsDone(nullptr, err == ERR_OK ? &address : nullptr, dns);
}
bool resolve(const char* host, FetchPolicy& policy, std::string& numeric, WeatherAttempt* attempt) {
    ip_addr_t address;
    if (ipaddr_aton(host, &address)) { numeric = host; return true; }
    auto* dns = new(std::nothrow) Dns(host);
    if (!dns) { failAttempt(attempt, WeatherOutcome::DnsAllocation, WeatherPhase::Dns, true); return false; }
    if (!dns->ready || tcpip_try_callback(dnsStart, dns) != ERR_OK) {
        failAttempt(attempt, !dns->ready ? WeatherOutcome::DnsAllocation : WeatherOutcome::DnsQueue, WeatherPhase::Dns, !dns->ready);
        dns->release(); dns->release(); return false;
    }
    int remain = policy.remaining(now_ms());
    const bool ready = remain > 0 && xSemaphoreTake(dns->ready, pdMS_TO_TICKS(remain)) == pdTRUE;
    bool ok = ready && dns->ok;
    if (!ok) failAttempt(attempt, ready ? WeatherOutcome::DnsFailure : WeatherOutcome::Deadline, WeatherPhase::Dns);
    if (ok) { char buf[IPADDR_STRLEN_MAX]; ipaddr_ntoa_r(&dns->address, buf, sizeof(buf)); numeric = buf; }
    dns->release();
    return ok;
}
struct Transport {
    FetchPolicy& policy;
    bool secure;
    esp_tls_t* tls = nullptr;
    std::size_t wireBytes = 0;
    bool rejected = false;
    WeatherAttempt* attempt = nullptr;
};
// Copy errors while the handle exists. HTTP may close the adapter internally
// before perform returns, so every failing transport callback records here.
void transportFailure(Transport& s, WeatherOutcome outcome, WeatherPhase phase, int socketError = errno) {
    recordWeatherTransportFailure(s.attempt, s.tls, outcome, phase, socketError);
}
Transport& state(esp_transport_handle_t t) { return *static_cast<Transport*>(esp_transport_get_context_data(t)); }
int closeTransport(esp_transport_handle_t t) {
    auto& s = state(t);
    if (s.tls) {
        // Also covers an SDK-internal close before perform reports HTTP failure.
        recordWeatherTlsErrors(s.attempt, s.tls, errno);
        esp_tls_conn_destroy(s.tls); s.tls = nullptr;
    }
    return 0;
}
int connectTransport(esp_transport_handle_t t, const char* host, int port, int) {
    auto& s = state(t);
    std::string address;
    if (!resolve(host, s.policy, address, s.attempt)) return -1;
    if (s.attempt) s.attempt->beforeTls = diagnosticHeap();
    s.tls = esp_tls_init();
    if (!s.tls) { failAttempt(s.attempt, WeatherOutcome::TransportAllocation, WeatherPhase::Tls, true); return -1; }
    return connectResolvedTransport(s.policy, s.tls, s.secure, host, address, port, s.attempt);
}

bool retry(int n) {
    return n == ESP_TLS_ERR_SSL_WANT_READ || n == ESP_TLS_ERR_SSL_WANT_WRITE ||
           (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR));
}
int readTransport(esp_transport_handle_t t, char* data, int length, int) {
    auto& s = state(t);
    while (!s.rejected && s.policy.remaining(now_ms()) > 0) {
        errno = 0;
        int n = esp_tls_conn_read(s.tls, data, length);
        if (n > 0) {
            // Also bound HTTP headers/chunk framing, before SDK parser allocation.
            if (static_cast<std::size_t>(n) > 32768 - s.wireBytes) { s.rejected = true; failAttempt(s.attempt, WeatherOutcome::WireLimit, WeatherPhase::Http); return -1; }
            s.wireBytes += n;
            return n;
        }
        if (n == 0) return ERR_TCP_TRANSPORT_CONNECTION_CLOSED_BY_FIN;
        if (!retry(n)) { transportFailure(s, s.secure ? WeatherOutcome::TlsFailure : WeatherOutcome::TcpFailure,
                                         WeatherPhase::Http); return -1; }
        vTaskDelay(1);
    }
    errno = ETIMEDOUT;
    transportFailure(s, WeatherOutcome::Deadline, WeatherPhase::Http);
    return -1;
}
int writeTransport(esp_transport_handle_t t, const char* data, int length, int) {
    auto& s = state(t);
    while (!s.rejected && s.policy.remaining(now_ms()) > 0) {
        errno = 0;
        int n = esp_tls_conn_write(s.tls, data, length);
        if (n > 0) return n;
        if (!retry(n)) { transportFailure(s, s.secure ? WeatherOutcome::TlsFailure : WeatherOutcome::TcpFailure,
                                         WeatherPhase::Http); return -1; }
        vTaskDelay(1);
    }
    errno = ETIMEDOUT;
    transportFailure(s, WeatherOutcome::Deadline, WeatherPhase::Http);
    return -1;
}
int pollTransport(esp_transport_handle_t t, int) { return state(t).policy.remaining(now_ms()) > 0 ? 1 : 0; }
int destroyTransport(esp_transport_handle_t t) { return closeTransport(t); }
struct Response {
    Transport& transport;
    std::string body;
    std::string location;
};
esp_err_t received(esp_http_client_event_t* event) {
    auto& response = *static_cast<Response*>(event->user_data);
    auto& transport = response.transport;
    if (event->event_id == HTTP_EVENT_ON_DATA) {
        if (!transport.policy.append(event->data_len)) {
            transport.rejected = true; failAttempt(transport.attempt, WeatherOutcome::BodyLimit, WeatherPhase::Http);
        }
        else response.body.append(static_cast<const char*>(event->data), event->data_len);
    }
    if (event->event_id == HTTP_EVENT_ON_HEADER && strcasecmp(event->header_key, "location") == 0) {
        if (std::strlen(event->header_value) > 1024) {
            transport.rejected = true; failAttempt(transport.attempt, WeatherOutcome::RedirectHeader, WeatherPhase::Http);
        }
        else response.location = event->header_value;
    }
    return ESP_OK; // SDK callbacks do not reliably propagate errors; transport enforces rejection.
}
std::optional<std::string> fetch(const SourceConfig& source, WeatherAttempt* attempt = nullptr) {
    FetchPolicy policy(source.url, now_ms());
    while (policy.remaining(now_ms()) > 0) {
        const auto tls = transportPolicy(policy.url, std::time(nullptr));
        // SNTP failure is honest unavailability, never disabled certificate time checks.
        if (!tls.mayConnect) {
            failAttempt(attempt, tls.secure && std::time(nullptr) < 1700000000 ? WeatherOutcome::ClockWait : WeatherOutcome::InvalidTarget, WeatherPhase::Gate);
            return std::nullopt;
        }
        if (attempt) {
            // Each redirect hop has a new handle. Earlier observed successes
            // must not mask the last hop's unavailable status or TLS failure.
            attempt->httpResult.reset(); attempt->httpStatus.reset();
            attempt->espError.reset(); attempt->tlsCode.reset(); attempt->verifyFlags.reset(); attempt->socketErrno.reset();
            attempt->beforeTls = {};
            attempt->beforeHttp = diagnosticHeap();
        }
        Transport transport{policy, tls.secure, nullptr, 0, false, attempt};
        auto adapter = esp_transport_init();
        if (!adapter) { failAttempt(attempt, WeatherOutcome::TransportAllocation, WeatherPhase::Http, true); return std::nullopt; }
        esp_transport_set_context_data(adapter, &transport);
        esp_transport_set_func(adapter, connectTransport, readTransport, writeTransport,
                               closeTransport, pollTransport, pollTransport, destroyTransport);
        Response response{transport, {}, {}};
        esp_http_client_config_t cfg{};
        cfg.url = policy.url.c_str(); cfg.transport = adapter;
        cfg.disable_auto_redirect = true; cfg.max_authorization_retries = -1;
        cfg.event_handler = received; cfg.user_data = &response;
        cfg.buffer_size = 512; cfg.buffer_size_tx = 2048;
        cfg.timeout_ms = policy.remaining(now_ms());
        auto client = esp_http_client_init(&cfg);
        if (!client) { failAttempt(attempt, WeatherOutcome::HttpAllocation, WeatherPhase::Http, true); esp_transport_destroy(adapter); return std::nullopt; }
        esp_http_client_set_header(client, "Accept", "application/json");
        esp_http_client_set_header(client, "User-Agent", "DeskDuo/1.0");
        esp_http_client_set_header(client, "Connection", "close");
        bool headersOk = true;
        if (policy.sendHeaders) for (const auto& [key, value] : source.headers)
            headersOk &= esp_http_client_set_header(client, key.c_str(), value.c_str()) == ESP_OK;
        const auto result = headersOk ? esp_http_client_perform(client) : ESP_FAIL;
        const int status = esp_http_client_get_status_code(client);
        if (attempt) {
            // An unperformed request has no HTTP-client result. Zero status
            // means no status line was observed, not an upstream HTTP 0.
            if (headersOk) attempt->httpResult = result;
            if (status > 0) attempt->httpStatus = status;
            attempt->wireBytes += transport.wireBytes;
            attempt->bodyBytes = policy.bodyBytes; attempt->redirects = policy.redirects;
            if (!headersOk) attempt->fail(WeatherOutcome::HttpHeaders, WeatherPhase::Http);
            else if (result != ESP_OK) attempt->fail(policy.remaining(now_ms()) == 0 ? WeatherOutcome::Deadline : WeatherOutcome::HttpPerform, WeatherPhase::Http);
        }
        // Client owns only its built-in transport list. Our adapter remains owned here.
        esp_http_client_cleanup(client);
        esp_transport_destroy(adapter);
        if (result != ESP_OK || transport.rejected || policy.remaining(now_ms()) == 0) {
            if (policy.remaining(now_ms()) == 0) failAttempt(attempt, WeatherOutcome::Deadline, WeatherPhase::Http);
            return std::nullopt;
        }
        if (status >= 200 && status < 300) return response.body;
        if (!(status == 301 || status == 302 || status == 303 || status == 307 || status == 308)) {
            failAttempt(attempt, WeatherOutcome::HttpStatus, WeatherPhase::Http); return std::nullopt;
        }
        // The same portable policy resolves, validates and strips headers.
        if (!policy.redirect(response.location)) {
            failAttempt(attempt, WeatherOutcome::RedirectRejected, WeatherPhase::Http); return std::nullopt;
        }
        if (attempt) attempt->redirects = policy.redirects;
    }
    failAttempt(attempt, WeatherOutcome::Deadline, WeatherPhase::Http);
    return std::nullopt;
}
}
std::string network_diagnostics_json() {
    const auto attempt = weatherDiagnostics.snapshot();
    const auto current = runtime_weather_view();
    return weatherDiagnosticsJson(attempt, current, online.load(), std::time(nullptr), now_ms());
}
void network_init() {
    // ESP's std::mutex backing allocation is warmed before any source fetch.
    (void)weatherDiagnostics.snapshot();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    auto* ap = esp_netif_create_default_wifi_ap();
    esp_netif_ip_info_t info{};
    IP4_ADDR(&info.ip, 192,168,4,1); IP4_ADDR(&info.gw, 192,168,4,1); IP4_ADDR(&info.netmask,255,255,255,0);
    esp_netif_dhcps_stop(ap); ESP_ERROR_CHECK(esp_netif_set_ip_info(ap, &info)); ESP_ERROR_CHECK(esp_netif_dhcps_start(ap));
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEvent, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEvent, nullptr));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    auto config = runtime_config().config;
    wifi_config_t settings{};
    auto name = ap_name();
    std::memcpy(settings.ap.ssid, name.data(), name.size()); settings.ap.ssid_len = name.size();
    std::memcpy(settings.ap.password, config.setupPassword.data(), config.setupPassword.size());
    settings.ap.channel = 1; settings.ap.max_connection = 2; settings.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &settings));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();
}
std::string station_address() {
    // ui_init can reveal setup before esp_netif_init; never enter lwIP IPC then.
    if (!online) return "--";
    esp_netif_ip_info_t info{};
    auto* station = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!online || !station || esp_netif_get_ip_info(station, &info) != ESP_OK) return "--";
    char value[16]; std::snprintf(value,sizeof(value),IPSTR,IP2STR(&info.ip)); return value;
}
void network_task(void*) {
    std::string ssid, password;
    bool stationReady = false;
    uint64_t generation = UINT64_MAX, weatherDue = 0, stocksDue = 0, retryDue = 0;
    for (;;) {
        // Config and generation are captured together, before any I/O.
        auto request = runtime_config();
        const auto& config = request.config;
        const auto now = now_ms();
        {
            MaintenanceLease stationLease(Maintenance::Network);
            if (!stationLease) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
            if (ssid != config.ssid || password != config.wifiPassword) {
                esp_wifi_disconnect(); online = false;
                wifi_config_t station{};
                std::memcpy(station.sta.ssid, config.ssid.data(), config.ssid.size());
                std::memcpy(station.sta.password, config.wifiPassword.data(), config.wifiPassword.size());
                stationReady = esp_wifi_set_config(WIFI_IF_STA, &station) == ESP_OK;
                ssid = config.ssid; password = config.wifiPassword; retryDue = 0;
            }
            if (stationReady && !ssid.empty() && !online && now >= retryDue) { esp_wifi_connect(); retryDue = now + 15000; }
        } // Station configuration cannot race OTA admission.
        if (maintenance.updating()) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
        if (generation != request.generation) { generation = request.generation; weatherDue = stocksDue = 0; }
        if (online && !config.weather.url.empty() && now >= weatherDue) {
            auto attempt = weatherDiagnostics.begin(request.generation, now_ms(), std::time(nullptr) >= 1700000000);
            auto effectiveWeather = config.weather;
            effectiveWeather.url = weather_request_url(config.weather);
            auto data = fetch(effectiveWeather, &attempt);
            const auto parsed = data ? parseWeather(*data, config.weather) : ParseResult<Weather>{std::nullopt, "unavailable"};
            runtime_weather(parsed, request.generation);
            if (data) {
                if (!parsed.value) {
                    attempt.parserReason = weatherParserReason(parsed.error);
                    attempt.fail(WeatherOutcome::ParseFailure, WeatherPhase::Parse);
                } else {
                    attempt.outcome = runtime_weather_view().generation == request.generation ? WeatherOutcome::Accepted : WeatherOutcome::Obsolete;
                    attempt.phase = WeatherPhase::Complete;
                }
            }
            const auto finished = now_ms();
            weatherDue = finished + config.weather.refreshSeconds * 1000ULL;
            weatherDiagnostics.finish(attempt, finished, weatherDue);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue; // Re-capture config/generation before starting another source.
        }
        // Re-capture next iteration after weather; old generation remains ineligible.
        if (online && (config.stocks.provider == StockProvider::Yahoo || !config.stocks.url.empty()) && now_ms() >= stocksDue) {
            if(config.stocks.provider == StockProvider::Yahoo) {
                const auto previous = runtime_snapshot().stocks.data;
                auto result = pollYahooStocks(config.stocks, previous ? &*previous : nullptr, std::time(nullptr),
                    [&](const SourceConfig& source) -> std::optional<std::string> {
                        // Stop this batch after settings changes or OTA admission.
                        if(maintenance.updating() || runtime_config().generation != request.generation) return std::nullopt;
                        return fetch(source);
                    });
                runtime_stocks(result, request.generation);
            } else {
                auto data = fetch(config.stocks);
                runtime_stocks(data ? parseStocks(*data) : ParseResult<Stocks>{std::nullopt, "unavailable"}, request.generation);
            }
            stocksDue = now_ms() + config.stocks.refreshSeconds * 1000ULL;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
}
