#include <iomanip>
#include <iostream>
#include <fstream>

#include "Config.h"
#include "ReportGenerator.h"
#include "httplib.h"

void difference_tracker(ReportGenerator &gen) {
  json last = gen();
  std::cout << last << std::endl;
  for (int T = 0; true; T++) {
    json now = last;
    now["complain"] = nullptr;
    now.merge_patch(gen());
    if (now["achievements"] != last["achievements"]) {
      std::vector<std::string> result;
      std::ranges::set_difference(now["achievements"], last["achievements"],
                                  back_inserter(result));
      std::cout << std::format("{:5}: Got achievement: {}", T,
                               json(result).dump()) << std::endl;
    }
    for (const auto &[key, value] : now["collectible"].items()) {
      if (now["collectible"][key] != last["collectible"][key]) {
        for (int i = 0; i < now["collectible"][key].size(); ++i) {
          if (now["collectible"][key][i] != last["collectible"][key][i]) {
            std::cout << std::format("{:5}: Got {} #{}", T, key, i) <<
              std::endl;
          }
        }
      }
    }

    for (const auto &[key, value] : now["challenge_mode"].items()) {
      if (now["challenge_mode"][key] != last["challenge_mode"][key]) {
        std::vector<std::string> result;
        std::ranges::set_difference(now["challenge_mode"][key],
                                    last["challenge_mode"][key],
                                    back_inserter(result));
        std::cout << std::format("{:5}: Got challenge: {} in chapter {}", T,
                                 json(result).dump(), key) << std::endl;
      }
    }
    last = now;
    Sleep(10);
  }
}

std::mutex main_mutex;

int main() {
  Config cfg;
  if (std::ifstream config_file("config.json"); config_file) {
    json config_json;
    config_file >> config_json;
    cfg = Config(config_json);
  }

  ReportGenerator gen{cfg};

  std::cout << gen().dump() << std::endl;

  httplib::Server svr;

  svr.Get("/api", [&gen](const httplib::Request &req, httplib::Response &res)
  {
    std::unique_lock lock(main_mutex);
    res.set_content(gen().dump(2), "application/json");
    res.set_header("Access-Control-Allow-Origin", "*");
  });

  if (!svr.listen(cfg.host.c_str(), cfg.port)) {
    std::cout << "port " << cfg.port << " in use." << std::endl;
    return -1;
  }
  return 0;
}
