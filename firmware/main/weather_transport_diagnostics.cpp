#include "weather_transport_diagnostics.hpp"
#include "esp_heap_caps.h"
namespace deskduo {
WeatherHeap diagnosticHeap() {
    return {static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
            static_cast<uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT))};
}
void recordWeatherTlsErrors(WeatherAttempt* attempt, esp_tls_t* tls, int socketError) {
    if (!attempt || attempt->espError) return;
    if (!attempt->socketErrno) attempt->socketErrno = socketError;
    if (!tls) return;
    esp_tls_error_handle_t errors = nullptr;
    if (esp_tls_get_error_handle(tls, &errors) == ESP_OK && errors) {
        int code = 0, flags = 0;
        attempt->espError = esp_tls_get_and_clear_last_error(errors, &code, &flags);
        attempt->tlsCode = code; attempt->verifyFlags = flags;
    }
}
void recordWeatherTransportFailure(WeatherAttempt* attempt, esp_tls_t* tls,
                                  WeatherOutcome outcome, WeatherPhase phase, int socketError) {
    if (!attempt || attempt->outcome != WeatherOutcome::Running) return;
    recordWeatherTlsErrors(attempt, tls, socketError);
    // ESP-TLS uses this error tracker for both TCP setup and handshake.
    if (phase == WeatherPhase::Tls && attempt->espError &&
        (*attempt->espError == ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET ||
         *attempt->espError == ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST ||
         *attempt->espError == ESP_ERR_ESP_TLS_SOCKET_SETOPT_FAILED)) {
        outcome = WeatherOutcome::TcpFailure; phase = WeatherPhase::Tcp;
    }
    attempt->fail(outcome, phase); attempt->failureHeap = diagnosticHeap();
}
}
