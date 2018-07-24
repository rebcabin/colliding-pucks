
SIMDIR	   = ../../twsim
TWDIR	= ../../tw
INCDIR	= ../../tw






IFLAGS	      =  -DBF_MACH -DFAST_VTIME_MACROS \
		 -I.. -I$(INCDIR)


CFLAGS	      =   -O  -X130 -DOLDTW
# -X129 -X140


LIB_FLAGS     =

 


OBJS = \
		array.o		\
		ctlslib.o	\
		distributor.o	\
		div.o		\
		grid.o		\
		initializer.o	\
		motion.o	\
		random.o	\
		stat.o		\
		stb88_1.o	\
		stb_debug.o	\
		transform.o	\
		xcorps.o	


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

all:	stb88	stb88tw


stb88:		$(PARALLEL)	$(OBJS)  
		cc $(CFLAGS) $(IFLAGS)  -o stb88	\
		$(OBJS)					\
		$(SIMLIB)			\
		-lm

	


stb88tw:	$(OBJS)  
		cc $(CFLAGS) $(LIB_FLAGS)  $(IFLAGS)  -o stb88tw	\
		$(OBJS)					\
		$(TWLIB)		\
		-lm

	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c
