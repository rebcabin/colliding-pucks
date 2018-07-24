/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */
/***********************************************************************
*
*  twsp3.c
*
*  Module for simulator I/O commands and dynamic memory operations.
*  The BBN host interface is same as sun and should use ramfiles.
*  Formerly twfileio.c  and getmemory.c
*
*   JJW 3/16/88  and 4/19/88 for dyn memory
*   JJW March 90, insert newBlockWithPtrs.
*   JJW  Change tw_printf & tw_fprintf to use varargs. THEY USE MAXPKTL
*        for buffer size.
*************************************************************************/


#ifdef BF_MACH          /* Surely this repetitive nonsense   */
#include <stdio.h>      /* can be fix by a properly worked   */
#include <varargs.h>
#endif                  /* Makefile?                         */
#ifdef BF_PLUS
#include <stdio.h>
#endif

#ifdef SUN
#include <stdio.h>
#include <varargs.h>
#endif SUN


#ifdef MARK3
#include "stdio.h"
#endif

static char  twsp3id[] = "%W%\t%G%";

#include "twcommon.h"
#include "machdep.h"
#include "tws.h"
#include "twsd.h"

/* FILE_NAME_LENGTH is size of the file name strings in this file */ 
#define CLOSED_S 1
#define WRITE_S 2
#define READ_S 3
#define UNDEF_S 5
#define EOF_S 4
#define UNUSED_S 0

#define FAIL -1
#define ERROR 0

#define STRSIZ 40

typedef struct
{
    char path[FILE_NAME_LENGTH];
    char fname[FILE_NAME_LENGTH];
    int filesz;
    int stat;
    FILE *ioptr;
    char *inbuf;
} IOFILE;

typedef struct  { double deletime;  Name_object name; } DELETARY;


/* global variables for this module */
int max_file_num = 0;
IOFILE fileary[MAX_TW_FILES];
DELETARY deleteary[MAX_TW_FILES];  /* args plus sentinel */
extern char *sim_malloc();
extern char *sim_free();
extern void error();
extern double atof();
/* the following also for delfile */
extern int file_del_flag;
int max_delary_size = 0;   /* how many we have */
int cur_delary_idx = 0;    /* how many left */
long selectval;  /* temp kludge for selector in tw_printf */
/*************************************************************************
*
*	erprnt
*
*
*  Prints the object name and current virtual time to stderr
*  Assumes that gl_hdr_ind is correct. Internally used by error procedures.
*
*************************************************************************/
FUNCTION void  erprnt()

{
   char strng[STRSIZ];

   sprintVTime(strng, emq_current_ptr->rlvt);
   fprintf(stderr,"current time: %s\n",strng);
   fprintf(stderr,"obj name: %s\n",obj_hdr[gl_hdr_ind].name);
   sim_debug("continue ?");
}

/***************************************************************************
* io_ary_setup (path,name)
*
* local function which sets up a file description in the fileary
* called by io_get_file and io_put_file
****************************************************************************/
FUNCTION io_ary_setup(path,name)
   char *path;
   char *name;
{

   int indx;

/* initial condition status is UNUSED_S = 0 */

   if (max_file_num >= MAX_TW_FILES) 
        return(FAIL);

   for(indx=0; indx < MAX_TW_FILES; indx++)
	{
	if (strcmp(fileary[indx].path,path) == 0)
	      {
	      fprintf(stderr,"getfile: file already defined - %s\n",path);
	      return(indx);   /* ignore this call */
	      }
	}

   indx = ++max_file_num;
   strncpy(fileary[indx].path,path,FILE_NAME_LENGTH-1);
   strncpy(fileary[indx].fname,name,FILE_NAME_LENGTH-1);
   fileary[indx].stat = UNDEF_S;
   return(indx);
}


/************************************************************************
*
* io_put_file
* set up array element describing the file
* called by put_file() in newconfO.c
* file is opened for writing as far as unix is concerned
*
************************************************************************/
FUNCTION io_put_file(path,name)
   char *path;
   char *name;
{ 
   int indx;
   FILE *fioptr;

   indx = io_ary_setup(path,name);
   if (indx == FAIL)
      { 
      fprintf(stderr,"putfile: too many files\n"); 
      return(FAILURE);
      }
   fioptr = fopen(path,"w");
   if (fioptr == NULL)
	{
	fprintf(stderr,"putfile: can't open unix file - %s\n",path);
	 return(FAILURE);
	}
   else
        {
	fileary[indx].ioptr = fioptr;
	fileary[indx].stat = WRITE_S;
	}
   return(SUCCESS);
}

