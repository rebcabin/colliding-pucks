/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */ 
	 
#include <fcntl.h>
#include <stdio.h>

#define MAX_OBJS 1000
#define MAX_NODES 128
#define BLOCKSIZE 128

struct
{
    char name[20];
    int node;
    int exc;
    int cpu;
}
    obj[MAX_OBJS];

int nobjs, nodes;

int tot_exc, tot_cpu;

int node_exc[MAX_NODES];
int node_cpu[MAX_NODES];

typedef struct
{
    int objno;
    float vt;
    int cpuf;
    int cput;
}
    COBJ;

#define MAX_ITEMS (BLOCKSIZE*3420)

/*COBJ cobj[MAX_ITEMS+BLOCKSIZE];*/
int cobjsize;
COBJ * cobj;

int cno;

read_flow_data ( flowname )

    char * flowname;
{
    FILE * fp;
    int i, n, node, cpuf, cput, cpu;
    float vt;
    char object[20];
    char filename[50];

    strcpy ( filename, flowname );
    strcat ( filename, "cname" );

    fp = fopen ( filename, "r" );

    if ( fp == 0 )
    {
	printf ( "File %s does not exits\n", filename );
	exit (0);
    }

    while ( ( n = fscanf ( fp, "%s %d", object, &node ) ) == 2 )
    {
	for ( i = 0; i < nobjs; i++ )
	{
	    if ( strcmp ( object, obj[i].name ) == 0 )
		break;
	}
	if ( i == nobjs )
	{
	    if ( i == MAX_OBJS )
	    {
		fprintf ( stderr, "exceeded %d objects\n", i );
		exit (1);
	    }
	    strcpy ( obj[i].name, object );
	    obj[i].node = node;
	    nobjs++;
	}
    }

    fclose ( fp ) ;

    strcpy ( filename, flowname );
    strcat ( filename, "cflow" );

    fp = fopen ( filename, "r" );

    if ( fp == 0 )
    {
	printf ( "File %s does not exits\n", filename );
	exit (0);
    }

    fseek ( fp, 0, 2 );
    cobjsize = ftell ( fp );

    fclose ( fp );

    printf ( "%s size = %d\n", filename, cobjsize );

    cobj = (COBJ *) valloc ( cobjsize );

    if ( cobj == 0 )
	exit (0);

    printf ( "Reading %s\n", filename );

    getin ( filename );

    for ( n = 0; n < cno; n++ )
    {
	i = cobj[n].objno;

	cpu = cobj[n].cput - cobj[n].cpuf;

	obj[i].exc ++;
	obj[i].cpu += cpu;

	node = obj[i].node;

	if ( node > nodes )
	    nodes = node;
    }

    nodes++;

#ifdef PRINT
    printf ( "Object        Execs    Time\n\n" );

    for ( i = 0; i < nobjs; i++ )
    {
	printf ( "%-12s %6d %7d\n", obj[i].name, obj[i].exc, obj[i].cpu );

	node = obj[i].node;

	node_exc[node] += obj[i].exc;
	node_cpu[node] += obj[i].cpu;
    }

    printf ( "\n" );

    for ( i = 0; i < nodes; i++ )
    {
	printf ( "Node %-7d %6d %7d\n", i, node_exc[i], node_cpu[i] );

	tot_exc += node_exc[i];
	tot_cpu += node_cpu[i];
    }

    printf ( "\n%-12s %6d %7d\n", "Totals", tot_exc, tot_cpu );
#endif
}

getin ( filename )

    char * filename;
{
    int n, fd;

    fd = open ( filename, O_RDONLY, 0 );

    while ( ( n = read ( fd, &cobj[cno], sizeof(cobj[0]) * BLOCKSIZE ) ) > 0 )
    {
	cno += ( n / sizeof(cobj[0]) );

	if ( cno > MAX_ITEMS )
	{
	    fprintf ( stderr, "exceeded %d items\n", MAX_ITEMS );
	    exit (1);
	}
    }

    close ( fd );

    fprintf ( stderr, "%d items\n", cno );

#ifdef PRINT
    for ( n = 0; n < cno; n++ )
    {
	if ( cobj[n].objno < nobjs )
	{
	    printf ( "%s %f %d %d\n",
	    obj[cobj[n].objno].name, cobj[n].vt, cobj[n].cpuf, cobj[n].cput );
	}
	else
	{
	    printf ( "?%d %f %d %d\n",
	    cobj[n].objno, cobj[n].vt, cobj[n].cpuf, cobj[n].cput );
	}
    }
#endif
}


