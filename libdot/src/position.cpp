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
 * position(g): set n->u.coord (x and y) for all nodes n of g, using g->u.rank.
 * (the graph may be modified by merging certain edges with a common endpoint.)
 * the coordinates are computed by constructing and ranking an auxiliary graph.
 * then leaf nodes are inserted in the fast graph.  cluster boundary nodes are
 * created and correctly separated.
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static void clust_ht(Agraph_t *g);


void dot_position(graph_t* g)
{
	if (g->u.nlist == NULL) return;		/* ignore empty graph */
	mark_lowclusters(g);				/* we could remove from splines.c now */
	set_ycoords(g);
	if (Concentrate) dot_concentrate(g);
	expand_leaves(g);
	flat_edges(g);
	create_aux_edges(g);
	rank(g,FALSE,nsiter2(g));
	set_xcoords(g);
	remove_aux_edges(g);
	set_aspect(g);
}

int nsiter2(graph_t* g)
{
	int		maxiter = MAXINT;
	char	*s;

	if ((s = agget(g,"nslimit")))
		maxiter = (int)(atof(s) * agnnodes(g));
	return maxiter;
}

static int searchcnt;
static int go(node_t* u, node_t* v)
{
	int		i;
	edge_t	*e;

	if (u == v) return TRUE;
	for (i = 0; (e = u->u.out.list[i]); i++) {
		if (go(e->head,v))
			return TRUE;
	}
	return FALSE;
}

static int canreach(node_t* u, node_t* v)
{
	if (++searchcnt == 0) searchcnt = 1;
	return go(u,v);
}

edge_t *
make_aux_edge(node_t *u, node_t *v, int len, int wt)
{
	edge_t	*e;

	e = NEW(edge_t);
	e->tail = u;
	e->head = v;
	e->u.minlen = len;
	e->u.weight = wt;
	fast_edge(e);
	return e;
}


void allocate_aux_edges(graph_t* g)
{
	int		i,j,n_in;
	node_t	*n;

	/* allocate space for aux edge lists */
	for (n = g->u.nlist; n; n = n->u.next) {
		n->u.save_in = n->u.in;
		n->u.save_out = n->u.out;
		for (i = 0; n->u.out.list[i]; i++);
		for (j = 0; n->u.in.list[j]; j++);
		n_in = i + j;
		alloc_elist(n_in + 3,n->u.in);
		alloc_elist(3,n->u.out);
	}
}

