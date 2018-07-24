#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U.S. Government Sponsorship under NASA Contract NAS7-918
#		is acknowledged.
#

CC = cc

CFLAGS = -f68881 -O -Bstatic

MPOBJS = ident.o indata.o magnify.o main.o menu.o text.o zoom.o

HCOBJS = hc.o indata.o

FPOBJS = fplot.o magnify.o

tw:	tw.o
	$(CC) $(CFLAGS) tw.o -o tw -lsuntool -lsunwindow -lpixrect -lm

fplot:	$(FPOBJS)
	$(CC) $(CFLAGS) $(FPOBJS) -o fplot -lsuntool -lsunwindow -lpixrect

mplot: $(MPOBJS)
	$(CC) $(CFLAGS) $(MPOBJS) -o mplot -lsuntool -lsunwindow -lpixrect -lm

.c.o:
	$(CC) $(CFLAGS)  -c $<

ident.o indata.o main.o menu.o text.o zoom.o: mplot.h


hc: $(HCOBJS)
	$(CC)  -o hc $(HCOBJS) $(CFLAGS) -lsuntool -lsunwindow -lpixrect -lm

hc.o: mplot.h