#include <suntool/sunview.h>
#include <suntool/canvas.h>

#define BLACK	0
#define RED	1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

int color = RED;
static int vt0;
static int X1, Y1, X2, Y2;
static int charx, chary;
static int xorg, xend, xlen, xmax, xmin, xrange;
static int yorg, yend, ylen, ymax, ymin, yrange;
static double xscale, yscale;
int width=1152, height=900;
static int xsmin, xsmax, xpagediv, xpage, xpagesize, xnum_pages;
static int ysmin, ysmax, ypagediv, ypage, ypagesize, ynum_pages;
static int max_ydiff;
static int font_width = 8;
static int font_height = 14;
static int select_node;
static int select_obj;
static int flat, rollbacks, scaling_y;
static int low_vt[1024], high_vt[1024];
static int last_vt[MAX_OBJS];
static int zoomed;
static int annotate;
static int highlight;
static int search;
static short screen_x1[MAX_ITEMS];
static short screen_x2[MAX_ITEMS];
static short screen_y1[MAX_ITEMS];

#define ZSTKSIZE 20
static struct
{
    int xsmin, xsmax, ysmin, ysmax;
    int xpagediv, xnum_pages, xpage;
    int scaling_y;
}
    zstk[ZSTKSIZE];
static int zsp;

    Menu main_menu;

node_func ( m, mi )

    Menu m;
    Menu_item mi;
{
    caddr_t choice;
    int node;

    choice = (caddr_t) menu_get ( mi, MENU_VALUE );

    node = (int) choice - 4;

    switch ( choice )
    {
	case 1:
	    node = nodes;
	    break;
	case 2:
	    node = select_node + 1;
	    if ( node >= nodes )
		return ( 0 );
	    break;
	case 3:
	    if ( select_node == 0 || select_node == nodes )
		return ( 0 );
	    node = select_node - 1;
	    break;
    }

    if ( select_node == node )
	return ( 0 );

    select_node = node;
    select_obj = nobjs;		/* all objs */

    if ( select_node == nodes )
	menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
		MENU_INACTIVE, TRUE, 0 );
    else
	menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
		MENU_INACTIVE, FALSE, 0 );

    return ( 2 );
}

page_func ( m, mi )

    Menu m;
    Menu_item mi;
{
    caddr_t choice;

    choice = (caddr_t) menu_get ( mi, MENU_VALUE );

    switch ( choice )
    {
	case 1:
	    xpage++;
	    break;
	case 2:
	    xpage += 10;
	    break;
	case 3:
	    xpage--;
	    break;
	case 4:
	    xpage -= 10;
    }

    if ( xpage < 0 )
	xpage = 0;
    else
    if ( xpage >= xnum_pages )
	xpage = xnum_pages - 1;

    return ( 1 );
}

static Menu obj_menu[MAX_NODES];

Menu generate_obj ( mi, operation )

    Menu_item mi;
    Menu_generate operation;
{
    return ( obj_menu[select_node] );
}

obj_func ( m, mi )

    Menu m;
    Menu_item mi;
{
    int choice;
    int i, num;

    choice = (int) menu_get ( mi, MENU_VALUE );

    if ( choice == 1 )
	select_obj = nobjs;
    else
	select_obj = choice - 2;
/*
    for ( i = 0, num = 1; i < nobjs; i++ )
    {
	if ( obj[i].node == select_node )
	{
	    if ( ++num == choice )
	    {
		select_obj = i;
		break;
	    }
	}
    }
*/
    return ( 3 );
}

div_func ( m, mi )

    Menu m;
    Menu_item mi;
{
    caddr_t choice;

    choice = (caddr_t) menu_get ( mi, MENU_VALUE );

    switch ( choice )
    {
	case 1:
	    xpagediv = 1;
	    break;
	case 2:
	    xpagediv = 2;
	    break;
	case 3:
	    xpagediv = 5;
	    break;
	case 4:
	    xpagediv = 10;
	    break;
	case 5:
	    xpagediv = 20;
	    break;
	case 6:
	    xpagediv = 50;
	    break;
	case 7:
	    xpagediv = 100;
    }
    xpage = 0;

    zoomed = 0;
    zsp = 0;

    return ( 4 );
}

