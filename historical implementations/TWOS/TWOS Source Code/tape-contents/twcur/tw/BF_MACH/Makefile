#       Copyright (C) 1989, 1991, California Institute of Technology.
#		U. S. Government Sponsorship under NASA Contract NAS7-918
#		is acknowledged.
#
#
# $Log:	Makefile,v $
# Revision 1.12  91/11/01  14:16:28  reiher
# Added critpath.c module
# 
# Revision 1.11  91/11/01  13:38:53  pls
# Replace only recompiled modules in tw.a
# 
# Revision 1.10  91/07/25  11:16:11  configtw
# Make twl.a depend on twulib source rather than objects.
# 
# Revision 1.9  91/07/17  15:52:01  judy
# New copyright notice.
# 
# Revision 1.8  91/07/10  09:57:05  steve
# Added the cp_msg.[co] file. Improved the depend: rule and reran
# make depend.
# 
# Revision 1.7  91/06/14  10:41:21  configtw
# Add dependencies for twl.a.
# 
# Revision 1.6  91/06/13  16:23:35  configtw
# Complete changes for making twl.a
# 
# Revision 1.5  91/03/26  16:39:53  configtw
# Fix bugs in twl.a creation.
# 
# Revision 1.4  91/03/26  14:18:30  configtw
# Change .l to .z for library version suffix.
# 
# Revision 1.3  91/03/26  09:59:32  pls
# 1.  Clean up to work with lint.
# 2.  Add target for library version.
# 3.  Add RBC modifications for Steve.
# 
# Revision 1.2  90/08/16  11:17:00  steve
# removed the Uniform System lib
# removed the files touch.c and faults.c
# 
# Revision 1.1  90/08/06  14:25:01  configtw
# Initial revision
# 
# 
CFLAGS     = -O -X18 -X130 -X140
DFLAGS     = -DBF_MACH -DDLM  -DFAST_VTIME_MACROS -DSOM
LFLAGS     = -DTWUSRLIB
# for rollback chip turn on -DRBC
 
IFLAGS     = -I../

TOUCH=	touch -cf

SOURCE_DIR = ..

LIB_SRC = ../../twulib
LIB_DIR = ../../twulib/BF_MACH

VPATH = ..

# ****** ******** ****** ******
# Application independent files
# ****** ******** ****** ******

BF_MACH_CFILES  = 	\
	BBNclock.c	\
	BBNtime.c	\
	BF_MACHrun.c	\
	BF_MACH_Hg.c	\
	cp_msg.c


BF_MACH_OBJS	=	\
	BF_MACHrun.o	\
	BF_MACH_Hg.o	\
	cp_msg.o



TESTER_AFILES =			\
	BBNswitch.s		\
	BBNclear.s		\
	BBNcomp.s		\
	BBNcopy.s

TESTER_CFILES =			\
	$(SOURCE_DIR)/moninit.c		\
	$(SOURCE_DIR)/toupper.c		\
	$(SOURCE_DIR)/timewarp.c	\
	$(SOURCE_DIR)/machdep.c		\
	$(SOURCE_DIR)/brkpt.c		\
	$(SOURCE_DIR)/tester.c		\
	$(SOURCE_DIR)/msglog.c		\
	$(SOURCE_DIR)/flowlog.c		\
	$(SOURCE_DIR)/islog.c		\
	$(SOURCE_DIR)/msgcntl.c		\
	$(SOURCE_DIR)/tstrinit.c	\
	$(SOURCE_DIR)/command.c		\
	$(SOURCE_DIR)/objifc.c		\
	$(SOURCE_DIR)/machifc.c		\
	$(SOURCE_DIR)/hostifc.c		\
	$(SOURCE_DIR)/comdtab.c


TESTER_OBJS =			\
	moninit.o		\
	toupper.o		\
	BBNswitch.o		\
	BBNclear.o		\
	BBNcomp.o		\
	BBNcopy.o		\
	BBNtime.o		\
	BBNclock.o		\
	timewarp.o		\
	machdep.o		\
	brkpt.o			\
	tester.o		\
	msglog.o		\
	flowlog.o		\
	islog.o			\
	msgcntl.o		\
	command.o		\
	objifc.o		\
	machifc.o		\
	hostifc.o		\
	comdtab.o

NONLIB_OBJS =			\
	tstrinit.o

LIB_CODE =		        \
    $(LIB_SRC)/twpackages.c    \
    $(LIB_SRC)/TWULarray.c \
    $(LIB_SRC)/outList.c \
    $(LIB_DIR)/TWULfastcpy.s   \
    $(LIB_SRC)/twusrlib.c       \
    $(LIB_SRC)/evtList.c   \
    $(LIB_SRC)/msgList.c   \
    $(LIB_SRC)/multiPacket.c    \
    tstrinit.z

