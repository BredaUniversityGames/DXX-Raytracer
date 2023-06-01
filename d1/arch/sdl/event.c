/*
 *
 * SDL Event related stuff
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "event.h"
#include "SDL_syswm.h"
#include "key.h"
#include "mouse.h"
#include "window.h"
#include "timer.h"
#include "config.h"
#include "playsave.h"

#include "joy.h"
//I do not like doing ifdef's here.... ah well it is what it is.
#ifdef RT_DX12
#include "RTgr.h"
#endif //RT_DX12




extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void mouse_cursor_autohide();

static int initialised=0;

#ifdef RT_DX12

#pragma pack (push, 8)
ImGuiKey RT_SDLKeycodeToImGuiKey(int keycode)
{
	switch (keycode)
	{
		case SDLK_TAB: return ImGuiKey_Tab;
		case SDLK_LEFT: return ImGuiKey_LeftArrow;
		case SDLK_RIGHT: return ImGuiKey_RightArrow;
		case SDLK_UP: return ImGuiKey_UpArrow;
		case SDLK_DOWN: return ImGuiKey_DownArrow;
		case SDLK_PAGEUP: return ImGuiKey_PageUp;
		case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
		case SDLK_HOME: return ImGuiKey_Home;
		case SDLK_END: return ImGuiKey_End;
		case SDLK_INSERT: return ImGuiKey_Insert;
		case SDLK_DELETE: return ImGuiKey_Delete;
		case SDLK_BACKSPACE: return ImGuiKey_Backspace;
		case SDLK_SPACE: return ImGuiKey_Space;
		case SDLK_RETURN: return ImGuiKey_Enter;
		case SDLK_ESCAPE: return ImGuiKey_Escape;
		case SDLK_QUOTE: return ImGuiKey_Apostrophe;
		case SDLK_COMMA: return ImGuiKey_Comma;
		case SDLK_MINUS: return ImGuiKey_Minus;
		case SDLK_PERIOD: return ImGuiKey_Period;
		case SDLK_SLASH: return ImGuiKey_Slash;
		case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
		case SDLK_EQUALS: return ImGuiKey_Equal;
		case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
		case SDLK_BACKSLASH: return ImGuiKey_Backslash;
		case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
		case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
		case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
		case SDLK_PAUSE: return ImGuiKey_Pause;
		case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
		case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
		case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
		case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
		case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
		case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
		case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
		case SDLK_LSHIFT: return ImGuiKey_LeftShift;
		case SDLK_LALT: return ImGuiKey_LeftAlt;
		case SDLK_RCTRL: return ImGuiKey_RightCtrl;
		case SDLK_RSHIFT: return ImGuiKey_RightShift;
		case SDLK_RALT: return ImGuiKey_RightAlt;
		case SDLK_0: return ImGuiKey_0;
		case SDLK_1: return ImGuiKey_1;
		case SDLK_2: return ImGuiKey_2;
		case SDLK_3: return ImGuiKey_3;
		case SDLK_4: return ImGuiKey_4;
		case SDLK_5: return ImGuiKey_5;
		case SDLK_6: return ImGuiKey_6;
		case SDLK_7: return ImGuiKey_7;
		case SDLK_8: return ImGuiKey_8;
		case SDLK_9: return ImGuiKey_9;
		case SDLK_a: return ImGuiKey_A;
		case SDLK_b: return ImGuiKey_B;
		case SDLK_c: return ImGuiKey_C;
		case SDLK_d: return ImGuiKey_D;
		case SDLK_e: return ImGuiKey_E;
		case SDLK_f: return ImGuiKey_F;
		case SDLK_g: return ImGuiKey_G;
		case SDLK_h: return ImGuiKey_H;
		case SDLK_i: return ImGuiKey_I;
		case SDLK_j: return ImGuiKey_J;
		case SDLK_k: return ImGuiKey_K;
		case SDLK_l: return ImGuiKey_L;
		case SDLK_m: return ImGuiKey_M;
		case SDLK_n: return ImGuiKey_N;
		case SDLK_o: return ImGuiKey_O;
		case SDLK_p: return ImGuiKey_P;
		case SDLK_q: return ImGuiKey_Q;
		case SDLK_r: return ImGuiKey_R;
		case SDLK_s: return ImGuiKey_S;
		case SDLK_t: return ImGuiKey_T;
		case SDLK_u: return ImGuiKey_U;
		case SDLK_v: return ImGuiKey_V;
		case SDLK_w: return ImGuiKey_W;
		case SDLK_x: return ImGuiKey_X;
		case SDLK_y: return ImGuiKey_Y;
		case SDLK_z: return ImGuiKey_Z;
		case SDLK_F1: return ImGuiKey_F1;
		case SDLK_F2: return ImGuiKey_F2;
		case SDLK_F3: return ImGuiKey_F3;
		case SDLK_F4: return ImGuiKey_F4;
		case SDLK_F5: return ImGuiKey_F5;
		case SDLK_F6: return ImGuiKey_F6;
		case SDLK_F7: return ImGuiKey_F7;
		case SDLK_F8: return ImGuiKey_F8;
		case SDLK_F9: return ImGuiKey_F9;
		case SDLK_F10: return ImGuiKey_F10;
		case SDLK_F11: return ImGuiKey_F11;
		case SDLK_F12: return ImGuiKey_F12;
	}
	return ImGuiKey_None;
}

ImGuiKey RT_SDLKeycodeToImGuiMod(int keycode)
{
	switch (keycode)
	{
		case SDLK_LCTRL: return ImGuiMod_Ctrl;
		case SDLK_LSHIFT: return ImGuiMod_Shift;
		case SDLK_LALT: return ImGuiMod_Alt;
		case SDLK_RCTRL: return ImGuiMod_Ctrl;
		case SDLK_RSHIFT: return ImGuiMod_Shift;
		case SDLK_RALT: return ImGuiMod_Alt;
	}
	return ImGuiKey_None;
}

static void RT_SDL_UpdateKeyModifier(ImGuiIO* io, SDLMod keyMods)
{
	if (keyMods & KMOD_SHIFT)
	{
		int t = 5;
	}
	ImGuiIO_AddKeyEvent(io, ImGuiMod_Ctrl, (keyMods & KMOD_CTRL) != 0);
	ImGuiIO_AddKeyEvent(io, ImGuiMod_Shift, (keyMods & KMOD_SHIFT) != 0);
	ImGuiIO_AddKeyEvent(io, ImGuiMod_Alt, (keyMods & KMOD_ALT) != 0);
	ImGuiIO_AddKeyEvent(io, ImGuiMod_Super, (keyMods & KMOD_META) != 0);
}

void RT_Event_Poll(ImGuiIO* io, SDL_Event* ev, int* clean_uniframe, int* idle)
{
	SDL_Event event = *ev;
	switch (event.type)
	{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (*clean_uniframe)
				memset(unicode_frame_buffer,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
			*clean_uniframe = 0;
			*idle = 0;

			//Only do input on true
			{
#if 0
				RT_LOGF(RT_LOGSERVERITY_INFO, "KEY EVENT: %i STATE: %i", event.key.keysym.scancode, (event.key.type == SDL_KEYDOWN));
#endif
				ImGuiKey key = RT_SDLKeycodeToImGuiKey(event.key.keysym.sym);

				/// Old way of doing it, we do not update key modifiers we just take the key and add the modifier since clicking can also use shift or cntrl.
				//RT_SDL_UpdateKeyModifier(io, event.key.keysym.mod);

				ImGuiKey mod = RT_SDLKeycodeToImGuiMod(event.key.keysym.sym);
				if (mod != ImGuiKey_None)
				{
					ImGuiIO_AddKeyEvent(io, mod, (event.key.type == SDL_KEYDOWN));
				}

				ImGuiIO_AddKeyEvent(io, key, (event.key.type == SDL_KEYDOWN));

				if (event.key.type == SDL_KEYDOWN) {
					ImGuiIO_AddInputCharacterUTF16(io, event.key.keysym.unicode);
				}
			}
			if (io->WantCaptureKeyboard != true || event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_F1)
				key_handler((SDL_KeyboardEvent*)&event);

			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			idle = 0;
			
			{
				int mouse_button = -1;
				if (event.button.button == SDL_BUTTON_LEFT) { mouse_button = 0; }
				if (event.button.button == SDL_BUTTON_RIGHT) { mouse_button = 1; }
				if (event.button.button == SDL_BUTTON_MIDDLE) { mouse_button = 2; }
				if (event.button.button == SDL_BUTTON_X1) { mouse_button = 3; }
				if (event.button.button == SDL_BUTTON_X2) { mouse_button = 4; }
				
				if (mouse_button == -1) {
					// Check for scrollwheel.
					if (event.button.button == SDL_BUTTON_WHEELUP)
						ImGuiIO_AddMouseWheelEvent(io, 0, 1.0);

					if (event.button.button == SDL_BUTTON_WHEELDOWN)
						ImGuiIO_AddMouseWheelEvent(io, 0, -1.0);
				}
				else
					ImGuiIO_AddMouseButtonEvent(io, mouse_button, (event.type == SDL_MOUSEBUTTONDOWN));

			}
			if (io->WantCaptureMouse != true)
				mouse_button_handler((SDL_MouseButtonEvent*)&event);
			break;

		case SDL_MOUSEMOTION:
			idle = 0;
			
			ImGuiIO_AddMousePosEvent(io, (float)event.motion.x, (float)event.motion.y);
			if (!igIsWindowFocused(ImGuiFocusedFlags_AnyWindow))
				mouse_motion_handler((SDL_MouseMotionEvent*)&event);
			
			break;

		case SDL_ACTIVEEVENT:
		{
			Uint8 window_event = event.active.state;
			if (window_event == SDL_APPMOUSEFOCUS && event.active.gain == 1)
				ImGuiIO_AddFocusEvent(io, true);
			else if (window_event == SDL_APPMOUSEFOCUS && event.active.gain == 0)
				ImGuiIO_AddFocusEvent(io, false);
		} break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			joy_button_handler((SDL_JoyButtonEvent*)&event);
			idle = 0;
			break;
		case SDL_JOYAXISMOTION:
			if (joy_axis_handler((SDL_JoyAxisEvent*)&event))
				idle = 0;
			break;
		case SDL_JOYHATMOTION:
			joy_hat_handler((SDL_JoyHatEvent*)&event);
			idle = 0;
			break;
		case SDL_JOYBALLMOTION:
			break;

		case SDL_QUIT: {
			d_event qevent = { EVENT_QUIT };
			call_default_handler(&qevent);
			*idle = 0;
		}
	}
}
#pragma pack (pop)
#endif // RT_DX12

void event_poll()
{
	SDL_Event event;
	int clean_uniframe=1;
	window *wind = window_get_front();
	int idle = 1;
	
	// If the front window changes, exit this loop, otherwise unintended behavior can occur
	// like pressing 'Return' really fast at 'Difficulty Level' causing multiple games to be started
	while ((wind == window_get_front()) && SDL_PollEvent(&event))
	{
#ifdef RT_DX12
		ImGuiIO* io = igGetIO();

		RT_Event_Poll(io, &event, &clean_uniframe, &idle);
#else
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (clean_uniframe)
					memset(unicode_frame_buffer,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
				clean_uniframe=0;
				key_handler((SDL_KeyboardEvent *)&event);
				idle = 0;
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouse_button_handler((SDL_MouseButtonEvent *)&event);
				idle = 0;
				break;
			case SDL_MOUSEMOTION:
				mouse_motion_handler((SDL_MouseMotionEvent *)&event);
				idle = 0;
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				joy_button_handler((SDL_JoyButtonEvent *)&event);
				idle = 0;
				break;
			case SDL_JOYAXISMOTION:
				if (joy_axis_handler((SDL_JoyAxisEvent *)&event))
					idle = 0;
				break;
			case SDL_JOYHATMOTION:
				joy_hat_handler((SDL_JoyHatEvent *)&event);
				idle = 0;
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				d_event qevent = { EVENT_QUIT };
				call_default_handler(&qevent);
				idle = 0;
			} break;
		}
#endif //RT_DX12
	}

	// Send the idle event if there were no other events
	if (idle)
	{
		d_event ievent;
		
		ievent.type = EVENT_IDLE;
		event_send(&ievent);
	}
	else
		event_reset_idle_seconds();
	
	mouse_cursor_autohide();
}

void event_flush()
{
	SDL_Event event;
	
	while (SDL_PollEvent(&event));
}

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}

int (*default_handler)(d_event *event) = NULL;

void set_default_handler(int (*handler)(d_event *event))
{
	default_handler = handler;
}

int call_default_handler(d_event *event)
{
	if (default_handler)
		return (*default_handler)(event);
	
	return 0;
}

void event_send(d_event *event)
{
	window *wind;
	int handled = 0;

	for (wind = window_get_front(); wind != NULL && !handled; wind = window_get_prev(wind))
		if (window_is_visible(wind))
		{
			handled = window_send_event(wind, event);

			if (!window_exists(wind)) // break away if necessary: window_send_event() could have closed wind by now
				break;
			if (window_is_modal(wind))
				break;
		}
	
	if (!handled)
		call_default_handler(event);
}

// Process the first event in queue, sending to the appropriate handler
// This is the new object-oriented system
// Uses the old system for now, but this may change
void event_process(void)
{
	d_event event;
	window* wind = window_get_front();

	timer_update();

	event_poll();	// send input events first

	// Doing this prevents problems when a draw event can create a newmenu,
	// such as some network menus when they report a problem
	if (window_get_front() != wind)
		return;

	event.type = EVENT_WINDOW_DRAW;	// then draw all visible windows
	wind = window_get_first();
	while (wind != NULL)
	{
		window* prev = window_get_prev(wind);
		if (window_is_visible(wind))
			window_send_event(wind, &event);

		if (!window_exists(wind))
		{
			if (!prev) // well there isn't a previous window ...
				break; // ... just bail out - we've done everything for this frame we can.
			wind = window_get_next(prev); // the current window seemed to be closed. so take the next one from the previous which should be able to point to the one after the current closed
		}
		else
			wind = window_get_next(wind);
	}

	gr_flip();
}

void event_toggle_focus(int activate_focus)
{
	//SAM: Old code is commented below, we will just show the cursor and not grab it when we are not using mouse controls.
	if (activate_focus && PlayerCfg.ControlType & CONTROL_USING_MOUSE)
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);

	mouse_toggle_cursor(!activate_focus);
}

static fix64 last_event = 0;

void event_reset_idle_seconds()
{
	last_event = timer_query();
}

fix event_get_idle_seconds()
{
	return (timer_query() - last_event)/F1_0;
}