void make_LR_constraints(graph_t* g)
{
	int		i,j,k;
	int		sw;		/* self width */
	int		m0,m1;
	int		width;
	edge_t	*e, *e0, *e1, *f, *ff;
	node_t	*u,*v, *t0, *h0;
	rank_t	*rank = g->u.rank;

	/* make edges to constrain left-to-right ordering */
	for (i = g->u.minrank; i <= g->u.maxrank; i++) {
		int		last;
		last = rank[i].v[0]->u.rank = 0;
		for (j = 0; j < rank[i].n; j++) {
			u = rank[i].v[j];
			u->u.mval = u->u.rw;	/* keep it somewhere safe */
			if (u->u.other.size > 0) {	/* compute self size */
				sw = 0;
				for (k = 0; (e = u->u.other.list[k]); k++) {
					if (e->tail == e->head) {
						sw += SELF_EDGE_SIZE;
						if (e->u.label) {
							double	label_width;
							label_width = g->u.left_to_right? e->u.label->dimen.y : e->u.label->dimen.x;
							sw += POINTS(label_width);
						}
					}
				}
				u->u.rw += sw;			/* increment to include self edges */
			}
			v = rank[i].v[j+1];
			if (v) {
				width = u->u.rw + v->u.lw + g->u.nodesep;
				e0 = make_aux_edge(u,v,width,0);
				last = (v->u.rank = last + width);
			}

				/* position flat edge endpoints */
			for (k = 0; k < u->u.flat_out.size; k++) {
				e = u->u.flat_out.list[k];
				v = e->head;
				if (e->tail->u.order < e->head->u.order)
					{ t0 = e->tail; h0 = e->head; }
				else
					{ t0 = e->head; h0 = e->tail; }

				/* case 1: flat edge with a label */
				if ((f = e->u.to_virt)) {
					while (f->u.to_virt) f = f->u.to_virt;
					e0 = f->tail->u.save_out.list[0];
					e1 = f->tail->u.save_out.list[1];
					if (e0->head->u.order > e1->head->u.order)
						{ ff = e0; e0 = e1; e1 = ff; }
					m0 = (e->u.minlen *g->u.nodesep)/2;
					m1 = m0 + e0->head->u.rw + e0->tail->u.lw;
					/* these guards are needed because the flat edges
						work very poorly with cluster layout */
					if (canreach(e0->tail,e0->head) == FALSE)
						make_aux_edge(e0->head,e0->tail,m1,e->u.weight);
					m1 = m0 + e1->tail->u.rw + e1->head->u.lw;
					if (canreach(e1->head,e1->tail) == FALSE)
						make_aux_edge(e1->tail,e1->head,m1,e->u.weight);
					continue;
				}

				m0 = e->u.minlen *g->u.nodesep + t0->u.rw + h0->u.lw;

				if ((e0 = find_fast_edge(t0,h0)))
					/* case 2: flat edge between neighbors */
					e0->u.minlen = MAX(e0->u.minlen,m0);
				else
					/* case 3: flat edge between non-neighbors */
					make_aux_edge(t0,h0,m0,e->u.weight);
			}
		}
	}
}

/* make_edge_pairs: make virtual edge pairs corresponding to input edges */
void make_edge_pairs(graph_t* g)
{
	int			i,m0,m1;
	node_t		*n,*sn;
	edge_t		*e;

	for (n = g->u.nlist; n; n = n->u.next) {
		if (n->u.save_out.list) for (i = 0; (e = n->u.save_out.list[i]); i++) {
			sn = virtual_node(g);
			sn->u.node_type = SLACKNODE;
			m0 = (e->u.head_port.p.x - e->u.tail_port.p.x);
			if (m0 > 0) m1 = 0;
			else {m1 = -m0; m0 = 0;}
			make_aux_edge(sn,e->tail,m0+1,e->u.weight);
			make_aux_edge(sn,e->head,m1+1,e->u.weight);
			sn->u.rank = MIN(e->tail->u.rank - m0 -1, e->head->u.rank - m1 - 1);
		}
	}
}

/* pos_clusters: create constraints for:
 *	node containment in clusters,
 *	cluster containment in clusters,
 *	separation of sibling clusters.
 */
void pos_clusters(graph_t* g)
{
	if (g->u.n_cluster > 0) {
		contain_clustnodes(g);
		keepout_othernodes(g);
		contain_subclust(g);
		separate_subclust(g);
	}
}

void contain_clustnodes(graph_t* g)
{
	int		c;

	if (g != g->root) {
		contain_nodes(g);
		make_aux_edge(g->u.ln, g->u.rn, 1, 128);	/* clust compaction edge */
	}
	for (c = 1; c <= g->u.n_cluster; c++)
		contain_clustnodes(g->u.clust[c]);
}

int vnode_not_related_to(graph_t* g, node_t* v)
{
	edge_t	*e;

	if (v->u.node_type != VIRTUAL) return FALSE;
	for (e = v->u.save_out.list[0]; e->u.to_orig; e = e->u.to_orig);
	if (agcontains(g,e->tail)) return FALSE;
	if (agcontains(g,e->head)) return FALSE;
	return TRUE;
}

