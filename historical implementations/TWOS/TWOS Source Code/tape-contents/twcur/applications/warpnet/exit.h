/* exit.h -- header for exit.c for warpnet */

#define EXIT 1  /* 0 = exit by when message count at DIAD */
#define EXIT0 0 /* 1 = exit by ending creation stream after DIAD creations */
#define EXIT1 1 /* 2 = exit by vtime DIAD */
#define EXIT2 0

Int check_exit();
