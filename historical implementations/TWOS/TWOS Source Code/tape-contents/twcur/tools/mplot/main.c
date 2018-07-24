/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef XVIEW
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cms.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#endif

ObjArray obj[MAX_OBJS];

int nobjs = 0, nodes = 0;
int cno = 0;
int shift_up = 0;
int first_nobjs = 0;

COBJ cobj[MAX_ITEMS];

int vt0;
int X1, Y1, X2, Y2, Z1, Z2;
int charx, chary;
int xorg, xend, xlen, xmax, xmin, xrange;
int yorg, yend, ylen;
double ymax, ymin, yrange;
double xscale, yscale;
int width=1152, height=900;
int xsmin, xsmax, xpagediv, xpage, xpagesize, xnum_pages;
double ysmin, ysmax, ypagesize;
int ypagediv, ypage, ynum_pages;
int max_ydiff;
int font_width = 8;
int font_height = 14;
int select_node;
int select_obj;
int flat, antimessages, committed, rollbacks, scaling_y;
double low_vt[1152], high_vt[1152];
double last_vt[MAX_OBJS];
int zoomed;
int annotate;
int highlight;
int search;
int page_scan;
int from_to;
int screen_x1[MAX_ITEMS];
int screen_x2[MAX_ITEMS];
int screen_y1[MAX_ITEMS];
int screen_y2[MAX_ITEMS];

int list[2][LISTSIZE];
int il=0, ol=1;
int nlist[2];
int identi;
int listx, listy;

int stb89, stb88, ctls, pucks, ants, slooow;

#ifdef X11
	Display * x_display;
	Window x_window;
	GC x_GC;
	GC mask_GC;
	int x_screen;
	unsigned long x_foreground, x_background;
	XEvent x_event;
	KeySym x_key;
	Colormap x_cmp;
#endif

	Frame frame;
	Canvas canvas;

#ifdef SUNVIEW
	Pixwin *pw;
 
unsigned char red   [256] = { 0, 255,   0, 255,   0, 255,   0, 255 };
unsigned char green [256] = { 0,   0, 255, 255,   0,   0, 255, 255 };
unsigned char blue  [256] = { 0,   0,   0,   0, 255, 255, 255, 255 };

static char cmsname[CMS_NAMESIZE] = "mplot";
#endif
 
	int dev, val;
	int dev_x, dev_y;
	int redraw_page;
	int identify_on;
	int zooming;

int color = -1;

Rect screen_rect = { 0, 0, 1152, 900 };

#ifdef X11

recti ( x1, y1, x2, y2 )
	int x1, y1, x2, y2;
{
	XDrawRectangle ( x_display, x_window, x_GC,
		x1, height - y1, x2 - x1, y1 - y2 );
}

mask_recti ( x1, y1, x2, y2 )
	int x1, y1, x2, y2;
{
	XDrawRectangle ( x_display, x_window, mask_GC,
		x1, height - y1, x2 - x1, y1 - y2 );
}
#endif

#ifdef SUNVIEW
Cursor cursor;
 
recti ( x1, y1, x2, y2 )
 
	int x1, y1, x2, y2;
{
	line ( x1, y1, x1, y2 );
	line ( x1, y1, x2, y1 );
	line ( x1, y2, x2, y2 );
	line ( x2, y1, x2, y2 );
}
#endif

repaint_proc (/* c, pwin, dpy, xwin, xrect */)
/*
	Canvas c;
	Xv_Window pwin;
	Display *dpy;
	Window xwin;
	Xv_xrectlist *xrect;
*/
{
	redraw_page = 1;
	identify_on = 0;
	zooming = 0;
}

resize_proc (c, new_w, new_h)
	Canvas c;
	int new_w;
	int new_h;
{

	width = new_w;
	height = new_h;

	xorg = 60; xend = width  - 41; xlen = xend - xorg + 1;
	yorg = 50; yend = height - 51; ylen = yend - yorg + 1;
}

void my_event_proc();

