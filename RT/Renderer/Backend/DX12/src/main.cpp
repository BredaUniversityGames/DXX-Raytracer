// ------------------------------------------------------------------
// DXR Renderer Test Application

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------------
// Dear ImGui

#include <backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ------------------------------------------------------------------

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = ".\\D3D12\\"; }

#include "Core/Common.h"
#include "Core/Arena.h"
#include "Core/MiniMath.h"
#include "GLTFLoader.h"

#include "Renderer.h"

static HWND g_hwnd;
static BOOL g_running;
static LARGE_INTEGER g_perf_freq;
static bool g_capture_mouse;

static void Win32_FatalError(const wchar_t *message)
{
    MessageBoxW(g_hwnd, message, L"Fatal Error", MB_OK);
    ExitProcess(1);
}

// TODO: FormatError for talking about HRESULT errors

static LARGE_INTEGER Win32_GetTime(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);

    return result;
}

static float Win32_GetTimeElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    return (float)((double)(end.QuadPart - start.QuadPart) / (double)g_perf_freq.QuadPart);
}

typedef struct RT_Button
{
    bool down;
    bool pressed;
    bool released;
} RT_Button;

typedef enum RT_ButtonKind
{
    RT_BUTTON_LMB,
    RT_BUTTON_RMB,

    RT_BUTTON_LEFT,
    RT_BUTTON_RIGHT,
    RT_BUTTON_FORWARD,
    RT_BUTTON_BACKWARD,
    RT_BUTTON_UP,
    RT_BUTTON_DOWN,
    RT_BUTTON_SPRINT,
    RT_BUTTON_CRAWL,
    RT_BUTTON_KILL_MOMENTUM,
    RT_BUTTON_FLASHLIGHT,

    RT_BUTTON_ESCAPE,

    RT_BUTTON_COUNT,
} RT_ButtonKind;

typedef struct RT_Input
{
    RT_Vec2 mouse_p;
    RT_Vec2 mouse_dp; // delta position
    float   mouse_scroll;

    RT_Button buttons[RT_BUTTON_COUNT];
} RT_Input;

static RT_Input g_input;

static inline void RT_ProcessButton(RT_ButtonKind kind, bool down)
{
	RT_Button* button = &g_input.buttons[kind];
	button->pressed  =  down && (down != button->down);
	button->released = !down && (down != button->down);
	button->down = down;
}

static inline void RT_NewInputFrame(void)
{
    g_input.mouse_scroll = 0;

    for (int i = 0; i < RT_BUTTON_COUNT; i++)
    {
		g_input.buttons[i].pressed  = false;
		g_input.buttons[i].released = false;
    }
}

static inline bool RT_ButtonPressed(RT_ButtonKind kind)
{
    return g_input.buttons[kind].pressed;
}

static inline bool RT_ButtonReleased(RT_ButtonKind kind)
{
    return g_input.buttons[kind].released;
}

static inline bool RT_ButtonDown(RT_ButtonKind kind)
{
    return g_input.buttons[kind].down;
}

typedef struct RT_FreeCam
{
    float yaw;
    float target_yaw;

    float pitch;
    float target_pitch;

    RT_Vec3 velocity;

    float target_fov;
    float fov;
} RT_FreeCam;

static float g_curr_time;
static bool  g_camera_physics;
static float g_camera_stiffness = 2.0f;
static float g_camera_friction  = 1.0f;
static float g_camera_bob;

