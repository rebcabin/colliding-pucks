/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	fileio.c,v $
 * Revision 1.9  91/11/01  09:22:53  pls
 * 1.  Change ifdef's, version id.
 * 2.  Add fileNum() routine.
 * 
 * Revision 1.8  91/07/17  15:08:06  judy
 * New copyright notice.
 * 
 * Revision 1.7  91/06/03  12:24:02  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/03/26  09:25:23  pls
 * Change tell to schedule.
 * 
 * Revision 1.5  90/10/19  16:26:44  pls
 *      (1) fix fscanf bugs
 *      (2) check file name length
 *      (3) add object name to tw_fopen error message
 *      (4) fix bug in closing file
 * 
 * Revision 1.4  90/09/18  09:52:29  configtw
 * fix up len bugs in tw_printf() and tw_fprint()
 * 
 * Revision 1.3  90/08/28  11:02:48  configtw
 * Fix Mark3 path in tw_printf().
 * 
 * Revision 1.2  90/08/27  10:41:08  configtw
 * Make tw_printf, tw_fprintf work with varargs.
 * 
 * Revision 1.1  90/08/07  15:38:16  configtw
 * Initial revision
 * 
*/
char fileio_id [] = "@(#)fileio.c       $Revision: 1.9 $\t$Date: 91/11/01 09:22:53 $\tTIMEWARP";


#include <stdio.h>
#include <varargs.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

FILE * HOST_fopen ();

fileNum(name)

	char	*name;

{
	Int		i;

	for ( i = 0; i < MAX_TW_FILES; i++ )
	{
		if ( strcmp ( name, tw_file[i].name ) == 0 )
			break;
	}

	return (i);
}	/* fileNum */

getfile ( filename, name )
 
	char * filename, * name;
{
	FILE * fp;
	int i, size, n;
	Byte * filearea;

#if MARK3
	dep ();
#endif
	for ( i = 0; i < MAX_TW_FILES; i++ )
	{
		if ( tw_file[i].name[0] == 0 )
			break;
	}

	if ( i == MAX_TW_FILES )
	{
		printf ( "too many files (%d)\n", MAX_TW_FILES );
#if MARK3
		indep ();
#endif
		return;
	}

	/* Check to make sure that the provided name is no longer than 
		FILE_NAME_LENGTH characters long.  Otherwise, complain and
		return.  The equivalence part of the comparison is because
		FILE_NAME_LENGTH includes the null, and strlen() doesn't.  */

	if ( strlen ( name ) >= FILE_NAME_LENGTH )
	{
		printf("file name %s too long; shorten to %d characters\n",
				name, FILE_NAME_LENGTH - 1 );
#if MARK3
		indep ();
#endif
		return;
	}

	fp = fopen ( filename, "r" );
 
	if ( fp == NULL )
	{
		printf ( "file %s not found\n", filename );
#if MARK3
		indep ();
#endif
		return;
	}

	fseek ( fp, 0, 2 );
	size = ftell ( fp );

	filearea = m_allocate ( size + 1 );

	if ( filearea == NULL )
	{
		fclose ( fp );
		printf ( "not enough memory (%d bytes) for %s\n", size, filename );
#if MARK3
		indep ();
#endif
		return;
	}

	rewind ( fp );

	n = fread ( filearea, size, 1, fp );
	fclose ( fp );
 
	if ( n != 1 )
	{
		printf ( "fread didn't work: n = %d\n", n );
		return;
	}

	* ( filearea + size ) = 0;
/* 
	printf ( "%s read into %d bytes at %x\n", filename, size, filearea );
*/ 
	strcpy ( tw_file[i].name, name );
	strcpy ( tw_file[i].filename, filename );
	tw_file[i].area = filearea;
	tw_file[i].size = size;
 
#if MARK3
	indep ();
#endif
}

delfile ( name )

	char * name;
{
	int i;

	i = fileNum(name);
	if ( i == MAX_TW_FILES )
	{
		_pprintf ( "delfile: file %s not found\n", name );
		return;
	}

	m_release ( (Mem_hdr *) tw_file[i].area );

	tw_file[i].name[0] = 0;
}

