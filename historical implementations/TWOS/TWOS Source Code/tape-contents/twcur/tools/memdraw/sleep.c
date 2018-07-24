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

extern Frame frame;
extern int redraw_page;

struct itimerval timer;

/* It seems that idle_proc gets called periodically when the program is just
   waiting for input, and busy_proc gets called periodically (quite often)
   while the page is being plotted (once for every 100 lines in the graph).
   This allows XView to process input during the drawing process, which is
   good because a full draw (repaint) can take longer than anyone wants to
   wait. 
   -dp */

Notify_value
idle_proc()
{

	if ( redraw_page )
	{
		notify_set_itimer_func(frame, 
			NOTIFY_FUNC_NULL,
			ITIMER_REAL, NULL,
			NULL);
		plot_page();
	}

	return NOTIFY_DONE;
}

idle_mode()
/* sets the timer to make infrequent calls to idle_proc, just to check 
   whether it needs to start redrawing the page */
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
/* called from plot_page when it is done plotting one set of 100 lines in
   order to set the timer to trigger the next call to plot_page to continue
   in the stepwise (100 lines at a time) drawing process */
{
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 100;

		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;

		notify_set_itimer_func(frame, busy_proc,
			ITIMER_REAL, &timer, NULL);
}
