/* These macros save typing or increase readability of programs */

#ifndef until
#define until(x) while(!(x))
#endif 

#ifndef FUNCTION
#define FUNCTION
#endif

#define NL printf ("\n")
#define SP printf (" ")
#define DQ printf ("\"")

#define sc(t,x)			scanf ("%t", &x)
#define sc1(t,x)		sc(t,x)
#define sc2(t,x,y)		sc(t,x) , sc1(t,y)
#define sc3(t,x,y,z)		sc(t,x) , sc2(t,y,z)
#define sc4(t,x,y,z,w)		sc(t,x) , sc3(t,y,z,w)
#define sc5(t,x,y,z,w,u)	sc(t,x) , sc4(t,y,z,w,u)
#define sc6(t,x,y,z,w,u,v)	sc(t,x) , sc5(t,y,z,w,u,v)
#define sc7(t,x,y,z,w,u,v,r)	sc(t,x) , sc6(t,y,z,w,u,v,r)

#define scd(x)			sc(d,x)
#define scld(x)			sc(ld,x)
#define scf(x)			sc(f,x)
#define sclf(x)			sc(lf,x)
#define scc(x)			sc(c,x)
#define scs(x)			scanf ("%s", x)
#define scu(x)			sc(u,x)
#define scx(h)			sc(x,h)
#define sclx(h)			sc(lx,h)
#define sco(x)			sc(o,x)

#define fsc(f,t,x)		fscanf (f, "%t", &x)
#define fsc1(f,t,x)		fsc(f,t,x)
#define fsc2(f,t,x,y)		fsc(f,t,x) , fsc1(f,t,y)
#define fsc3(f,t,x,y,z)		fsc(f,t,x) , fsc2(f,t,y,z)
#define fsc4(f,t,x,y,z,w)	fsc(f,t,x) , fsc3(f,t,y,z,w)
#define fsc5(f,t,x,y,z,w,u)	fsc(f,t,x) , fsc4(f,t,y,z,w,u)
#define fsc6(f,t,x,y,z,w,u,v)	fsc(f,t,x) , fsc5(f,t,y,z,w,u,v)
#define fsc7(f,t,x,y,z,w,u,v,r)	fsc(f,t,x) , fsc6(f,t,y,z,w,u,v,r)

#define fscd(f,x)		fsc(f,d,x)
#define fscld(f,x)		fsc(f,ld,x)
#define fscf(F,x)		fsc(F,f,x)
#define fsclf(f,x)		fsc(f,lf,x)
#define fscc(f,x)		fsc(f,c,x)
#define fscs(f,x)		scanf (f, "%s", x)
#define fscu(f,x)		fsc(f,u,x)
#define fscx(f,h)		fsc(f,x,h)
#define fsclx(f,h)		fsc(f,lx,h)
#define fsco(f,x)		fsc(f,o,x)

#define prs(x)                  printf ("%s\n",x)
#define PRS(x)                  printf ("%s ",x)

#define pr(t,x)                 printf ("x = %t ", x)

#define pr1(t,x)                pr(t,x) , NL
#define pr2(t,x,y)              pr(t,x) , pr1(t,y)
#define pr3(t,x,y,z)            pr(t,x) , pr2(t,y,z)
#define pr4(t,x,y,z,w)          pr(t,x) , pr3(t,y,z,w)
#define pr5(t,x,y,z,w,u)        pr(t,x) , pr4(t,y,z,w,u)
#define pr6(t,x,y,z,w,u,v)      pr(t,x) , pr5(t,y,z,w,u,v)
#define pr7(t,x,y,z,w,u,v,r)    pr(t,x) , pr6(t,y,z,w,u,v,r)

#define prm1(x,t)               printf ("x = %t", x), NL
#define prm2(x,t1,t2)           printf ("x = %t1, %t2", x, x), NL
#define prm3(x,t1,t2,t3)        printf ("x = %t1, %t2, %t3", x, x, x), NL

#define prd(x)                  pr1(d,x)
#define prld(x)                 pr1(ld,x)
#define prf(x)                  pr1(f,x)
#define prlf(x)			pr1(lf,x)
#define prc(a)                  pr1(c,a)
#define pru(i)                  pr1(u,i)
#define prx(h)                  printf ("h = 0x%x", h), NL
#define prlx(h)                 printf ("h = 0x%lx", h), NL
#define pro(x)                  printf ("x = 0%o", x), NL

#define PRM1(x,t)               printf ("x = %t; ", x)
#define PRM2(x,t1,t2)           printf ("x = %t1, %t2; ", x, x)
#define PRM3(x,t1,t2,t3)        printf ("x = %t1, %t2, %t3; ", x, x, x)

#define PRD(x)                  pr(d,x)
#define PRLD(x)                 pr(ld,x)
#define PRF(x)                  pr(f,x)
#define PRLF(x)			pr(lf,x)
#define PRC(a)                  pr(c,a)
#define PRI(i)                  pr(d,i)
#define PRU(i)                  pr(u,i)
#define PRX(h)                  printf ("h = 0x%x ", h)
#define PRO(x)                  printf ("x = 0%o ", x)

#define RD(t,b) fread(&b,sizeof(t),1,stdin)
#define readInt(i)    RD(int,i)
#define readLong(l)   RD(long,l)
#define readFloat(f)  RD(float,f)
#define readChar(c)   RD(char,c)
#define readDouble(d) RD(double,d)

#define WR(t,b) fwrite(&b,sizeof(t),1,stdout)
#define writeInt(i)    WR(int,i)
#define writeLong(l)   WR(long,l)
#define writeFloat(f)  WR(float,f)
#define writeChar(c)   WR(char,c)
#define writeDouble(d) WR(double,d)

#define ENL fprintf (stderr, "\n")

#define eprs(x)                 fprintf (stderr,"%s\n",x)
#define epr(t,x)                fprintf (stderr, "x = %t ", x)
#define epr1(t,x)               epr(t,x) , ENL
#define epr2(t,x,y)             epr(t,x) , epr1(t,y)
#define epr3(t,x,y,z)           epr(t,x) , epr2(t,y,z)
#define epr4(t,x,y,z,w)         epr(t,x) , epr3(t,y,z,w)
#define epr5(t,x,y,z,w,u)       epr(t,x) , epr4(t,y,z,w,u)
#define epr6(t,x,y,z,w,u,v)     epr(t,x) , epr5(t,y,z,w,u,v)
#define epr7(t,x,y,z,w,u,v,r)   epr(t,x) , epr6(t,y,z,w,u,v,r)

#define eprd(x)                 epr1(d,x)
#define eprld(x)                epr1(ld,x)
#define eprf(x)                 epr1(f,x)
#define eprlf(x)		epr1(lf,x)
#define eprc(a)                 epr1(c,a)
#define epru(i)                 epr1(u,i)
#define eprx(h)                 fprintf (stderr, "h = 0x%x", h), NL
#define eprlx(h)                fprintf (stderr, "h = 0x%lx", h), NL
#define epro(x)                 fprintf (stderr, "x = 0%o", x), NL

