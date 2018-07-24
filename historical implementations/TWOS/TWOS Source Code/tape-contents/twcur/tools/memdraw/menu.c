#include "mplot.h"

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>

extern Frame see_colors_frame, text_frame, object_frame, paging_frame;
extern GC x_GC; 
extern Window x_window;
extern Display * x_display;
Menu main_menu;
Menu mode_submenu, zoom_submenu;
Menu cluster_submenu;
Menu colors_submenu, color_submenu, object_submenu, memtype_submenu;
Menu hide_submenu;

/* This mask controls what types of memory are shown in the histogram.  Only
   memory whose type AND's (&) with show_mask will be included in the 
   histogram. */
long int show_mask = ALL_MEM;
extern int nobj_names;
extern OBJ_NAME_ENTRY obj_names[];

int obj_cycle = 0, show_object = -1;

char * color_list[] = {
 "black",
 "red",
 "green", 
 "yellow",
 "blue",
 "magenta",
 "dark green",
 "white" 
};

#define NMEM_TYPES 29
/* These are the names of the different types of memory.
   **The order is important for the settings at the bottom of color_settings.c**
 */
char * memtype_list[] = {
"free ",
"lost ",
"pool ",
"object ",
"obj ctrl blk ",
"current state ",
"state queue ",
"state address ",
"state dyn mem ",
"state sent ",
"state error ",
"stack memory ",
"message ",
"input msg q ",
"output msg q ",
"message vector ",
"now emsgs ",
"positive msg ",
"negative msg ",
"system msg ",
"queue header ",
"tw overhead ",
"cache memory ",
"home list ",
"in transit ",
"msg src:  on node ",
"msg src:  off node ",
"file & type area ",
"all "
};

/* These are the memory type values, as in the "type" field of the
   memory input file */
long int memtype_values[] = {
FREE_MEM,
LOST_MEM,
POOL_MEM,
OBJECT_MEM,
OCB_MEM,
STATE_CUR,
STATE_Q,
STATE_ADD,
STATE_DYM,
STATE_SENT,
STATE_ERR,
STACK_MEM,
MESSAGE_MEM,
INPUT_MSG,
OUTPUT_MSG,
MESS_VEC,
NOW_EMSGS,
POS_MSG,
NEG_MSG,
SYS_MSG,
QUEUE_HDR,
TW_OVER,
CACHE_MEM,
HOME_LIST,
IN_TRANS,
ONNODE,
OFFNODE,
FILETYPE_AREA,
ALL_MEM
};


object_notify ()
{
	xv_set ( object_frame, XV_SHOW, TRUE, NULL );
}

object_func( item, string, client_data, op )
	Panel_item item;
	char *string;
	caddr_t client_data;
	Panel_list_op op;
{
	if ( op == 1 )
	{
		if ( (int) client_data == -2 )
		{
			obj_cycle = 1;
			if ( show_object == -1 )
				show_object = 0;
		}	
		else
		{
			obj_cycle = 0;
			show_object = (int) client_data;
		}
		if ( show_object == -1 )
			xv_set ( object_frame, XV_SHOW, FALSE, NULL );
		redraw_page = 1;
	}
}

extern settings_win_refresh();

see_colors_notify ()
{
	xv_set ( see_colors_frame, XV_SHOW, TRUE, NULL );
	settings_win_refresh();
}


extern clear_color_settings ();

extern message_color_settings ();

extern state_color_settings ();

extern unused_color_settings ();

extern communications_color_settings ();


memtype_func( m, mi )
	Menu m;
	Menu_item mi;
{
	set_color_type = (int) menu_get ( mi, MENU_VALUE );
}


extern push_color_setting();

color_func( m, mi )
	Menu m;
	Menu_item mi;
{
	push_color_setting ( set_color_type, 
			(int) menu_get ( mi, MENU_VALUE ) );
}

