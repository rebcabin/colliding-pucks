extern char *malloc(), *realloc();

# line 11 "../simpar.y"
#ifdef SUN
#include <sys/types.h>
#endif SUN

static char simparid[] = "%W%\t%G%";
extern int command_num;
extern int yXylineno; 
extern int no_stdout;
extern int displ_sw;
extern get_errline();
extern char yXytext[];

# line 24 "../simpar.y"
typedef union 
{
    int     int_val; /* Integer valued tokens. */
    long   long_val; /* Long    valued tokens. */
    double  dbl_val; /* Double  valued tokens. */
    char *  str_val; /* String  valued tokens. */ 
} YxYSTYPE;
# define NUL_TKN 257
# define NEST_TKN 258
# define OBCR_TKN 259
# define TIMEV_TKN 260
# define TIMECHG_TKN 261
# define MSG_TKN 262
# define RMINT_TKN 263
# define GO_TKN 264
# define NOSTDT_TKN 265
# define MACKS_TKN 266
# define END_TKN 267
# define MONON_TKN 268
# define MONOFF_TKN 269
# define OBJSTKSIZE_TKN 270
# define CUTOFF_TKN 271
# define DISP_TKN 272
# define GETF_TKN 273
# define PUTF_TKN 274
# define INT_TKN 275
# define LONG_TKN 276
# define DOUBLE_TKN 277
# define STR_TKN 278
# define PKTLEN_TKN 279
# define ISLOG_TKN 280
# define RFGET_TKN 281
# define DLM_TKN 282
# define IDLEDLM_TKN 283
# define MAXOFF_TKN 284
# define MIGRATIONS_TKN 285
# define DLMINT_TKN 286
# define OBSTATS_TKN 287
# define DELFILE_TKN 288
# define TPINIT_TKN 289
# define MAXPOOL_TKN 290
# define ALLOWNOW_TKN 291
#define yXyclearin yXychar = -1
#define yXyerrok yXyerrflag = 0
extern int yXychar;
extern int yXyerrflag;
#ifndef YxYMAXDEPTH
#define YxYMAXDEPTH 150
#endif
YxYSTYPE yXylval, yXyval;
# define YxYERRCODE 256

# line 521 "../simpar.y"

int yXyexca[] ={
-1, 0,
	0, 1,
	-2, 0,
-1, 1,
	0, -1,
	-2, 0,
	};
# define YxYNPROD 69
# define YxYLAST 146
int yXyact[]={

     4,     2,    59,    36,    37,    38,    39,    40,    41,    42,
    43,    66,    44,    45,    46,    47,    48,    52,    53,    62,
    96,    61,    35,    49,    50,    51,    54,    55,    56,    58,
    57,    60,   145,    63,    65,    64,    95,   140,   139,   138,
   124,   123,   122,   115,   114,   113,   106,   104,   101,   105,
    34,    97,    93,    87,    86,    85,    81,    73,    80,   144,
    72,    69,    94,    70,   133,   129,   128,   127,   126,   112,
    99,    92,    91,    90,    89,    84,    83,    79,    76,    71,
   146,   143,   142,   141,   137,   136,   135,   134,   132,   131,
   130,   125,   121,   120,   119,   118,   117,   116,   111,   110,
   109,   108,   107,   103,   102,   100,    98,    88,    82,    78,
    77,    75,    74,    68,    67,    33,    32,    31,    30,    29,
    28,    27,    26,    25,    24,    23,    22,    21,    20,    19,
    18,    17,    16,    15,    14,    13,    12,    11,    10,     9,
     8,     7,     6,     5,     1,     3 };
