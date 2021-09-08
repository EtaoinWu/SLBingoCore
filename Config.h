#pragma once
#include <string>
#include "json.hpp"

struct Config
{
  enum class PointerMethod
  {
    stable,
    experimental
  };

  static const char *to_string(PointerMethod e) {
    switch (e) {
    case PointerMethod::stable: return "stable";
    case PointerMethod::experimental: return "experimental";
    }
    return nullptr;
  }

  std::string game_process = "SuperliminalSteam.exe";
  std::string host = "127.0.0.1";
  int port = 11450;
  PointerMethod method = PointerMethod::stable;

  Config() = default;

  explicit Config(const nlohmann::json &data) {
    if(data.contains("game_process")) {
      game_process = data["game_process"];
    }

    if (data.contains("listen")) {
      if (data["listen"].contains("host")) {
        host = data["listen"]["host"];
      }

      if (data["listen"].contains("port")) {
        port = data["listen"]["port"];
      }
    }

    if (data.contains("method")) {
      if (std::string(data["method"]) == "experimental") {
        method = PointerMethod::experimental;
      }
    }
  }

  operator nlohmann::json() {
    return {
      {
        "game_process", game_process
      },
      {
        "listen", {
          {
            "host", host
          },
          {
            "port", port
          }
        },
      },
      {
        "method", to_string(method)
      }
    };
  }
};
