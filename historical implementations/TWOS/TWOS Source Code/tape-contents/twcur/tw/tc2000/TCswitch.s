;	Copyright (C) 1989, 1991, California Institute of Technology.
;	U.S. Government Sponsorship under NASA Contract NAS7-918
;	is acknowledged.		
;
; $Log:	TCswitch.s,v $
; Revision 1.2  91/07/17  16:22:33  judy
; New copyright notice.
; 
; Revision 1.1  90/12/12  11:07:28  configtw
; Initial revision
; 
;
; @(#)switch.s	1.9 10/30/90
; Context Switch Routines
;
; ----------------------------------------------------
; SWITCH_OVER
;  interesting arguments:
;     r2 - object stack pointer
;     r3 - object state pointer
; ----------------------------------------------------
	text
	align	4
;
	global	_switch_over
_switch_over:
;
; save return pc and fp, adjust sp
;
	subu	r31,r31,72
	st	r1 ,r31,68
	st	r30,r31,64
	addu	r30,r31,64
;
; save registers r14-r29
;
	st.d	r14,r31,0
	st.d	r16,r31,8
	st.d	r18,r31,16
	st.d	r20,r31,24
	st.d	r22,r31,32
	st.d	r24,r31,40
	st.d	r26,r31,48
	st.d	r28,r31,56
;
; grab copies of the interesting arguments
;
	or	r20,r0 ,r2
	or	r21,r0 ,r3
;
; save current sp to 'save_sp'
;
	or.u	r12,r0 ,hi16(save_sp)
	st	r31,r12,lo16(save_sp)
;
; object stack address: argument in r2(r20)
;  check object entry point for zero
;  if zero then fall through, else branch to s_start
;
	subu	r20,r20,8
	ld	r10,r20,4
	bcnd	ne0,r10,s_start
;
; grab new sp, restore object registers (r14-r29) and
; return control to object
;
s_resume:
	ld	r31,r20,0
;
	ld.d	r14,r31,0
	ld.d	r16,r31,8
	ld.d	r18,r31,16
	ld.d	r20,r31,24
	ld.d	r22,r31,32
	ld.d	r24,r31,40
	ld.d	r26,r31,48
	ld.d	r28,r31,56
;
	ld	r30,r31,64
	ld	r1 ,r31,68
	addu	r31,r31,72
	jmp	r1
;
; object state: argument in r3(r21)
;  clear object entry point (in memory), adjust sp, put the
;  state address in argument register r2, then jump to object
;  entry point (value in r10)
;
s_start:
	st	r0 ,r20,4
	or	r31,r0 ,r20
	or	r2 ,r0 ,r3
;
	jsr	r10
;
; restore main sp to 'save_sp'
;
	or.u	r12,r0 ,hi16(save_sp)
	ld	r31,r12,lo16(save_sp)
;
; jump to _tobjend routine
;
	bsr	_tobjend
;
; restore registers and return to caller
;
	ld.d	r14,r31,0
	ld.d	r16,r31,8
	ld.d	r18,r31,16
	ld.d	r20,r31,24
	ld.d	r22,r31,32
	ld.d	r24,r31,40
	ld.d	r26,r31,48
	ld.d	r28,r31,56
;
	ld	r30,r31,64
	ld	r1 ,r31,68
	addu	r31,r31,72
	jmp	r1
;
; ----------------------------------------------------
; SWITCH_BACK
;  interesting arguments:
;     r2 - object service routine entry point (address)
;     r3 - object stack pointer
;     r4 - service routine arg1 (all)
;     r5 - service routine arg2 (obcreate_b)
;     r6 - service routine arg3 (obcreate_b)
;
;     for sv_tell, sv_create, and sv_destroy, we need
;      to worry about the RCVTIME structure passed on
;      the stack (sv_tell is "worse" case):
;
;
;      SWITCH_BACK STACK ARGUMENT AREA -- SV_TELL
;
;	|--------------|
;	|   skipped    | switch_back arg1, in r2
;	|--------------|      
;	|   skipped    | switch_back arg2, in r3
;	|--------------|
;	|   skipped    | sv_tell arg1, in r4
;	|--------------|      
;	|   skipped    | alignment
;	|--------------|      
;	| arg2         | sv_tell arg2 (16-bytes)
;	|--------------|
;	|  ''          |
;	|--------------|
;	|  ''          |
;	|--------------|
;	|  ''          |
;	|--------------|
;	| arg3         | sv_tell arg3
;	|--------------|
;	| arg4         | sv_tell arg4
;	|--------------|
;	| arg5         | sv_tell arg5
;	|--------------|      
;	|   padding    | alignment (?)
;	|--------------|
;
;
;      SV_TELL STACK ARGUMENT AREA -- EXPECTED
;
;	|--------------|
;	|   skipped    | sv_tell arg1, in r2
;	|--------------|      
;	|   skipped    | alignment
;	|--------------|      
;	| arg2         | sv_tell arg2 (16-bytes)
;	|--------------|
;	|  ''          |
;	|--------------|
;	|  ''          |
;	|--------------|
;	|  ''          |
;	|--------------|  
;	|   skipped    | sv_tell arg3, in r8
;	|--------------|
;	|   skipped    | sv_tell arg4, in r9
;	|--------------|
;	| arg5         | sv_tell arg5
;	|--------------|      
;	|   padding    | alignment (?)
;	|--------------|
;
;
; ----------------------------------------------------
;
	global	_switch_back
_switch_back:
;
; save return pc and fp, adjust sp
;
	subu	r31,r31,72
	st	r1 ,r31,68
	st	r30,r31,64
	addu	r30,r31,64
;
; save registers r14-r29
;
	st.d	r14,r31,0
	st.d	r16,r31,8
	st.d	r18,r31,16
	st.d	r20,r31,24
	st.d	r22,r31,32
	st.d	r24,r31,40
	st.d	r26,r31,48
	st.d	r28,r31,56
;
; grab copies of arguments
;
	or	r20,r0 ,r2
	or	r21,r0 ,r3
	or	r22,r0 ,r4
	or	r23,r0 ,r5
	or	r24,r0 ,r6
;
; object stack address: argument in r3(r21)
;  save object stack pointer
;
	subu	r21,r21,8
	st	r31,r21,0
;
; restore main sp to 'save_sp'
;
	or.u	r12,r0 ,hi16(save_sp)
	ld	r31,r12,lo16(save_sp)
;
; arguments in r4,r5,r6(r22,r23,r24)
;  copy service routine arguments to argument registers
;
	or	r2,r0 ,r22
	or	r3,r0 ,r23
	or	r4,r0 ,r24
;
; adjust sp (pushing part of argument area, 40-bytes)
;
	subu	r31,r31,40
;
; using current frame pointer (r30), copy part of the
;  argument area to the current stack
;
	ld.d	r28,r30,16
	st.d	r28,r31, 0
;
	ld.d	r28,r30,24
	st.d	r28,r31, 8
;
	ld.d	r28,r30,32
	st.d	r28,r31,16
;
	ld.d	r28,r30,40
	st.d	r28,r31,24
;
; also copy these to r8/r9
;
	ld.d	r8 ,r30,40
;
	ld.d	r28,r30,48
	st.d	r28,r31,32
;
; jump to object service routine:
;  address in r2(r20)
;
	jsr	r20
;
; adjust sp (popping allocated argument area, 40-bytes)
;
	addu	r31,r31,40
;
; restore main registers (r14-r29) and return
;
	ld.d	r14,r31,0
	ld.d	r16,r31,8
	ld.d	r18,r31,16
	ld.d	r20,r31,24
	ld.d	r22,r31,32
	ld.d	r24,r31,40
	ld.d	r26,r31,48
	ld.d	r28,r31,56
;
	ld	r30,r31,64
	ld	r1 ,r31,68
	addu	r31,r31,72
	jmp	r1
;
; allocate space for data (save_sp)
;
	align	4
	data
;
save_sp:
	word	0
