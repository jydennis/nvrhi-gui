cmake_minimum_required(VERSION 4.0)
project(nvrhi_demo CXX)
set(CMAKE_CXX_STANDARD 17)

link_directories("C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64")
#include_directories("C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")

message("cmake_module_path: " ${CMAKE_MODULE_PATH})
message("--")
find_package(glfw3 REQUIRED)

add_subdirectory(nvrhi)

add_executable(${PROJECT_NAME} src/nvrhi_main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE  glfw nvrhi d3dx11 d3d11 nvrhi_d3d11 d3dcompiler)