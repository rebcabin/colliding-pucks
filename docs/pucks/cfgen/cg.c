#include <stdio.h>
#include <math.h>

#include "prdefs.h"

#define	FALSE	0
#define	TRUE	1

#define	new_cushions

#define MAX_INT	0x7fffffff


/*

a max and min val for xdot and ydot need to be est.
xdot and ydot can be pos or neg.

create pucks structs using number of pucks and malloc
each rn is tested against all prev pucks created
Once 2 ok rns are gen'ed, the puck is sent its evtmsg.
*/



typedef	struct
{
    int		x;
    int		y;
    double	xdot;
    double	ydot;
}
Puck_state;

/**********************************************************************/
int 	nnodes; 

main ( argc, argv )

int	argc;
char ** argv ;

{
    register int i, j ;

    /*  input parsing stuff  */

    char	id_string    [128];
    int		interactive;

    int		nsect_x, nsect_y, npucks ;
    int		min_board_x, max_board_x;
    int		min_board_y, max_board_y;


    /*  booleans */

    char	do_graphics ;
    char	read_puck_states_from_file ;
    char	show_puck_names ;
    char	show_sector_names ;
    char	show_velocity_vectors ;

    /*  an input file name */

    char	puck_state_file [128] ;

    /* puck parameters  */

    int		x, y;
    int		rn_ok;

    int		min_x, min_y, max_x, max_y;
    int		x_xmin, x_xmax, y_ymin, y_ymax ;
    int		hit_left, hit_right, hit_up, hit_down;
    int		hit_four ;
    int		xprime, yprime;

    double	xdot, ydot;

    double	diam;
    float	radius;
    float	mass;


    int		int_diam 	= 0 ;
    int		int_radius 	= 0 ;
    int		int_mass	= 0 ;

    Puck_state	* puck_state_table;

    double	rand_vel ();

    /* cushion paramenters  */

    int		x1, y1, x2, y2;

   /*  sector parameters  */

    char	sector [32];
    int		width, height;


    double sqrt () , dispersion , mean ;

    unsigned long random () ;

    register int r ;

    int even_flag ;


    if ( argc > 1 )
    {
	if ( strcmp ( argv[1], "-i" ) == 0 )
	{
	    interactive = TRUE;
	}
	else
	{
	    interactive = FALSE;
	}
    }

    if ( interactive )
    {
	char junk [128] ;

	fprintf ( stderr, "number of nodes: ");
	scanf   ( "%d",  & nnodes );

	fprintf ( stderr, "number of sector rows: ");
	scanf   ( "%d", & nsect_y );

	fprintf ( stderr, "number of sector cols: ");
	scanf   ( "%d", & nsect_x );

	fprintf ( stderr, "number of pucks: ");
	scanf   ( "%d", & npucks );

	fprintf ( stderr, "puck radius: ");
	scanf   ( "%f", & radius );

	fprintf ( stderr, "puck mass: ");
	scanf   ( "%f", & mass );

	fprintf ( stderr, "min x: ");
	scanf  ( "%d", & min_board_x );

	fprintf ( stderr, "max x: ");
	scanf   ( "%d", & max_board_x );

	fprintf ( stderr, "min y: ");
	scanf  ( "%d", & min_board_y );

	fprintf ( stderr, "max y: ");
	scanf   ( "%d", & max_board_y );

	fprintf ( stderr, "do graphics (yes/no)?: ");
	scanf   ( "%s", junk);
	do_graphics = ( junk[0] == 'y' ) ;

	if (do_graphics)
	{
	    fprintf ( stderr, "show puck names (yes/no)?: ");
	    scanf   ( "%s", junk);
	    show_puck_names = ( junk[0] == 'y' ) ;

	    fprintf ( stderr, "show sector names (yes/no)?: ");
	    scanf   ( "%s", junk);
	    show_sector_names = ( junk[0] == 'y' ) ;

	    fprintf ( stderr, "show velocity vectors (yes/no)?: ");
	    scanf   ( "%s", junk);
	    show_velocity_vectors = ( junk[0] == 'y' ) ;
	}

	fprintf ( stderr, "read puck states from file (yes/no)?: ") ;
	scanf   ( "%s", junk) ;
	read_puck_states_from_file = ( junk[0] == 'y' ) ;

	if ( read_puck_states_from_file )
	{
	    fprintf ( stderr, "Enter file name: ") ;
	    scanf   ( "%s", puck_state_file ) ;
	}
    }

    else /* non-interactive input */
    {
	char junk [128] ;



	scanf  ( "%s %d", id_string, & nnodes );

	if (strcmp(id_string, "number_of_nodes")!=0)
	{
	    fprintf (stderr, 
		"string \"number_of_nodes\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & nsect_y );

	if (strcmp(id_string, "number_of_sector_rows")!=0)
	{
	    fprintf (stderr, 
		"string \"number_of_sector_rows\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & nsect_x);

	if (strcmp(id_string, "number_of_sector_cols")!=0)
	{
	    fprintf (stderr, 
		"string \"number_of_sector_cols\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & npucks );

	if (strcmp(id_string, "number_of_pucks")!=0)
	{
	    fprintf (stderr, 
		"string \"number_of_pucks\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %f", id_string, & radius );

	if (strcmp(id_string, "puck_radius")!=0)
	{
	    fprintf (stderr, 
		"string \"puck_radius\" expected: exiting\n");
	    exit (-1) ;
	}

	scanf  ( "%s %f", id_string, & mass );

	if (strcmp(id_string, "puck_mass")!=0)
	{
	    fprintf (stderr, 
		"string \"puck_mass\" expected: exiting\n");
	    exit (-1) ;
	}




	scanf  ( "%s %d", id_string, & min_board_x );

	if (strcmp(id_string, "min_x")!=0)
	{
	    fprintf (stderr, 
		"string \"min_x\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & max_board_x );

	if (strcmp(id_string, "max_x")!=0)
	{
	    fprintf (stderr, 
		"string \"max_x\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & min_board_y );

	if (strcmp(id_string, "min_y")!=0)
	{
	    fprintf (stderr, 
		"string \"min_y\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf  ( "%s %d", id_string, & max_board_y );

	if (strcmp(id_string, "max_y")!=0)
	{
	    fprintf (stderr, 
		"string \"max_y\" expected: exiting\n");
	    exit (-1) ;
	}



	scanf ("%s %s", id_string, junk) ;

	if (strcmp(id_string, "do_graphics?") != 0)
	{
	    fprintf (stderr, 
		"string \"do_graphics?\" expected: exiting\n");
	    exit (-1) ;
	}

	do_graphics = junk[0] == 'y' ;



	scanf ("%s %s", id_string, junk) ;

	if (strcmp(id_string, "show_puck_names?") != 0)
	{
	    fprintf (stderr, 
		"string \"show_puck_names?\" expected: exiting\n");
	    exit (-1) ;
	}

	show_puck_names = junk[0] == 'y' ;



	scanf ("%s %s", id_string, junk) ;

	if (strcmp(id_string, "show_sector_names?") != 0)
	{
	    fprintf (stderr, 
		"string \"show_sector_names?\" expected: exiting\n");
	    exit (-1) ;
	}

	show_sector_names = junk[0] == 'y' ;



	scanf ("%s %s", id_string, junk) ;

	if (strcmp(id_string, "show_velocity_vectors?") != 0)
	{
	    fprintf (stderr, 
		"string \"show_velocity_vectors?\" expected: exiting\n");
	    exit (-1) ;
	}

	show_velocity_vectors = junk[0] == 'y' ;



	scanf ("%s %s", id_string, junk) ;
	
	if (strcmp(id_string, "read_puck_states_from_file?") != 0)
	{
	    fprintf (stderr, 
	    "string \"read_puck_states_from_file?\" expected: exiting\n");
	    exit (-1) ;
	}

	read_puck_states_from_file = ( junk[0] == 'y' ) ;



	scanf ("%s %s", id_string, puck_state_file) ;
	
	if (strcmp(id_string, "file_name") != 0)
	{
	    fprintf (stderr, 
		"string \"file_name\" expected: exiting\n");
	    exit (-1) ;
	}
    }

/*???PJH
    if ( nsect_x != nsect_y )	
    {
	fprintf (stderr, "number of sector rows must = number of cols \n" );
	fprintf (stderr, "so that the neighbor-preserving map from\n") ;
	fprintf (stderr, "table to hypercube will work easily.\n") ;

	exit ( -1 );
    }
	*/

    switch ( nsect_x )
    {
	case	2:
	case	4:
	case	8:
	case   16:
	case   32:
	case   64:
        case   48:

		break;
/*???PJH
	default:
	{
		fprintf (stderr, "num sects must be 2, 4, 8, 16, or 32 \n");
		fprintf (stderr, "so that the neighbor-preserving map from\n") ;
		fprintf (stderr, "table to hypercube will work easily.\n") ;

		exit ( -1 );
	}		*/

    }

    if ( nnodes > 128 )
    {
        fprintf (stderr, "must be fewer than 128 nodes\n") ;
        exit (-1) ;
    }  
 

    puck_state_table = 
	(Puck_state *) calloc ( npucks+1, sizeof ( Puck_state ) );

    /*  we alloc one more than needed so 
     *	the array will be 'null terminated'  
     */


    if ( puck_state_table == NULL )
    {
	fprintf ( stderr,"failed to calloc puck state table \n" );
	exit (-1);
    }


    diam = radius * 2;

    int_radius = ((int) radius) + 1 ;  /* integer upper bound */
    
    int_diam =   int_radius * 2 ;    /* integer upper bound */

    printf ( "nostdout\n" );
    printf ( "objstksize 7000\n" );
    printf ( "\n" ) ;
    printf ( "\n" ) ;

    srandom ( time (0) ) ;


    /*  generate obcreates for sectors and cushions  */

    gmap ( nsect_x, nsect_y );


    /*  generate obcreates for pucks */

    even_flag = 0;
 
    for ( i = 0; i < npucks; i++ )
    {
        if (even_flag == 0)    r = random () % nnodes ;
        else                   r = i % nnodes ;
 
	printf 
        (
	    "obcreate puck%-4.4d puck %d \n", 
	    i, r 
	) ;

    }



    /*  write out evtmsg's  */


    /*  C U S H I O N   E V T M S G S  */

    width  = max_board_x / nsect_x;
    height = max_board_y / nsect_y;


    /*  Note: (x2,y2) must be ccw wrt (x1,y1)  */

    for ( i = 0; i < nsect_y; i++ )
    {

	/* This printf takes care of cushions on the western edge.  */

	printf 
	( 
	    "tell cushion_0_%-2.2d -1 2 \"%d %d %d %d %d %d %d %d\" \n", 
	    i, 
	    min_board_x,
	    min_board_y + ( i * height ), 
	    min_board_x, 
	    min_board_y + ( (i + 1) * height ),
	    min_board_x, min_board_y,
	    max_board_x, max_board_y
	);
    }


    for ( i = 0; i < nsect_x; i++ )
    {
	/*  This printf takes care of cushions on the northern edge */

	printf 
	( 
	    "tell cushion_1_%-2.2d -1 2 \"%d %d %d %d %d %d %d %d\" \n", 
	    i, 
	    min_board_x + ( i * width ),
	    min_board_y,
	    min_board_x + ( (i + 1) * width ),
	    min_board_y,
	    min_board_x, min_board_y,
	    max_board_x, max_board_y
 
	);
    }

    for ( i = 0; i < nsect_y; i++ )
    {
	/*  This printf takes care of cushions on the eastern edge.  */

	printf 
	( 
	    "tell cushion_2_%-2.2d -1 2 \"%d %d %d %d %d %d %d %d\" \n", 
	    i, 
	    max_board_x,
	    min_board_y + ( i * height ) ,
	    max_board_x, 
	    min_board_y + ( (i + 1) * height ),
	    min_board_x, min_board_y,
	    max_board_x, max_board_y
	);
    }

    for ( i = 0; i < nsect_x; i++ )
    {
	/*  This printf takes care of cushions on the southern edge */

	printf 
	( 
	    "tell cushion_3_%-2.2d -1 2 \"%d %d %d %d %d %d %d %d\" \n", 
	    i, 
	    min_board_x + ( i * width ), 
	    max_board_y,
	    min_board_x + ( (i + 1) * width ),
	    max_board_y,
	    min_board_x, min_board_y,
	    max_board_x, max_board_y
 
	);
    }

    printf ( "\n" );
	

    /*  S E C T O R   E V T M S G S  */

    for ( i=0; i < nsect_y; i++ )
    {

	for ( j = 0; j < nsect_x; j++ )
	{
	    printf 
            (
	        "tell sector_%-2.2d_%-2.2d -1 3 \"%d %d %d %d %d %d\"\n",
	        i, 
	        j, 
	        nsect_y,nsect_x,
		min_board_x + ( j * width ),
		min_board_y + ( i * height ),
	        height, width
	    );	   

	    if (do_graphics)
	    {
		static struct point
		{
		    int x, y ;
		}
		    tl, tr, bl, br ;  /* top left, top right, etc. */

		tl.x = min_board_x + ( j * width ) ;
		tl.y = min_board_y + ( i * height ) ;

		tr.x = tl.x + width ;
		tr.y = tl.y ;

		bl.x = tl.x ;
		bl.y = tl.y + height ;

		br.x = tr.x ;
		br.y = bl.y ;

		printf ( "pad:vector %4d %4d %4d %4d\n", 
		    tl.x, tl.y, tr.x, tr.y) ;

		printf ( "pad:vector %4d %4d %4d %4d\n", 
		    tr.x, tr.y, br.x, br.y) ;

		printf ( "pad:vector %4d %4d %4d %4d\n", 
		    br.x, br.y, bl.x, bl.y) ;

		printf ( "pad:vector %4d %4d %4d %4d\n", 
		    bl.x, bl.y, tl.x, tl.y) ;

		if (show_sector_names)
		{
		    printf 
		    (   
			"pad:printstring %d %d %02.2d_%02.2d\n",
			    tl.x + 8, tl.y + 16, i, j
		    ) ;
		}
	    }
	}
    }

    printf ( "\n" );

    /*  B A L L    E V T M S G S  */


    if ( max_board_x % nsect_x != 0 )
    {
	max_board_x = width * nsect_x ;

	fprintf (stderr, "Max_board_x being truncated to %d\n", max_board_x) ;
	fprintf (stderr, "  to be commensurate with sector width\n") ;
	fflush (stderr) ;
    }


    if ( max_board_y % nsect_y != 0 )
    {
	max_board_y = height * nsect_x ;

	fprintf (stderr, "Max_board_y being truncated to %d\n", max_board_y) ;
	fprintf (stderr, "  to be commensurate with sector height\n") ;
	fflush (stderr) ;
    }
 



    if ( read_puck_states_from_file )
    {
	FILE          * f ;
	char	  	TheLine [256] ;
	int		i ;  	/* index into puck_state_table */

	f = fopen (puck_state_file, "r") ;

	if ( (int)f <= 0 )
	{
	    fprintf ("Failure to open file %s, bailing out!\n", 
		puck_state_file) ;
	    exit (-2) ;
	}


	i = 0 ;

	while ( fgets (TheLine, 255, f) )  
	{
	    /*  fgets returns NULL on EOF  */

	    if ( strncmp (TheLine, "tell puck", 9) != 0 )
	    {
		/*  no match  */

		continue ;  /*  get next line in the file  */
	    }

	    else  /*  got a line we can use  */
	    {
		char 		puckname [32] ;
		static char 	last_puckname [32] ;
		char 		junkjunk [128] ;
		int		radius, mass;
		int		time ;
		int		retval ;
		int		msgsel ;

		retval = sscanf 
		(
		    TheLine, "tell %s %d %d \"%d %d %d %d %lf %lf",

		    puckname, &time, &msgsel, 
		    & radius, & mass,

		    & x, & y, & xdot, & ydot
		);


		/***   pr2(d,retval,time) ;   ***/
		/***   pr2(s,puckname,junkjunk) ;   ***/
		/***   pr2(d,x,y) ;   ***/
		/***   pr2(lf,xdot,ydot) ;   ***/

		/***   fflush (stdout) ;   ***/


		if (strcmp(puckname, last_puckname) == 0)
		{
		    /*  This is a second or later evtmsg for 
		     *  the same puck... we only want the 
		     *  first occurrence to get the x, y... from
		     */
		    continue ;
		}

		else
		{
		    strcpy (last_puckname, puckname) ;

		    add_state_to_table 
		    ( 
			puck_state_table, i++, x, y, xdot, ydot 
		    );
		}

	    }  /*  got a line we can use  */

	}  /*  while fgets  */

	if ( i != npucks )
	{
	    fprintf ("Something wrong in your input file; bye!\n") ;
	    exit (2) ;
	}

	fclose (f) ;

    }  /*  read puck states from file  */



    else  /*  generating puck states randomly  */
    {
	for ( i = 0; i < npucks; i++ )
	{
	    rn_ok = FALSE;

	    while ( rn_ok != TRUE )
	    {
		/*  for now, start pts are at integer positions  */
		/*  make sure they don't overlap the edges       */
		/*  of the table to start with (no 'dead' pucks) */
		/*  To make sure of this, we will create them on */
		/*  a smaller board and then bump them one to    */
		/*  keep them off the left edge                  */

		/*  The code corrects the table size by truncation
		 *  in the case where table_size % sector_size != 0.
		 *  This must be done to prevent puck positions from
		 *  being generated on the table but outside any 
		 *  sector.  Search for the word 'truncate' in this source
		 *  to find the code that truncates the table size.
		 */


		x = random() % (max_board_x - int_diam - 2) ;	

		x += min_board_x + int_radius + 1 ;


		y = random() % (max_board_y - int_diam - 2) ;

		y += min_board_y + int_radius + 1 ;


		if 
		(
		    y>max_board_y-int_radius-1 || 
		    y<min_board_y+int_radius+1 ||
		    x>max_board_x-int_radius-1 || 
		    x<min_board_x+int_radius+1 
		)
		{
		    fprintf 
		    (
			stderr, 
			"A puck was generated outside the table\007\n"
		    ) ;

		    exit (-1) ;
		}

		rn_ok = check_puck_pos ( puck_state_table, i, diam, x, y );
	    }

	    xdot = rand_vel ( radius, max_board_x, max_board_y );
	    ydot = rand_vel ( radius, max_board_x, max_board_y );

	    add_state_to_table ( puck_state_table, i, x, y, xdot, ydot );

	}   /* for each puck */

    }   /* generating puck states randomly */




    /***   dump_puck_state_table (puck_state_table) ;   ***/




    for ( i = 0; i < npucks; i++ )
    {
	x = puck_state_table [i] . x ;
	y = puck_state_table [i] . y ;

	xdot = puck_state_table [i] . xdot ;
	ydot = puck_state_table [i] . ydot ;


	if (do_graphics)
	{
	    printf ( "pad:circle16 %4d %4d %4d\n", x, y, (int) radius );

	    if (show_puck_names)
	    {
		printf ( "pad:printstring %4d %4d %04.4d\n", x, y+4, i ) ;
	    }
	}

	if (do_graphics && show_velocity_vectors)
	{
	    int             end_x,   end_y ;
	    int             arrow_x, arrow_y ;

	    register double ca, sa ;
	    register double xa, ya, xp, yp ;
	    register double dx, dy ;
	    register double r ;
	    double          sqrt () ;

	    end_x = x + xdot ;
	    end_y = y + ydot ;

	    printf ("pad:vector %4d %4d %4d %4d\n", x, y, end_x, end_y) ;

	    dx = end_x - x ;
	    dy = end_y - y ;

	    r = sqrt ( dx * dx   +   dy * dy ) ;
	    ca = dx / r ;
	    sa = dy / r ;

	    xa = end_x - 8 * ca ;
	    ya = end_y - 8 * sa ;

	    xp = xa - 3 * sa ;
	    yp = ya + 3 * ca ;

	    arrow_x = xp ;
	    arrow_y = yp ;

	    printf ("pad:vector %4d %4d %4d %4d\n", 
		end_x, end_y, arrow_x, arrow_y) ;

	    xp = xa + 3 * sa ;
	    yp = ya - 3 * ca ;

	    arrow_x = xp ;
	    arrow_y = yp ;

	    printf ("pad:vector %4d %4d %4d %4d\n", 
		end_x, end_y, arrow_x, arrow_y) ;
	}

	/* this next bit of conditionals figures out what sectors a puck
	 * is touching and sends appropriate messages to all such sectors
	 */

	min_x = ( x / width ) * width;
	max_x = min_x + width;

	min_y = ( y / height ) * height;
	max_y = min_y + height;

	x_xmin = (x - min_x) ;
	y_ymin = (y - min_y) ;
	x_xmax = (x - max_x) ;
	y_ymax = (y - max_y) ;

	hit_left  = x_xmin <= radius;  /* you are challenged to figger */
	hit_up    = y_ymin <= radius;  /* these out! */
	hit_right = x_xmax >= - radius;
	hit_down  = y_ymax >= - radius; 

	if (hit_left && hit_up)
	{
	    hit_four = sqrt (x_xmin * x_xmin + y_ymin * y_ymin) <= radius ;

	    if (hit_four &&
		next (sector, y/height - 1, x/width - 1, nsect_y, nsect_x) )
	    {
 	     printf 
	      (
	       "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\"\n", 
	        i ,  (int)radius, (int)mass, x, y, xdot, ydot, sector, 
		    width, height, min_board_x, min_board_y, max_board_x, 
		    max_board_y );
	    }
	}

	else if (hit_up && hit_right)
	{
	    hit_four = sqrt (x_xmax * x_xmax + y_ymin * y_ymin) <= radius ;

	    if (hit_four &&
		next (sector, y/height - 1, x/width + 1, nsect_y, nsect_x) )
	    {
	     printf 
	      (
	       "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
	        i , (int)radius, (int)mass, x, y, xdot, ydot, sector, 
	        width, height,  min_board_x, min_board_y, max_board_x, max_board_y
	      );
	    }
	}

	else if (hit_right && hit_down)
	{
	    hit_four = sqrt (x_xmax * x_xmax + y_ymax * y_ymax) <= radius ;

	    if (hit_four &&
		next (sector, y/height + 1, x/width + 1, nsect_y, nsect_x) )
	    {
	     printf 
	      (
	       "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
		i , (int)radius, (int)mass,  x, y, xdot, ydot, sector,
		width, height, min_board_x, min_board_y, max_board_y, max_board_y
	      );
	    }
	}

	else if (hit_down && hit_left)
	{
	    hit_four = sqrt (x_xmin * x_xmin + y_ymax * y_ymax) <= radius ;

	    if (hit_four &&
		next (sector, y/height + 1, x/width - 1, nsect_y, nsect_x) )
	    {
	     printf 
	      (
	       "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
	        i , (int)radius, (int)mass, x, y, xdot, ydot, sector,
	        width, height, min_board_x, min_board_y, max_board_x, max_board_y
	      );
	    }
	}

	if (hit_left && 
	    next (sector, y/height, x/width - 1, nsect_y, nsect_x))
	{
	    printf 
	    (
	      "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d  %d %d %d %d\" \n", 
	       i , (int)radius, (int)mass, x, y, xdot, ydot, sector,
	       width, height, min_board_x, min_board_y, max_board_x, max_board_y
	    );
	}

	if (hit_up && 
	    next (sector, y/height - 1, x/width, nsect_y, nsect_x))
	{
	    printf ( "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n",
	    i , (int)radius, (int)mass, x, y, xdot, ydot, sector,
	    width, height,  min_board_x, min_board_y,max_board_x, max_board_y); 
	}

 if (hit_right &&
	    next (sector, y/height, x/width + 1, nsect_y, nsect_x))
	{
	    printf 
	    (
	     "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
	      i , (int)radius, (int)mass, x, y, xdot, ydot, sector,
	      width, height,  min_board_x, min_board_y,max_board_x, max_board_y
	    );
	}

	if (hit_down &&
	    next (sector, y/height + 1, x/width, nsect_y, nsect_x))
	{
	    printf 
	    (
	      "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
	       i , (int)radius, (int)mass,  x, y, xdot, ydot, sector,
	       width, height, min_board_x, min_board_y,max_board_x, max_board_y
	    );
	}

        sprintf ( sector, "sector_%-2.2d_%-2.2d", y / height, x / width   );

	printf 
	(
	    "tell puck%-4.4d 0 1 \"%d %d %d %d %3.1f %3.1f %s %d %d %d %d %d %d\" \n", 
	    i , (int)radius, (int)mass,  x, y, xdot, ydot, sector,
	    width, height, min_board_x, min_board_y, max_board_x, max_board_y
	);

        fflush (stdout);

    } /*  for each puck */


    printf ("\n") ;
 
    exit (0) ;
}

/**********************************************************************/
/*

strategy :

look through the puck state table.

do quick checks first.

if either dx or dy is greater than 2r, then this coord pr is ok.

if the quick tests are passed, perform the complete test.

*/

#define	NOT_OK	0
#define	OK	1

check_puck_pos ( bp_ptr, npucks, diam, x, y )

Puck_state    * bp_ptr;
int		npucks;
double		diam;
int		x;
int		y;

{
    register	int		i;
    register	double		dx, dy;
    double			dist;


    for ( i = 0; i < npucks; i++ )
    {
	if ( dx = abs ( x - bp_ptr-> x ) <= diam )
	{
	    if ( dy = abs ( y - bp_ptr-> y ) <= diam )
	    {
		if ( dist = sqrt ( dx*dx + dy*dy ) <= diam )
		{
		    return ( NOT_OK );
		}
	    }
	}

	bp_ptr++;
    }

    return ( OK );
}


/**********************************************************************/

add_state_to_table ( bp_ptr, index, x, y, xdot, ydot )

Puck_state *		bp_ptr;
int			index;
int			x;
int			y;
double			xdot ;
double			ydot ;

{
    bp_ptr += index;

    bp_ptr -> x = x;
    bp_ptr -> y = y;

    bp_ptr -> xdot = xdot ;
    bp_ptr -> ydot = ydot ;
}

/**********************************************************************/


dump_puck_state_table (b)
Puck_state	      *	b ;
{
    int i ;

    for ( i=0; b->x > 0 ; i++, b++ )
    {
	epr3(d, i, b->x, b->y) ;
	epr2(lf, b->xdot, b->ydot) ;
	ENL ;
    }
}

/**********************************************************************/

/*  
This is a velocity component - not true velocity
which is the sqrt of the square of the component velocities.
*/

double
rand_vel ( radius, max_board_x, max_board_y )

float 	radius;
int	max_board_x;
int	max_board_y;

{
    double		t;
    static	double	max_vel_comp = 0;


    if ( max_vel_comp == 0 )
    {
	/*
    	 * max_vel_comp = i
	 *  ( max_board_x < max_board_y ? max_board_x : max_board_y ) / 10.0;
	 */

    	max_vel_comp = radius * 10.0;
    }

    t = ( (double) random () / MAX_INT ) * ( 2 * max_vel_comp );

    return ( t < max_vel_comp ? -t : t );
}

/**********************************************************************/

next  ( sector, row, col, max_row, max_col )

char	sector [32];
int	row, col, max_row, max_col;

{
    if 
    (
	row > max_row      ||
	row < 0            ||
	col > max_col      ||
	col < 0
    )
	return 0 ;

    else
    {
	sprintf ( sector, "sector_%-2.2d_%-2.2d", row, col );

	return 1 ;
    }
}

/**********************************************************************/
