cmake_minimum_required(VERSION 3.21)

project(discord-rpc)

set(CMAKE_CXX_STANDARD 23)
add_subdirectory(discord-presence)

if (WIN32 OR MINGW)
  add_executable(WiiURichPresence WIN32 src/main.cpp src/win/resource.rc)
  if(MINGW)
    target_link_libraries(WiiURichPresence PRIVATE winhttp -static-libgcc -static-libstdc++)
  endif()
else()
  add_executable(WiiURichPresence src/main.cpp)
endif()

target_link_libraries(WiiURichPresence PRIVATE ${PROJECT_NAME} fmt)
if (UNIX)
  target_link_libraries(WiiURichPresence PRIVATE stdc++ pthread)

  # Link CURL
  find_package(CURL REQUIRED)
  target_link_libraries(WiiURichPresence PRIVATE CURL::libcurl)
endif()
