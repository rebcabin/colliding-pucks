#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U.S. Government Sponsorship under NASA Contract NAS7-918
#		is acknowledged.       */	
# 
#
# $Log:	Makefile,v $
# Revision 1.8  91/11/01  14:23:03  reiher
# Added critpath.c (PLR)
# 
# Revision 1.7  91/07/17  15:59:04  judy
# New copyright notice.
# 
# Revision 1.6  91/07/17  09:19:53  configtw
# Make twl depend on library source, not objects.
# 
# Revision 1.5  91/07/10  10:30:06  steve
# Updated host rules.  Reran make depend.
# 
# Revision 1.4  91/07/10  10:25:52  configtw
# Add support for new library code.
# 
# Revision 1.3  91/06/07  14:41:30  configtw
# Fix bugs.
# 
# Revision 1.2  91/06/07  14:28:48  configtw
# Change dependency stuff.
# 
# Revision 1.1  90/08/07  11:12:48  configtw
# Initial revision
# 
#
#CFLAGS     = -f68881 -g  -DSUN -DFAST_VTIME_MACROS -Bstatic
CFLAGS     = -f68881 -O  -DSUN -DFAST_VTIME_MACROS -Bstatic -DSOM
IFLAGS     = -I../
LFLAGS     = -DTWUSRLIB

LIB_SRC = ../../twulib
LIB_DIR = ../../twulib/Sun3

# ****** ******** ****** ******
# Application independent files
# ****** ******** ****** ******

TESTER_AFILES =			\
	SUNswitch.s		\
	SUNclear.s		\
	SUNcomp.s		\
	SUNcopy.s		\
	SUNtime.c		\
	SUN_Hg.c		\
	SUNnetwork.c

TESTER_CFILES =			\
	moninit.c		\
	toupper.c		\
	timewarp.c		\
	machdep.c		\
	brkpt.c			\
	tester.c		\
	msglog.c		\
	flowlog.c		\
	islog.c			\
	msgcntl.c		\
	tstrinit.c		\
	command.c		\
	objifc.c		\
	machifc.c		\
	hostifc.c		\
	comdtab.c		

TESTER_OBJS =			\
	moninit.o		\
	toupper.o		\
	SUNswitch.o		\
	SUNclear.o		\
	SUNcomp.o		\
	SUNcopy.o		\
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
	comdtab.o		\
	SUNtime.o		\
	SUN_Hg.o		\
	SUNnetwork.o

NONLIB_OBJS =           \
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

LIB_OBJS =          \
    $(LIB_DIR)/twpackages.o    \
    $(LIB_DIR)/TWULarray.o \
    $(LIB_DIR)/outList.o \
    $(LIB_DIR)/TWULfastcpy.o   \
    $(LIB_DIR)/twusrlib.o       \
    $(LIB_DIR)/evtList.o   \
    $(LIB_DIR)/msgList.o   \
    $(LIB_DIR)/multiPacket.o    \
    tstrinit.z

TW_HFILES =			\
	func.h			\
	machdep.h		\
	twcommon.h		\
	twsys.h			\
	tester.h		\
	vtime.h


TW_CFILES =			\
	buglib2.c		\
	cmp.c			\
	commit.c		\
	critpath.c		\
	cubeio.c		\
	deliver.c		\
	fileio.c		\
	format.c		\
	getime.c		\
	gvt2.c			\
	int.c			\
	idlist.c		\
	list.c			\
	loadman.c		\
	loads.c			\
	make.c			\
	mem.c			\
	migr.c			\
	mkmsg.c			\
	monitor.c		\
	mproc.c			\
	nq.c			\
	nullobj.c		\
	objend.c		\
	objhead.c		\
	objloc.c		\
	quelog.c		\
	rollback.c		\
	save.c			\
	sched.c			\
	sendback.c		\
	serve.c			\
	services.c		\
	start.c			\
	state.c			\
	stats.c			\
	storage.c		\
	turboq2.c		\
	tlib.c			\
	oldtw.c			\
	vtime.c			\
	HOST_fileio.c

HOST_CFILES =	\
	SUNhost.c		\
	SUNnetwork.c	\
	SUN_Hg.c		\
	cp_msg.c

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
	turboq2.o		\
	tlib.o			\
	oldtw.o			\
	vtime.o			\
	HOST_fileio.o

HOST_OBJS =	\
	SUNhost.o		\
	SUNnetwork.o	\
	SUN_Hg.o		\
	cp_msg.o

all:	tw.a host


tw.a:	$(TESTER_OBJS) $(TW_OBJS) $(NONLIB_OBJS)
	ar rv $@ $?
	ranlib $@

twl.a:  $(TESTER_OBJS) $(TW_OBJS) $(LIB_CODE)
	cd $(LIB_DIR); rm -f *.o; make -f Makefile.tw
	make tstrinit.z
	ar rcv twl.a $(TESTER_OBJS) $(TW_OBJS) $(LIB_OBJS)
	ranlib twl.a

q:	$(TESTER_OBJS) $(TW_OBJS) q.o qrand.o random.o
	cc $(CFLAGS) -o q $(TESTER_OBJS) $(TW_OBJS) q.o qrand.o random.o -lm
	nm -g q > tester.map
	readmap

testio:	$(TESTER_OBJS) $(TW_OBJS) testio.o
	cc $(CFLAGS) -o testio $(TESTER_OBJS) $(TW_OBJS) testio.o -lm
	nm -g testio > tester.map
	readmap

host:   $(HOST_OBJS)
	cc $(CFLAGS) -o host $(HOST_OBJS)

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

lint_tw:	$(TW_CFILES) $(TESTER_CFILES)
	lint $(TW_CFILES) $(TESTER_CFILES) > lint_tw

saber:
	#load  -DSABER $(CFLAGS) $(TW_CFILES) $(TESTER_CFILES)

tstrinit.z: ../tstrinit.c
	cc $(CFLAGS) $(LFLAGS) $(IFLAGS) -c ../tstrinit.c
	mv tstrinit.o tstrinit.z


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c

.s.o:
	as -o $*.o $*.s

%.o: ../%.c
	cc $(CFLAGS) $(IFLAGS) -c ../$*.c

%.s: ../%.c
	cc $(CFLAGS) $(IFLAGS) -S ../$*.c

%.o: ../%.s
	as -o $*.o ../$*.s



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

#depend: $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES) $(HOST_CFILES)
depend:
	cd ..; egrep '^#include' $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES) $(HOST_CFILES) | \
	sed     -e '/</d' \
		-e '/#include "kmsgh.h"/d' \
		-e '/#include "XPUTER.h"/d' \
		-e '/#include "BBN.h"/d' \
		-e 's/\.c/.o/' \
		-e 's/:[^"]*"\([^"]*\)".*/: ..\/\1/'\
		-e 's,^../[a-zA-Z0-9]*/\([^\.]*\)\.[cs],\1.o \1.L,' \
		| \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r ../makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ex - Makefile < eddep
	-rm eddep ../makedep
# DO NOT DELETE THIS LINE

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
turboq2.o: ../twcommon.h ../twsys.h
oldtw.o: ../twcommon.h ../twsys.h
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
SUNhost.o: ../twcommon.h ../twsys.h ../machdep.h
SUNnetwork.o: ../twcommon.h ../twsys.h ../machdep.h
SUN_Hg.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
cp_msg.o: ../twcommon.h ../logdefs.h
