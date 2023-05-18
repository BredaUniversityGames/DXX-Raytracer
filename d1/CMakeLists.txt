# CMakeList.txt : CMake project for D1-Raytraced, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.13)
project(descent1)
add_compile_definitions(PUBLIC  
HAVE_STRUCT_TIMEVAL
IPv6
NETWORK
NO_ASM
WINDOWS_IGNORE_PACKING_MISMATCH 
USE_TRACKER)

if(${GRAPHICS_API} STREQUAL "DirectX12")
	#Raytracing interface Source code
set(Graphic_Includes
	"$ENV{RT_EXT_CODE_DIR}/RTgr.c"
	"$ENV{RT_EXT_CODE_DIR}/RTgr.h"
	"$ENV{RT_EXT_CODE_DIR}/Game/Level.h"
	"$ENV{RT_EXT_CODE_DIR}/Game/Level.c"
	"$ENV{RT_EXT_CODE_DIR}/Game/Lights.h"
	"$ENV{RT_EXT_CODE_DIR}/Game/Lights.c"
	"$ENV{RT_EXT_CODE_DIR}/dx12.c"
	"$ENV{RT_EXT_CODE_DIR}/dx12.h"
	"$ENV{RT_EXT_CODE_DIR}/polymodel_viewer.cpp"
	"$ENV{RT_EXT_CODE_DIR}/polymodel_viewer.h"
	"$ENV{RT_EXT_CODE_DIR}/material_viewer.cpp"
	"$ENV{RT_EXT_CODE_DIR}/material_viewer.h"
	"$ENV{RT_EXT_CODE_DIR}/RTmaterials.c"
	"$ENV{RT_EXT_CODE_DIR}/RTmaterials.h"
)
set (Graphic_Libs
Renderer
)
elseif(${GRAPHICS_API} STREQUAL "OpenGL")
set(Graphic_Includes
	"arch/ogl/gr.c"
	"arch/ogl/ogl.c"
)
set (Graphic_Libs
	opengl32
	glu32
)
endif()

if(DEFINED ENV{EDITOR})
file(GLOB EDITOR_SOURCES
     "editor/*.c"
)

file(GLOB UI_SOURCES
     "ui/*.c"
)
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
#disable console on release.
add_executable(descent1 WIN32)
else()
add_executable(descent1)
endif()

target_sources(descent1 PRIVATE
	${UI_SOURCES}
	${EDITOR_SOURCES}

	#non-raytracing extensions on the engine.
	$ENV{RT_EXT_CODE_DIR}/RText.c
	$ENV{RT_EXT_CODE_DIR}/RText.h

	"2d/2dsline.c"
	"2d/bitblt.c"
	"2d/bitmap.c"
	"2d/box.c"
	"2d/canvas.c"
	"2d/circle.c"
	"2d/disc.c"
	"2d/font.c"
	"2d/gpixel.c"
	"2d/line.c"
	"2d/palette.c"
	"2d/pcx.c"
	"2d/pixel.c"
	"2d/poly.c"
	"2d/rect.c"
	"2d/rle.c"
	"2d/scalec.c"
	"3d/clipper.c"
	"3d/draw.c"
	"3d/globvars.c"
	"3d/instance.c"
	"3d/interp.c"
	"3d/matrix.c"
	"3d/points.c"
	"3d/rod.c"
	"3d/setup.c"
	"arch/sdl/event.c"
	"arch/sdl/init.c"
	"arch/sdl/joy.c"
	"arch/sdl/key.c"
	"arch/sdl/mouse.c"
	"arch/sdl/rbaudio.c"
	"arch/sdl/timer.c"
	"arch/sdl/window.c"
	"arch/sdl/digi.c"
	"arch/sdl/digi_audio.c"
	"iff/iff.c"
	"main/ai.c"
	"main/aipath.c"
	"main/automap.c"
	"main/bm.c"
	"main/bmread.c"
	"main/cntrlcen.c"
	"main/collide.c"
	"main/config.c"
	"main/console.c"
	"main/controls.c"
	"main/credits.c"
	"main/custom.c"
	"main/digiobj.c"
	"main/dumpmine.c"
	"main/effects.c"
	"main/endlevel.c"
	"main/fireball.c"
	"main/fuelcen.c"
	"main/fvi.c"
	"main/game.c"
	"main/gamecntl.c"
	"main/gamefont.c"
	"main/gamemine.c"
	"main/gamerend.c"
	"main/gamesave.c"
	"main/gameseg.c"
	"main/gameseq.c"
	"main/gauges.c"
	"main/hostage.c"
	"main/hud.c"
	"main/inferno.c"
	"main/kconfig.c"
	"main/kmatrix.c"
	"main/laser.c"
	"main/lighting.c"
	"main/menu.c"
	"main/mglobal.c"
	"main/mission.c"
	"main/morph.c"
	"main/multi.c"
	"main/multibot.c"
	"main/newdemo.c"
	"main/newmenu.c"
	"main/object.c"
	"main/paging.c"
	"main/physics.c"
	"main/piggy.c"
	"main/player.c"
	"main/playsave.c"
	"main/polyobj.c"
	"main/powerup.c"
	"main/render.c"
	"main/robot.c"
	"main/scores.c"
	"main/slew.c"
	"main/snddecom.c"
	"main/songs.c"
	"main/state.c"
	"main/switch.c"
	"main/terrain.c"
	"main/texmerge.c"
	"main/text.c"
	"main/titles.c"
	"main/vclip.c"
	"main/wall.c"
	"main/weapon.c"
	"maths/fixc.c"
	"maths/rand.c"
	"maths/tables.c"
	"maths/vecmat.c"
	"mem/mem.c"
	"misc/args.c"
	"misc/dl_list.c"
	"misc/error.c"
	"misc/hash.c"
	"misc/hmp.c"
	"misc/ignorecase.c"
	"misc/physfsx.c"
	"misc/strio.c"
	"misc/strutil.c"
	"misc/logger.c"
	"texmap/ntmap.c"
	"texmap/scanline.c"

	#network
	"main/net_udp.c"
	
	${Graphic_Includes}
)



