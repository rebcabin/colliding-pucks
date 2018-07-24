/**************************************************************************
* simparO.y
* parser for the timewarp simulator configuration file.
*  JJW Dec 17, 1987  update for files 3/15/88,  8/9/88
*  modify error handling  JJW 2/16/89
**********************************************************************/

/* the numeric value of -infinity is built into this file !!! */

%{
#ifdef SUN
#include <sys/types.h>
#endif SUN

static char simparid[] = "%W%\t%G%";
extern int command_num;
extern int yylineno; 
extern int no_stdout;
extern int displ_sw;
extern get_errline();
extern char yytext[];
%}

%union
{
    int     int_val; /* Integer valued tokens. */
    long   long_val; /* Long    valued tokens. */
    double  dbl_val; /* Double  valued tokens. */
    char *  str_val; /* String  valued tokens. */ 
} 

%token    <int_val>     NUL_TKN
%token    <int_val>     NEST_TKN
%token    <int_val>     OBCR_TKN
%token    <int_val>     TIMEV_TKN
%token    <int_val>     TIMECHG_TKN
%token    <int_val>     MSG_TKN
%token    <int_val>     RMINT_TKN
%token    <int_val>     GO_TKN
%token    <int_val>     NOSTDT_TKN
%token    <int_val>     MACKS_TKN
%token    <int_val>     END_TKN
%token    <int_val>     MONON_TKN
%token    <int_val>     MONOFF_TKN
%token	  <int_val>	OBJSTKSIZE_TKN
%token	  <int_val>	CUTOFF_TKN
%token    <int_val>     DISP_TKN
%token    <int_val>     GETF_TKN
%token    <int_val>     PUTF_TKN
%token    <int_val>     INT_TKN
%token   <long_val>     LONG_TKN
%token    <dbl_val>     DOUBLE_TKN
%token    <str_val>     STR_TKN
%token    <int_val>     PKTLEN_TKN
%token    <int_val>     ISLOG_TKN     /*PJH_IS        */
%token    <str_val>     RFGET_TKN
%token    <int_val>     DLM_TKN
%token    <int_val>     IDLEDLM_TKN
%token    <int_val>     MAXOFF_TKN
%token    <int_val>     MIGRATIONS_TKN
%token    <int_val>     DLMINT_TKN
%token    <int_val>     OBSTATS_TKN
%token    <int_val>     DELFILE_TKN
%token    <int_val>     TPINIT_TKN
%token    <int_val>     MAXPOOL_TKN
%token    <int_val>     ALLOWNOW_TKN

%type	<int_val>	line
%type   <int_val>	command

%start command

%%
command:
	{
        return(1);
	}
    | NUL_TKN
	{
        return(0);
	}
    | line NUL_TKN
    | error NUL_TKN
    {
       yyerrok;
	yyclearin;
        return(0);
    }


line:   obcr_command { return(0); }
    |   timev_command  { return(0); }
    |   timechg_command { return(0); }
    |   tell_command { return(0); }
    |   rmint_command { return(0); }
    |   go_command { return(0); }
    |   nostdout_command { return(0); }
    |   maxacks_command { return(0); }
    |   monon_command { return(0); }
    |   monoff_command { return(0); }
    |   objstksize_command { return(0); }
    |   cutoff_command { return(0); }
    |   display_command { return(0); }
    |   pktlen_command { return(0); }
    |   islog_command { return(0); }
    |   rfget_command { return(0); }
    |   getfile_command { return(0); }
    |   putfile_command { return(0); }
    |	dlm_command  {return(0); }
    |	idledlm_command  {return(0); }
    |	maxoff_command  {return(0); }
    |	dlmint_command  {return(0); }
    |	migrations_command  {return(0); }
    |   nest_command { return(0); }
    |   objstats_command { return(0); }
    |	delfile_command { return(0); }
    |	type_init_command { return(0); }
    |	allow_now_command { return(0); }
    |   maxmsgpool_command {return(0); }
    |   end_command { return(1); }
    |
	STR_TKN
    {
	printf("syntax error, first token line %d\n", yylineno);
	YYERROR;
    }
    ;


