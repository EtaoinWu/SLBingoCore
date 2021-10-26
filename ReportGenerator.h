#pragma once

#include "Config.h"
#include "json.hpp"
#include "memread.h"
#include <chrono>

using namespace MemRead;
using namespace nlohmann;

class ReportGenerator
{
  Config config_;
  Process *process_;
  Node scene_;
  Node save_and_checkpoint_manager_;
  Node vague_base_;
  Node sacm_backup_;
  json cache_;
  std::chrono::time_point<std::chrono::system_clock> cached_time_;

  [[nodiscard]] json level_info() const;

  [[nodiscard]] json timer_info() const;

  [[nodiscard]] json vague_info() const;

  [[nodiscard]] json collectible_info() const;

  [[nodiscard]] json achievement_info() const;

  [[nodiscard]] json challenge_mode_info() const;

  void sanity_check(const Node &sacm);

  void work(json &result);

public:
  explicit ReportGenerator(const Config &config): config_(config),
    process_(new Process),
    scene_(make_node<FixedAddress>(process_, 0)),
    save_and_checkpoint_manager_(make_node<FixedAddress>(process_, 0)),
    vague_base_(make_node<FixedAddress>(process_, 0)),
    sacm_backup_(make_node<FixedAddress>(process_, 0)),
    cache_(nullptr),
    cached_time_(std::chrono::system_clock::now() - std::chrono::seconds{1}) {
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