/************************************************************************
*
* io_get_file
* set up array element describing the file
* called by get_file() in newconfO.c
* file is opened for reading and then read into buffer for access
*
************************************************************************/
FUNCTION io_get_file(path,name)
   char *path;
   char *name;
{ 
   extern long ftell();
   int indx,fsk,fsk2;
   long ofst, ofst2;
   FILE *fioptr;
   char *bf;
 
   indx = io_ary_setup(path,name);
   if (indx == FAIL)
      { 
      fprintf(stderr,"getfile: too many files\n"); 
      return(FAILURE);
      }

   fioptr = fopen(path,"r");
   if (fioptr == NULL)
	{
	 fprintf(stderr,"getfile: can't open file - %s\n",path);
	 return(FAILURE);
	}
   else
        {
	fileary[indx].ioptr = fioptr;
	fileary[indx].stat = READ_S;
	}
   fsk = fseek(fioptr,0L,2);
   ofst = ftell(fioptr);
   fsk2 = fseek(fioptr,0L,0);
   if (ofst < 0 || fsk != 0 || fsk2 != 0)
	{
	fprintf(stderr,"getfile: fseek or ftell failed\n");
	return(FAILURE);
	}
   fileary[indx].filesz= ofst;
#ifdef DEBUG
      fprintf(stderr,"getfile: size = %d\n",ofst);
#endif
   bf = sim_malloc((unsigned)ofst);
   if (bf == NULL)
	{
	fprintf(stderr,"getfile: out of memory\n");
	return(FAILURE);
	} 
   clear(bf,(int)ofst);
   fileary[indx].inbuf = bf;
   for (fsk2=0; fsk2<ofst; fsk2++)
	{
	bf[fsk2] = fgetc(fioptr);
	}
   if ( fclose(fioptr) == EOF ) 
	fprintf(stderr,"getfile: Unix fclose failed\n");
   return (SUCCESS);
}

/**************************************************************************
*
* io_del_file(name time)   set up data for deleting a "getfile"
*
* called by delfile config command.  Set the flag to indicate at least one
* file is to be deleted and then set up the data structure with the information
* to delete the file.
*
***************************************************************************/
FUNCTION io_del_file(name, time)
char* name;
double time;
{
    int indexdel;

	file_del_flag = 1;
	if (max_delary_size == 0) 
		{
		deleteary[1].deletime = POSINF;
		deleteary[0].deletime = time;
		strcpy(deleteary[0].name, name);
		max_delary_size  = 2;
		}
	else
	   {	
	   for  ( indexdel = max_delary_size -1;
			 deleteary[indexdel].deletime > time && indexdel >= 0;
			 indexdel--) 
		{
		deleteary[indexdel+1].deletime = deleteary[indexdel].deletime;
		strcpy(deleteary[indexdel+1].name, deleteary[indexdel].name);
		}
	   deleteary[indexdel+1].deletime = time;
	   strcpy(deleteary[indexdel+1].name, name);
	   max_delary_size++;
/*	   deleteary[max_delary_size].deletime = POSINF; */
	   }
#ifdef DEBUG
for  ( indexdel = 0; indexdel < max_delary_size; indexdel++)

printf("delete %s: at time: %f\n",deleteary[indexdel].name, deleteary[indexdel].deletime);
#endif
}
/**************************************************************************
*
* io_rel_file(name time)  actually release a file (undo a getfile) 
*
* called per event if file_del_flag = 1. Check if rlvt has been reached and
* if so delete the copy of the file stored in the simulator.
*
***************************************************************************/
FUNCTION io_rel_file(savelvt)
  double savelvt;

