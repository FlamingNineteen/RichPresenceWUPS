cmake_minimum_required(VERSION 3.21)

project(discord-rpc)

set(CMAKE_CXX_STANDARD 23)
include(cmake/CPM.cmake)

add_library(${PROJECT_NAME} STATIC rpc/discord-rpc.cpp rpc/serialization.cpp rpc/command-queue.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Windows has some extra code
if (WIN32)
  target_sources(${PROJECT_NAME} PRIVATE rpc/platform/windows.cpp)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
endif()

if (NOT TARGET fmt::fmt)
  CPMAddPackage("gh:fmtlib/fmt#11.2.0")
endif()
if (NOT TARGET glaze::glaze)
  CPMAddPackage("gh:stephenberry/glaze@5.5.4")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE fmt glaze::glaze)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(DISCORD_RPC_BUILD_TESTS ON)
endif()

if (DISCORD_RPC_BUILD_TESTS)
  add_subdirectory(src)
endif()