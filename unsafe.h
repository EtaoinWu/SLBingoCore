#pragma once

#include <string>
#include <vector>

inline std::string trunk_string(std::wstring_view s) {
  return std::string(s.begin(), s.end());
}

inline std::wstring untrunk_string(std::string_view s) {
  return std::wstring(s.begin(), s.end());
}

template<typename T>
auto to_vector(const T &z) -> std::vector<decltype(*z.begin())> {
  return std::vector(z.begin(), z.end());
}
