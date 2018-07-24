/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#define MAX_OBJS 1000
#define MAX_NODES 128
#define BLOCKSIZE 128
#define MAX_ITEMS (BLOCKSIZE * 900)
#define LISTSIZE 1000
#define INIT_VTIME (-10)

typedef struct
{
	int cputims, cputimr, hgtimef, hgtimet;
	float sndtim, rcvtim;
	int id_num;
	short snder, rcver, len;
	char mtype, flags;
}
	COBJ;

typedef struct
{
	char name[20];
	int node;
}
	ObjArray;

extern ObjArray obj[MAX_OBJS];

extern int nobjs, nodes;
extern int cno;
extern int shift_up;
extern int first_nobjs;

extern COBJ cobj[MAX_ITEMS];

extern int vt0;
extern int X1, Y1, X2, Y2, Z1, Z2;
extern int charx, chary;
extern int xorg, xend, xlen, xmax, xmin, xrange;
extern int yorg, yend, ylen;
extern double ymax, ymin, yrange;
extern double xscale, yscale;
extern int width, height;
extern int xsmin, xsmax, xpagediv, xpage, xpagesize, xnum_pages;
extern double ysmin, ysmax, ypagesize;
extern int ypagediv, ypage, ynum_pages;
extern int max_ydiff;
extern int font_width;
extern int font_height;
extern int select_node;
extern int select_obj;
extern int flat, antimessages, committed, rollbacks, scaling_y;
extern double low_vt[1152], high_vt[1152];
extern double last_vt[MAX_OBJS];
extern int zoomed;
extern int annotate;
extern int highlight;
extern int search;
extern int page_scan;
extern int from_to;
extern int screen_x1[MAX_ITEMS];
extern int screen_x2[MAX_ITEMS];
extern int screen_y1[MAX_ITEMS];
extern int screen_y2[MAX_ITEMS];

extern int list[2][LISTSIZE];
extern int il, ol;
extern int nlist[2];
extern int identi;
extern int listx, listy;

extern int stb89, stb88, ctls, pucks, ants, slooow;

extern int dev, val;
extern int dev_x, dev_y;
extern int redraw_page;
extern int identify_on;
extern int zooming;
 
#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define WHITE   7

extern int color;

