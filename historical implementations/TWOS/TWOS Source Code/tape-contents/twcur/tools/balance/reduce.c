/*	Copyright (C) 1989, 1991, California Institute of Technology.
        	U. S. Government Sponsorship under NASA Contract NAS7-918 
                is acknowledged.	*/


/* 	This program reduces the trace file produced by the
	simulator (-s option) to a summary form which is used
	by the configuration balancing program "balt".

	Link with "sortarrayd".
*/

/* updated 3/21/90 by JJW to support the new simulator format for the
   object statistics. The new format is the same as that of XL_STATS  */

/* Implementation:

	The references to XL_STATS refer to the simulator version which is
    defined in the statsio.c file of the timewarp simulator.

	The stats struct is used in XL_STATS to define the fields. This struct
    is identical to the same struct in the timewarp Ocb and is copied from
    twsys.h.  The fields in the file are scanned with an sscanf argument list
    modified from the statsio..c file as necessary to convert an sprintf into an
    sscanf. Program has been changed to support 1 argument with XL_STATS as the
    default input or the previous 2 argument format. Program not altered except
    for reading the stats file and the above default arg change. */

#include <stdio.h>

#define MAXOBJ 1000		/* maximum number of objects */
#define MSGTIME 0.0015		/* microseconds to allow for receipt of msg */

#define BUFFERLEN	256
/* the above is same as SYSDEFSIZE in twsys.h */
#define NAMELEN		31
struct
{
    char name[NAMELEN+1];	/* Object name */
    double time;		/* Total time for this object */
    double weight;		/* Total "weight" for this object */
    int    imsg;		/* Total incoming messages */
    int    events;		/* number of events for this object */
    int    node;		/* eventual node assignment */
}
    object[MAXOBJ];

int	map[MAXOBJ];
double	val[MAXOBJ];

char *progname;

/* struct copied from Ocb in twsys.h */
struct stats_s {

        long     numestart;     /* Number of events which started     */
        long     numecomp;      /* Number of events which ended       */
 
        long     numcreate;     /* Number of times object created          */
        long     numdestroy;    /* Number of times object destroyed        */
        long     ccrmsgs;       /* Number of committed creates             */
        long     cdsmsgs;       /* Number of committed destroys            */
 
        long     cemsgs;        /* Committed event messages                */
        long     cebdls;        /* Committed event bundles                 */
                                /* BUNDLE = group of messages at same time */
                                /*                                         */
        long     coemsgs;       /* Committed OUTPUT event messages        */
        long     coebdls;       /* Committed OUTPUT event bundles         */
                                /* BUNDLE = group of messages at same time */
                                /*                                         */
        long     nssave;        /* number of states saved                  */
        long     nscom;         /* number of states committed              */
 
        long     eposfs;        /* number of Events, POSitive Forward Sent */
        long     eposfr;        /* number of Events, POSitive Forward Rec'd*/
       
        long     enegfs;        /* number of Events, NEGative Forward Sent */
        long     enegfr;        /* number of Events, NEGative Forward Rec'd */
 
        long     ezaps;         /* number of event annihilations           */
 
        long     evtmsg;        /* number of times evtmsg called           */

        long    eposrs;         /* number of Events, Positive Reverse Sent */
        long    eposrr;         /* number of Events, Positive Reverse Rec'd*/

        long    enegrs;         /* number of Events, Negative Reverse Sent */
        long    enegrr;         /* number of Events, Negative Reverse Rec'd*/

        long    cputime;        /* accumulated cpu time                    */

        long    rbtime;         /* amount of work rolled back              */
        long    cycletime;      /* effective utilization                   */
        float   utilization;    /* last effective utilization calculated   */
        long    nummigr;        /* Number of times object migrated         */
        long    numstmigr;      /* Number of states migrated               */
        long    numimmigr;      /* Number of input messages migrated       */
        long    numommigr;      /* Number of output messages migrated      */
        long    sqlen;          /* Average length of state queue           */
        long    iqlen;          /* Average length of input queue           */
        long    oqlen;          /* Average length of output queue          */
        long    sqmax;          /* Max length of state queue at GVT        */
        long    iqmax;          /* Max length of input queue at GVT        */
        long    oqmax;          /* Max length of output queue at GVT       */
        long    stforw;         /* # of states forwarded                   */
  } stats;

