#include <iostream>

#include "memread.h"
#include <vector>

int main() {
  using namespace MemRead;
  auto process = new Process;
  process->open_process(L"SuperliminalSteam.exe");

  auto save_and_checkpoint_manager =
    (make_node<Module>(process, L"UnityPlayer.dll") + 0x17c8588)
    [0x8][0xb0][0x28];

  while (true) {
    auto statusChessPiece = save_and_checkpoint_manager
      [0x78][0x10][0x18][0x78][0x20]->get_array<BYTE>(15);
    for (bool x : statusChessPiece) {
      std::cout << x << ' ';
    }
    std::cout << std::endl;
    Sleep(100);
  }
  return 0;
}