hide_func ( m, mi )

	Menu m;
	Menu_item mi;
{
	long int mem_type = menu_get ( mi, MENU_VALUE );
	register int i;

	switch ( mem_type )
	{
		case -2:
		show_mask = NO_MEM; /* Hide all memory */
		for ( i = (int) xv_get(hide_submenu, MENU_NITEMS);
			  i > 0; i--)
			xv_set( xv_get ( hide_submenu, MENU_NTH_ITEM, i),
				MENU_SELECTED, FALSE,
				NULL); 
		break;

		case -1: /* Hide no memory */
		show_mask = ALL_MEM;
		for ( i = (int) xv_get(hide_submenu, MENU_NITEMS);
			  i > 0; i--)
			xv_set( xv_get ( hide_submenu, MENU_NTH_ITEM, i),
				MENU_SELECTED, TRUE,
				NULL);
		break;

		default:
		show_mask ^= mem_type;
	}

	redraw_page = 1;
}

cluster_func ( m, mi )

	Menu m;
	Menu_item mi;
{
	grouping_threshold = (int) menu_get ( mi, MENU_VALUE );

	redraw_page = 1;
}


act_by_addr_menus ( by_address )
	int by_address;
{
	menu_set ( menu_find (main_menu, MENU_STRING, "colors", NULL),
		MENU_INACTIVE, !by_address, NULL);
	menu_set ( menu_find (main_menu, MENU_STRING, "identify", NULL),
		MENU_INACTIVE, !by_address, NULL);
	menu_set ( menu_find (zoom_submenu, MENU_STRING, "by page...", NULL),
		MENU_INACTIVE, !by_address, NULL);
	menu_set ( menu_find (main_menu, MENU_STRING, "cluster", NULL),
		MENU_INACTIVE, by_address, NULL);
	menu_set ( menu_find (main_menu, MENU_STRING, "hide", NULL),
		MENU_INACTIVE, by_address, NULL);
	if ( by_address )
		menu_set ( menu_find (mode_submenu, MENU_STRING, "address", NULL),
			MENU_SELECTED, TRUE,
			NULL);
	else
		menu_set ( menu_find (mode_submenu, MENU_STRING, "block size", NULL),
			MENU_SELECTED, TRUE,
			NULL);
}

by_size_notify()
{
	if (mode != BY_SIZE)
	{
		mode = BY_SIZE;

		unzoom_notify ();
		redraw_page = 1;

		act_by_addr_menus (FALSE);
	}
}

by_address_notify()
{
	if (mode != BY_ADDRESS)
	{
		mode = BY_ADDRESS;

		unzoom_notify ();
		redraw_page = 1;

		act_by_addr_menus (TRUE);
	}
}

magnify (); /* is its own notify */


identify_notify()
{
	xv_set ( text_frame,
		XV_SHOW, TRUE,
		NULL);
	identify_on = 1;
}


extern zoom_start();

set_zoom_menus ( unzoomable )
	int unzoomable;
{
	menu_set ( menu_find ( zoom_submenu, MENU_STRING, "unzoom", NULL),
		MENU_INACTIVE, !unzoomable, NULL);
	menu_set ( menu_find ( zoom_submenu, MENU_STRING, "zoom out", NULL),
		MENU_INACTIVE, !unzoomable, NULL);
}

zoom_out_notify()
{
	zoom_pop ();
	redraw_page = 1;

	if ( !unzoom_ok () )
		{
		set_zoom_menus( FALSE );
		zoomed = 0;
		}
}

extern set_paging_things();

zoom_page_notify()
{
	unzoom_notify ();
	xv_set ( paging_frame, XV_SHOW, TRUE, NULL );

	zoom_to_page();
}

