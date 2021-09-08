#include <iomanip>
#include <iostream>
#include "ReportGenerator.h"
#include "httplib.h"

int main() {
  ReportGenerator gen;
  httplib::Server svr;

  svr.Get("/api", [&gen](const httplib::Request &req,httplib::Response &res)
  {
    res.set_content(gen().dump(2), "application/json");
  });

  svr.listen("127.0.0.1", 11451);
  return 0;
}
