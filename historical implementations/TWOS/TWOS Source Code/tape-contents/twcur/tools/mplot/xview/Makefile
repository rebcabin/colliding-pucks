#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U.S. Government Sponsorship under NASA Contract NAS7-918
#		is acknowledged.
	
CC = cc

CFLAGS = -f68881 -O -DXVIEW -DX11 -L/u1f/xview/xview2/lib/libxview \
		 -L/u1f/xview/xview2/lib/libolgx
IFLAGS = -I.. -I/u1f/xview/xview2/usr_latest/usr/include

OBJS = ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o


mplot: $(OBJS)
	rm -f mplot
	$(CC) $(CFLAGS) $(OBJS) -o mplot -lxview -lolgx -lX11 -lm


.c.o:
	$(CC) $(CFLAGS)  -c $<

%.o: ../%.c
	cc $(CFLAGS) $(IFLAGS) -c ../$*.c


ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o: ../mplot.h