int yXypact[]={

  -256, -1000, -1000,  -143,  -144, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000,  -217,  -214,  -196,  -218,
  -221,  -145,  -146,  -197,  -147,  -148,  -198,  -219,  -149,  -199,
  -200,  -223,  -224,  -225,  -150,  -201,  -202,  -203,  -204,  -226,
  -215,  -252,  -268,  -227,  -151,  -205,  -152, -1000, -1000,  -230,
  -153,  -154,  -228,  -232, -1000, -1000,  -155, -1000, -1000,  -156,
  -157,  -158, -1000,  -159,  -206,  -233,  -234,  -235, -1000,  -160,
  -161,  -162,  -163,  -164,  -165,  -236,  -237,  -238, -1000,  -166,
 -1000,  -207, -1000, -1000,  -208,  -209,  -210, -1000, -1000, -1000,
 -1000, -1000,  -167,  -168,  -169,  -211, -1000, -1000, -1000, -1000,
 -1000, -1000,  -170,  -171,  -172, -1000,  -173,  -239,  -240,  -241,
 -1000, -1000, -1000,  -174, -1000, -1000, -1000, -1000,  -175,  -176,
  -216, -1000, -1000, -1000,  -246,  -177, -1000 };
int yXypgo[]={

     0,   145,   144,   143,   142,   141,   140,   139,   138,   137,
   136,   135,   134,   133,   132,   131,   130,   129,   128,   127,
   126,   125,   124,   123,   122,   121,   120,   119,   118,   117,
   116,   115,    50 };
int yXyr1[]={

     0,     2,     2,     2,     2,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     3,    29,    30,    31,
    16,    27,    17,    18,    26,     6,     6,    32,     9,    13,
    14,    14,    15,    19,    20,    28,    28,     5,     4,     8,
    10,    11,     7,    12,    21,    22,    23,    24,    25 };
int yXyr2[]={

     0,     1,     3,     4,     5,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,    11,     9,     5,     7,
     7,     7,     9,     9,     7,    13,    13,     5,     5,     7,
     7,     7,     5,     9,    11,     9,     9,     7,     7,     5,
     7,     5,    17,     5,     5,     7,     7,     7,     7 };
int yXychk[]={

 -1000,    -2,   257,    -1,   256,    -3,    -4,    -5,    -6,    -7,
    -8,    -9,   -10,   -11,   -12,   -13,   -14,   -15,   -16,   -17,
   -18,   -19,   -20,   -21,   -22,   -23,   -24,   -25,   -26,   -27,
   -28,   -29,   -30,   -31,   -32,   278,   259,   260,   261,   262,
   263,   264,   265,   266,   268,   269,   270,   271,   272,   279,
   280,   281,   273,   274,   282,   283,   284,   286,   285,   258,
   287,   277,   275,   289,   291,   290,   267,   257,   257,   278,
   277,   275,   278,   278,   257,   257,   275,   257,   257,   275,
   277,   275,   257,   275,   275,   278,   278,   278,   257,   275,
   275,   275,   275,   278,   277,   288,   288,   278,   257,   275,
   257,   278,   257,   257,   275,   277,   278,   257,   257,   257,
   257,   257,   275,   278,   278,   278,   257,   257,   257,   257,
   257,   257,   278,   278,   278,   257,   275,   275,   275,   275,
   257,   257,   257,   275,   257,   257,   257,   257,   278,   278,
   278,   257,   257,   257,   275,   278,   257 };
int yXydef[]={

    -2,    -2,     2,     0,     0,     5,     6,     7,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    34,    35,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     3,     4,     0,
     0,     0,     0,     0,    59,    48,     0,    61,    63,     0,
     0,     0,    52,     0,     0,     0,     0,     0,    64,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    38,     0,
    47,     0,    58,    57,     0,     0,     0,    60,    49,    50,
    51,    40,     0,     0,     0,     0,    65,    66,    67,    68,
    44,    41,     0,     0,     0,    39,     0,     0,     0,     0,
    42,    43,    53,     0,    55,    56,    37,    36,     0,     0,
     0,    54,    45,    46,     0,     0,    62 };
typedef struct { char *t_name; int t_val; } yXytoktype;
#ifndef YxYDEBUG
#	define YxYDEBUG	0	/* don't allow debugging */
#endif

#if YxYDEBUG

