find_package(Qhull REQUIRED)

if(NOT TARGET Qhull::qhullcpp)
  message(FATAL_ERROR
    "Qhull C++ library not found. Install it:\n"
    "  macOS:  brew install qhull\n"
    "  Linux:  sudo apt install libqhull-dev")
endif()
