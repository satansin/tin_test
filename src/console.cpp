#include "tin_gen/console.hpp"

#include <cstdio>
#include <iostream>

namespace tin_gen {

void configure_immediate_console_output() {
  std::ios::sync_with_stdio(true);
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

#if defined(_WIN32)
  if (FILE* out = stdout) {
    setvbuf(out, nullptr, _IOLBF, 0);
  }
  if (FILE* err = stderr) {
    setvbuf(err, nullptr, _IONBF, 0);
  }
#else
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
#endif
}

}  // namespace tin_gen
