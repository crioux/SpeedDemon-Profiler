/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include "dot.h"                  

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*
 * operations on the fast internal graph.
 */

static edge_t* ffe(node_t *u, elist uL, node_t *v, elist vL)
{
	int		i;
	edge_t	*e;

	if (uL.size < vL.size) {
		for (i = 0; (e = uL.list[i]); i++)
			if (e->head == v) break;
	}
	else {
		for (i = 0; (e = vL.list[i]); i++)
			if (e->tail == u) break;
	}
	return e;
}

edge_t* find_fast_edge(node_t *u,node_t *v)
{
	return ffe(u,u->u.out,v,v->u.in);
}

node_t* find_fast_node(graph_t *g, node_t *n)
{
	node_t		*v;
	for (v = g->u.nlist; v; v = v->u.next)
		if (v == n) break;
	return v;
}

edge_t* find_flat_edge(node_t *u, node_t *v)
{
	return ffe(u,u->u.flat_out,v,v->u.flat_in);
}

/* safe_list_append - append e to list L only if e not already a member */
void safe_list_append(edge_t *e, elist *L)
{
	int		i;

	for (i = 0; i < L->size; i++) if (e == L->list[i]) return;
	elist_append(e,(*L));
}

edge_t*
fast_edge(edge_t *e)
{
#ifdef DEBUG
	int		i;
	edge_t	*f;
	for (i = 0; (f = e->tail->u.out.list[i]); i++) {
		if (e == f) {fprintf(stderr,"duplicate fast edge\n"); return;}
		assert (e->head != f->head);
	}
	for (i = 0; (f = e->head->u.in.list[i]); i++) {
		if (e == f) {fprintf(stderr,"duplicate fast edge\n"); return;}
		assert (e->tail != f->tail);
	}
#endif
	elist_append(e,e->tail->u.out);
	elist_append(e,e->head->u.in);
	return e;
}

/* zapinlist - remove e from list and fill hole with last member of list */
void zapinlist(elist *L, edge_t *e)
{
	int	i;

	for (i = 0; i < L->size; i++) {
		if (L->list[i] == e) {
			L->size--;
			L->list[i] = L->list[L->size];
			L->list[L->size] = NULL;
			break;
		}
	}
}

/* disconnects e from graph */
void delete_fast_edge(edge_t *e)
{
	assert(e != NULL);
	zapinlist(&(e->tail->u.out),e);
	zapinlist(&(e->head->u.in),e);
}

void safe_delete_fast_edge(edge_t *e)
{
	int		i;
	edge_t	*f;

	assert(e != NULL);
	for (i = 0; (f = e->tail->u.out.list[i]); i++)
		if (f == e) zapinlist(&(e->tail->u.out),e);
	for (i = 0; (f = e->head->u.in.list[i]); i++)
		if (f == e) zapinlist(&(e->head->u.in),e);
}

void other_edge(edge_t *e)
{
	elist_append(e,e->tail->u.other);
}

void safe_other_edge(edge_t *e)
{
	safe_list_append(e,&(e->tail->u.other));
}

void delete_other_edge(edge_t *e)
{
	assert(e != NULL);
	zapinlist(&(e->tail->u.other),e);
}

/* orig might be an input edge, reverse of an input edge, or virtual edge */
edge_t*
new_virtual_edge(node_t *u, node_t *v, edge_t *orig)
{
	edge_t		*e;

	e = NEW(edge_t);
	e->tail = u;
	e->head = v;
	e->u.edge_type = VIRTUAL;

	if (orig) {
		e->u.count = orig->u.count;
		e->u.xpenalty = orig->u.xpenalty;
		e->u.weight = orig->u.weight;
		e->u.minlen = orig->u.minlen;
		if (e->tail == orig->tail) e->u.tail_port = orig->u.tail_port;
		else if (e->tail == orig->head) e->u.tail_port = orig->u.head_port;
		if (e->head == orig->head) e->u.head_port = orig->u.head_port;
		else if (e->head == orig->tail) e->u.head_port = orig->u.tail_port;

		if (orig->u.to_virt == NULL) orig->u.to_virt = e;
		e->u.to_orig = orig;
	}
	else e->u.minlen = e->u.count = e->u.xpenalty = e->u.weight = 1;
	return e;
}

edge_t*
virtual_edge(node_t *u, node_t *v, edge_t *orig)
{
	return fast_edge(new_virtual_edge(u,v,orig));
}

void fast_node(graph_t *g, Agnode_t *n)
{

#ifdef DEBUG
	assert (find_fast_node(g,n) == NULL);
#endif
	n->u.next = g->u.nlist;
	if (n->u.next) n->u.next->u.prev = n;
	g->u.nlist = n;
	n->u.prev = NULL;
	assert (n != n->u.next);
}