yXytoktype yXytoks[] =
{
	"NUL_TKN",	257,
	"NEST_TKN",	258,
	"OBCR_TKN",	259,
	"TIMEV_TKN",	260,
	"TIMECHG_TKN",	261,
	"MSG_TKN",	262,
	"RMINT_TKN",	263,
	"GO_TKN",	264,
	"NOSTDT_TKN",	265,
	"MACKS_TKN",	266,
	"END_TKN",	267,
	"MONON_TKN",	268,
	"MONOFF_TKN",	269,
	"OBJSTKSIZE_TKN",	270,
	"CUTOFF_TKN",	271,
	"DISP_TKN",	272,
	"GETF_TKN",	273,
	"PUTF_TKN",	274,
	"INT_TKN",	275,
	"LONG_TKN",	276,
	"DOUBLE_TKN",	277,
	"STR_TKN",	278,
	"PKTLEN_TKN",	279,
	"ISLOG_TKN",	280,
	"RFGET_TKN",	281,
	"DLM_TKN",	282,
	"IDLEDLM_TKN",	283,
	"MAXOFF_TKN",	284,
	"MIGRATIONS_TKN",	285,
	"DLMINT_TKN",	286,
	"OBSTATS_TKN",	287,
	"DELFILE_TKN",	288,
	"TPINIT_TKN",	289,
	"MAXPOOL_TKN",	290,
	"ALLOWNOW_TKN",	291,
	"-unknown-",	-1	/* ends search */
};

char * yXyreds[] =
{
	"-no such reduction-",
	"command : /* empty */",
	"command : NUL_TKN",
	"command : line NUL_TKN",
	"command : error NUL_TKN",
	"line : obcr_command",
	"line : timev_command",
	"line : timechg_command",
	"line : tell_command",
	"line : rmint_command",
	"line : go_command",
	"line : nostdout_command",
	"line : maxacks_command",
	"line : monon_command",
	"line : monoff_command",
	"line : objstksize_command",
	"line : cutoff_command",
	"line : display_command",
	"line : pktlen_command",
	"line : islog_command",
	"line : rfget_command",
	"line : getfile_command",
	"line : putfile_command",
	"line : dlm_command",
	"line : idledlm_command",
	"line : maxoff_command",
	"line : dlmint_command",
	"line : migrations_command",
	"line : nest_command",
	"line : objstats_command",
	"line : delfile_command",
	"line : type_init_command",
	"line : allow_now_command",
	"line : maxmsgpool_command",
	"line : end_command",
	"line : STR_TKN",
	"obcr_command : OBCR_TKN STR_TKN STR_TKN INT_TKN NUL_TKN",
	"type_init_command : TPINIT_TKN STR_TKN STR_TKN NUL_TKN",
	"allow_now_command : ALLOWNOW_TKN NUL_TKN",
	"maxmsgpool_command : MAXPOOL_TKN INT_TKN NUL_TKN",
	"pktlen_command : PKTLEN_TKN INT_TKN NUL_TKN",
	"objstats_command : OBSTATS_TKN DOUBLE_TKN NUL_TKN",
	"islog_command : ISLOG_TKN INT_TKN INT_TKN NUL_TKN",
	"rfget_command : RFGET_TKN STR_TKN STR_TKN NUL_TKN",
	"nest_command : NEST_TKN STR_TKN NUL_TKN",
	"tell_command : MSG_TKN STR_TKN INT_TKN INT_TKN STR_TKN NUL_TKN",
	"tell_command : MSG_TKN STR_TKN DOUBLE_TKN INT_TKN STR_TKN NUL_TKN",
	"end_command : END_TKN NUL_TKN",
	"nostdout_command : NOSTDT_TKN NUL_TKN",
	"objstksize_command : OBJSTKSIZE_TKN INT_TKN NUL_TKN",
	"cutoff_command : CUTOFF_TKN DOUBLE_TKN NUL_TKN",
	"cutoff_command : CUTOFF_TKN INT_TKN NUL_TKN",
	"display_command : DISP_TKN NUL_TKN",
	"getfile_command : GETF_TKN STR_TKN STR_TKN NUL_TKN",
	"putfile_command : PUTF_TKN STR_TKN STR_TKN INT_TKN NUL_TKN",
	"delfile_command : DOUBLE_TKN DELFILE_TKN STR_TKN NUL_TKN",
	"delfile_command : INT_TKN DELFILE_TKN STR_TKN NUL_TKN",
	"timechg_command : TIMECHG_TKN INT_TKN NUL_TKN",
	"timev_command : TIMEV_TKN DOUBLE_TKN NUL_TKN",
	"go_command : GO_TKN NUL_TKN",
	"maxacks_command : MACKS_TKN INT_TKN NUL_TKN",
	"monon_command : MONON_TKN NUL_TKN",
	"rmint_command : RMINT_TKN STR_TKN STR_TKN INT_TKN STR_TKN INT_TKN STR_TKN NUL_TKN",
	"monoff_command : MONOFF_TKN NUL_TKN",
	"dlm_command : DLM_TKN NUL_TKN",
	"idledlm_command : IDLEDLM_TKN INT_TKN NUL_TKN",
	"maxoff_command : MAXOFF_TKN INT_TKN NUL_TKN",
	"dlmint_command : DLMINT_TKN INT_TKN NUL_TKN",
	"migrations_command : MIGRATIONS_TKN INT_TKN NUL_TKN",
};
#endif /* YxYDEBUG */
#line 1 "/usr/lib/yaccpar"
/*	@(#)yaccpar 1.10 89/04/04 SMI; from S5R3 1.10	*/

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YxYERROR		goto yXyerrlab
#define YxYACCEPT	{ free(yXys); free(yXyv); return(0); }
#define YxYABORT		{ free(yXys); free(yXyv); return(1); }
#define YxYBACKUP( newtoken, newvalue )\
{\
	if ( yXychar >= 0 || ( yXyr2[ yXytmp ] >> 1 ) != 1 )\
	{\
		yXyerror( "syntax error - cannot backup" );\
		goto yXyerrlab;\
	}\
	yXychar = newtoken;\
	yXystate = *yXyps;\
	yXylval = newvalue;\
	goto yXynewstate;\
}
#define YxYRECOVERING()	(!!yXyerrflag)
#ifndef YxYDEBUG
#	define YxYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yXydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YxYFLAG		(-1000)

