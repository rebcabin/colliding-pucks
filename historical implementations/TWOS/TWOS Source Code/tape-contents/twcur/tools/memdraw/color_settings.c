#include "mplot.h"

#include <xview/xview.h>
#include <xview/canvas.h>

extern Display * x_display;
extern GC x_GC; 
extern Canvas see_colors_canvas;

extern char * memtype_list[];

settings_win_refresh()
/* Forces refresh of the color settings window */
{
	XEvent theevent;
	Window window = xv_get (canvas_paint_window (see_colors_canvas), XV_XID);

	theevent.type = Expose;
	theevent.xexpose.send_event = True;
	theevent.xexpose.display = x_display;
	theevent.xexpose.window = window;
	theevent.xexpose.count = 0;
	theevent.xexpose.x = theevent.xexpose.y = 0;
	theevent.xexpose.width = theevent.xexpose.height = 1;

	XSendEvent( x_display, window, True, ExposureMask, &theevent );
	XFlush ( x_display );
}


repaint_color_settings ()
{
	register int i;
	char string[50];
	Window window = xv_get (canvas_paint_window (see_colors_canvas), XV_XID);
	int height = xv_get ( see_colors_canvas, XV_HEIGHT );

	XClearWindow ( x_display, window );
 
	for (i = 0;  i < ncolor_settings && 15*(i+1) < height;  i++)
	{
		change_color(color_settings[i].color);
		sprintf (string,"%s", memtype_list[color_settings[i].mem_type]);
		XDrawString ( x_display, window, x_GC, 15, 15*(i+1), string, strlen (string) );
	}

	XFlush ( x_display );
}


push_color_setting (type, color)
	int type, color;

{
	color_settings[ncolor_settings].mem_type = type;
	color_settings[ncolor_settings].color = color;
	ncolor_settings++;

	settings_win_refresh();
	redraw_page = 1;
}


clear_color_settings ()
{
	ncolor_settings = 0;

	settings_win_refresh();
	redraw_page = 1;
}

message_color_settings ()
{
	push_color_setting ( 17, GREEN ); /* positive message */
	push_color_setting ( 18, RED ); /* negative message */
	push_color_setting ( 19, YELLOW ); /* system message */
}

state_color_settings ()
{
	push_color_setting ( 6, YELLOW ); /* state Q */
	push_color_setting ( 7, GREEN ); /* state address */
	push_color_setting ( 8, CYAN ); /* state dyn mem */
	push_color_setting ( 9, MAGENTA ); /* state sent */
	push_color_setting ( 10, RED ); /* state error */
}

unused_color_settings ()
{
	push_color_setting ( 0, BLACK ); /* free */
	push_color_setting ( 2, CYAN ); /* pool */
	push_color_setting ( 1, MAGENTA ); /* lost */
}

communications_color_settings ()
{
	push_color_setting ( 24, CYAN ); /* in transit */
	push_color_setting ( 25, RED ); /* on node */
	push_color_setting ( 26, YELLOW ); /* off node */
}
