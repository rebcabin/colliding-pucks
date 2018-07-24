/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/


/* 	
   Calculate statistics on rows of tab-separated, column-formatted data
   J. W. Ruffles, 10-88

*/

#include <stdio.h>
#include <math.h>
#include <string.h>

#define  MAXFIELDS 256
#define  BUFFERLEN 1025
#define  MAXENTRIES 10001
#define  TRUE 1
#define  FALSE 0

/* Tags for the various data types */
#define  DT_INT    'I'
#define  DT_DBL    'D'
#define  DT_STR    'S'

/* Names used to choose statistics in format input file */
#define  ST_AMEAN   "AMEAN"
#define  ST_GMEAN   "GMEAN"
#define  ST_MAX     "MAX"
#define  ST_MIN     "MIN"
#define  ST_SUM     "SUM"
#define  ST_COUNT   "COUNT"
#define  ST_KEYNAME "KEY"
#define  ST_ON      "ON"
#define  ST_OFF     "OFF"
#define  ST_ALL     "ALL"

/* Default name to use for key column */
#define KEYDEFAULT "nodes"

/* Specify which statistics are to be produced for a column */
typedef struct statflags {
    unsigned max   : 1;
    unsigned min   : 1;
    unsigned avg   : 1;
    unsigned sum   : 1;
    unsigned gmean : 1;
    unsigned num   : 1;
} STATFLAGS;
 
/* Information describing a column */
typedef struct fieldfmt {
    char name[20];		/* Field name */
    char datatype;		/* type of data expected in field */
    STATFLAGS flags;		/* statistics to produce */
} FIELDFMT;

/* Union of the various datatypes a field may take */
typedef union fielddata {
    char   *strdata;
    int     intdata;
    double  dbldata;
} FIELDDATA;
 
/* Holds running information on data seen to date in a field */
typedef struct fieldentry {
    FIELDDATA max, min, sum;
    double    lntotal;
    int       count;
} FIELDENTRY;

/* (convenient) GLOBAL VARIABLES */
FIELDENTRY *lines[MAXENTRIES];  /* matrix of data derived from input data */

FIELDFMT    fields[MAXFIELDS];	/* array of column information */

char *progname;			/* name of executing program */
int   keyfield;			/* column index which marks column used as key */
int   numfields;		/* total number of fields in data */

FILE *fpi, *fpo, *fpc;		/* file pointers: input data, output, input format */

char *strsv(str)
register char *str;
/* Sock a string away somewhere */
{
register char *ptr;

     if ( (ptr = (char *) malloc(strlen(str)+1)) != NULL)
        strcpy(ptr, str);

     return(ptr);
}

char *getNameOfInvokedFile(argv0)
char *argv0;
/* Get the name of our executable file */
{
    register char *c;

    for (c = argv0; *c != '\0'; c++);

    while (c > argv0 && *(c-1) != '/' && *(c-1) != '~')
          c--;

    return( strsv(c) );
}

char *nextsubstr(string, word)
/* Pull out the next substring in a string and reposition
   pointer to the next word after the one that's returned */
register char *string, *word;
{
register char  *next;

if (string != NULL)
{
   /* Remove leading blanks */
   while (*string == '\t' || *string == ' ' || *string == '\n')
	 string++;

   /* Copy target string */
   while (*string != '\t' && *string != '\n' && *string != '\0')
	 *word++ = *string++;

   /* Skip any trailing blanks */
   while (*string == '\t' || *string == ' ' || *string == '\n')
	 string++;
}

*word = '\0';

if (*string != '\0')
   next = string;
 else
   next = NULL; 

return(next);
}

