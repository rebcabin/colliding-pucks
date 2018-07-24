#include "mplot.h"
#include <xview/xview.h>
#include <xview/canvas.h>

extern Canvas canvas;
extern Menu main_menu;

Notify_value
mag_handler ()
{
	menu_set ( menu_find ( main_menu, MENU_STRING, "magnify", 0 ),
		MENU_INACTIVE, FALSE, 0 );

	return NOTIFY_DONE;
}

magnify ()
{
	int pid;
	extern int errno;

	menu_set ( menu_find ( main_menu, MENU_STRING, "magnify", 0 ),
		MENU_INACTIVE, TRUE, 0 );

	pid = vfork ();

	if ( pid == -1 ) /* fork failed */
	{
		printf ( "fork failed errno is %d\n", errno );
	}
	else if ( pid ) /* parent */
	{
		notify_set_wait3_func ( canvas, mag_handler, pid );
	}
	else /* child */
	{
		execlp ( "/bin/X11/xmag",
			"xmag", "-display", getenv("DISPLAY"), (char *) 0);

		printf ( "exec failed\n" );
	}
}
