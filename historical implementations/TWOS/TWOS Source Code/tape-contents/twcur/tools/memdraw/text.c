#include "mplot.h"
#include "math.h"

#define SPACE_FOR_COUNT 50

extern int obj_cycle, show_object;
extern OBJ_NAME_ENTRY obj_names[];

#include <X11/Xlib.h>

extern Display * x_display;
extern Window x_window;
extern GC x_GC;
extern GC mask_GC;
extern int x_screen;

text ( x, y, string )
/* Draws "string" at window coordinates x,y */

	int x, y;
	char * string;
{
	y = height - y;
	XDrawString ( x_display, x_window, x_GC, MAX(x,0), y, string, strlen (string) );
}

scale_text ( x, y, string )
/* Draws "string" at _data_ coordinates x,y */

	int x;
	double y;
	char * string;
{
	x = ( x - xsmin ) / xscale + xorg;
	y = ( y - ysmin ) / yscale + yorg;

	if (x > xend + 3)
	{
		x = xend + 1;
		strcat ( string, "->" );
	}

	if (x >= xorg && y >= yorg && y <= yend)
		text ( x, (int) y, string);
}

scale_text_left ( x, y, string )
/* Draws "string" to the left of _data_ coordinates x,y */

	int x;
	double y;
	char * string;
{
	x = ( x - xsmin ) / xscale + xorg;
	y = ( y - ysmin ) / yscale + yorg;

	if (x < xorg)
		x = xorg;
	if (x <= xend && y >= yorg - font_height && y <= yend + font_height)
		text ( x - strlen(string) * font_width, (int) y, string);

}

center ( string, x1, y1, x2, y2 )
/* Draws "string" centered in rectangle bounded by screen coordinates */

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

vertical_text ( x, y, string )

	int x, y;
	char * string;
{
	int len = strlen ( string );
	int i;

	y = height - y + font_height;
 
	for ( i = 0; i < len; i++ )
	{
		static char c[2];
 
		c[0] = string[i];

		XDrawString ( x_display, x_window, x_GC, x, y, c, 1 );
 
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
/* Draws the text around the edges of the main display and the scale lines
   on the "by address" graph */
{
	char string[500];
	double i;

	line ( xorg, yend, xend, yend );    /* top */
	line ( xorg, yorg, xend, yorg );    /* bottom */
	line ( xend, yend, xend, yorg );    /* right */
	line ( xorg, yend, xorg, yorg );    /* left */

	if (mode == BY_SIZE)
	{
		center ( "Timewarp Memory Usage by 'Chunk' Size", xorg, yend, xend, height );
		if ( show_object != -1 )
		{
			sprintf ( string, "currently viewing memory for object %s", obj_names[show_object].name );
			center ( string, xorg, yend, xend, height - font_height * 2 );
		}

		vertical_center ( "Size of chunk (bytes)", 0, yorg, font_width * 2, yend );

		center ( "Number of chunks", xorg, 0, xend, yorg); 
	}

	else
	/* display mode is by address */
	{

		/* Draw scale lines to indicate sizes */
		for (i=ysmin; i <= ysmax; i += (ysmax - ysmin) / 10.0)
		{
			scale_line ( xsmin, i, xsmax, i);
			sprintf ( string, "%i", (int) (16 * exp ( i )) );
			scale_text_left ( xsmin, i, string );
		}


		center ( "Timewarp Memory Usage by Address", xorg, yend, xend, height );
		if ( show_object != -1 )
		{
			sprintf ( string, "currently viewing memory for object %s", obj_names[show_object].name );
			center ( string, xorg, yend, xend, height - font_height * 2 );
		}

		vertical_center ( "Size of chunk", 0, yorg, font_width * 2, yend );
	
		center ( "Address -->", xorg, yorg, xend, yorg - font_height); 
};


}
