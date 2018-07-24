|	Copyright (C) 1989, 1991, California Institute of Technology.
|	U. S. Government Sponsorship under NASA Contract NAS7-918
|	is acknowledged.
|
| 
|
|	TWULfastcpy ( dest, src, numbytes )

	.text
	.globl	_TWULfastcpy
_TWULfastcpy:
	movl	a7@(4),a1	| dest 
	movl	a7@(8),a0	| src 
	movl	a1,d0
	subl	a0,d0
	bpls	bkwrd

fwd:
	movl	a7@(12),d0	| numbytes
	lsrl	#2,d0
	bras	g2
l2:	movl	a0@+,a1@+
g2:	dbf		d0,l2
	movl	#3,d0
	andl	a7@(12),d0
	bras	g3
l3:	movb	a0@+,a1@+
g3:	dbf		d0,l3
	rts

bkwrd:
	movl	a7@(12),d0	| numbytes
	addl	d0,a0
	addl	d0,a1
	lsrl	#2,d0
	bras	g4
l4:	movl	a0@-,a1@-
g4:	dbf		d0,l4
	movl	#3,d0
	andl	a7@(12),d0
	bras	g5
l5:	movb	a0@-,a1@-
g5:	dbf		d0,l5
	rts
