|
|	Copyright (C) 1989, 1991, California Institute of Technology.
|	U. S. Government Sponsorship under NASA Contract NAS7-918
|	is acknowledged.
|
| $Log:	BBNswitch.s,v $
| Revision 1.2  91/07/17  15:51:42  judy
| New copyright notice.
| 
| Revision 1.1  90/08/07  15:37:41  configtw
| Initial revision
| 

|	switch_over ( stack_addr, entry_point  );

	.text
	.globl	_switch_over
_switch_over:
	link	a6,#0		| save frame pointer
	moveml	#0xfffc,a7@-	| save registers
	movl	a7,savesp	| save stack pointer
	movl	a6@(8),a0	| get object stack address
	subl	#8,a0		| make room for stack pointer
	movl	a0@(4),d0	| get object entry point
	orl	d0,d0		| start or resume ?
	bnes	sws		|
swr:
	movl	a0@,a7		| restore object stack pointer
|	fmovc	a7+, fcsi
|	fmovem	a7@+,#0xff	| restore floating point registers
	.word	0xf21f, 0xd0ff
	moveml	a7@+,#0x7fff	| restore object registers
	rts			| return to object
sws:
	clrl	a0@(4)		| clear to indicate resume
	movl	a0,a7		| load object stack pointer
	movl	a6@(12),a7@-	| push state address
	movl	d0,a0		|
	jbsr	a0@		| call object

	movl	savesp,a7	| restore main stack pointer
	jbsr	_tobjend	| call object end routine

	moveml	a7@+,#0x7fff	| restore main registers
	rts			| return to main

	.globl	_switch_back
_switch_back:
	link	a6,#0		| save frame pointer
	moveml	#0xfffc,a7@-	| save registers

|	fmovem	#0xff,a7@-	| save floating point registers
|	fmovc    fcsi, -a7
	
	.word	0xf227, 0xe0ff
	movl	a6@(12),a0	| get object stack address
	subl	#8,a0		| make room for stack pointer
	movl	a7,a0@		| save object stack pointer
	movl	savesp,a7	| restore main stack pointer
	lea	a6@(16),a0	| PJH
	addl	#32,a0		| find argument list (8 + 8 args)
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	| copy arguments to main stack
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	|
	movl	a0@-,a7@-	|
	movl	a6@(8),a0	| get object service routine address
	jbsr	a0@		| call object service routine
	addl	#32,a7		|

	moveml	a7@+,#0x7fff	| restore main registers
	rts			| return to main

	.data
savesp:	.long	0
