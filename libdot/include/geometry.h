/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/


#ifndef GEOMETRY_H
#define GEOMETRY_H

typedef struct Point {
    float x,y;
} Point;

extern Point origin;

extern float xmin, xmax, ymin, ymax; /* extreme x,y values of sites */
extern float deltax, deltay;         /* xmax - xmin, ymax - ymin */

extern int     nsites;               /* Number of sites */
extern int     sqrt_nsites;

extern void geominit();
extern float dist_2(Point *, Point*); /* Distance squared between two points */
extern void subPt (Point* a, Point b, Point c);
extern void addPt (Point* a, Point b, Point c);
extern float area_2(Point a, Point b, Point c );
extern int leftOf(Point a, Point b, Point c);
extern int intersection( Point a, Point b, Point c, Point d, Point* p );

#endif

