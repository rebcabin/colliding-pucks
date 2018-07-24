/*
 * $Log:	rbc_public.h,v $
 * Revision 1.2  91/06/03  12:26:27  configtw
 * Tab conversion.
 * 
 * Revision 1.1  91/03/26  10:34:54  pls
 * Initial revision
 * 
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

/* rbc_public.h */

extern unsigned int rbc_num_KB;         /* actual memory installed */
extern unsigned long rbc_memory_start;  /* so macro's below will work */

#define MAX_FRAME       63
#define NUM_FRAMES      (MAX_FRAME + 1)
#define ARCH_FRAME      0x0100
#define MAX_SEG         255
#define NUM_SEGS        (MAX_SEG + 1)

/* function returns */
#define RBC_SUCCESS     0               /* zero return means success */
#define RBC_FATAL_ERROR -1
#define HARD_ERROR      -3
#define BAD_PARAMETER   -5
#define FRAME_OVERFLOW  -7
#define ABORTED_COMMAND 1
#define MARK_OVERFLOW   3

/* macro's */
#define rbc_kilo2real(x)        ((void *)(1024 * (x) + rbc_memory_start))
#define rbc_real2kilo(y)        ((((unsigned long)(y)) - rbc_memory_start)>>10)

#ifdef __STDC__
/* prototypes */
int rbc_rollback ( unsigned int seg_no, unsigned int num_frames );
int rbc_advance ( unsigned int seg_no, unsigned int num_frames );
int rbc_mark ( unsigned int seg_no, unsigned int num_frames );

int rbc_init_start ( void );
int rbc_init_done ( void );
unsigned long rbc_status ( void );
unsigned long rbc_seg_status( unsigned int seg_no );
int rbc_ready ( unsigned int seg_no );

int rbc_allocate ( unsigned int seg_no, unsigned long begin_addr, unsigned long end_addr );
int rbc_dealloc_start ( unsigned int seg_no, unsigned long begin_addr, unsigned long end_addr );
int rbc_dealloc_done ( unsigned int seg_no, unsigned long begin_addr, unsigned long end_addr, int wait );


int rbc_read_memory ( unsigned int frame_num, unsigned long memory_loc, char * destination, int num_of_chars );
int rbc_write_memory ( unsigned int frame_num, unsigned long memory_loc, char * source, int num_of_chars );

int rbc_get_def ( unsigned int seg_no, unsigned long *start_addr, unsigned long *end_addr );
int rbc_put_def ( unsigned int seg_no, unsigned long start_addr, unsigned long end_addr );
unsigned int rbc_get_encode ( unsigned long address );
unsigned int rbc_put_encode ( unsigned long address, unsigned int seg_no );

int rbc_get_omf ( unsigned int seg_no );
int rbc_put_omf ( unsigned int seg_no, unsigned int new_frame );
int rbc_get_cmf ( unsigned int seg_no );
int rbc_put_cmf ( unsigned int seg_no, unsigned int new_frame );
int rbc_get_wb ( unsigned long address, unsigned long *wb_low, unsigned long *wb_high );

void print_seg_encode_table ( void );
void print_seg_encode ( unsigned int i );
void print_seg_def_table ( void );
void print_seg_def ( unsigned int i );
void print_wb_seg ( unsigned int seg_no );
void print_wb ( unsigned long address );
void print_frame_counter_table ( void );
void print_frame_counter ( unsigned int seg_no );
void print_status ( void );
void print_seg_status ( unsigned int seg_no );
int set_print_file ( char * filename );
#else
unsigned long rbc_status ();
unsigned long rbc_seg_status();

unsigned int rbc_get_encode ( );
unsigned int rbc_put_encode ( );

void print_seg_encode_table ( );
void print_seg_encode ( );
void print_seg_def_table ( );
void print_seg_def ( );
void print_wb_seg ( );
void print_wb ( );
void print_frame_counter_table ( );
void print_frame_counter ( );
void print_status ( );
void print_seg_status ( );
#endif
