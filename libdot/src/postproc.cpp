/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static boolean	Flip;
static point	Offset;

static void place_flip_graph_label(graph_t* g);

#define M1 \
	"/pathbox { /Y exch %d sub def /X exch %d sub def /y exch %d sub def /x exch %d sub def newpath x y moveto X y lineto X Y lineto x Y lineto closepath stroke } def\n"
#define M2 \
	"/pathbox { /X exch neg %d sub def /Y exch %d sub def /x exch neg %d sub def /y exch %d sub def newpath x y moveto X y lineto X Y lineto x Y lineto closepath stroke } def\n"

point
map_point(point p)
{
	int		x = p.x;
	int 	y = p.y;

	if (Flip) { p.x = -y - Offset.x; p.y = x - Offset.y; }
	else { p.x = x - Offset.x; p.y = y - Offset.y; }
	return p;
}

void
map_edge(edge_t* e)
{
	int			j,k;
	bezier		bz;

if (e->u.spl == NULL) {
	if ((Concentrate == FALSE) || (e->u.edge_type != IGNORED))
		fprintf(stderr,"lost %s %s edge\n",e->tail->name,e->head->name);
	return;
}
	for (j = 0; j < e->u.spl->size; j++) {
		bz = e->u.spl->list[j];
		for (k = 0; k < bz.size; k++)
			bz.list[k] = map_point(bz.list[k]);
		if (bz.sflag)
			e->u.spl->list[j].sp = map_point (e->u.spl->list[j].sp);
		if (bz.eflag)
			e->u.spl->list[j].ep = map_point (e->u.spl->list[j].ep);
	}
	if (e->u.label) e->u.label->p = map_point(e->u.label->p);
    /* vladimir */
	if (e->u.head_label) e->u.head_label->p = map_point(e->u.head_label->p);
	if (e->u.tail_label) e->u.tail_label->p = map_point(e->u.tail_label->p);
}

void translate_bb(graph_t* g, int lr)
{
	int			c;
	box			bb,new_bb;

	bb = g->u.bb;
	if (lr) {
		new_bb.LL = map_point(pointof(bb.LL.x,bb.UR.y));
		new_bb.UR = map_point(pointof(bb.UR.x,bb.LL.y));
	}
	else {
		new_bb.LL = map_point(pointof(bb.LL.x,bb.LL.y));
		new_bb.UR = map_point(pointof(bb.UR.x,bb.UR.y));
	}
	g->u.bb = new_bb;
	if (g->u.label) {
		g->u.label->p = map_point(g->u.label->p);
    }
	for (c = 1; c <= g->u.n_cluster; c++) translate_bb(g->u.clust[c],lr);
}

static void translate_drawing(graph_t* g, nodesizefn_t ns)
{
	node_t		*v;
	edge_t		*e;

	for (v = agfstnode(g); v; v = agnxtnode(g,v)) {
		ns(v,FALSE);
		v->u.coord = map_point(v->u.coord);
		for (e = agfstout(g,v); e; e = agnxtout(g,e)) map_edge(e);
	}
	translate_bb(g,g->u.left_to_right);
}

static void
place_root_label (graph_t* g)
{
    point       p,d;

    d = cvt2pt(g->u.label->dimen);
    if (Flip) {
        p.y = (g->u.bb.LL.y + g->u.bb.UR.y)/2;
        p.x = g->u.bb.LL.x + d.y/2;
    }
    else {
        p.x = (g->u.bb.LL.x + g->u.bb.UR.x)/2;
        p.y = g->u.bb.LL.y + d.y/2;
    }
    g->u.label->p = p;
}

void dotneato_postprocess(Agraph_t *g, nodesizefn_t ns)
{
	Flip = g->u.left_to_right;
	if (Flip) place_flip_graph_label(g);
	else place_graph_label(g);
	if (Flip) {
		if (g->u.label) {
			int		yd = POINTS(g->u.label->dimen.x);
			g->u.bb.LL.x -= POINTS(g->u.label->dimen.y);
				/* in case label is wide than the rest of the drawing */
			if (yd > g->u.bb.UR.y - g->u.bb.LL.y) {
				yd = yd/2;
				g->u.bb.LL.y -= yd; g->u.bb.UR.y += yd;
			}
		}
		Offset.x = -g->u.bb.UR.y;
		Offset.y = g->u.bb.LL.x;
	}
	else {
		if (g->u.label) {
			int		xd = POINTS(g->u.label->dimen.x);
			g->u.bb.LL.y -= POINTS(g->u.label->dimen.y);
			if (xd > g->u.bb.UR.x - g->u.bb.LL.x) {
				xd = xd/2;
				g->u.bb.LL.x -= xd; g->u.bb.UR.x += xd;
			}
		}
		Offset = g->u.bb.LL;
	}
	translate_drawing(g, ns);
	if (g->u.label) place_root_label (g);

	if (Show_boxes) {
		if (Flip)
			fprintf (stderr, M2, Offset.x, Offset.y, Offset.x, Offset.y);
		else
			fprintf (stderr, M1, Offset.y, Offset.x, Offset.y, Offset.x);
	}
}

