/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

/* adjust.c
 * Routines for repositioning nodes after initial layout in
 * order to reduce/remove node overlaps.
 */

#include "neato.h"
#include "voronoi.h"
#include "info.h"
#include "edges.h"
#include "site.h"
#include "info.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static float    margin = 0.05f;     /* Create initial bounding box by adding
                                    * margin * dimension around box enclosing
                                    * nodes.
                                    */
static float    incr = 0.05f;       /* Increase bounding box by adding
                                    * incr * dimension around box.
                                    */
static float    pmargin = 5.0f/72;  /* Margin around polygons, in inches */
static int      iterations = -1;  /* Number of iterations */
static int      useIter    = 0;   /* Use specified number of iterations */

static int      doAll      = 0;  /* Move all nodes, regardless of overlap */
static Site**   sites;           /* Array of pointers to sites; used in qsort */
static Site**   endSite;         /* Sentinel on sites array */
static Point nw, ne, sw, se;     /* Corners of clipping window */

static Site** nextSite;

static void
setBoundBox (Point* ll, Point* ur)
{
    pxmin = ll->x;
    pxmax = ur->x;
    pymin = ll->y;
    pymax = ur->y;
    nw.x = sw.x = pxmin;
    ne.x = se.x = pxmax;
    nw.y = ne.y = pymax;
    sw.y = se.y = pymin;
}

 /* freeNodes:
  * Free node resources.
  */
static void
freeNodes ()
{
    int          i;
    Info_t*      ip = nodeInfo;

    for (i=0; i < nsites; i++) {
      breakPoly (&ip->poly);
      ip++;
    }
    polyFree ();
    infoinit ();       /* Free vertices */
    free (nodeInfo);
}

/* chkBoundBox:
 *   Compute extremes of graph, then set up bounding box.
 *   If user supplied a bounding box, use that;
 *   else if "window" is a graph attribute, use that; 
 *   otherwise, define bounding box as a percentage expansion of
 *   graph extremes.
 *   In the first two cases, check that graph fits in bounding box.
 */
static void
chkBoundBox (Agraph_t* graph)
{
    char*        marg;
    Point        ll, ur;
    int          i;
    float        x, y;
    float        xmin, xmax, ymin, ymax;
    float        xmn, xmx, ymn, ymx;
    float        ydelta, xdelta;
    Info_t*      ip;
    Poly*        pp;
    /* int          cnt; */

    ip = nodeInfo;
    pp = &ip->poly;
    x = ip->site.coord.x;
    y = ip->site.coord.y;
    xmin = pp->origin.x + x;
    ymin = pp->origin.y + y;
    xmax = pp->corner.x + x;
    ymax = pp->corner.y + y;
    for(i = 1; i < nsites; i++) {
        ip++;
        pp = &ip->poly;
        x = ip->site.coord.x;
        y = ip->site.coord.y;
        xmn = pp->origin.x + x;
        ymn = pp->origin.y + y;
        xmx = pp->corner.x + x;
        ymx = pp->corner.y + y;
        if(xmn < xmin) xmin = xmn;
        if(ymn < ymin) ymin = ymn;
        if(xmx > xmax) xmax = xmx;
        if(ymx > ymax) ymax = ymx;
    }

    marg = agget (graph, "voro_margin");
    if (marg && (*marg != '\0')) {
      margin = atof (marg);
    }
    ydelta = margin * (ymax - ymin);
    xdelta = margin * (xmax - xmin);
    ll.x = xmin - xdelta;
    ll.y = ymin - ydelta;
    ur.x = xmax + xdelta;
    ur.y = ymax + ydelta;

    setBoundBox (&ll, &ur);
}

 /* makeInfo:
  * For each node in the graph, create a Info data structure 
  */
static void
makeInfo(Agraph_t* graph)
{
    Agnode_t*    node;
    int          i;
    Info_t*      ip;
    char*        marg;

    nsites = agnnodes (graph);
    geominit ();

    nodeInfo = (Info_t *) malloc(nsites * (sizeof (Info_t)));

    node = agfstnode (graph);
    ip = nodeInfo;

    marg = agget (graph, "voro_pmargin");
    if (marg && (*marg != '\0')) {
      pmargin = atof (marg);
    }
    for (i = 0; i < nsites; i++) {
        ip->site.coord.x = node->u.pos[0];
        ip->site.coord.y = node->u.pos[1];

        makePoly (&ip->poly, node, pmargin);

        ip->site.sitenbr = i;
        ip->site.refcnt = 1;
        ip->node = node;
        ip->verts = NULL;
        node = agnxtnode (graph, node);
        ip++;
    }
}