main ( argc, argv )

	int argc;
	char ** argv;
{
	register int i;
	char * flowname = "";
	char * secondname = "";
	char * anothername = "";
#ifdef XVIEW
	XGCValues x_gcv;
	Xv_font font;
	XFontStruct * x_font_ptr;

	xv_init ( XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL );
#endif

	if ( argc >= 2 )
	{
		flowname = argv[1];
	}

	read_flow_data ( flowname );

	if ( argc >= 3 )
	{
		secondname = argv[2];
		shift_up = 10;
		if ( argc >= 4 )
		{ 
			sscanf ( argv[3], "%d", &shift_up);
		}
		read_flow_data ( secondname );
	}

	if ( argc >= 5 )
	{
		anothername = argv[4];
		shift_up *= 2;
		read_flow_data ( anothername );
	}

	/* create frame and canvas */

#ifdef XVIEW
	height = 700;
	width = 800;


	frame = xv_create ( NULL, FRAME,
		FRAME_LABEL, argv[0],
		FRAME_SHOW_LABEL, FALSE,
		XV_HEIGHT, height,
		XV_WIDTH, width,
		WIN_X, 0,
		WIN_Y, 0,
		NULL );

	font = (Xv_font) xv_find ( frame, FONT, FONT_NAME, "fixed", NULL );
	if ( !font )
	{
		printf ( "font not found\n" );
		font = (Xv_font) xv_get (frame, XV_FONT );
	}

	x_display = (Display *) xv_get ( frame, XV_DISPLAY );
	x_screen = DefaultScreen ( x_display );

	x_foreground = BlackPixel ( x_display, x_screen );
	x_background = WhitePixel ( x_display, x_screen );

	init_colormap();

	canvas = xv_create ( frame, CANVAS,
		CANVAS_REPAINT_PROC, repaint_proc,
		CANVAS_RESIZE_PROC, resize_proc,
		CANVAS_RETAINED, FALSE,
		CANVAS_MIN_PAINT_WIDTH, 800,
		CANVAS_MIN_PAINT_HEIGHT, 700,
		CANVAS_AUTO_EXPAND, TRUE,
		CANVAS_AUTO_SHRINK, TRUE,
		WIN_DYNAMIC_VISUAL, TRUE,
		XV_HEIGHT, 900,
		XV_WIDTH, 1152,
		WIN_X, 0,
		WIN_Y, 0,
		NULL );

	xv_set ( canvas_paint_window ( canvas ),
		WIN_CONSUME_EVENT, WIN_ASCII_EVENTS,
		WIN_CONSUME_EVENT, LOC_MOVE,
		WIN_EVENT_PROC, my_event_proc,
		XV_FONT, font,
		NULL );
	x_window = xv_get ( canvas_paint_window ( canvas ), XV_XID );

	x_gcv.font = (Font) xv_get ( font, XV_XID );
	x_gcv.graphics_exposures = False;
	x_gcv.foreground = x_foreground;
	x_gcv.background = x_background;

	x_GC = XCreateGC ( x_display, x_window,
		GCForeground | GCBackground | GCFont | GCGraphicsExposures,
		&x_gcv );

	x_gcv.plane_mask = the_mask();
	x_gcv.foreground = mask_color();

	mask_GC = XCreateGC ( x_display, x_window,
		GCPlaneMask| GCForeground | GCBackground | GCFont | GCGraphicsExposures,
		&x_gcv );

	change_color ( RED );

	init_menus ();

	init_var();

	idle_mode();
	xv_main_loop ( frame );
#endif

#ifdef SUNVIEW
	frame = window_create ( NULL, FRAME,
		FRAME_SHOW_LABEL, FALSE,
		WIN_HEIGHT, height,
		WIN_WIDTH, width,
		WIN_X, 0,
		WIN_Y, 0,
		0 );

	canvas = window_create ( frame, CANVAS,
		WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS,
		WIN_EVENT_PROC, my_event_proc,
		CANVAS_RETAINED, FALSE,
		CANVAS_REPAINT_PROC, repaint_proc,
		CANVAS_RESIZE_PROC, resize_proc,
		WIN_HEIGHT, height,
		WIN_WIDTH, width,
		WIN_X, 0,
		WIN_Y, 0,
		0 );

	cursor = window_get ( canvas, WIN_CURSOR );

	pw = (Pixwin *) window_get ( canvas, WIN_PIXWIN );

	init_colormap();

	pw_setcmsname ( pw, cmsname );
 
	pw_putcolormap ( pw, 0, 64, red, green, blue );

	pw = canvas_pixwin ( canvas );
 
	pw_setcmsname ( pw, cmsname );
 
	pw_putcolormap ( pw, 0, 64, red, green, blue );

	window_set ( frame, WIN_SHOW, TRUE, 0 );

	init_menus ();

	init_var();

	redraw_page = 1;
	writemask ( 255 );
	plot_page ();
	window_main_loop ( frame );
#endif
}

init_var()
{
	register int i;

	select_node = nodes;        /* all */
	select_obj = nobjs;         /* all */

	xorg = 60; xend = width  - 41; xlen = xend - xorg + 1;
	yorg = 50; yend = height - 51; ylen = yend - yorg + 1;

	vt0 = 0;

	xmin = cobj[vt0].cputims;
	xmax = cobj[vt0].cputimr;
	ymin = cobj[vt0].sndtim;
	ymax = cobj[vt0].rcvtim;

	for ( i = vt0; i < cno; i++ )
	{
		if ( xmin > cobj[i].cputims ) xmin = cobj[i].cputims;
		if ( xmax < cobj[i].cputimr ) xmax = cobj[i].cputimr;
		if ( ymin > cobj[i].sndtim ) ymin = cobj[i].sndtim;
		if ( ymax < cobj[i].rcvtim ) ymax = cobj[i].rcvtim;
	}

	xrange = xmax - xmin + 1;
	yrange = ymax - ymin;

	xsmax = xmax; xsmin = xmin;
	ysmax = ymax; ysmin = ymin;

	printf ( "xmin %d xmax %d xrange %d ymin %f ymax %f yrange %f\n",
		xmin, xmax, xrange, ymin, ymax, yrange );

	xpagediv = ypagediv = 1;

	for ( i = vt0; i < cno; i++ )
	{
		register int objno = cobj[i].snder;

		register char * name = obj[objno].name;

		if ( strncmp ( name, "RedDiv", 6 ) == 0 )
		{
			stb89 = 1;
			break;
		}

		if ( strncmp ( name, "red_div", 7 ) == 0 )
		{
			if ( strlen ( name ) > 9 )
				stb88 = 1;
			else
				ctls = 1;
			break;
		}

		if ( strncmp ( name, "puck", 4 ) == 0 )
		{
			pucks = 1;
			break;
		}

		if ( strncmp ( name, "ant", 3 ) == 0 )
		{
			ants = 1;
			break;
		}

		if ( strncmp ( name, "slooow", 6 ) == 0 )
		{
			slooow = 1;
			break;
		}
	}
}
