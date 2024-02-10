cmake_minimum_required(VERSION 3.0)

project(ITCH50_Hourly_VWAP)

set(Boost_USE_STATIC_LIBS off)
set(Boost_USE_MULTITHREADED off)
set(Boost_USE_STATIC_RUNTIME off)

find_package(Boost 1.36.0 COMPONENTS container iostreams REQUIRED)

set(CMAKE_BUILD_TYPE release) # TODO check if relwithdebinfo is O2 or O3?
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++20")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_INCLUDE_CURRENT_DIR on)
set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
int main()
{
  char c = 0;
  const char * cp = &c;
  __builtin_prefetch(cp, 0, 0);
  return 0;
}
" COMPILER_SUPPORTS_BUILTIN_PREFETCH)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <xmmintrin.h>
#endif

int main()
{
  char c = 0;
  const char * cp = &c;
  _mm_prefetch(cp, _MM_HINT_NTA);
  return 0;
}
" COMPILER_SUPPORTS_MM_PREFETCH)

if (COMPILER_SUPPORTS_BUILTIN_PREFETCH)
  add_compile_definitions(COMPILER_SUPPORTS_BUILTIN_PREFETCH)
endif()

if (COMPILER_SUPPORTS_MM_PREFETCH)
  add_compile_definitions(COMPILER_SUPPORTS_MM_PREFETCH)
endif()

include_directories(${Boost_INCLUDE_DIRS})
add_executable(ITCH50_Hourly_VWAP main.cpp Message.cpp)
target_link_libraries(ITCH50_Hourly_VWAP ${Boost_LIBRARIES})
