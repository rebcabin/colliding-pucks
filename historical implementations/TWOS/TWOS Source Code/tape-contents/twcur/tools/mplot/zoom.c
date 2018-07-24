/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef X11
#include <X11/Xlib.h>

extern Display * x_display;
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#endif

int zoomed;
static int zsp = 0;

#define ZSTKSIZE 20

struct
{
	int xsmin, xsmax, ysmin, ysmax;
	int xpagediv, xnum_pages, xpage;
	int scaling_y;
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
	zstk[zsp].xpagediv = xpagediv;
	zstk[zsp].xnum_pages = xnum_pages;
	zstk[zsp].xpage = xpage;
	zstk[zsp].scaling_y = scaling_y;
	zsp++;
}

zoom_pop ()
{
	zsp--;
	xsmin = zstk[zsp].xsmin;
	xsmax = zstk[zsp].xsmax;
	ysmin = zstk[zsp].ysmin;
	ysmax = zstk[zsp].ysmax;
	xpagediv = zstk[zsp].xpagediv;
	xnum_pages = zstk[zsp].xnum_pages;
	xpage = zstk[zsp].xpage;
	scaling_y = zstk[zsp].scaling_y;
}

static int x1, x2, last_x1, last_x2;
static int y1, y2, last_y1, last_y2;

zoom_start()
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

#ifdef SUNVIEW
	writemask ( 16 );
	color = 16;

	line ( x1 - 5, y1, x1 + 5, y1 );
	line ( x1, y1 + 5, x1, y1 - 5 );
#endif
#ifdef X11
	set_mark();
	mask_red ();

	mask_line ( x1 - 5, y1, x1 + 5, y1 );
	mask_line ( x1, y1 + 5, x1, y1 - 5 );
	XFlush ( x_display );
#endif

	last_x1 = x1; last_y1 = y1;

	zooming = 1;
	unzoom_menu_on();
}

zoom_first( x, y)
	int x, y;
{
	x -= 10; y += 10;

	if ( x >= xorg && x <= xend )
		x1 = x;

	if ( y >= yorg && y <= yend )
		y1 = y;

	if ( x1 == last_x1 && y1 == last_y1 )
		return;
#ifdef SUNVIEW
	color = 0;
	line ( last_x1 - 5, last_y1, last_x1 + 5, last_y1 );
	line ( last_x1, last_y1 + 5, last_x1, last_y1 - 5 );
	color = 16;
	line ( x1 - 5, y1, x1 + 5, y1 );
	line ( x1, y1 + 5, x1, y1 - 5 );
#endif
#ifdef X11
	set_unmark();
	mask_line ( last_x1 - 5, last_y1, last_x1 + 5, last_y1 );
	mask_line ( last_x1, last_y1 + 5, last_x1, last_y1 - 5 );
	set_mark();
	mask_line ( x1 - 5, y1, x1 + 5, y1 );
	mask_line ( x1, y1 + 5, x1, y1 - 5 );
	XFlush ( x_display );
#endif

	last_x1 = x1; last_y1 = y1;
}

zoom_start2( x, y)
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
#ifdef SUNVIEW
	color = 16;
	recti ( x1, y1, x2, y2 );
#endif
#ifdef X11
	set_mark();
	mask_recti ( x1, y1, x2, y2 );
	XFlush ( x_display );
#endif

	last_x2 = x2; last_y2 = y2;
	zooming = 2;
}

zoom_second( x, y)
	int x, y;
{
	if ( x >= xorg && x <= xend )
		x2 = x;

	if ( y >= yorg && y <= yend )
		y2 = y;

	if ( x2 == last_x2 && y2 == last_y2 )
		return;
#ifdef SUNVIEW
	color = 0;
	recti ( x1, y1, last_x2, last_y2 );
	color = 16;
	recti ( x1, y1, x2, y2 );
#endif
#ifdef X11
	set_unmark();
	mask_recti ( x1, y1, last_x2, last_y2 );
	set_mark();
	mask_recti ( x1, y1, x2, y2 );
	XFlush ( x_display );
#endif

	last_x2 = x2; last_y2 = y2;
}

zoom_end()
{
#ifdef SUNVIEW
	writemask ( -1 );
#endif
	if ( x1 <= x2 )
	{
		xsmax = ((int)(( x2 - xorg ) * xscale + .5)) + xsmin;
		xsmin = ((int)(( x1 - xorg ) * xscale + .5)) + xsmin;
	}
	else
	{
		xsmax = ((int)(( x1 - xorg ) * xscale + .5)) + xsmin;
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

	xpagediv = 1;
	xnum_pages = 1;
	xpage = 0;

	zoomed = 1;
	scaling_y = 0;

	redraw_page = 1;
	zooming = 0;
}
