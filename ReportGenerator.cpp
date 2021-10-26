#include "ReportGenerator.h"

#include <iostream>
#include "cstypes.h"
#include "unsafe.h"
#include <ranges>

using namespace MemRead;
using namespace nlohmann;
using namespace CSharpType;

json ReportGenerator::level_info() const {
  json result;
  const auto save_game_state = save_and_checkpoint_manager_[0xa0];
  const auto level_name = (save_game_state + 0x8)->load<CSharpString>();
  auto levels_unlocked = save_and_checkpoint_manager_[0x60]
    ->get<long long>();
  const auto cp_name = (save_game_state + 0x10)->load<CSharpString>();
  return {
    {"current", trunk_string(level_name)},
    {"n_unlocked", levels_unlocked},
    {"current_cp", trunk_string(cp_name)}
  };
}

json ReportGenerator::timer_info() const {
  auto timer = save_and_checkpoint_manager_[0x128]->get<double>();
  auto cp_timer = save_and_checkpoint_manager_[0x134]->get<float>();
  return {
    {"igt", timer},
    {"cp_timer", cp_timer}
  };
}

json ReportGenerator::vague_info() const {
  vague_base_->preload();
  auto vague120 = vague_base_[0x40]->get<float>();
  auto vague15 = vague_base_[0x44]->get<float>();
  auto vague30 = vague_base_[0x48]->get<float>();
  auto vague_state = vague_base_[0x4c]->get<uint32_t>();
  if (abs(vague120 - 120.) > 1e-6
    || abs(vague15 - 15.) > 1e-6
    || abs(vague30 - 30.) > 1e-6
    || !(vague_state == 1 || vague_state == 2 || vague_state == 3)) {
    complain(L"bad Vague");
  }
  
  auto phase1 = vague_base_[0x60]->get<float>();
  auto phase2 = vague_base_[0x64]->get<float>();
  auto phase3 = vague_base_[0x68]->get<float>();
  json result;
  result["limit"] = std::vector{vague120, vague15, vague30};
  result["state"] = vague_state;
  result["now"] = std::vector{phase1, phase2, phase3};
  return result;
}

json ReportGenerator::collectible_info() const {
  json result;
  const auto collectible_status = save_and_checkpoint_manager_[0x78][0x10][
    0x18];

  for (auto [diff, name] :
       {
         std::make_tuple(0x30, "fire_alarm"),
         std::make_tuple(0x48, "extinguisher"),
         std::make_tuple(0x60, "constellation"),
         std::make_tuple(0x78, "chess_piece"),
         std::make_tuple(0x90, "blueprint"),
         std::make_tuple(0xa8, "soda_type"),
         std::make_tuple(0xc0, "actual_egg")
       }) {
    auto cs = collectible_status[diff]
      ->load<CSharpArray<BYTE>>();
    result[name] = to_vector(cs | std::views::transform(
      [](auto x)
      {
        return static_cast<bool>(x);
      }));
  }
  return result;
}

json ReportGenerator::achievement_info() const {
  auto achievements = save_and_checkpoint_manager_
    [0x80][0x10]->load<CSharpSetString>();
  return to_vector(
    achievements | std::views::transform(trunk_string));
}

json ReportGenerator::challenge_mode_info() const {
  auto mini_challenge_status = save_and_checkpoint_manager_
    [0x90][0x10]->load<MiniChallengeDict>();
  json result = json::object({});
  for (const auto &[key, value] : mini_challenge_status) {
    result[trunk_string(key)] = to_vector(
      value | std::views::transform(trunk_string));
  }
  return result;
}

void ReportGenerator::sanity_check(const Node &sacm) {
  auto save_folder = untrunk_string(config_.save_folder);
  auto sacm_folder = sacm[0x50]->load<CSharpString>();
  if (sacm_folder.rfind(save_folder, 0) != 0) {
    complain_fmt(L"Bad save address {}", sacm_folder);
  }
}

void ReportGenerator::work(json &result) {
  string scene_name;
  try {
    scene_name = scene_->get_astr();
  } catch (const Complaint &c) {
    external_error(L"Game exited or not loaded.");
  }

  try {
    save_and_checkpoint_manager_ =
      (make_node<Module>(process_, L"UnityPlayer.dll") + 0x17a9300)
      [0x8][0x8][0xc8][0x118][0x28];
    save_and_checkpoint_manager_->preload();
    sanity_check(save_and_checkpoint_manager_);
  } catch (const Complaint &c) {
    try {
      save_and_checkpoint_manager_ =
        (make_node<Module>(process_, L"UnityPlayer.dll") + 0x17c8588)
        [0x8][0xb0][0x28];
      save_and_checkpoint_manager_->preload();
      sanity_check(save_and_checkpoint_manager_);
    } catch (const Complaint &c1) {
      try {
        sanity_check(sacm_backup_);
        save_and_checkpoint_manager_ = sacm_backup_;
      } catch (const Complaint &c2) {
        external_error(L"Game exited or not loaded.");
      }
    }
  }

  try {
    result["collectible"] = collectible_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error(L"complain forward: " + c.wwhat());
  }

  try {
    result["achievements"] = achievement_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error(L"complain forward: " + c.wwhat());
  }

  try {
    result["challenge_mode"] = challenge_mode_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error(L"complain forward: " + c.wwhat());
  }

  const string load_name = "Assets/_Levels/_LiveFolder/ACT";

  if (const bool unsafe = scene_->address_opt() == 0 ||
    scene_name.substr(0, load_name.size()) != load_name; !unsafe) {
    try {
      result["level"] = level_info();
      result["timer"] = timer_info();
    } catch (const Complaint &c) {
      std::wcerr << "Level Complain: " << c.wwhat() <<
        ", probably level restarting" << std::endl;
      result["complain"].push_back(trunk_string(c.wwhat()));
    }

    try {
      result["vague"] = vague_info();
    } catch (const Complaint &c) {
      result["complain"].push_back(trunk_string(c.wwhat()));
    }
  }
  sacm_backup_ = save_and_checkpoint_manager_->store();
}

json ReportGenerator::operator()() {
  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = now - cached_time_;
  if (abs(diff.count()) < config_.cache_time) {
    return cache_;
  }
  json result;
  result["ver"] = "0.0.3";
  try {
    if (!process_->exist()) {
      process_->open_process(untrunk_string(config_.game_process));
      auto dll = make_node<Module>(process_, L"UnityPlayer.dll");
      scene_ = (dll + 0x180b4f8)[0x48][0x10][0x0];
      vague_base_ = (dll + 0x017C8358)[0xA8][0xD8][0x110][0x30][0xA8][0x28];
    }
    work(result);
  } catch (const ExternalError &e) {
    result["error"] = trunk_string(e.wwhat());
    process_->close();
  }
  cache_ = result;
  cached_time_ = std::chrono::system_clock::now();
  return result;
}
