cmake_minimum_required(VERSION 3.21)

project(discord-rpc)

set(CMAKE_CXX_STANDARD 23)
add_subdirectory(discord-presence)

if (WIN32)
  add_executable(WiiURichPresence src/main.cpp src/resource.rc)
else()
  add_executable(WiiURichPresence src/main.cpp)
endif()

target_include_directories(WiiURichPresence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(WiiURichPresence PRIVATE ${PROJECT_NAME} fmt)
if (UNIX)
  target_link_libraries(WiiURichPresence PRIVATE stdc++ pthread)

  # Link Boost libraries
  find_package(Boost REQUIRED COMPONENTS system)
  target_link_libraries(WiiURichPresence PRIVATE Boost::system)

  # Link OpenSSL libraries
  find_package(OpenSSL REQUIRED)
  target_link_libraries(WiiURichPresence PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  
  # Link CURL
  find_package(CURL REQUIRED)
  target_link_libraries(WiiURichPresence PRIVATE CURL::libcurl)
endif()
