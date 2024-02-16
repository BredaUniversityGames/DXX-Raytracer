# CMakeList.txt : Cmake project for including the RT code
cmake_minimum_required (VERSION 3.8)
add_library(RT_CORE INTERFACE
"Arena.h"
"Common.h"
"VirtualMemory.h"
"MemoryScope.hpp"
"SlotMap.hpp"
"Config.h" 
"String.h" 
"Hash.h"   
"FileIO.h")

target_sources(RT_CORE INTERFACE
"${CMAKE_CURRENT_LIST_DIR}/Config.cpp"
"${CMAKE_CURRENT_LIST_DIR}/Arena.c"
"${CMAKE_CURRENT_LIST_DIR}/FileIO.cpp"
"${CMAKE_CURRENT_LIST_DIR}/Common.c"
"${CMAKE_CURRENT_LIST_DIR}/String.c"
"${CMAKE_CURRENT_LIST_DIR}/VirtualMemory.c"
)