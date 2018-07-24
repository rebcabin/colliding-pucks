
/*
 * FWLdata.h - Fred Wieland, Jet Propulsion Laboratory, January 1988
 *
 * This file contains all of the data for the lanchester coefficients.
 * Although officially disallowed, I am linking this data in as static
 * data to the objects; it never changes during the run, and by 
 * centralizing it here I can easily change it between runs.  It is
 * a requirement of the model that the data be easily changed between
 * runs.
 *
 * The #include file ctls88.h must precede this .h file in whatever
 * .c file the definition resides.
 */

#define MAXFWL  70
typedef struct FWLdata
	{
	enum Sysnames shooter, target;
	double coef;
	} FWLdata;

static FWLdata FWL_coef[MAXFWL] =  {
/*
   BLUE ON RED:
	shooter			target					coefficient								*/

{	M60A1,			T_62,						0.000115	},
{	M60A1,			Util_truck,				0.000003	},
{	M60A1,			Fred1,					0.000010	},
{	M60A1,			Fred3,					0.000008	},
{	M60A1,			Fred5,					0.000009	},
{	M60A1,			Fred7,					0.000011	},
{	M60A1,			Troop_carrier,			0.000011	},
{  Fred0,			T_62,						0.000117	},
{  Fred0,			Fred1,					0.000125	},
{  Fred0,			Fred3,					0.000124	},
{  Fred0,			Fred5,					0.000120	},
{  Fred0,			Fred7,					0.000119	},
{	Fred0,			Util_truck,				0.000003	},
{	Fred0,			Troop_carrier,			0.000003	},
{  Fred2,			T_62,						0.000110	},
{  Fred2,			Fred1,					0.000115	},
{  Fred2,			Fred3,					0.000110	},
{  Fred2,			Fred5,					0.000117	},
{  Fred2,			Fred7,					0.000118	},
{	Fred2,			Util_truck,				0.000003	},
{	Fred2,			Troop_carrier,			0.000003	},
{  Fred4,			T_62,						0.000120	},
{  Fred4,			Fred1,					0.000115	},
{  Fred4,			Fred3,					0.000113	},
{  Fred4,			Fred5,					0.000111	},
{  Fred4,			Fred7,					0.000120	},
{	Fred4,			Util_truck,				0.000003	},
{	Fred4,			Troop_carrier,			0.000003	},
{  Fred6,			T_62,						0.000119	},
{  Fred6,			Fred1,					0.000125	},
{  Fred6,			Fred3,					0.000120	},
{  Fred6,			Fred5,					0.000120	},
{  Fred6,			Fred7,					0.000119	},
{	Fred6,			Util_truck,				0.000003	},
{	Fred6,			Troop_carrier,			0.000003	},
 
/*
   RED ON BLUE:
	shooter			target					coefficient								*/
{	T_62,				M60A1,					0.000500	},
{	T_62,				Five_ton_truck,		0.000012	},
{	T_62,				Fred0,					0.000128	},
{	T_62,				Fred2,					0.000125	},
{	T_62,				Fred4,					0.000120	},
{	T_62,				Fred6,					0.000111	},
{	T_62,				APC,						0.000111	},
{	Fred1,			M60A1,					0.000120	},
{	Fred1,			Fred0,					0.000123	},
{	Fred1,			Fred2,					0.000120	},
{	Fred1,			Fred4,					0.000115	},
{	Fred1,			Fred6,					0.000110	},
{	Fred1,			Five_ton_truck,		0.000004	},
{	Fred1,			APC,						0.000004	},
{	Fred3,			M60A1,					0.000120	},
{	Fred3,			Fred0,					0.000121	},
{	Fred3,			Fred2,					0.000110	},
{	Fred3,			Fred4,					0.000117	},
{	Fred3,			Fred6,					0.000115	},
{	Fred3,			Five_ton_truck,		0.000004	},
{	Fred3,			APC,						0.000004	},
{	Fred5,			M60A1,					0.000122	},
{	Fred5,			Fred0,					0.000119	},
{	Fred5,			Fred2,					0.000118	},
{	Fred5,			Fred4,					0.000117	},
{	Fred5,			Fred6,					0.000116	},
{	Fred5,			Five_ton_truck,		0.000004	},
{	Fred5,			APC,						0.000004	},
{	Fred7,			M60A1,					0.000119	},
{	Fred7,			Fred0,					0.000111	},
{	Fred7,			Fred2,					0.000125	},
{	Fred7,			Fred4,					0.000123	},
{	Fred7,			Fred6,					0.000122	},
{	Fred7,			Five_ton_truck,		0.000004	},
{	Fred7,			APC,						0.000004	}
	};
