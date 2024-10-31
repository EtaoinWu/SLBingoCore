#include "ReportGenerator.h"

#include <iostream>
#include "cstypes.h"
#include "unsafe.h"
#include "version.h"
#include <ranges>
#include <map>

using namespace MemRead;
using namespace nlohmann;
using namespace CSharpType;

auto superliminal_version_map = std::map<size_t, std::string>{
  {25210880, "legacy"},
  {25579520, "2020.7.6"},
  {25563136, "2020.11.4"},
  {24654280, "2020.11.4"}, // GOG
  {27049984, "GamePassPC"},
  {26861568, "2020.12.10"}, // downpatch
  {27074560, "2022+"}, // latest
};

struct PointerInfo {
  std::pair<ptrdiff_t, std::vector<ptrdiff_t>> scene;
  std::pair<ptrdiff_t, std::vector<ptrdiff_t>> vague_base;
  std::pair<ptrdiff_t, std::vector<ptrdiff_t>> save_and_checkpoint_manager;
  std::vector<ptrdiff_t> collectible_status;
  std::vector<ptrdiff_t> achievements;
  std::vector<ptrdiff_t> save_game_state;
  std::vector<ptrdiff_t> levels_unlocked;
  std::vector<ptrdiff_t> timer;
  std::vector<ptrdiff_t> cp_timer;
  std::vector<ptrdiff_t> mini_challenge_status;
  std::vector<ptrdiff_t> sacm_folder;
};

auto pointer_info_map = std::map<std::string, std::shared_ptr<PointerInfo>>{
  {"2022+", std::make_shared<PointerInfo>(PointerInfo{
      .scene = {0x183cf10, {0x48, 0x10, 0x0}},
      .vague_base = {0x17D9BA0, {0, 0x1F8, 0x188, 0x1D0, 0x78, 0x10, 0x198}},
      .save_and_checkpoint_manager = {0x17f9d28, {0x8, 0xb0, 0x28}},
      .collectible_status = {0x78, 0x10, 0x18},
      .achievements = {0x80, 0x10},
      .save_game_state = {0xb0},
      .levels_unlocked = {0x60},
      .timer = {0x130},
      .cp_timer = {0x13c},
      .mini_challenge_status = {0x90, 0x10},
      .sacm_folder = {0x50}
    })
  },
  {"2020.12.10", std::make_shared<PointerInfo>(PointerInfo{
      .scene = {0x180b4f8, {0x48, 0x10, 0x0}},
      .vague_base = {0x17C8358, {0xa8, 0xd8, 0x110, 0x30, 0xa8, 0x28}},
      .save_and_checkpoint_manager = {0x17c8588, {0x8, 0xb0, 0x28}},
      .collectible_status = {0x78, 0x10, 0x18},
      .achievements = {0x80, 0x10},
      .save_game_state = {0xa8},
      .levels_unlocked = {0x60},
      .timer = {0x128},
      .cp_timer = {0x134},
      .mini_challenge_status = {0x90, 0x10},
      .sacm_folder = {0x50}
    })
  }
};

json ReportGenerator::level_info() const {
  json result;
  const auto save_game_state = save_and_checkpoint_manager_(this->pointer_info_->save_game_state);
  const auto level_name = (save_game_state + 0x0)->load<CSharpString>();
  const auto cp_name = (save_game_state + 0x8)->load<CSharpString>();
  auto levels_unlocked = save_and_checkpoint_manager_(this->pointer_info_->levels_unlocked)->get<long long>();
  return {
    {"current", trunk_string(level_name)},
    {"n_unlocked", levels_unlocked},
    {"current_cp", trunk_string(cp_name)}
  };
}

json ReportGenerator::timer_info() const {
  auto timer = save_and_checkpoint_manager_(this->pointer_info_->timer)->get<double>();
  auto cp_timer = save_and_checkpoint_manager_(this->pointer_info_->cp_timer)->get<float>();
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
    complain(L"bad Vague"sv);
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
  const auto collectible_status = save_and_checkpoint_manager_(this->pointer_info_->collectible_status);

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
  auto achievements = save_and_checkpoint_manager_(this->pointer_info_->achievements)->load<CSharpSetString>();
  return to_vector(
    achievements | std::views::transform(trunk_string));
}

json ReportGenerator::challenge_mode_info() const {
  auto mini_challenge_status = save_and_checkpoint_manager_(this->pointer_info_->mini_challenge_status)->load<MiniChallengeDict>();
  json result = json::object({});
  for (const auto &[key, value] : mini_challenge_status) {
    result[trunk_string(key)] = to_vector(
      value | std::views::transform(trunk_string));
  }
  return result;
}

void ReportGenerator::sanity_check(const Node &sacm) {
  auto save_folder = untrunk_string(config_.save_folder);
  auto sacm_folder = sacm(this->pointer_info_->sacm_folder)->load<CSharpString>();
  if (sacm_folder.rfind(save_folder, 0) != 0) {
    complain_fmt(L"Bad save address {}"sv, sacm_folder);
  }
}

void ReportGenerator::work(json &result) {
  auto dll_module = std::make_shared<Module>(process_, L"UnityPlayer.dll");
  auto unity_size = dll_module->module_size();
  if (unity_size <= 0) {
    external_error(L"Cannot find UnityPlayer.dll; Game possibly not loaded."sv);
  }
  if (superliminal_version_map.find(unity_size) == superliminal_version_map.end()) {
    external_error_fmt(L"Unsupported UnityPlayer.dll size: {:x}. This version may not be supported."sv, unity_size);
  }
  auto superliminal_version = superliminal_version_map[unity_size];
  result["superliminal_ver"] = superliminal_version;
  if (pointer_info_map.find(superliminal_version) == pointer_info_map.end()) {
    external_error_fmt(L"Unsupported Superliminal version: {}."sv, untrunk_string(superliminal_version));
  }
  auto pointer_info = pointer_info_map[superliminal_version];
  this->pointer_info_ = pointer_info;
  auto dll = Node{dll_module};
  scene_ = dll(pointer_info->scene);
  vague_base_ = dll(pointer_info->vague_base);

  string scene_name;
  try {
    scene_name = scene_->get_astr();
  } catch (const Complaint &c) {
    external_error(L"Game exited or not loaded."sv);
  }

  try {
    save_and_checkpoint_manager_ = dll(pointer_info->save_and_checkpoint_manager);
    save_and_checkpoint_manager_->preload();
    sanity_check(save_and_checkpoint_manager_);
  } catch (const Complaint &c1) {
    try {
      sanity_check(sacm_backup_);
      save_and_checkpoint_manager_ = sacm_backup_;
    } catch (const Complaint &c2) {
      external_error(L"Game exited or not loaded."sv);
    }
  }

  try {
    result["collectible"] = collectible_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error_fmt(L"complain forward: {}", c.wwhat());
  }

  try {
    result["achievements"] = achievement_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error_fmt(L"complain forward: {}", c.wwhat());
  }

  try {
    result["challenge_mode"] = challenge_mode_info();
  } catch (const Complaint &c) {
    std::wcerr << "Complain: " << c.wwhat() << std::endl;
    external_error_fmt(L"complain forward: {}", c.wwhat());
  }

  const string load_name = "Assets/_Levels/_LiveFolder/ACT";

  if (const bool unsafe = scene_->address_opt() == 0 ||
    scene_name.substr(0, load_name.size()) != load_name; !unsafe) {
    try {
      result["timer"] = timer_info();
      result["level"] = level_info();
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
  result["ver"] = SLBINGOCORE_VERSION;
  try {
    if (!process_->exist()) {
      process_->open_process(untrunk_string(config_.game_process));
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
