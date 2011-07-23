/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#define NOT(v)		(!(v))
#ifndef FALSE
#define	FALSE		0
#endif
#ifndef TRUE
#define TRUE		NOT(FALSE)
#endif

#ifdef DMALLOC
#define NEW(t)		 (t*)calloc(1,sizeof(t))
#define N_NEW(n,t)	 (t*)calloc((n),sizeof(t))
#define ALLOC(size,ptr,type) (ptr? (type*)realloc(ptr,(size)*sizeof(type)):(type*)malloc((size)*sizeof(type)))
#define ZALLOC(size,ptr,type,osize) (ptr? (type*)recalloc(ptr,(size)*sizeof(type)):(type*)calloc((size),sizeof(type)))
#else
#define NEW(t)		 (t*)zmalloc(sizeof(t))
#define N_NEW(n,t)	 (t*)zmalloc((n)*sizeof(t))
#define ALLOC(size,ptr,type) (ptr? (type*)realloc(ptr,(size)*sizeof(type)):(type*)malloc((size)*sizeof(type)))
#define ZALLOC(size,ptr,type,osize) (ptr? (type*)zrealloc(ptr,size,sizeof(type),osize):(type*)calloc((size),sizeof(type)))
#endif

#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))
#define ABS(a)		((a) >= 0 ? (a) : -(a))
#ifndef MAXINT
#define	MAXINT		((int)(~(unsigned)0 >> 1))
#endif
#ifndef MAXSHORT
#define	MAXSHORT	(0x7fff)
#endif
#define BETWEEN(a,b,c)	(((a) <= (b)) && ((b) <= (c)))
#define ROUND(f)		(round(f))
#define RADIANS(deg)	((deg)/180.0 * PI)
#define DEGREES(rad)	((rad)/PI * 180.0)
#define DIST(x1,y1,x2,y2) (sqrt(((x1) - (x2))*((x1) - (x2)) + ((y1) - (y2))*((y1) - (y2))))
#define POINTS_PER_INCH	72.0
#define POINTS(f_inch)	(ROUND(f_inch*POINTS_PER_INCH))
#define PS2INCH(ps)		((ps)/POINTS_PER_INCH)

#define is_virtual(n)	((n)->u.node_type == VIRTUAL)
#ifndef streq
#define streq(s,t)		(!strcmp((s),(t)))
#endif