void fast_nodeapp(node_t *u, node_t *v)
{
	assert (u != v);
	assert (v->u.next == NULL);
	v->u.next = u->u.next;
	if (u->u.next) u->u.next->u.prev = v;
	v->u.prev = u;
	u->u.next = v;
}

void delete_fast_node(graph_t *g, node_t *n)
{
	assert(find_fast_node(g,n));
	if (n->u.next) n->u.next->u.prev = n->u.prev;
	if (n->u.prev) n->u.prev->u.next = n->u.next;
	else g->u.nlist = n->u.next;
}

node_t* virtual_node(graph_t *g)
{
	node_t		*n;

	n = NEW(node_t);
	n->name = "virtual";
	n->graph = g;
	n->u.node_type = VIRTUAL;
	n->u.lw = n->u.rw = 1;
	n->u.ht = 1;
	n->u.UF_size = 1;
	alloc_elist(4,n->u.in);
	alloc_elist(4,n->u.out);
	fast_node(g,n);
	g->u.n_nodes++;
	return n;
}

void flat_edge(graph_t *g, edge_t *e)
{
	elist_append(e,e->tail->u.flat_out);
	elist_append(e,e->head->u.flat_in);
	g->root->u.has_flat_edges = g->u.has_flat_edges = TRUE;
}

void delete_flat_edge(edge_t *e)
{
	assert(e != NULL);
	zapinlist(&(e->tail->u.flat_out),e);
	zapinlist(&(e->head->u.flat_in),e);
}

#ifdef DEBUG
static char*
NAME(node_t *n)
{
	static char buf[20];
	if (n->u.node_type == NORMAL) return n->name;
	sprintf(buf,"V%x",n);
	return buf;
}

void fastgr(graph_t *g)
{
	int			i,j;
	node_t		*n,*w;
	edge_t		*e,*f;

	for (n = g->u.nlist; n; n = n->u.next) {
		fprintf(stderr,"%s %d: (",NAME(n), n->u.rank);
		for (i = 0; e = n->u.out.list[i]; i++) {
			fprintf(stderr," %s:%d",NAME(e->head),e->u.count);
			w = e->head;
			if (g == g->root) {
				for (j = 0; f = w->u.in.list[j]; j++) if (e == f) break;
				assert (f != NULL);
			}
		}
		fprintf(stderr," ) (");
		for (i = 0; e = n->u.in.list[i]; i++) {
			fprintf(stderr," %s:%d",NAME(e->tail),e->u.count);
			w = e->tail;
			if (g == g->root) {
				for (j = 0; f = w->u.out.list[j]; j++) if (e == f) break;
				assert (f != NULL);
			}
		}
		fprintf(stderr," )\n");
	}
}
#endif

void merge_oneway(edge_t *e, edge_t *rep)
{
	if (rep == e->u.to_virt) {fprintf(stderr,"warning, merge_oneway glitch\n"); return;}
	assert(e->u.to_virt == NULL);
	e->u.to_virt = rep;
	basic_merge(e,rep);
}

void basic_merge(edge_t *e, edge_t *rep)
{
	if (rep->u.minlen < e->u.minlen)
		rep->u.minlen = e->u.minlen;
	while (rep) {
		rep->u.count += e->u.count;
		rep->u.xpenalty += e->u.xpenalty;
		rep->u.weight += e->u.weight;
		rep = rep->u.to_virt;
	}
}

static void unrep(edge_t *rep, edge_t *e)
{
	rep->u.count -= e->u.count;
	rep->u.xpenalty -= e->u.xpenalty;
	rep->u.weight -= e->u.weight;
}

void unmerge_oneway(edge_t *e)
{
	edge_t	*rep,*nextrep;
	for (rep = e->u.to_virt; rep; rep = nextrep) {
		unrep(rep,e);
		nextrep = rep->u.to_virt;
		if (rep->u.count == 0) safe_delete_fast_edge(rep);	/* free(rep)? */

		/* unmerge from a virtual edge chain */
		while ((rep->u.edge_type == VIRTUAL)
		&& (rep->head->u.node_type == VIRTUAL)
		&& (rep->head->u.out.size == 1)) {
			rep = rep->head->u.out.list[0];
			unrep(rep,e);
		}
	}
	e->u.to_virt = NULL;
}

int is_fast_node(graph_t *g, node_t *v)
{
	node_t		*n;

	for (n = g->u.nlist; n; n = n->u.next)
		if (v == n) return TRUE;
	return FALSE;
}
