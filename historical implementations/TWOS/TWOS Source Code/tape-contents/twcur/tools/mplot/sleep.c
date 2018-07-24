/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#ifdef XVIEW
#include <X11/Xos.h> /* for  <sys/time.h> */
#include <xview/xview.h>
#include <xview/notify.h>
#endif

#ifdef SUNVIEW
#include <suntool/sunview.h>
#include <sunwindow/notify.h>
#include <sys/time.h>
#endif

struct itimerval timer;

extern Frame frame;

extern int search;
extern int redraw_page;
extern int select_node;
extern int page_scan, xpage, xnum_pages;
extern int nodes;

int clicks_to_next_search;

Notify_value
next_search()
{
	if ( page_scan )
	{
		if ( xpage < xnum_pages - 1 )
		{
			xpage++;
			redraw_page = 1;
			return;
		}
		else if ( search && (select_node < nodes - 1 ) )
			xpage = 0;
		else
			page_scan = 0;
	}

	if ( search )
	{
		if (select_node < nodes - 1 )
		{
			select_node++;
			redraw_page = 1;
		}
		else
			search = 0;
	}

	return NOTIFY_DONE;
}

seconds_to_wait ( value )
int value;
{
	if (value > 0)
		clicks_to_next_search = 5 * value;
/*
	if (value > 0)
	{
		timer.it_value.tv_sec = value;
		timer.it_value.tv_usec = 0;

		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;

		notify_set_itimer_func(frame, next_search,
			ITIMER_REAL, &timer, NULL);
	}
	else ?* turn it off *?
		notify_set_itimer_func(frame, NOTIFY_FUNC_NULL,
			ITIMER_REAL, NULL, NULL);
*/
}

Notify_value
idle_proc()
{
	if ( clicks_to_next_search > 0 )
	{
		clicks_to_next_search--;
		
	}
	else if ( clicks_to_next_search == 0 )
	{
		clicks_to_next_search = -1;
		next_search();
	}

	if ( redraw_page )
	{
		clicks_to_next_search = -1;
		notify_set_itimer_func(frame, NOTIFY_FUNC_NULL,
			ITIMER_REAL, NULL, NULL);
		plot_page();
	}

	return NOTIFY_DONE;
}

idle_mode()
{
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 200 * 1000;

		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 200 * 1000;

		notify_set_itimer_func(frame, idle_proc,
			ITIMER_REAL, &timer, NULL);
}

Notify_value
busy_proc()
{
	plot_page();

	return NOTIFY_DONE;
}

busy_mode()
{
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 100;

		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;

		notify_set_itimer_func(frame, busy_proc,
			ITIMER_REAL, &timer, NULL);
}
