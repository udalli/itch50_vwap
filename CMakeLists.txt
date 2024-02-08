cmake_minimum_required(VERSION 3.16.3)

project(ITCH50_Hourly_VWAP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_BUILD_TYPE release) # TODO check if release is O2 or O3?
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

find_package(Boost 1.36.0 COMPONENTS iostreams REQUIRED)

if(Boost_FOUND)
  set(Boost_USE_STATIC_LIBS on) # only find static libs
  set(Boost_USE_MULTITHREADED on)
  set(Boost_USE_STATIC_RUNTIME off)

  include_directories(${Boost_INCLUDE_DIRS})
  add_executable(ITCH50_Hourly_VWAP main.cpp Message.cpp)
  target_link_libraries(ITCH50_Hourly_VWAP ${Boost_LIBRARIES})
endif()
