/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define MAXINPUTLENGTH	128

#define POSINF 0x3FFFFFFFL  		/* positive infinity for virtual time */
#define NEGINF (int) 0xc0000000L	/* negative infinity for virtual time */
#define ZERO   0x00000000L  		/* zero for virtual time	*/


typedef struct {
	double	rT ;
	long	vT ;
	} LineRecordType ;

double interpolate(dLow, dHigh, rLow, rHigh, probe)
long	dLow, dHigh ;
double 	rLow, rHigh ;
long  probe ;
{
double	range ;
double	ratio ;
double	result ;

	if (probe < dLow || probe > dHigh || dHigh == dLow)
		fprintf(stderr, "domain = [%ld,%ld], probe = %ld\n",
				dLow, dHigh, probe);
	range = rHigh - rLow;
	ratio = (probe - dLow) / (dHigh - dLow);
	result = rLow + (range * ratio) ;

	return( result );
}

/* Reads one record out of the specified stream.  Will
 * return 1 if a record was read or a 0 otherwise.  A
 * 0 signals EOF.  Positive and negative infinity are
 * ignored.
 */
int readRecord( streamIn, rec )
FILE		*streamIn ;
LineRecordType	*rec ;
{
	char	string[MAXINPUTLENGTH] ;
	int	found ;

	found = 0 ;

	while(  ! found && fgets(string, MAXINPUTLENGTH, streamIn) != 0)
	{
		if(sscanf(string, "%*d %*d %lf %lf", &(rec->rT), &(rec->vT) ) == 2)
		{
			if (rec->vT > NEGINF &&
			    rec->vT < POSINF)
				found++ ;
		}
	}

	return(found) ;
}


main( argc, argv )
int argc ;
char *argv[] ;
{

char		string[MAXINPUTLENGTH+1] ;
char 		*progname ;
char 		*fileNameTW ;
char 		*fileNameSS ;
double		simRT ;
double		deltaGVT ;
double		dSdGVT ;
double		dTdGVT ;
double		iSpeed ;
double		prevRTSS ;
double		prevRTTW ;
FILE		*streamTW ;
FILE		*streamSS ;
FILE		*streamOut ;
int		first ;
long		prevVT ;
long		clock ;
LineRecordType	timeTW ;
LineRecordType	lowTimeSS ;
LineRecordType	highTimeSS ;
LineRecordType	*malloc() ;
struct timeb	tp ;


	/* Get the name of the program */
	progname = argv[0] + strlen(argv[0]) - 1;
	while (*(progname-1) != '/' && progname > argv[0])
		progname-- ;

	/* see if our arguments are reasonable */
	if (argc != 3)
	{
		fprintf(stderr,
			"usage:\t%s <TWDataFile> <SSDataFile>\n",
			progname);
		exit(0);
	}

	/* open the file of TimeWarp times */
	fileNameTW = argv[1] ;
	if ((streamTW = fopen(fileNameTW, "r")) == 0)
	{
		fprintf(stderr,
			"%s:\tcould not open TW data file, %s\n",
			progname,
			fileNameTW);
		exit(-1) ;
	}

	/* open the file of Sequential Simulator times */
	fileNameSS = argv[2] ;
	if ((streamSS = fopen(fileNameSS, "r")) == 0)
	{
		fprintf(stderr,
			"%s:\tcould not open TW data file, %s\n",
			progname,
			fileNameSS);
		exit(-1) ;
	}

	/* for now, we write to standard output */
	streamOut = stdout ;

	/* Get the first SS Time */
	if ( readRecord(streamSS, &lowTimeSS) == 0)
	{
		fprintf(stderr,
			"%s:\tCouldn't read SS data from %s\n",
			progname,
			fileNameSS) ;
		exit(-1);
	}

	/* Get the second SS Time */
	if ( readRecord(streamSS, &highTimeSS) == 0)
	{
		fprintf(stderr,
			"%s:\tCouldn't read SS data from %s\n",
			progname,
			fileNameSS) ;
		exit(-1);
	}


	first = 1;

	while (readRecord(streamTW, &timeTW) != 0)
	{
		/* It's always possible that TW will have
		 * a lower time than the SS in the beginning
		 * of the simulation. After that, though, this
		 * should never happen.
		 */
		if (timeTW.vT < lowTimeSS.vT)
			continue ;

		while (timeTW.vT > highTimeSS.vT)
		{
			memcpy(	&lowTimeSS,
				&highTimeSS,
				sizeof(LineRecordType) ) ;

			if( readRecord(streamSS, &highTimeSS) == 0)
				break;
		}

		if (	lowTimeSS.vT <= timeTW.vT &&
			timeTW.vT <= highTimeSS.vT)
		{
			simRT = interpolate(	lowTimeSS.vT,
						highTimeSS.vT,
						lowTimeSS.rT,
						highTimeSS.rT,
						timeTW.vT );

			if (first)
			{
				first = 0 ;

				ftime(&tp) ;
				clock = tp.time ;

				fprintf(streamOut,
					"\nProduced %s",
					ctime(&clock) );

				fprintf(streamOut,
					"TimeWarp File: %s\t\tSimulator File: %s\n\n",
					fileNameTW, 
					fileNameSS );

				fprintf(streamOut,
					"%11s\t%7s\t%7s\t%7s\t%7s\n",
					"GVT",
					"I(g)",
					"t(g)",
					"s(g)",
					"A(g)");
			}
			else
			{
				deltaGVT = (timeTW.vT - prevVT);

				dSdGVT = (simRT - prevRTSS) / deltaGVT ;

				dTdGVT = (timeTW.rT - prevRTTW) / deltaGVT ;

				iSpeed = dSdGVT / dTdGVT ;

				fprintf(streamOut,
					"%11ld\t%7.3f\t%7.2f\t%7.2f\t%7.3f\n",
					prevVT,
					iSpeed,
					prevRTTW,
					prevRTSS,
					(prevRTSS / prevRTTW) /* A(g) */ );
			}

			prevRTSS = simRT ;
			prevRTTW = timeTW.rT ;
			prevVT   = timeTW.vT ;

		}
	}
}
