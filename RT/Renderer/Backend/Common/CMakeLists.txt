# CMakeList.txt : Cmake project for the common renderer headers.
cmake_minimum_required (VERSION 3.8)

#for now no headers due to the export headers for the dll. However, we still want the include directories.
add_library(Renderer_Common INTERFACE) 

target_include_directories(Renderer_Common INTERFACE
"${CMAKE_CURRENT_LIST_DIR}/include")