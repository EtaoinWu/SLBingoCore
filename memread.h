#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <psapi.h>
#include <iostream>
#include <format>
#include <utility>
#include <set>
#include <map>
#include <memory>
#include <vector>
#include "error.h"
#include <mutex>

#pragma comment(lib, "psapi")
using std::string;
using std::wstring;
#undef max

namespace MemRead
{
  using ssize_t = std::make_signed_t<std::size_t>;
  const intptr_t small_address = 0x100000;

  class Process
  {
    DWORD process_id_;
    HANDLE process_handle_;
    std::mutex process_working_;

  public:
    Process(): process_id_{0}, process_handle_{nullptr} {
    }

    ~Process() {
      close();     
    }

    HANDLE handle() const {
      return process_handle_;
    }

    bool exist() const {
      return process_id_ && process_handle_;
    }

    void close() {
      std::lock_guard guard(process_working_);
      if (!exist()) return;
      process_id_ = 0;
      CloseHandle(process_handle_);
      process_handle_ = nullptr;
    }

    static DWORD get_process_id(const WCHAR *processName) {
      SetLastError(0);
      PROCESSENTRY32 pe32;
      GetLastError();
      pe32.dwSize = sizeof(PROCESSENTRY32);
      HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

      if (Process32First(h_snapshot, &pe32)) {
        do {
          if (wcscmp(pe32.szExeFile, processName) == 0)
            break;
        } while (Process32Next(h_snapshot, &pe32));
      }

      if (h_snapshot != INVALID_HANDLE_VALUE)
        CloseHandle(h_snapshot);
      int err = GetLastError();
      //std::cout << err << std::endl;
      if (err != 0) {
        return 0;
      }
      return pe32.th32ProcessID;
    }

    void open_process(const wstring &name) {
      std::lock_guard guard(process_working_);
      process_id_ = get_process_id(name.c_str());
      if (process_id_ == 0) {
        external_error(L"Cannot find process with name " + name);
      }
      process_handle_ = OpenProcess(PROCESS_ALL_ACCESS, false, process_id_);
      if (process_handle_ == nullptr) {
        external_error_fmt(L"Cannot open process with name {}, pid {}",
                           name, process_id_);
      }
    }

    template <typename T>
    T get(intptr_t address) {
      std::lock_guard guard(process_working_);
      if (!exist())
        complain(L"Process not open");
      if (address < small_address)
        complain_fmt(L"Memory read error: pointer {:x}", address);
      T buffer;
      size_t bytes_to_read = sizeof(T);
      size_t bytes_actually_read;
      bool success = ReadProcessMemory(process_handle_, (LPCVOID)address,
                                       &buffer,
                                       bytes_to_read,
                                       &bytes_actually_read);
      if (!success || bytes_actually_read != bytes_to_read) {
        complain_fmt(L"Memory read error: pointer {:x}", address);
        return -1;
      }
      return buffer;
    }

    template <typename T>
    void get_array(intptr_t address, ssize_t num, T *result) {
      std::lock_guard guard(process_working_);
      if (!exist())
        complain(L"Process not open");
      if (address < small_address)
        complain_fmt(L"Array read error: pointer {:x}", address);
      size_t bytes_to_read = sizeof(T) * num;
      size_t bytes_actually_read;
      bool success = ReadProcessMemory(process_handle_, (LPCVOID)address,
                                       result,
                                       bytes_to_read,
                                       &bytes_actually_read);
      if (!success || bytes_actually_read != bytes_to_read) {
        complain_fmt(L"Array read error: pointer {:x}", address);
      }
    }

    string get_astr(intptr_t address, ssize_t max_length = 255) {
      std::lock_guard guard(process_working_);
      if (!exist())
        complain(L"Process not open");
      if (address < small_address) {
        complain_fmt(L"AString read error: pointer {:x}", address);
      }
      std::vector<char> buffer(max_length, 0);
      size_t bytes_to_read = sizeof(char) * max_length;
      size_t bytes_actually_read;
      bool success = ReadProcessMemory(process_handle_, (LPCVOID)address,
                                       buffer.data(),
                                       bytes_to_read,
                                       &bytes_actually_read);
      if (!success) {
        complain_fmt(L"AString read error: pointer {:x}", address);
      }
      buffer.push_back(0);
      return string{buffer.data()};
    }

    wstring get_wstr(intptr_t address, ssize_t max_length = 255) {
      std::lock_guard guard(process_working_);
      if (!exist())
        complain(L"Process not open");
      if (address < small_address) {
        complain_fmt(L"LString read error: pointer {:x}", address);
      }
      std::vector<wchar_t> buffer(max_length, 0);
      size_t bytes_to_read = sizeof(wchar_t) * max_length;
      size_t bytes_actually_read;
      bool success = ReadProcessMemory(process_handle_, (LPCVOID)address,
                                       buffer.data(),
                                       bytes_to_read,
                                       &bytes_actually_read);
      if (!success) {
        complain_fmt(L"LString read error: pointer {:x}", address);
      }
      buffer.push_back(0);
      return wstring{buffer.data()};
    }
  };

