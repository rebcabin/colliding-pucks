/*
 * sysdata.h - Fred Wieland, Jet Propulsion Laboratory
 * 
 * Data for combat systems for both Blue and Red
 */

/**************************************************************************
 * The following data definitions are in ctls88.h and are part
 * of this data structure.
 *
 * enum Sysnames {M60A1, T_62, Util_truck, Five_ton_truck };
 *
 * #define MAX_CS   4  
 * #define MAX_CSS  2  
 *
 * #define MAX_BLUE_CS 2
 * #define MAX_RED_CS 2
 *
 * #define MAX_BLUE_CSS 1
 * #define MAX_RED_CSS 1
 **************************************************************************/

typedef struct Sysdata
	{
	enum Sides side;
 	enum Sysnames name;
	int operational;
	int mtoe;  /* maximum you could have */
	double strength;
	} Sysdata;

static Sysdata combatsys[MAX_CS] =
	{
/* Side		System				Num Operational 	Num TOE		Strength			*/
{	Blue,		M60A1,					285,				300,			1.0		},
{	Blue,		Fred0,					315,				350,			1.0		},
{	Blue,		Fred2,					285,				300,			1.0		},
{	Blue,		Fred4,					322,				350,			1.0		},
{	Blue,		Fred6,					375,				400,			1.05		},
{	Red,		T_62,						350,				375,			1.0		},
{	Red,		Fred1,					310,				325,			1.1		},
{	Red,		Fred3,					250,				275,			1.03		},
{	Red,		Fred5,					365,				400,			1.0		},
{	Red,		Fred7,					375,				400,			1.02		}
	};

static Sysdata combatsupp[MAX_CSS] =
	{
/* Side		System				Num Operational 	Num TOE		Strength			*/
{	Blue,		Five_ton_truck,		3200,				3500,			0.0		},
{  Blue,		APC,						1600,				1700,			0.0		},
{	Red,		Troop_carrier,			1750,				1800,			0.0		},
{	Red,		Util_truck,				1200,				1500,			0.0		}
	};
