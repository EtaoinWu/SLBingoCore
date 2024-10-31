#pragma once
#include <iostream>
#include <format>

class Complaint final : public std::exception
{
  std::wstring msg_;
public:
  Complaint(std::wstring_view msg):msg_{msg} {}
  const std::wstring_view &wwhat() const {
    return msg_;
  }
};

class ExternalError final : public std::exception
{
  std::wstring msg_;
public:
  ExternalError(std::wstring_view msg):msg_{msg} {}
  const std::wstring_view &wwhat() const {
    return msg_;
  }
};

inline void complain(std::wstring_view msg) {
  throw Complaint{msg};
  // std::wcerr << L"Complain: " << msg << std::endl;
}

template<typename ...Args>
void complain_fmt(std::wformat_string<Args...> fmt, Args&& ... args) {
  complain(std::format(fmt, std::forward<Args>(args)...));
}

inline void external_error(std::wstring_view msg) {
  throw ExternalError{msg};
}

template<typename ...Args>
void external_error_fmt(std::wformat_string<Args...> fmt, Args&& ... args) {
  external_error(std::format(fmt, std::forward<Args>(args)...));
}

