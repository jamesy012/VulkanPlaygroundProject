cmake_minimum_required(VERSION 3.21.0)
message("----  Starting ----")
project(VulkanPlayground VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)

add_executable(VulkanPlayground ${CPP_FILES} ${HPP_FILES})

add_subdirectory(Libraries/glm EXCLUDE_FROM_ALL)