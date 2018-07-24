#include "mem_type.h"
#define MAX_OBJS 1000
#define MAX_NODES 128
#define BLOCKSIZE 128
#define MAX_ITEMS 4000
#define LISTSIZE 1000
#define INIT_VTIME (-10)
#define MAX_COLOR_SETTINGS 50
#define NMEM_CATEGORIES 27
#define MAX_SIZES 200
#define MAX_NHIST_BARS 60
#define MAX(x,y) x>y?x:y
#define MIN(x,y) x<y?x:y

typedef struct
{
	int objno;
	char name[50];
}
	OBJ_NAME_ENTRY;

typedef struct
{
	int address;
	long type;
	int size;
	int objno;
}
	MEM_ENTRY;

typedef struct
{
	int beg_size, end_size;
	/* groupings need bounds.  end_size==0 for sizes that are not groups */
	int how_many;
}
	SIZE_ENTRY;
extern SIZE_ENTRY sizes[];
extern int nsize_entries, max_how_many;
 
typedef struct
{
	int mem_type, color;
}
	COLOR_SETTING;
extern COLOR_SETTING color_settings[MAX_COLOR_SETTINGS];
extern int set_color_type, ncolor_settings;

extern MEM_ENTRY mem_entry[MAX_ITEMS];

extern int cno;		/* number of mem_entry's */

extern int X1, Y1, X2, Y2;

/* xorg is the left of the graph area,
   xend the right,
   yorg the top, 
   yend the bottom */
extern int xorg, xend, xlen;
extern int yorg, yend, ylen;

/* These represent the "data" ranges that will be plotted on the
   y axis, i.e. the largest size memory block and the smallest. */
extern int xmax, xmin, xrange;
extern double ymax, ymin, yrange;
/* These are the "data" ranges for each display mode. */
extern int addr_xmax, addr_xmin, addr_xrange;
extern double addr_ymax, addr_ymin, addr_yrange;
extern int size_xmax, size_xmin, size_xrange;
extern double size_ymax, size_ymin, size_yrange;

/* divide "data" coordinates by xscale & yscale to scale them to screen
   coordinates.  */
extern double xscale, yscale;

/* width and height are the dimensions of the entire graphics area */
extern int width, height;

/* xs--- and ys--- are the dimensions of the piece of the entire page
   that are zoomed into now. */
extern int xsmin, xsmax;
extern double ysmin, ysmax;
extern int max_ydiff;

extern int font_width;
extern int font_height;
extern int mode;
/* Possible view "mode"s: */
#define BY_SIZE 0
#define BY_ADDRESS 1

extern int zoomed;

/*extern int search;
extern int page_scan; */

extern int screen_x1[MAX_ITEMS], screen_x2[MAX_ITEMS];
extern int screen_y1[MAX_ITEMS], screen_y2[MAX_ITEMS];

extern int dev, val;
extern int dev_x, dev_y;
extern int redraw_page;
extern int identify_on;
extern int zooming;
extern int grouping_threshold; /* controls clustering of sizes in histogram */
extern long show_mask;
 
#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define WHITE   7

extern int color;

