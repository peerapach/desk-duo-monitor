#pragma once
#include "core.hpp"
namespace deskduo {
// Bounded administrator presentation DTO; configuration never enters this surface.
std::string dashboard_json(const Snapshot& snapshot);
}
