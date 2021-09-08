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
  auto levels_unlocked = save_and_checkpoint_manager_[0x60]->get<long
    long>();
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
  json result;
  for (const auto &[key, value] : mini_challenge_status) {
    result[trunk_string(key)] = to_vector(
      value | std::views::transform(trunk_string));
  }
  return result;
}

json ReportGenerator::work() const {
  json result;
  string scene_name;
  try {
    scene_name = scene_->get_astr();
    save_and_checkpoint_manager_->preload();
  } catch (const Complaint &c) {
    external_error(L"Game exited or not loaded.");
  }

  const string load_name = "Assets/_Levels/_LiveFolder/ACT";

  if (const bool unsafe = scene_->address_opt() == 0 ||
    scene_name.substr(0, load_name.size()) != load_name; !unsafe) {
    try {
      result["level"] = level_info();
      result["timer"] = timer_info();
    } catch (const Complaint &c) {
      std::wcerr << "Level Complain: " << c.wwhat() << std::endl;
      result["complain"].push_back(trunk_string(c.wwhat()));
    }
  }

  try {
    result["collectible"] = collectible_info();
  } catch (const Complaint &c) {
    result["complain"].push_back(trunk_string(c.wwhat()));
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
  }

  try {
    result["achievements"] = achievement_info();
  } catch (const Complaint &c) {
    result["complain"].push_back(trunk_string(c.wwhat()));
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
  }

  try {
    result["challenge_mode"] = challenge_mode_info();
  } catch (const Complaint &c) {
    result["complain"].push_back(trunk_string(c.wwhat()));
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
  }
  return result;
}

json ReportGenerator::operator()() {
  json result;
  try {
    if (!process_->exist()) {
      process_->open_process(untrunk_string(config_.game_process));
      scene_ =
        (make_node<Module>(process_, L"UnityPlayer.dll") + 0x180b4f8)
        [0x48][0x10][0x0];

      switch (config_.method) {
      case Config::PointerMethod::experimental:
        save_and_checkpoint_manager_ =
          (make_node<Module>(process_, L"UnityPlayer.dll") + 0x17a9300)
          [0x8][0x8][0xc8][0x118][0x28];
        break;

      case Config::PointerMethod::stable:
        save_and_checkpoint_manager_ =
          (make_node<Module>(process_, L"UnityPlayer.dll") + 0x17c8588)
          [0x8][0xb0][0x28];
        break;
      }
    }
    result = work();
  } catch (const ExternalError &e) {
    result["error"] = trunk_string(e.wwhat());
    process_->close();
  }
  return result;
}