/* struct for this program */
struct  fake_Ocb {
	int   node;	/* note there is also object.node */
	char  name[NAMELEN];
	struct stats_s  stats;
	} Ocb;


main ( argc, argv )

    int argc;
    char *argv[];
{
    int i, j, n;
    int numobj;
    int numevents;
    FILE *fp, *fpo;
    int last;
    char buff[BUFFERLEN];
    double time;
    double cumobjtime;
    double total;
    char name[NAMELEN+1];
    struct fake_Ocb *o = &Ocb;

    progname = argv[0];

    if ( argc != 3  &&  argc != 2)
    {
	printf ( "usage: %s statisticsfile outfile\n", progname );
	printf ( "or %s outfile  - to input from XL_STATS\n", progname);
	printf ( "(use the -s option on the simulator to generate the XL_STATS file)\n" );
	exit (0);
    }
    if (argc == 3)
       {
	fp = fopen ( argv[1], "r" );

        if ( fp == 0 )
         {
	  fprintf ( stderr, "%s: File %s not found\n", progname, argv[1] );
	  exit (0);
         }
       }
    else
       {
        fp = fopen ( "XL_STATS" , "r" );
        if ( fp == 0 )
         {
	  fprintf ( stderr, "%s: XL_STATS not found\n", progname );
	  exit (0);
         }
       }


    /* No objects in the list yet. */
    numobj = 0;
    total  = 0.0;

    for ( j = 0; fgets ( buff, BUFFERLEN, fp ); j++)
    {

	/* Make sure we don't get complete garbage */
        buff[BUFFERLEN-1] = '\0';


	n = sscanf ( buff,
	     "%d%s%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%f",
	&o->node, o->name, &o->stats.eposfs,
	&o->stats.enegfs, &o->stats.eposfr, &o->stats.enegfr, &o->stats.cemsgs,
	&o->stats.numestart, &o->stats.numecomp, &o->stats.cebdls, &o->stats.ezaps,
	&o->stats.nssave, &o->stats.nscom, &o->stats.evtmsg,
	&o->stats.eposrs, &o->stats.enegrs, &o->stats.eposrr, &o->stats.enegrr,
	&o->stats.numcreate, &o->stats.numdestroy,
	&o->stats.ccrmsgs, &o->stats.cdsmsgs,
	&o->stats.nummigr, &o->stats.numstmigr, &o->stats.numimmigr,
	&o->stats.numommigr, &o->stats.sqlen, &o->stats.iqlen, &o->stats.oqlen,
	&o->stats.sqmax, &o->stats.iqmax, &o->stats.oqmax,
/* the cpu time is already in units of seconds in the file */
	&o->stats.cputime );


	if ( n == 33)
        {
              /* Add another object to list */
              strcpy (object[numobj].name, o->name);
              object[numobj].time = o->stats.cputime;
              object[numobj].events = o->stats.numestart;
              object[numobj].weight =  o->stats.cputime + (MSGTIME * o->stats.numestart);

              /* set up mapping array */
              map[numobj] = numobj;
              val[numobj] = object[numobj].weight;

              total = total + object[numobj].weight;

	      numobj++;
        }
    }

    printf("%s: %d objects read.\n", progname, numobj);

    fclose ( fp );

    /* sort the mapping array */
    sortarrayd( val,  map, numobj);

    if (argc == 3)
      fpo = fopen ( argv[2], "w" );
    else
      fpo = fopen ( argv[1], "w" );

    sprintf(buff, "Total Objects:\t\t%4d\n", numobj);
    fputs( buff, fpo );
    sprintf(buff, "Total Weight:\t%6.4f\n\n", total);
    fputs( buff, fpo );

    /* A header for human eyes */
    sprintf(buff, "%-10s\t%10s\n",
                  "Name","% Weight");
    fputs( buff, fpo );

    for (j = 0; j < numobj; j++)
    {
        i = map[j];
        sprintf(buff, "%-10s\t%10.8lf\n",
                object[i].name,
                object[i].weight * 100 / total);

        fputs( buff, fpo );
    }

    fclose ( fpo );
}