/*
** static variables used by the parser
*/
static YxYSTYPE *yXyv;			/* value stack */
static int *yXys;			/* state stack */

static YxYSTYPE *yXypv;			/* top of value stack */
static int *yXyps;			/* top of state stack */

static int yXystate;			/* current state */
static int yXytmp;			/* extra var (lasts between blocks) */

int yXynerrs;			/* number of errors */

int yXyerrflag;			/* error recovery flag */
int yXychar;			/* current input token number */


/*
** yXyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yXyparse()
{
	register YxYSTYPE *yXypvt;	/* top of value stack for $vars */
	unsigned yXymaxdepth = YxYMAXDEPTH;

	/*
	** Initialize externals - yXyparse may be called more than once
	*/
	yXyv = (YxYSTYPE*)malloc(yXymaxdepth*sizeof(YxYSTYPE));
	yXys = (int*)malloc(yXymaxdepth*sizeof(int));
	if (!yXyv || !yXys)
	{
		yXyerror( "out of memory" );
		return(1);
	}
	yXypv = &yXyv[-1];
	yXyps = &yXys[-1];
	yXystate = 0;
	yXytmp = 0;
	yXynerrs = 0;
	yXyerrflag = 0;
	yXychar = -1;

	goto yXystack;
	{
		register YxYSTYPE *yXy_pv;	/* top of value stack */
		register int *yXy_ps;		/* top of state stack */
		register int yXy_state;		/* current state */
		register int  yXy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YxYBACKUP was called.
		*/
	yXynewstate:
		yXy_pv = yXypv;
		yXy_ps = yXyps;
		yXy_state = yXystate;
		goto yXy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yXystack:
		yXy_pv = yXypv;
		yXy_ps = yXyps;
		yXy_state = yXystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yXy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YxYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yXydebug )
		{
			register int yXy_i;

			(void)printf( "State %d, token ", yXy_state );
			if ( yXychar == 0 )
				(void)printf( "end-of-file\n" );
			else if ( yXychar < 0 )
				(void)printf( "-none-\n" );
			else
			{
				for ( yXy_i = 0; yXytoks[yXy_i].t_val >= 0;
					yXy_i++ )
				{
					if ( yXytoks[yXy_i].t_val == yXychar )
						break;
				}
				(void)printf( "%s\n", yXytoks[yXy_i].t_name );
			}
		}
