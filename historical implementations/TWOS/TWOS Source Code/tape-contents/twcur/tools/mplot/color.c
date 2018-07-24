/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	
#ifdef X11
#include "mplot.h"
#include <X11/Xlib.h>

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
mask_clear()
{
	set_unmark();
	XFillRectangle ( x_display, x_window, mask_GC, 0, 0, width, height );
	set_mark();
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
	x_background = pixels[i];
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "red", pixels[i], ALL ) )
		printf ( "red failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "green", pixels[i], ALL ) )
		printf ( "green failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "yellow", pixels[i], ALL ) )
		printf ( "yellow failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "blue", pixels[i], ALL ) )
		printf ( "blue failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "magenta", pixels[i], ALL ) )
		printf ( "magenta failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "cyan", pixels[i], ALL ) )
		printf ( "cyan failed\n" );
	i++;

	if ( XStoreNamedColor ( x_display, x_cmp, "white", pixels[i], ALL ) )
		printf ( "white failed\n" );
	x_foreground = pixels[i];
	i++;

	mask_yellow();
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
#endif

#ifdef SUNVIEW
#include "mplot.h"
#include <suntool/sunview.h>
#include <suntool/canvas.h>

extern Pixwin *pw;
extern unsigned char red[], green[], blue[];

writemask ( planes )
	int planes;
{
	if ( planes == -1 )
		planes = 31;

	pw_putattributes ( pw, &planes );
}

mapcolor ( i, r, g, b )
	int i, r, g, b;
{
	red[i] = r;
	green[i] = g;
	blue[i] = b;
}

clear ()
{
	pw_writebackground ( pw, 0, 0, width, height, PIX_SRC );
}

init_colormap()
{
	int i;

	for ( i = 8; i < 16; i++ )
		mapcolor ( i, 255, 255, 0 );
	for ( i = 16; i < 32; i++ )
		mapcolor ( i, 255, 0, 0 );
	for ( i = 32; i < 256; i++ )
		mapcolor ( i, 200, 200, 200 );
}

change_color ( new_color )
	int new_color;
{
	color = new_color;
}
#endif
