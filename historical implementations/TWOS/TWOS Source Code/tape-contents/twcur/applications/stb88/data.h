/*
 * data.h - Fred Wieland, Jet Propulsion Laboratory, January 1988
 *
 * This file contains all of the initial setup data for ctls88. A
 * change to this file and then a recompilation will change the 
 * layout of the game board and the initial parameters of the system.
 *
 * The #include file ctls88.h must precede this .h file in whatever
 * .c file the definition resides.
 */

typedef struct data
	{
	Name_object who;
	double x_pos;
	double y_pos;
	int pradius;
	} Data;

static Data init_data[24] =  {
/*
  Name         x_pos  y_pos radius                                       */

{ "blue_div1",  40.0, 40.0,  50 },
{ "blue_div2",  40.0, 72.0,  50 },
{ "blue_div3",  40.0, 104.0, 50 },
{ "blue_div4",  40.0, 250.0, 50 },
{ "blue_div5",  40.0, 150.0, 50 },
{ "blue_div6",  40.0, 325.0, 50 },
{ "blue_div7",  40.0, 375.0, 50 },
{ "blue_div8",  40.0, 405.0, 50 },
{ "blue_div9",  40.0, 450.0, 50 },
{ "blue_div10",  40.0, 1100.0, 50 },
{ "blue_div11",  40.0, 1250.0, 50 },
{ "blue_div12",  40.0, 1500.0, 50 },
{ "red_div1",  1950.0, 40.0,  50 },
{ "red_div2",  1950.0, 72.0,  50 },
{ "red_div3",  1950.0, 104.0, 50 },
{ "red_div4",  1950.0, 250.0, 50 },
{ "red_div5",  1950.0, 150.0, 50 },
{ "red_div6",  1950.0, 325.0, 50 },
{ "red_div7",  1950.0, 375.0, 50 },
{ "red_div8",  1950.0, 405.0, 50 },
{ "red_div9",  1950.0, 450.0, 50 },
{ "red_div10",  1950.0, 1100.0, 50 },
{ "red_div11",  1950.0, 1250.0, 50 },
{ "red_div12",  1950.0, 1500.0, 50 }
	}; 