LIB_OBJS =			\
    $(LIB_DIR)/twpackages.o    \
    $(LIB_DIR)/TWULarray.o \
    $(LIB_DIR)/outList.o \
    $(LIB_DIR)/TWULfastcpy.o   \
    $(LIB_DIR)/twusrlib.o		\
    $(LIB_DIR)/evtList.o   \
    $(LIB_DIR)/msgList.o   \
    $(LIB_DIR)/multiPacket.o	\
	tstrinit.z

TW_HFILES =			\
	$(SOURCE_DIR)/BBN.h		\
	$(SOURCE_DIR)/func.h		\
	$(SOURCE_DIR)/logdefs.h		\
	$(SOURCE_DIR)/machdep.h		\
	$(SOURCE_DIR)/tester.h		\
	$(SOURCE_DIR)/twcommon.h	\
	$(SOURCE_DIR)/twsys.h		\
	$(SOURCE_DIR)/vtime.h

TW_CFILES =			\
	$(SOURCE_DIR)/buglib2.c		\
	$(SOURCE_DIR)/cmp.c		\
	$(SOURCE_DIR)/commit.c		\
	$(SOURCE_DIR)/critpath.c	\
	$(SOURCE_DIR)/cubeio.c		\
	$(SOURCE_DIR)/deliver.c		\
	$(SOURCE_DIR)/fileio.c		\
	$(SOURCE_DIR)/format.c		\
	$(SOURCE_DIR)/getime.c		\
	$(SOURCE_DIR)/gvt2.c		\
	$(SOURCE_DIR)/idlist.c		\
	$(SOURCE_DIR)/int.c		\
	$(SOURCE_DIR)/list.c		\
	$(SOURCE_DIR)/loadman.c		\
	$(SOURCE_DIR)/loads.c		\
	$(SOURCE_DIR)/make.c		\
	$(SOURCE_DIR)/mem.c		\
	$(SOURCE_DIR)/migr.c		\
	$(SOURCE_DIR)/mkmsg.c		\
	$(SOURCE_DIR)/monitor.c		\
	$(SOURCE_DIR)/mproc.c		\
	$(SOURCE_DIR)/nq.c		\
	$(SOURCE_DIR)/nullobj.c		\
	$(SOURCE_DIR)/objend.c		\
	$(SOURCE_DIR)/objhead.c		\
	$(SOURCE_DIR)/objloc.c		\
	$(SOURCE_DIR)/oldtw.c		\
	$(SOURCE_DIR)/quelog.c		\
	$(SOURCE_DIR)/rollback.c	\
	$(SOURCE_DIR)/save.c		\
	$(SOURCE_DIR)/sched.c		\
	$(SOURCE_DIR)/sendback.c	\
	$(SOURCE_DIR)/serve.c		\
	$(SOURCE_DIR)/services.c	\
	$(SOURCE_DIR)/start.c		\
	$(SOURCE_DIR)/state.c		\
	$(SOURCE_DIR)/stats.c		\
	$(SOURCE_DIR)/stdout.c		\
	$(SOURCE_DIR)/storage.c		\
	$(SOURCE_DIR)/str.c		\
	$(SOURCE_DIR)/tlib.c		\
	$(SOURCE_DIR)/turboq2.c		\
	$(SOURCE_DIR)/vtime.c		\
	$(SOURCE_DIR)/HOST_fileio.c

TW_OBJS	=			\
	buglib2.o		\
	cmp.o			\
	commit.o		\
	critpath.o		\
	cubeio.o		\
	deliver.o		\
	fileio.o		\
	format.o		\
	getime.o		\
	gvt2.o			\
	int.o			\
	idlist.o		\
	list.o			\
	loadman.o		\
	loads.o			\
	make.o			\
	mem.o			\
	migr.o			\
	mkmsg.o			\
	monitor.o		\
	mproc.o			\
	nq.o			\
	nullobj.o		\
	objend.o		\
	objhead.o		\
	objloc.o		\
	quelog.o		\
	rollback.o		\
	save.o			\
	sched.o			\
	sendback.o		\
	serve.o			\
	services.o		\
	start.o			\
	state.o			\
	stats.o			\
	stdout.o		\
	storage.o		\
	str.o			\
	turboq2.o		\
	tlib.o			\
	vtime.o			\
	oldtw.o			\
	HOST_fileio.o

RBC_HFILES =				\
	$(SOURCE_DIR)/rbc_emulator.h	\
	$(SOURCE_DIR)/rbc_private.h	\
	$(SOURCE_DIR)/rbc_public.h

RBC_CFILES =				\
	$(SOURSE_DIR)/rbc_debug.c	\
	$(SOURSE_DIR)/rbc_driver.c	\
	$(SOURSE_DIR)/rbc_emulator.c	\
	$(SOURSE_DIR)/rbc_malloc.c	\
	$(SOURSE_DIR)/rbc_op.c

