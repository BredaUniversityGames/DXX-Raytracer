# CMakeList.txt : Cmake project for including the RT code
cmake_minimum_required (VERSION 3.8)

set(ENV{RT_EXT_CODE_DIR}
"${CMAKE_CURRENT_LIST_DIR}"
)

message("RT Dir is: $ENV{RT_EXT_CODE_DIR}")

add_subdirectory("Core")
target_include_directories(RT_CORE INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory("Renderer/Backend/Common")
add_subdirectory("Renderer/Backend/DX12")
