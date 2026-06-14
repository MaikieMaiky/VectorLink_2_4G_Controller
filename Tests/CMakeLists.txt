cmake_minimum_required(VERSION 3.20)
project(vectorlink_host_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vectorlink_tests
  test_main.cpp
  ../App/Src/link_quality.cpp
  ../Drivers/User/Src/battery_monitor.cpp
  ../Drivers/User/Src/buttons.cpp
  ../Drivers/User/Src/joystick.cpp
  ../Protocol/Src/control_protocol.cpp
)

target_include_directories(vectorlink_tests PRIVATE
  ../App/Inc
  ../Drivers/User/Inc
  ../Protocol/Inc
)

enable_testing()
add_test(NAME vectorlink_tests COMMAND vectorlink_tests)