static void RT_UpdateFreecam(RT_FreeCam *freecam, RT_Camera *camera, float dt)
{
    bool physics = g_camera_physics;

    if (g_capture_mouse)
    {
        if (RT_ButtonDown(RT_BUTTON_KILL_MOMENTUM))
        {
			freecam->target_yaw = freecam->yaw;
			freecam->target_pitch = freecam->pitch;
            physics = false;
        }
    }

    float look_speed = 0.5f;

    float reference_fov = 60.0f;
    float fov_ratio = camera->vfov / reference_fov;

    float scaled_look_speed = fov_ratio*look_speed;

    float mouse_dx = scaled_look_speed*g_input.mouse_dp.x;
    float mouse_dy = scaled_look_speed*g_input.mouse_dp.y;

    freecam->target_yaw   += mouse_dx;
    freecam->target_pitch += mouse_dy;

    freecam->target_pitch = RT_CLAMP(freecam->target_pitch, -88.0f, 88.0f);

    if (physics)
    {
		freecam->yaw   += g_camera_stiffness*dt*(freecam->target_yaw   - freecam->yaw);
		freecam->pitch += g_camera_stiffness*dt*(freecam->target_pitch - freecam->pitch);
    }
    else
    {
        freecam->yaw = freecam->target_yaw;
        freecam->pitch = freecam->target_pitch;
    }

    RT_Vec3 up      = RT_Vec3Make(0, 1, 0);
    RT_Vec3 forward = RT_Vec3Make(0, 0,-1);

    RT_Quat rotation_yaw   = RT_QuatFromAxisAngle(RT_Vec3Make(0, 1, 0), RT_RadiansFromDegrees(freecam->yaw));
    RT_Quat rotation_pitch = RT_QuatFromAxisAngle(RT_Vec3Make(1, 0, 0), RT_RadiansFromDegrees(freecam->pitch));
    RT_Quat rotation = RT_QuatMul(rotation_yaw, rotation_pitch);

    forward = RT_QuatRotateVec3(rotation, forward);

    camera->forward = forward;

    RT_Vec3 z = RT_Vec3NormalizeOrZero(camera->forward);
    RT_Vec3 x = RT_Vec3NormalizeOrZero(RT_Vec3Cross(up, z));
    RT_Vec3 y = RT_Vec3NormalizeOrZero(RT_Vec3Cross(z, x));
    (void)y;

    camera->right = x;
    camera->up    = y;

    RT_Vec3 acceleration = { 0 };

    if (!freecam->target_fov)
		freecam->target_fov = camera->vfov;

    if (g_capture_mouse)
    {
		freecam->target_fov -= freecam->target_fov*0.05f*g_input.mouse_scroll;
		freecam->target_fov = RT_CLAMP(freecam->target_fov, 1.0f, 120.0f);

		if (RT_ButtonDown(RT_BUTTON_FORWARD))  acceleration = RT_Vec3Add(acceleration, camera->forward);
		if (RT_ButtonDown(RT_BUTTON_BACKWARD)) acceleration = RT_Vec3Sub(acceleration, camera->forward);
		if (RT_ButtonDown(RT_BUTTON_LEFT))     acceleration = RT_Vec3Sub(acceleration, x);
		if (RT_ButtonDown(RT_BUTTON_RIGHT))    acceleration = RT_Vec3Add(acceleration, x);
		if (RT_ButtonDown(RT_BUTTON_UP))       acceleration = RT_Vec3Add(acceleration, up);
		if (RT_ButtonDown(RT_BUTTON_DOWN))     acceleration = RT_Vec3Sub(acceleration, up);
    }

	camera->vfov = RT_Lerp(freecam->target_fov, camera->vfov, 0.9f);

    float move_speed = 50.0f;

    if (RT_ButtonDown(RT_BUTTON_CRAWL))  
    {
        if (RT_ButtonDown(RT_BUTTON_SPRINT))
        {
			move_speed *= 0.125f;
        }
        else
        {
			move_speed *= 0.25f;
        }
    }
    else if (RT_ButtonDown(RT_BUTTON_SPRINT)) 
    {
		move_speed *= 2.0f;
    }

    move_speed *= g_camera_friction; // hmmhmh?

    acceleration = RT_Vec3Muls(acceleration, move_speed);

    if (physics)
    {
		// NOTE(daniel): With varying dt this is not really framerate independent
		float friction = g_camera_friction;
		acceleration = RT_Vec3Sub(acceleration, RT_Vec3Muls(freecam->velocity, friction));

		freecam->velocity = RT_Vec3Add(freecam->velocity, RT_Vec3Muls(acceleration, dt));
		camera->position = RT_Vec3Add(camera->position, RT_Vec3Muls(freecam->velocity, dt));
    }
    else
    {
        freecam->velocity = { 0, 0, 0 };
        camera->position = RT_Vec3Add(camera->position, RT_Vec3Muls(RT_Vec3Divs(acceleration, g_camera_friction), dt)); // sketchy divide... 
    }

    RT_Vec3 offset = { 0, 0, 0 };
    offset.y += g_camera_bob*sinf(2.0f*g_curr_time);

    camera->position = RT_Vec3Add(camera->position, RT_Vec3Muls(offset, dt));
}

static LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT dear_imgui_result = ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);

    if (dear_imgui_result)
    {
        return dear_imgui_result;
    }

	ImGuiIO &imgui_io = ImGui::GetIO();

    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{
			RT_ProcessButton(RT_BUTTON_LMB, message == WM_LBUTTONDOWN);
		} break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			RT_ProcessButton(RT_BUTTON_RMB, message == WM_RBUTTONDOWN);
		} break;

        case WM_MOUSEWHEEL:
        {
            g_input.mouse_scroll = ((float)GET_WHEEL_DELTA_WPARAM(wparam)) / 120.0f;
        } break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
			bool down = ((message == WM_KEYDOWN) || (message == WM_SYSKEYDOWN));
			WPARAM vk = wparam;
			
			if (!imgui_io.WantCaptureKeyboard)
			{
				switch (vk)
				{
					case 'W':           RT_ProcessButton(RT_BUTTON_FORWARD, down); break;
					case 'A':           RT_ProcessButton(RT_BUTTON_LEFT, down); break;
					case 'S':           RT_ProcessButton(RT_BUTTON_BACKWARD, down); break;
					case 'D':           RT_ProcessButton(RT_BUTTON_RIGHT, down); break;
					case 'V':           RT_ProcessButton(RT_BUTTON_KILL_MOMENTUM, down); break;
					case 'F':           RT_ProcessButton(RT_BUTTON_FLASHLIGHT, down); break;
					case VK_SPACE:      RT_ProcessButton(RT_BUTTON_UP, down); break;
					case VK_CONTROL:    RT_ProcessButton(RT_BUTTON_DOWN, down); break;
					case VK_SHIFT:      RT_ProcessButton(RT_BUTTON_SPRINT, down); break;
					case VK_MENU:       RT_ProcessButton(RT_BUTTON_CRAWL, down); break;
					case VK_ESCAPE:     RT_ProcessButton(RT_BUTTON_ESCAPE, down); break;
				}
			}
		} break;
        
        default:
        {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        } break;
    }

    return 0;
}

static void RT_RenderGLTF(RT_GLTFNode *node, RT_Mat4 transform, RT_Mat4 prev_transform, RT_Vec4 color)
{
    if (!node)
        return;

    transform      = RT_Mat4Mul(transform, node->transform);
    prev_transform = RT_Mat4Mul(prev_transform, node->transform);

    if (node->model)
    {
        RT_GLTFModel *model = node->model;
        RT_RaytraceMeshColor(model->handle, color, &transform, &prev_transform, RT_RENDER_MASK_OBJECTS);
    }

    for (size_t i = 0; i < node->children_count; i++)
    {
        RT_GLTFNode *child = node->children[i];
        RT_RenderGLTF(child, transform, prev_transform, color);
    }
}

typedef struct RT_DuckBullet
{
    RT_Vec3 p;
    RT_Vec3 dp;
    RT_Vec4 color;
    float lifespan;
} RT_DuckBullet;

#define RT_MAX_DUCKBULLETS (512)

static float g_duckbullet_timer;
static int g_duckbullet_index;
static RT_DuckBullet g_duckbullets[RT_MAX_DUCKBULLETS];

static void RT_SpawnDuckBullet(RT_Vec3 p, RT_Vec3 dp)
{
    RT_DuckBullet *bullet = &g_duckbullets[g_duckbullet_index];
    g_duckbullet_index = (g_duckbullet_index + 1) % RT_MAX_DUCKBULLETS;

    bullet->p = p;
    bullet->dp = dp;
    bullet->lifespan = 1000.0f;
    bullet->color.x = 1.0f;
    bullet->color.y = 1.0f;
    bullet->color.z = 1.0f;
    bullet->color.w = 1.0f;

    g_duckbullet_timer = 0.01f;
}

static void RT_UpdateAndRenderDuckBullets(RT_GLTFNode *duck, float dt)
{
    g_duckbullet_timer -= dt;

    for (size_t i = 0; i < RT_MAX_DUCKBULLETS; i++)
    {
        RT_DuckBullet *bullet = &g_duckbullets[i];
        if (bullet->lifespan > 0.0f)
        {
            RT_Vec3 old_p = bullet->p;

            bullet->p = bullet->p + dt*bullet->dp;
            bullet->lifespan -= dt;

            RT_Mat4 transform = RT_Mat4FromTranslation(bullet->p);
            RT_Mat4 prev_transform = RT_Mat4FromTranslation(old_p);

            RT_RenderGLTF(duck, transform, prev_transform, bullet->color);
        }
    }
}

