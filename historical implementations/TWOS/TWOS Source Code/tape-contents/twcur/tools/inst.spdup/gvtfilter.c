/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/



#define MAXINPUTLENGTH	128
#define MAXRECORDS	120000

#define POSINF 0x3FFFFFFFL              /* positive infinity for virtual time */
#define NEGINF (int) 0xc0000000L        /* negative infinity for virtual time */
#define ZERO   0x00000000L              /* zero for virtual time        */

#include <stdio.h>

typedef struct {
	int	node ;
	int	sequence ;
	double	realTime ;
	long	virtTime ;
	} LineRecordType ;

/* declare this here, statically.  We run out of
 * stack space otherwise.
 */
LineRecordType	*recList[MAXRECORDS] ;

int	compareLines(a, b)
/* Performs a comparison of two line records, using GVT
 * as the primary field and, if that is equal, comparing
 * on the real time field.
 */
LineRecordType	*a ;
LineRecordType	*b ;
{
  int	retVal ;

  retVal = 0 ;

  if (retVal == 0)
  {
	if (a->virtTime < b->virtTime)
		retVal-- ;
	else
		if (a->virtTime > b->virtTime)
			retVal++ ;
  }

  if (retVal == 0)
  {
	if (a->realTime < b->realTime)
		retVal-- ;
	else
		if (a->realTime > b->realTime)
			retVal++ ;
  }

  return(retVal);
}

main(argc, argv)
int	argc ;
char *argv[] ;
{

FILE		*streamIn ;
FILE		*streamOut ;
char		string[MAXINPUTLENGTH+1] ;
char		*progname ;
double		lastRT ;
int		lastSeq ;
register int	j ;
int		i ;
int		first ;
int		flag ;
int		noSeqRepeat ;
int		numRecords ;
long		minVT ;
long		maxVT ;
LineRecordType	*currentLine ;
LineRecordType	*greatestRT ;
LineRecordType	*tempRecPtr ;
LineRecordType	*malloc() ;

	/* For now, we take use standard input and output */
	streamIn = stdin ;
	streamOut = stdout ;

        /* Get the name of the program */
        progname = argv[0] + strlen(argv[0]) - 1;
        while (*(progname-1) != '/' && progname > argv[0])
                progname-- ;
	
	first = 1 ;

	noSeqRepeat = 0 ;

	if (argc == 2 && ! strcmp(argv[1], "-n"))
		noSeqRepeat = 1 ;
	else if (argc > 1)
	{
		fprintf(stderr, "usage:\t%s [-n]\n", progname) ; 
		fprintf(stderr,
			"\t\tThe -n option causes records with duplicate\n");
		fprintf(stderr,
			"\t\tsequence numbers to be eliminated. Input is read\n");
		fprintf(stderr,
			"\t\tfrom standard input, and output is sent to\n");
		fprintf(stderr,
			"\t\tstandard output.\n") ; 

		exit(-1) ;
	}

	j = 0;

	if ( (recList[j] = malloc(sizeof(LineRecordType))) == NULL)	{
		fprintf(stderr, "%s:\t couldn't allocate storage\n", progname);
		exit(-4);
	}

	minVT = POSINF ;
	maxVT = NEGINF ;

	/* Read in the lines of records */
	while(fgets(string, MAXINPUTLENGTH, streamIn) != 0)
		if(sscanf(	string,
				"%d %d %lf %lf",
				&(recList[j]->node),
				&(recList[j]->sequence),
				&(recList[j]->realTime),
				&(recList[j]->virtTime) ) == 4)
		{
			if (recList[j]->virtTime < POSINF &&
			    recList[j]->virtTime > maxVT)
				maxVT = recList[j]->virtTime ;

			if (recList[j]->virtTime > NEGINF &&
			    recList[j]->virtTime < minVT)
				minVT = recList[j]->virtTime ;

			if ( (recList[++j] =
				malloc(sizeof(LineRecordType))) == NULL)
			{
				fprintf(stderr,
					"%s:\t couldn't allocate storage\n",
					progname);
				exit(-4);
			}
		}

	numRecords = j ;

	minVT --;
	maxVT ++;

	fprintf(stderr, "%s: VT range is [%ld, %ld]\n", progname, minVT, maxVT) ;

	/* replace NEGINF and POSINF with real bounding values */
	for (j = 0; j < numRecords; j++)
		if (recList[j]->virtTime >= POSINF)
			recList[j]->virtTime  = maxVT ;
		else
			if (recList[j]->virtTime <= NEGINF)
				recList[j]->virtTime = minVT ;

	/* sort the records by virtual time */
	for (i = 0; i < numRecords; i++)
	{
		flag = 1 ;

		for (j = numRecords - 2; j >= i; j--)
			if (compareLines(recList[j], recList[j+1]) > 0)
			{
				/* swap the records */
				tempRecPtr = recList[j];
				recList[j] = recList[j+1];
				recList[j+1] = tempRecPtr ;

				/* and note that we swapped */
				flag = 0;
			}

		if (flag)
			break ;

	}

	/* print a header line */
	fprintf(streamOut,
		"%4s\t%5s\t%9s\t%11s\n",
		"Node",
		"Seq",
		"Real Time",
		"GVT" );


	/* remove points which fall behind the line of GVT */
	for (i = 0; i < numRecords; i++)
	{
		currentLine = recList[i];

		if (first)
		{
			first = 0 ;
			lastRT = currentLine->realTime ;
			lastSeq = currentLine->sequence ;
			greatestRT = currentLine ;

			continue ;
		}

		if (currentLine->virtTime > greatestRT->virtTime)
		{
			if (	(lastRT < greatestRT->realTime) &&
				(! noSeqRepeat || lastSeq != greatestRT->sequence))
			{
				fprintf(streamOut,
					"%4d\t%5d\t%9.4lf\t%11ld\n",
					greatestRT->node,
					greatestRT->sequence,
					greatestRT->realTime,
					greatestRT->virtTime) ;

				lastRT  = greatestRT->realTime ;
				lastSeq = greatestRT->sequence ;
			}

			greatestRT = currentLine ;

			continue ;
		}

		if (currentLine->realTime > greatestRT->realTime)
		{
			greatestRT = currentLine ;

			continue ;
		}

	}
}
