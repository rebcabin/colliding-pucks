#
# $Log:	Makefile,v $
# Revision 1.2  91/06/07  14:28:48  configtw
# Change dependency stuff.
# 
# Revision 1.1  90/08/07  11:12:48  configtw
# Initial revision
# 
#
#CFLAGS     = -f68881 -g  -DSUN -DFAST_VTIME_MACROS -Bstatic
CFLAGS     = -f68881 -O  -DSUN -DFAST_VTIME_MACROS -Bstatic
IFLAGS     = -I../

# ****** ******** ****** ******
# Application independent files
# ****** ******** ****** ******

TESTER_AFILES =			\
	SUNswitch.s		\
	SUNclear.s		\
	SUNcomp.s		\
	SUNcopy.s

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
	tstrinit.o		\
	command.o		\
	objifc.o		\
	machifc.o		\
	hostifc.o		\
	comdtab.o		\
	SUNnetwork.o

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

TW_OBJS	=			\
	buglib2.o		\
	cmp.o			\
	commit.o		\
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

all:	tw.a


tw.a:	$(TESTER_OBJS) $(TW_OBJS)
	ar rv $@ $?
	ranlib $@

q:	$(TESTER_OBJS) $(TW_OBJS) q.o qrand.o random.o
	cc $(CFLAGS) -o q $(TESTER_OBJS) $(TW_OBJS) q.o qrand.o random.o -lm
	nm -g q > tester.map
	readmap

testio:	$(TESTER_OBJS) $(TW_OBJS) testio.o
	cc $(CFLAGS) -o testio $(TESTER_OBJS) $(TW_OBJS) testio.o -lm
	nm -g testio > tester.map
	readmap

host:   SUNhost.o SUNnetwork.o
	cc $(CFLAGS) -o host SUNhost.o SUNnetwork.o

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

#depend: $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES)
depend:
    cd ..; egrep '^#include' $(TW_HFILES) $(TW_CFILES) $(TESTER_CFILES) | \
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

buglib2.o: ../twcommon.h ../twsys.h
cmp.o: ../twcommon.h ../twsys.h
commit.o: ../twcommon.h ../twsys.h
cubeio.o: ../twcommon.h ../twsys.h
deliver.o: ../twcommon.h ../twsys.h
fileio.o: ../machdep.h ../twcommon.h ../twsys.h
format.o: ../twcommon.h ../twsys.h
gvt2.o: ../machdep.h ../twcommon.h ../twsys.h
int.o: ../twcommon.h ../twsys.h
idlist.o: ../machdep.h
list.o: ../twcommon.h ../twsys.h
loadman.o: ../twcommon.h ../twsys.h
make.o: ../twcommon.h ../twsys.h
mem.o: ../machdep.h ../twcommon.h ../twsys.h ../tester.h
mproc.o: ../twcommon.h ../twsys.h
nq.o: ../twcommon.h ../twsys.h
objend.o: ../twcommon.h ../twsys.h
objhead.o: ../twcommon.h ../twsys.h
quelog.o: ../twcommon.h ../twsys.h
rollback.o: ../twcommon.h ../twsys.h
save.o: ../twcommon.h ../twsys.h
sched.o: ../twcommon.h ../twsys.h
sendback.o: ../twcommon.h ../twsys.h
serve.o: ../twcommon.h ../twsys.h
services.o: ../twcommon.h ../twsys.h
start.o: ../twcommon.h ../twsys.h
state.o: ../twcommon.h ../twsys.h
stats.o: ../twcommon.h ../twsys.h
storage.o: ../twcommon.h ../twsys.h
turboq2.o: ../twcommon.h ../twsys.h
objloc.o: ../twcommon.h ../twsys.h
getime.o: ../machdep.h
mkmsg.o: ../twcommon.h ../twsys.h
monitor.o: ../func.h
timewarp.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
machdep.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
brkpt.o: ../twcommon.h ../twsys.h
tester.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
msglog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
flowlog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
islog.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
msgcntl.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
tstrinit.o: ../machdep.h ../twcommon.h ../twsys.h ../tester.h
command.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
objifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
machifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
hostifc.o: ../twcommon.h ../twsys.h ../tester.h ../machdep.h
comdtab.o: ../twcommon.h ../twsys.h ../tester.h
HOST_fileio.o: ../twsys.h
migr.o: ../twcommon.h ../twsys.h ../tester.h
