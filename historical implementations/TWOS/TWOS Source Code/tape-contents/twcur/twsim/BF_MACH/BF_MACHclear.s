|	Copyright (C) 1989, 1991, California Institute of Technology.
|		U. S. Government Sponsorship under NASA Contract NAS7-918
|		is acknowledged.

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
l1:	clrb	a0@+
g1:	movl	a0,d2
	andl	d1,d2
	dbeq	d0,l1
	bnes	done
	andl	d0,d1
	lsrl	#2,d0
	bras	g2
l2:	clrl	a0@+
g2:	dbf	d0,l2
	bras	g3
l3:	clrb	a0@+
g3:	dbf	d1,l3

done:	movl	a7@+,d1
	movl	a7@+,d2
	unlk	a6
	rts