  class Address;
  class AddressPlus;
  class AddressOffset;

  class Node
  {
    std::shared_ptr<Address> ptr_;
  public:
    Node(std::shared_ptr<Address> ptr): ptr_{std::move(ptr)} {
    }

    template <typename T>
    Node(std::shared_ptr<T> ptr): ptr_{std::move(ptr)} {
    }

    Address &operator*() const {
      return *ptr_;
    }

    Address *operator->() const {
      return ptr_.operator->();
    }

    Node operator+(ptrdiff_t rhs) const;
    Node operator[](ptrdiff_t rhs) const;
  };

  class Address : public std::enable_shared_from_this<Address>
  {
  private:
    intptr_t loaded_address_;
  protected:
    Process *process_;
  public:
    Address(Process *process): process_{process}, loaded_address_{0} {
    }

    virtual ~Address() = default;
    virtual intptr_t address() = 0;

    [[nodiscard]] Process *process() const {
      return process_;
    }

    [[nodiscard]] bool loaded() const {
      return loaded_address_ != 0;
    }

    intptr_t preload() {
      return loaded_address_ = address();
    }

    void unload() {
      loaded_address_ = 0;
    }

    intptr_t address_opt() {
      return loaded_address_ ? loaded_address_ : address();
    }

    template <typename T>
    typename T::result_type load() {
      return T::parse(shared_from_this());
    }

    template <typename T>
    T get() {
      return process_->get<T>(address_opt());
    }

    template <typename T>
    std::vector<T> get_array(ssize_t n) {
      std::vector<T> result(n);
      process_->get_array(address_opt(), n, result.data());
      return result;
    }

    string get_astr(ssize_t max_length = 255) {
      return process_->get_astr(address_opt(), max_length);
    }

    wstring get_wstr(ssize_t max_length = 255) {
      return process_->get_wstr(address_opt(), max_length);
    }

    Node plus(ptrdiff_t offset);
    Node next(ptrdiff_t offset);
    Node store();
  };

  class Module final : public Address
  {
    wstring name_;
  public:
    Module(Process *process, wstring name): Address{process},
      name_{std::move(name)} {
    }

    ~Module() override = default;

    intptr_t address() override {
      HMODULE *h_modules = nullptr;
      DWORD c_modules;
      intptr_t dw_base = -1;

      EnumProcessModules(process_->handle(), h_modules, 0, &c_modules);
      h_modules = new HMODULE[c_modules / sizeof(HMODULE)];

      if (EnumProcessModules(process_->handle(), h_modules,
                             c_modules / sizeof(HMODULE),
                             &c_modules)) {
        for (ssize_t i = 0; i < c_modules / sizeof(HMODULE); i++) {
          if (WCHAR sz_buf[50]; GetModuleBaseName(
            process_->handle(), h_modules[i], sz_buf,
            sizeof sz_buf) && name_.compare(sz_buf) == 0) {
            dw_base = reinterpret_cast<intptr_t>(h_modules[i]);
            break;
          }
        }
      }

      delete[] h_modules;
      return dw_base;
    }
  };

  class FixedAddress : public Address
  {
    intptr_t address_;
  public:
    FixedAddress(Process *process, intptr_t address): Address{process},
      address_{address} {
    }

    ~FixedAddress() override = default;

    intptr_t address() override {
      return address_;
    }
  };

  class AddressPlus : public Address
  {
    ptrdiff_t offset_;
    Node parent_;
  public:
    AddressPlus(const Node &parent, ptrdiff_t offset): Address{
        parent->process()
      },
      offset_{offset}, parent_{parent} {
    }

    ~AddressPlus() override = default;

    intptr_t address() override {
      return parent_->address() + offset_;
    }
  };

  class AddressOffset : public Address
  {
    ptrdiff_t offset_;
    Node parent_;
  public:
    AddressOffset(const Node &parent, ptrdiff_t offset): Address{
        parent->process()
      },
      offset_{offset}, parent_{parent} {
    }

    ~AddressOffset() override = default;

    intptr_t address() override {
      return parent_->get<intptr_t>() + offset_;
    }
  };

  inline Node Address::plus(ptrdiff_t offset) {
    return std::make_shared<AddressPlus>(shared_from_this(), offset);
  }

  inline Node Address::next(ptrdiff_t offset) {
    return std::make_shared<AddressOffset>(shared_from_this(), offset);
  }

  inline Node Address::store() {
    return std::make_shared<FixedAddress>(this->process_, address());
  }


  inline Node Node::operator+(ptrdiff_t rhs) const {
    return ptr_->plus(rhs);
  }

  inline Node Node::operator[](ptrdiff_t rhs) const {
    return ptr_->next(rhs);
  }

  template <typename T, typename ...Args>
  Node make_node(Args &&... args) {
    return Node{std::make_shared<T>(std::forward<Args>(args)...)};
  }

  template <typename T>
  struct ParserBase
  {
    using result_type = T;
  };
}