target_include_directories(descent1 PRIVATE
"include"
"include/editor"
"main"
"arch/include"
"arch"
"3d"
"../SDL"
"../sdl-master"
"../sdl-master/sdl"
"../sdl-master/sdl/include"
"../physfs-main/src"
$ENV{RT_EXT_CODE_DIR}
)

target_link_libraries(descent1 
physfs-static
sdlmain
sdl
dxguid
Winmm
Ws2_32
dinput8
${Graphic_Libs}
)

install(TARGETS descent1             
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(${GRAPHICS_API} STREQUAL "DirectX12")
#copy shaders when a change happened.
add_custom_target(game_copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/RT/Renderer/Backend/DX12/assets/shaders"
    ${PROJECT_BINARY_DIR}/assets/shaders
    COMMENT "Copying DX12 asset folder to output d1 folder")

add_dependencies(descent1 game_copy_shaders)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/RT/Renderer/Backend/DX12/assets/shaders" DESTINATION "${CMAKE_INSTALL_BINDIR}/assets")


add_custom_target(game_copy_textures ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/RT/Renderer/Backend/DX12/assets/textures"
    ${PROJECT_BINARY_DIR}/assets/textures
    COMMENT "Copying DX12 asset folder to output d1 folder")

add_dependencies(descent1 game_copy_textures)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/RT/Renderer/Backend/DX12/assets/textures" DESTINATION "${CMAKE_INSTALL_BINDIR}/assets")

#copy assets when a change happened.
add_custom_target(game_copy_custom_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/assets"
    ${PROJECT_BINARY_DIR}/assets
    COMMENT "Copying Descent custom assets folder to output d1 folder")

add_dependencies(descent1 game_copy_custom_assets)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/d1/assets" DESTINATION ${CMAKE_INSTALL_BINDIR})
else()

#copy only the splash screens for OpenGL.
add_custom_target(game_copy_splash_logo ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/assets/splash-logo"
    ${PROJECT_BINARY_DIR}/assets/splash-logo
    COMMENT "Copying Descent custom assets folder to output d1 folder")

	
install(DIRECTORY "${PROJECT_SOURCE_DIR}/d1/assets/splash-logo" DESTINATION ${CMAKE_INSTALL_BINDIR}/assets/splash-logo)

add_dependencies(descent1 game_copy_splash_logo)
endif()



if(EXISTS "${CMAKE_SOURCE_DIR}/descent.hog")
add_custom_command(TARGET descent1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/descent.hog"
        $<TARGET_FILE_DIR:descent1>
		COMMENT "copying descent.hog to output d1 folder")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/descent.pig")
add_custom_command(TARGET descent1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/descent.pig"
        $<TARGET_FILE_DIR:descent1>
		COMMENT "copying descent.pig to output d1 folder")
endif()