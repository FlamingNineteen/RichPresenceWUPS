cmake_minimum_required(VERSION 3.21)

add_executable(${PROJECT_NAME}-test main.cpp)
target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} fmt)