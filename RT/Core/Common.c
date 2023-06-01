#include <Windows.h>
#include <wchar.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi")

#include "Core/Common.h"
#include "Core/Arena.h"
#include "Core/String.h"

#define SUPPORT_STRING "For support, screenshot this message and visit the #support channel in our discord server:\n https://discord.gg/9dm93hKrnp\nOr create an issue on our GitHub:\nhttps://github.com/BredaUniversityGames/DXX-Raytracer"

void RT_FATAL_ERROR_(const char *explanation, const char *title, const char *file, int line)
{
    char file_stripped[256];
    RT_SaneStrncpy(file_stripped, file, RT_ARRAY_COUNT(file_stripped));

    PathStripPathA(file_stripped);

    char *message = RT_ArenaPrintF(&g_thread_arena, "Location:\n%s:%d\n\nExplanation:\n%s\n\n" SUPPORT_STRING, file_stripped, line, explanation);
    MessageBoxA(NULL, message, title, MB_OK|MB_ICONERROR);

    __debugbreak(); // If you're using a debugger, now is your chance to debug.

    ExitProcess(1); // goodbye forever
}
