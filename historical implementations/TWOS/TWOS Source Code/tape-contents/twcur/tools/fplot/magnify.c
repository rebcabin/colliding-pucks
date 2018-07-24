/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */ 
	 
#include <suntool/sunview.h>
#include <suntool/canvas.h>

extern Pixwin *pw;

extern char red[256], green[256], blue[256];

extern int width, height, color;

extern int dev, dev_x, dev_y;

typedef char RCOLORS[40];
typedef char WCOLORS[200];

magnify ()
{
    struct pixrect *prr, *prw, *prc;

    RCOLORS * rcolors;
    WCOLORS * wcolors;
    WCOLORS * ccolors;

    int x1, x2, y1, y2;
    int last_x1, last_x2;
    int last_y1, last_y2;
    int x, y;
    int i, j, k, l;

    prr = mem_create ( 40, 40, 8 );
    prw = mem_create ( 200, 200, 8 );
    prc = mem_create ( 200, 200, 8 );

    rcolors = (RCOLORS *)mpr_d(prr)->md_image;
    wcolors = (WCOLORS *)mpr_d(prw)->md_image;
    ccolors = (WCOLORS *)mpr_d(prw)->md_image;

    for ( i = 0; i < 200; i++ )
    for ( j = 0; j < 200; j++ )
    {
	wcolors[i][j] = ccolors[i][j] = 8;
    }

    for ( i = (1<<3); i < (1<<3)+8; i++ ) mapcolor ( i,   0,   0,   0 );
    for ( i = (2<<3); i < (2<<3)+8; i++ ) mapcolor ( i, 255,   0,   0 );
    for ( i = (3<<3); i < (3<<3)+8; i++ ) mapcolor ( i,   0, 255,   0 );
    for ( i = (4<<3); i < (4<<3)+8; i++ ) mapcolor ( i, 255, 255,   0 );
    for ( i = (5<<3); i < (5<<3)+8; i++ ) mapcolor ( i,   0,   0, 255 );
    for ( i = (6<<3); i < (6<<3)+8; i++ ) mapcolor ( i, 255,   0, 255 );
    for ( i = (7<<3); i < (7<<3)+8; i++ ) mapcolor ( i,   0, 255, 255 );
    for ( i = (8<<3); i < (8<<3)+8; i++ ) mapcolor ( i, 255, 255, 255 );

    pw_putcolormap ( pw, 0, 64, red, green, blue );

    writemask ( 7 << 3 );

    last_x1 = last_x2 = 0;
    last_y1 = last_y2 = 0;

    for ( ;; )
    {
/*
	    for ( dev = 0; dev == 0; )
	    {
*/
		notify_dispatch ();

		x = dev_x;
		y = dev_y;

		if ( x < 20 || x > (width-20) ) continue;
		if ( y < 20 || y > (height-20) ) continue;

		x1 = x - 20; x2 = x + 20;
		y1 = y - 20; y2 = y + 20;

		if ( x1 == last_x1 && y1 == last_y1 ) continue;

		color = 0;
		recti ( last_x1, last_y1, last_x1+40, last_y1+40 );
		color = 16;
		recti ( x1, y1, x2, y2 );

		last_x1 = x1; last_y1 = y1;
/*
	    }
*/
	if ( dev == MS_LEFT )
	    break;

	    pw_write ( pw, last_x2, last_y2, 200, 200, PIX_SRC, prc, 0, 0 );

	    pw_read ( prr, 0, 0, 40, 40, PIX_SRC, pw, x-20, height-y-20 );

	recti ( x1, y1, x2, y2 );

	    for ( i = 0; i < 40; i++ )
	    {
		for ( j = 0; j < 40; j++ )
		{
		    for ( k = 1; k <= 3; k++ )
		    for ( l = 0; l <= 4; l++ )
		    {
			wcolors[i*5+k][j*5+l] = ((rcolors[i][j]&7)+1) << 3;
		    }
		}
	    }

	    x1 = x + 60;
	    if ( x1 > (width-200) )
		x1 = x - 260;
	    x2 = x1 + 199;

	    y1 = y - 100;
	    if ( y1 < 0 )
		y1 = 0;
	    if ( y1 > (height-200) )
		y1 = height-200;
	    y2 = y1 + 199;

	    pw_write ( pw, x1, height - y2, 200, 200, PIX_SRC, prw, 0, 0 );

	    color = 16;
	    recti ( x1, y1, x2, y2 );

	    last_x2 = x1; last_y2 = height - y2;
    }

    pr_destroy ( prr );
    pr_destroy ( prw );
    pr_destroy ( prc );

    clear ();

    writemask ( -1 );
}
