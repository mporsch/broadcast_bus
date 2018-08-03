cmake_minimum_required (VERSION 3.0.0)

project (message_bus CXX)
add_executable (message_bus_test message_bus_test.cpp message_bus.h)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries (message_bus_test pthread)
endif()