/* sort sites on y, then x, coord */
static int 
scomp(const void *S1, const void *S2)
{
    Site *s1,*s2;

    s1 = *(Site**)S1;
    s2 = *(Site**)S2;
    if(s1 -> coord.y < s2 -> coord.y) return(-1);
    if(s1 -> coord.y > s2 -> coord.y) return(1);
    if(s1 -> coord.x < s2 -> coord.x) return(-1);
    if(s1 -> coord.x > s2 -> coord.x) return(1);
    return(0);
}

 /* sortSites:
  * Fill array of pointer to sites and sort the sites using scomp
  */
static void
sortSites ()
{
    int          i;
    Site**       sp;
    Info_t*      ip;

    if (sites == 0) {
        sites = (Site**)malloc(nsites * sizeof(Site*));
        endSite = sites + nsites;
    }

    sp = sites;
    ip = nodeInfo;
    infoinit ();
    for (i=0; i < nsites; i++) {
        *sp++ = &(ip->site);
        ip->verts = NULL;
        ip->site.refcnt = 1;
        ip++;
    }

    qsort(sites, nsites, sizeof (Site *), scomp);

    /* Reset site index for nextOne */
    nextSite = sites;

}

static void
geomUpdate ()
{
    int      i;

    sortSites ();

    /* compute ranges */
    xmin=sites[0]->coord.x; 
    xmax=sites[0]->coord.x;
    for(i = 1; i < nsites; i++) {
        if(sites[i]->coord.x < xmin) xmin = sites[i]->coord.x;
        if(sites[i]->coord.x > xmax) xmax = sites[i]->coord.x;
    }
    ymin = sites[0]->coord.y;
    ymax = sites[nsites-1]->coord.y;

    deltay = ymax - ymin;
    deltax = xmax - xmin;
}

static 
Site *nextOne()
{
    Site*   s;

    if(nextSite < endSite) {
        s = *nextSite++;
        return (s);
    }
    else
        return((Site *)NULL);
}

static int
countOverlap (int iter)
{
    int          count = 0;
    int          i, j;
    Info_t*      ip = nodeInfo;
    Info_t*      jp;

    for (i = 0; i < nsites; i++)
      nodeInfo[i].overlaps = 0;

    for (i = 0; i < nsites-1; i++) {
      jp = ip+1;
      for (j = i+1; j < nsites; j++) {
        if (polyOverlap (ip->site.coord, &ip->poly, jp->site.coord, &jp->poly)){
          count++;
          ip->overlaps = 1;
          jp->overlaps = 1;
        }
        jp++;
      }
      ip++;
    }

    if (Verbose > 1)
      fprintf (stderr, "overlap [%d] : %d\n", iter, count);
    return count;
}

static void
increaseBoundBox ()
{
    float        ydelta, xdelta;
    Point        ll, ur;
    
    ur.x = pxmax;
    ur.y = pymax;
    ll.x = pxmin;
    ll.y = pymin;
    
    ydelta = incr * (ur.y - ll.y);
    xdelta = incr * (ur.x - ll.x);

    ur.x += xdelta;
    ur.y += ydelta;
    ll.x -= xdelta;
    ll.y -= ydelta;

    setBoundBox (&ll, &ur);
}

 /* areaOf:
  * Area of triangle whose vertices are a,b,c
  */
static float
areaOf (Point a,Point b,Point c)
{
    float area;

    area = (float)(fabs(a.x*(b.y-c.y) + b.x*(c.y-a.y) + c.x*(a.y-b.y))/2);
    return area;
}

 /* centroidOf:
  * Compute centroid of triangle with vertices a, b, c.
  * Return coordinates in x and y.
  */
static void
centroidOf (Point a,Point b,Point c, float *x, float *y)
{
    *x = (a.x + b.x + c.x)/3;
    *y = (a.y + b.y + c.y)/3;
}

 /* newpos;
  * The new position is the centroid of the
  * voronoi polygon. This is the weighted sum of the
  * centroids of a triangulation, normalized to the
  * total area.
  */
static void
newpos (Info_t* ip)
{
    PtItem*  anchor = ip->verts;
    PtItem   *p, *q;
    float    totalArea = 0.0;
    float    cx = 0.0;
    float    cy = 0.0;
    float    x;
    float    y;
    float    area;

    p = anchor->next;
    q = p->next;
    while(q != NULL) {
      area = areaOf (anchor->p, p->p, q->p);
      centroidOf (anchor->p, p->p, q->p, &x, &y);
      cx = cx + area*x;
      cy = cy + area*y;
      totalArea = totalArea + area;
      p = q;
      q = q->next;
    }

    ip->site.coord.x = cx/totalArea;
    ip->site.coord.y = cy/totalArea;
}

 /* addCorners:
  * Add corners of clipping window to appropriate sites.
  * A site gets a corner if it is the closest site to that corner.
  */
