#include <iomanip>
#include <iostream>
#include <fstream>

#include "Config.h"
#include "ReportGenerator.h"
#include "httplib.h"

int main() {
  Config cfg;
  if (std::ifstream config_file("config.json"); config_file) {
    json config_json;
    config_file >> config_json;
    cfg = Config(config_json);
  }

  ReportGenerator gen{cfg};
  httplib::Server svr;

  svr.Get("/api", [&gen](const httplib::Request &req, httplib::Response &res)
  {
    res.set_content(gen().dump(2), "application/json");
  });

  svr.listen(cfg.host.c_str(), cfg.port);
  return 0;
}
