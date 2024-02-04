cmake_minimum_required(VERSION 3.16.3)
project(ITCH50_Hourly_VWAP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20")

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.36.0 COMPONENTS iostreams REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_executable ( ITCH50_Hourly_VWAP main.cpp Message.cpp )
  target_link_libraries(ITCH50_Hourly_VWAP ${Boost_LIBRARIES})
endif()