static void
addCorners ()
{
    Info_t    *ip = nodeInfo;
    Info_t    *sws = ip;
    Info_t    *nws = ip;
    Info_t    *ses = ip;
    Info_t    *nes = ip;
    float   swd = dist_2(&ip->site.coord, &sw);
    float   nwd = dist_2(&ip->site.coord, &nw);
    float   sed = dist_2(&ip->site.coord, &se);
    float   ned = dist_2(&ip->site.coord, &ne);
    float   d;
    int     i;
    
    ip++;
    for (i = 1; i < nsites; i++) {
        d = dist_2(&ip->site.coord, &sw);
        if (d < swd) {
            swd = d;
            sws = ip;
        }
        d = dist_2(&ip->site.coord, &se);
        if (d < sed) {
            sed = d;
            ses = ip;
        }
        d = dist_2(&ip->site.coord, &nw);
        if (d < nwd) {
            nwd = d;
            nws = ip;
        }
        d = dist_2(&ip->site.coord, &ne);
        if (d < ned) {
            ned = d;
            nes = ip;
        }
        ip++;
    }

    addVertex (&sws->site, sw.x, sw.y);
    addVertex (&ses->site, se.x, se.y);
    addVertex (&nws->site, nw.x, nw.y);
    addVertex (&nes->site, ne.x, ne.y);
}

 /* newPos:
  * Calculate the new position of a site as the centroid
  * of its voronoi polygon, if it overlaps other nodes.
  * The polygons are finite by being clipped to the clipping
  * window.
  * We first add the corner of the clipping windows to the
  * vertex lists of the appropriate sites.
  */
static void
newPos ()
{
    int      i;
    Info_t*    ip = nodeInfo;

    addCorners ();
    for (i = 0; i < nsites; i++) {
        if (doAll || ip->overlaps) newpos (ip);
        ip++;
    }
}

static void
vAdjust ()
{
    int                iterCnt = 0;
    int                overlapCnt = 0;
    int                badLevel = 0;
    int                increaseCnt = 0;
    int                cnt;

    if (!useIter || (iterations > 0))
      overlapCnt = countOverlap (iterCnt);
    
    if ((overlapCnt == 0) || (iterations == 0))
      return;

    geomUpdate ();
    voronoi(0, nextOne); 
    while (1) {
      newPos ();
      iterCnt++;
      
      if (useIter && (iterCnt == iterations)) break;
      cnt = countOverlap (iterCnt);
      if (cnt == 0) break;
      if (cnt >= overlapCnt) badLevel++;
      else badLevel = 0;
      overlapCnt = cnt;

      switch (badLevel) {
      case 0:
        doAll = 1;
        break;
/*
      case 1:
        doAll = 1;
        break;
*/
      default :
        doAll = 1;
        increaseCnt++;
        increaseBoundBox ();
        break;
      }

      geomUpdate ();
      voronoi(0, nextOne); 
    }

    if (Verbose) {
      fprintf (stderr, "Number of iterations = %d\n", iterCnt);
      fprintf (stderr, "Number of increases = %d\n", increaseCnt);
    }

}

 /* updateGraph:
  * Enter new node positions into the graph
  */
static void
updateGraph (Agraph_t* graph)
{
    /* Agnode_t*    node; */
    int          i;
    Info_t*        ip;
    /* char         pos[100]; */

    ip = nodeInfo;
    for (i = 0; i < nsites; i++) {
        ip->node->u.pos[0] = ip->site.coord.x;
        ip->node->u.pos[1] = ip->site.coord.y;
        ip++;
    }
}

static void normalize(graph_t *g)
{
	node_t	*v;
	edge_t	*e;

	double	theta;
	pointf	p;

	if (!mapbool(agget(g,"normalize"))) return;

	v = agfstnode(g); p.x = v->u.pos[0]; p.y = v->u.pos[1];
	for (v = agfstnode(g); v; v = agnxtnode(g,v))
		{v->u.pos[0] -= p.x; v->u.pos[1] -= p.y;}

	e = NULL;
	for (v = agfstnode(g); v; v = agnxtnode(g,v))
		if ((e = agfstout(g,v))) break;
	if (e == NULL) return;

	theta = -atan2(e->head->u.pos[1] - e->tail->u.pos[1],
		e->head->u.pos[0] - e->tail->u.pos[0]);

	for (v = agfstnode(g); v; v = agnxtnode(g,v)) {
		p.x = v->u.pos[0]; p.y = v->u.pos[1];
		v->u.pos[0] = p.x * cos(theta) - p.y * sin(theta);
		v->u.pos[1] = p.x * sin(theta) + p.y * cos(theta);
	}
}

void adjustNodes (graph_t* G)
{
    /* int          userWindow = 0; */
    char*        flag;

    normalize(G);
    flag = agget(G,"overlap");
    if ((flag == NULL) || mapbool(flag)) return;
    
          /* create main array */
    makeInfo(G);

      /* establish and verify bounding box */
    chkBoundBox (G);

    vAdjust ();

    updateGraph (G);

    freeNodes ();
    free (sites);

}