{
  int indx,flag;
  Name_object  *fnm;

  if (deleteary[cur_delary_idx].deletime >= savelvt) return(SUCCESS);
/*  strcpy(fnm, deleteary[cur_delary_idx].name); */
   fnm =  (Name_object *)(deleteary[cur_delary_idx].name);
   for(indx=0, flag= 0; indx < MAX_TW_FILES; indx++)
	if (strcmp(fileary[indx].fname,fnm) == 0)
         { flag = 1;  break;  }
   if (flag != 1)
      {
      fprintf(stderr,"delete file: no such file in configuration - %s\n",fnm);
      erprnt();
      return(ERROR);
      }
#ifdef DEBUG
   else fprintf(stderr,"delete file: deleting %s  (%d of %d)\n",
	fnm, cur_delary_idx+1, max_delary_size-1);
#endif
   flag = 0;
   if (fileary[indx].stat == UNDEF_S || fileary[indx].stat == UNUSED_S)
      {
	fprintf(stderr,"delete file: file never defined or never opened -%s\n",
		fnm);
	erprnt();
	flag = 1;
      }
   else
   if (fileary[indx].stat == WRITE_S)
      {
	fprintf(stderr,"delete file: file open for WRITING -%s\n", fnm);
	erprnt();
	flag = 1;
      }
   
/* getfile and putfile set the status but open/close do not modify it
   because they are stream specific */
   if (flag == 0)
      {
	fileary[indx].stat = UNUSED_S;
	fclose(fileary[indx].ioptr);
	sim_free(fileary[indx].inbuf);
      }
   cur_delary_idx++;
   if (deleteary[cur_delary_idx].deletime == POSINF)
	file_del_flag = 0;   /* this is the last one */


  return(SUCCESS);
}


/**************************************************************************
*
* tw_fopen(file_name, "r" or "w") returns int file number or -1 if fails
*
* open a file which must have been setup and described in fileary. This
* setup is done by io_get_file and io_put_file
*
*
***************************************************************************/

FUNCTION tw_fopen(name,type)
   char *name;
   char *type;

{
   int  indx, idx2;
   int  flag;

   for(indx=0, flag= 0; indx < MAX_TW_FILES; indx++)
	if (strcmp(fileary[indx].fname,name) == 0)
         { flag = 1;  break;  }
   if (flag != 1)
      {
      fprintf(stderr,"tw_fopen: no such file in configuration - %s\n",name);
      erprnt();
      return(ERROR);
      }
#ifdef DEBUG
   else fprintf(stderr,"tw_fopen: opening %s, gl_file  %d\n",fileary[indx].fname,indx);
#endif
  
   if (strcmp(type,"r") == 0 && fileary[indx].stat == READ_S ) 
	{
	for (idx2 = 0, flag= 0; idx2 < MAX_TW_STREAMS; idx2++)
	   if ( obj_bod[gl_bod_ind].stream[idx2].open_flag == UNUSED_S)
	      { obj_bod[gl_bod_ind].stream[idx2].open_flag = READ_S;
		obj_bod[gl_bod_ind].stream[idx2].file_num = indx;
		obj_bod[gl_bod_ind].stream[idx2].char_pos = 0;
		flag = 1;
		break;
	      }
	}
   else
   if (strcmp(type,"w") == 0 && fileary[indx].stat == WRITE_S )
	{
	for (idx2 = 0, flag= 0; idx2 < MAX_TW_STREAMS; idx2++)
	   if ( obj_bod[gl_bod_ind].stream[idx2].open_flag == UNUSED_S)
	      { obj_bod[gl_bod_ind].stream[idx2].open_flag = WRITE_S;
		obj_bod[gl_bod_ind].stream[idx2].file_num = indx;
		obj_bod[gl_bod_ind].stream[idx2].char_pos = 0;
		flag = 1;
		break;
	      }

	}
   else 
	{
	fprintf(stderr,"tw_fopen: error, bad mode or system config error\n");
	erprnt();
	return(ERROR);
	}
   if (flag != 1)
	   {
	   fprintf(stderr,"tw_fopen: can't open, too many streams \n");
	   erprnt();
	   return(ERROR);
	   }
   return(idx2+1);	/* 1 thru 5 with current maximum */
}

/*************************************************************************
*
* tw_fgets (dest pointer, number of chars, stream arg)
*
* read from stream into dest. Read n-1 chars or up to a
* newline, putting newline into dest. end the dest with a null.
* called by user 
***************************************************************************/

