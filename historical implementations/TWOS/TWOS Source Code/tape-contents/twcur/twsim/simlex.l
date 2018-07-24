/**************************************************************************
*   simlexO.l
* lexical analyzer for timewarp simulator
* based on version of same by LVW.
*   JJW 12/18/87  update  for files 3/15/88
*  fix get_string to copy all (incl nulls) between double quotes 
*  and use msgtextlen for size  ###   JJW 7/12/88
*  add include files recognized by @    jjw 2/18/89
*****************************************************************************/

%a 4000

%{
#include "simpar.h"
extern char *calloc();
#undef input
char mygetc();
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):\
mygetc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)

extern long atol();
extern double atof();
extern YYSTYPE yylval;
extern int displ_sw;
static char floatid[] = "%W%\t%G%";
int msgtextlen = 0;     /* length of evtmesg from config file  ### */
%}

D [0-9]
E [Ee][-+]?{D}+

%%

^[ \t\n]		{}
^\*			{}
^obcreate		{return(OBCR_TKN);}
^timeval		{return(TIMEV_TKN);}
^timechg		{return(TIMECHG_TKN);}
tell			{return(MSG_TKN);}
schedule		{return(MSG_TKN);}
^rmint			{return(RMINT_TKN);}
^go			{return(GO_TKN);}
maxacks			{return(MACKS_TKN);}
^objend			{return(END_TKN);}
monon			{return(MONON_TKN);}
monoff			{return(MONOFF_TKN);}
objstksize		{return(OBJSTKSIZE_TKN);}
cutoff			{return(CUTOFF_TKN);}
^fileecho		{return(DISP_TKN);}
^getfile		{return(GETF_TKN);}
delfile			{return(DELFILE_TKN);}
^putfile		{return(PUTF_TKN);}
nostdout		{return(NOSTDT_TKN);}
pktlen			{return(PKTLEN_TKN);}
islog			{return(ISLOG_TKN);}
dlm			{return(DLM_TKN);}
idledlm			{return(IDLEDLM_TKN);}
maxoff			{return(MAXOFF_TKN);}
migrations		{return(MIGRATIONS_TKN);}
dlmint			{return(DLMINT_TKN);}
objstats		{return(OBSTATS_TKN);}
typeinit		{return(TPINIT_TKN);}
maxmsgpool		{return(MAXPOOL_TKN);}
allownow		{return(ALLOWNOW_TKN);}
^@			{return(NEST_TKN);}


-?{D}+ {
        yylval.int_val = atoi(yytext);
        return (INT_TKN);
    }

-?{D}+[lL] {
        yylval.long_val = atol(yytext);
        return (LONG_TKN);
    }

-?{D}*"."{D}+({E})?     |
-?{D}+"."{D}*({E})?     |
-?{D}+"."{D}+({E})?     {
                
	yylval.dbl_val = atof(yytext);
	return(DOUBLE_TKN);
    }

"#" {
        /* Call input until a newline is found. */
        get_comment();
	return(NUL_TKN);
    }

\" {
        /* Call input until a closing double quote is found. */
        get_string();
        return(STR_TKN);
    }
[/A-Za_z_][/A-Za_z_0-9]* |

[\./A-Za-z_][\./A-Za-z_0-9]* {
        yylval.str_val = calloc(yyleng + 1, sizeof(char));
        strncpy (yylval.str_val, yytext, yyleng);
	msgtextlen = yyleng;			/* ### */
        return (STR_TKN);
    }

[ \t]		{}

[ \t\n]		 {return(NUL_TKN);}


%%
/* rfget			{return(RFGET_TKN);} */

yywrap()
{
	extern int command_num;
	extern int error_count;
	extern int warning_count;
	extern int nesting;
	extern FILE *oldfildes;
	extern FILE *yyin;
	extern  char *cfg_bptr;
	extern char inbuf[];

	if (! nesting)
	{
/*	printf("Simulation configured \n");  */
	HOST_fclose(yyin);
	return(1);
	}
	else
    {
    HOST_fclose(yyin);
    yyin = oldfildes;
    nesting = 0;
    cfg_bptr = inbuf;
    if (displ_sw == 1)
	      printf("include file EOF \n");
    command_num++;
    return(0);
    }

}

get_comment()
{
    char c;
    int n;
    char  txt[80];

    n = 0;
    while( (c = input()) != '\n' )
    {
	txt[n] = c;
	n++;       
    }
    txt[n] = 0;
    if (displ_sw) printf("#%s\n",txt);
    unput('\n');
    return(NUL_TKN);
}

/* ******************************************************************* */
/* These defines necessary for lexical analyser and parser statistics. */
/* ******************************************************************* */
int command_num = 0;
int error_count = 0;
int warning_count = 0;
/*************************************************************************/

get_string()
{
    int  i;
    char c;
    char *p;
    char stringbuf[512];

    for(i = 0; i < 512 ;i++)
    {
        stringbuf[i] = '\0';
    }

    for(i = 0; i < 511 ;i++)
    {
         c = input();

        if(c == '"')
        {
            break;
        }
        else
        {
            stringbuf[i] = c;
        }
    }

    yylval.str_val = (char *)calloc(i + 1, sizeof(char));
    msgtextlen = i;
    p = yylval.str_val;
    for(i = 0; i < msgtextlen ;i++)	/* ### */
       *p++ = stringbuf[i];
    *p = stringbuf[i];
}

/* *********************** */
/* Error recovery routine. */
/* *********************** */
yyerror(message) char *message ;
{
    fprintf
	(
		stderr,
		"    Error, Command %d, Line %d:      %s\n",
		command_num +1, yylineno , message
	);
	error_count++;
}

/* *********************** */
/* Warning        routine. */
/* *********************** */
yywarn(message) char *message ;
{
    fprintf
	(
		stderr,
		"  Warning, Command %d, Line %d:      %s\n",
		command_num + 1, yylineno , message
	);
	warning_count++;
}


