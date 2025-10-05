cmake_minimum_required(VERSION 3.21)

add_executable(WiiURichPresence main.cpp)
target_link_libraries(WiiURichPresence PRIVATE ${PROJECT_NAME} fmt)