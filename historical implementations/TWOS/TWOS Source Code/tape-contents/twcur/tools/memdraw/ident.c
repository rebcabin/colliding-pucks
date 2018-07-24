#include "mplot.h"
#define NULL 0
#define STR_LEN 512

#include <X11/Xlib.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>

extern Display * x_display;
extern Window x_window;
extern GC x_GC;
extern GC mask_GC;
extern int x_screen;
extern char * memtype_list[];
extern long int memtype_values[];
extern Textsw textsw;

extern OBJ_NAME_ENTRY obj_names[];
extern int nobj_names;


display_object ( i, dist )
	/* puts out information about mem_entry[i] */
	int i;
	double dist;
{
	char string[STR_LEN], typedesc[STR_LEN];
	register int j;

	/* Put together the text */
	typedesc[0] = '('; typedesc[1] = NULL;
	for ( j = 0; j <= NMEM_CATEGORIES; j++ )
		if ( mem_entry[i].type & memtype_values[j] )
			strcat( typedesc, memtype_list[j] );
	strcat ( typedesc, ")" );

	if ( mem_entry[i].objno != -1 )
	{
		for ( j = 0; j < nobj_names; j++ )
			if ( mem_entry[i].objno == obj_names[j].objno)
				break;
		sprintf ( string, "%x.  memory of type(s) %s, at address %x, of size %i belongs to object %s\n", mem_entry[i].address + 32, typedesc, mem_entry[i].address, mem_entry[i].size, obj_names[j].name );
	}
	else
		sprintf ( string, "%x.  memory of type(s) %s, at address %x is of size %i\n", mem_entry[i].address + 32, typedesc, mem_entry[i].address, mem_entry[i].size, mem_entry[i].type );


	/* Write it to the "memdraw info" window */
	textsw_insert ( textsw, string, strlen(string) );
}


identify_item ( x, y )
	/* given screeen coordinates of memory bar to identify, puts out text*/

	int x, y;
{
	register int i;
	register double top, bot, dist;
	double sqrt (), fabs ();

	/* draw the little square */
	mask_clear ();
	mask_yellow ();

	mask_recti ( x-3, y-3, x+3, y+3);

	/* erase old text in the "memdraw info" window */
	textsw_reset ( textsw, 0, 0 );

	/* Find the item(s) that were pointed to & display them. */
	for ( i = xsmin; i <= xsmax; i++ )
		if ( screen_y2[i] > (y-1) && screen_y1[i] < (y+1) &&
			screen_x1[i] > (x-2) && screen_x1[i] < (x+2) )
			display_object ( i, dist );
}

mask_line ( x1, y1, x2, y2 )
	int x1, y1, x2, y2;
{
	int Y1, Y2;
 
	Y1 = height - y1; Y2 = height - y2;
	XDrawLine ( x_display, x_window, mask_GC, x1, Y1, x2, Y2);
}

line ( x1, y1, x2, y2 )
/* Inputs are screen coordinates -d.p. */

	int x1, y1, x2, y2;
{
	int Y1, Y2;
 
	Y1 = height - y1; Y2 = height - y2;
 
	XDrawLine ( x_display, x_window, x_GC, x1, Y1, x2, Y2);
}


scale_line ( x1, y1, x2, y2 )
/* Inputs are "data" coordinates -d.p. */

	int x1;
	float y1;
	int x2;
	float y2;
{

	X1 = ( x1 - xsmin ) / xscale + xorg;
	X2 = ( x2 - xsmin ) / xscale + xorg;

	Y1 = ( y1 - ysmin ) / yscale + yorg;
	Y2 = ( y2 - ysmin ) / yscale + yorg;
	if ( Y1 < yorg )
		Y1 = yorg;

	if ( Y2 > yorg )
		XDrawLine ( x_display, x_window, x_GC, X1, height - Y1, X2, height - Y2);

}

scale_rect ( x1, y1, x2, y2 )

	int x1;
	float y1;
	int x2;
	float y2;
{

	X1 = ( x1 - xsmin ) / xscale + xorg;
	X2 = ( x2 - xsmin ) / xscale + xorg;

	Y1 = ( y1 - ysmin ) / yscale + yorg;
	Y2 = ( y2 - ysmin ) / yscale + yorg;

	/* chop off ends if we're zoomed and they are off the graph */
	if ( X1 < xorg )
		X1 = xorg;
	if ( X2 > xend )
		X2 = xend;

	/* don't draw at all if we're above or below the top of the graph */
	if ( Y2 <= yend + 3 && Y1 >= yorg)
	{
		if ( Y2 - Y1 <= 20 )
			XFillRectangle ( x_display, x_window, x_GC,
				X1, height - Y2, MAX( 0, X2 - X1 ), Y2 - Y1 );
		else
			XFillRectangle ( x_display, x_window, x_GC,
				X1, height - Y1 - 20, MAX( 0, X2 - X1 ), 20 );
	}
}
