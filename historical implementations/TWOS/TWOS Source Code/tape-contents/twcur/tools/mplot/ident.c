/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef X11
#include <X11/Xlib.h>

extern Display * x_display;
extern Window x_window;
extern GC x_GC;
extern GC mask_GC;
extern int x_screen;
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <suntool/canvas.h>

extern Pixwin *pw;
#endif

char porm[] = { '+', '-', '!', '@' };

struct
{
	int mtype;
	char * mtypes;
}
	mtype_str[] =
			{
				{ 6, "EMSG" },
				{ 9, "GVTS" },
				{ 18, "STAT" },
				{ 19, "SACK" },
				{ 20, "SNCK" },
				{ 21, "SDNE" },
				{ 33, "HASK" },
				{ 34, "HANS" },
				{ 0, "?"    }
			};

char * mtyper ( mtype )

	int mtype;
{
	int i;

	for ( i = 0; mtype_str[i].mtype != 0; i++ )
	{
		if ( mtype_str[i].mtype == mtype )
			break;
	}

	return ( mtype_str[i].mtypes );
}

#define STR_LEN 256

display_object ( i, dist )

	int i;
	double dist;
{
	char string[STR_LEN];
	int j;

	double delta = (cobj[i].cputimr - cobj[i].cputims) * 62.5 / 1000000.;

	int snder = cobj[i].snder;
	int rcver = cobj[i].rcver;

	sprintf ( string, "from Node %2d %9.6f %9.6f %9.6f %-12s %4.2f to Node %2d %9.6f %-12s %4.2f %9.6f %-4s%c %4d",
		obj[snder].node, (cobj[i].cputims - xmin) * 62.5 / 1000000.,
		(cobj[i].hgtimef - xmin) * 62.5 / 1000000.,
		(cobj[i].hgtimet - xmin) * 62.5 / 1000000.,
		obj[snder].name, cobj[i].sndtim,
		obj[rcver].node, (cobj[i].cputimr - xmin) * 62.5 / 1000000.,
		obj[rcver].name, cobj[i].rcvtim,
		delta,
		mtyper(cobj[i].mtype), porm[cobj[i].flags&3],
		cobj[i].id_num );

#ifdef SUNVIEW
	writemask ( 8 );
	if ( listy < yorg )
	{
		listy = yend - font_height * 2;

		clear ();
	}

	text ( listx, listy, string );
	listy -= font_height;

	writemask ( -1 );
#endif
#ifdef X11
	if ( listy < yorg )
	{
		listy = yend - font_height * 2;

		mask_clear ();
	}

	mask_text ( listx, listy, string );
	listy -= font_height;
#endif
}

identify_item ( x, y )

	int x, y;
{
	register int i;

	register double top, bot, dist;

	int lf_rt_1, lf_rt_2;

	double sqrt (), fabs ();


	listx = xorg + 20;
	listy = yend - font_height * 2;

#ifdef X11
	mask_clear ();
	mask_yellow ();

	mask_recti ( x-3, y-3, x+3, y+3);
#endif
#ifdef SUNVIEW
	writemask ( 8 );

	clear ();

	color = 8;

	recti ( x-3, y-3, x+3, y+3);
#endif

	for ( i = vt0; i < cno; i++ )
	{
		lf_rt_1 = screen_x1[i];
		lf_rt_2 = screen_x2[i];

		if ( screen_y2[i] < (y-1) || screen_y1[i] > (y+1) )
			continue;

		if ( lf_rt_1 < lf_rt_2 )
		{
			if ( lf_rt_2 < (x-1) || lf_rt_1 > (x+1) )
				continue;
		}
		else
		{
			if ( lf_rt_1 < (x-1) || lf_rt_2 > (x+1) )
				continue;
		}

		top = (lf_rt_2 - lf_rt_1) * (y - screen_y1[i])
			+ (screen_y1[i] - screen_y2[i]) * (x - lf_rt_1);

		bot = sqrt ( ((float)
			(lf_rt_2 - lf_rt_1) * (lf_rt_2 - lf_rt_1)
			+
			(screen_y2[i] - screen_y1[i]) * (screen_y2[i] - screen_y1[i])
			));

		dist = fabs ( top / bot );

		if ( dist <= 3. )
		{
			display_object ( i, dist );

			identi = i;
		}
	}
}

#ifdef X11
mask_line ( x1, y1, x2, y2 )
	int x1, y1, x2, y2;
{
	int Y1, Y2;
 
	Y1 = height - y1; Y2 = height - y2;
	XDrawLine ( x_display, x_window, mask_GC, x1, Y1, x2, Y2);
}
#endif

line ( x1, y1, x2, y2 )

	int x1, y1, x2, y2;
{
	int Y1, Y2;
 
	Y1 = height - y1; Y2 = height - y2;
 
#ifdef X11
	XDrawLine ( x_display, x_window, x_GC, x1, Y1, x2, Y2);
#endif
#ifdef SUNVIEW
	pw_vector ( pw, x1, Y1, x2, Y2, PIX_SRC /*| PIX_DONTCLIP*/, color );
#endif
}


scale_line ( x1, y1, x2, y2 )

	int x1;
	float y1;
	int x2;
	float y2;
{
	extern int rollbacks;

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
		Y2 = ( y2 - y ) / yscale + yorg;
	}
	else
	{
		Y1 = ( y1 - ysmin ) / yscale + yorg;
		Y2 = ( y2 - ysmin ) / yscale + yorg;
	}
#ifdef SUNVIEW
  if ( rollbacks )
	pw_vector ( pw, X1, height - Y1, X2, height - Y2, PIX_SRC|PIX_DST, color );
  else
	pw_vector ( pw, X1, height - Y1, X2, height - Y2, PIX_SRC, color );
#endif
#ifdef X11
  if ( rollbacks )
	/* change GC to different mode */
	XDrawLine ( x_display, x_window, x_GC, X1, height - Y1, X2, height - Y2);
  else
	XDrawLine ( x_display, x_window, x_GC, X1, height - Y1, X2, height - Y2);
#endif
}
