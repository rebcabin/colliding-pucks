/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef XVIEW
#include <xview/xview.h>
#include <xview/canvas.h>
#endif


#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#endif

void my_event_proc ( canvas, event )
 
	Canvas canvas;
	Event *event;
{
	extern Menu main_menu;

	switch ( event_id(event) )
	{
		case MS_LEFT:
		case MS_MIDDLE:
		case MS_RIGHT:
 
		if ( event_is_down(event) )
		{
			search = 0;
			page_scan = 0;
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
#ifdef X11
					mask_clear();
#endif
#ifdef SUNVIEW
					writemask ( 8 );
					clear();
					writemask ( -1 );
#endif
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

