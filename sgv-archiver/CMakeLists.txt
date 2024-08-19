cmake_minimum_required (VERSION 3.8)

project(sgv-archiver)

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER_ID, "MSVC")
set(CMAKE_CXX_COMPILER_ID, "MSVC")

message("Build the SFV Archiver")

add_executable(sgv-archiver)

target_sources(sgv-archiver PRIVATE
	
	"main.cpp"
	"sgv-header.h"
	"sgv-index-entry.h"
	
)