char * FUNCTION tw_fgets(s,n,st)
   char *s;
   int   n;
   int  st;

{
   int indx;
   int charidx;
   int  ct;

   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == READ_S)
	{
	indx =  obj_bod[gl_bod_ind].stream[st].file_num; 
	charidx = obj_bod[gl_bod_ind].stream[st].char_pos; 
	ct = charidx + n - 1;  

	if (  charidx < fileary[indx].filesz) 

	  {
	  do  *s++ = fileary[indx].inbuf[charidx++];

	  while ( charidx < ct
		&& fileary[indx].inbuf[charidx-1] != 0
		&& fileary[indx].inbuf[charidx-1] != 10 
		&& charidx < fileary[indx].filesz );

	  if (fileary[indx].inbuf[charidx-1] != 0)
	   *(s) = 0;
	
	  obj_bod[gl_bod_ind].stream[st].char_pos = charidx;
	  }

	if (charidx >= fileary[indx].filesz) 
	  {
	  obj_bod[gl_bod_ind].stream[st].open_flag = EOF_S;
#ifdef DEBUG
	  fprintf(stderr,"tw_fgets: EOF\n");
#endif
	  return(NULL);
	  }
	return(s);
	}
   return(NULL);
   }
   else
      {
      fprintf(stderr,"tw_fgets: invalid stream number - %d\n",st+1);
      return(NULL);
      }
}

/*************************************************************************
*
* tw_fputs(sss, fileid)  - char * sss,  int fileid
*
* put string into  stream. return  EOF if error
* called by user
*
************************************************************************/
FUNCTION tw_fputs(sss,st)
   char *sss;
   int   st;


{
   FILE *fioptr;


   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == WRITE_S)
	{
	fioptr = fileary[obj_bod[gl_bod_ind].stream[st].file_num].ioptr;
	fputs(sss,fioptr);
	if (ferror(fioptr))
	   {
	   fprintf(stderr,"tw_fputs: unix fputs failed on stream %d\n",st+1);
	   clearerr(fioptr);
	   return(EOF);
	   }
	return(0);
	}
      else
	{
	fprintf(stderr,"tw_fputs: stream %d is not open for writing\n",st+1);
	return(EOF);
	}

      }
      else
      {
      fprintf(stderr,"tw_fputs: invalid stream number\n");
      return(EOF);
      }
}


/*************************************************************************
*
* tw_fgetc(fileid)  - fileid is an int
*
* get char from stream. return char or EOF
* called by user
*
************************************************************************/
FUNCTION tw_fgetc(st)
   int st;
{
   int chr;
   int indx;
   int charidx;

   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == READ_S)
	{
	indx =  obj_bod[gl_bod_ind].stream[st].file_num; 
	charidx = obj_bod[gl_bod_ind].stream[st].char_pos; 
	if (  charidx < fileary[indx].filesz) 
	     {
	     chr = fileary[indx].inbuf[charidx];
	     obj_bod[gl_bod_ind].stream[st].char_pos++;
	     return(chr);
	     }
	else
	     {
	     obj_bod[gl_bod_ind].stream[st].open_flag =  EOF_S;
#ifdef DEBUG
	     fprintf(stderr,"tw_fgetc: EOF\n");
#endif
	     return(EOF);
	     }   
	}
      else
	{
	fprintf(stderr,"tw_fgetc: stream %d is not open for reading\n",st+1);
        return(EOF);
	}

      }
      else
      {
      fprintf(stderr,"tw_fgetc: invalid stream number - %d\n",st+1);
      return(EOF);
      }
}

/***************************************************************************
*
* tw_fputc(char, fileid)
* do unix fputc on the fileid (an integer)
* called by the user
*   check for error JJW####
**********************************************************************/
FUNCTION tw_fputc(chr,st)
   char chr;
   int st;

{
   FILE *fioptr;


   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == WRITE_S)
	{
	fioptr = fileary[obj_bod[gl_bod_ind].stream[st].file_num].ioptr;
	fputc(chr,fioptr);
	return(chr);
	}
      else
	{
	fprintf(stderr,"tw_fputc: stream %d is not open for writing\n",st+1);
        return(EOF);
	}

      }
      else
      {
      fprintf(stderr,"tw_fputc: invalid stream number - %d\n",st+1);
      return(EOF);
      }
}

