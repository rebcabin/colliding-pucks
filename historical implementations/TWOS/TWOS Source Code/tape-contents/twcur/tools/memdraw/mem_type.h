#define ALL_MEM		0xFFFFFFFFF
#define NO_MEM		0
/* zero is free, non-zero is allocated, == 1 is unknown */

#define FREE_MEM	0x040000000
#define LOST_MEM	0x01
#define ALLOCATED	0x01
#define POOL_MEM	0x02
/* Object Stuff */
#define OBJECT_MEM	0x04
#define OCB_MEM		0x08
#define FILETYPE_AREA	0x080000000
	/* State stuff */
#define	STATE_CUR	0x010
#define STATE_Q		0x020
#define STATE_ADD	0x040
#define STATE_DYM	0x0800
#define	STATE_SENT	0x01000
#define STATE_ERR	0x02000
#define STACK_MEM	0x04000
	/* Message stuff */
#define MESSAGE_MEM	0x08000
#define INPUT_MSG	0x010000
#define OUTPUT_MSG	0x020000
		/* Current Message copies */
#define MESS_VEC	0x040000
#define NOW_EMSGS	0x080000
		/* signs */
#define POS_MSG		0x0100000
#define NEG_MSG		0x0200000
#define SYS_MSG		0x0400000
	/* Queue HEADER */
#define QUEUE_HDR	0x0800000
/* TW overhead */
#define TW_OVER		0x01000000
#define CACHE_MEM	0x02000000
#define HOME_LIST	0x04000000
/* COMMUNICATIONS */
#define IN_TRANS	0x08000000
#define ONNODE		0x010000000
#define OFFNODE		0x020000000
