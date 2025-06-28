cmake_minimum_required(VERSION 3.10)
project(WiiURichPresence)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if(WIN32)
    add_executable(WiiURichPresence main.cpp app.rc)
else()
    add_executable(WiiURichPresence main.cpp)
endif()

# Define some handy Social SDK variables
set(DISCORD_SDK_ROOT "${CMAKE_SOURCE_DIR}/lib/discord_social_sdk")
set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

# Include for Social SDK headers
target_include_directories(WiiURichPresence PRIVATE ${DISCORD_SDK_INCLUDE_DIR})
target_include_directories(WiiURichPresence PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Platform-specific Social SDK library paths
if(WIN32)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/discord_partner_sdk.lib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_DIR}/discord_partner_sdk.dll")
elseif(APPLE)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
else() # Linux
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
endif()

# Link the Social SDK library
target_link_libraries(WiiURichPresence PRIVATE ${DISCORD_LIB_PATH})

# Set the runtime search path (RPATH) for Linux and macOS
if(UNIX)
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
    # Use RPATH when building
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # Set the RPATH to use the lib directory relative to the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    endif()
endif()

# Copy Social SDK shared library to output directory, so it's available at runtime.
add_custom_command(TARGET WiiURichPresence POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DISCORD_SHARED_LIB}"
    $<TARGET_FILE_DIR:WiiURichPresence>
)

if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG)
        string (REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()
