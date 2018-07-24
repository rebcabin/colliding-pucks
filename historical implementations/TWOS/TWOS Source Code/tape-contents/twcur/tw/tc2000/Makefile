#	Copyright (C) 1989, 1991, California Institute of Technology.
#	U.S. Government Sponsorship under NASA Contract NAS7-918
#	is acknowledged.
#
# $Log:	Makefile,v $
# Revision 1.5  91/11/01  14:26:04  reiher
# Added critpath.c (PLR)
# 
# Revision 1.4  91/07/24  10:46:56  configtw
# Add cp_msg module
# 
# Revision 1.3  91/07/17  16:19:18  judy
# New copyright notice.
# 
# Revision 1.2  91/01/24  15:28:33  pls
# add -gs flag to CFLAGS
# 
# Revision 1.1  90/12/12  11:02:59  configtw
# Initial revision
# 
#
# Makefile for TWOS Library (TC2000)
#------------------------------------------------#

DEP = .include.mk

TWOS = .compile.mk

.SUFFIXES:
.SUFFIXES: .c .c~ .h .h~ .s .s~ .o .dcl~ .dcl

CFLAGS = -w -gs -DBF_MACH -DTC2000 -DSOM
DEBUG  = -w -g -DBF_MACH -DTC2000

LFLAGS = -hvacnuz -DBF_MACH


CMODS   = \
	BBNclock.c 	BBNtime.c	\
	BF_MACH_Hg.c	BF_MACHrun.c	\
	HOST_fileio.c 	TCclear.c	\
	TCcomp.c 	TCcopy.c	\
	brkpt.c 	buglib2.c	\
	cmp.c 		comdtab.c	\
	command.c 	commit.c	\
	cp_msg.c	critpath.c	\
	cubeio.c 	deliver.c	\
	fileio.c	\
	flowlog.c 	format.c	\
	getime.c 	gvt2.c		\
	hostifc.c 	idlist.c	\
	int.c 		islog.c		\
	list.c 		loadman.c	\
	loads.c 	machdep.c	\
	machifc.c 	make.c		\
	mem.c 		migr.c		\
	mkmsg.c 	moninit.c	\
	monitor.c 	mproc.c		\
	msgcntl.c 	msglog.c	\
	nq.c 		nullobj.c	\
	objend.c 	objhead.c	\
	objifc.c 	objloc.c	\
	oldtw.c 	quelog.c	\
	rollback.c 	save.c		\
	sched.c 	sendback.c	\
	serve.c 	services.c	\
	start.c 	state.c		\
	stats.c 	stdout.c	\
	storage.c 	str.c		\
	test.c 		tester.c	\
	timewarp.c 	tlib.c		\
	toupper.c	\
	tstrinit.c 	turboq2.c	\
	vtime.c

IMODS   = \
	BBN.h 		func.h		\
	logdefs.h 	machdep.h	\
	tester.h 	twsys.dcl	\
	twsys.h		twcommon.h	\
	vtime.h

SMODS   = TCswitch.s


# Need these definitions since macro substitution
# is not supported on nX.
OMODS   = \
	BBNclock.o 	BBNtime.o	\
	BF_MACH_Hg.o	BF_MACHrun.o	\
	HOST_fileio.o 	TCclear.o	\
	TCcomp.o 	TCcopy.o	\
	brkpt.o 	buglib2.o	\
	cmp.o 		comdtab.o	\
	command.o 	commit.o	\
	cp_msg.o	critpath.o	\
	cubeio.o 	deliver.o	\
	fileio.o	\
	flowlog.o 	format.o	\
	getime.o 	gvt2.o		\
	hostifc.o 	idlist.o	\
	int.o 		islog.o		\
	list.o 		loadman.o	\
	loads.o 	machdep.o	\
	machifc.o 	make.o		\
	mem.o 		migr.o		\
	mkmsg.o 	moninit.o	\
	monitor.o 	mproc.o		\
	msgcntl.o 	msglog.o	\
	nq.o 		nullobj.o	\
	objend.o 	objhead.o	\
	objifc.o 	objloc.o	\
	oldtw.o 	quelog.o	\
	rollback.o 	save.o		\
	sched.o 	sendback.o	\
	serve.o 	services.o	\
	start.o 	state.o		\
	stats.o 	stdout.o	\
	storage.o 	str.o		\
	test.o 		tester.o	\
	timewarp.o 	tlib.o		\
	toupper.o	\
	tstrinit.o 	turboq2.o	\
	vtime.o		TCswitch.o



all	:     $(TWOS)
dbg_all	: dbg $(TWOS)


# IF NOT ON TC2000, THEN DON'T COMPILE
dbg	:
	 if [ "$(ARCH)" = "tc2000" ]; then                   \
	    rm -f $(OMODS);                                \
	    $(CC) $(DEBUG) -c $(CMODS) $(SMODS);           \
	    ar rc tw_DBG.a $(OMODS) >/dev/null 2>&1;    \
	    ranlib tw_DBG.a >/dev/null 2>&1;     \
	    rm -f $(OMODS);                                \
         fi


# IF NOT ON TC2000, THEN DON'T COMPILE.
# NOTE: $(DEP) SHOULD BE FIRST IN DEPENDENCY LIST;
#       AND $(OMODS) SHOULD COME AFTER $(PARALLEL).
#
$(TWOS)	: $(DEP) $(PARALLEL) $(OMODS)
	 if [ "$(ARCH)" = "tc2000" ]; then            \
	    rm -f $(TWOS); touch $(TWOS);           \
	    ar rc tw.a $(OMODS) >/dev/null 2>&1; \
	    ranlib tw.a >/dev/null 2>&1;  \
         fi

# .c files depend on local and shared includes.
$(DEP)	: $(IMODS)
	rm -f $(OMODS); rm -f $(DEP); touch $(DEP)


lint	:
	lint $(LFLAGS) $(CMODS) $(SMODS) >LINT.out 2>&1


.dcl~.dcl	:
	$(GET) $(GFLAGS) -p $< > $*.dcl

.c~.c		:
	$(GET) $(GFLAGS) -p $< > $*.c

.s~.s		:
	$(GET) $(GFLAGS) -p $< > $*.s

.h~.h		:
	$(GET) $(GFLAGS) -p $< > $*.h


# IF NOT ON TC2000, THEN DON'T COMPILE
.c.o		:
	 if [ "$(ARCH)" = "tc2000" ]; then \
            $(CC) $(CFLAGS) -c $< ;      \
         fi

.s.o		:
	 if [ "$(ARCH)" = "tc2000" ]; then \
            $(CC) $(CFLAGS) -c $< ;      \
         fi

clean	:
	rm -f ,* *.o $(TWOS) libtw*.a


# re-compile sources
remake	:
	rm -f $(TWOS); make all

INCLUDES:
#------------------------------------------------#