void keepout_othernodes(graph_t* g)
{
	int		i,c,r;
	node_t	*u,*v;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		if (g->u.rank[r].n == 0) continue;
		v = g->u.rank[r].v[0];
		if (v == NULL) continue;
		for (i = v->u.order - 1; i >= 0; i--) {
			u = g->root->u.rank[r].v[i];
				/* can't use "is_a_vnode_of" because elists are swapped */
			if ((u->u.node_type == NORMAL) || vnode_not_related_to(g,u)) {
				make_aux_edge(u,g->u.ln,CL_OFFSET+u->u.rw+g->u.border[LEFT_IX].x,0);
				break;
			}
		}
		for (i = v->u.order + g->u.rank[r].n; i < g->root->u.rank[r].n; i++) {
			u = g->root->u.rank[r].v[i];
			if ((u->u.node_type == NORMAL) || vnode_not_related_to(g,u)) {
				make_aux_edge(g->u.rn,u,CL_OFFSET+u->u.lw+g->u.border[RIGHT_IX].x,0);
				break;
			}
		}
	}
	
	for (c = 1; c <= g->u.n_cluster; c++)
		keepout_othernodes(g->u.clust[c]);
}

void contain_subclust(graph_t* g)
{
	int		c;
	graph_t	*subg;

	make_lrvn(g);
	for (c = 1; c <= g->u.n_cluster; c++) {
		subg = g->u.clust[c];
		make_lrvn(subg);
		make_aux_edge(g->u.ln, subg->u.ln, CL_OFFSET + subg->u.border[LEFT_IX].x, 0);
		make_aux_edge(subg->u.rn, g->u.rn, CL_OFFSET + subg->u.border[RIGHT_IX].x, 0);
		contain_subclust(subg);
	}
}

void separate_subclust(graph_t* g)
{
	int			i,j;
	graph_t		*low,*high;
	graph_t		*left,*right;

	for (i = 1; i <= g->u.n_cluster; i++) make_lrvn(g->u.clust[i]);
	for (i = 1; i <= g->u.n_cluster; i++) {
		for (j = i + 1; j <= g->u.n_cluster; j++) {
			low = g->u.clust[i]; high = g->u.clust[j];
			if (low->u.minrank > high->u.minrank)
				{ graph_t	*temp = low; low = high; high= temp; }
			if (low->u.maxrank < high->u.minrank) continue;
			if ((low->u.rank[high->u.minrank].v[0]->u.order)
				< (high->u.rank[high->u.minrank].v[0]->u.order))
					{ left = low; right = high; }
				else
					{ left = high; right = low; }
			make_aux_edge(left->u.rn, right->u.ln,
				CL_OFFSET+left->u.border[RIGHT_IX].x+right->u.border[LEFT_IX].x,0);
		}
		separate_subclust(g->u.clust[i]);
	}
}

void create_aux_edges(graph_t* g)
{
	allocate_aux_edges(g);
	make_LR_constraints(g);
	make_edge_pairs(g);
	pos_clusters(g);
	compress_graph(g);
}

void remove_aux_edges(graph_t* g)
{
	int		i;
	node_t	*n,*nnext,*nprev;
	edge_t	*e;

	for (n = g->u.nlist; n; n = n->u.next) {
		for (i = 0; (e = n->u.out.list[i]); i++) free(e);
		free_list(n->u.out);
		free_list(n->u.in);
		n->u.out = n->u.save_out;
		n->u.in = n->u.save_in;
	}
	/* cannot be merged with previous loop */
	nprev = NULL;
	for (n = g->u.nlist; n; n = nnext) {
		nnext = n->u.next;
		if (n->u.node_type == SLACKNODE) {
			if (nprev) nprev->u.next = nnext;
			else g->u.nlist = nnext;
			free(n);
		}
		else nprev = n;
	}
	g->u.nlist->u.prev = NULL;
}