main ( argc, argv )

    int argc;
    char *argv[];
{
    char *buff;

    char name[20];
    char keyname[20];
    char action[20];
    char buffer[BUFFERLEN];
    char scratch[BUFFERLEN];
    char pattern[BUFFERLEN];

    double adouble;

    int  anint;
    int  first;
    int  curline;
    int  inputindex;
    int  j, n;
    int  dot;

    register int i;

    STATFLAGS flags;

    progname = getNameOfInvokedFile(argv[0]);

    if ( argc != 3 && argc != 4 )
    {
	fprintf ( stderr, "usage: %s <input data file name> <output file name> [<format file name>]\n",
                 progname );
	fprintf ( stderr, "(use 'measure' to generate the measurements file)\n" );
	exit (0);
    }

    /* Open input file (columns of data) */
    if ((fpi = fopen ( argv[1], "r" )) == 0)
    {
	fprintf ( stderr, "%s: File %s not found\n", progname, argv[1] );
	exit (1);
    }

    /* Open format input file (if specified) */
    if ( argc == 4)
    {
       if ((fpc = fopen ( argv[3], "r" )) == 0) 
       {
	  fprintf ( stderr, "%s: File %s not found\n", progname, argv[3] );
	  exit (1);
       }
    }
    else
     fpc = NULL;

    /* initialize array to empty */
    for (i = 0; i < MAXENTRIES; lines[i++] = NULL);

    /* Read the first line of the measurements file (the column
       headings) and begin to set the fields data structure.   */

    /* No objects in the list yet. */
    numfields = 0;

    if ( ! fgets ( buffer, BUFFERLEN, fpi ) )
    {
       fprintf (stderr, "%s: File %s is empty.\n", progname, argv[1] );
       exit(1);
    }

    /* Parse out the column headers */
    for(buff = buffer, i = 0; buff = nextsubstr(buff, fields[i].name); i++);
    numfields = i + 1;

/* Macro to handle calls to set appropriate flags */
#define HANDLE(TEXT, FLAG)         			\
            if (! strcmp(action, TEXT) )		\
            {						\
               clear_flags(&flags);			\
               flags.FLAG = 1;				\
               set_by_name(flags, fields, buffer, 1);	\
               continue;				\
            }

    if (fpc != NULL) while(fgets(buffer, BUFFERLEN, fpc))
    {
         if (sscanf(buffer, "%s %[\t-~]", action, buffer) == 2)
         {
            if (! strcmp(action, ST_KEYNAME) )
            {
               sscanf(buffer, "%s", keyname);
	       continue;
            }
	    HANDLE(ST_AMEAN, avg);
	    HANDLE(ST_GMEAN, gmean);
	    HANDLE(ST_MAX,   max);
	    HANDLE(ST_MIN,   min);
	    HANDLE(ST_SUM,   sum);
	    HANDLE(ST_COUNT, num);
         }
    }

    if (fpc == NULL)
       strcpy(keyname, KEYDEFAULT);

    /* Find the index of the key field */
    keyfield = -1;
    pattern[0] = '\0';
    for (i = 0; i < numfields; i++)
        if (! strcmp(keyname, fields[i].name))
         {
	    keyfield = i;
	    strcat(pattern, "%d ");
            break;
	 }
	else
	 strcat(pattern, "%*s ");

    if (keyfield < 0)
    {
       fprintf(stderr, "%s: key name %s not found in column headers.\n",
	       progname, keyname);
       exit(1);
    }
                   
    first = 1;
    inputindex = 1;

    while (  fgets ( buffer, BUFFERLEN, fpi ) )
    {
       inputindex++;

       if (! sscanf(buffer, pattern, &curline ) )
       {
          fprintf(stderr, "%s: could not parse key column '%s' from line %d of %s\n",
		   fields[keyfield].name, progname, inputindex, argv[1]);
          continue;
       }

       if (first) 
       {
          /* Now read the first line of data and infer the data types of 
             the various fields.                                         */

          buff = buffer;
          for(i = 0; i < numfields; i++)
          {
 	      buff = nextsubstr(buff, scratch);

              /* Does the string contain a decimal point? */
	      for (dot = n = 0; (dot == 0) && (scratch[n] != '\0'); n++)
                  if (scratch[n] == '.')
                     dot++;

	      /* Assume weakest case - string data */
              fields[i].datatype = DT_STR;

	      /* Check for numeric data instead */
              if (dot && sscanf(scratch, "%lf", &adouble) )
                   fields[i].datatype = DT_DBL;
              else if ((! dot) && sscanf(scratch, "%d", &anint) )
                   fields[i].datatype = DT_INT;

	      if (buff == NULL)
		 break;

          }

          first = 0;

	  if (fields[keyfield].datatype != DT_INT)
	  {
	     fprintf(stderr, "%s: data type of key field '%s' is not integer!\n",
		     progname, fields[keyfield].name); 
	     exit(1);
	  }

       }

       if (curline < 0 || curline > (MAXENTRIES-1) )
       {
          fprintf(stderr,
		  "%s: on input line %d of %s, key index value %d > %d (Maximum)\n",
		  progname, inputindex, argv[1], curline, (MAXENTRIES-1) );
          continue;
       }

       /* Initiialize row if necessary */
       if (lines[curline] == NULL)
       {
          lines[curline] = (FIELDENTRY *) malloc(sizeof(FIELDENTRY) * numfields);
          
          for (i = 0; i < numfields; i++)
          {
              lines[curline][i].count = 0;
              lines[curline][i].lntotal = 0;
              switch (fields[i].datatype)
              {
                 case DT_STR :  
                                lines[curline][i].sum.strdata = NULL;
                                break;

                 case DT_DBL : 
                                lines[curline][i].sum.dbldata = 0.0;
                                break;

                 case DT_INT : 
                                lines[curline][i].sum.intdata = 0;
                                break;
              }
          }
       }

       buff = buffer;
       for (i = 0; i < numfields; i++)
       {
   
	  buff = nextsubstr(buff, scratch);

          switch (fields[i].datatype)
          {
             case DT_STR : readstr(&lines[curline][i], scratch);
			   break;

             case DT_INT : if(! readint(&lines[curline][i],scratch,fields[i].flags))
                             fprintf(stderr,
			   	     "%s: could not parse expected integer for field %d of line %d in %s\n",
				     progname, (i+1), inputindex, argv[1]);
			   break;

             case DT_DBL : if(! readdbl(&lines[curline][i],scratch,fields[i].flags))
			     fprintf(stderr,
				     "%s: could not parse expected real for field %d of line %d in %s\n",
				     progname, (i+1), inputindex, argv[1]);
			   break;
          }

	  if (buff == NULL)
	  {
             if (i < numfields-1)
                fprintf(stderr, "%s: could only parse %d columns from line %d of %s\n",
		        progname, (i+1), inputindex, argv[1]);
	     break;
	  }
       }

    } /* end while(not EOF) */

   fpo = fopen ( argv[2], "w" );   

   /* Set up default output flags */
   if (fpc == NULL)
      for (i = 0; i < numfields; i++)
      {
          clear_flags(&fields[i].flags);

          switch (fields[i].datatype)
          {
             case DT_STR : fields[i].flags.max = 1;
	  		   break;

             case DT_INT : 
			   if (keyfield == i)
			     fields[i].flags.num = 1;
			    else
			     fields[i].flags.sum = 1;
			    break;

	     case DT_DBL : fields[i].flags.sum = 1;
			    break;
          }
      }


   /* Write out a header */
   for (i = 0; i < numfields; i++)
   {
       if (i == keyfield)
          fprintf(fpo, "%s\t", fields[i].name);
       printhdr(&fields[i]);
   }
   fprintf(fpo, "\n");

   for (j = 0; j < MAXENTRIES; j++)
       if (lines[j] != NULL)
       {
          for (i = 0; i < numfields; i++)
	      switch(fields[i].datatype)
              {
                 case DT_DBL :  printdbl(fields+i, &lines[j][i]);
				break;

		 case DT_INT :  if (i == keyfield)
				   fprintf(fpo,
					   "%d\t",
					   lines[j][i].max.intdata);
				printint(fields+i, &lines[j][i]);
                                break;

		 case DT_STR :  printstr(fields+i, &lines[j][i]);
				break;

		 otherwise   :  fprintf(stderr,
				        "%s: internal failure in switch",
					progname);
				break;
  	      } 

              fprintf(fpo, "\n");
       }

   fclose(fpo);
   fclose(fpi);
   fclose(fpc);
}

