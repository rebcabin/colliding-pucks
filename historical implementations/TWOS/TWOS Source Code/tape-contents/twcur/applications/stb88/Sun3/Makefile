
SIMDIR	   = ../../../twsim/Sun3

TWH	= ../../../tw
TWDIR	= ../../../tw/Sun3






IFLAGS	      =  -I.. -I$(TWH)
		 


CFLAGS	      =   -f68881 -O -DSUN -DFAST_VTIME_MACROS -DOLDTW -Bstatic
 


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


stb88:		$(OBJS)  
		cc $(CFLAGS) $(IFLAGS)  -o stb88	\
		$(OBJS)					\
		$(SIMLIB)				\
		-lm

	


stb88tw:	$(OBJS)  
		cc $(CFLAGS) $(IFLAGS)  -o stb88tw	\
		$(OBJS)					\
		$(TWLIB)				\
		-lm
	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c

%.o: ../%.c
	cc $(CFLAGS) $(IFLAGS) -c ../$*.c