unzoom_notify()
{
	null_zoom ();
	xv_set ( paging_frame, XV_SHOW, FALSE, NULL );
	redraw_page = 1;
	set_zoom_menus( FALSE );
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


extern exit ();

init_menus ()
{
	register int i;

	mode_submenu = (Menu) xv_create (NULL, MENU_CHOICE_MENU,
		MENU_ITEM,
			MENU_STRING, "block size",
			MENU_NOTIFY_PROC, by_size_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "address",
			MENU_NOTIFY_PROC, by_address_notify,
			NULL,
		NULL);


	cluster_submenu =  (Menu) xv_create (NULL, MENU_CHOICE_MENU,
		MENU_ITEM, 
			MENU_STRING, "none",
			MENU_VALUE, 0,
			MENU_SELECTED, TRUE,
			NULL,
		MENU_NCOLS, 8,
		MENU_NOTIFY_PROC, cluster_func,
		NULL);
	for ( i = 20; i <= MAX_NHIST_BARS; i++ )
		menu_set ( cluster_submenu, MENU_STRING_ITEM, num_list[i], i, NULL);


	hide_submenu =  (Menu) xv_create (NULL, MENU_TOGGLE_MENU,
		MENU_NCOLS, 3,
		MENU_NOTIFY_PROC, hide_func,
		MENU_STRING_ITEM, "all", -2,
		MENU_STRING_ITEM, "none", -1,
		NULL );
	for ( i = 0; i < NMEM_TYPES; i++)
		menu_set ( hide_submenu, 
			MENU_STRING_ITEM, memtype_list[i], memtype_values[i], 
			NULL);
	for ( i = (int) xv_get(hide_submenu, MENU_NITEMS);
		  i > 0; i--)
		xv_set( xv_get ( hide_submenu, MENU_NTH_ITEM, i),
			MENU_SELECTED, TRUE,
			NULL); 


	color_submenu = (Menu) xv_create (NULL, MENU_CHOICE_MENU,
		MENU_NOTIFY_PROC, color_func,
		NULL);
	for ( i = 0;  i < 8;  i++ )
		menu_set ( color_submenu, 
			MENU_STRING_ITEM, color_list[i], i, 
			NULL);


	memtype_submenu = (Menu) xv_create (NULL, MENU_CHOICE_MENU,
		MENU_NCOLS, 3,
		MENU_NOTIFY_PROC, memtype_func,
		NULL);
	for ( i = 0; i < NMEM_TYPES; i++)
		menu_set ( memtype_submenu, 
			MENU_STRING_ITEM, memtype_list[i], i, 
			NULL);


	colors_submenu = menu_create (
		MENU_ITEM,
			MENU_STRING, "clear colors",
			MENU_NOTIFY_PROC, clear_color_settings,
			NULL,
		MENU_ITEM,
			MENU_STRING, "message colors",
			MENU_NOTIFY_PROC, message_color_settings,
			NULL,
		MENU_ITEM,
			MENU_STRING, "state colors",
			MENU_NOTIFY_PROC, state_color_settings,
			NULL,
		MENU_ITEM,
			MENU_STRING, "unused colors",
			MENU_NOTIFY_PROC, unused_color_settings,
			NULL,
		MENU_ITEM,
			MENU_STRING, "communications colors",
			MENU_NOTIFY_PROC, communications_color_settings,
			NULL,
		MENU_PULLRIGHT_ITEM, "set color of", memtype_submenu,
		MENU_PULLRIGHT_ITEM, "to color", color_submenu,
		MENU_ITEM,
			MENU_STRING, "see color settings...",
			MENU_NOTIFY_PROC, see_colors_notify,
			NULL,
		NULL );


	zoom_submenu = menu_create (
		MENU_ITEM,
			MENU_STRING, "zoom in",
			MENU_NOTIFY_PROC, zoom_start,
			NULL,
		MENU_ITEM,
			MENU_STRING, "zoom out",
			MENU_NOTIFY_PROC, zoom_out_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "by page...",
			MENU_NOTIFY_PROC, zoom_page_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "unzoom",
			MENU_NOTIFY_PROC, unzoom_notify,
			MENU_INACTIVE, TRUE,
			NULL,
		NULL );


	main_menu = menu_create (
		MENU_NCOLS, 2,
		MENU_PULLRIGHT_ITEM, "view by", mode_submenu,
		MENU_PULLRIGHT_ITEM, "zoom", zoom_submenu,
		MENU_PULLRIGHT_ITEM, "colors", colors_submenu,
		MENU_ITEM,
			MENU_STRING, "identify",
			MENU_NOTIFY_PROC, identify_notify,
			NULL,
		MENU_PULLRIGHT_ITEM, "cluster", cluster_submenu,
		MENU_PULLRIGHT_ITEM, "hide", hide_submenu,
		MENU_ITEM,
			MENU_STRING, "objects...",
			MENU_NOTIFY_PROC, object_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "magnify",
			MENU_NOTIFY_PROC, magnify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "refresh",
			MENU_NOTIFY_PROC, refresh_notify,
			NULL,
		MENU_ITEM,
			MENU_STRING, "exit",
			MENU_NOTIFY_PROC, exit,
			NULL,
		NULL );

	act_by_addr_menus (FALSE);
}
