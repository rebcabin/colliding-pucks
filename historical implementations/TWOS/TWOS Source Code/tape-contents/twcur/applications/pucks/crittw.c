/****************************************************************/
/* crit.c			06-07-88 pjh			*/
/*								*/
/*	This file contains two functions which fix the		*/
/* problem of structures not being re-enterant on the		*/
/* Counterpoint compiler. The solution is to bracket every	*/
/* function which returns a structure with these functions	*/
/* so they will not get interrupted by Time Warp running on	*/
/* the Mark III hypercube.					*/
/* These functions will only work with Time Warp version 110	*/
/*								*/
/****************************************************************/

#ifndef SIMULATOR
#ifdef  MARK3

extern long interrupt_disable;

#endif MARK3
#endif SIMULATOR


enter_critical_section ()

{

#ifndef SIMULATOR

#ifdef	MARK3

    interrupt_disable = 1;

#endif MARK3

#endif SIMULATOR

}

leave_critical_section ()

{

#ifndef SIMULATOR

#ifdef	MARK3

    interrupt_disable = 0;

#endif MARK3

#endif SIMULATOR

}
