#define MAXM(Y,Z,W)	{int X;Y=W[0];for(X=0;X<Z;X++) if(Y<W[X]) Y=W[X];}
#define	EMAXM(Y,Z,W)	{int X;for(X=0;X<Z;X++) if(Y<W[X]) Y=W[X];}
	/*m must be set before the use of EMAXM*/
#define MINM(Y,Z,W)	{int X;Y=W[0];for(X=0;X<Z;X++) if(Y>W[X]) Y=W[X];}
#define EMINM(Y,Z,W)	{int X;for(X=0;X<Z;X++) if(Y>W[X]) Y=W[X];}
	/*m must be set before the use of EMINM*/
#define MEAN(Y,Z,W)	{int X;Y=0;for(X=0;X<Z;M+=W[X++]);Y/=Z;}

#define	FPOW(X,Y)	exp(Y*log(X))
#define	RPOW(X,Y)	exp(Y*log(X))
#define LAMBDAEXP	2.3025850929940456801799
#define NATURAL		2.718281828459045235360
/*To Express E, Remember To Memorize A Sentance To Simplify This*/
#define PI		3.14159265358979324
#define EXP10(X)	DEXP(X)
#define LOG10(X)	DLOG(X)

#define MAG(L) (-2.5*LOG10((L)))

#define STIRL(X) (.91893853320467274202+((X)+.5)*log((X)+1.)-(X)-1.+log(1.+1./(12.*((X)+1.))))
/*returns log nfactorial*/

#define FRAND ((double)rand()/32768.0)
/*Between 0 and 1*/
#define FRAND2 (((double)((unsigned)(rand2())))/65535.)
#define DRAND2 FRAND2
#define DEXP(X) 	exp(LAMBDAEXP*(X))
#define DLOG(X) 	(log((X))/LAMBDAEXP)

#define BIGFLOAT	(1.0e33)
#define SMALLFLOAT	(1.0e-33)

#ifndef NULL
#define NULL 0
#endif

#define DANGER(X,Y) {if(fabs((X)-(Y))/fabs((X)+(Y))<5.e-11)write(2,"Fp subtract uflow.\n",19);}

extern double fabs(), floor(), ceil(), fmod(), ldexp(), frexp();
extern double sqrt(), hypot(), atof();
extern double sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern double exp(), log(), log10(), pow();
extern double sinh(), cosh(), tanh();
extern double gamma();
extern int	atoi();
extern long   atol();

#define HUGE	1.701411733192644270e38

#ifndef TRUE
#define TRUE   1
#endif
#ifndef FALSE
#define FALSE  0
#endif


#define isWHITE(X)	((X)==' '||(X)=='\n'||(X)=='\t') 
#define isNUM(X)	((X)>='0'&&(X)<='9')

/*---For lseek*/
#define FROMHERE	1
#define FROMTOF 	0
#define FROMEOF 	2

#define STREQ(x,y)	(!strcmp(x,y))

#define HUP		1 /*used in signal calls*/
#define INT   		2 
#define QUIT		3
#define	ILL		4
#define TRAP		5
#define IOT        	6
#define	EMT		7
#define FPE		8
#define	KILL  		9
#define BUS		10
#define SEGV		11
#define SYS		12
#define	ALRM		14
#define	TERM		15
#define UNASSIGNED	16

#define IGNORE		1
#define TERMINATE	0

typedef char *va_list;
# define va_dcl int va_alist;
# define va_start(list) list = (char *) &va_alist
# define va_end(list)
# define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