void set_xcoords(graph_t* g)
{
	int		i,j;
	node_t	*v;
	rank_t	*rank = g->u.rank;

	for (i = g->u.minrank;  i <= g->u.maxrank; i++) {
		for (j = 0; j < rank[i].n; j++) {
			v = rank[i].v[j];
			v->u.coord.x = v->u.rank;
			v->u.rank = i;
		}
	}
}

/* set y coordinates of nodes, a rank at a time */
void set_ycoords(graph_t* g)
{
	int		i,r,ht2,maxht,delta,d0,d1;
	node_t	*n;
	rank_t	*rank = g->u.rank;
	graph_t	*clust;
	
	ht2 = maxht = 0;

	/* scan ranks for tallest nodes.  */
	for (r = g->u.minrank; r <=  g->u.maxrank; r++) {
		for (i = 0; i < rank[r].n; i++) {
			n = rank[r].v[i];

			/* assumes symmetry, ht1 = ht2 */
			ht2 = (n->u.ht + 1)/2;

			/* update global rank ht */
			if (rank[r].pht2 < ht2) rank[r].pht2 = rank[r].ht2 = ht2;
			if (rank[r].pht1 < ht2) rank[r].pht1 = rank[r].ht1 = ht2;

			/* update nearest enclosing cluster rank ht */
			if ((clust = n->u.clust)) {
				if (n->u.rank == clust->u.minrank)
					clust->u.ht2 = MAX(clust->u.ht2,ht2 + CL_OFFSET);
				if (n->u.rank == clust->u.maxrank)
					clust->u.ht1 = MAX(clust->u.ht1,ht2 + CL_OFFSET);
			}
		}
	}

	/* scan sub-clusters */
	clust_ht(g);

	/* make the initial assignment of ycoords to leftmost nodes by ranks */
	maxht = 0;
	r = g->u.maxrank;
	rank[r].v[0]->u.coord.y = rank[r].ht1;
	while (--r >= g->u.minrank) {
		d0 = rank[r+1].pht2 + rank[r].pht1 + g->u.ranksep; /* prim node sep */
		d1 = rank[r+1].ht2 + rank[r].ht1 + CL_OFFSET;	/* cluster sep */
		delta = MAX(d0,d1);
		if (rank[r].n > 0) /* this may reflect some problem */
		rank[r].v[0]->u.coord.y = rank[r + 1].v[0]->u.coord.y + delta;
#ifdef DEBUG
		else fprintf(stderr,"dot set_ycoords: rank %d is empty\n",rank[r].n);
#endif
		maxht = MAX(maxht,delta);
	}

	/* re-assign if ranks are equally spaced */
	if (g->u.exact_ranksep)
		for (r = g->u.maxrank - 1; r <=  g->u.minrank; r--)
			if (rank[r].n > 0) /* this may reflect the same problem :-() */
				rank[r].v[0]->u.coord.y = rank[r + 1].v[0]->u.coord.y + maxht;

	/* copy ycoord assignment from leftmost nodes to others */
	for (n = g->u.nlist; n; n = n->u.next)
		n->u.coord.y = rank[n->u.rank].v[0]->u.coord.y;
}

void compute_bb(graph_t* g, graph_t* root)
{
	int		c,r,x;
	node_t	*v;
	point	LL,UR,p,offset;
	
	LL.x = LL.y = MAXINT;
	UR.x = UR.y = -MAXINT;
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		if (g->u.rank[r].n == 0) continue;
		if ((v = g->u.rank[r].v[0]) == NULL) continue;
		x = v->u.coord.x - v->u.lw;
		if (g != g->root) x-= CL_OFFSET;
		LL.x = MIN(LL.x,x);
		v = g->u.rank[r].v[g->u.rank[r].n-1];
		x = v->u.coord.x + v->u.rw;
		if (g != g->root) x+= CL_OFFSET;
		UR.x = MAX(UR.x,x);
	}
	offset.x = offset.y = CL_OFFSET;
	for (c = 1; c <= g->u.n_cluster; c++) {
		p = sub_points(g->u.clust[c]->u.bb.LL,offset);
		if (LL.x > p.x) LL.x = p.x;
		p = add_points(g->u.clust[c]->u.bb.UR,offset);
		if (UR.x < p.x) UR.x = p.x;
	}
	LL.y = root->u.rank[g->u.maxrank].v[0]->u.coord.y - g->u.ht1;
	UR.y = root->u.rank[g->u.minrank].v[0]->u.coord.y + g->u.ht2;
	g->u.bb.LL = LL; g->u.bb.UR = UR;
}

