#Cmake for all the third_party libraries, they will be compiled or collected here.
cmake_minimum_required (VERSION 3.8)

add_library(DX12_ShaderCompiler INTERFACE
"inc/dxcapi.h"
"inc/d3d12shader.h")
target_include_directories(DX12_ShaderCompiler INTERFACE ${CMAKE_CURRENT_LIST_DIR})