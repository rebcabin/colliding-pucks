#include <stdio.h>
/* Modified by Brian Beckman, 4 Sept 1987, for use in generating
   config files for Time Warp pool balls demo
   */

/*** #include <cros.h> ***/

static struct cubenv
{
    int doc ;
    int procnum ;
    int nproc ;
    int cpmask ;
    int cubemake ;
}
    env ;

#define MAXDIM 5
#define mxbitsperinT 8		/* this is presumably a procnum   */



static int      mask[16] = 
{   
    0x0001, 0x0002, 0x0004, 0x0008,
    0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800,
    0x1000, 0x2000, 0x4000, 0x8000
};

/******************************************************/
int             graytobin (g)
    int             g;
{
    int             i,
                    n;

    n = mask[14] & g;
    for (i = 13; i >= 0; --i)
	n += (mask[i] & g) ^ ((mask[i + 1] & n) >> 1);
    return (n);
}

/******************************************************/
int             bintogray (n)
    int             n;
{
    int             temp;

    temp = n << 1;
    return ((temp ^ n) >> 1);
}

/******************************************************/
int             int_log2 (n)
    unsigned int    n;
{
    int             i;

    for (i = 0; i < 16; ++i)
    {
	if (((n >> i) & 1) == 1)
	{
	    if ((n >> i) == 1)
	    {
		return (i);
	    }
	    else
	    {
		fprintf (stderr,"Returning -1 from int_log\n") ;
		return (-1);
	    }
	}
    }
}

static int      ginit = 0;	/* indicates whether gridinit called */
static int      gdim;		/* keep a copy of dim */
static int      gnum[MAXDIM];	/* keep a copy of array num */
static int      goffset[MAXDIM];/* offset of first bit in procnum */
static int      gmask[MAXDIM];	/* mask of bits for each dimension */
static int      grem;		/* mask of bits not used in topology */

/******************************************************/
int             gridinit (dim, num)
    int             dim;	/* dimension of decomposition topology */
    int            *num;	/* number of nodes in each dimension */
{
    int             i,
                    n,
                    offset,
                    Lwstpwr,
                    Lwstpwrmask,
                    Mask = 1,
                    doc;
    /*** struct cubenv   env; ***/

    /*** cparam (&env); ***/

    env.doc = 5 ;
    env.nproc = 32 ;

    doc = env.doc;
    ginit = 1;			/* indicate that gridinit called */
    n = 1;			/* keep count of total nodes used */
    offset = 0;

    if (dim > MAXDIM)
    {
	fprintf (stderr,"MAXDIM error in gridinit\n") ;
	return (-1);		/* check if dim is too large */
    }

    gdim = dim;			/* save dim */

    for (i = 0; i < dim; ++i)
    {
	goffset[i] = offset;		/* save current offset */
	Lwstpwr = LWSTNMBITS (num[i]);
	offset += Lwstpwr;		/* incrmt offset by # bits in dim i */
	Lwstpwrmask = Mask << Lwstpwr;
	gmask[i] = Lwstpwrmask - 1;	/* mask of bits needed by dimension i */
	gnum[i] = Lwstpwrmask;		/* save # of nodes in dimension i */
	n *= Lwstpwrmask;		/* update total number of node used */
    }

    if (n >> doc + 1)
    {
	fprintf (stderr,"doc error in gridinit\n") ;
	return (-1);		/* check that enough nodes are available */
    }

    grem = ((1 << (doc - offset)) - 1) << offset;	
				/* mask of bits not used */

    return (0);			/* no error - return 0 */
}

/******************************************************/
int             gridcoord (proc, coord)
    int             proc;
    int            *coord;
{
    int             i,
                    temp;

    if (ginit == 0)
	return (-1);

    if ((proc & grem) != 0)
	return (-1);

    for (i = 0; i < gdim; ++i)
    {
	temp = (proc >> goffset[i]) & gmask[i];

	coord[i] = graytobin (temp);
    }

    return (0);
}

/******************************************************/
int             gridproc (coord)
    int            *coord;
{
    int             i,
                    proc;

    if (ginit == 0)
    {
	fprintf (stderr,"ginit error in gridproc\n") ;
	return (-1);
    }

    proc = 0;

    for (i = 0; i < gdim; ++i)
    {
	if (coord[i] > gnum[i])
	{
	    fprintf (stderr,"coord gnum error in gridproc\n") ;
	    return (-1);
	}

	if (coord[i] < 0)
	{
	    fprintf (stderr,"coord < 0 error in gridproc\n") ;
	    return (-1);
	}

	proc |= bintogray (coord[i]) << goffset[i];
    }
    return (proc);
}

/******************************************************/
int             gridchan (proc, dim, dir)
    int             proc;
    int             dim;	/* this is prolly the man page  dir   arg  */
    int             dir;	/* this is prolly the man page  sign  arg  */
{
    int             temp,
                    coord[MAXDIM];


    if (ginit == 0)
	return (-1);

    if ((dir != 1) && (dir != -1))
	return (-1);		/* sign must be +/- 1 */

    if ((dim < 0) || (dim >= gdim))
	return (-1);

    if (gridcoord (proc, coord) == -1)
	return (-1);

    coord[dim] += dir;

    if (coord[dim] < 0)
	coord[dim] = gnum[dim] - 1;

    if (coord[dim] >= gnum[dim])
	coord[dim] = 0;

    if ((temp = gridproc (coord)) == -1)
	return (-1);

    return (proc ^ temp);	/* xor two proc numbers gives chan mask  */
}


/******************************************************/
LWSTNMBITS (Nmbr)
    int             Nmbr;

{
    int             Mask = 1,
                    Log2n;
    int             Lwrbitsareon;


    switch (Nmbr)
    {
    case 0:
	return (0);
    case 1:
	return (1);
    default:

	Lwrbitsareon = 0;
	for (Log2n = 0; Log2n < mxbitsperinT; Log2n++)
	{
	    if ((Nmbr >> Log2n) & Mask)
	    {
		if ((Nmbr >> Log2n) == 1)
		{
		    if (Lwrbitsareon)
			return (Log2n + 1);
		    else
			return (Log2n);
		}
		Lwrbitsareon = 1;
	    }
	}			/* for loop */
    }
}