void update_bb(graph_t* g, point pt)
{
	if (pt.x > g->u.bb.UR.x)  g->u.bb.UR.x = pt.x;
	if (pt.y > g->u.bb.UR.y)  g->u.bb.UR.y = pt.y;
	if (pt.x < g->u.bb.LL.x)  g->u.bb.LL.x = pt.x;
	if (pt.y < g->u.bb.LL.y)  g->u.bb.LL.y = pt.y;
}

void rec_bb(graph_t *g, graph_t *root)
{
	int		c;
	for (c = 1; c <= g->u.n_cluster; c++)
		rec_bb(g->u.clust[c],root);
	compute_bb(g,root);
}

void set_aspect(graph_t* g)
{
	double	xf,yf,actual,desired;
	char	*str;
	node_t	*n;
	boolean	scale_it,filled;

	rec_bb(g,g);
	if ((g->u.maxrank > 0) && (str = agget(g,"ratio"))) {
		g->u.bb.UR.x -= g->u.bb.LL.x;
		g->u.bb.UR.y -= g->u.bb.LL.y;	/* normalize */
		if (g->u.left_to_right)
			{int t = g->u.bb.UR.x; g->u.bb.UR.x = g->u.bb.UR.y; g->u.bb.UR.y = t;}
		scale_it = TRUE;
		if (streq(str,"auto")) filled = idealsize(g,.5);
		else filled = (streq(str,"fill"));
		if (filled) {
			/* fill is weird because both X and Y can stretch */
			if (g->u.drawing->size.x <= 0) scale_it = FALSE;
			else {
				xf = (double)g->u.drawing->size.x / (double)g->u.bb.UR.x;
				yf = (double)g->u.drawing->size.y / (double)g->u.bb.UR.y;
				if ((xf < 1.0) || (yf < 1.0)) {
					if (xf < yf) {yf = yf / xf; xf = 1.0;}
					else {xf = xf / yf; yf = 1.0;}
				}
			}
		}
		else {
			desired = atof(str);
			if (desired == 0.0) scale_it = FALSE;
			else {
				actual = ((float)g->u.bb.UR.y)/((float)g->u.bb.UR.x);
				if (actual < desired) {yf = desired/actual; xf = 1.0;}
				else {xf = actual/desired; yf = 1.0;}
			}
		}
		if (scale_it) {
			if (g->u.left_to_right) {double t = xf; xf = yf; yf = t;}
			for (n = g->u.nlist; n; n = n->u.next) {
				n->u.coord.x = (int)(n->u.coord.x * xf);
				n->u.coord.y = (int)(n->u.coord.y * yf);
			}
		}
	}
	rec_bb(g,g);
}

point
resize_leaf(node_t* leaf, point lbound)
{
	dot_nodesize(leaf,leaf->graph->u.left_to_right);
	leaf->u.coord.y = lbound.y;
	leaf->u.coord.x = lbound.x + leaf->u.lw;
	lbound.x = lbound.x + leaf->u.lw + leaf->u.rw + leaf->graph->u.nodesep;
	return lbound;
}

point
place_leaf(node_t* leaf, point lbound, int order)
{
	node_t	*leader;
	graph_t	*g = leaf->graph;

	leader = UF_find(leaf);
	if (leaf != leader) fast_nodeapp(leader,leaf);
	leaf->u.order = order;
	leaf->u.rank = leader->u.rank;
	g->u.rank[leaf->u.rank].v[leaf->u.order] = leaf;
	return resize_leaf(leaf,lbound);
}

