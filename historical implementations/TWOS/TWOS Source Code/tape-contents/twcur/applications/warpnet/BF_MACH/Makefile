
SIMDIR	   = ../../../twsim/BF_MACH
TWDIR	= ../../../tw/BF_MACH
INCDIR	= ../../../tw






IFLAGS	      =  -DBF_MACH -DFAST_VTIME_MACROS \
		 -I.. -I$(INCDIR)


CFLAGS	      =   -O  -X130 -DOLDTW
# -X129 -X140

VPATH = ..

LIB_FLAGS     =

 


OBJS = \
		create.o		\
		debug.o	\
		exit.o	\
		obj_tab.o		\
		process.o	\
		router.o	\
		senile.o	\
		update.o		\
		warpinit.o	\
		warpnet.o	


SIMOBJS	      = \
		$(SIMDIR)/simconfig.o\
		$(SIMDIR)/stdine.o\
		$(SIMDIR)/twsd.o\
		$(SIMDIR)/twsp1.o\
		$(SIMDIR)/twsp2.o\
		$(SIMDIR)/long.o\
		$(SIMDIR)/twerror.o\
		$(SIMDIR)/twhelp.o\
		$(SIMDIR)/float.o\
		$(SIMDIR)/stdout.o\
		$(SIMDIR)/itimer.o	\
		$(SIMDIR)/SUNclear.o\
		$(SIMDIR)/copy.o

SIMLIB   = $(SIMDIR)/twsim.a
TWLIB	=  $(TWDIR)/tw.a

all:	warpnet warpnettw


warpnet:	$(PARALLEL)	$(OBJS)  
		cc $(CFLAGS) $(IFLAGS)  -o warpnet	\
		$(OBJS)					\
		$(SIMLIB)			\
		-lm

	


warpnettw:	$(OBJS)  
		cc $(CFLAGS) $(LIB_FLAGS)  $(IFLAGS)  -o warpnettw	\
		$(OBJS)					\
		$(TWLIB)		\
		-lm

	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c
