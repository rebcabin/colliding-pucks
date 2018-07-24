#include "mplot.h"

#include <X11/Xlib.h>
#include <xview/panel.h>

extern Display * x_display;
Panel_item panel_button_plus, panel_button_minus, panel_message_page;

int zoomed;
static int page = 1;
static int zsp = 0; /* number of zooms performed (on the stack) */

/* the zoom stack */
#define ZSTKSIZE 10
struct
{
	int xsmin, xsmax;
	double ysmin, ysmax;
}
	zstk[ZSTKSIZE];

unzoom_ok ()
{
	return ( zsp > 0 );
}

zoom_ok ()
{
	return ( zsp < ZSTKSIZE );
}

null_zoom ()
{
	zoomed = 0;
	zsp = 0;
}

zoom_push ()
{
	zstk[zsp].xsmin = xsmin;
	zstk[zsp].xsmax = xsmax;
	zstk[zsp].ysmin = ysmin;
	zstk[zsp].ysmax = ysmax;
	zsp++;
}

zoom_pop ()
{
	zsp--;
	xsmin = zstk[zsp].xsmin;
	xsmax = zstk[zsp].xsmax;
	ysmin = zstk[zsp].ysmin;
	ysmax = zstk[zsp].ysmax;
}

/* x1,y1 are the position of the first corner (initially the red cross)
   x2,y2 are the position of the second corner
   last_* are for keeping the last position of things so it can erase */
static int x1, x2, last_x1, last_x2;
static int y1, y2, last_y1, last_y2;

extern set_zoom_menus();

zoom_start()
/* Starts the first part of zooming--moving the red cross to the upper right
   corner of the area to zoom.  This gets called by menu choice "zoom in". */
{
	int x, y;

	if ( ! zoom_ok () )
	{
		return;
	}
	zoom_push ();

	x1 = xorg; x2 = xend;
	y1 = yorg; y2 = yend;

	x = dev_x;
	y = dev_y;

	x -= 10; y += 10;

	if ( x >= xorg && x <= xend )
		x1 = x;

	if ( y >= yorg && y <= yend )
		y1 = y;

	set_mark();
	mask_red ();

	mask_line ( x1 - 5, y1, x1 + 5, y1 );
	mask_line ( x1, y1 + 5, x1, y1 - 5 );
	XFlush ( x_display );

	last_x1 = x1; last_y1 = y1;

	zooming = 1;
	set_zoom_menus( True );
}

zoom_first( x, y)
/* Tracks the first part of zooming--moves the red cross around.  This gets
   called when the mouse moves and zooming==1 . */
	int x, y;
{
	x -= 10; y += 10;

	if ( x >= xorg && x <= xend )
		x1 = x;

	if ( y >= yorg && y <= yend )
		y1 = y;

	if ( x1 == last_x1 && y1 == last_y1 )
		return;
	set_unmark();
	mask_line ( last_x1 - 5, last_y1, last_x1 + 5, last_y1 );
	mask_line ( last_x1, last_y1 + 5, last_x1, last_y1 - 5 );
	set_mark();
	mask_line ( x1 - 5, y1, x1 + 5, y1 );
	mask_line ( x1, y1 + 5, x1, y1 - 5 );
	XFlush ( x_display );

	last_x1 = x1; last_y1 = y1;
}

zoom_start2( x, y)
/* Starts the second part of zooming--drawing the red rectangle.  This gets
   called when a mouse button is hit and zooming==1. */
	int x, y;
{

	color = 0;
	line ( last_x1 - 5, last_y1, last_x1 + 5, last_y1 );
	line ( last_x1, last_y1 + 5, last_x1, last_y1 - 5 );
	color = 16;

	last_x2 = x1; last_y2 = y1;

	if ( x >= xorg && x <= xend )
		x2 = x;

	if ( y >= yorg && y <= yend )
		y2 = y;
	set_mark();
	mask_recti ( x1, y1, x2, y2 );
	XFlush ( x_display );

	last_x2 = x2; last_y2 = y2;
	zooming = 2;
}

zoom_second( x, y)
/* Tracks the red rectangle.  Called when the mouse moves and zooming==2. */
	int x, y;
{
	if ( x >= xorg && x <= xend )
		x2 = x;

	if ( y >= yorg && y <= yend )
		y2 = y;

	if ( x2 == last_x2 && y2 == last_y2 )
		return;
	set_unmark();
	mask_recti ( x1, y1, last_x2, last_y2 );
	set_mark();
	mask_recti ( x1, y1, x2, y2 );
	XFlush ( x_display );

	last_x2 = x2; last_y2 = y2;
}

zoom_end()
/* Puts the zoom into effect.  Called when a mouse button is hit and 
   zooming==2. */
{
	if ( x1 <= x2 )
	{
		xsmax = ((int)(( x2 - xorg + 1) * xscale + .5)) + xsmin;
		xsmin = ((int)(( x1 - xorg ) * xscale + .5)) + xsmin;
	}
	else
	{
		xsmax = ((int)(( x1 - xorg + 1) * xscale + .5)) + xsmin;
		xsmin = ((int)(( x2 - xorg ) * xscale + .5)) + xsmin;
	}

	if ( y1 <= y2 )
	{
		ysmax = ( y2 - yorg ) * yscale  + ysmin;
		ysmin = ( y1 - yorg ) * yscale  + ysmin;
	}
	else
	{
		ysmax = ( y1 - yorg ) * yscale  + ysmin;
		ysmin = ( y2 - yorg ) * yscale  + ysmin;
	}


	zoomed = 1;

	redraw_page = 1;
	zooming = 0;
}

set_paging_things ()
{
	char string[20];

	sprintf( string, "%i", page );
	xv_set ( panel_message_page, PANEL_LABEL_STRING, string, NULL );

	if ( page > 1 )
		xv_set ( panel_button_minus, PANEL_INACTIVE, FALSE, NULL );
	else
		xv_set ( panel_button_minus, PANEL_INACTIVE, TRUE, NULL );

	if ( page < ((float) xrange) / (xlen/2) )
		xv_set ( panel_button_plus, PANEL_INACTIVE, FALSE, NULL );
	else
		xv_set ( panel_button_plus, PANEL_INACTIVE, TRUE, NULL );

}

extern set_zoom_menus( );

zoom_to_page()
{
	xsmin = (page - 1) * (xlen / 2);
	xsmax = page * (xlen / 2);

	if ( xsmax > xmax )
		xsmax = xmax;

	set_zoom_menus( TRUE );
	set_paging_things();
	zoomed = 1;
	redraw_page = 1;
}

plus_page_notify()
{
	page++;
	zoom_to_page();
}

minus_page_notify()
{
	page--;
	zoom_to_page();
}
