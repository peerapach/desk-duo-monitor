#pragma once
#include "core.hpp"
namespace deskduo {
// Effective request URL only; does not mutate saved configuration.
std::string weather_request_url(const SourceConfig& source);
}
