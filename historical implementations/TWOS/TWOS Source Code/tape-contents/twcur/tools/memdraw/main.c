#include "mplot.h"

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cms.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>

int cno = 0;
int first_nobjs = 0;

/* see "mplot.h" for comments on these variables */
int X1, Y1, X2, Y2;
int xorg, xend, xlen;
int yorg, yend, ylen;
int xmax, xmin, xrange;
double ymax, ymin, yrange;
int addr_xmax, addr_xmin, addr_xrange;
double addr_ymax, addr_ymin, addr_yrange;
int size_xmax, size_xmin, size_xrange;
double size_ymax, size_ymin, size_yrange;
double xscale, yscale;
int width=1152, height=900;
int xsmin, xsmax;
double ysmin, ysmax;
int font_width = 8;
int font_height = 14;
int mode;
int zoomed;
int screen_x1[MAX_ITEMS], screen_x2[MAX_ITEMS];
int screen_y1[MAX_ITEMS], screen_y2[MAX_ITEMS];

	Display * x_display;
	Window x_window;
	GC x_GC;
	GC mask_GC;
	int x_screen;
	unsigned long x_foreground, x_background;
	XEvent x_event;
	KeySym x_key;
	Colormap x_cmp;

	Frame frame, see_colors_frame, text_frame, object_frame, paging_frame;
	Canvas canvas, see_colors_canvas;
	Textsw textsw;
	Panel_item panel_button_plus, panel_button_minus, panel_message_page;

	int dev, val;
	int dev_x, dev_y;
	int redraw_page;
	int identify_on;
	int zooming;

extern OBJ_NAME_ENTRY obj_names[];
extern int nobj_names;

int color = -1;

Rect screen_rect = { 0, 0, 1152, 900 };

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

repaint_proc ()
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
	yorg = 90; yend = height - 51; ylen = yend - yorg + 1;
}

void my_event_proc();

extern repaint_color_settings ();

extern object_func ();

extern plus_page_notify();

extern minus_page_notify();

extern unzoom_notify();

main ( argc, argv )

	int argc;
	char ** argv;
{
	register int i;
	XGCValues x_gcv;
	Xv_font font;
	XFontStruct * x_font_ptr;
	Scrollbar scrollbar;
	Panel panel;
	Panel_item panel_list;

	xv_init ( XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL );

	if ( argc >= 2 )
		read_data ( argv[1] );
	else
		read_data ("");

	/* create frame and canvas */

	height = 700;
	width = 850;


	frame = xv_create ( NULL, FRAME,	/* the main frame */
		FRAME_LABEL, argv[0],
		FRAME_SHOW_LABEL, FALSE,
		XV_HEIGHT, height,
		XV_WIDTH, width,
		WIN_X, 0,
		WIN_Y, 0,
		FRAME_INHERIT_COLORS, TRUE,
		NULL );

		/* the frame for the list of objects */
	object_frame = (Frame) xv_create ( frame, FRAME_CMD,
		XV_HEIGHT, 550,
		XV_WIDTH, 190,
		FRAME_LABEL, "Object view",
		XV_SHOW, FALSE,
		FRAME_SHOW_RESIZE_CORNER, TRUE,
		XV_X, 870,
		XV_Y, 240,
		NULL );

	panel = (Panel) xv_get ( object_frame, FRAME_CMD_PANEL );

	panel_list = (Panel_item) xv_create ( panel, PANEL_LIST,
		PANEL_NOTIFY_PROC, object_func,
		PANEL_LIST_STRINGS, "off", "cycle with right button", NULL,
		PANEL_LIST_CLIENT_DATAS, -1, -2,
		NULL );
	for ( i = 0; i < nobj_names; i++ )
	{
		xv_set ( panel_list,
			PANEL_LIST_INSERT, i+2,
			PANEL_LIST_STRING, i+2, obj_names[i].name,
			PANEL_LIST_CLIENT_DATA, i+2, i,
			NULL );
	}
	xv_set ( panel_list,
		PANEL_LIST_DISPLAY_ROWS,
			MIN ( 28, xv_get (panel_list, PANEL_LIST_NROWS) ),
		NULL );

		/* the frame for paging */ 
	paging_frame = (Frame) xv_create ( frame, FRAME_CMD,
		XV_HEIGHT, 50,
		XV_WIDTH, 190,
		FRAME_LABEL, "Zoom by page",
		XV_SHOW, FALSE,
		XV_X, 870,
		XV_Y, 720,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		PANEL_LABEL_STRING, "Page 1",
		NULL );

	panel = (Panel) xv_get ( paging_frame, FRAME_CMD_PANEL );

	panel_button_minus = xv_create ( panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, minus_page_notify,
		PANEL_LABEL_STRING, "-",
		NULL );

	panel_message_page = xv_create ( panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "1",
		NULL);

	panel_button_plus = xv_create ( panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, plus_page_notify,
		PANEL_LABEL_STRING, "+",
		NULL );

	xv_create ( panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, unzoom_notify,
		PANEL_LABEL_STRING, "end",
		NULL );

		/* the "identify info" frame */
	text_frame = (Frame) xv_create ( frame, FRAME,
		XV_HEIGHT, 150,
		XV_WIDTH, 850,
		FRAME_LABEL, "Memdraw info",
		XV_SHOW, FALSE,
		FRAME_SHOW_RESIZE_CORNER, TRUE,
		XV_Y, 720,
		NULL );

	textsw = (Textsw) xv_create ( text_frame, TEXTSW, 
		TEXTSW_BROWSING, TRUE,
		TEXTSW_DISABLE_CD, TRUE,
		TEXTSW_DISABLE_LOAD, TRUE,
		NULL);

	scrollbar = (Scrollbar) xv_create ( textsw, SCROLLBAR,
		SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
		NULL );

		/* the "see colors" frame */
	see_colors_frame = (Frame) xv_create ( frame, FRAME,
		XV_HEIGHT, 200,
		XV_WIDTH, 200,
		FRAME_LABEL, "color settings",
		XV_SHOW, FALSE,
		FRAME_SHOW_RESIZE_CORNER, TRUE,
		XV_X, 870,
		NULL );

	see_colors_canvas = (Canvas) xv_create (see_colors_frame, CANVAS,
		CANVAS_REPAINT_PROC, repaint_color_settings,
		CANVAS_AUTO_EXPAND, TRUE,
		CANVAS_AUTO_SHRINK, TRUE,
		WIN_DYNAMIC_VISUAL, TRUE,
		CANVAS_HEIGHT, 200,
		CANVAS_WIDTH, 200,
		WIN_X, 0,
		WIN_Y, 0,
		NULL);


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

		/* the main drawing canvas */
	canvas = xv_create ( frame, CANVAS,
		CANVAS_REPAINT_PROC, repaint_proc,
		CANVAS_RESIZE_PROC, resize_proc,
		CANVAS_RETAINED, FALSE,
		CANVAS_MIN_PAINT_WIDTH, 800,
		CANVAS_MIN_PAINT_HEIGHT, 700,
		CANVAS_AUTO_EXPAND, TRUE,
		CANVAS_AUTO_SHRINK, TRUE,
		WIN_DYNAMIC_VISUAL, TRUE,
		CANVAS_HEIGHT, 900,
		CANVAS_WIDTH, 1152,
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

	init_menus ();

	init_var();

	idle_mode();
	xv_main_loop ( frame );
}
