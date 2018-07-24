/*      Copyright (C) 1989, California Institute of Technology.
        U. S. Government Sponsorship under NASA Contract NAS7-918
        is acknowledged.        */

/* BBN.h Contains some of the required parameter for    */
/* the BBN implementations of the message system.       */


#define	CP  (tw_num_nodes) 
#define	ALL	-1
#define	MAX_NODES	128
#define	OK	1
#define DONE	1

typedef	struct mstrct {

  int	*buf;	/* message buffer	*/
  int	blen;
  int	mlen;
  int	dest;
  int	source;
  int   type;

 } MSG_STRUCT,  *MSG_STRUCT_PTR;

#ifdef BF_PLUS
 OID  goid;
#endif
