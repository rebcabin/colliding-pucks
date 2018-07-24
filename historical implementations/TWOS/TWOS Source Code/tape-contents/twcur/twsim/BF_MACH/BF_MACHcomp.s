|	Copyright (C) 1989, 1991, California Institute of Technology.
|		U. S. Government Sponsorship under NASA Contract NAS7-918
|		is acknowledged.

|	bytcmp ( a, b, l )

	.text
	.globl	_bytcmp
_bytcmp:
	movl	a7@(4),a0	| a 
	movl	a7@(8),a1	| b 
	movl	a7@(12),d0	| l
	lsrl	#2,d0
	bras	g2
l2:	cmpml	a1@+,a0@+
	bnes	out
g2:	dbf	d0,l2
	movl	#3,d0
	andl	a7@(12),d0
	bras	g3
l3:	cmpmb	a1@+,a0@+
	bnes	out
g3:	dbf	d0,l3
	clrl	d0
	rts
out:	bccs	gtr
	movl	#-1,d0
	rts
gtr:	movl	#1,d0
	rts