int tw_fopen ( name, mode )

	char * name;
	char * mode;
{
	int i, j;

	i = fileNum(name);
	if ( i == MAX_TW_FILES )
	{
		_pprintf ( "tw_fopen: file %s not found\n", name );
		return ( 0 );
	}
	
	for ( j = 0; j < MAX_TW_STREAMS; j++ )
	{
		if ( xqting_ocb->sb->stream[j].open_flag == 0 )
			break;
	}

	if ( j == MAX_TW_STREAMS )
	{
		_pprintf ( "tw_fopen: too many streams (%d) for object %s\n",
			MAX_TW_STREAMS,xqting_ocb->name);
		return ( 0 );
	}

	xqting_ocb->sb->stream[j].open_flag = 1;
	xqting_ocb->sb->stream[j].file_num = i;
	xqting_ocb->sb->stream[j].char_pos = 0;

	return ( j + 1 );
}

/*************************************************************************

Try to find a stream open for reading. if so is it a file called STDIN. 
if so call tw_fscanf. if none is found try to tw_fopen STDIN and if OK
call tw_fscanf. if no file return EOF.  Return value from tw_fscanf if
all is well.
*************************************************************************/

int tw_scanf ( form, var )

	char * form;
	char * var;
{
	int stream, i, j;

	for ( stream = 0; stream < MAX_TW_STREAMS; stream++ )
	{
		if ( xqting_ocb->sb->stream[stream].open_flag == 1 )
		{
			j = xqting_ocb->sb->stream[stream].file_num;

			if ( strcmp ( tw_file[j].name, "STDIN" ) == 0 )
				break;
		}
	}

	if ( stream == MAX_TW_STREAMS )
	{
		stream = tw_fopen ( "STDIN", "r" );
		if ( stream == 0 ) return ( EOF );

/* A zero would be difficult to distinguish from tw_fscanf returning zero because
it couldn't match the item.  This EOF return should be preceded by an error msg
from tw_fopen saying no file. If returned zero you could get into an
infinite loop here by repeating the call assuming that tw_fscanf returned the
zero. */

	}
	else
		stream++;

	i = tw_fscanf ( stream, form, var );

	return ( i );
}

int tw_fread ( buff, itemsize, nitems, stream )

	Byte * buff;
	int itemsize;
	int nitems;
	int stream;
{
	int file_num = xqting_ocb->sb->stream[stream-1].file_num;
	int char_pos = xqting_ocb->sb->stream[stream-1].char_pos;

	Byte * area = tw_file[file_num].area;
	int filesize = tw_file[file_num].size;

	int i;

	area += char_pos;

	for ( i = 0; i < nitems; i++ )
	{
		if ( char_pos >= filesize )
			break;

		entcpy ( buff, area, itemsize );

		buff += itemsize;
		area += itemsize;
		char_pos += itemsize;
	}

	xqting_ocb->sb->stream[stream-1].char_pos = char_pos;

	return ( i );
}

Byte * tw_fgets ( buff, n, stream )

	Byte * buff;
	int n;
	int stream;
{
	register Byte * bp = buff;

	int file_num = xqting_ocb->sb->stream[stream-1].file_num;
	int char_pos = xqting_ocb->sb->stream[stream-1].char_pos;

	Byte * area = tw_file[file_num].area;
	int filesize = tw_file[file_num].size;
	int i;

	area += char_pos;

	for ( i = 1; i < n; i++ )
	{
		if ( char_pos >= filesize )
		{
			buff = NULL;
			break;
		}

		*bp++ = *area;
		char_pos++;

		if ( *area++ == '\n' )
			break;
	}

	*bp = 0;

	xqting_ocb->sb->stream[stream-1].char_pos = char_pos;

	return ( buff );
}

int tw_fgetc ( stream )

	int stream;
{
	int buff;

	int file_num = xqting_ocb->sb->stream[stream-1].file_num;
	int char_pos = xqting_ocb->sb->stream[stream-1].char_pos;

	Byte * area = tw_file[file_num].area;
	int filesize = tw_file[file_num].size;

	area += char_pos;

	if ( char_pos >= filesize )
		buff = EOF;
	else
	{
		buff = *area;
		char_pos++;
	}

	xqting_ocb->sb->stream[stream-1].char_pos = char_pos;

	return ( buff );
}

int tw_feof ( stream )

	int stream;
{
	int file_num = xqting_ocb->sb->stream[stream-1].file_num;
	int char_pos = xqting_ocb->sb->stream[stream-1].char_pos;
	int filesize = tw_file[file_num].size;

	if ( char_pos >= filesize )
		return ( EOF );

	return ( FALSE );
}

