#include <iomanip>
#include <iostream>
#include "ReportGenerator.h"

int main() {
  ReportGenerator gen;

  while (true) {
    std::wcout << "===================================" << std::endl;
    
    std::cout << std::setw(2) << gen() << std::endl;
    Sleep(500);
  }
  return 0;
}
