/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include "geometry.h"
#include <math.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

Point origin = {0,0};

float xmin, xmax, ymin, ymax;    /* min and max x and y values of sites */
float deltax,                    /* xmax - xmin */
      deltay;                    /* ymax - ymin */

int             nsites;
int             sqrt_nsites;

void
geominit()
{
    double sn;

    sn = nsites+4;
    sqrt_nsites = (int)sqrt(sn);
    /* deltay = ymax - ymin; */
    /* deltax = xmax - xmin; */
}

float
dist_2 ( Point* pp, Point* qp)
{
    float dx    = pp->x - qp->x;
    float dy    = pp->y - qp->y;

    return (dx*dx + dy*dy);
}

void
subPt (Point* a, Point b, Point c)
{
    a->x = b.x - c.x;
    a->y = b.y - c.y;
}

void
addPt (Point* c, Point a, Point b)
{
    c->x = a.x + b.x;
    c->y = a.y + b.y;
}

float 
area_2(Point a, Point b, Point c )
{
        return (a.x * b.y - a.y * b.x +
                a.y * c.x - a.x * c.y +
                b.x * c.y - c.x * b.y);
}

int
leftOf(Point a, Point b, Point c)
{
        return  (area_2( a, b, c ) > 0);
}

int
intersection( Point a, Point b, Point c, Point d, Point* p )
{
    double  s, t;   /* The two parameters of the parametric eqns. */
    double  denom;  /* Denominator of solutions. */

    denom =
      a.x * ( d.y - c.y ) +
      b.x * ( c.y - d.y ) +
      d.x * ( b.y - a.y ) +
      c.x * ( a.y - b.y );

      /* If denom is zero, then the line segments are parallel. */
      /* In this case, return false even though the segments might overlap. */
    if (denom == 0.0) return 0;

    s = ( a.x * ( d.y - c.y ) +
          c.x * ( a.y - d.y ) +
          d.x * ( c.y - a.y )
        ) / denom;
    t = -( a.x * ( c.y - b.y ) +
           b.x * ( a.y - c.y ) +
           c.x * ( b.y - a.y )
         ) / denom;

    p->x = a.x + s * ( b.x - a.x );
    p->y = a.y + s * ( b.y - a.y );

    if ((0.0 <= s) && (s <= 1.0) &&
        (0.0 <= t) && (t <= 1.0))
                return 1;
    else return 0;
}

