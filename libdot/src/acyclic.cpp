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
 * Break cycles in a directed graph by depth-first search.
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void acyclic(graph_t* g)
{
	int		c;
	node_t	*n;

	for (c = 0; c < g->u.comp.size; c++) {
		g->u.nlist = g->u.comp.list[c];
		for (n = g->u.nlist; n; n = n->u.next) n->u.mark = FALSE;
		for (n = g->u.nlist; n; n = n->u.next) dfs(n);
	}
}

void dfs(node_t* n)
{
	int		i;
	edge_t	*e;
	node_t	*w;
	
	if (n->u.mark) return;
	n->u.mark = TRUE;
	n->u.onstack = TRUE;
	for (i = 0; (e = n->u.out.list[i]); i++) {
		w = e->head;
		if (w->u.onstack) { reverse_edge(e); i--; }
		else { if (w->u.mark == FALSE) dfs(w); }
	}
	n->u.onstack = FALSE;
}

void reverse_edge(edge_t* e)
{
	edge_t		*f;

	delete_fast_edge(e);
	if ((f = find_fast_edge(e->head,e->tail))) merge_oneway(e,f);
	else virtual_edge(e->head,e->tail,e);
}
