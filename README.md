![scrn0003](https://github.com/BredaUniversityGames/DXX-Raytracer/assets/34250026/2acfa740-8f79-4e78-a977-02a4fc3d79b9)

# DXX Raytracer
DXX Raytracer is a fork of the [DXX Retro](https://github.com/CDarrow/DXX-Retro) project for Windows. DXX Raytracer uses modern raytracing techniques to update the graphics of the beloved retro game known as Descent.



## Installation
[Download dxx_raytracer.zip here.](https://github.com/BredaUniversityGames/DXX-Raytracer/releases/latest) 

Extract the zip file to a location of your choosing, and just run descent1.exe to play! This build of the game uses the shareware assets. If you have bought the original version of Descent 1995, you can replace the descent.pig and descent.hog files with your versions to play the whole game.

## Features
- Physically-based rendering
- Soft shadows
- Pathtraced global illumination
- Bloom
- Temporal anti-aliasing
- Motion blur
- Post-processing (Vignette, tonemap, etc.)

## Community discord
You can join the community discord to make suggestions, report bugs, or just to hang around: https://discord.gg/vaEH5ryjvc

## Instructions
- SHIFT + ALT + F1: Open debug menus, more on those below
- SHIFT + ALT + F2: Toggle depth testing for debug lines
- SHIFT + ALT + F: Toggle free camera option
- F1: Menu with important keybindings from the game
- ALT + F2: Save your game
- ALT + F3: Load a save game

## Debug Menus
- Render Settings: All kinds of graphics settings and debug utility for rendering
- GPU Profiler: A profiling tool to see how much time individual render passes took on the GPU
- Material Editor: Adjust material properties from any level
- Polymodel Viewer: Allows you to view a polymodel in the current scene.
- Dynamic Lights: Allows for tweaking of dynamic lights, like weapons, explosions, and muzzle flashes.
- Light Explorer: Allows for tweaking of individual lights, and adds ability to debug view lights in the level.

## Texture compression - generating DDS textures
To create DDS textures: Download NVIDIA's https://developer.nvidia.com/gpu-accelerated-texture-compression to compress the textures. We have prepared a script for you to convert a batch of textures for you to the correct format and settings. You can run the python script in a command line python PNGToDDS.py with a path to a directory to generate an nvtt file. That file can then be executed with nvtt_export --batch "DXX_Raytracer_Convert_PNG_To_DDS.nvtt". That will generate DDS files for all PNG files in that directory that follow our naming conventions (filename_basecolor.png, filename_normal.png, filename_metallic.png, filename_roughness.png, and filename_emissive.png) and use the correct settings for each of these types. Make sure you generate DDS textures with mips.

## Team
- [Sam Boots](https://github.com/samboots) - Lead Programmer/Graphics Programmer
- [Justin Kujawa](https://jkujawa.com/) - Graphics Programmer
- [Lyubomir Kostadinov](https://github.com/lyubokostadinov) - Graphics Programmer
- [Daniël Cornelisse](https://github.com/TheSandvichMaker) - Graphics Programmer
- [Stan Vogels](http://www.stanvogels.nl/) - Engine Programmer
- [Wessel Frijters](https://www.wesselfrijters.com/) - Engine Programmer
- [Kylian Dekker](https://www.kyliandekker.com/) - Engine Programmer/Audio Programmer
- [Lily Haverlag](https://flannyh.github.io/portfolio/) - Engine Programmer
