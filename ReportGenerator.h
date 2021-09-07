#pragma once

#include "json.hpp"
#include "memread.h"

using namespace MemRead;
using namespace nlohmann;

class ReportGenerator
{
  Process *process_;
  Node scene_;
  Node save_and_checkpoint_manager_;

  [[nodiscard]] json level_info() const;

  [[nodiscard]] json timer_info() const;

  [[nodiscard]] json collectible_info() const;

  [[nodiscard]] json achievement_info() const;

  [[nodiscard]] json challenge_mode_info() const;

  [[nodiscard]] json work() const;

public:
  ReportGenerator(): process_(new Process),
    scene_(make_node<FixedAddress>(process_, 0)),
    save_and_checkpoint_manager_(make_node<FixedAddress>(process_, 0)) {
  }

  ReportGenerator(const ReportGenerator &rhs) = delete;
  ReportGenerator(ReportGenerator &&rhs) = delete;
  ReportGenerator &operator=(const ReportGenerator &rhs) = delete;
  ReportGenerator &operator=(ReportGenerator &&rhs) = delete;

  ~ReportGenerator() {
    delete process_;
  }

  json operator()();
};