/*************************************************************************
*
* tw_fread(buffer ptr, size of item, count of items, fileid)
*
* read 'count' items of 'size' bytes into buffer. return actual
* count read or zero if EOF or error. Thus if it returns n, which is less
* than 'count', an EOF (or error) ocurred on item n+1 which may or may not
* mean that the char_ptr is at EOF.
* called by user
*
************************************************************************/
FUNCTION tw_fread(buf,size,count,st)
   char *buf;
   int size;
   int count;
   int st;

{
   int indx;
   int charidx;
   int itemsz,ict;

   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == READ_S)
	{
	indx =  obj_bod[gl_bod_ind].stream[st].file_num; 
	charidx = obj_bod[gl_bod_ind].stream[st].char_pos; 
	for (ict=0; ict < count; ict++)
	   {
	   if (charidx < fileary[indx].filesz 
		 && (charidx + size) <= fileary[indx].filesz)  /* JJW 7/10 <= */
	     {
	     for (itemsz=0; itemsz < size; itemsz++)
	       *buf++ = fileary[indx].inbuf[charidx++];
	     obj_bod[gl_bod_ind].stream[st].char_pos += size;
	     }
	   else break; /* JJW 7/10 */
	   }
	if (charidx >= fileary[indx].filesz)
	  obj_bod[gl_bod_ind].stream[st].open_flag  = EOF_S;
	return(ict);
	}
       else return(ERROR);

      }
      else
      {
      fprintf(stderr,"tw_fread: invalid stream number - %d\n",st+1);
      return(ERROR);
      }
}

/*************************************************************************
*
* tw_fwrite(buffer ptr, size of item, count of items, fileid)
*
* read 'count' items of 'size' bytes into buffer. return actual
* count read or zero if EOF or error.
* called by user
*
************************************************************************/
FUNCTION tw_fwrite(buf,size,count,st)
   char *buf;
   int size;
   int count;
   int st;

{
   FILE *fioptr;
   int retval;


   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == WRITE_S)
	{
	fioptr = fileary[obj_bod[gl_bod_ind].stream[st].file_num].ioptr;
	retval =fwrite(buf,size,count,fioptr);
	return(retval);
	}
      else
	{
	fprintf(stderr,"tw_fwrite: stream %d is not open for writing\n",st+1);
	return(0);
	}

      }
      else
      {
      fprintf(stderr,"tw_fwrite: invalid stream number - %d\n",st+1);
      return(0);
      }


}
/*************************************************************************
*
* tw_feof(fileid)  - fileid is an int
*
* return non zero if the file specified by fileid is not open
* called by user
*
************************************************************************/
FUNCTION tw_feof(st)
   int st;
{

   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if (obj_bod[gl_bod_ind].stream[st].open_flag  == EOF_S)
	return(EOF);
      else return(0);
       }
   else
      {
      fprintf(stderr,"tw_feof: invalid stream number - %d\n",st+1);
      return(EOF);
      }
}


/*************************************************************************
*
* tw_fclose(fileid)  - fileid is an int
*
* perform close on the stream structure in a file
* if reading: some other object may be reading the file so don't disturb the
*    array.
* if writing: config has already set up the only possible array items but
* close the file anyway to be sure buffer is flushed. 
* called by user
*
************************************************************************/
FUNCTION tw_fclose(st)
   int st;
{

   if (--st <= MAX_TW_STREAMS && st >= 0)
      {
      if ( obj_bod[gl_bod_ind].stream[st].open_flag  == WRITE_S)
	{
	if (fclose(fileary[obj_bod[gl_bod_ind].stream[st].file_num].ioptr)
              == EOF ) 
	fprintf(stderr,"tw_fclose: Unix fclose failed: %s\n",
		fileary[obj_bod[gl_bod_ind].stream[st].file_num].fname);
	fileary[obj_bod[gl_bod_ind].stream[st].file_num].stat = CLOSED_S;
	}
      obj_bod[gl_bod_ind].stream[st].open_flag  = UNUSED_S;
      obj_bod[gl_bod_ind].stream[st].file_num = 0;
      obj_bod[gl_bod_ind].stream[st].char_pos = 0;
      return(0);
       }
   else
      {
      fprintf(stderr,"tw_fclose: invalid stream number - %d\n",st+1);
      return(EOF);
      }
}


