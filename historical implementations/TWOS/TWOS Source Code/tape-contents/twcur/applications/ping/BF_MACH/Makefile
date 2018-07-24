
SIMDIR	   = ../../../twsim/BF_MACH
TWDIR	= ../../../tw/BF_MACH
INCDIR	= ../../../tw






IFLAGS	      =  -DBF_MACH -DFAST_VTIME_MACROS\
		 -I.. -I$(INCDIR)


CFLAGS	      =   -O  -X130
# -X129 -X140

VPATH = ..

LIB_FLAGS     =

 


OBJS = \
		pp.o		\
		pongtab.o

SIMLIB   = $(SIMDIR)/twsim.a
TWLIB	=  $(TWDIR)/tw.a

all:	ping	pingtw


ping:		$(PARALLEL)	$(OBJS)  
		cc $(CFLAGS) $(IFLAGS)  -o ping	\
		$(OBJS)				\
		$(SIMLIB)			\
		-lm

	


pingtw:		$(OBJS)  
		cc $(CFLAGS) $(LIB_FLAGS)  $(IFLAGS)  -o pingtw	\
		$(OBJS)					\
		$(TWLIB)		\
		-lm

	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c
