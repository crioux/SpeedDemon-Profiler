/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
/* 
 * Decompose finds the connected components of a graph.
 * It searches the temporary edges and ignores non-root nodes.
 * The roots of the search are the real nodes of the graph,
 * but any virtual nodes discovered are also included in the
 * component.
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static graph_t		*G;
static node_t		*Last_node;
static char			Cmark;

void begin_component(void)
{
	Last_node = G->u.nlist = NULL;
}

void add_to_component(node_t* n)
{
	G->u.n_nodes++;
	n->u.mark = Cmark;
	if (Last_node) {
		n->u.prev = Last_node;
		Last_node->u.next = n;
	}
	else {
		n->u.prev = NULL;
		G->u.nlist = n;
	}
	Last_node = n;
	n->u.next = NULL;
}

void end_component(void)
{
	int		i;

	i = G->u.comp.size++;
	G->u.comp.list = ALLOC(G->u.comp.size,G->u.comp.list,node_t*);
	G->u.comp.list[i] = G->u.nlist;
}

void search_component(graph_t* g, node_t* n)
{
	int		c,i;
	elist	vec[4];
	node_t	*other;
	edge_t	*e;

	add_to_component(n);
	vec[0] = n->u.out;		vec[1] = n->u.in;
	vec[2] = n->u.flat_out;	vec[3] = n->u.flat_in;

	for (c = 0; c <= 3; c++) {
		if (vec[c].list) for (i = 0; (e = vec[c].list[i]); i++) {
			if ((other = e->head) == n) other = e->tail;
			if ((other->u.mark != Cmark) && (other == UF_find(other)))
				search_component(g,other);
		}
	}
}

void decompose(graph_t* g, int pass)
{
	graph_t	*subg;
	node_t	*n,*v;

	G = g;
	if (++Cmark == 0) Cmark = 1;
	g->u.n_nodes = g->u.comp.size = 0;
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		v = n;
		if ((pass > 0) && (subg = v->u.clust))
			v = subg->u.rankleader[v->u.rank];
		else if (v != UF_find(v)) continue;
		if (v->u.mark != Cmark) {
			begin_component();
			search_component(g,v);
			end_component();
		}
	}
}
