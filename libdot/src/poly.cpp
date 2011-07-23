/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
/* poly.c
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "neato.h"
#include "poly.h"
#include "mem.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int maxcnt = 0;
static Point* tp1 = NULL;
static Point* tp2 = NULL;
static Point* tp3 = NULL;

void polyFree ()
{
    maxcnt = 0;
    free (tp1);
    free (tp2);
    free (tp3);
    tp1 = NULL;
    tp2 = NULL;
    tp3 = NULL;
}

void breakPoly (Poly* pp)
{
    free (pp->verts);
    free (pp->overts);
}

static void
bbox (Point* verts, int cnt, Point* o, Point* c)
{
    float  xmin, ymin, xmax, ymax;
    int    i;

    xmin = xmax = verts->x;
    ymin = ymax = verts->y;
    for (i = 1; i < cnt; i++) {
      verts++;
      if (verts->x < xmin) xmin = verts->x;
      if (verts->y < ymin) ymin = verts->y;
      if (verts->x > xmax) xmax = verts->x;
      if (verts->y > ymax) ymax = verts->y;
    }
    o->x = xmin;
    o->y = ymin;
    c->x = xmax;
    c->y = ymax;
}

static void
inflate (Point* prev, Point* cur, Point* next, float margin)
{
    double theta = atan2 (prev->y - cur->y, prev->x - cur->x);
    double phi = atan2 (next->y - cur->y, next->x - cur->x);
    double beta = (theta + phi)/2.0;
    double gamma = (PI + phi - theta)/2.0;
    double denom;

    denom =  cos (gamma);
    cur->x -= margin*(cos (beta))/denom;
    cur->y -= margin*(sin (beta))/denom;
}

static void
inflatePts (Point* verts, int cnt, float margin)
{
    int    i;
    Point  first;
    Point  savepoint;
    Point  prevpoint;
    Point* prev = &prevpoint;
    Point* cur;
    Point* next;

    first = verts[0];
    prevpoint = verts[cnt-1];
    cur = &verts[0];
    next = &verts[1];
    for (i = 0; i < cnt-1; i++) {
      savepoint = *cur;
      inflate (prev,cur,next,margin);
      cur++;
      next++;
      prevpoint = savepoint;
    }

    next = &first;
    inflate (prev,cur,next,margin);
}

static int
isBox (Point* verts, int cnt)
{
    if (cnt != 4) return 0;

    if (verts[0].y == verts[1].y)
      return ((verts[2].y == verts[3].y) &&
              (verts[0].x == verts[3].x) &&
              (verts[1].x == verts[2].x));
    else 
      return ((verts[0].x == verts[1].x) &&
              (verts[2].x == verts[3].x) &&
              (verts[0].y == verts[3].y) &&
              (verts[1].y == verts[2].y));
}

#define DFLT_SAMPLE 20

static Point
makeScaledPoint(int x, int y)
{
	Point	rv;
	rv.x = PS2INCH(x);
	rv.y = PS2INCH(y);
	return rv;
}

void
makePoly (Poly* pp, Agnode_t* n, float margin)
{
    extern void poly_init(node_t *);
    extern void record_init(node_t *);
    int    i;
    int    sides;
    Point* verts;
    Point* overts;
    polygon_t *poly;

    if (n->u.shape->initfn == poly_init) {
      poly = (polygon_t*) n->u.shape_info;
      sides = poly->sides;
      if (sides >= 3) {        /* real polygon */
        verts = (Point*)myalloc (sides * sizeof(Point));
		for (i = 0; i < sides; i++) {
          verts[i].x = poly->vertices[i].x;
          verts[i].y = poly->vertices[i].y;
        }
      }
      else {
		char *p = agget(n,"samplepoints");
		if (p) sides = atoi(p);
		else sides = DFLT_SAMPLE;
		if (sides < 3) sides = DFLT_SAMPLE;
        verts = (Point*)myalloc (sides * sizeof(Point));
		for (i = 0; i < sides; i++) {
          verts[i].x = n->u.width/2.0  * cos(i/(double)sides * PI * 2.0);
          verts[i].y = n->u.height/2.0 * sin(i/(double)sides * PI * 2.0);
        }
      }

      if (streq (n->u.shape->name, "box"))
        pp->isBox = 1;
      else if (streq (n->u.shape->name, "polygon"))
        pp->isBox = isBox (verts, sides);
      else
        pp->isBox = 0;

    }
    else if (n->u.shape->initfn == record_init) {
	  box b;
      sides = 4;
      verts = (Point*)myalloc (sides * sizeof(Point));
	  b = ((field_t*) n->u.shape_info)->b;
	  verts[0] = makeScaledPoint(b.LL.x,b.LL.y);
	  verts[1] = makeScaledPoint(b.UR.x,b.LL.y);
	  verts[2] = makeScaledPoint(b.UR.x,b.UR.y);
	  verts[3] = makeScaledPoint(b.LL.x,b.UR.y);
      pp->isBox = 1;
    }
    else {
      fprintf (stderr, "makePoly: unknown shape type %s\n", n->u.shape->name);
      exit(1);
    }

    overts = (Point*)myalloc (sides * sizeof(Point));
    for (i = 0; i < sides; i++)
      overts[i] = verts[i];

    if (margin != 0.0)
      inflatePts (verts,sides,margin);

    pp->overts = overts;
    pp->verts = verts;
    pp->nverts = sides;
    bbox (verts, sides, &pp->origin, &pp->corner);

    if (sides > maxcnt)
      maxcnt = sides;
}

