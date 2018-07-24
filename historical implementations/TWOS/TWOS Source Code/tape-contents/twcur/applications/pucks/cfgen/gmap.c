

#include <stdio.h>
#include <ctype.h>

/**********************************************************************/
static int cheat_node_num = 0;
extern int nnodes;



gmap ( max_ii, max_jj )

int 	max_ii, max_jj ;

{
    int hyper_i , hyper_j ;
    int NodeNum [4] [8] ;


    register int i, j, k ;

    int num [2] ;  /*** see gridmap.c/gridinit() for 
			the docs for this parameter 
		    ***/

    num [0] = hyper_i = max_ii < 4 ? max_ii : 4 ;
    num [1] = hyper_j = max_jj < 8 ? max_jj : 8 ;

    check (    gridinit ( 2, num )    ) ;

    for (i=0; i<hyper_i; i++)
    {
	for (j=0; j<hyper_j; j++)
	{
	    num [0] = i ;
	    num [1] = j ;

	    check (   k = gridproc (num)   ) ;
/*
	    fprintf (stderr, 
	    "The Node Num for (%2d,%2d) is %2d\n",i,j,k) ;
*/
	    
	    NodeNum [i] [j] = k ;
	}
    }




    for (j = 0 ;  j < max_jj ;  j++) 
    {
	for (i = 0 ;  i < max_ii ;  i++)
	{
	    k = NodeNum [ (i*hyper_i)/max_ii ] [ (j*hyper_j)/max_jj ] ;
/*
	    if ( cheat_node_num <  nnodes )
	     {
		k = cheat_node_num++;
	     }
	    else
	        k = cheat_node_num = 0;

	*/

	    printf 
	    (
		"obcreate sector_%02.2d_%02.2d sector %2d\n",
		j, i, k  
	    );
	}
    }

    printf ("\n\n");

    cheat_node_num = 0;

    /*  West cushion  */

    for (i = 0 ;  i < max_jj ;  i++) 
    {
	k = NodeNum [ (i*hyper_i)/max_jj ] [ 0 ] ;
/*
	    if ( cheat_node_num <  nnodes )
	     {
		k = cheat_node_num++;
	     }
	    else
	        k = cheat_node_num = 0;
	*/

	printf 
	(
	    "obcreate cushion_0_%02.2d cushion %2d\n",
	    i, k 
	);
    }

    printf ("\n");

    /*  --- North cushion ---  */

    for (j = 0 ;  j < max_ii ;  j++)
    {
	k = NodeNum [ 0 ] [ (j*hyper_j)/max_jj ] ;
/*
	    if ( cheat_node_num <  nnodes )
	     {
		k = cheat_node_num++;
	     }
	    else
	        k = cheat_node_num = 0;
	*/

	printf 
	(
	    "obcreate cushion_1_%02.2d cushion %2d\n",
	    j, k 
	);
    }

    printf ("\n");

    /*  --- East cushion ---  */

   for (i = 0 ;  i < max_jj ;  i++) 
    {
	k = NodeNum [ (i*hyper_i)/max_ii ] [ (hyper_j - 1) ] ;
/*
	    if ( cheat_node_num <  nnodes )
	     {
		k = cheat_node_num++;
	     }
	    else
	        k = cheat_node_num = 0;
	*/

	printf 
	(
	    "obcreate cushion_2_%02.2d cushion %2d\n",
	    i,  k 
	);
    }

    printf ("\n");

    /* ---  South cushion ---  */

    for (j = 0 ;  j < max_ii ;  j++)
    {
	k = NodeNum [ (hyper_i - 1) ] [ (j*hyper_j)/max_jj ] ;
	fprintf ( stderr, "j %d k %d cord1 %d cord2 %d\n", j, k,
	    hyper_i - 1, (j*hyper_j)/max_jj );
/*
	if ( cheat_node_num <  nnodes )
	  {
	      k = cheat_node_num++;
	  }
	  else
	      k = cheat_node_num = 0;
	*/

	printf 
	(
	    "obcreate cushion_3_%02.2d cushion %2d\n",
	    j,  k 
	);
    }

    printf ("\n");

    fflush (stdout) ;
}

/**********************************************************************/

check (r)

int r ;

{
    if ( r == -1 ) 
    {
	fprintf (stderr,"Sorry, the call to gridmap.c failed -- bye!\n") ;
	exit (0) ;
    }
}

/**********************************************************************/
/* zzz */