#endif /* YxYDEBUG */
		if ( ++yXy_ps >= &yXys[ yXymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			int yXyps_index = (yXy_ps - yXys);
			int yXypv_index = (yXy_pv - yXyv);
			int yXypvt_index = (yXypvt - yXyv);
			yXymaxdepth += YxYMAXDEPTH;
			yXyv = (YxYSTYPE*)realloc((char*)yXyv,
				yXymaxdepth * sizeof(YxYSTYPE));
			yXys = (int*)realloc((char*)yXys,
				yXymaxdepth * sizeof(int));
			if (!yXyv || !yXys)
			{
				yXyerror( "yacc stack overflow" );
				return(1);
			}
			yXy_ps = yXys + yXyps_index;
			yXy_pv = yXyv + yXypv_index;
			yXypvt = yXyv + yXypvt_index;
		}
		*yXy_ps = yXy_state;
		*++yXy_pv = yXyval;

		/*
		** we have a new state - find out what to do
		*/
	yXy_newstate:
		if ( ( yXy_n = yXypact[ yXy_state ] ) <= YxYFLAG )
			goto yXydefault;		/* simple state */
#if YxYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yXytmp = yXychar < 0;
#endif
		if ( ( yXychar < 0 ) && ( ( yXychar = yXylex() ) < 0 ) )
			yXychar = 0;		/* reached EOF */
#if YxYDEBUG
		if ( yXydebug && yXytmp )
		{
			register int yXy_i;

			(void)printf( "Received token " );
			if ( yXychar == 0 )
				(void)printf( "end-of-file\n" );
			else if ( yXychar < 0 )
				(void)printf( "-none-\n" );
			else
			{
				for ( yXy_i = 0; yXytoks[yXy_i].t_val >= 0;
					yXy_i++ )
				{
					if ( yXytoks[yXy_i].t_val == yXychar )
						break;
				}
				(void)printf( "%s\n", yXytoks[yXy_i].t_name );
			}
		}
#endif /* YxYDEBUG */
		if ( ( ( yXy_n += yXychar ) < 0 ) || ( yXy_n >= YxYLAST ) )
			goto yXydefault;
		if ( yXychk[ yXy_n = yXyact[ yXy_n ] ] == yXychar )	/*valid shift*/
		{
			yXychar = -1;
			yXyval = yXylval;
			yXy_state = yXy_n;
			if ( yXyerrflag > 0 )
				yXyerrflag--;
			goto yXy_stack;
		}

	yXydefault:
		if ( ( yXy_n = yXydef[ yXy_state ] ) == -2 )
		{
#if YxYDEBUG
			yXytmp = yXychar < 0;
#endif
			if ( ( yXychar < 0 ) && ( ( yXychar = yXylex() ) < 0 ) )
				yXychar = 0;		/* reached EOF */
#if YxYDEBUG
			if ( yXydebug && yXytmp )
			{
				register int yXy_i;

				(void)printf( "Received token " );
				if ( yXychar == 0 )
					(void)printf( "end-of-file\n" );
				else if ( yXychar < 0 )
					(void)printf( "-none-\n" );
				else
				{
					for ( yXy_i = 0;
						yXytoks[yXy_i].t_val >= 0;
						yXy_i++ )
					{
						if ( yXytoks[yXy_i].t_val
							== yXychar )
						{
							break;
						}
					}
					(void)printf( "%s\n", yXytoks[yXy_i].t_name );
				}
			}
#endif /* YxYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yXyxi = yXyexca;

				while ( ( *yXyxi != -1 ) ||
					( yXyxi[1] != yXy_state ) )
				{
					yXyxi += 2;
				}
				while ( ( *(yXyxi += 2) >= 0 ) &&
					( *yXyxi != yXychar ) )
					;
				if ( ( yXy_n = yXyxi[1] ) < 0 )
					YxYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yXy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yXyerrflag )
			{
			case 0:		/* new error */
				yXyerror( "syntax error" );
				goto skip_init;
			yXyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yXy_pv = yXypv;
				yXy_ps = yXyps;
				yXy_state = yXystate;
				yXynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yXyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yXy_ps >= yXys )
				{
					yXy_n = yXypact[ *yXy_ps ] + YxYERRCODE;
					if ( yXy_n >= 0 && yXy_n < YxYLAST &&
						yXychk[yXyact[yXy_n]] == YxYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yXy_state = yXyact[ yXy_n ];
						goto yXy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YxYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yXydebug )
						(void)printf( _POP_, *yXy_ps,
							yXy_ps[-1] );
#	undef _POP_
#endif
					yXy_ps--;
					yXy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YxYABORT;
			case 3:		/* no shift yet; eat a token */
#if YxYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yXydebug )
				{
					register int yXy_i;

					(void)printf( "Error recovery discards " );
					if ( yXychar == 0 )
						(void)printf( "token end-of-file\n" );
					else if ( yXychar < 0 )
						(void)printf( "token -none-\n" );
					else
					{
						for ( yXy_i = 0;
							yXytoks[yXy_i].t_val >= 0;
							yXy_i++ )
						{
							if ( yXytoks[yXy_i].t_val
								== yXychar )
							{
								break;
							}
						}
						(void)printf( "token %s\n",
							yXytoks[yXy_i].t_name );
					}
				}
#endif /* YxYDEBUG */
				if ( yXychar == 0 )	/* reached EOF. quit */
					YxYABORT;
				yXychar = -1;
				goto yXy_newstate;
			}
		}/* end if ( yXy_n == 0 ) */
		/*
		** reduction by production yXy_n
		** put stack tops, etc. so things right after switch
		*/