printdbl(head, rec)
register FIELDFMT   *head;
register FIELDENTRY *rec;
{
if (head->flags.num)
   fprintf(fpo, "%d\t", rec->count );

if (head->flags.min)
   fprintf(fpo, "%4.2f\t", rec->min.dbldata);

if (head->flags.max)
   fprintf(fpo, "%4.2f\t", rec->max.dbldata);

if (head->flags.sum)
   fprintf(fpo, "%4.2f\t", rec->sum.dbldata);

if (head->flags.avg)
   fprintf(fpo, "%4.2f\t", (rec->sum.dbldata / rec->count ) );

if (head->flags.gmean)
   fprintf(fpo, "%4.2f\t", exp(rec->lntotal / rec->count) );
}

printint(head, rec)
register FIELDFMT   *head;
register FIELDENTRY *rec;
{
if (head->flags.num)
   fprintf(fpo, "%d\t", rec->count );

if (head->flags.min)
   fprintf(fpo, "%d\t", rec->min.intdata);

if (head->flags.max)
   fprintf(fpo, "%d\t", rec->max.intdata);

if (head->flags.sum)
   fprintf(fpo, "%d\t", rec->sum.intdata);

if (head->flags.avg)
   fprintf(fpo, "%3.1f\t", ((double) rec->sum.intdata) / rec->count );

if (head->flags.gmean)
   fprintf(fpo, "%3.1f\t", exp(((double) rec->lntotal) / rec->count) );
}

printstr(head, rec)
register FIELDFMT   *head;
register FIELDENTRY *rec;
{
if (head->flags.num)
   fprintf(fpo, "%d\t", rec->count );

if (head->flags.min)
   fprintf(fpo, "%s\t", rec->min.strdata);

if (head->flags.max)
   fprintf(fpo, "%s\t", rec->max.strdata);
}

