cmake_minimum_required(VERSION 3.10)

project(testApp VERSION 1.0)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

file(GLOB SOURCES "*.cpp")

add_executable(testApp ${SOURCES})

target_include_directories(testApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(testApp PRIVATE "$ENV{VULKAN_SDK}/include")
target_include_directories(testApp PRIVATE "/home/trw/Documents/Libraries/glm")
target_include_directories(testApp PRIVATE "/home/trw/Documents/Libraries/glfw/include/GLFW")

target_link_directories(testApp PRIVATE "$ENV{VULKAN_SDK}/lib")
target_link_libraries(testApp PRIVATE vulkan)

target_link_directories(testApp PRIVATE "/home/trw/Documents/Libraries/glfw/build/src")
target_link_libraries(testApp PRIVATE glfw3)