/* ************************************************************************
 *  create object
 *  obcreate object objecttype node
 *
 * **************************************************************************/
obcr_command:
       OBCR_TKN STR_TKN STR_TKN INT_TKN NUL_TKN
    {
        config_obj($2, $3, $4);
	command_num++;
    }
    ;

/* ************************************************************************
 *  typeinit
 *  typeinit objecttype
 *
 * **************************************************************************/
type_init_command:
      TPINIT_TKN STR_TKN STR_TKN NUL_TKN
    {
	init_type($2,$3);
	command_num++;
    }
    ;

/* ************************************************************************
 *  allownow
 *  allownow
 *
 * **************************************************************************/
allow_now_command:
      ALLOWNOW_TKN  NUL_TKN
    {
	allow_now_msgs();
	command_num++;
    }
    ;

/* ***************************************************************
* maxmsgpool_command
* maxmsgpool integer
*
*****************************************************************/
maxmsgpool_command:
      MAXPOOL_TKN  INT_TKN  NUL_TKN
    {
       set_maxmsgpool($2);
       command_num++;
    }
    ;


/*************************************************************************
*
*  pktlen -  set message packet length
*  pktlen value
*************************************************************************/
pktlen_command:
        PKTLEN_TKN INT_TKN NUL_TKN 
    {
	pktlen( $2 );
	command_num++;
    }
    ;

/**********************************************************
*
*  objstats - collect objstats at a given time
*  objstats value (value is a double)
*************************************************************/
objstats_command:
	OBSTATS_TKN DOUBLE_TKN NUL_TKN
    {
	objstats($2);
	command_num++;
    }
    ;	

/*PJH_IS        */
/*************************************************************************
*
*  islog - Set Up Instantaneous Speedup logging 
*  logsize value, loginterval  value
*************************************************************************/
islog_command:
        ISLOG_TKN INT_TKN INT_TKN NUL_TKN
    {
        islog_init( $2, $3 );
        command_num++;
    }
    ;
 
/*************************************************************************
*
*  rfget  <path spec>
*  get files into ramfile system on Butterfly
*************************************************************************/
rfget_command:
	RFGET_TKN STR_TKN STR_TKN NUL_TKN
     {
	command_num++;
	rfgetx($2,$3);
     }
     ;

/*************************************************************************
*
*  nest_command
*  @<include file>
*************************************************************************/
nest_command:
	NEST_TKN STR_TKN NUL_TKN
     {
	nest($2);
	command_num++;
     }
     ;

/* *******************************************************************
*  send event message
*  evtmsg rcver rcvtime message
/* ***************************************************************** */
tell_command:
	 MSG_TKN STR_TKN INT_TKN INT_TKN STR_TKN NUL_TKN
    {
        config_msg($2,(double)$3, (long)$4, $5);
	command_num++;
    }
    |   MSG_TKN STR_TKN DOUBLE_TKN INT_TKN STR_TKN NUL_TKN
    {
        config_msg($2, $3, (long)$4, $5);
	command_num++;
    }
/* config_msg uses a hidden argument msgtextlen for the text length - see
   the lex file */
   ;

/* **********************************************************************
*   objend    finish configuration
*   objend
**************************************************************************/
end_command:
	END_TKN  NUL_TKN
     {
	config_sim_start();
	command_num++;
     }
     ;

/**********************************************************************
*   nostdout
*   nostdout
************************************************************************/
nostdout_command:
	NOSTDT_TKN NUL_TKN
    {
	command_num++;
	set_nostdout();
    }
    ;


/************************************************************************
*   objstksize  value
*
************************************************************************/
objstksize_command:
	OBJSTKSIZE_TKN  INT_TKN  NUL_TKN
    {
	command_num++;
 	set_size($2);
    }
    ;

