#include "tin_gen/app.hpp"
#include "tin_gen/console.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
  tin_gen::configure_immediate_console_output();

  try {
    const auto request = tin_gen::parse_app_request(argc, argv);
    if (!request) {
      return EXIT_SUCCESS;
    }
    return tin_gen::run_app(*request);
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
