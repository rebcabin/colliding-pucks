/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	idlist.c,v $
 * Revision 1.4  91/08/07  16:36:14  pls
 * Insert RCS variables.
 * 
 * Revision 1.3  91/07/17  15:08:45  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:24:22  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:32  configtw
 * Initial revision
 * 
*/
char idlist_id [] = "@(#)idlist.c       $Revision: 1.4 $\t$Date: 91/08/07 16:36:14 $\tTIMEWARP";


/*

Purpose:

		idlist.c maintains the list of modules used in the Time Warp system.
		Only one short routine is necessary.  The bulk of the module is
		taken up with external definitions and the definition of an array
		containing one character string per module.

Functions:

		dump_id_list(fp) - print a list of TW's modules into a file
				Parameters - int *fp
				Return - Always returns 0

Implementation:

		Every module in the system contains, at its head, a short
		declaration of a static string.  This string contains the name
		of the module.  By printing all of these strings, the system
		can produce a list of its constituent modules.  In some cases,
		experimental or alternate modules might have been used, and, if
		those modules had their identification string altered, then
		they can be identified in the compiled module, at run time,
		through this string list.  idlist.c does the general work
		of maintaining and printing this list.

		There isn't much work to do.  idlist.c needs an array of the
		strings, to efficiently print them, so such an array is 
		defined here.  Of course, the actual strings reside in the
		modules, so idlist.c also needs an extern statement for each,
		so the compiler knows that the strings aren't here.  Finally,
		one function is used to dump the strings.  It simply runs
		through the array, printing strings one after another.  It
		uses an fprintf() to put them into a file specified by the
		parameter.
*/

#include <stdio.h> 

extern char brkpt_id[];
extern char buglib2_id[];
extern char cmp_id[];
extern char comdtab_id[];
extern char command_id[];
extern char commit_id[];
extern char cubeio_id[];
extern char deliver_id[];
extern char fileio_id[];
extern char format_id[];
extern char getime_id[];
extern char gvt_id[];
extern char hostifc_id[];
extern char int_id[];
extern char list_id[];
extern char loadman_id[];
extern char machdep_id[];
extern char machifc_id[];
extern char make_id[];
extern char mem_id[];
extern char migr_id[];
extern char mkmsg_id[];
extern char moninit_id[];
extern char monitor_id[];
extern char mproc_id[];
extern char msgcntl_id[];
extern char nq_id[];
extern char objend_id[];
extern char objhead_id[];
extern char objifc_id[];
extern char rollback_id[];
extern char save_id[];
extern char sched_id[];
extern char sendback_id[];
extern char serve_id[];
extern char services_id[];
extern char start_id[];
extern char state_id[];
extern char stats_id[];
extern char storage_id[];
extern char tester_id[];
extern char timewarp_id[];
extern char tlib_id[];

#ifndef TRANSPUTER
extern char toupper_id[];
#endif
  
extern char tstrinit_id[];
extern char turboq2_id[];
extern char objloc_id[];
extern char vtime_id[];

char * id_list [] =
{
	brkpt_id,
	buglib2_id,
	cmp_id,
	comdtab_id,
	command_id,
	commit_id,
	cubeio_id,
	deliver_id,
	fileio_id,
	format_id,
	getime_id,
	gvt_id,
	hostifc_id,
	int_id,
	idlist_id,
	list_id,
	loadman_id,
	machdep_id,
	machifc_id,
	make_id,
	mem_id,
	migr_id,
	mkmsg_id,
	moninit_id,
	monitor_id,
	mproc_id,
	msgcntl_id,
	nq_id,
	objend_id,
	objhead_id,
	objifc_id,
	rollback_id,
	save_id,
	sched_id,
	sendback_id,
	serve_id,
	services_id,
	start_id,
	state_id,
	stats_id,
	storage_id,
	tester_id,
	timewarp_id,
	tlib_id,

#ifndef TRANSPUTER
	toupper_id,
#endif
	tstrinit_id,
	turboq2_id,
	objloc_id,
	vtime_id,
	0
};

dump_id_list ( fp )

	FILE * fp;
{
	char ** id;

	for ( id = id_list; *id != 0; id++ )
	{
		HOST_fprintf ( fp, "%s\n", *id );
	}
}
