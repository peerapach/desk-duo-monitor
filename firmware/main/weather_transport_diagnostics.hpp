#pragma once
#include "weather_diagnostics.hpp"
#include "esp_tls.h"
namespace deskduo {
WeatherHeap diagnosticHeap();
void recordWeatherTlsErrors(WeatherAttempt* attempt, esp_tls_t* tls, int socketError);
void recordWeatherTransportFailure(WeatherAttempt* attempt, esp_tls_t* tls,
                                  WeatherOutcome outcome, WeatherPhase phase, int socketError);
}
