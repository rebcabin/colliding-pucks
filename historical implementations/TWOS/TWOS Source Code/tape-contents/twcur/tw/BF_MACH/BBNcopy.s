|
|	Copyright (C) 1989, 1991, California Institute of Technology.
|	U. S. Government Sponsorship under NASA Contract NAS7-918
|	is acknowledged.
|
| $Log:	BBNcopy.s,v $
| Revision 1.3  91/11/01  13:21:47  pls
| Handle 64K boundary conditions (SCR 165).
| 
| Revision 1.2  91/07/17  15:51:37  judy
| New copyright notice.
| 
| Revision 1.1  90/08/07  15:37:38  configtw
| Initial revision
| 

|	entcpy ( dest, src, numbytes )

	.text
	.globl	_entcpy
_entcpy:
	movl	a7@(4),a1	| dest 
	movl	a7@(8),a0	| src 
	movl	a7@(12),d0	| numbytes
	lsrl	#2,d0
	bras	g2
l20:
	swap	d0			| restore count
l2:
	movl	a0@+,a1@+
g2:
	dbf		d0,l2
	swap	d0			| look at high 16 bits of count
	dbf		d0,l20		| decrement high part
	movl	#3,d0
	andl	a7@(12),d0
	bras	g3
l3:
	movb	a0@+,a1@+
g3:
	dbf		d0,l3
	rts