RBC_OBJS =			\
	rbc_debug.o		\
	rbc_driver.o		\
	rbc_emulator.o		\
	rbc_malloc.o		\
	rbc_op.o


all: regular

rbc:
	if [ -f RBC_compiled ]; then \
		make CFLAGS="-DRBC $(CFLAGS)" rbc.a; \
	else \
		rm -f tw.a *.o; \
		touch RBC_compiled; \
		make CFLAGS="-DRBC $(CFLAGS)" rbc.a; \
	fi

regular:
	if [ -f RBC_compiled ]; then \
		rm -f tw.a *.o; \
		rm -f RBC_compiled; \
		make tw.a; \
	else \
		make tw.a; \
	fi



tw.a: $(PARALLEL) $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS) $(NONLIB_OBJS)
	ar rv $@ $?
	ranlib tw.a

twl.a: $(PARALLEL) $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS) $(LIB_CODE)
	cd $(LIB_DIR); rm -f *.o; make -f Makefile.tw
	make tstrinit.z
	ar rcv twl.a $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS) $(LIB_OBJS)
	ranlib twl.a


rbc.a: $(PARALLEL) $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS)  $(RBC_OBJS)
	ar rcv tw.a $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS)  $(RBC_OBJS)
	ranlib tw.a


twdbg.a: $(PARALLEL) $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS) 
	ar rcv twdbg.a $(BF_MACH_OBJS) $(TESTER_OBJS) $(TW_OBJS) 
	ranlib twdbg.a


check:
	cc -O -o check check.c

readmap:
	cc -O -o readmap SUNreadmap.c

clean:
	-rm *.o

backup:
	-chmod 0777 back/*
	cp *.[chsd]* str levels *akefil* back/.
	-chmod 0444 back/*
	ls -l back

lint_tw:	$(TW_CFILES) $(TESTER_CFILES) $(BF_MACH_CFILES) $(TW_HFILES)
	lint -u $(DFLAGS) $(IFLAGS) $(TW_CFILES) $(BF_MACH_CFILES) \
	  $(TESTER_CFILES) > lint_tw


tstrinit.z:	tstrinit.c
	cc $(CFLAGS) $(DFLAGS) $(LFLAGS) $(IFLAGS) -c ../tstrinit.c
	mv tstrinit.o tstrinit.z

.c.o:
	cc $(CFLAGS) $(DFLAGS) $(IFLAGS) -c $*.c

.c.s:
	cc $(CFLAGS) $(DFLAGS) $(IFLAGS) -S $*.c

.s.o:
	gas -o $*.o $*.s

#----------------------------------------------------------------------------
# the following bit of magic makes the "str" and "levels" files required by
# Mike Di Loreto's "monitor" utility
#----------------------------------------------------------------------------
# This version here only gets func definitions for functions that say Debug
#----------------------------------------------------------------------------
#str:	$(TW_CFILES) $(TESTER_CFILES)
#	mv str str.bak
#	awk '/FUNCTION/ { str = "/*-----FUNCTION in " FILENAME "-----*/\n" ; \
#			keeping = 1 ; } \
#			{ if (keeping) { \
#			    for (i=1; i<=NF; i++) { \
#			      if ($$i == "{") { keeping = 0 ; } \
#			      else if($$i!="FUNCTION"){str=str $$i " ";} \
#			      if (i == NF) { str = str "\n" ; } \
#			      } } } \
#	     /Debug/ {print str; print "{}"}' \
#	     $(TW_CFILES) $(TESTER_CFILES) > .temp
#	-indent  -i4 -di16 -l79 -cli0 -c33 -ip -bc  -npsl -cdb -sc -nfc1 \
#		    -bl -nce -pcs -bap -bad -bbb -nsob -d0 -nv .temp
#	cat .temp | sed -e '/{/d' -e '/}/d' \
#			-e 's/*\([a-zA-Z]\)/\* \1/' | tee str | \
#	awk '/FUNCTION/ { ok=1 } \
#	          { for (i=2; i<=NF; i++) { \
#		      prev = $$(i-1) ; if (ok==1 && substr($$i,1,1) == "(") { \
#			print prev "\t\t0" ; ok = 0 ; }}} ' > levels
#	-rm .temp
#----------------------------------------------------------------------------
# This version here gets func definitions for functions that say FUNCTION
#----------------------------------------------------------------------------
str:	$(TW_CFILES) $(TESTER_CFILES)
#	mv str str.bak
	awk '\
	    /\".*\"/ {next}\
	    /FUNCTION/ \
		{ str = "/*-----FUNCTION in " FILENAME "-----*/\n" ; \
		keeping = 1 ; } \
		{ if (keeping) { \
		    for (i=1; i<=NF; i++) { \
		      if ($$i == "{") { ok=1 ; keeping = 0 ; } \
		      else if($$i!="FUNCTION"){str=str $$i " ";} \
		      if (i == NF) { str = str "\n" ; } \
		      } } } \
	     /{/ { if(ok) { ok=0; print str; print "{}"}}' \
	     $(TW_CFILES) $(TESTER_CFILES) > .temp
	-indent  -i4 -di16 -l79 -cli0 -c33 -ip -bc  -npsl -cdb -sc -nfc1 \
		    -bl -nce -pcs -bap -bad -bbb -nsob -d0 -nv .temp
	cat .temp | sed -e '/{/d' -e '/}/d' \
			-e 's/*\([a-zA-Z]\)/\* \1/' | tee str | \
	awk '/FUNCTION/ { ok=1 } \
	          { for (i=2; i<=NF; i++) { \
		      prev = $$(i-1) ; if (ok==1 && substr($$i,1,1) == "(") { \
			ok = 0; \
			if (length(prev) < 8){ print prev "\t\t\t\t0"} \
			else if (length(prev)<16) {print prev "\t\t\t0"} \
			else if (length(prev)<24) {print prev "\t\t0"} \
			else {print prev "\t0"} \
			}}} ' > levels
	-rm .temp
#----------------------------------------------------------------------------

depend: $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES)
	egrep '^#include' $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES) | \
	sed     -e '/</d' \
		-e '/#include "kmsgh.h"/d' \
		-e '/#include "XPUTER.h"/d' \
		-e 's/\.c/.o/' \
		-e 's/:[^"]*"\([^"]*\)".*/: ..\/\1/'\
		-e 's,^../[a-zA-Z0-9]*/\([^\.]*\)\.[cs],\1.o \1.L,' \
		-e 's/^..\///' \
		| \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ex - Makefile < eddep
	-rm eddep makedep

