/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef XVIEW
#include <xview/xview.h>
#include <xview/canvas.h>
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#endif

static Menu obj_menu[MAX_NODES];

Menu generate_obj ( mi, operation )

	Menu_item mi;
	Menu_generate operation;
{
	return ( obj_menu[select_node] );
}

Menu main_menu;
Menu node_submenu;
Menu page_submenu;
Menu div_menu;

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
	select_obj = nobjs;         /* all objs */

	if ( select_node == nodes )
		menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
				MENU_INACTIVE, TRUE, 0 );
	else
		menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
				MENU_INACTIVE, FALSE, 0 );

	redraw_page = 1;
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
			xpage--;
			break;
		default:
			xpage = (int) choice - 3;
	}

	if ( xpage < 0 )
		xpage = 0;
	else
	if ( xpage >= xnum_pages )
		xpage = xnum_pages - 1;

	redraw_page = 1;
}


obj_func ( m, mi )

	Menu m;
	Menu_item mi;
{
	int choice;
	int i, num;

	choice = (int) menu_get  ( mi, MENU_VALUE );

	if ( choice == 1 )
		select_obj = nobjs;
	else
		select_obj = choice - 2;

	redraw_page = 1;
}

page_div_func ( m, mi )

	Menu m;
	Menu_item mi;
{
	caddr_t choice;

	choice = (caddr_t) menu_get ( mi, MENU_VALUE );
	xpagediv = (int) choice;

	xpage = 0; /* start at begining */
	page_scan = 1;
	null_zoom ();

	if ( xpagediv == 1 )
	{
		menu_set ( menu_find ( main_menu, MENU_STRING, "page", 0 ),
			MENU_INACTIVE, TRUE, 0 );
		menu_set ( menu_find ( main_menu, MENU_STRING, "page_scan", 0 ),
			MENU_INACTIVE, TRUE, 0 );
	}
	else
	{
		menu_set ( menu_find ( main_menu, MENU_STRING, "page", 0 ),
			MENU_INACTIVE, FALSE, 0 );
		menu_set ( menu_find ( main_menu, MENU_STRING, "page_scan", 0 ),
			MENU_INACTIVE, FALSE, 0 );
	}

	redraw_page = 1;
}

flat_notify()
{
	flat = 1 - flat;
	redraw_page = 1;
}

antimessages_notify()
{
	antimessages = 1 - antimessages;
	if ( antimessages )
		committed = rollbacks = 0;
	redraw_page = 1;
}

committed_notify()
{
	committed = 1 - committed;
	if ( committed )
		antimessages = rollbacks = 0;
	redraw_page = 1;
}

rollbacks_notify()
{
	rollbacks = 1 - rollbacks;
	if ( rollbacks )
		antimessages = committed = 0;
	redraw_page = 1;
}

scaling_notify()
{
	scaling_y = 1 - scaling_y;
	redraw_page = 1;
}

annotate_notify()
{
	annotate = 1 - annotate;
	redraw_page = 1;
}

magnify (); /* is its own notify */

highlight_notify()
{
	highlight = 1 - highlight;
	redraw_page = 1;
	if ( select_obj == nobjs )
		redraw_page = 0;
}

search_notify()
{
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
	redraw_page = 1;
}

page_scan_notify()
{
	page_scan = 1;
	redraw_page = 1;
}

identify_notify()
{
	identify_on = 1;
}

trace_notify()
{
	listx = xorg + 20;
	listy = yend - font_height * 2;
	list[il][nlist[il]++] = identi;
	redraw_page = 1;
}

extern zoom_start();

unzoom_menu_on()
{
	menu_set ( menu_find ( main_menu, MENU_STRING, "unzoom", 0 ),
		MENU_INACTIVE, FALSE, 0 );
}

unzoom_notify()
{
	if ( unzoom_ok () )
	{
		zoom_pop ();
		redraw_page = 1;
	}

	if ( !unzoom_ok () )
	{
		menu_set ( menu_find ( main_menu, MENU_STRING, "unzoom", 0 ),
			MENU_INACTIVE, TRUE, 0 );
	}
}

from_to_notify()
{
	from_to = 1 - from_to;
	redraw_page = 1;
}

refresh_notify()
{
	redraw_page = 1;
}

