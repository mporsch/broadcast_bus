cmake_minimum_required (VERSION 3.0.0)

project (broadcast_bus CXX)
add_executable (broadcast_bus_test broadcast_bus_test.cpp broadcast_bus.h broadcast_bus_impl.h)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries (broadcast_bus_test pthread)
endif()
