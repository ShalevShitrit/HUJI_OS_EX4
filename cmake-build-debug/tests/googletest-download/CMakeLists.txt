cmake_minimum_required(VERSION 2.8.2)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           main
  SOURCE_DIR        "/mnt/c/Users/omer1/Desktop/degree/year 2/sem 2/OS/HUJI_OS_EX4/cmake-build-debug/tests/googletest-src"
  BINARY_DIR        "/mnt/c/Users/omer1/Desktop/degree/year 2/sem 2/OS/HUJI_OS_EX4/cmake-build-debug/tests/googletest-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
