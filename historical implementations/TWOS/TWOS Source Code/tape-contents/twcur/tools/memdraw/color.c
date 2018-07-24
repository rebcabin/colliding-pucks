#include "mplot.h"

#include <X11/Xlib.h>

/* This stores the color settings for the "by address" display */
COLOR_SETTING color_settings[MAX_COLOR_SETTINGS];
int set_color_type = FREE_MEM, ncolor_settings = 0;

extern Display * x_display;
extern Window x_window;
extern unsigned long x_foreground, x_background;
extern GC x_GC;
extern GC mask_GC;
extern int x_screen;
extern Colormap x_cmp;

#define ALL (DoRed | DoGreen | DoBlue)

#define N_PLANES	1
#define N_COLORS	8
unsigned long plane_masks[N_PLANES];
unsigned long pixels[N_COLORS];

clear()
{
	XSetForeground ( x_display, x_GC, x_background );
	XFillRectangle ( x_display, x_window, x_GC, 0, 0, width, height );
	XSetForeground ( x_display, x_GC, pixels[color] );
}

clear_rectangle( x1, y1, width, height )
int x1, y1, width, height;
{
	XSetForeground ( x_display, x_GC, x_background );
	XFillRectangle ( x_display, x_window, x_GC, x1, y1, width, height );
	XSetForeground ( x_display, x_GC, pixels[color] );
}

mask_clear()
{
	set_unmark();
	XFillRectangle ( x_display, x_window, mask_GC, 0, 0, width, height );
	set_mark();
}

the_mask()
{
	return plane_masks[0];
}

mask_color()
{
	return plane_masks[0] | pixels[0];
}

change_color ( new_color )
	int new_color;
{
	if ( new_color != color )
	{
		color = new_color;
		
		XSetForeground ( x_display, x_GC, pixels[color] );
	}
}

#define MARK	1
#define UNMARK  0

int old_mark = MARK;
int old_mask_color = RED;

set_mark ()
{
	if ( old_mark == UNMARK )
	{
		old_mark = MARK;
		XSetForeground ( x_display, mask_GC, pixels[0]|plane_masks[0] );
	}
}

set_unmark ()
{
	if ( old_mark == MARK )
	{
		old_mark = UNMARK;
		XSetForeground ( x_display, mask_GC, pixels[0] );
	}
}

mask_red()
{
	int i;

	if ( old_mask_color != RED )
	{
		old_mask_color = RED;
		for ( i = 0; i < N_COLORS; i++ )
			if ( XStoreNamedColor ( x_display, x_cmp, "red",
					plane_masks[0] | pixels[i], ALL ) )
				printf ( "red failed\n" );
	}
}

mask_yellow()
{
	int i;

	if ( old_mask_color != YELLOW )
	{
		old_mask_color = YELLOW;
		for ( i = 0; i < N_COLORS; i++ )
			if ( XStoreNamedColor ( x_display, x_cmp, "yellow",
					plane_masks[0] | pixels[i], ALL ) )
				printf ( "yellow failed\n" );
	}
}

mask_black()
{
	int i;

	if ( old_mask_color != BLACK )
	{
		old_mask_color = BLACK;
		for ( i = 0; i < N_COLORS; i++ )
			if ( XStoreNamedColor ( x_display, x_cmp, "black",
					plane_masks[0] | pixels[i], ALL ) )
				printf ( "black failed\n" );
	}
}

init_colormap()
{
	int i;

	x_cmp = DefaultColormap ( x_display, x_screen );

	if ( ! XAllocColorCells ( x_display, x_cmp, False /* contig */,
		plane_masks, N_PLANES, pixels, N_COLORS ) )
	{
		printf ( "XAllocColorCells failed: not enough colors\n" );
		exit ( 1 );
	}

	i = 0;

	if ( XStoreNamedColor ( x_display, x_cmp, "black", pixels[i], ALL ) )
		printf ( "black failed\n" );
	x_foreground = pixels[i];
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "red", pixels[i], ALL ) )
		printf ( "red failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "green", pixels[i], ALL ) )
		printf ( "green failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "gold", pixels[i], ALL ) )
		printf ( "yellow failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "blue", pixels[i], ALL ) )
		printf ( "blue failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "magenta", pixels[i], ALL ) )
		printf ( "magenta failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "ForestGreen", pixels[i], ALL ) )
		printf ( "cyan failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "white", pixels[i], ALL ) )
		printf ( "white failed\n" );
	x_background = pixels[i];
	i++;

	mask_yellow();
}
