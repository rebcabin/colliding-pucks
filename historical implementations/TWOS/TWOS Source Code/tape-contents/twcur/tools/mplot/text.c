/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef X11
#include <X11/Xlib.h>

extern Display * x_display;
extern Window x_window;
extern GC x_GC;
extern GC mask_GC;
extern int x_screen;
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>

extern Pixwin *pw;
#endif

text ( x, y, string )

	int x, y;
	char * string;
{
	y = height - y;
#ifdef SUNVIEW
	pw_text ( pw, x, y, PIX_SRC | PIX_COLOR(color), 0, string );
#endif
#ifdef X11
	XDrawString ( x_display, x_window, x_GC, x, y, string, strlen (string) );
#endif
}

#ifdef X11
mask_text ( x, y, string )

	int x, y;
	char * string;
{
	y = height - y;

	XDrawString ( x_display, x_window, mask_GC, x, y, string, strlen (string) );
}
#endif

center ( string, x1, y1, x2, y2 )

	char * string;
	int x1, y1, x2, y2;
{
	int xlen, ylen;
	int twidth, theight;
	int x, y;

	xlen = x2 - x1 + 1;
	ylen = y2 - y1 + 1;

	twidth  = font_width * strlen ( string );
	theight = font_height;

	x = ( xlen - twidth  ) / 2 + x1;
	y = ( ylen - theight ) / 2 + y1;

	text ( x, y, string );
}

xcenter ( string, x1, x2, y )

	char * string;
	int x1, x2, y;
{
	int xlen;
	int twidth;
	int x;

	xlen = x2 - x1 + 1;

	twidth  = font_width * strlen ( string );

	x = ( xlen - twidth  ) / 2 + x1;

	text ( x, y, string );
}

ycenter ( string, x, y1, y2 )

	char * string;
	int x, y1, y2;
{
	int ylen, y;

	ylen = y2 - y1 + 1;

	y = ( ylen - font_height ) / 2 + y1;

	text ( x, y, string );
}

rjust ( string, x2, y1, y2 )

	char * string;
	int x2, y1, y2;
{
	int ylen;
	int twidth, theight;
	int x, y;

	ylen = y2 - y1 + 1;

	twidth  = font_width * strlen ( string );
	theight = font_height;

	x = x2 - twidth;
	y = ( ylen - theight ) / 2 + y1;

	text ( x, y, string );
}

vertical_text ( x, y, string )

	int x, y;
	char * string;
{
	int len = strlen ( string );
	int i;

	y = height - y;
 
	for ( i = 0; i < len; i++ )
	{
		static char c[2];
 
		c[0] = string[i];

#ifdef SUNVIEW
		pw_text ( pw, x, y, PIX_SRC | PIX_COLOR(color), 0, c);
#endif
#ifdef X11
		XDrawString ( x_display, x_window, x_GC, x, y, c, 1 );
#endif
 
		y += font_height;
	}
}

vertical_center ( string, x1, y1, x2, y2 )

	char * string;
	int x1, y1, x2, y2;
{
	int xlen, ylen, tlen;
	int twidth, theight;
	int x, y;

	xlen = x2 - x1 + 1;
	ylen = y2 - y1 + 1;

	tlen = strlen ( string );

	twidth  = font_width;
	theight = font_height * tlen;

	x = ( xlen - twidth  ) / 2 + x1;
	y = ( ylen - theight ) / 2 + y1 + theight;

	vertical_text ( x, y, string );
}

banner()
{
	char string[5000];

	line ( xorg, yend, xend, yend );    /* top */
	line ( xorg, yorg, xend, yorg );    /* bottom */
	line ( xend, yend, xend, yorg );    /* right */
	line ( xorg, yend, xorg, yorg );    /* left */

	center ( "Timewarp Message Graph", xorg, yend, xend, height );

	if ( flat )
	{
		if ( scaling_y )
		{
			vertical_center ( "Relative Virtual Time", 0, yorg, xorg, yend );

			sprintf ( string, "%d", max_ydiff );
		}
		else
		{
			vertical_center ( "Virtual Time minus LVT", 0, yorg, xorg, yend );

			sprintf ( string, "%.2lf", ysmax - ysmin );
		}

		xcenter ( "0", 0, xorg, yorg ); /* lower left side */

		center ( string, 0, yend - font_height, xorg, yend ); /* upper left */
	}
	else
	{
		vertical_center ( "Virtual Time", 0, yorg, xorg, yend );

		sprintf ( string, "%.2lf", ysmin );
		xcenter ( string, 0, xorg, yorg );      /* lower left side */

		sprintf ( string, "%.2lf", ysmax );
		center ( string, 0, yend - font_height, xorg, yend ); /* upper left */
	}

	if ( stb89 )
	{
		if ( select_node == nodes )
			sprintf ( string, "S T B 8 9   %d Nodes", nodes );
		else
			sprintf ( string, "S T B 8 9   Node %d", select_node );
	}
	else
	if ( stb88 )
	{
		if ( select_node == nodes )
			sprintf ( string, "S T B 8 8   %d Nodes", nodes );
		else
			sprintf ( string, "S T B 8 8   Node %d", select_node );
	}
	else
	if ( ctls )
	{
		if ( select_node == nodes )
			sprintf ( string, "C T L S   %d Nodes", nodes );
		else
			sprintf ( string, "C T L S   Node %d", select_node );
	}
	else
	if ( pucks )
	{
		if ( select_node == nodes )
			sprintf ( string, "P U C K S   %d Nodes", nodes );
		else
			sprintf ( string, "P U C K S   Node %d", select_node );
	}
	else
	if ( ants )
	{
		if ( select_node == nodes )
			sprintf ( string, "A N T S   %d Nodes", nodes );
		else
			sprintf ( string, "A N T S   Node %d", select_node );
	}
	else
	if ( slooow )
	{
		if ( select_node == nodes )
			sprintf ( string, "S L O O O W   %d Nodes", nodes );
		else
			sprintf ( string, "S L O O O W   Node %d", select_node );
	}
	else
	{
		if ( select_node == nodes )
			sprintf ( string, "%d Nodes", nodes );
		else
			sprintf ( string, "Node %d", select_node );
	}

	vertical_center ( string, xend, yorg, width, yend );        /* right side */

	sprintf ( string, "Measured CPU Seconds - Page %d of %d",
		xpage+1,xnum_pages );

	center ( string, xorg, 0, xend, yorg );     /* below */

	sprintf ( string, "%f", (float)(xsmin-xmin) * 62.5 / 1000000. );
	ycenter ( string, xorg, 0, yorg );  /* bottom left */

	sprintf ( string, "%f", (float)(xsmax-xmin) * 62.5 / 1000000. );
	rjust ( string, xend, 0, yorg );            /* bottom right */

	if ( select_obj < nobjs )
	{
		text ( xorg + 20, yend - font_height, obj[select_obj].name );
	}
}
