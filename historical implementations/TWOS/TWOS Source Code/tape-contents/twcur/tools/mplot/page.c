/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include "mplot.h"

#ifdef X11
#include <X11/Xlib.h>

extern Display *x_display;
#endif

static int from, to, by;


page_start ()
{
	register int i, j;

	xpagesize = ( xrange + xpagediv - 1 ) / xpagediv;
	xnum_pages = ( xrange + xpagesize - 1 ) / xpagesize;

	ypagesize = yrange / ypagediv;
	ynum_pages = ypagediv;

	if ( ! zoomed )
	{
		xsmin = xpagesize * xpage + xmin;
		xsmax = xpagesize * ( xpage + 1 ) + xmin - 1;

		ysmin = ypagesize * ypage + ymin;
		ysmax = ypagesize * ( ypage + 1 ) + ymin;
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

			int objno = cobj[i].snder;

			if ( from_to )
				objno = cobj[i].rcver;

			if ( obj[objno].node != select_node && select_node != nodes )
				continue;

			if ( ! highlight )
			{
				if ( objno != select_obj && select_obj != nobjs )
					continue;
			}

			if ( cobj[i].cputims < xsmin || cobj[i].cputimr > xsmax )
				continue;

			if ( cobj[i].sndtim < ysmin || cobj[i].sndtim > ysmax )
				continue;

			if ( committed && ( cobj[i].flags & 1 ) )
				continue;

			x1 = ( cobj[i].cputims - xsmin ) / xscale + xorg;
			x2 = ( cobj[i].cputimr - xsmin ) / xscale + xorg;

			for ( j = x1; j <= x2; j++ )
			{
				if ( low_vt[j] > cobj[i].sndtim )
					low_vt[j] = cobj[i].sndtim;

				if ( high_vt[j] < cobj[i].sndtim )
					high_vt[j] = cobj[i].sndtim;
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
	else if ( scaling_y )
	{
		ysmin = ymax; ysmax = ymin;

		for ( i = vt0; i < cno; i++ )
		{
			int objno = cobj[i].snder;

			if ( from_to )
				objno = cobj[i].rcver;

			if ( obj[objno].node != select_node && select_node != nodes )
				continue;

		  if ( ! highlight )
		  {
			if ( objno != select_obj && select_obj != nobjs )
				continue;
		  }
			if ( cobj[i].cputims < xsmin || cobj[i].cputimr > xsmax )
				continue;

			if ( committed && ( cobj[i].flags & 1 ) )
				continue;

			if ( ysmin > cobj[i].sndtim ) ysmin = cobj[i].sndtim;
			if ( ysmax < cobj[i].sndtim ) ysmax = cobj[i].sndtim;
		}

		yscale = (double)( ysmax - ysmin ) / ylen;
	}

	charx = chary = 0;

	from = vt0; to = cno; by = 1;

	if ( rollbacks )
	{
		for ( i = 0; i < nobjs; i++ )
			last_vt[i] = ymax;

		from = cno - 1; to = vt0 - 1; by = -1;
	}

	if ( nlist[il] )
		from = list[il][0];

	clear ();
	change_color ( CYAN );
	banner();
#ifdef X11
	XFlush ( x_display );
#endif

	redraw_page = 0;
}

plot_page ()
{
	register int i, j, k;

	if ( redraw_page )
		page_start();

/* 
#ifdef SUNVIEW
	cursor_set ( cursor, CURSOR_SHOW_CURSOR, FALSE, 0 );
	window_set ( canvas, WIN_CURSOR, cursor, 0 );
	pw_lock ( pw, &screen_rect );
#endif
*/
 
	k = 100;

	for ( i = from; i != to; i += by )
	{
		int line_color = MAGENTA;
		int objno = cobj[i].snder;

		if ( from_to )
			objno = cobj[i].rcver;

		screen_x1[i] = screen_x2[i] = 0;
		screen_y1[i] = screen_y2[i] = 0;

		if ( obj[objno].node != select_node && select_node != nodes )
			continue;

		if ( ! highlight )
		{
			if ( objno != select_obj && select_obj != nobjs )
				continue;
		}

		if ( (! committed) || (! (cobj[i].flags&1)) )
		{
			if ( cobj[i].cputims < xsmin || cobj[i].cputimr > xsmax )
				continue;

			if ( cobj[i].sndtim < ysmin || cobj[i].rcvtim > ysmax )
				continue;
		}

		if ( nlist[il] )
		{
			if ( list[il][0] == i )
				display_object ( i );
			else
			{
				for ( j = 0; j < nlist[il]; j++ )
				{
					if ( cobj[i].snder  == cobj[list[il][j]].rcver
					&&   cobj[i].sndtim == cobj[list[il][j]].rcvtim )
						break;
				}

				if ( j == nlist[il] )
					continue;

				display_object ( i );

				if ( nlist[il] < LISTSIZE )
					list[il][nlist[il]++] = i;
			}
		}

		if ( committed )
		{
			line_color = GREEN;
			j = -1;

			if ( cobj[i].flags & 1 )
			{
				for ( j = i-1; j >= vt0; j-- )
				{
					if ( cobj[j].snder == cobj[i].snder
					&&   cobj[j].id_num == cobj[i].id_num )
					{
						break;
					}
				}
				if ( j >= vt0 )
					line_color = RED;
			}
		}
		else if ( antimessages )
		{
			if ( cobj[i].flags & 2 )
				line_color = BLUE;
			else if ( cobj[i].flags & 1 )
				line_color = RED;
			else
				line_color = GREEN;
		}
		else if ( rollbacks )
		{
			if ( cobj[i].sndtim <= last_vt[objno] )
			{
				line_color = GREEN;
				last_vt[objno] = cobj[i].sndtim;
			}
			else
				line_color = RED;
		}
		else if ( stb89 )
		{
			if ( obj[objno].name[0] == 'G' )
				if ( obj[objno].name[1] == 'I' )
					line_color = YELLOW;
				else
					line_color = GREEN;
			if ( obj[objno].name[0] == 'R' ) line_color = RED;
			if ( obj[objno].name[0] == 'B' ) line_color = BLUE;
		}
		else if ( ctls || stb88 )
		{
			if ( obj[objno].name[0] == 'i' ) line_color = YELLOW;
			if ( obj[objno].name[0] == 'G' ) line_color = GREEN;
			if ( obj[objno].name[0] == 'r' ) line_color = RED;
			if ( obj[objno].name[0] == 'b' ) line_color = BLUE;
		}
		else if ( pucks )
		{
			if ( obj[objno].name[0] == 's' ) line_color = RED;
			if ( obj[objno].name[0] == 'c' ) line_color = BLUE;
			if ( obj[objno].name[0] == 'p' ) line_color = GREEN;
		}
		else if ( ants )
		{
			if ( obj[objno].name[0] == 'a' ) line_color = RED;
			if ( obj[objno].name[0] == 'n' ) line_color = BLUE;
			if ( obj[objno].name[0] == 'g' ) line_color = GREEN;
		}
		else if ( slooow )
		{
			if ( obj[objno].name[6] == '0' ) line_color = GREEN;
			else
			if ( obj[objno].name[0] == 'g' ) line_color = RED;
			else
				line_color = BLUE;
		}

		if ( obj[objno].name[0] == 'G'
		&&   obj[objno].name[1] == 'V' )
			line_color = CYAN;

		if ( highlight )
		{
			if ( objno == select_obj )
				line_color = WHITE;
		}

		change_color ( line_color );

		if ( committed && j >= vt0 )
		{
			scale_line ( cobj[j].cputims,  cobj[j].sndtim,
						 cobj[j].cputimr, cobj[j].rcvtim );
		}
		else
		{
			scale_line ( cobj[i].cputims, cobj[i].sndtim,
						 cobj[i].cputimr, cobj[i].rcvtim );

			screen_x1[i] = X1; screen_y1[i] = Y1;
			screen_x2[i] = X2; screen_y2[i] = Y2;
		}
		if ( annotate )
		{
			if ( cobj[i].sndtim == low_vt[X1] )
			if ( X1 > charx || Y1 > chary + font_height * 2 )
			{
				chary = Y1 - font_height - 1;
				text ( X1, chary, obj[objno].name );
				charx = X1 + strlen ( obj[objno].name ) * font_width;
			}
		}

		if ( --k == 0 )
		{
#ifdef X11
			XFlush ( x_display );
#endif
			from = i + by;
			busy_mode();
			return;
		}
	}

	if ( nlist[il] )
	{
#ifdef X11
		set_mark();
		mask_yellow();
		mask_text ( listx, listy, "end" );
#endif
#ifdef SUNVIEW
		writemask ( 8 );
		color = 8;
		text ( listx, listy, "end" );
		writemask ( -1 );
#endif
		nlist[il] = 0;
	}

/* 
#ifdef SUNVIEW
	pw_unlock ( pw );
	cursor_set ( cursor, CURSOR_SHOW_CURSOR, TRUE, 0 );
	window_set ( canvas, WIN_CURSOR, cursor, 0 );
#endif
*/

#ifdef X11
	XFlush ( x_display );
#endif

	if ( search || page_scan )
	{
		seconds_to_wait( 3 );
	}

	idle_mode();
}