/* make space for the leaf nodes of each rank */
void make_leafslots(graph_t* g)
{
	int		i,j,r;
	node_t	*v;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		j = 0;
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];
			v->u.order = j;
			if (v->u.ranktype == LEAFSET) j = j + v->u.UF_size;
			else j++;
		}
		if (j <= g->u.rank[r].n) continue;
		g->u.rank[r].v = ALLOC(j+1,g->u.rank[r].v,node_t*);
		for (i = g->u.rank[r].n - 1; i >= 0; i--) {
			v = g->u.rank[r].v[i];
			g->u.rank[r].v[v->u.order] = v;
		}
		g->u.rank[r].n = j;
		g->u.rank[r].v[j] = NULL;
	}
}

void do_leaves(graph_t* g, node_t* leader)
{
	int		j;
	point	lbound;
	node_t	*n;
	edge_t	*e;

	if (leader->u.UF_size <= 1) return;
	lbound.x = leader->u.coord.x - leader->u.lw;
	lbound.y = leader->u.coord.y;
	lbound = resize_leaf(leader,lbound);
	if (leader->u.out.size > 0) {		/* in-edge leaves */
		n = leader->u.out.list[0]->head;
		j = leader->u.order + 1;
		for (e = agfstin(g,n); e; e = agnxtin(g,e)) {
			if ((e->tail != leader) && (UF_find(e->tail) == leader)) {
				lbound = place_leaf(e->tail,lbound,j++);
				unmerge_oneway(e);
				elist_append(e,e->head->u.in);
			}
		}
	}
	else {							/* out edge leaves */
		n = leader->u.in.list[0]->tail;
		j = leader->u.order + 1;
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
			if ((e->head != leader) && (UF_find(e->head) == leader)) {
				lbound = place_leaf(e->head,lbound,j++);
				unmerge_oneway(e);
				elist_append(e,e->tail->u.out);
			}
		}
	}
}

int ports_eq(edge_t *e,edge_t *f)
{
	return (
		   (e->u.head_port.defined == f->u.head_port.defined)
		&& ( ((e->u.head_port.p.x == f->u.head_port.p.x) &&
			 (e->u.head_port.p.y == f->u.head_port.p.y))
			|| (e->u.head_port.defined == FALSE))
		&& ( ((e->u.tail_port.p.x == f->u.tail_port.p.x) &&
			 (e->u.tail_port.p.y == f->u.tail_port.p.y))
			|| (e->u.tail_port.defined == FALSE))
	);
}

void expand_leaves(graph_t* g)
{
	int		i,d;
	node_t	*n;
	edge_t	*e,*f;

	make_leafslots(g);
	for (n = g->u.nlist; n; n = n->u.next) {
		if (n->u.inleaf) do_leaves(g,n->u.inleaf);
		if (n->u.outleaf) do_leaves(g,n->u.outleaf);
		if (n->u.other.list) for (i = 0; (e = n->u.other.list[i]); i++) {
			if ((d = e->head->u.rank - e->head->u.rank) == 0) continue;
			f = e->u.to_orig;
			if (ports_eq(e,f) == FALSE) {
				zapinlist(&(n->u.other),e);
				if (d == 1) fast_edge(e);
				/*else unitize(e); ### */
				i--;
			}
		}
	}
}

void compress_graph(graph_t* g)
{
	char		*str;
	double		x;
	point		p;

	p = g->u.drawing->size;
	if ((str = agget(g,"ratio")) == NULL) return;
	if (strcmp(str,"compress")) return;
	if (p.x * p.y <= 1) return;
	contain_nodes(g);
	if (g->u.left_to_right == FALSE) x = p.x; else x = p.y;
	make_aux_edge(g->u.ln,g->u.rn,(int)x,1000);
}

