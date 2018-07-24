#include <stdio.h>
#include <ctype.h>

main ( argc, argv )
int argc ;
char ** argv ;
{
    fprintf (stderr,"*********************************************************************\n") ;
    fprintf (stderr,"    This program addresses the problem of assigning pool table sectors to the\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"hypercube nodes in a neighbor-preserving way.  Neighbor-preserving means that\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"neighboring sectors on the pool table will be assigned at worst to \n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"neighboring nodes of the hypercube.  The methods in this program are \n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"applicable to mapping any two-dimensional grid, such as a chessboard or a\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"terrain database in a wargame, to the hypercube.  Please analyze my source\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"code for those methods.  \n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"    The sectors are named \"sector_ii_jj\" where ii and jj vary from 0 to\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"MAX_I - 1 and MAX_J - 1.  ii denotes the row number of a sector and jj denotes\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"the column.  To use this program, you will be asked to input MAX_II and MAX_JJ,\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"and then sit back and watch the node numbers get printed out.\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"    To save the output of this program, don't forget you can use the '>'\n") ;
    fprintf (stderr,"\n") ;
    fprintf (stderr,"operator of the C shell.\n") ;
    fprintf (stderr,"\n") ;
    {
	int max_ii, max_jj ;

	int hyper_i , hyper_j ;

	register int i, j, k ;

	int num [2] ;  /*** see gridmap.c/gridinit() in this directory for 
			    the docs for this parameter 
			***/


top:
	fprintf (stderr,"*********************************************************************\n") ;

	fprintf (stderr,"    Please enter the number of rows of sectors :  \n") ;
	scanf ("%d", & max_ii) ;
	fprintf (stderr,"        OK, I got MAX_II = %d\n", max_ii) ;

	fprintf (stderr,"    Please enter the number of columns of sectors :  \n") ;
	scanf ("%d", & max_jj) ;
	fprintf (stderr,"        OK, I got MAX_JJ = %d\n", max_jj) ;
	fprintf (stderr,"*********************************************************************\n") ;

	num [0] = hyper_i = max_ii < 4 ? max_ii : 4 ;
	num [1] = hyper_j = max_jj < 8 ? max_jj : 8 ;

	fprintf (stderr,"OK, calling gridinit\n") ;

	check (    gridinit ( 2, num )    ) ;

	{
	    int NodeNum [4] [8] ;
	    int num [2] ;

	    for (i=0; i<hyper_i; i++)
	    {
		for (j=0; j<hyper_j; j++)
		{
		    num [0] = i ;
		    num [1] = j ;

		    check (   k = gridproc (num)   ) ;

		    fprintf (stderr, 
		    "stderr: The Node Num for (%2d,%2d) is %2d\n",i,j,k) ;
		    
		    NodeNum [i] [j] = k ;
		}
	    }

	    for (i = 0 ;  i < max_ii ;  i++) 
	    {
		for (j = 0 ;  j < max_jj ;  j++)
		{
		    k = NodeNum [ (i*hyper_i)/max_ii ] [ (j*hyper_j)/max_jj ] ;

		    printf ("sector_%02.2d_%02.2d\t%2d\n", i, j, k) ;
		}
	    }
	}

	/***
	{
	    int Cartesian [32] [2] ;

	    for (i = 0 ;  i < 32 ;  i++)
	    {
		check (   gridcoord (i,Cartesian[i])   ) ;

		fprintf (stderr,"The BIG coordinates of node %d are (%4d,%4d)\n", 
		    i, Cartesian[i][0], Cartesian[i][1] ) ;
	    }
	}
	***/

    }
    fflush (stdout) ;

    goto top ;
	
}

check (r)
int r ;
{
    if ( r == -1 ) 
    {
	fprintf (stderr,"Sorry, the call to gridmap.c failed -- bye!\n") ;
	exit (0) ;
    }
}
