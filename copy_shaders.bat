@echo off

echo copying shaders!

set SHADER_SOURCE=RT\Renderer\Backend\DX12\assets\shaders
set BUILD_DIR_BASE=out\build

rem NOTE(daniel): The lock file is there to prevent the code from reloading shaders while the directory is still
rem being copied.

rem This script is made slightly hard on the eyes by the fact that there are multiple directories to copy to.

echo lock > %SHADER_SOURCE%\lock_file.temp

robocopy %SHADER_SOURCE% %BUILD_DIR_BASE%\directx12-win-debug\d1\assets\shaders   /MIR > nul
del %BUILD_DIR_BASE%\directx12-win-debug\d1\assets\shaders\lock_file.temp

echo lock > %BUILD_DIR_BASE%\directx12-win-release\d1\assets\shaders\lock_file.temp
robocopy %SHADER_SOURCE% %BUILD_DIR_BASE%\directx12-win-release\d1\assets\shaders /MIR > nul
del %BUILD_DIR_BASE%\directx12-win-release\d1\assets\shaders\lock_file.temp

del %SHADER_SOURCE%\lock_file.temp
