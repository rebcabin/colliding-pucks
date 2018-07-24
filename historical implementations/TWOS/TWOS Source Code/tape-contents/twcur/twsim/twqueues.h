/*      Copyright (C) 1989, California Institute of Technology.
        U. S. Government Sponsorship under NASA Contract NAS7-918
        is acknowledged.        */

/**********************************************************************
*
* Declarations for the twqueue.c module
* Feb 23, 1988
* Sept 20, 1988  altered for splay tree only module
*  most of this is now copied from splay.h
* may 19, 1989  change lots of names to integrate with TW mesg_block
*********************************************************************/





typedef struct
{
    mesg_block	* root;		/* root node */

    /* Statistics, not strictly necessary, but handy for tuning  */

    int		deqs;		/* number of spdeq()s */
    int		deqloops;	/* number of forloop cycles in above */
    
    int		enqs;		/* number of spenq()s */
    int		enqcmps;	/* compares in spenq */
    
    int		splays;
    int		splayloops;

} SPTREE;

#define quel mesg_block

extern int last_q;



extern SPTREE * spinit();	/* init tree */
extern int spempty();		/* is tree empty? */
extern mesg_block * spenq();		/* insert item into the tree */
extern mesg_block * spdeq();		/* return and remove lowest item in subtree */
extern void splay();		/* reorganize tree */
extern void spdelete();
void add_free();
extern mesg_block *spfhead();	/* find head of tree rapidly */
extern mesg_block *spfnext();	/* find next in tree rapidly */



