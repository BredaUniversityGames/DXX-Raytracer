# CMakeList.txt : Cmake project for the DirectX12DLL
cmake_minimum_required (VERSION 3.8)

if (MSVC)
    add_compile_options(/W4 /WX /wd4201 /wd4324)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror /wd4201 /wd4324)
endif()
set (CMAKE_COMPILE_WARNING_AS_ERROR ON)

#general settings
set (IMGUI_SOURCE_FILES
    "cimgui/cimgui.cpp"
    "cimgui/imgui/imgui.cpp"
    "cimgui/imgui/imgui_draw.cpp"
    "cimgui/imgui/imgui_demo.cpp"
    "cimgui/imgui/imgui_widgets.cpp"
	"cimgui/imgui/imgui_tables.cpp"
    "cimgui/imgui/backends/imgui_impl_win32.cpp"
    "cimgui/imgui/backends/imgui_impl_dx12.cpp"
)

set (IMPLOT_SOURCE_FILES
    "implot/implot.cpp"
    "implot/implot_items.cpp"
)

set (Renderer_HEADER
"src/RenderBackend.h"
"src/CommandQueue.h"
"src/CommandList.h"
"src/GPUProfiler.h"
"src/Resource.h"
"src/RingBuffer.h"
"src/ShaderTable.h"
"src/GlobalDX.h"
"D3DX/d3dx12.h"
"CGLTF/cgltf.h"
"src/DescriptorArena.hpp"
"src/mikktspace.h"
"DirectXTK12/src/DDSc.h"
)

#I hate cmake for not allowing me to link interface libs. Ah well, now we do this indirection!
set (Renderer_PUBLIC_HEADER
"../Common/include/Renderer.h"
"../Common/include/ApiTypes.h"
"../Common/include/GLTFLoader.h"
"../Common/include/ImageReadWrite.h") 

set (Renderer_SOURCE
"src/Renderer.cpp"
"src/RenderBackend.cpp"
"src/CommandQueue.cpp"
"src/CommandList.cpp"
"src/GPUProfiler.cpp"
"src/Resource.cpp"
"src/RingBuffer.cpp"
"src/ShaderTable.cpp"
"src/GLTFLoader.cpp"
"src/ImageReadWrite.cpp"
"src/DescriptorArena.cpp"
"src/ResourceTracker.cpp"
"src/mikktspace.c"
"src/MeshTracker.cpp"
${IMGUI_SOURCE_FILES}
${IMPLOT_SOURCE_FILES}
)

add_subdirectory("DXC")

# Export all Symbols by default (on Windows creates '.lib'+ '.dll').
set ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

message("Building the renderer as an .dll file.")
#Add source to this project's executable.
add_library (Renderer SHARED 
${Renderer_SOURCE}
${Renderer_HEADER})

install(TARGETS Renderer 
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES  "DXC/bin/x64/dxcompiler.dll" "DXC/bin/x64/dxil.dll" DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${Renderer_PUBLIC_HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#Executable-Object
set_target_properties ( Renderer PROPERTIES
	PUBLIC_HEADER "${Renderer_PUBLIC_HEADER}"
	RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:descent1>
)

set_property(TARGET Renderer PROPERTY CXX_STANDARD 17)

add_compile_definitions(UNICODE=1)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # microsoft is a menace

target_include_directories(Renderer PUBLIC
    "include"
    "assets/shaders/include_shared"
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/cimgui
    ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTK12/src
)
target_include_directories(
    Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cimgui/imgui
    Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implot
    Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTK12/src
)

target_link_libraries(Renderer  
    DX12_ShaderCompiler
    d3d12.lib
    dxgi.lib
    dxguid.lib
    Renderer_Common
    RT_CORE
    "${CMAKE_CURRENT_SOURCE_DIR}/DXC/lib/x64/dxcompiler.lib"
)

add_custom_command(TARGET Renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/DXC/bin/x64/dxcompiler.dll"
        $<TARGET_FILE_DIR:Renderer>)

add_custom_command(TARGET Renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/DXC/bin/x64/dxil.dll"
        $<TARGET_FILE_DIR:Renderer>)

add_executable(Showcase_Renderer
"src/main.cpp")

target_link_libraries(Showcase_Renderer
Renderer)

set_target_properties ( Showcase_Renderer PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:descent1>
)

#copies over external assets if they are changed.
add_subdirectory("assets")