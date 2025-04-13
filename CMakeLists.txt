cmake_minimum_required(VERSION 4.0)
project(nvrhi_demo CXX)

link_directories("C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64")
include_directories("C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")

message("cmake_module_path: " ${CMAKE_MODULE_PATH})
message("--")
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw d3d11 d3dx11)