void make_lrvn(graph_t* g)
{
	node_t		*ln,*rn;

	if (g->u.ln) return;
	ln = virtual_node(g->root); ln->u.node_type = SLACKNODE;
	rn = virtual_node(g->root); rn->u.node_type = SLACKNODE;
	g->u.ln = ln; g->u.rn = rn;
}

/* contain_nodes: make left and right bounding box virtual nodes,
 * 		constrain interior nodes
 */
void contain_nodes(graph_t* g)
{
	int			r;
	node_t		*ln,*rn,*v;

	make_lrvn(g); ln = g->u.ln; rn = g->u.rn;
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		if (g->u.rank[r].n == 0) continue;
		v = g->u.rank[r].v[0];
		if (v == NULL) {
			fprintf(stderr,"contain_nodes clust %s rank %d missing node\n",g->name,r);
			continue;
		}
		make_aux_edge(ln,v,v->u.lw + CL_OFFSET,0);
		v = g->u.rank[r].v[g->u.rank[r].n - 1];
		make_aux_edge(v,rn,v->u.rw + CL_OFFSET,0);
	}
}

/* set g->drawing->size to a reasonable default.
 * returns a boolean to indicate if drawing is to
 * be scaled and filled */
int idealsize(graph_t* g, double minallowed)
{
	double		xf,yf,f,R;
	point		b,relpage,margin;

	/* try for one page */
	relpage = g->u.drawing->page;
	if (relpage.x == 0) return FALSE;				/* no page was specified */
	margin = g->u.drawing->margin;
	relpage = sub_points(relpage,margin);
	relpage = sub_points(relpage,margin);
	b.x = g->u.bb.UR.x; b.y = g->u.bb.UR.y;
	xf = (double)relpage.x / b.x;
	yf = (double)relpage.y / b.y;
	if ((xf >= 1.0) && (yf >= 1.0)) return FALSE;	/* fits on one page */

	f = MIN(xf,yf);
	xf = yf = MAX(f,minallowed);

	R = ceil((xf * b.x)/relpage.x);
	xf = ((R * relpage.x) / b.x);
	R = ceil((yf * b.y)/relpage.y);
	yf = ((R * relpage.y) / b.y);
	g->u.drawing->size.x = (int)(b.x * xf);
	g->u.drawing->size.y = (int)(b.y * yf);
	return TRUE;
}

/*
 * recursively compute cluster ht requirements.  assumes subg->u.ht1 and ht2
 * are computed from primitive nodes only.  updates ht1 and ht2 to reflect
 * cluster nesting and labels.  also maintains global rank ht1 and ht2.
 */
static void
clust_ht(Agraph_t *g)
{
	int	c, ht1, ht2;
	graph_t	*subg;
	rank_t	*rank = g->root->u.rank;

	ht1 = g->u.ht1;
	ht2 = g->u.ht2;

	/* account for sub-clusters */
	for (c = 1; c <= g->u.n_cluster; c++) {
		subg = g->u.clust[c];
		clust_ht(subg);
		if (subg->u.maxrank == g->u.maxrank)
			ht1 = MAX(ht1,subg->u.ht1 + CL_OFFSET);
		if (subg->u.minrank == g->u.minrank)
			ht2 = MAX(ht2,subg->u.ht2 + CL_OFFSET);
	}

	/* account for a possible cluster label */
	ht1 += g->u.border[BOTTOM_IX].y;
	ht2 += g->u.border[TOP_IX].y;
	g->u.ht1 = ht1;
	g->u.ht2 = ht2;

	/* update the global ranks */
	if (g != g->root) {
		rank[g->u.minrank].ht2 = MAX(rank[g->u.minrank].ht2,ht2);
		rank[g->u.maxrank].ht1 = MAX(rank[g->u.maxrank].ht1,ht1);
	}
}
