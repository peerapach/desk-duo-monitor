#pragma once
#include <string>
namespace deskduo {
void input_init();
std::string input_diagnostics_json();
bool input_read(int& x, int& y);
}
