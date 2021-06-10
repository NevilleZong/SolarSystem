cmake_minimum_required(VERSION 3.19)

project(SolarSystem)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(main main.c glm.c)

target_link_libraries(main m glut GLU GL)
