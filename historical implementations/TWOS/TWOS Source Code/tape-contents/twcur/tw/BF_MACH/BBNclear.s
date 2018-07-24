|	Copyright (C) 1989, 1991, California Institute of Technology.
|	U. S. Government Sponsorship under NASA Contract NAS7-918
|	is acknowledged.


|
| $Log:	BBNclear.s,v $
| Revision 1.4  91/11/01  13:19:55  pls
| 1.  Handle 64K boundary conditions (SCR 165).
| 
| Revision 1.3  91/07/17  15:51:19  judy
| New copyright notice.
| 
| Revision 1.2  90/12/10  10:59:36  configtw
| fix 16 bit loop limit problem (Matt)
| 
| Revision 1.1  90/08/07  15:37:31  configtw
| Initial revision
| 

|	clear ( addr, numbytes )

	.text
	.globl	_clear
_clear:
	link	a6,#0
	movl	a6@(8),a0	| addr
	movl	a6@(12),d0	| numbytes
	movl	d2,a7@-
	movl	d1,a7@-
	movl	#3,d1
	bras	g1
l1:
	clrb	a0@+
g1:
	movl	a0,d2
	andl	d1,d2
	dbeq	d0,l1
	beqs	notdone2
	swap	d0			| check high 16 bits
	dbf		d0,notdone1
	bras	done
notdone1:				| count > 16 bits
	swap	d0			| swap count back
	andl	d0,d1		| save low order 2 bits of count
	lsrl	#2,d0		| convert to word count
	bras	l2			| start loop with write
notdone2:
	andl	d0,d1
	lsrl	#2,d0
	bras	g2
l20:
	swap	d0			| get low 16 bits again
l2:
	clrl	a0@+
g2:
	dbf		d0,l2
	swap	d0			| look at high 16 bits
	dbf		d0,l20
	bras	g3
l3:
	clrb	a0@+
g3:
	dbf		d1,l3

done:
	movl	a7@+,d1
	movl	a7@+,d2
	unlk	a6
	rts
