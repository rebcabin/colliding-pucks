#include "mplot.h"

#include <xview/xview.h>
#include <xview/canvas.h>

extern int nobj_names, obj_cycle, show_object;

void my_event_proc ( canvas, event )
 
	Canvas canvas;
	Event *event;
{
	extern Menu main_menu;

	switch ( event_id (event) )
	{
		case MS_LEFT:
		case MS_MIDDLE:
		case MS_RIGHT:
 
		if ( event_is_down(event) )
		{
			dev = event_id(event);
 
			dev_x = event_x ( event );
			dev_y = height - event_y ( event );

			if ( zooming == 1 )
				zoom_start2 ( dev_x, dev_y );
			else if ( zooming == 2 )
				zoom_end();
			else if ( identify_on )
			{
				if ( dev == MS_RIGHT )
					identify_item ( dev_x, dev_y );
				else if ( dev == MS_LEFT )
				{
					identify_on = 0;
					mask_clear();
				}
			}
			else if ( obj_cycle )
			{
				if ( dev != MS_RIGHT )
					obj_cycle = 0;
				else
				{
					show_object++;
					if ( show_object == nobj_names )
						show_object = 0;
					redraw_page = 1;
				}				
			}
			else if ( dev == MS_RIGHT )
			{
				menu_show ( main_menu, canvas, event, 0 );
			}
		}
		break;
 
		case LOC_MOVE:
 
			dev_x = event_x ( event );
			dev_y = height - event_y ( event );

			if ( zooming == 1 )
				zoom_first ( dev_x, dev_y );
			else if ( zooming == 2 )
				zoom_second ( dev_x, dev_y );
 
		break;
	}
}

