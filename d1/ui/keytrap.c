/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "key.h"


UI_GADGET_KEYTRAP * ui_add_gadget_keytrap( UI_DIALOG * dlg, int key_to_trap, int (*function_to_call)(void)  )
{
	UI_GADGET_KEYTRAP * keytrap;

	keytrap = (UI_GADGET_KEYTRAP *)ui_gadget_add( dlg, 8, 0, 0, 0, 0 );
	keytrap->parent = (UI_GADGET *)keytrap;

	keytrap->trap_key = key_to_trap;
	keytrap->user_function = function_to_call;

	return keytrap;

}

int ui_keytrap_do( UI_GADGET_KEYTRAP * keytrap, d_event *event )
{
	int keypress = 0;
	int rval = 0;
	
	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	if ( keypress == keytrap->trap_key )
	{
		keytrap->user_function();
		rval = 1;
	}

	return rval;
}
