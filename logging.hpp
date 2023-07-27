#pragma once
#include <iostream>
#define BMCWEB_LOG_DEBUG std::cout
#define BMCWEB_LOG_ERROR std::cout
#define BMCWEB_LOG_CRITICAL std::cout
#define BMCWEB_LOG_INFO std::cout
#define BMCWEB_LOG_WARNING std::cout
inline void pritnFileds(auto &m) {
  for (auto &p : m) {
    std::cout << p.name() << ": " << p.value() << "\n";
  }
}