/*************************************************************************
*
* tw_ftell(fileid)  - fileid is an int
*
* return data on stream array if fileid is ALL_F
* called by user
*
************************************************************************/
#define ALL_F 25
FUNCTION tw_ftell(st)
   int st;
{
   int ii;

   for (ii = 0; ii < MAX_TW_STREAMS; ii++)
    if (st == ALL_F || st == ii)
     printf("stream = %d, file_num= %d, open_flag= %d, char_pos= %d\n",
	 ii+1,
	 obj_bod[gl_bod_ind].stream[ii].file_num,
	 obj_bod[gl_bod_ind].stream[ii].open_flag,  
	 obj_bod[gl_bod_ind].stream[ii].char_pos  );
   return(0);

}

/*************************************************************************
*
*  tw_printf( format, ARGS)
*
*  Does a schedule to stdout object.  Consecutive calls to tw_printf will
*  do a schedule with the selector incremented by one each time.
*
*************************************************************************/
#if defined(MARK3) || defined(BF_PLUS)
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

    selector = selectval++;

    schedule ( "stdout", now, selector, len+1, buff );
}
#endif


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

/*    len = strlen ( buf ); */
    len = MAXPKTL - stream._cnt;
    buf[len] = 0;
    selector = selectval++;
    schedule ( "stdout", now, selector, len+1, buf );
}

#endif


/*************************************************************************
*
*  tw_fprintf( stream, format, ARGS)
*
*************************************************************************/
#if defined(MARK3) || defined(BF_PLUS)
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

#endif


/*************************************************************************

Try to find a stream open for reading. if so is it a file called STDIN. 
if so call tw_fscanf. if none is found try to tw_fopen STDIN and if OK
call tw_fscanf. if no file return EOF.  Return value from tw_fscanf if
all is well.
*************************************************************************/

FUNCTION int tw_scanf ( form, var )

    char * form;
    char * var;
{
    int st, i, j;

    for ( st = 0; st < MAX_TW_STREAMS; st++ )
    {
        if (obj_bod[gl_bod_ind].stream[st].open_flag  == READ_S)
	{
	    j =  obj_bod[gl_bod_ind].stream[st].file_num; 
	    if ( strcmp ( fileary[j].fname, "STDIN" ) == 0 )
		break;
	}
    }

    if ( st == MAX_TW_STREAMS )
    {
	st = tw_fopen ( "STDIN", "r" );

	if ( st == 0 ) return ( EOF );

/* A zero would be difficult to distinguish from tw_fscanf returning zero because
it couldn't match the item.  This EOF return should be preceded by an error msg
from tw_fopen saying no file. If returned zero you could get into an
infinite loop here by repeating the call assuming that tw_fscanf returned the
zero. */

    }
    else
	st++;

    i = tw_fscanf ( st, form, var );

    return ( i );
}

/*************************************************************************

Call sscanf on a single variable only.  This is because we havn't figured
out how to use sscanf with varargs. return 1, 0 if no match, or EOF if
reached end of file. Most files will end with \n and thus will have to 
be scanned again after last genuine read and will return 0 and set EOF.

*************************************************************************/