/* Trap nasty error msgs from log() */
/* PJH  10-25-89 This function will not compile under MACH */
/*	and since it is not called anywhere, we comment it */
/*	out for now...	
matherr(x)
register struct exception *x;
{
   if ((x->type) == SING)
      return(TRUE);
    else
      return(FALSE);
}
	*/

readstr(rec, string)
register FIELDENTRY *rec;
register char *string;
{
 if (rec->count == 0)
    rec->min.strdata = rec->max.strdata  = rec->sum.strdata = strsv(string);
 else
 {
    if (strcmp(string, rec->min.strdata) < 0)
       rec->min.strdata = strsv(string);
    if (strcmp(string, rec->max.strdata) > 0)
       rec->max.strdata = strsv(string);
 }
 rec->count++;
}

readint(rec, string, flags)
register FIELDENTRY *rec;
char *string;
register STATFLAGS flags;
{
int anint;

if (sscanf(string, "%d", &anint))
 {
   if (rec->count == 0)
    {
      rec->min.intdata = anint;
      rec->max.intdata = anint;
      rec->sum.intdata = anint;
    }
   else
    {
      if (anint < rec->min.intdata)
         rec->min.intdata = anint;
      if (anint > rec->max.intdata)
         rec->max.intdata = anint;
      rec->sum.intdata += anint;
    }

   rec->count++;
   if (flags.gmean)
      rec->lntotal += log( (double) anint);
   return(TRUE);
 }
else
   return(FALSE);
}

readdbl(rec, string, flags)
register FIELDENTRY *rec;
char *string;
register STATFLAGS flags;
{
double adouble;
if (sscanf(string, "%lf", &adouble))
  {
   if (rec->count == 0)
   {
      rec->min.dbldata = adouble;
      rec->max.dbldata = adouble;
      rec->sum.dbldata = adouble;
   }
   else
   {
      if (adouble < rec->min.dbldata)
         rec->min.dbldata = adouble;
      if (adouble > rec->max.dbldata)
         rec->max.dbldata = adouble;
      rec->sum.dbldata += adouble;
   }

   rec->count++;
   if (flags.gmean)
      rec->lntotal += log(adouble);

   return(TRUE);
  }
 else
   return(FALSE);
}

printhdr(head)
register FIELDFMT   *head;
{
if (head->flags.num)
   fprintf(fpo, "ct:%s\t", head->name );

if (head->flags.min)
   fprintf(fpo, "mn:%s\t", head->name );

if (head->flags.max)
   fprintf(fpo, "mx:%s\t", head->name );

if (head->flags.sum)
   fprintf(fpo, "sm:%s\t", head->name );

if (head->flags.avg)
   fprintf(fpo, "am:%s\t", head->name );

if (head->flags.gmean)
   fprintf(fpo, "gm:%s\t", head->name );
}

clear_flags(flagptr)
register STATFLAGS *flagptr;
{
 flagptr->max   = 
 flagptr->min   = 
 flagptr->avg   = 
 flagptr->sum   = 
 flagptr->gmean = 
 flagptr->num   =  0;
}

set_by_name(flags, fields, string, value)
STATFLAGS flags;
register FIELDFMT  *fields;
char      *string;
int        value;
{
register i;
int   n;
char  name[100];

while (n = sscanf(string, "%s %[\t-~]", name, string))
   {
      for (i = 0; i < numfields; i++)
      {
          if (! strcmp(name, ST_ON))
          {
             value = 1;
             break;
          }

          if (! strcmp(name, ST_OFF))
          {
             value = 0;
             break;
          }

          if (! strcmp(name, ST_ALL))
          {
             set_all(flags, fields, value);
             return;
          }

          if (! strcmp(name, fields[i].name))
          {
             if (flags.max)
                fields[i].flags.max = value;
 
             if (flags.min)
                fields[i].flags.min = value;
 
             if (flags.avg)
                fields[i].flags.avg = value;
 
             if (flags.sum)
                fields[i].flags.sum = value;
 
             if (flags.gmean)
                fields[i].flags.gmean = value;
 
             if (flags.num)
                fields[i].flags.num = value;
             break;
          }
      }
      if (i == numfields)
         fprintf(stderr, "%s: field '%s' not found.\n",
			 progname, name);
      if (n == 1)
         break;
   }
}

set_all(flags, fields, value)
STATFLAGS flags;
FIELDFMT  *fields;
int        value;
{
register i;

for (i = 0; i < numfields; i++)
 {
    if (flags.max)
       fields[i].flags.max = value;
 
    if (flags.min)
       fields[i].flags.min = value;
 
    if (flags.avg)
       fields[i].flags.avg = value;
 
    if (flags.sum)
       fields[i].flags.sum = value;
 
    if (flags.gmean)
       fields[i].flags.gmean = value;
 
    if (flags.num)
       fields[i].flags.num = value;
 }
}
 
