cmake_minimum_required (VERSION 3.8)

project(gav-archiver)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_C_COMPILER_ID, "MSVC")
set(CMAKE_CXX_COMPILER_ID, "MSVC")

message("Build the GAV Archiver")

add_executable(gav-archiver)

target_sources(gav-archiver PRIVATE
	
	"main.cpp"
	
)