int stb89, stb88, ctls, pool, ants, mctls;

    Frame frame;
    Canvas canvas;
    Pixwin *pw;

    int dev, val;
    int dev_x, dev_y;

wait_for_button ()
{
    for ( ;; )
    {
        for ( dev = 0; dev == 0; )
            notify_dispatch ();

        if ( dev == MS_LEFT || dev == MS_RIGHT || dev == MS_MIDDLE )
            break;
    }
}

void my_event_proc ( canvas, event )

    Canvas canvas;
    Event *event;
{
    switch ( event_id(event) )
    {
	case MS_LEFT:
	case MS_RIGHT:
	case MS_MIDDLE:

	if ( event_is_down(event) )
	{
	    dev = event_id(event);

	    dev_x = event_x ( event );
	    dev_y = height - event_y ( event );
	}

	if ( dev == MS_RIGHT && val == 0 )
	    val = (int) menu_show ( main_menu, canvas, event, 0 );

	break;

	case LOC_MOVE:

	    dev_x = event_x ( event );
	    dev_y = height - event_y ( event );

	break;
    }
}

char string[5000];
char nodestr[500];

char red   [256] = { 0, 255, 0, 255, 0, 255, 0, 255 };
char green [256] = { 0, 0, 255, 255, 0, 0, 255, 255 };
char blue  [256] = { 0, 0, 0, 0, 255, 255, 255, 255 };

static char cmsname[CMS_NAMESIZE] = "fplot";

Rect screen_rect = { 0, 0, 1152, 900 };

Cursor cursor;

writemask ( planes )

    int planes;
{
    if ( planes == -1 )
	planes = 63;

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

recti ( x1, y1, x2, y2 )

    int x1, y1, x2, y2;
{
    line ( x1, y1, x1, y2 );
    line ( x1, y2, x2, y2 );
    line ( x2, y2, x2, y1 );
    line ( x2, y1, x1, y1 );
}

main ( argc, argv )

    int argc;
    char ** argv;
{
    register int i, j;
    Menu node_submenu;
    Menu page_submenu;
    Menu div_menu;
    int node;
    char * flowname = "";

    if ( argc == 2 )
    {
	flowname = argv[1];
    }

    read_flow_data ( flowname );

    /* create frame and canvas */

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
	WIN_HEIGHT, height,
	WIN_WIDTH, width,
	WIN_X, 0,
	WIN_Y, 0,
	0 );

    cursor = window_get ( canvas, WIN_CURSOR );

    for ( i = 8; i < 256; i++ )
	mapcolor ( i, 255, 255, 0 );

    /* get the canvas pixwin to draw into */

    pw = (Pixwin *) window_get ( canvas, WIN_PIXWIN );

    pw_setcmsname ( pw, cmsname );

    pw_putcolormap ( pw, 0, 64, red, green, blue );

    pw = canvas_pixwin ( canvas );

    pw_setcmsname ( pw, cmsname );

    pw_putcolormap ( pw, 0, 64, red, green, blue );
/*
    window_set ( canvas, CANVAS_RETAINED, TRUE, 0 );
*/
    window_set ( frame, WIN_SHOW, TRUE, 0 );

    select_node = nodes;	/* all */
    select_obj = nobjs;		/* all */

    for ( node = 0; node < nodes; node++ )
    {
	obj_menu[node] = menu_create ( MENU_STRINGS, "all", 0,
		MENU_NOTIFY_PROC, obj_func, 0 );

	for ( i = 0; i < nobjs; i++ )
	{
	    if ( obj[i].node != node )
		continue;

	    menu_set ( obj_menu[node], MENU_STRING_ITEM, obj[i].name, i+2, 0 );
	}

    }

    node_submenu = menu_create ( MENU_STRINGS, "all", "+1", "-1", 0, MENU_NOTIFY_PROC, node_func, 0 );

    for ( i = 0; i < nodes; i++ )
    {
	char * n;

	if ( i == 0 )
	    n = nodestr;

	sprintf ( n, "%d", i );

	menu_set ( node_submenu, MENU_STRING_ITEM, n, i+4, 0 );

	n += strlen ( n ) + 1;
    }

    page_submenu = menu_create ( MENU_STRINGS, "fwd 1", "fwd 10", "bwd 1", "bwd 10", 0, MENU_NOTIFY_PROC, page_func, 0 );

    div_menu = menu_create ( MENU_STRINGS, "1", "2", "5", "10", "20", "50", "100", 0, MENU_NOTIFY_PROC, div_func, 0 );

    main_menu = menu_create (
			MENU_PULLRIGHT_ITEM, "page", page_submenu,
			MENU_PULLRIGHT_ITEM, "node", node_submenu,
			MENU_GEN_PULLRIGHT_ITEM, "object", generate_obj,
			MENU_PULLRIGHT_ITEM, "page", div_menu,
			MENU_STRINGS, "flat", "rollback", "scale y", "annotate", "magnify", "highlight", "search", "identify", "zoom", "unzoom", "exit", 0, 0 );

    menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
	MENU_INACTIVE, TRUE, 0 );

    xorg = 50; xend = width  - 51; xlen = xend - xorg + 1;
    yorg = 50; yend = height - 51; ylen = yend - yorg + 1;

    xmin = ymin = 0x7fffffff;
    xmax = ymax = 0x80000000;

    vt0 = 0;

    for ( i = vt0; i < cno; i++ )
    {
	if ( xmin > cobj[i].cpuf ) xmin = cobj[i].cpuf;
	if ( xmax < cobj[i].cput ) xmax = cobj[i].cput;
	if ( ymin > cobj[i].vt   ) ymin = cobj[i].vt;
	if ( ymax < cobj[i].vt   ) ymax = cobj[i].vt;
    }

    xrange = xmax - xmin + 1;
    yrange = ymax - ymin + 1;

    printf ( "xmin %d xmax %d xrange %d ymin %d ymax %d yrange %d\n",
	xmin, xmax, xrange, ymin, ymax, yrange );

    xpagediv = ypagediv = 1;

    for ( i = vt0; i < cno; i++ )
    {
	register int objno = cobj[i].objno;

	register char * name = obj[objno].name;

	if ( strncmp ( name, "main", 4 ) == 0 )
	{
	    mctls = 1;
	    break;
	}

	if ( strncmp ( name, "RedDiv", 6 ) == 0 )
	{
	    stb89 = 1;
	    break;
	}

	if ( strncmp ( name, "distrib", 7 ) == 0 )
	{
	    stb88 = 1;
	    break;
	}

	if ( strcmp ( name, "red_div1" ) == 0 )
	{
	    ctls = 1;
	    break;
	}

	if ( strncmp ( name, "ball", 4 ) == 0 )
	{
	    pool = 1;
	    break;
	}

	if ( strncmp ( name, "ant", 3 ) == 0 )
	{
	    ants = 1;
	    break;
	}
    }

    for ( ;; )
    {
	dev = val = 0;

	plot_page ();

	do {
	    if ( val == 0 )
		wait_for_button ();

	    if ( val )
		handle_selection (); /* might zero val */
	}
	while ( val == 0 );
    }
}

