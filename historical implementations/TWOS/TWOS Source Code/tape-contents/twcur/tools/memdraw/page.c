#include "mplot.h"
#include <math.h>
#include <X11/Xlib.h>

extern Display *x_display;
extern Window x_window;
extern GC x_GC;

extern long int memtype_values[];
extern OBJ_NAME_ENTRY obj_names[];
extern int show_object, ncolor_settings;
static int from, to, by;

extern collect_sizes ();

page_start ()
{
	register int i, j;

	if (mode == BY_ADDRESS)
	{
		from = 0; to = cno; by = 1;
		xmax = addr_xmax;  xmin = addr_xmin;  xrange = addr_xrange;
		ymax = addr_ymax;  ymin = addr_ymin;  yrange = addr_yrange;
	}
	else
	{ /* mode == BY_SIZE */
		collect_sizes(0, cno);
		from = 0;
		to = nsize_entries;
		by = 1;
		xmax = size_xmax;  xmin = size_xmin;  xrange = size_xrange;
		ymax = size_ymax;  ymin = size_ymin;  yrange = size_yrange;
	}

	if ( ! zoomed )
	{
		xsmin = xmin;
		xsmax = xmax;

		ysmin = ymin;
		ysmax = ymax;
	}

	xscale = (double) ( xsmax - xsmin ) / xlen;
	yscale = (double) ( ysmax - ysmin ) / ylen;

	clear ();
	change_color ( CYAN );
	banner();

	XFlush ( x_display );

	redraw_page = 0;
}


select_color(type)
/* selects the appropriate color for showing memory of type "type".  Uses
   the color_settings set using the menus.  Newer settings take precedence. */
	int type;
{
	register int i;

	change_color(BLUE);
	for (i = 0;  i < ncolor_settings;  i++)
		if (type & memtype_values [color_settings[i].mem_type])
		/* If the memory is of the same type as this color setting
		   applies to, use this setting's color. */
			change_color( color_settings[i].color );
}


plot_page ()
{
	register int i, j, k;
	char string[80];

	if ( redraw_page )
		page_start();

	k = 100;

	for ( i = from; i != to; i += by )
	{
		if (mode == BY_ADDRESS)
		{ /* start plot by address */

			if ( show_object == -1 )
				/* select color based on memory type */
				select_color(mem_entry[i].type);
			else
				/* select color based on object */
				if ( mem_entry[i].objno ==
					obj_names[show_object].objno )
					select_color(mem_entry[i].type);
				else
					change_color (BLUE);

			if ( i < xsmin || i > xsmax )
				continue;

			scale_line ( i, log (1.0), i, log ( (double) (mem_entry[i].size/16) ) );
			/* store the coordinates for "identify" use later */
			screen_x1[i] = X1; screen_y1[i] = Y1;
			screen_x2[i] = X2; screen_y2[i] = Y2;

		} /* end of plot by address */

		else 
		{ /* start plot by size */

			change_color ( YELLOW );
			scale_rect ( 0, (double) i, sizes[i].how_many, (double) (i+1) );

			change_color ( BLACK );

			/* Annotate how many are in the group */
			sprintf(string, "%i", sizes[i].how_many);
			scale_text ( sizes[i].how_many, (double) i, string );

			/* Annotate what size(s) are in the group */
			if ( sizes[i].end_size )
				sprintf(string, "%i-%i", sizes[i].beg_size,
					sizes[i].end_size );
			else
				sprintf(string, "%i", sizes[i].beg_size);
			scale_text_left ( 0, (double) i, string );

		} /* end plot by size */

		if ( --k == 0 )
		/* if we've drawn 100 lines and are not yet done with the
		   page, return control to XView but call busy_mode() so
		   we can do another 100 soon. */
		{
			XFlush ( x_display );

			from = i + by;
			busy_mode();
			return;
		}
	}

	XFlush ( x_display );

	idle_mode();
}