/************************************************************************
*   cutoff  value
*
************************************************************************/
cutoff_command:
	CUTOFF_TKN  DOUBLE_TKN  NUL_TKN
    {
	command_num++;
 	set_cutoff($2);
    }
        |
	CUTOFF_TKN  INT_TKN  NUL_TKN
    {
	command_num++;
 	set_cutoff((double)$2);
    }

    ;

/************************************************************************
*   filecho
*
************************************************************************/
display_command:
	DISP_TKN NUL_TKN
     {
	command_num++;
	set_display();
     }
     ;

/************************************************************************
*   getfile_command
*
************************************************************************/
getfile_command:
	GETF_TKN STR_TKN STR_TKN NUL_TKN
     {
	command_num++;
	get_file($2,$3);
     }
     ;

/************************************************************************
*   putfile_command
*
************************************************************************/
putfile_command:
	PUTF_TKN STR_TKN STR_TKN INT_TKN NUL_TKN
     {
	command_num++;
	put_file($2,$3,$4);
     }
     ;

/************************************************************************
*   delfile_command
*
************************************************************************/
delfile_command:
	DOUBLE_TKN DELFILE_TKN STR_TKN NUL_TKN
     {
	command_num++;
	del_file($1,$3);
     }
    |
	INT_TKN DELFILE_TKN STR_TKN NUL_TKN
     {
	command_num++;
	del_file((double)$1,$3);
     }
     ;




/************* These are in TW and do nothing in the simulator **********/

/*************************************************************************
*
*  timechg  interval change time
*  timechg  interval_change_time
*************************************************************************/
timechg_command:
	TIMECHG_TKN INT_TKN NUL_TKN
     {
	command_num++;
	timechg($2);
     }
     ;

/*************************************************************************
*
*  timeval -  interval timer value
*  timeval interval
*************************************************************************/
timev_command:
        TIMEV_TKN DOUBLE_TKN NUL_TKN 
    {
	timeval( $2 );
	command_num++;
    }
    ;

/**********************************************************************
*   go
*   go
***********************************************************************/
go_command:
	GO_TKN NUL_TKN
     {
	command_num++;
	go();
     }
     ;

/*********************************************************************
*   maxacks
*   maxacks
************************************************************************/
maxacks_command:
	MACKS_TKN INT_TKN NUL_TKN
    {
	command_num++;
	set_maxacks($2);
    }
    ;

/************************************************************************
*   monon
*
************************************************************************/
monon_command:
	MONON_TKN NUL_TKN
    {
	command_num++;
	set_monon();
    }
    ;

/**************************************************************************
*   rmint  received message interrupt
*   rmint  mtype snder sandtim rcver rcvtim message
**************************************************************************/
rmint_command:
	RMINT_TKN STR_TKN STR_TKN INT_TKN STR_TKN INT_TKN STR_TKN NUL_TKN
     {
	rmint($2, $3, (long)$4, $5, (long)$6, $7);
	command_num++;
     }
     ;

/************************************************************************
*   monoff
*
************************************************************************/
monoff_command:
	MONOFF_TKN NUL_TKN
    {
	command_num++;
 	set_monoff();
   }
    ;

/***********************************************************************
*
*  More TW only commands which are checked but don't do anything in the
*  simulator.
***********************************************************************/
dlm_command: 
	DLM_TKN NUL_TKN
    {
	command_num++;
	set_dlm();
    }
    ;

 
idledlm_command:
	IDLEDLM_TKN INT_TKN NUL_TKN
    {
	command_num++;
	set_idledlm($2);
    }
    ;

  
maxoff_command:
	MAXOFF_TKN INT_TKN NUL_TKN
    {
	command_num++;
	set_maxoff($2);
    }
    ;

  
dlmint_command:
	DLMINT_TKN INT_TKN NUL_TKN
    {
	command_num++;
	set_dlmint($2);
    }
    ;

  
migrations_command:
	MIGRATIONS_TKN INT_TKN NUL_TKN
    {
	command_num++;
	set_migrations($2);
    }
    ;




%%
