# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.13)
project ("DXX-Raytracer")
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER_ID, "MSVC")
set(CMAKE_CXX_COMPILER_ID, "MSVC")
set(BUILD_ARGS "-w" "dupbuild=warn")

#Version Control & Identification
message("Build the Descent 1")
add_compile_definitions(PUBLIC  

#This enum decides if we are in the shipping build or not, this is for the future.
#SHIPPING_BUILD 

D1X_RAYTRACER_NAME="D1X_RAYTRACER"

D1X_RAYTRACER_VERSION_MAJORi=1
D1X_RAYTRACER_VERSION_MINORi=1
D1X_RAYTRACER_VERSION_MICROi=0

#DXX-Retro last used version
DXX_VERSION_MAJORi=0 
DXX_VERSION_MINORi=58 
DXX_VERSION_MICROi=1

BASE_SCREEN_SIZE_X=1280
BASE_SCREEN_SIZE_Y=720
BASE_SCREEN_ASPECT_X=4
BASE_SCREEN_ASPECT_Y=3
BASE_SCREEN_WINDOWED=1
BASE_FPS_INDICATOR=1)

#compile Definitions.
if (WIN32)
add_compile_definitions(PUBLIC 
_WIN32 WINDOWS_IGNORE_PACKING_MISMATCH 
WIN32_LEAN_AND_MEAN
NOMINMAX
)
endif ()
if(DEFINED ENV{EDITOR})
message("Using editor")
add_compile_definitions(PUBLIC EDITOR)
endif()

if(${GRAPHICS_API} STREQUAL "DirectX12")
message("Render api is ${GRAPHICS_API}")
add_compile_definitions(PUBLIC RT_DX12)
elseif(${GRAPHICS_API} STREQUAL "OpenGL")
message("Render api is ${GRAPHICS_API}")
add_compile_definitions(PUBLIC OGL)
else()
message(FATAL_ERROR "No graphic API selected!")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
add_compile_definitions(PUBLIC 
DEBUG
_DEBUG)
message(STATUS, "Build type selected: Debug")
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
add_compile_definitions(PUBLIC 
RELEASE
_RELEASE)
message(STATUS, "Build type selected: RelWithDebInfo")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
add_compile_definitions(PUBLIC 
RELEASE
_RELEASE
SHIPPING_BUILD)
message(STATUS, "Build type selected: Release")
else()
message(FATAL_ERROR, "No build type selected!")
endif()

if(DEFINED ENV{QUICK_START})
add_compile_definitions(PUBLIC QUICK_START)
message(STATUS "quick build selected, will immediatly go to level select.")
message(STATUS ${QUICKSTART})
else()
message(STATUS "normal build selected")
message(STATUS ${QUICKSTART})
endif()

add_subdirectory ("sdl-master")
add_subdirectory ("physfs-main")
add_subdirectory ("RT")
add_subdirectory ("d1")