/*************************************************************************

Call sscanf on a single variable only.  This is because we havn't figured
out how to use sscanf with varargs. return 1, 0 if no match, or EOF if
reached end of file. Most files will end with \n and thus will have to 
be scanned again after last genuine read and will return 0 and set EOF.

*************************************************************************/

int tw_fscanf ( stream, form, var )

	int stream;
	char * form;
	int * var;
{
	int file_num = xqting_ocb->sb->stream[stream-1].file_num;
	int char_pos = xqting_ocb->sb->stream[stream-1].char_pos;

	Byte * area;
	int filesize = tw_file[file_num].size;
	int i = 0;

	if (char_pos < filesize ) 
		area = tw_file[file_num].area + char_pos;
	 else
		return(EOF); /* bug. open_flag should not have been READ_S */

/* below is incorrect. The input will skip over leading whitespace which is
not right if the first form character is  %c or %[  */

	while ( *area == ' ' || *area == '\t' || *area == '\n' &&
						( char_pos < filesize) )
	{
		area++;
		char_pos++;
		if ( char_pos >= filesize )
		{
				i = EOF;
				break;
		}
	}

	if (i != EOF) i = sscanf ( area, form, var );
	if ( i == EOF )
	{
		return (EOF);
	}

/* Because sscanf returns EOF, 0, or objects matched (1 in our case ) we need
to read the memory image of the file to find the next whitespace where the next
record begins */  

	while ( *area != 0 && *area != ' ' && *area != '\t' && *area != '\n'
		&& char_pos < filesize )
	{
		area++;
		char_pos++;
	}

	xqting_ocb->sb->stream[stream-1].char_pos = char_pos;

	return ( i );
}

tw_fclose ( stream )

	int stream;
{
	xqting_ocb->sb->stream[stream-1].open_flag = 0;
}

FILE * output_fp_for_mproc;

putfile ( filename, name, node )

	char * filename;
	char * name;
	int * node;
{
	register int i;

#if MARK3
	dep ();
#endif

	for ( i = 0; i < MAX_TW_FILES; i++ )
	{
		if ( tw_file[i].name[0] == 0 )
			break;
	}

	if ( i == MAX_TW_FILES )
	{
		printf ( "too many files (%d)\n", MAX_TW_FILES );
#if MARK3
		indep ();
#endif
		return;
	}

	/* Check to make sure that the provided name is no longer than 
		FILE_NAME_LENGTH characters long.  Otherwise, complain and
		return.  The equivalence part of the comparison is because
		FILE_NAME_LENGTH includes the null, and strlen() doesn't.  */

	if ( strlen ( name ) >= FILE_NAME_LENGTH )
	{
		printf("file name %s too long; shorten to %d characters\n",
				name, FILE_NAME_LENGTH - 1 );
#if MARK3
		indep ();
#endif
		return;
	}

#if MARK3
	indep ();
#endif
 
	strcpy ( tw_file[i].name, name );
	strcpy ( tw_file[i].filename, filename );
	tw_file[i].area = NULL;
	tw_file[i].size = 0;

	if ( miparm.me == 0 )
	{
		obcreate_b ( name, "stdout", *node );
	}
}

tw_fputs ( buff, stream )

	Byte * buff;
	int stream;
{
	TW_STREAM * stream_ptr = &xqting_ocb->sb->stream[stream-1];

	int file_num = stream_ptr->file_num;

	char * object_name = tw_file[file_num].name;

	int len = strlen ( buff );

	int selector = xqting_ocb->oid + stream_ptr->sequence++;

	schedule ( object_name, now, selector, len, buff );
}

tw_fputc ( buff, stream )

	Byte buff;
	int stream;
{
	TW_STREAM * stream_ptr = &xqting_ocb->sb->stream[stream-1];

	int file_num = stream_ptr->file_num;

	char * object_name = tw_file[file_num].name;

	int len = 1;

	int selector = xqting_ocb->oid + stream_ptr->sequence++;

	schedule ( object_name, now, selector, len, &buff );
}

/**************************************************************
*
*       tw_fprintf()
*
*  Send file output to the on node file object for later commitment
*
***************************************************************/

#if defined(SUN) || defined(BF_MACH)
tw_fprintf(iostream,fmt, va_alist)
int iostream;
char *fmt;
va_dcl