# DO NOT DELETE THIS LINE

machdep.h: ../BBN.h
twcommon.h: ../vtime.h
twsys.h: ../twsys.dcl
buglib2.o: ../twcommon.h ../twsys.h ../tester.h
cmp.o: ../twcommon.h ../twsys.h
commit.o: ../twcommon.h ../twsys.h
critpath.o: ../twcommon.h ../twsys.h ../machdep.h ../tester.h
cubeio.o: ../twcommon.h ../twsys.h ../machdep.h
deliver.o: ../twcommon.h ../twsys.h ../machdep.h
fileio.o: ../twcommon.h ../twsys.h ../machdep.h
format.o: ../twcommon.h ../twsys.h
getime.o: ../twcommon.h ../twsys.h ../machdep.h
gvt2.o: ../twcommon.h ../twsys.h ../machdep.h
int.o: ../twcommon.h ../twsys.h
list.o: ../twcommon.h ../twsys.h
loadman.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
loads.o: ../twcommon.h ../twsys.h ../machdep.h ../tester.h
make.o: ../twcommon.h ../twsys.h
mem.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
migr.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
mkmsg.o: ../twcommon.h ../twsys.h
monitor.o: ../func.h
mproc.o: ../twcommon.h ../twsys.h
nq.o: ../twcommon.h ../twsys.h
objend.o: ../twcommon.h ../twsys.h
objhead.o: ../twcommon.h ../twsys.h
objloc.o: ../twcommon.h ../twsys.h
oldtw.o: ../twcommon.h ../twsys.h
quelog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h ../logdefs.h
rollback.o: ../twcommon.h ../twsys.h
save.o: ../twcommon.h ../twsys.h
sched.o: ../twcommon.h ../twsys.h
sendback.o: ../twcommon.h ../twsys.h ../machdep.h
serve.o: ../twcommon.h ../twsys.h
services.o: ../twcommon.h ../twsys.h
start.o: ../twcommon.h ../twsys.h
state.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
stats.o: ../twcommon.h ../twsys.h ../machdep.h
storage.o: ../twcommon.h ../twsys.h ../machdep.h
str.o: ../twcommon.h ../twsys.h
turboq2.o: ../twcommon.h ../twsys.h
vtime.o: ../twcommon.h
HOST_fileio.o: ../twcommon.h ../twsys.h ../machdep.h
moninit.o: ../twcommon.h ../twsys.h ../func.h
timewarp.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
machdep.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
brkpt.o: ../twcommon.h ../twsys.h
tester.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
msglog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h ../logdefs.h
flowlog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h ../logdefs.h
islog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h ../logdefs.h
msgcntl.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
tstrinit.o: ../twcommon.h ../twsys.h ../machdep.h ../tester.h
command.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
objifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
machifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
hostifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
comdtab.o: ../twcommon.h ../twsys.h ../tester.h
