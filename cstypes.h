#pragma once
#include "memread.h"

namespace CSharpType
{
  using MemRead::ParserBase;
  using MemRead::Node;
  struct CSharpString : ParserBase<wstring>
  {
    static result_type parse(const Node &x) {
      return x[0x14]->get_str();
    }
  };

  template <typename T>
  struct CSharpArray : ParserBase<std::vector<T>>
  {
    static std::vector<T> parse(const Node &x) {
      auto length = x[0x18]->get<size_t>();
      return x[0x20]->get_array<T>(length);
    }
  };

  struct CSharpSetString : ParserBase<std::set<wstring>>
  {
    static std::set<wstring> parse(const Node &x) {
      auto length = std::max(x[0x30]->get<int>(), x[0x34]->get<int>());
      std::set<wstring> result;
      auto base = x[0x18];
      base->load();
      for (int i = 0; i < length; ++i) {
        result.insert(base[0x28 + i * 0x10]->load<CSharpString>());
      }
      return result;
    }
  };

  struct MiniChallengeDict : ParserBase<std::map<wstring, std::set<wstring>>>
  {
    static std::map<wstring, std::set<wstring>> parse(const Node &x) {
      auto length = x[0x40]->get<int>();
      std::map<wstring, std::set<wstring>> result;
      auto base = x[0x18];
      base->load();
      for (int i = 0; i < length; ++i) {
        result[base[0x28 + i * 0x18]->load<CSharpString>()]
          = base[0x30 + i * 0x18]->load<CSharpSetString>();
      }
      return result;
    }
  };
}
