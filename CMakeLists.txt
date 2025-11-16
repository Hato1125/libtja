cmake_minimum_required(VERSION 3.20)

project(libtja CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TTFS_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  ttfs
  GIT_REPOSITORY https://github.com/Hato1125/ttfs.git
  GIT_TAG main
)

FetchContent_Declare(
  simde
  GIT_REPOSITORY https://github.com/simd-everywhere/simde.git
  GIT_TAG master
)

FetchContent_MakeAvailable(ttfs simde)

add_library(libtja INTERFACE)

target_link_libraries(libtja INTERFACE ttfs)

target_include_directories(libtja INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${simde_SOURCE_DIR}
)

target_compile_definitions(libtja INTERFACE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
  $<$<CONFIG:Release>:RELEASE_BUILD>
)

set(SANDBOX_DIR ${CMAKE_SOURCE_DIR}/sandbox)
set(SANDBOX_CMAKELISTS ${SANDBOX_DIR}/CMakeLists.txt)

if(EXISTS ${SANDBOX_DIR} AND EXISTS ${SANDBOX_CMAKELISTS})
  add_subdirectory(${SANDBOX_DIR})
endif()
