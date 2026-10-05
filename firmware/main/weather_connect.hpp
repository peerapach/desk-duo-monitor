#pragma once
#include "network.hpp"
#include "weather_transport_diagnostics.hpp"
namespace deskduo {
// DNS, handle allocation and cleanup remain owned by the transport caller.
int connectResolvedTransport(FetchPolicy& policy, esp_tls_t* tls, bool secure,
                             const char* host, const std::string& address, int port,
                             WeatherAttempt* attempt);
}