typedef enum RT_ShowcaseScene
{
    RT_ShowcaseScene_Sponza,
    RT_ShowcaseScene_Chess,
    RT_ShowcaseScene_PBR,
    RT_ShowcaseScene_COUNT,
} RT_ShowcaseScene;

static char *g_showcase_scene_names[RT_ShowcaseScene_COUNT] = {
    "Sponza",
    "Chess",
    "PBR",
};

static RT_ShowcaseScene g_showcase_scene = RT_ShowcaseScene_Sponza;
static bool g_scene_menu_open;

static void RT_ShowcaseSceneMenuBegin(void)
{
    static bool open = true;

    if (g_capture_mouse)
    {
        g_scene_menu_open = false;
        return;
    }

    if (ImGui::Begin("Showcase Scenes", &open))
    {
        if (ImGui::Button("Destroy Duckbullets"))
        {
            for (size_t i = 0; i < RT_MAX_DUCKBULLETS; i++)
            {
                g_duckbullets[i].lifespan = 0;
            }
        }

        ImGui::Checkbox("Camera physics", &g_camera_physics);
        ImGui::SliderFloat("Camera stiffness", &g_camera_stiffness, 0.1f, 8.0f);
        ImGui::SliderFloat("Camera friction", &g_camera_friction, 0.1f, 8.0f);
        ImGui::SliderFloat("Camera bob", &g_camera_bob, 0.0f, 25.0f);
        ImGui::Text("Current Scene: %s", g_showcase_scene_names[g_showcase_scene]);
        for (size_t i = 0; i < RT_ShowcaseScene_COUNT; i++)
        {
            if (ImGui::Button(g_showcase_scene_names[i]))
            {
                g_showcase_scene = (RT_ShowcaseScene)i;

                RT_RendererIO *io = RT_GetRendererIO();
                io->scene_transition = true;
            }
        }

        g_scene_menu_open = ImGui::CollapsingHeader("Scene");
		if (g_scene_menu_open)
		{
			ImGui::PushID("Scene");
        }
    }
}

static void RT_ShowcaseSceneMenuEnd(void)
{
    if (g_capture_mouse)
        return;

    if (g_scene_menu_open)
    {
        ImGui::PopID();
    }
    ImGui::End();
}

// ------------------------------------------------------------------
// This is just a way to not have to keep manual track of previous
// transforms.

typedef struct RT_TrackedMeshState
{
    RT_GLTFNode *root;
    bool prev_transform_valid;
    RT_Mat4 prev_transform;
} RT_TrackedMeshState;

typedef struct RT_TrackedMesh
{
    size_t index;
} RT_TrackedMesh;

static size_t g_tracked_mesh_count;
static RT_TrackedMeshState g_tracked_meshes[1024];

static RT_TrackedMesh RT_AddTrackedMesh(RT_GLTFNode *node)
{
    RT_TrackedMesh result = { 0 };

    if (ALWAYS(g_tracked_mesh_count < RT_ARRAY_COUNT(g_tracked_meshes)))
    {
        result.index = g_tracked_mesh_count++;
		RT_TrackedMeshState *state = &g_tracked_meshes[result.index];
        state->root = node;
    }

    return result;
}

static void RT_RenderTrackedMesh(RT_TrackedMesh mesh, RT_Mat4 transform, RT_Vec4 color = { 1, 1, 1, 1 })
{
    if (ALWAYS(mesh.index < g_tracked_mesh_count))
    {
		RT_TrackedMeshState *state = &g_tracked_meshes[mesh.index];

        if (!state->prev_transform_valid)
        {
            state->prev_transform = transform;
            state->prev_transform_valid = true;
        }

        RT_RenderGLTF(state->root, transform, state->prev_transform, color);
        state->prev_transform = transform;
    }
}

// ------------------------------------------------------------------

