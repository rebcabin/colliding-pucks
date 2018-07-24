/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

#include <stdio.h>

main ( argc, argv )

    int argc;
    char * argv[];
{
    int node, eposfs, enegfs, eposfr, enegfr, cemsgs,
    numestart, numecomp, cebdls, ezaps,
    nssave, nscom, tells, eposrs,
    enegrs, eposrr, enegrr,
    phase_begin,
    numcreate, numdestroy, ccrmsgs, cdsmsgs,
    nummigr, numstmigr, numimmigr, numommigr, pfaults;
    double comtime;
    double cputime;
    int config_eposfs;
    char config_file[60];

    char object[20];
	char objtmp[20], maxobj[20];

    static int num, tot_eposfs, tot_enegfs, tot_eposfr, tot_enegfr, tot_cemsgs,
    tot_numestart, tot_numecomp, tot_cebdls, tot_ezaps, tot_numqstart,
    tot_nssave, tot_nscom, tot_tells,
    tot_eposrs, tot_enegrs, tot_eposrr, tot_enegrr,
    tot_numcreate, tot_numdestroy, tot_ccrmsgs, tot_cdsmsgs,
    tot_sqlen, tot_iqlen, tot_oqlen, tot_asqlen, tot_aiqlen, tot_aoqlen,
    tot_nummigr, tot_numstmigr,tot_numimmigr,tot_numommigr,
    tot_pfaults, max_pfaults,
    sqmax_ov, iqmax_ov, oqmax_ov;
    static double tot_cputime, aver_pfaults;
    static int tot_stforw;

    static double tot_comtime;

    static int cprobe, chit, cmiss, tot_cprobe, tot_chit, tot_cmiss, nodetmp;
	static int epttmp, maxept = -1;
    static int sqlen,iqlen,oqlen;
    static int sqmax, iqmax, oqmax;
    static int migrin, migrout, tot_migrin, tot_migrout;
    static int stforw;
    static double HitRatio;

    double elapsed_time;
    static int high_node;

    static int slept, tot_slept;
    static int jumpForw, tot_jumpForw;
    static int tot_phaseNaksSent, phaseNaksSent, tot_phaseNaksRecv, 
	phaseNaksRecv, tot_stateNaksSent, stateNaksSent, tot_stateNaksRecv,
	 stateNaksRecv;

    char line[500];
    char * filename, * mfile;
    int n, ok;
    static char s[2];
    static char version[20];
    FILE * fp, * out = NULL, * appendit;
	float critlength;

    if ( argc == 1 )
	filename = "XL_STATS";
    else
	filename = argv[1];

    fp = fopen ( filename, "r" );

    if ( fp == 0 )
    {
	printf ( "File %s Not Found\n", filename );
	exit (0);
    }

if ( ( n = strlen ( argv[0] ) ) >= 7
&&   strcmp ( argv[0]+n-7, "measure" ) == 0 )
{
    if ( argc <= 2 )
	mfile = "Measurements";
    else
	mfile = argv[2];

    appendit = fopen ( mfile, "r" );

    if ( appendit != NULL )
	fclose ( appendit );

    out = fopen ( mfile, "a" );

    if ( out == NULL )
    {
	printf ( "Can't Open File %s\n", mfile );
	exit (0);
    }
}

#ifdef DETAIL
	printf (
"\n num object       eposfr enegfr cemsgs estart  ecomp cebdls ezaps nssave  nscom\n\n" );
#endif

#ifdef COMMIT
	printf (
"\n object     cemsgs cebdls nscom\n\n" );
#endif

    while ( fgets ( line, 500, fp ) )
    {
	n = sscanf ( line, "%d %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lf  %lf",
	    &node, object, &eposfs, &enegfs, &eposfr, &enegfr,
	    &cemsgs, &numestart, &numecomp, &cebdls, &ezaps,
	    &nssave, &nscom,
	    &tells, &eposrs,
	    &enegrs, &eposrr, &enegrr,
	    &numcreate, &numdestroy, &ccrmsgs, &cdsmsgs,
	    &nummigr, &numstmigr, &numimmigr, &numommigr,
	    &sqlen, &iqlen, &oqlen,
	    &sqmax, &iqmax, &oqmax,
	    &stforw,
	    &comtime,
	    &cputime );
/* this does not seem close to correct
	if ( n == 32 )
	{
	n = sscanf ( line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lf  %lf",
	    &node, &eposfs, &enegfs, &eposfr, &enegfr,
	    &cemsgs, &numestart, &numecomp, &cebdls, &ezaps,
	    &nssave, &nscom,
	    &tells, &eposrs,
	    &enegrs, &eposrr, &enegrr,
	    &numcreate, &numdestroy, &ccrmsgs, &cdsmsgs,
	    &nummigr, &numstmigr, &numimmigr, &numommigr,
	    &sqlen, &iqlen, &oqlen,
	    &sqmax, &iqmax, &oqmax, 
	    &comtime,
	    &cputime );
	    object[0] = 0;
	}
	else
*/
	if ( n < 34 )
	{
	    n = sscanf ( line, "Version %s", version );

	    if ( n == 1 )
		continue;

	    n = sscanf ( line, "Config File %s", config_file );

	    if ( n == 1 )
		continue;

	    n = sscanf ( line, "Elapsed time for this run was %lf",
					&elapsed_time );
	    if ( n == 1 )
		continue;

	    n = sscanf ( line, "Config eposfs %d", &config_eposfs );

	    if ( n == 1 )
	    {
		tot_eposfs += config_eposfs;
		continue;
	    }

	    n = sscanf ( line, "Node %d: Cache Probes %d, Cache Hits %d, Cache Misses %d Hit Ratio %lf",
		&nodetmp, &cprobe, &chit, &cmiss, &HitRatio );

	    if ( n == 5 )
	    {
		tot_cprobe += cprobe;
		tot_chit += chit;
		tot_cmiss += cmiss;
		continue;
            }

	    n = sscanf ( line,"Foreign node went to sleep %d times\n", &slept );

	    if ( n == 1 )
	    {
		tot_slept += slept;
		continue;
	    }

	    n = sscanf ( line,"Limited jump forward saved %d state transmissions\n",
		&jumpForw );

	    if ( n == 1 )
	    {
		tot_jumpForw += jumpForw;
		continue;
	    }
		
	    n = sscanf ( line,"Migration Naks: %d phase sent, %d phase recv, %d state sent, %d state recv\n", 
	       &phaseNaksSent, &phaseNaksRecv, &stateNaksSent, &stateNaksRecv );

	    if ( n == 4 )
	    {
		tot_phaseNaksSent += phaseNaksSent;
		tot_phaseNaksRecv += phaseNaksRecv;
		tot_stateNaksSent += stateNaksSent;
		tot_stateNaksRecv += stateNaksRecv;
		continue;
	    }

	    n = sscanf ( line, "Migration Count Node %d: Migrations In %d, Migrations Out %d\n",
		&nodetmp, &migrin, &migrout );

	    if ( n == 3 )
	    {
		tot_migrin += migrin;
		tot_migrout += migrout;
		continue;
	    }

    	n = sscanf ( line, "Page Fault Node %d: %d\n", &nodetmp, &pfaults );

	    if ( n == 2 )
	    {
		tot_pfaults += pfaults;
		max_pfaults = (pfaults > max_pfaults) ? pfaults : max_pfaults;
		continue;
	    }

    	n = sscanf ( line, "Node %d: Max Local Time %d for Object %s\n",
				&nodetmp, &epttmp, objtmp );

		if ( n == 3 )
		{
			if ( epttmp > maxept )
			{
				maxept = epttmp;
				strcpy ( maxobj, objtmp );
			}
		continue;
		}

	    continue;
	}

	num++;
	tot_eposfs += eposfs;
	tot_enegfs += enegfs;
	tot_eposfr += eposfr;
	tot_enegfr += enegfr;
	tot_cemsgs += cemsgs;
	tot_numestart += numestart;
	tot_numecomp += numecomp;
	tot_cebdls += cebdls;
	tot_ezaps += ezaps;
	tot_nssave += nssave;
	tot_nscom += nscom;
	tot_tells += tells;
	tot_eposrs += eposrs;
	tot_enegrs += enegrs;
	tot_eposrr += eposrr;
	tot_enegrr += enegrr;
	tot_numcreate += numcreate ;
	tot_numdestroy += numdestroy;
	tot_ccrmsgs += ccrmsgs;
	tot_cdsmsgs += cdsmsgs;
	if (sqmax > sqmax_ov)
	    sqmax_ov = sqmax;
	if (iqmax > iqmax_ov)
	    iqmax_ov = iqmax;
	if (oqmax > oqmax_ov)
	    oqmax_ov = oqmax;
	tot_nummigr += nummigr;
	tot_numstmigr += numstmigr;
	tot_numimmigr += numimmigr;
	tot_numommigr += numommigr;
	tot_sqlen += sqlen;
	tot_iqlen += iqlen;
	tot_oqlen += oqlen;
	if (sqlen > 1)
	    tot_asqlen += sqlen;
	if (iqlen > 0)
	    tot_aiqlen += iqlen;
	if (oqlen > 0)
	    tot_aoqlen += oqlen;
	tot_stforw += stforw;
	tot_cputime += cputime;
	tot_comtime += comtime;

	if ( high_node < node )
	    high_node = node;

#ifdef DETAIL
	printf (
" %3d %-12s %6d %6d %6d %6d %6d %6d %5d %6d %6d\n",
num, object, eposfr, enegfr, cemsgs, numestart, numecomp, cebdls, ezaps, nssave, nscom );
#endif

#ifdef COMMIT
	printf (
"%-10s %6d %6d %6d\n",
object, cemsgs, cebdls, nscom );
#endif
    }

#ifdef COMMIT
    exit (0);
#endif

    printf ( "\n Config File: %s\n", config_file );

	printf (
"\n num filename     eposfr enegfr cemsgs estart  ecomp cebdls ezaps nssave  nscom\n\n" );

    printf (
" %3d %-12s %6d %6d %6d %6d %6d %6d %5d %6d %6d\n\n",
tot_ccrmsgs, filename, tot_eposfr, tot_enegfr, tot_cemsgs, tot_numestart, tot_numecomp, tot_cebdls, tot_ezaps, tot_nssave, tot_nscom );

#define YES 1
#define NO  0

    ok = YES;

    if ( (n = (tot_eposfs - tot_eposfr)) == 0 )
	printf ( "     eposfs %d == eposfr %d\n", tot_eposfs, tot_eposfr );
    else
    {
	printf ( "  ** eposfs %d != eposfr %d diff = %d\n",
	    tot_eposfs, tot_eposfr, n );

	ok = NO;
    }
    if ( (n = (tot_enegfs - tot_enegfr)) == 0 )
	printf ( "     enegfs %d == enegfr %d\n", tot_enegfs, tot_enegfr );
    else
    {
	printf ( "  ** enegfs %d != enegfr %d diff = %d\n",
	    tot_enegfs, tot_enegfr, n );

	ok = NO;
    }
    if ( (n = (tot_enegfs - tot_enegrs)) == tot_ezaps )
	printf ( "     enegfs %d - enegrs %d == ezaps %d\n",
	    tot_enegfs, tot_enegrs, tot_ezaps );
    else
    {
	printf ( " ** enegfs %d - enegrs %d = %d != ezaps %d diff = %d\n",
	    tot_enegfs, tot_enegrs, n, tot_ezaps, n - tot_ezaps );

	ok = NO;
    }

  if ( tot_ccrmsgs > 0 || tot_cdsmsgs > 0 )
  {
    if ( (n = ((tot_cebdls + tot_ccrmsgs + tot_cdsmsgs) - tot_nscom)) == 0)
	printf ( "     cebdls %d + creates %d + destroys %d == nscom %d\n",
	    tot_cebdls, tot_ccrmsgs, tot_cdsmsgs, tot_nscom );
    else
    {
	printf ( "  ** cebdls %d + creates %d + destroys %d != nscom %d diff = %d\n",
	    tot_cebdls, tot_ccrmsgs, tot_cdsmsgs, tot_nscom, n );

	ok = NO;
    }
  }
  else
  {
    if ( (n = (tot_cebdls - tot_nscom)) == 0 )
	printf ( "     cebdls %d == nscom %d\n", tot_cebdls, tot_nscom );
    else
    {
	printf ( "  ** cebdls %d != nscom %d diff = %d\n",
	    tot_cebdls, tot_nscom, n );

	ok = NO;
    }
  }

if ( tot_eposrs != 0 || tot_enegrs != 0 )
{
    if ( (n = (tot_eposfr - tot_enegfr - tot_eposrs + tot_enegrs)) == tot_cemsgs )
	printf (
"\n     eposfr %d - enegfr %d - eposrs %d + enegrs %d == cemsgs %d\n",
tot_eposfr, tot_enegfr, tot_eposrs, tot_enegrs, tot_cemsgs );
    else
    {
	printf (
"\n  ** eposfr %d - enegfr %d - eposrs %d + enegrs %d = %d != cemsgs %d diff = %d\n",
tot_eposfr, tot_enegfr, tot_eposrs, tot_enegrs, n, tot_cemsgs, n - tot_cemsgs );
	ok = NO;
    }
}
else
{
    if ( (n = (tot_eposfr - tot_enegfr)) == tot_cemsgs )
	printf ( "     eposfr %d - enegfr %d == cemsgs %d\n",
	    tot_eposfr, tot_enegfr, tot_cemsgs );
    else
    {
	printf ( "  ** eposfr %d - enegfr %d = %d != cemsgs %d diff = %d\n",
	    tot_eposfr, tot_enegfr, n, tot_cemsgs, n - tot_cemsgs );
	ok = NO;
    }
}

if ( tot_migrin != 0 || tot_migrout != 0 || tot_nummigr != 0)
{
    if (tot_migrin == tot_migrout && tot_migrin == tot_nummigr)
    {
        printf("     tot_migrin %d == tot_migrout %d == tot_nummigr %d\n",
                tot_migrin,tot_migrout,tot_nummigr);
    }
    else
    {   
        if (tot_migrin != tot_migrout)
        {
            printf("     tot_migrin %d != tot_migrout %d diff = %d\n",
                        tot_migrin, tot_migrout, tot_migrin - tot_migrout);
        }
        else
        {
            printf("     tot_migrin %d != tot_nummigr %d diff = %d\n",
                        tot_migrin, tot_nummigr, tot_migrin - tot_nummigr);
        }
    }
}      

if ( tot_jumpForw != 0 )
{
	printf("	Limited jump forward avoided state transmission %d times\n",
		tot_jumpForw );
}

printf("     Max SQ Length %d, Max IQ Length %d, Max OQ Length %d\n",
                sqmax_ov, iqmax_ov, oqmax_ov);

    printf ( "\n" );

    if ( high_node > 0 )
	s[0] = 's';

    printf ( "     Timewarp Version %s Elapsed Time %.2f Seconds on %d Node%s\n\n",
	version, elapsed_time, high_node + 1, s );

	/* max ept is in microseconds */
	critlength = (float) maxept/1000000.;

    printf ( "     Total Object CPU Time %.2f Seconds\n\n", tot_cputime );
    printf ( "     Total Object COM Time %.2f Seconds\n", tot_comtime );
	if (maxept >= 0)
	  printf ( "	   Critical Path Length %.2f Seconds, Last Object %s\n\n",
					critlength, maxobj ); 

    if ( tot_slept )
    {
	printf ( "	*** Run Times Suspect - Some nodes went to sleep %d times\n",
		tot_slept);
    }

    if ( phaseNaksSent + phaseNaksRecv + stateNaksSent + stateNaksRecv )
    {
	printf ( "	*** Run Times Suspect - Migration Naks present\n");
	printf ( "	    phaseSent %d, phaseRecv %d, stateSent %d, stateRecv %d\n",
	      tot_phaseNaksSent, tot_phaseNaksRecv, tot_stateNaksSent, 
	      tot_stateNaksRecv );
    }

	aver_pfaults = (float) tot_pfaults / (float) (high_node + 1);
    printf ( "     Total Page Faults %d, Max %d, Average %.2f Faults per node\n",
	tot_pfaults, max_pfaults, aver_pfaults );

    if ( ok )
	printf ( "  All Checks are OK\n\n" );
    else
	printf ( "  ** Check Failed\n\n" );

if ( out != NULL )
{
    if ( appendit == NULL )
    {
        fprintf( out, "filename	nodes	time	eposfs	enegfs	eposfr	enegfr	cemsgs	nestart	necomp	cevents	ecancel	nssave	nscom	tells	eposrs	enegrs	eposrr	enegrr");
        fprintf( out, "	numcr	numds	comcr	comds	cprobes	chits	cmisses");
        fprintf( out, "	nummigr	numstm	numimm	numomm	sqlen	iqlen	oqlen");
        fprintf( out, "	asqlen	aiqlen	aoqlen	sqmax	iqmax	oqmax	ComTime	Objcpu");
	fprintf( out, "	tpagfs	mpagfs	apagfs	stforw	ok\n" );
    }

    fprintf ( out, "%s\t%d\
\t%7.2lf\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%7d\t%7d\t%7d\
\t%7d\t%7d\t%.2lf\t%.2lf\
\t%7d\t%7d\t%.2lf\t%7d\t%s\n",

    filename, high_node + 1,
    elapsed_time,
    tot_eposfs,    tot_enegfs,    tot_eposfr,    tot_enegfr,    tot_cemsgs,
    tot_numestart, tot_numecomp,  tot_cebdls,    tot_ezaps,     tot_nssave,
    tot_nscom,     tot_tells,     tot_eposrs,    tot_enegrs,
    tot_eposrr,    tot_enegrr,    tot_numcreate, tot_numdestroy,tot_ccrmsgs,
    tot_cdsmsgs,   tot_cprobe,    tot_chit,      tot_cmiss,     tot_nummigr,
    tot_numstmigr, tot_numimmigr, tot_numommigr, tot_sqlen,     tot_iqlen,
    tot_oqlen,     tot_asqlen,    tot_aiqlen,    tot_aoqlen,    sqmax_ov,
    iqmax_ov,      oqmax_ov,      tot_comtime,	tot_cputime,
    tot_pfaults,   max_pfaults,   aver_pfaults,	tot_stforw,	ok ? "" : "**" );

}
    exit (! ok);
}
