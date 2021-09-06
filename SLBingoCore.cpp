#include <iostream>
#include <numeric>

#include "memread.h"
#include "cstypes.h"
#include <vector>

int main() {
  using namespace MemRead;
  using namespace CSharpType;
  auto process = new Process;
  process->open_process(L"SuperliminalSteam.exe");

  auto save_and_checkpoint_manager =
    (make_node<Module>(process, L"UnityPlayer.dll") + 0x17c8588)
    [0x8][0xb0][0x28];

  while (true) {
    system("cls");
    save_and_checkpoint_manager->load();

    auto save_game_state = save_and_checkpoint_manager[0xa0];
    auto level_name = (save_game_state + 0x8)->load<CSharpString>();
    auto levels_unlocked = save_and_checkpoint_manager[0x60]->get<long long>();
    auto cp_name = (save_game_state + 0x10)->load<CSharpString>();

    std::wcout << std::format(L"Level: {} ({} unlocked), CP: {}", level_name,
                              levels_unlocked, cp_name) << std::endl;

    auto timer = save_and_checkpoint_manager[0x128]->get<double>();
    std::cout << "Timer: " << timer << std::endl;

    auto cp_timer = save_and_checkpoint_manager[0x134]->get<float>();
    std::cout << "CP timer: " << cp_timer << std::endl;

    auto collectible_status = save_and_checkpoint_manager[0x78][0x10][0x18];
    for (auto [diff, name] :
         {
           std::make_tuple(0x30, "fire alarm"),
           std::make_tuple(0x48, "extinguisher"),
           std::make_tuple(0x60, "constellation"),
           std::make_tuple(0x78, "chess piece"),
           std::make_tuple(0x90, "blueprint"),
           std::make_tuple(0xa8, "soda type"),
           std::make_tuple(0xc0, "actual egg")
         }) {
      auto status_collectible = collectible_status[diff]
        ->load<CSharpArray<BYTE>>();
      std::cout << name << " (" << std::accumulate(
        status_collectible.begin(), status_collectible.end(), 0) << "): ";
      for (bool x : status_collectible) {
        std::cout << x;
      }
      std::cout << std::endl;
    }

    auto achievements = save_and_checkpoint_manager
      [0x80][0x10]->load<CSharpSetString>();
    std::wcout << "Achievements (" << achievements.size() << "): [";
    for (const auto &s : achievements) {
      std::wcout << s << ", ";
    }
    std::wcout << "]" << std::endl;

    auto mini_challenge_status = save_and_checkpoint_manager
      [0x90][0x10]->load<MiniChallengeDict>();
    std::wcout << "Challenge mode:" << std::endl;
    for (const auto &[chapter, finished_challenges] : mini_challenge_status) {
      std::wcout << std::format(L"  {} ({}): [", chapter,
                                finished_challenges.size());
      for (const auto &s : finished_challenges) {
        std::wcout << s << ", ";
      }
      std::wcout << "]" << std::endl;
    }
    Sleep(500);
  }
  return 0;
}