static bool g_flashlight;

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    // ------------------------------------------------------------------
    // Query performance frequency (used for high resolution timer)

    QueryPerformanceFrequency(&g_perf_freq);
    g_running = TRUE;

    ImGui::CreateContext();

    // ------------------------------------------------------------------
    // Create window

    {
        int w = 1280;
        int h = 720;

        WNDCLASSEXW wclass = {};
		wclass.cbSize        = sizeof(wclass);
		wclass.style         = CS_HREDRAW|CS_VREDRAW;
		wclass.lpfnWndProc   = WindowProc;
		wclass.hIcon         = LoadIconW(NULL, L"APPICON");
		wclass.hIcon         = LoadIconW(NULL, L"APPICON");
		wclass.hCursor       = LoadCursorW(NULL, IDC_ARROW);
		wclass.lpszClassName = L"retro_window_class";

        if (!RegisterClassExW(&wclass))
        {
            Win32_FatalError(L"Failed to register window class");
        }

        RECT wrect = {};
		wrect.left   = 0;
		wrect.top    = 0;
		wrect.right  = w;
		wrect.bottom = h;

        AdjustWindowRect(&wrect, WS_OVERLAPPEDWINDOW, FALSE);

        g_hwnd = CreateWindowExW(0, L"retro_window_class", L"retro fps",
                                 WS_OVERLAPPEDWINDOW,
                                 32, 32,
                                 wrect.right - wrect.left,
                                 wrect.bottom - wrect.top,
                                 NULL, NULL, NULL, NULL);

        if (!g_hwnd)
        {
            Win32_FatalError(L"Failed to create window");
        }
    }

    // ------------------------------------------------------------------
    // Init Dear ImGui

    ImGui_ImplWin32_Init(g_hwnd);

    // ------------------------------------------------------------------

    RT_FreeCam freecam = {};

    RT_Camera camera = {};
	camera.position  = RT_Vec3Make(0.0f, 0.0f,  1.0f);
    camera.up        = RT_Vec3Make(0.0f, 1.0f,  0.0f);
	camera.forward   = RT_Vec3Make(0.0f, 0.0f, -1.0f);
    camera.vfov = 60.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 10000.0f;

    // ------------------------------------------------------------------
    // Init renderer

    RT_Arena renderer_arena = {0};

    RT_RendererInitParams renderer_init_params = {0};
	renderer_init_params.arena         = &renderer_arena;
	renderer_init_params.window_handle = g_hwnd;
	RT_RendererInit(&renderer_init_params);

    // Show the window now so that if sponza takes a while to load, you
    // at least see a window.
    ShowWindow(g_hwnd, SW_SHOW);

    // Load test level
    // mine_err = load_level("level07.rdl");

    // int* verts_ids = RT_ArenaAlloc(&renderer_arena, sizeof(int) * MAX_SEGMENTS * MAX_SIDES_PER_SEGMENT * 6,alignof(int));
	// RT_Vertex* verts = RT_ArenaAlloc(&renderer_arena, sizeof(RT_Vertex) * MAX_SEGMENTS * MAX_VERTICES_PER_SEGMENT,alignof(RT_Vertex));
	// RT_GetLevelGeomerty(verts_ids, verts);
	//free(verts_ids);
	//free(verts);

    // ------------------------------------------------------------------
    // Load test meshes

    RT_GLTFNode *duck   = RT_LoadGLTF(&renderer_arena, "assets/models/duck/Duck.gltf", nullptr);
    RT_GLTFNode *helmet = RT_LoadGLTF(&renderer_arena, "assets/models/helmet/DamagedHelmet.gltf", nullptr);
    // RT_GLTFNode *sponza = RT_LoadGLTF(&renderer_arena, "assets/models/NewSponza/NewSponza_Main_glTF_002.gltf", nullptr);
    RT_GLTFNode *sponza = RT_LoadGLTF(&renderer_arena, "assets/models/sponza/Sponza.gltf", nullptr);
    RT_GLTFNode *chess = RT_LoadGLTF(&renderer_arena, "assets/models/ABeautifulGame/ABeautifulGame.gltf", nullptr);
    RT_GLTFNode *metal_rough_spheres = RT_LoadGLTF(&renderer_arena, "assets/models/MetalRoughSpheres/MetalRoughSpheres.gltf", nullptr);

    RT_TrackedMesh sponza_duck = RT_AddTrackedMesh(duck);
    RT_TrackedMesh beam_duck   = RT_AddTrackedMesh(duck);

    RT_Mat4 prev_duck_transform = RT_Mat4Identity();

    // ------------------------------------------------------------------
    // Get the main loop underway!

    LARGE_INTEGER current_time = Win32_GetTime();

    float dt = 1.0f / 60.0f; // Not zero because maybe some bozo wants to divide by dt 

    bool entered_mouse_capture_this_click = false;

    while (g_running)
    {
        // ------------------------------------------------------------------
        // Process input

        RT_NewInputFrame();

        ImGui_ImplWin32_NewFrame();

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
                case WM_QUIT:
                {
                    g_running = FALSE;
                } break;

                default:
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                } break;
            }
        }

        if (RT_ButtonPressed(RT_BUTTON_ESCAPE))
        {
            g_running = false;
        }

        if (!g_running)
        {
            break;
        }

        // ------------------------------------------------------------------
        // Mouse handling

        RECT client_rect;
        GetClientRect(g_hwnd, &client_rect);

        int client_w = client_rect.right;
        int client_h = client_rect.bottom;

        POINT clientspace_center = { client_w / 2, client_h / 2 };
        POINT screenspace_center = clientspace_center;
        ClientToScreen(g_hwnd, &screenspace_center);

        ImGuiIO& imgui_io = ImGui::GetIO();
        if (!imgui_io.WantCaptureMouse)
        {
            if (RT_ButtonPressed(RT_BUTTON_LMB))
            {
                if (!g_capture_mouse)
                {
                    g_capture_mouse = true;
                    entered_mouse_capture_this_click = true;

                    SetCapture(g_hwnd);
                    ShowCursor(FALSE);

                    SetCursorPos(screenspace_center.x, screenspace_center.y);
                }
            }
        }

        if (g_capture_mouse && RT_ButtonReleased(RT_BUTTON_LMB))
        {
            entered_mouse_capture_this_click = false;
        }

        if (RT_ButtonPressed(RT_BUTTON_RMB))
        {
            if (g_capture_mouse)
            {
                SetCapture(NULL);
                ShowCursor(TRUE);
            }

            g_capture_mouse = false;
        }

        bool has_focus = (GetActiveWindow() == g_hwnd);

        g_input.mouse_dp.x = 0.0f;
        g_input.mouse_dp.y = 0.0f;

        if (has_focus && g_capture_mouse)
        {
            POINT cursor;
            GetCursorPos(&cursor);
            ScreenToClient(g_hwnd, &cursor);

            g_input.mouse_dp.x =   (float)cursor.x - (float)clientspace_center.x;
            g_input.mouse_dp.y = -((float)cursor.y - (float)clientspace_center.y);

            g_input.mouse_p = RT_Vec2Make((float)cursor.x, (float)client_h - (float)cursor.y - 1);

            SetCursorPos(screenspace_center.x, screenspace_center.y);
        }

        // ------------------------------------------------------------------
        // Welcome to the main loop proper

		RT_UpdateFreecam(&freecam, &camera, dt);

        if (g_capture_mouse)
        {
            if (!entered_mouse_capture_this_click)
            {
				if (RT_ButtonDown(RT_BUTTON_LMB) && g_duckbullet_timer <= 0.0f)
				{
					RT_Vec3 dir_jitter = RT_Vec3Make(
						2.0f*(float)((double)rand() / RAND_MAX) - 1.0f,
						2.0f*(float)((double)rand() / RAND_MAX) - 1.0f,
						2.0f*(float)((double)rand() / RAND_MAX) - 1.0f
					);

					RT_Vec3 dir = 0.1f*dir_jitter + camera.forward;

					RT_SpawnDuckBullet(camera.position, 10.0f*dir);
				}
            }
        }

        if (RT_ButtonPressed(RT_BUTTON_FLASHLIGHT))
        {
            g_flashlight = !g_flashlight;
        }

        RT_SceneSettings scene_settings = { 0 };
        scene_settings.camera = &camera;
        RT_BeginFrame();
        RT_BeginScene(&scene_settings);

        // ------------------------------------------------------------------
        // ImGui new frame

		ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ------------------------------------------------------------------

        {
            RT_DoRendererDebugMenuParams params = {};
            params.ui_has_cursor_focus = !g_capture_mouse;

			// This exists because the renderer needs a place to legally be 
			// allowed to call Dear ImGui stuff. Could be replaced in favor of a
			// more formal debug menu kind of system, if that seems useful.
			RT_DoRendererDebugMenus(&params);
        }

        static float t = 0.0f;
        g_curr_time = t;

        RT_RendererIO *io = RT_GetRendererIO();

        RT_ShowcaseSceneMenuBegin();

        switch (g_showcase_scene)
        {
            case RT_ShowcaseScene_Sponza:
            {
                static bool    sun_on        = true;
                static bool    animate_sun   = true;
                static RT_Vec3 sun_direction = RT_Vec3Make(0.916445f, 0.75f, -0.335488f);
                static RT_Vec3 sun_color     = RT_Vec3Make(1.0f, 0.95f, 0.75f);
                static float   sun_strength  = 16.0f;

				if (g_scene_menu_open)
				{
					ImGui::Checkbox("Sun On", &sun_on);
					ImGui::Checkbox("Animate Sun", &animate_sun);
					ImGui::ColorEdit3("Sun Color", &sun_color.x);
					ImGui::SliderFloat("Sun Strength", &sun_strength, 0, 32.0f);
					ImGui::SliderFloat("Sun Direction X", &sun_direction.x, -1.0f, 1.0f);
					ImGui::SliderFloat("Sun Direction Y", &sun_direction.y, -1.0f, 1.0f);
					ImGui::SliderFloat("Sun Direction Z", &sun_direction.z, -1.0f, 1.0f);
                }

				if (sun_on)
				{
					RT_Vec3 animated_sun_direction = sun_direction;

					if (animate_sun)
					{
						animated_sun_direction = RT_Vec3Make(0.916445f, 0.25f*cosf(0.125f*t) + 0.5f, -0.335488f);
					}

					RT_Vec3 sun_emission = RT_Vec3Muls(sun_color, sun_strength);
					// RT_Light sun = RT_MakeDirectionalLight(sun_emission, RT_Vec3Normalize(animated_sun_direction));
					// RT_SubmitLight(sun);
				}

                static bool random_ass_light_on = true;
				if (g_scene_menu_open)
				{
					ImGui::Checkbox("Random-ass Light On", &random_ass_light_on);
                }

                if (random_ass_light_on)
                {
                    RT_RaytraceSubmitLight(RT_MakeSphericalLight(RT_Vec3Muls(RT_Vec3Make(1.3f, 2.2f, 3.3f), 10.0),
														 RT_Vec3Make(50.0f*cosf(0.25f*t), 50.0f, 10.0f*sinf(0.5f*t)),
														 1.5f));
                }

				RT_Light rect_light = RT_MakeRectangularLight(RT_Vec3Muls(RT_Vec3Make(1.0f, 0.5f, 0.25f), 10.0),
															  RT_Vec3Make(0, 35, 50),
															  RT_QuatFromAxisAngle(RT_Vec3Make(0, 0, 1), RT_PI32 + 0.25f*RT_PI32*sinf(0.1f*t)),
															  RT_Vec2Make(1.5f, 1.5f));
                rect_light.spot_angle    = RT_Uint8FromFloat(0.35f);
                rect_light.spot_softness = RT_Uint8FromFloat(0.35f);
                RT_RaytraceSubmitLight(rect_light);

                static bool spotlight_on = true;
				static RT_Vec3 spot_color     = RT_Vec3Make(1, 1, 1);
				static float   spot_strength  = 10.0;
				static RT_Vec3 spot_direction = RT_Vec3Make(1, -0.6f, 0);
				static float   spot_radius    = 0.3f;
				static float   spot_angle     = 0.1f;
				static float   spot_softness  = 0.02f;
				static float   spot_vignette  = 0.2f;
				static RT_Vec3 spot_position  = RT_Vec3Make(100, 100, -3.0f);

				if (g_scene_menu_open)
				{
					ImGui::Checkbox("Spotlight On", &spotlight_on);
					ImGui::ColorEdit3("Spot Color", &spot_color.x);
					ImGui::SliderFloat("Spot Strength", &spot_strength, 0, 20.0f);
					ImGui::SliderFloat("Spot Direction X", &spot_direction.x, -1.0f, 1.0f);
					ImGui::SliderFloat("Spot Direction Y", &spot_direction.y, -1.0f, 1.0f);
					ImGui::SliderFloat("Spot Direction Z", &spot_direction.z, -1.0f, 1.0f);
					ImGui::SliderFloat("Spot Radius", &spot_radius, 0.01f, 10.0f);
					ImGui::SliderFloat("Spot Angle", &spot_angle, 0, 1.0f);
					ImGui::SliderFloat("Spot Softness", &spot_softness, 0.0f, 0.1f);
					ImGui::SliderFloat("Spot Vignette", &spot_vignette, 0.0f, 1.0f);
				}

				if (spotlight_on)
				{
                    RT_Vec3 emission  = RT_Vec3Muls(spot_color, spot_strength);
                    RT_Vec3 direction = RT_Vec3Normalize(spot_direction);

					RT_Light spot = RT_MakeSphericalSpotlight(emission, spot_position, direction, spot_radius, spot_angle, spot_softness, spot_vignette);
                    RT_RaytraceSubmitLight(spot);
				}

				{
					RT_Mat4 transform = RT_Mat4FromScale(RT_Vec3FromScalar(40));
					RT_RenderGLTF(chess, transform, transform, RT_Vec4Make(1, 1, 1, 1));
				}

				{
					RT_Mat4 transform = RT_Mat4FromScale(RT_Vec3FromScalar(15));
					RT_RenderGLTF(sponza, transform, transform, RT_Vec4Make(1, 1, 1, 1));
				}

				{
					RT_Mat4 transform = RT_Mat4FromTRS(RT_Vec3Make(40.0f*sinf(0.25f*t), 2.5f + cosf(t), 50.0f),
													   RT_QuatFromAxisAngle(RT_Vec3Make(0, 1, 0), t),
													   RT_Vec3FromScalar(10.0f));
                    RT_Vec4 color = RT_Vec4Make(1, 1, 1, 0.5f*sinf(0.5f*t) + 0.5f);
                    RT_RenderTrackedMesh(sponza_duck, transform, color);
				}

				{
					RT_Mat4 transform = RT_Mat4FromTRS(RT_Vec3Make(-2.5f, 7.5f, 0.0f), 
													   RT_QuatIdentity(),
													   RT_Vec3FromScalar(5.0f));

					RT_RenderGLTF(helmet, transform, transform, RT_Vec4Make(1, 1, 1, 1));
				}
            } break;

            case RT_ShowcaseScene_Chess:
            {
				{
                    // RT_Light sun = RT_MakeDirectionalLight(RT_Vec3Make(16.0f, 15.0f, 12.0f), RT_Vec3Make(0.916445f, 0.25f*cosf(0.125f*t) + 0.5f, -0.335488f));
					// RT_SubmitLight(sun);
				}

				{
					RT_Mat4 transform = RT_Mat4FromScale(RT_Vec3FromScalar(100));
					RT_RenderGLTF(chess, transform, transform, RT_Vec4Make(1, 1, 1, 1));
				}
            } break;

            case RT_ShowcaseScene_PBR:
            {
                /*
				{
					RT_Light lights[1] = {};

					lights[0].kind     = RT_LightKind_Directional;
					lights[0].emission = RT_Vec3Muls(RT_Vec3Make(16.0f, 15.0f, 12.0f), 1.0f);
					lights[0].pos_dir  = RT_Vec3Normalize(RT_Vec3Make(0.916445f, 0.25f*cosf(0.125f*t) + 0.5f, -0.335488f));

				}
                */

				{
                    RT_Mat4 transform = RT_Mat4Identity();
					RT_RenderGLTF(metal_rough_spheres, transform, transform, RT_Vec4Make(1, 1, 1, 1));
				}
            } break;
        }

        RT_UpdateAndRenderDuckBullets(duck, io->frame_frozen ? 0.0f : dt);

        if (!io->frame_frozen)
        {
            t += dt;
        }

        if (g_flashlight)
        {
            RT_Vec3 emission  = RT_Vec3FromScalar(10.0);
            RT_Vec3 position  = camera.position;
            RT_Vec3 direction = camera.forward;
            float   radius    = 0.2f;
            float   angle     = 0.04f;
            float   softness  = 0.005f;
			RT_Light flashlight = RT_MakeSphericalSpotlight(emission, position, direction, radius, angle, softness, 0.5f);
            RT_RaytraceSubmitLight(flashlight);
        }

        RT_ShowcaseSceneMenuEnd();
        
        ImGui::Render();

        RT_EndScene();
        RT_EndFrame();
        RT_RenderImGui();
        RT_SwapBuffers();

        // ------------------------------------------------------------------

        LARGE_INTEGER end_time = Win32_GetTime();

        dt = Win32_GetTimeElapsed(current_time, end_time);
        current_time = end_time;

        // ------------------------------------------------------------------

        RT_ArenaResetAndDecommit(&g_thread_arena);
    }

    RT_RendererExit();

    // ------------------------------------------------------------------
    // Shut down Dear ImGui

    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}