plot_page ()
{
    register int i, j, k;
    int from, to, by;
/*
    mapcolor ( 3, 255, 255, 0 );
    pw_putcolormap ( pw, 0, 64, red, green, blue );
*/
plot_page_again:

    clear ();

    notify_dispatch ();

    color = CYAN;

    xpagesize = ( xrange + xpagediv - 1 ) / xpagediv;
    xnum_pages = ( xrange + xpagesize - 1 ) / xpagesize;

    ypagesize = ( yrange + ypagediv - 1 ) / ypagediv;
    ynum_pages = ( yrange + ypagesize - 1 ) / ypagesize;

    if ( ! zoomed )
    {
	xsmin = xpagesize * xpage + xmin;
	xsmax = xpagesize * ( xpage + 1 ) + xmin - 1;

	ysmin = ypagesize * ypage + ymin;
	ysmax = ypagesize * ( ypage + 1 ) + ymin - 1;
    }

    xscale = (double)( xsmax - xsmin ) / xlen;
    yscale = (double)( ysmax - ysmin ) / ylen;

    if ( flat || annotate )
    {
	for ( j = 0; j < width; j++ )
	{
	    low_vt[j] = ymax;
	    high_vt[j] = ymin;
	}

	for ( i = vt0; i < cno; i++ )
	{
	    register int x1, x2;

	    int objno = cobj[i].objno;

	    if ( obj[objno].node != select_node && select_node != nodes )
		continue;

	  if ( ! highlight )
	  {
	    if ( objno != select_obj && select_obj != nobjs )
		continue;
	  }
	    if ( cobj[i].cpuf < xsmin || cobj[i].cput > xsmax )
		continue;

	    if ( cobj[i].vt < ysmin || cobj[i].vt > ysmax )
		continue;

	    x1 = ( cobj[i].cpuf - xsmin ) / xscale + xorg;
	    x2 = ( cobj[i].cput - xsmin ) / xscale + xorg;

	    for ( j = x1; j <= x2; j++ )
	    {
		if ( low_vt[j] > cobj[i].vt )
		    low_vt[j] = cobj[i].vt;

		if ( high_vt[j] < cobj[i].vt )
		    high_vt[j] = cobj[i].vt;
	    }
	}
    }

    if ( flat )
    {
	if ( scaling_y )
	{
	    max_ydiff = 0;

	    for ( j = 0; j < width; j++ )
	    {
		register int ydiff = high_vt[j] - low_vt[j];

		if ( max_ydiff < ydiff )
		    max_ydiff = ydiff;
	    }

	    yscale = (double) max_ydiff / ylen;
	}
    }
    else
    if ( scaling_y )
    {
	ysmin = ymax; ysmax = ymin;

	for ( i = vt0; i < cno; i++ )
	{
	    int objno = cobj[i].objno;

	    if ( obj[objno].node != select_node && select_node != nodes )
		continue;

	  if ( ! highlight )
	  {
	    if ( objno != select_obj && select_obj != nobjs )
		continue;
	  }
	    if ( cobj[i].cpuf < xsmin || cobj[i].cput > xsmax )
		continue;

	    if ( ysmin > cobj[i].vt ) ysmin = cobj[i].vt;
	    if ( ysmax < cobj[i].vt ) ysmax = cobj[i].vt;
	}

	yscale = (double)( ysmax - ysmin ) / ylen;
    }

    line ( xorg, yend, xend, yend );	/* top */
    line ( xend, yend, xend, yorg );	/* right */
    line ( xend, yorg, xorg, yorg );	/* bottom */
    line ( xorg, yorg, xorg, yend );	/* left */

    center ( "Timewarp Progress Graph", xorg, yend, xend, height );

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

	    sprintf ( string, "%d", ysmax - ysmin );
	}

	xcenter ( "0", 0, xorg, yorg );	/* lower left side */

	center ( string, 0, yend - font_height, xorg, yend ); /* upper left */
    }
    else
    {
	vertical_center ( "Virtual Time", 0, yorg, xorg, yend );

	sprintf ( string, "%d", ysmin );
	xcenter ( string, 0, xorg, yorg );	/* lower left side */

	sprintf ( string, "%d", ysmax );
	center ( string, 0, yend - font_height, xorg, yend ); /* upper left */
    }

    if ( mctls )
    {
	if ( select_node == nodes )
	    sprintf ( string, "M C T L S     %d Nodes", nodes );
	else
	    sprintf ( string, "M C T L S   Node %d", select_node );
    }
    else
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
    if ( pool )
    {
	if ( select_node == nodes )
	    sprintf ( string, "P O O L   %d Nodes", nodes );
	else
	    sprintf ( string, "P O O L   Node %d", select_node );
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
    {
	if ( select_node == nodes )
	    sprintf ( string, "%d Nodes", nodes );
	else
	    sprintf ( string, "Node %d", select_node );
    }

    vertical_center ( string, xend, yorg, width, yend );	/* right side */

    sprintf ( string, "Measured CPU Seconds - Page %d of %d", xpage+1, xnum_pages );
    center ( string, xorg, 0, xend, yorg );	/* below */

    sprintf ( string, "%f", (float)(xsmin-xmin) * 62.5 / 1000000. );
    ycenter ( string, xorg, 0, yorg );	/* bottom left */

    sprintf ( string, "%f", (float)(xsmax-xmin) * 62.5 / 1000000. );
    rjust ( string, xend, 0, yorg );		/* bottom right */

    if ( select_obj < nobjs )
    {
	text ( xorg + 20, yend - font_height, obj[select_obj].name );
    }

    charx = chary = 0;

    from = vt0; to = cno; by = 1;

    if ( rollbacks )
    {
	for ( i = 0; i < nobjs; i++ )
	    last_vt[i] = ymax;

	from = cno - 1; to = vt0 - 1; by = -1;

	mapcolor ( 3, 0, 0, 255 );
	pw_putcolormap ( pw, 0, 64, red, green, blue );
    }

    k = 100;

    cursor_set ( cursor, CURSOR_SHOW_CURSOR, FALSE, 0 );
    window_set ( canvas, WIN_CURSOR, cursor, 0 );

    pw_lock ( pw, &screen_rect );

    for ( i = from; i != to; i += by )
    {
	int line_color = MAGENTA;
	int objno = cobj[i].objno;

	screen_x1[i] = screen_x2[i] = screen_y1[i] = 0;

	if ( obj[objno].node != select_node && select_node != nodes )
	    continue;

	if ( ! highlight )
	{
	    if ( objno != select_obj && select_obj != nobjs )
		continue;
	}

	if ( cobj[i].cpuf < xsmin || cobj[i].cput > xsmax )
	    continue;

	if ( cobj[i].vt < ysmin || cobj[i].vt > ysmax )
	    continue;

	if ( rollbacks )
	{
	    if ( cobj[i].vt <= last_vt[objno] )
	    {
		line_color = GREEN;
		last_vt[objno] = cobj[i].vt;
	    }
	    else
		line_color = RED;
	}
	else
	if ( mctls )
	{
	    if ( obj[objno].name[0] == 'i' ) line_color = YELLOW;
	    if ( obj[objno].name[0] == 'B' ) line_color = GREEN;
	    if ( obj[objno].name[0] == 'C' ) line_color = RED;
	    if ( obj[objno].name[0] == 'm' ) line_color = BLUE;
	}
	else
	if ( stb89 )
	{
	    if ( obj[objno].name[0] == 'G' )
		if ( obj[objno].name[1] == 'I' )
		    line_color = YELLOW;
		else
		    line_color = GREEN;
	    if ( obj[objno].name[0] == 'R' ) line_color = RED;
	    if ( obj[objno].name[0] == 'B' ) line_color = BLUE;
	}
	else
	if ( stb88 )
	{
	    if ( obj[objno].name[0] == 'd' ) line_color = YELLOW;
	    if ( obj[objno].name[0] == 'G' ) line_color = GREEN;
	    if ( obj[objno].name[0] == 'r' ) line_color = RED;
	    if ( obj[objno].name[0] == 'b' ) line_color = BLUE;
	}
	else
	if ( ctls )
	{
	    if ( obj[objno].name[0] == 'i' ) line_color = YELLOW;
	    if ( obj[objno].name[0] == 'G' ) line_color = GREEN;
	    if ( obj[objno].name[0] == 'r' ) line_color = RED;
	    if ( obj[objno].name[0] == 'b' ) line_color = BLUE;
	}
	else
	if ( pool )
	{
	    if ( obj[objno].name[0] == 's' ) line_color = RED;
	    if ( obj[objno].name[0] == 'c' ) line_color = BLUE;
	    if ( obj[objno].name[0] == 'b' ) line_color = GREEN;
	}
	else
	if ( ants )
	{
	    if ( obj[objno].name[0] == 'a' ) line_color = RED;
	    if ( obj[objno].name[0] == 'n' ) line_color = BLUE;
	    if ( obj[objno].name[0] == 'g' ) line_color = GREEN;
	}

	if ( highlight )
	{
	    if ( objno == select_obj )
		line_color = WHITE;
	}
	else
	if ( rollbacks )
	{
/*	    writemask ( line_color );*/
	}

	color = line_color;

	scale_line ( cobj[i].cpuf, (int) cobj[i].vt,
		     cobj[i].cput, (int) cobj[i].vt );

	screen_x1[i] = X1; screen_x2[i] = X2; screen_y1[i] = Y1;

	if ( annotate )
	{
	    if ( cobj[i].vt == low_vt[X1] )
	    if ( X1 > charx || Y1 > chary + font_height * 2 )
	    {
		chary = Y1 - font_height - 1;
		text ( X1, chary, obj[objno].name );
		charx = X1 + strlen ( obj[objno].name ) * font_width;
	    }
	}

	if ( rollbacks )
	{
/*	    writemask ( -1 );*/
	}

	if ( --k == 0 )
	{
	    pw_unlock ( pw );

	    notify_dispatch ();

	    pw_lock ( pw, &screen_rect );

	    if ( dev != 0 )
		break;

	    k = 100;
	}
    }

    pw_unlock ( pw );

    cursor_set ( cursor, CURSOR_SHOW_CURSOR, TRUE, 0 );
    window_set ( canvas, WIN_CURSOR, cursor, 0 );

    if ( search )
    {
	if ( dev == 0 )
	if ( select_node < nodes - 1 )
	{
	    int s;

	    for ( s = 0; s < 3; s++ )
	    {
		sleep ( 1 );

		notify_dispatch ();

		if ( dev != 0 )
		{
		    break;
		}
	    }
	    if ( dev == 0 )
	    {
		select_node++;
		goto plot_page_again;
	    }
	}

	search = 0;
    }
}

