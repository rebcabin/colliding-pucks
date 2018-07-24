#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U. S. Government Sponsorship under NASA Contract NAS7-918
#		is acknowledged.
#

all:		check collapse

check:
		rm -f check measure
		cc -o check  ../check.c
		ln -s check measure

collapse:
		cc -o collapse ../collapse.c -lm

clean:
		rm -f *.o


