#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace sse
{

std::string run_process(std::string_view executable,
                        std::vector<std::string> const &arguments,
                        std::chrono::seconds timeout = std::chrono::seconds(10));

std::string run_js(std::string_view code);
std::string run_py(std::string_view code);
std::string run_kt(std::string_view code);
std::string run_c(std::string_view code);
std::string run_java(std::string_view code);

} // namespace sse