{
	va_list xx;

	struct _iobuf  stream;
	unsigned char buf[MAXPKTL];
	int len;

	stream._ptr = buf;
	stream._base = buf;
	stream._cnt = 0;
	stream._bufsiz = MAXPKTL;
	stream._flag = 2;  /* IOWRT */
	stream._file = 0;

	va_start(xx);
	_doprnt(fmt,xx,&stream);
	va_end(xx);

	len = MAXPKTL - stream._cnt;
	buf[len] = 0;

	tw_fputs ( buf, iostream );
}

#else

FUNCTION tw_fprintf ( stream, form, arg1, arg2, arg3, arg4, arg5, arg6,
				arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15,
				 arg16, arg17, arg18, arg19, arg20)

	int stream;
	char * form;
	int *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9,
		*arg10, *arg11, *arg12, *arg13, *arg14, *arg15, *arg16, *arg17,
		*arg18, *arg19, *arg20;
{
	char buff[MAXPKTL];

	sprintf ( buff,
				form, arg1, arg2, arg3, arg4, arg5,
				arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15, arg16, arg17,
				arg18, arg19, arg20 );

	tw_fputs ( buff, stream );
}

#endif

/**************************************************************
*
*       tw_printf()
*
*  Send print output to the node's stdout object for later commitment
*
***************************************************************/

#if defined(SUN) || defined(BF_MACH)

FUNCTION tw_printf(fmt, va_alist)
char *fmt;
va_dcl

{
	va_list xx;

	struct _iobuf  stream;
	unsigned char buf[MAXPKTL];
	int len;
	long selector;

	stream._ptr = buf;
	stream._base = buf;
	stream._cnt = 0;
	stream._bufsiz = MAXPKTL;
	stream._flag = 2;  /* IOWRT */
	stream._file = 0;

	va_start(xx);
	_doprnt(fmt,xx,&stream);
	va_end(xx);

	len = MAXPKTL - stream._cnt;
	buf[len] = 0;
	selector = xqting_ocb->oid + xqting_ocb->sb->stdout_sequence++;
	schedule ( "stdout", now, selector, len+1, buf );
}

#else

FUNCTION tw_printf ( form, arg1, arg2, arg3, arg4, arg5, arg6,
				arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15,
				arg16, arg17, arg18, arg19, arg20)

	char * form;
	int *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9,
		*arg10, *arg11, *arg12, *arg13, *arg14, *arg15, *arg16, *arg17,
		*arg18, *arg19, *arg20;
{
	char buff[MAXPKTL];
	int len;
	long selector;

	sprintf ( buff,
				form, arg1, arg2, arg3, arg4, arg5,
				arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15, arg16, arg17,
				arg18, arg19, arg20 );

	len = strlen ( buff );

	selector = xqting_ocb->oid + xqting_ocb->sb->stdout_sequence++;

	schedule ( "stdout", now, selector, len+1, buff );
}

#endif

tw_fwrite ( buff, size, nitems, stream )

	char * buff;
	int size;
	int nitems;
	int stream;
{
	TW_STREAM * stream_ptr = &xqting_ocb->sb->stream[stream-1];

	int file_num = stream_ptr->file_num;

	char * object_name = tw_file[file_num].name;

	int len = size * nitems;

	int selector = xqting_ocb->oid + stream_ptr->sequence++;

	schedule ( object_name, now, selector, len, buff );
}

FILE * open_output_file ( name )

	char * name;
{
	FILE * fp;
	int i;

	i = fileNum(name);
	if ( i == MAX_TW_FILES )
	{
		_pprintf ( "open_output_file: %s not found\n", name );
		return ( NULL );
	}

	fp = HOST_fopen ( tw_file[i].filename, "w" );

	if ( fp == NULL )
	{
		_pprintf ( "can't create file %s\n", tw_file[i].filename );
		return ( NULL );
	}
/*
	_pprintf ( "open_output_file: %s %x\n", tw_file[i].filename, fp );
*/
	return ( fp );

}

#define ALL_F 25        /* Display all streams */
FUNCTION tw_ftell (st)
int st;
{
  int i;

	for ( i = 0; i < MAX_TW_STREAMS; i++ )
	{
	 if ( st == ALL_F || st == i )
	  {
		 _pprintf("stream = %d, file_num= %d, open_flag= %d, char_pos= %d\n",
		 i+1,
		 xqting_ocb->sb->stream[i].file_num,
		 xqting_ocb->sb->stream[i].open_flag,
		 xqting_ocb->sb->stream[i].char_pos );
	  }

	}
	return (0);
}

