@echo off

echo copying textures!

set TEXTURE_SOURCE=d1\assets\textures
set BUILD_DIR_BASE=out\build

echo lock > %TEXTURE_SOURCE%\lock_file.temp

robocopy %TEXTURE_SOURCE% %BUILD_DIR_BASE%\directx12-win-debug\d1\assets\textures   /MIR > nul
del %BUILD_DIR_BASE%\directx12-win-debug\d1\assets\textures\lock_file.temp

echo lock > %BUILD_DIR_BASE%\directx12-win-release\d1\assets\textures\lock_file.temp
robocopy %TEXTURE_SOURCE% %BUILD_DIR_BASE%\directx12-win-release\d1\assets\textures /MIR > nul
del %BUILD_DIR_BASE%\directx12-win-release\d1\assets\textures\lock_file.temp

del %TEXTURE_SOURCE%\lock_file.temp