FUNCTION int tw_fscanf ( stream, form, var )

    int stream;
    char * form;
    int * var;
{
    int file_num, char_pos, filesize;
    int  i = 0;
    char *area;

    if  (obj_bod[gl_bod_ind].stream[stream-1].open_flag != READ_S) {
	  fprintf(stderr,"tw_fscanf: stream %d is not open for reading\n",
		stream-1);
	  return(EOF);
	  }
    file_num = obj_bod[gl_bod_ind].stream[stream-1].file_num;
    char_pos = obj_bod[gl_bod_ind].stream[stream-1].char_pos;
    filesize = fileary[file_num].filesz;
    if (char_pos < filesize ) 
    	area = fileary[file_num].inbuf + char_pos;
     else
	return(EOF); /* bug. open_flag should not have been READ_S */

/* below is incorrect. The input will skip over leading whitespace which is
not right if the first form character is  %c or %[  */

    while (( *area == ' ' || *area == '\t' || *area == '\n') &&
			( char_pos < filesize) )
    {
	area++;
	char_pos++;
	if (char_pos >= filesize) {
		i = EOF;
		break;
		}
    }

    if (i != EOF) i = sscanf ( area, form, var );
    if (i == EOF) {
		obj_bod[gl_bod_ind].stream[stream-1].open_flag  = EOF_S;
		obj_bod[gl_bod_ind].stream[stream-1].char_pos= filesize;
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

    obj_bod[gl_bod_ind].stream[stream-1].char_pos= char_pos;
    if (char_pos == filesize)
		obj_bod[gl_bod_ind].stream[stream-1].open_flag  = EOF_S;

    return ( i );
}

/***************************************************************************
*
*	dynamic memory functions
*
*************************************************************************/
/* this implementation makes Pointer an int whose value is equal to
   the c-pointer returned by sim_malloc. */ 

/* typedef int Pointer  -----  in twcommon.h     */


/*************************************************************************
*
*	statePtr returns Pointer which is unique value
*	meaning state pointer.
*
*  This is a Timewarp entry point called by the user
*
*************************************************************************/
FUNCTION Pointer statePtr()
{

   return ((Pointer)Max_Addresses + 1);
}


/************************************************************************
*       cr_make_memryptr_array
*       called by new BlockPtr initially to make the memory pointer array
*       for  dynamic memory allocation and to enlarge it.
*       makes one segment.
*
*************************************************************************/

FUNCTION  void * cr_make_memryptr_array(bindx)
int bindx;
{
	int *ptr, *ptr2;
	int ix;
	
	if  ( (ptr =  (void *) sim_malloc((bindx)* sizeof(int))) == 0)
	   {
	   error("out of memory for dyn mem alloc table");
	   }
	else
	   {
	   ptr2 = ptr;
	   for (ix = 0; ix < (bindx); (ptr2[ix] = 0), ix++);
	   } 
        return ((void *)ptr);
}

/*************************************************************************
*
*      cr_expand_memryptr_array()
*
*      expand newBlockPtr memory allocation array by SEG_TABLE_INCREMENT
*      to provide more space for allocations
*
**********************************************************************/
FUNCTION cr_expand_memryptr_array()
{
  int *ptr1, *ptr2;
  int idx, tblsiz;

  tblsiz = obj_bod[gl_bod_ind].seg_table_siz;
  ptr1 = obj_bod[gl_bod_ind].memry_pointers_ptr;
  ptr2 = cr_make_memryptr_array(tblsiz + (SEG_TABLE_INCREMENT));
  for (idx = 0; idx < tblsiz; idx++)
    ptr2[idx] = ptr1[idx];
  tblsiz += (SEG_TABLE_INCREMENT);
  sim_free(ptr1);
  obj_bod[gl_bod_ind].memry_pointers_ptr = (void *) ptr2;
  obj_bod[gl_bod_ind].seg_table_siz = tblsiz;
}



  
/*************************************************************************
*
*	ptrsetup sets up array holding newBlockPtr values which are now indexes
*	         into an array
*
*  Takes a Pointer and puts it into the array at first empty location.
*  Creates array at first call.  Returns
*  index in the array where Pointer  is stored.  disposeBlockPtr zeros the entry at
*  the specified index.   The array is
*  Pointer memry_pointers_ptr[] and is seg_table_size  long.
*  the size is local to the object and is expanded as necessary
*
*************************************************************************/
FUNCTION ptrsetup(bkptr)
   Pointer bkptr;
{
   int indx, ixx, tblsiz;
   if (obj_bod[gl_bod_ind].memry_pointers_ptr == 0)
	{
	obj_bod[gl_bod_ind].memry_pointers_ptr = cr_make_memryptr_array(SEG_TABLE_INCREMENT);
	obj_bod[gl_bod_ind].memry_pointers_ptr[0] =  (Pointer *) bkptr;
        obj_bod[gl_bod_ind].seg_table_siz = SEG_TABLE_INCREMENT;
	return(1);   /* index +1 */
	}
    else 
      {
        tblsiz = obj_bod[gl_bod_ind].seg_table_siz;
	for ( indx = 0; (indx < tblsiz) &&
	  (obj_bod[gl_bod_ind].memry_pointers_ptr[indx] != 0) ; indx++);
	if (indx == tblsiz)
	   {
	   cr_expand_memryptr_array();
	   }
	obj_bod[gl_bod_ind].memry_pointers_ptr[indx] =  (Pointer *) bkptr;
	return (indx + 1);
      }
}

/*************************************************************************
*
*	newBlockPtr allocates memory and returns Pointer
*
*  
*
*************************************************************************/
FUNCTION Pointer newBlockPtr(size)
   int size;
{
   char *bkptr;
   int  retval;
   char strng[STRSIZ];

   if (obj_bod[gl_bod_ind].memry_limit++ == Max_Addresses) {
	fprintf(stderr,"newBlockPtr: too many alloc for object\n");
	erprnt();
	}
   if (debug_switch)
    {
      printf("     total blocks allocated: %d\n",obj_bod[gl_bod_ind].memry_limit);
      printf("     size: %d\n",size);
      sim_debug("newBlockPtr");
    }
   if (size == 0 || size > 100000) {
	fprintf(stderr,"newBlockPtr: invalid size: %d\n", size);
	erprnt();
	}

   if (obj_bod[gl_bod_ind].memry_limit > obj_bod[gl_bod_ind].segMax)
     	obj_bod[gl_bod_ind].segMax =  obj_bod[gl_bod_ind].memry_limit;

   bkptr = sim_malloc((unsigned)size);
   if (bkptr == NULL)
      {
/*      error("newBlockPtr: out of memory\n"); */
      fprintf(stderr,"newBlockPtr: out of memory: size = %d\n", size);
      erprnt();
      return(0);
      }
   else
      { retval = ptrsetup(bkptr);   /* index + 1 */
	return((Pointer)retval);
      }
}



/*************************************************************************
*
*	disposeBlockPtr returns memory to system
*
*  This is a Timewarp entry point called by the user
*
*************************************************************************/
FUNCTION int  disposeBlockPtr(bkpt)
   Pointer  bkpt;
{
   Pointer xxx;
   xxx = (Pointer ) obj_bod[gl_bod_ind].memry_pointers_ptr[(int)bkpt - 1];
   obj_bod[gl_bod_ind].memry_limit--;
   if (debug_switch)
    {
      printf("     total blocks allocated now: %d\n",obj_bod[gl_bod_ind].memry_limit);
      sim_debug("disposeBlockPtr");
    }
   obj_bod[gl_bod_ind].memry_pointers_ptr[(int)bkpt - 1] = 0;
   sim_free( (char * )xxx);
   return;
}




/*************************************************************************
*
*	pointerPtr returns a genuine c-pointer
*
*
*  This is a Timewarp entry point called by the user
*
*************************************************************************/
FUNCTION void  *pointerPtr(bkpt)
   Pointer bkpt;
{
   int truidx;

   truidx = ((int) bkpt) - 1;
   if (debug_switch)
	{
	fprintf(stderr,"  index = %d\n",truidx);
	fprintf(stderr,"  pointer: %d\n",obj_bod[gl_bod_ind].memry_pointers_ptr[truidx]);
	sim_debug("pointerPtr");
	}
   if (truidx == Max_Addresses)
	return ((void *) obj_bod[gl_bod_ind].current);
   if (obj_bod[gl_bod_ind].memry_pointers_ptr[truidx] == 0)
	{
	fprintf(stderr," Invalid block pointer (disposed)\n");
	erprnt();
        }
	

   return ( (void *) obj_bod[gl_bod_ind].memry_pointers_ptr[truidx]);
}

/*************************************************************************
*
*	newBlockWithPtrs returns a genuine c-pointer
*
*
*  This is a Timewarp entry point called by the user and combines
*  newBlockPtr with pointerPtr.
*
*************************************************************************/
FUNCTION void  *newBlockWithPtrs(size, ptr)
   int size;
   Pointer *ptr;

{
   Pointer pntr;
   Pointer  retval;
   *ptr = newBlockPtr(size);
   if (debug_switch)
	{
	fprintf(stderr,"newBlockWithPtrs: size=%d, index=%d\n",size,(int)*ptr);
	}
   pntr =(Pointer) pointerPtr(*ptr);
   return (void *) pntr;
}


/* ************************* */