char * num_list[] = {
"0","1","2","3","4","5","6","7","8","9",
"10","11","12","13","14","15","16","17","18","19",
"20","21","22","23","24","25","26","27","28","29",
"30","31","32","33","34","35","36","37","38","39",
"40","41","42","43","44","45","46","47","48","49",
"50","51","52","53","54","55","56","57","58","59",
"60","61","62","63","64","65","66","67","68","69",
"70","71","72","73","74","75","76","77","78","79",
"80","81","82","83","84","85","86","87","88","89",
"90","91","92","93","94","95","96","97","98","99",
"100","101","102","103","104","105","106","107","108","109",
"110","111","112","113","114","115","116","117","118","119",
"120","121","122","123","124","125","126","127","128","129"
};

#define MAX_NODE_MENU 130
#define MAX_PAGE_MENU 100

init_menus ()
{
	int node;

	int i;

	if ( nodes > MAX_NODE_MENU )
	{
		printf ( "Too many nodes for node menu\n" );
		exit (1);
	}

	i = nodes / 10; if ( i < 1) i = 1;

	node_submenu = menu_create ( MENU_STRINGS, "all", "+1", "-1", 0,
		MENU_NCOLS, i,
		MENU_NOTIFY_PROC, node_func, 0 );

	for ( i = 0; i < nodes; i++ )
	{   
		menu_set ( node_submenu, MENU_STRING_ITEM, num_list[i], i+4, 0 );
	}

	page_submenu = menu_create (
		MENU_STRINGS, "+1", "-1", 0,
		MENU_NCOLS, 8,
		MENU_NOTIFY_PROC, page_func, 0 );

	for ( i = 1; i <= MAX_PAGE_MENU; i++ )
	{   
		menu_set ( page_submenu, MENU_STRING_ITEM, num_list[i], i + 2, 0 );
	}

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

	div_menu = menu_create ( MENU_NCOLS, 8,
		MENU_NOTIFY_PROC, page_div_func, 0 );

	for ( i = 1; i <= MAX_PAGE_MENU; i++ )
	{   
		menu_set ( div_menu, MENU_STRING_ITEM, num_list[i], i, 0 );
	}

	main_menu = menu_create (
		MENU_NCOLS, 2,
		MENU_ITEM,
			MENU_STRING, "antimessages",
			MENU_NOTIFY_PROC, antimessages_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "committed",
			MENU_NOTIFY_PROC, committed_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "rollback",
			MENU_NOTIFY_PROC, rollbacks_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "from/to",
			MENU_NOTIFY_PROC, from_to_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "highlight",
			MENU_NOTIFY_PROC, highlight_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "zoom",
			MENU_NOTIFY_PROC, zoom_start,
			NULL,
		MENU_ITEM,
			MENU_STRING, "unzoom",
			MENU_NOTIFY_PROC, unzoom_notify,
			MENU_INACTIVE, TRUE,
			NULL,
		MENU_ITEM,
			MENU_STRING, "scale y",
			MENU_NOTIFY_PROC, scaling_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "annotate",
			MENU_NOTIFY_PROC, annotate_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "flat",
			MENU_NOTIFY_PROC, flat_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "magnify",
			MENU_NOTIFY_PROC, magnify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "identify",
			MENU_NOTIFY_PROC, identify_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "trace",
			MENU_NOTIFY_PROC, trace_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "refresh",
			MENU_NOTIFY_PROC, refresh_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "search",
			MENU_NOTIFY_PROC, search_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "page_scan",
			MENU_NOTIFY_PROC, page_scan_notify,
			MENU_INACTIVE, TRUE,
			NULL,
		MENU_PULLRIGHT_ITEM, "page", page_submenu,
		MENU_PULLRIGHT_ITEM, "node", node_submenu,
		MENU_GEN_PULLRIGHT_ITEM, "object", generate_obj,
		MENU_PULLRIGHT_ITEM, "pages", div_menu,
		MENU_ITEM,
			MENU_STRING, "exit",
			MENU_NOTIFY_PROC, exit,
			NULL,
		NULL
	);

	menu_set ( menu_find ( main_menu, MENU_STRING, "object", 0 ),
		MENU_INACTIVE, TRUE, 0 );
	menu_set ( menu_find ( main_menu, MENU_STRING, "page", 0 ),
		MENU_INACTIVE, TRUE, 0 );
}
