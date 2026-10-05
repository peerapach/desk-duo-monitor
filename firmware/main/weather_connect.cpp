#include "weather_connect.hpp"
#include "runtime.hpp"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <cerrno>
namespace deskduo {
int connectResolvedTransport(FetchPolicy& policy, esp_tls_t* tls, bool secure,
                             const char* host, const std::string& address, int port,
                             WeatherAttempt* attempt) {
    esp_tls_cfg_t cfg{};
    cfg.non_block = true;
    cfg.is_plain_tcp = !secure;
    cfg.common_name = host; // SNI AND hostname verification despite numeric connect
    if (secure) cfg.crt_bundle_attach = esp_crt_bundle_attach;
    while (true) {
        // ESP-IDF retains select's fd_sets across pending TCP retries. A select
        // timeout must spend the shared fetch deadline rather than strand them.
        cfg.timeout_ms = policy.remaining(now_ms());
        if (cfg.timeout_ms <= 0) break; // the SDK waits without a bound at zero
        int result = esp_tls_conn_new_async(address.c_str(), address.size(), port, &cfg, tls);
        if (result == 1) {
            if (secure) return 0;
            // Plain nonblocking esp_tls returns after connect(EINPROGRESS).
            // Check socket completion before HTTP writes, without reconnecting.
            int socket = -1;
            if (esp_tls_get_conn_sockfd(tls, &socket) != ESP_OK) { recordWeatherTransportFailure(attempt, tls, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, errno); return -1; }
            while (policy.remaining(now_ms()) > 0) {
                fd_set writable; FD_ZERO(&writable); FD_SET(socket, &writable);
                timeval interval{0, 1000};
                int ready = select(socket + 1, nullptr, &writable, nullptr, &interval);
                if (ready < 0) { recordWeatherTransportFailure(attempt, tls, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, errno); return -1; }
                if (ready > 0) {
                    int error = 0; socklen_t length = sizeof(error);
                    const bool connected = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) == 0 && error == 0;
                    if (!connected) recordWeatherTransportFailure(attempt, tls, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, error ? error : errno);
                    return connected ? 0 : -1;
                }
                vTaskDelay(1);
            }
            recordWeatherTransportFailure(attempt, tls, WeatherOutcome::Deadline, WeatherPhase::Tcp, ETIMEDOUT);
            return -1;
        }
        if (result < 0) { recordWeatherTransportFailure(attempt, tls, secure ? WeatherOutcome::TlsFailure : WeatherOutcome::TcpFailure,
                                         secure ? WeatherPhase::Tls : WeatherPhase::Tcp, errno); return -1; }
        vTaskDelay(1);
    }
    errno = ETIMEDOUT;
    recordWeatherTransportFailure(attempt, tls, WeatherOutcome::Deadline, secure ? WeatherPhase::Tls : WeatherPhase::Tcp, errno);
    return -1;
}
}
