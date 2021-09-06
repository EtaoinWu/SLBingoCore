#pragma once
#include <iostream>
#include <format>

class Complaint final : public std::exception
{
  std::wstring msg_;
public:
  Complaint(std::wstring msg):msg_{std::move(msg)} {}
  const std::wstring &wwhat() const {
    return msg_;
  }
};

void complain(const std::wstring &msg) {
  throw Complaint{msg};
  // std::wcerr << L"Complain: " << msg << std::endl;
}

template<typename ...Args>
void complain_fmt(const Args& ... args) {
  complain(std::format(args...));
}

void external_error(const std::wstring &msg) {
  std::wcerr << L"External error: " << msg << std::endl;
  exit(-1);
}

template<typename ...Args>
void external_error_fmt(const Args& ... args) {
  external_error(std::format(args...));
}

