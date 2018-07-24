/*
 * THIS IS THE FORMTAB FOR V1.1 OF STB88.1
 * Formtab.h - formation table for corps sectors
 *
 * This static data structure contains the formation table for 
 * the units.
 *
 * NOTE: If you change the number of units per sector, then the
 * constants NUM_RED_DIVS and NUM_BLUE_DIVS must be changed in
 * stb88.h
 */

#define MAX_FORMTAB 32
#define RED_DIVS_PER_CORPS 10
#define BLUE_DIVS_PER_CORPS 7

typedef struct formtab
	{
	enum Sides side;
	int formation;
	enum Sectors sector;
	int units_in_sector;
	int sector_width;
	} Formtab;

static Formtab form_table[MAX_FORMTAB] = 
	{										 
/*											  Units
   Side		Formation	Sector	in Sector	Sector Width     */
{ 	Red,			1,			Reserve,			2,				32					},
{	Red,			1,			Sector1,			4,				32					},
{	Red,			1,			Sector2,			1,				32					},
{	Red,			1,			Sector3,			3,				32					},
{	Red,			2,			Reserve,			3,				32 				},
{	Red,			2,			Sector1,			3,				32					},
{	Red,			2,			Sector2,			2,				32					},
{	Red,			2,			Sector3,			2,				32					},
{	Red,			3,			Reserve,			2,				32 				},
{	Red,			3,			Sector1,			3,				32					},
{	Red,			3,			Sector2,			2,				32 				},
{	Red,			3,			Sector3,			2,				32					},
{	Red,			4,			Reserve,			3,				32 				},
{	Red,			4,			Sector1,			2,				32					},
{	Red,			4,			Sector2,			3,				32 				},
{	Red,			4,			Sector3,			2,				32					},

{	Blue,			1,			Reserve,			5,				32					},
{	Blue,			1,			Sector1,			1,				32					},
{	Blue,			1,			Sector2,			1,				32					},
{	Blue,			1,			Sector3,			0,				32					},
{	Blue,			2,			Reserve,			2,				32					},
{	Blue,			2,			Sector1,			3,				32					},
{	Blue,			2,			Sector2,			1,				32					},
{	Blue,			2,			Sector3,			1,				32					},
{	Blue,			3,			Reserve,			3,				32					},
{	Blue,			3,			Sector1,			1,				32					},
{	Blue,			3,			Sector2,			2,				32					},
{	Blue,			3,			Sector3,			1,				32					},
{	Blue,			4,			Reserve,			2,				32					},
{	Blue,			4,			Sector1,			2,				32					},
{	Blue,			4,			Sector2,			1,				32					},
{	Blue,			4,			Sector3,			2,				32					}
	};

typedef struct Corpstab
	{
	Name_object corps_name;
	Vector location;
	int formation_num;
	} Corpstab;

static Corpstab corps_table[NUM_RED_CORPS+NUM_BLUE_CORPS] =
	{
/*
	Name				   X pos		Y pos			Formation
*/
{	"blue_corps1",		{90.0,	 53.0},				1				},
{	"blue_corps2",		{90.0,	159.0},				1				},
{	"blue_corps3",		{90.0,	265.0},				1				},
{	"blue_corps4",		{90.0,	371.0},				2				},
{	"blue_corps5",		{90.0,	477.0},				2				},
{	"blue_corps6",		{90.0,	583.0},				2				},
{	"blue_corps7",		{90.0,	689.0},				3 				},
{	"blue_corps8",		{90.0,	795.0},				3				},
{	"blue_corps9",		{90.0,	901.0},				4 				},
{	"red_corps1",		{300.0,	 53.0},				1				},
{	"red_corps2",		{300.0,	159.0},				2				},
{	"red_corps3",		{300.0,	265.0},				3				},
{	"red_corps4",		{300.0,	371.0},				1				},
{	"red_corps5",		{300.0,	477.0},				2				},
{	"red_corps6",		{300.0,	583.0},				3				},
{	"red_corps7",		{300.0,	689.0},				1				},
{	"red_corps8",		{300.0,	795.0},				2				},
{	"red_corps9",		{300.0,	901.0},				4				}
	};

	 