static int
pintersect (Point originp, Point cornerp, Point originq, Point cornerq)
{
    return ((originp.x <= cornerq.x) && (originq.x <= cornerp.x) &&
            (originp.y <= cornerq.y) && (originq.y <= cornerp.y));
}

#define Pin 1
#define Qin 2
#define Unknown 0

#define advance(A,B,N) (B++, A = (A+1)%N)

static int
edgesIntersect (Point* P, Point* Q, int n, int m)
{
    int      a = 0;
    int      b = 0;
    int      aa = 0;
    int      ba = 0;
    int      a1, b1;
    Point    A, B;
    float    cross;
    int      bHA, aHB;
    Point    p;
    int      inflag = Unknown;
    /* int      i = 0; */
    /* int      Reset = 0; */

    do {
      a1 = (a + n - 1) % n;
      b1 = (b + m - 1) % m;

      subPt(&A, P[a], P[a1]);
      subPt(&B, Q[b], Q[b1]);

      cross = area_2(origin, A, B );
      bHA = leftOf( P[a1], P[a], Q[b] );
      aHB = leftOf( Q[b1], Q[b], P[a] );

        /* If A & B intersect, update inflag. */
      if (intersection( P[a1], P[a], Q[b1], Q[b], &p ) )
        return 1;

                /* Advance rules. */
      if ( (cross == 0) && !bHA && !aHB ) {
        if ( inflag == Pin )
          advance( b, ba, m);
        else
          advance( a, aa, n);
      }
      else if ( cross >= 0 )
        if ( bHA )
          advance( a, aa, n);
        else {
          advance( b, ba, m);
      }
      else /* if ( cross < 0 ) */{
        if ( aHB )
          advance( b, ba, m);
        else
          advance( a, aa, n);
      }

    } while ( ((aa < n) || (ba < m)) && (aa < 2*n) && (ba < 2*m) );

    return 0;

}

static int	
inPoly(Point vertex[], int n, Point q)
{
	int	i, i1;		/* point index; i1 = i-1 mod n */
	double	x;		/* x intersection of e with ray */
	double	crossings = 0;	/* number of edge/ray crossings */

    if (tp3 == NULL)
      tp3 = (Point*)myalloc (maxcnt * sizeof(Point));

	/* Shift so that q is the origin. */
	for (i = 0; i < n; i++) {
		tp3[i].x = vertex[i].x - q.x;
		tp3[i].y = vertex[i].y - q.y;
	}

	/* For each edge e=(i-1,i), see if crosses ray. */
	for( i = 0; i < n; i++ ) {
		i1 = ( i + n - 1 ) % n;

		/* if edge is horizontal, test to see if the point is on it */
		if(  ( tp3[i].y == 0 ) && ( tp3[i1].y == 0 ) ) 
			if ((tp3[i].x*tp3[i1].x) < 0)
				return 1 ;
			else
				continue;

		/* if e straddles the x-axis... */
		if( ( ( tp3[i].y >= 0 ) && ( tp3[i1].y <= 0 ) ) ||
		    ( ( tp3[i1].y >= 0 ) && ( tp3[i].y <= 0 ) ) ) {
			/* e straddles ray, so compute intersection with ray. */
			x = (tp3[i].x * tp3[i1].y - tp3[i1].x * tp3[i].y)
				/ (double)(tp3[i1].y - tp3[i].y);

			/* if intersect at origin, we've found intersection */
			if (x == 0)
				return 1;;

			/* crosses ray if strictly positive intersection. */
			if (x > 0) 
				if ( (tp3[i].y == 0) || (tp3[i1].y == 0) )
					crossings += .5 ;  /* goes thru vertex*/
				else
					crossings += 1.0;
		}
	}

	/* q inside if an odd number of crossings. */
	if( (((int) crossings) % 2) == 1 )
		return	1;
	else	return	0;
}

static int 
inBox (Point p, Point origin, Point corner)
{
    return ((p.x <= corner.x) &&
            (p.x >= origin.x) &&
            (p.y <= corner.y) &&
            (p.y >= origin.y));

}

static void
transCopy (Point* inp, int cnt, Point off, Point* outp)
{
    int    i;

    for (i = 0; i < cnt; i++) {
      outp->x = inp->x + off.x;
      outp->y = inp->y + off.y;
      inp++;
      outp++;
    }
}

int 
polyOverlap (Point p, Poly* pp, Point q, Poly* qp)
{
    Point op, cp;
    Point oq, cq;

      /* translate bounding boxes */
    addPt (&op, p,pp->origin);
    addPt (&cp, p,pp->corner);
    addPt (&oq, q,qp->origin);
    addPt (&cq, q,qp->corner);

      /* If bounding boxes don't overlap, done */
    if (!pintersect(op,cp,oq,cq))
      return 0;
    /* else */
      /* return 1; */
    
    if (pp->isBox && qp->isBox) return 1;

    if (tp1 == NULL) {
      tp1 = (Point*)myalloc (maxcnt * sizeof(Point));
      tp2 = (Point*)myalloc (maxcnt * sizeof(Point));
    }

    transCopy (pp->verts, pp->nverts, p, tp1);
    transCopy (qp->verts, qp->nverts, q, tp2);
    return (edgesIntersect (tp1, tp2, pp->nverts, qp->nverts) ||
            (inBox(*tp1, oq, cq) && inPoly(tp2, qp->nverts, *tp1)) ||
            (inBox(*tp2, op, cp) && inPoly(tp1, pp->nverts, *tp2)) );
}