#if YxYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yXydebug )
			(void)printf( "Reduce by (%d) \"%s\"\n",
				yXy_n, yXyreds[ yXy_n ] );
#endif
		yXytmp = yXy_n;			/* value to switch over */
		yXypvt = yXy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yXy_state here as temporary
		** register variable, but why not, if it works...
		** If yXyr2[ yXy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yXy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yXy_len = yXyr2[ yXy_n ];

			if ( !( yXy_len & 01 ) )
			{
				yXy_len >>= 1;
				yXyval = ( yXy_pv -= yXy_len )[1];	/* $$ = $1 */
				yXy_state = yXypgo[ yXy_n = yXyr1[ yXy_n ] ] +
					*( yXy_ps -= yXy_len ) + 1;
				if ( yXy_state >= YxYLAST ||
					yXychk[ yXy_state =
					yXyact[ yXy_state ] ] != -yXy_n )
				{
					yXy_state = yXyact[ yXypgo[ yXy_n ] ];
				}
				goto yXy_stack;
			}
			yXy_len >>= 1;
			yXyval = ( yXy_pv -= yXy_len )[1];	/* $$ = $1 */
			yXy_state = yXypgo[ yXy_n = yXyr1[ yXy_n ] ] +
				*( yXy_ps -= yXy_len ) + 1;
			if ( yXy_state >= YxYLAST ||
				yXychk[ yXy_state = yXyact[ yXy_state ] ] != -yXy_n )
			{
				yXy_state = yXyact[ yXypgo[ yXy_n ] ];
			}
		}
					/* save until reenter driver code */
		yXystate = yXy_state;
		yXyps = yXy_ps;
		yXypv = yXy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yXytmp )
	{
		
case 1:
# line 75 "../simpar.y"
{
        return(1);
	} break;
case 2:
# line 79 "../simpar.y"
{
        return(0);
	} break;
case 4:
# line 84 "../simpar.y"
{
       yXyerrok;
	yXyclearin;
        return(0);
    } break;
case 5:
# line 91 "../simpar.y"
{ return(0); } break;
case 6:
# line 92 "../simpar.y"
{ return(0); } break;
case 7:
# line 93 "../simpar.y"
{ return(0); } break;
case 8:
# line 94 "../simpar.y"
{ return(0); } break;
case 9:
# line 95 "../simpar.y"
{ return(0); } break;
case 10:
# line 96 "../simpar.y"
{ return(0); } break;
case 11:
# line 97 "../simpar.y"
{ return(0); } break;
case 12:
# line 98 "../simpar.y"
{ return(0); } break;
case 13:
# line 99 "../simpar.y"
{ return(0); } break;
case 14:
# line 100 "../simpar.y"
{ return(0); } break;
case 15:
# line 101 "../simpar.y"
{ return(0); } break;
case 16:
# line 102 "../simpar.y"
{ return(0); } break;
case 17:
# line 103 "../simpar.y"
{ return(0); } break;
case 18:
# line 104 "../simpar.y"
{ return(0); } break;
case 19:
# line 105 "../simpar.y"
{ return(0); } break;
case 20:
# line 106 "../simpar.y"
{ return(0); } break;
case 21:
# line 107 "../simpar.y"
{ return(0); } break;
case 22:
# line 108 "../simpar.y"
{ return(0); } break;
case 23:
# line 109 "../simpar.y"
{return(0); } break;
case 24:
# line 110 "../simpar.y"
{return(0); } break;
case 25:
# line 111 "../simpar.y"
{return(0); } break;
case 26:
# line 112 "../simpar.y"
{return(0); } break;
case 27:
# line 113 "../simpar.y"
{return(0); } break;
case 28:
# line 114 "../simpar.y"
{ return(0); } break;
case 29:
# line 115 "../simpar.y"
{ return(0); } break;
case 30:
# line 116 "../simpar.y"
{ return(0); } break;
case 31:
# line 117 "../simpar.y"
{ return(0); } break;
case 32:
# line 118 "../simpar.y"
{ return(0); } break;
case 33:
# line 119 "../simpar.y"
{return(0); } break;
case 34:
# line 120 "../simpar.y"
{ return(1); } break;
case 35:
# line 123 "../simpar.y"
{
	printf("syntax error, first token line %d\n", yXylineno);
	YxYERROR;
    } break;
case 36:
# line 137 "../simpar.y"
{
        config_obj(yXypvt[-3].str_val, yXypvt[-2].str_val, yXypvt[-1].int_val);
	command_num++;
    } break;
case 37:
# line 150 "../simpar.y"
{
	init_type(yXypvt[-2].str_val,yXypvt[-1].str_val);
	command_num++;
    } break;
case 38:
# line 163 "../simpar.y"
{
	allow_now_msgs();
	command_num++;
    } break;
case 39:
# line 176 "../simpar.y"
{
       set_maxmsgpool(yXypvt[-1].int_val);
       command_num++;
    } break;
case 40:
# line 190 "../simpar.y"
{
	pktlen( yXypvt[-1].int_val );
	command_num++;
    } break;
case 41:
# line 203 "../simpar.y"
{
	objstats(yXypvt[-1].dbl_val);
	command_num++;
    } break;
case 42:
# line 217 "../simpar.y"
{
        islog_init( yXypvt[-2].int_val, yXypvt[-1].int_val );
        command_num++;
    } break;
case 43:
# line 230 "../simpar.y"
{
	command_num++;
	rfgetx(yXypvt[-2].str_val,yXypvt[-1].str_val);
     } break;
case 44:
# line 243 "../simpar.y"
{
	nest(yXypvt[-1].str_val);
	command_num++;
     } break;
case 45:
# line 255 "../simpar.y"
{
        config_msg(yXypvt[-4].str_val,(double)yXypvt[-3].int_val, (long)yXypvt[-2].int_val, yXypvt[-1].str_val);
	command_num++;
    } break;
case 46:
# line 260 "../simpar.y"
{
        config_msg(yXypvt[-4].str_val, yXypvt[-3].dbl_val, (long)yXypvt[-2].int_val, yXypvt[-1].str_val);
	command_num++;
    } break;
case 47:
# line 274 "../simpar.y"
{
	config_sim_start();
	command_num++;
     } break;
case 48:
# line 286 "../simpar.y"
{
	command_num++;
	set_nostdout();
    } break;
case 49:
# line 299 "../simpar.y"
{
	command_num++;
 	set_size(yXypvt[-1].int_val);
    } break;
case 50:
# line 311 "../simpar.y"
{
	command_num++;
 	set_cutoff(yXypvt[-1].dbl_val);
    } break;
case 51:
# line 317 "../simpar.y"
{
	command_num++;
 	set_cutoff((double)yXypvt[-1].int_val);
    } break;
case 52:
# line 330 "../simpar.y"
{
	command_num++;
	set_display();
     } break;
case 53:
# line 342 "../simpar.y"
{
	command_num++;
	get_file(yXypvt[-2].str_val,yXypvt[-1].str_val);
     } break;
case 54:
# line 354 "../simpar.y"
{
	command_num++;
	put_file(yXypvt[-3].str_val,yXypvt[-2].str_val,yXypvt[-1].int_val);
     } break;
case 55:
# line 366 "../simpar.y"
{
	command_num++;
	del_file(yXypvt[-3].dbl_val,yXypvt[-1].str_val);
     } break;
case 56:
# line 372 "../simpar.y"
{
	command_num++;
	del_file((double)yXypvt[-3].int_val,yXypvt[-1].str_val);
     } break;
case 57:
# line 390 "../simpar.y"
{
	command_num++;
	timechg(yXypvt[-1].int_val);
     } break;
case 58:
# line 403 "../simpar.y"
{
	timeval( yXypvt[-1].dbl_val );
	command_num++;
    } break;
case 59:
# line 415 "../simpar.y"
{
	command_num++;
	go();
     } break;
case 60:
# line 427 "../simpar.y"
{
	command_num++;
	set_maxacks(yXypvt[-1].int_val);
    } break;
case 61:
# line 439 "../simpar.y"
{
	command_num++;
	set_monon();
    } break;
case 62:
# line 451 "../simpar.y"
{
	rmint(yXypvt[-6].str_val, yXypvt[-5].str_val, (long)yXypvt[-4].int_val, yXypvt[-3].str_val, (long)yXypvt[-2].int_val, yXypvt[-1].str_val);
	command_num++;
     } break;
case 63:
# line 463 "../simpar.y"
{
	command_num++;
 	set_monoff();
   } break;
case 64:
# line 476 "../simpar.y"
{
	command_num++;
	set_dlm();
    } break;
case 65:
# line 485 "../simpar.y"
{
	command_num++;
	set_idledlm(yXypvt[-1].int_val);
    } break;
case 66:
# line 494 "../simpar.y"
{
	command_num++;
	set_maxoff(yXypvt[-1].int_val);
    } break;
case 67:
# line 503 "../simpar.y"
{
	command_num++;
	set_dlmint(yXypvt[-1].int_val);
    } break;
case 68:
# line 512 "../simpar.y"
{
	command_num++;
	set_migrations(yXypvt[-1].int_val);
    } break;
	}
	goto yXystack;		/* reset registers in driver code */
}
