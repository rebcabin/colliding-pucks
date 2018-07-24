|	Copyright (C) 1989, 1991, California Institute of Technology.
|	U. S. Government Sponsorship under NASA Contract NAS7-918
|	is acknowledged.

|
| $Log:	BBNcomp.s,v $
| Revision 1.3  91/11/01  13:21:05  pls
| 1.  Handle 64K boundary conditions (SCR 165).
| 
| Revision 1.2  91/07/17  15:51:32  judy
| New copyright notice.
| 
| Revision 1.1  90/08/07  15:37:37  configtw
| Initial revision
| 

|	bytcmp ( a, b, l )

	.text
	.globl	_bytcmp
_bytcmp:
	movl	a7@(4),a0	| a 
	movl	a7@(8),a1	| b 
	movl	a7@(12),d0	| l
	lsrl	#2,d0
	orb		#4,cc		| set Z bit
	bras	g2
l20:
	swap	d0			| restore count
l2:
	cmpml	a1@+,a0@+
g2:						| must jump here with Z bit set
	dbne	d0,l2
	bnes	out
	swap	d0			| look at high 16 bits of count
	dbf		d0,l20		| decrement high part
	movl	#3,d0
	andl	a7@(12),d0
	bras	g3
l3:
	cmpmb	a1@+,a0@+
	bnes	out
g3:
	dbf		d0,l3
	clrl	d0
	rts
out:
	bccs	gtr
	movl	#-1,d0
	rts
gtr:
	movl	#1,d0
	rts