handle_selection ()
{
    switch ( val )
    {
	case 5:
	    flat = 1 - flat;
	    break;
	case 6:
	    rollbacks = 1 - rollbacks;
	    break;
	case 7:
	    scaling_y = 1 - scaling_y;
	    break;
	case 8:
	    annotate = 1 - annotate;
	    break;
	case 9:
	    magnify ();
	    val = 0;
	    break;
	case 10:
	    highlight = 1 - highlight;
	    if ( select_obj == nobjs )
		val = 0;
	    break;
	case 11:
	    search = 1;
	    if ( (select_node+1) < nodes )
		select_node++;
	    else
	    {
		select_node = 0;

		menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
			MENU_INACTIVE, FALSE, 0 );
	    }
	    select_obj = nobjs;
	    break;
	case 12:
	    identify ();
	    val = 0;
	    break;
	case 13:
	    if ( zsp < ZSTKSIZE )
	    {
		zoom_push ();
		zoom_area ();
	    }
	    else
		val = 0;
	    break;
	case 14:
	    if ( zsp > 0 )
		zoom_pop ();
	    else
		val = 0;
	    break;
	case 15:
	    exit (0);
    }
}

zoom_area ()
{
    int x1, x2, last_x1, last_x2;
    int y1, y2, last_y1, last_y2;
    int i, x, y;

    for ( i = 8; i < 32; i++ )
	mapcolor ( i, 255, 0, 0 );

    pw_putcolormap ( pw, 0, 64, red, green, blue );

    writemask ( 16 );

    last_x1 = last_y1 = 0;

    x1 = xorg; x2 = xend;
    y1 = yorg; y2 = yend;

    for ( dev = 0; dev == 0; )
    {
	notify_dispatch ();

	x = dev_x;
	y = dev_y;

	x -= 10; y += 10;

	if ( x >= xorg && x <= xend )
	    x1 = x;

	if ( y >= yorg && y <= yend )
	    y1 = y;

	if ( x1 == last_x1 && y1 == last_y1 ) continue;

	color = 0;
	line ( last_x1, last_y1, last_x1 + 10, last_y1 );
	line ( last_x1, last_y1, last_x1, last_y1 - 10 );
	color = 16;
	line ( x1, y1, x1 + 10, y1 );
	line ( x1, y1, x1, y1 - 10 );

	last_x1 = x1; last_y1 = y1;
    }

    color = 0;
    line ( last_x1, last_y1, last_x1 + 10, last_y1 );
    line ( last_x1, last_y1, last_x1, last_y1 - 10 );
    color = 16;

    last_x2 = x1; last_y2 = y1;

    for ( dev = 0; dev == 0; )
    {
	notify_dispatch ();

	x = dev_x;
	y = dev_y;

	if ( x >= xorg && x <= xend )
	    x2 = x;

	if ( y >= yorg && y <= yend )
	    y2 = y;

	if ( x2 == last_x2 && y2 == last_y2 ) continue;

	color = 0;
	recti ( x1, y1, last_x2, last_y2 );
	color = 16;
	recti ( x1, y1, x2, y2 );

	last_x2 = x2; last_y2 = y2;
    }

    writemask ( -1 );

    if ( x1 <= x2 )
    {
	xsmax = ( x2 - xorg ) * xscale + xsmin;
	xsmin = ( x1 - xorg ) * xscale + xsmin;
    }
    else
    {
	xsmax = ( x1 - xorg ) * xscale + xsmin;
	xsmin = ( x2 - xorg ) * xscale + xsmin;
    }

    if ( y1 <= y2 )
    {
	ysmax = ( y2 - yorg ) * yscale + ysmin;
	ysmin = ( y1 - yorg ) * yscale + ysmin;
    }
    else
    {
	ysmax = ( y1 - yorg ) * yscale + ysmin;
	ysmin = ( y2 - yorg ) * yscale + ysmin;
    }

    xpagediv = 1;
    xnum_pages = 1;
    xpage = 0;

    zoomed = 1;
    scaling_y = 0;
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

identify ( event )

    Event *event;
{
    int i;

    for ( i = 8; i < 16; i++ )
	mapcolor ( i, 255, 255, 0 );

    pw_putcolormap ( pw, 0, 64, red, green, blue );

    writemask ( 8 );

    for ( ;; )
    {
	wait_for_button ();

	if ( dev == MS_LEFT )
	    break;

	if ( dev == MS_RIGHT )
	{
	    identify_item ( dev_x, dev_y );
	}
    }

    clear ();

    writemask ( -1 );
}

identify_item ( x, y )

    int x, y;
{
    register int i, objno;
    char string[100];
    int tx, ty;
    double delta;

    tx = xorg + 20;
    ty = yend - font_height * 2;

    clear ();
    color = 8;
#ifdef LATER
    circi ( x, y, 3 );
#endif
    recti ( x-3, y-3, x+3, y+3);
/*
    sprintf ( string, "x = %d y = %d", x, y );
    text ( 100, 100, string );
*/
    for ( i = vt0; i < cno; i++ )
    {
	if ( x+1 >= screen_x1[i] && x-1 <= screen_x2[i]
	&&   y+1 >= screen_y1[i] && y-1 <= screen_y1[i] )
	{
	    objno = cobj[i].objno;

	    delta = (cobj[i].cput - cobj[i].cpuf) * 62.5 / 1000000.;

	    sprintf ( string, "Node %2d %-12s %9.2f %9.6f %9.6f %9.6f\n",
		obj[objno].node, obj[objno].name, cobj[i].vt,
		(cobj[i].cpuf - xsmin) * 62.5 / 1000000.,
		(cobj[i].cput - xsmin) * 62.5 / 1000000.,
		delta );

	    text ( tx, ty, string );
	    ty -= font_height;
	}
    }
}

line ( x1, y1, x2, y2 )

    int x1, y1, x2, y2;
{
    int Y1, Y2;

    Y1 = height - y1; Y2 = height - y2;

    pw_vector ( pw, x1, Y1, x2, Y2, PIX_SRC /*| PIX_DONTCLIP*/, color );
}

scale_line ( x1, y1, x2, y2 )

    int x1, y1, x2, y2;
{
    X1 = ( x1 - xsmin ) / xscale + xorg;
    X2 = ( x2 - xsmin ) / xscale + xorg;

    if ( flat )
    {
	register int x, y = ymax;

	for ( x = X1; x <= X2; x++ )
	{
	    if ( y > low_vt[x] )
		y = low_vt[x];
	}

	Y1 = ( y1 - y ) / yscale + yorg;
	Y2 = Y1;
    }
    else
    {
	Y1 = ( y1 - ysmin ) / yscale + yorg;
	Y2 = Y1;
    }

  if ( rollbacks )
    pw_vector ( pw, X1, height - Y1, X2, height - Y2, PIX_SRC|PIX_DST /*| PIX_DONTCLIP*/, color );
  else
    pw_vector ( pw, X1, height - Y1, X2, height - Y2, PIX_SRC /*| PIX_DONTCLIP*/, color );
}

text ( x, y, string )

    int x, y;
    char * string;
{
    y = height - y;

    pw_text ( pw, x, y, PIX_SRC | PIX_COLOR(color) /*| PIX_DONTCLIP*/, 0, string );
}

center ( string, x1, y1, x2, y2 )

    char * string;
    int x1, y1, x2, y2;
{
    int xlen, ylen, tlen;
    int twidth, theight;
    int x, y;

    xlen = x2 - x1 + 1;
    ylen = y2 - y1 + 1;

    tlen = strlen ( string );

    twidth  = font_width * tlen;
    theight = font_height;

    x = ( xlen - twidth  ) / 2 + x1;
    y = ( ylen - theight ) / 2 + y1;

    text ( x, y, string );
}

xcenter ( string, x1, x2, y )

    char * string;
    int x1, x2, y;
{
    int xlen, tlen;
    int twidth;
    int x;

    xlen = x2 - x1 + 1;

    tlen = strlen ( string );

    twidth = font_width * tlen;

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
    int ylen, tlen;
    int twidth, theight;
    int x, y;

    ylen = y2 - y1 + 1;

    tlen = strlen ( string );

    twidth = font_width * tlen;
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

	pw_text ( pw, x, y, PIX_SRC | PIX_COLOR(color) /*| PIX_DONTCLIP*/, 0, c );

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