void osize_label(textlabel_t *label, int *b, int* t ,int *l, int *r)
{
	point	pt,sz2;
	sz2.x = POINTS(label->dimen.x)/2;
	sz2.y = POINTS(label->dimen.y)/2;
	pt = add_points(label->p,sz2);
	if (*r < pt.x) *r = pt.x;
	if (*t < pt.y) *t = pt.y;
	pt = sub_points(label->p,sz2);
	if (*l > pt.x) *l = pt.x;
	if (*b > pt.y) *b = pt.y;
}

/* place_flip_graph_label:
 * Put cluster labels recursively in the flip case.
 */
static void place_flip_graph_label(graph_t* g)
{
	int			c,maxx,minx;
	int			maxy, miny;
	point		p,d;
    char*       pos;

    if ((g != g->root) && (g->u.label)) {
        d = cvt2pt(g->u.label->dimen);
        pos = agget(g,"labeljust");
        if (pos && (pos[0] == 'r')) {
          p.y = g->u.bb.LL.y + d.x/2;
          maxy = p.y + d.x/2;
          if (g->root->u.bb.UR.y < maxy) g->root->u.bb.UR.y = maxy;
        }
        else {
          p.y = g->u.bb.UR.y - d.x/2;
          miny = p.y - d.x/2;
          if (g->root->u.bb.LL.y > miny) g->root->u.bb.LL.y = miny;
        }

        pos = agget(g,"labelloc");
        if (pos && (pos[0] == 'b')) {
          p.x = g->u.bb.LL.x - d.y/2;
          minx = g->u.bb.LL.x - d.y;
          if (g->root->u.bb.LL.x > minx) g->root->u.bb.LL.x = minx;
        }
        else {
          p.x = g->u.bb.UR.x + d.y/2;
          maxx = g->u.bb.UR.x + d.y;
          if (g->root->u.bb.UR.x < maxx) g->root->u.bb.UR.x = maxx;
        }
        g->u.label->p = p;
    }

	for (c = 1; c <= g->u.n_cluster; c++)
		place_flip_graph_label(g->u.clust[c]);
}

/* place_graph_label:
 * Put cluster labels recursively in the non-flip case.
 */
void place_graph_label(graph_t* g)
{
	int			c;
	/* int			maxy,miny; */
    int         minx, maxx;
	point		p,d;
    char*       pos;

    if ((g != g->root) && (g->u.label)) {
        d = cvt2pt(g->u.label->dimen);
        pos = agget(g,"labeljust");
        if (pos && (pos[0] == 'r')) {
          p.x = g->u.bb.UR.x - d.x/2;
          minx = p.x - d.x/2;
          if (g->root->u.bb.LL.x > minx) g->root->u.bb.LL.x = minx;
        }
        else {
          p.x = g->u.bb.LL.x + d.x/2;
          maxx = p.x + d.x/2;
          if (g->root->u.bb.UR.x < maxx) g->root->u.bb.UR.x = maxx;
        }
        pos = agget(g,"labelloc");
        if (pos && (pos[0] == 'b')) {
          p.y = g->u.bb.LL.y + d.y/2;
#ifdef NOTDEF  
 /* remove this code - fixed the graph labeling nightmare. */
          miny = g->u.bb.LL.y - d.y;
          if (g->root->u.bb.LL.y > miny) g->root->u.bb.LL.y = miny;
#endif
        }
        else {
          p.y = g->u.bb.UR.y - d.y/2;
#ifdef NOTDEF
          maxy = g->u.bb.UR.y + d.y;
          if (g->root->u.bb.UR.y < maxy) g->root->u.bb.UR.y = maxy;
#endif
        }
        g->u.label->p = p;
    }

	for (c = 1; c <= g->u.n_cluster; c++)
		place_graph_label(g->u.clust[c]);
}
