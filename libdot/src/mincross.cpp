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
 * dot_mincross(g) takes a ranked graphs, and finds an ordering
 * that avoids edge crossings.  clusters are expanded.
 * N.B. the rank structure is global (not allocated per cluster)
 * because mincross may compare nodes in different clusters.
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define MARK(v)			((v)->u.mark)
#define saveorder(v)	((v)->u.coord.x)
#define flatindex(v)	((v)->u.low)

	/* forward declarations */
static boolean medians(graph_t *g, int r0, int r1);
static int nodeposcmpf(node_t **n0, node_t **n1);
static int edgeidcmpf(edge_t **e0, edge_t **e1);
static void flat_breakcycles(graph_t* g);
static void flat_reorder(graph_t* g);
static void flat_search(graph_t* g, node_t* v);
static void init_mincross(graph_t* g);
static void merge2(graph_t* g);
static void init_mccomp(graph_t* g, int c);
static void cleanup2(graph_t* g, int nc);
static int mincross_clust(graph_t *par, graph_t *g);
static int mincross(graph_t *g, int startpass, int endpass);
static void init_mincross(graph_t* g);
static void mincross_step(graph_t* g, int pass);
static void mincross_options(graph_t* g);
static void save_best(graph_t* g);
static void restore_best(graph_t* g);
static adjmatrix_t* new_matrix(int i, int j);
static void free_matrix(adjmatrix_t* p);

	/* mincross parameters */
static int		MaxIter = 24;
static int		MinQuit = 8;
static double 	Convergence = .995;

static graph_t	*Root;
static int		GlobalMinRank,GlobalMaxRank;
static edge_t	**TE_list;
static int		*TI_list;
static boolean	ReMincross;

void dot_mincross(graph_t* g)
{
	int		c,nc;

	init_mincross(g);

	for (nc = c = 0; c < g->u.comp.size; c++) {
		init_mccomp(g,c);
		nc += mincross(g,0,2);
	}

	merge2(g);

	/* run mincross on contents of each cluster */
	for (c = 1; c <= g->u.n_cluster; c++) {
		nc += mincross_clust(g,g->u.clust[c]);
#ifdef DEBUG
		check_vlists(g->u.clust[c]);
		check_order();
#endif
	}

	if (g->u.n_cluster > 0) {
		mark_clusters(g);
		ReMincross = TRUE;
		nc = mincross(g,2,2);
#ifdef DEBUG
		for (c = 1; c <= g->u.n_cluster; c++)
			check_vlists(g->u.clust[c]);
#endif
	}
	cleanup2(g,nc);
}

static adjmatrix_t* new_matrix(int i, int j)
{
	adjmatrix_t	*rv = NEW(adjmatrix_t);
	rv->nrows = i; rv->ncols = j;
	rv->data = N_NEW(i*j,char);
	return rv;
}

static void free_matrix(adjmatrix_t* p)
{
	if (p) {free(p->data); free(p);}
}
#define ELT(M,i,j)		(M->data[((i)*M->ncols)+(j)])

static void init_mccomp(graph_t* g, int c)
{
	int		r;

	g->u.nlist = g->u.comp.list[c];
	if (c > 0) {
		for (r = g->u.minrank; r <= g->u.maxrank; r++) {
			g->u.rank[r].v = g->u.rank[r].v + g->u.rank[r].n;
			g->u.rank[r].n = 0;
		}
	}
}

static int mincross_clust(graph_t *par, graph_t *g)
{
	int		c,nc;

	expand_cluster(g);
	ordered_edges(g);
	flat_breakcycles(g);
	flat_reorder(g);
	nc = mincross(g,2,2);
	
	for (c = 1; c <= g->u.n_cluster; c++)
		nc += mincross_clust(g,g->u.clust[c]);

	save_vlist(g);
	return nc;
}

static int mincross(graph_t *g, int startpass, int endpass)
{
	int		maxthispass,iter,trying,pass;
	int		cur_cross,best_cross;

	if (startpass > 1) {
		cur_cross = best_cross = ncross(g);
		save_best(g);
	}
	else cur_cross = best_cross = MAXINT;
	for (pass = startpass; pass <= endpass; pass++) {
		if (pass <= 1) {
			maxthispass = MIN(4,MaxIter);
			if (g == g->root) build_ranks(g,pass);
			if (pass == 0) flat_breakcycles(g);
			flat_reorder(g);

			if ((cur_cross = ncross(g)) <= best_cross) {
				save_best(g);
				best_cross = cur_cross;
			}
			trying = 0;
		}
		else {
			maxthispass = MaxIter;
			if (cur_cross > best_cross) restore_best(g);
			cur_cross = best_cross;
		}
		trying = 0;
		for (iter = 0; iter < maxthispass; iter++) {
			if (Verbose) fprintf(stderr,"mincross: pass %d iter %d trying %d cur_cross %d best_cross %d\n",pass,iter,trying,cur_cross,best_cross);
			if (trying++ >= MinQuit) break;
			if (cur_cross == 0) break;
			mincross_step(g,iter);
			if ((cur_cross = ncross(g)) <= best_cross) {
				save_best(g);
				if (cur_cross < Convergence*best_cross) trying = 0;
				best_cross = cur_cross;
			}
		}
		if (cur_cross == 0) break;
	}
	if (cur_cross > best_cross) restore_best(g);
	if (best_cross > 0) {transpose(g,FALSE); best_cross= ncross(g);}
	return best_cross;
}

static void restore_best(graph_t* g)
{
	node_t	*n;
	int		r;
	
	for (n = g->u.nlist; n; n = n->u.next) n->u.order = saveorder(n);
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		g->u.rank[r].valid = FALSE;
		qsort(g->u.rank[r].v,g->u.rank[r].n,sizeof(g->u.rank[0].v[0]),(qsort_cmpf)nodeposcmpf);
	}
}

static void save_best(graph_t* g)
{
	node_t	*n;
	for (n = g->u.nlist; n; n = n->u.next) saveorder(n) = n->u.order;
}

/* merge connected components, create globally consistent rank lists */
static void merge2(graph_t* g)
{
	int		i,r;
	node_t	*v;

	/* merge the components and rank limits */
	merge_components(g);

	/* install complete ranks */
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		g->u.rank[r].n = g->u.rank[r].an;
		g->u.rank[r].v = g->u.rank[r].av;
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];
			if (v == NULL) {
				if (Verbose) fprintf(stderr,"merge2: graph %s, rank %d has only %d < %d nodes\n",g->name,r,i,g->u.rank[r].n);
					g->u.rank[r].n = i;
					break;
			}
			v->u.order = i;
		}
	}
}

static void cleanup2(graph_t* g, int nc)
{
	int		i,j,r,c;
	node_t	*v;
	edge_t	*e;

	free(TI_list); free(TE_list);
	/* fix vlists of clusters */
	for (c = 1; c <= g->u.n_cluster; c++)
		rec_reset_vlists(g->u.clust[c]);

	/* remove node temporary edges for ordering nodes */
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];
			v->u.order = i;
			if (v->u.flat_out.list) {
				for (j = 0; (e = v->u.flat_out.list[j]); j++)
				if (e->u.edge_type == FLATORDER) {
					delete_flat_edge(e);
					free(e);
					j--;
				}
			}
		}
		free_matrix(g->u.rank[r].flat);
	}
	if (Verbose) fprintf(stderr,"mincross %s: %d crossings, %.2f secs.\n",
		g->name,nc,elapsed_sec());
}

static node_t* neighbor(node_t* v, int dir)
{
	node_t		*rv;

	rv = NULL;
	if (dir < 0) {
		if (v->u.order > 0) rv = Root->u.rank[v->u.rank].v[v->u.order - 1];
	}
	else rv = Root->u.rank[v->u.rank].v[v->u.order + 1];
	return rv;
}

int inside_cluster(graph_t* g, node_t* v)
{
	return (is_a_normal_node_of(g,v) | is_a_vnode_of_an_edge_of(g,v));
}

int is_a_normal_node_of(graph_t* g, node_t* v)
{
	return ((v->u.node_type == NORMAL) && agcontains(g,v));
}

int is_a_vnode_of_an_edge_of(graph_t* g, node_t* v)
{
	if ((v->u.node_type == VIRTUAL)
		&& (v->u.in.size == 1) && (v->u.out.size == 1) )  {
			edge_t	*e = v->u.out.list[0];
			while (e->u.edge_type != NORMAL) e = e->u.to_orig;
			if (agcontains(g,e)) return TRUE;
	}
	return FALSE;
}

static node_t	*furthestnode(graph_t* g, node_t* v, int dir)
{
	node_t	*u,*rv;

	rv = u = v;
	while ((u = neighbor(u,dir))) {
		if (is_a_normal_node_of(g,u)) rv = u;
		else if (is_a_vnode_of_an_edge_of(g,u)) rv = u;
	}
	return rv;
}

void save_vlist(graph_t* g)
{
	int		r;

	if (g->u.rankleader)
		for (r = g->u.minrank; r <= g->u.maxrank; r++) {
			g->u.rankleader[r] = g->u.rank[r].v[0];
		}
}

void rec_save_vlists(graph_t* g)
{
	int		c;

	save_vlist(g);
	for (c = 1; c <= g->u.n_cluster; c++)
		rec_save_vlists(g->u.clust[c]);
}


void rec_reset_vlists(graph_t* g)
{
	int		r,c;
	node_t	*u,*v,*w;

	/* fix vlists of sub-clusters */
	for (c = 1; c <= g->u.n_cluster; c++)
		rec_reset_vlists(g->u.clust[c]);
	
	if (g->u.rankleader)
		for (r = g->u.minrank; r <= g->u.maxrank; r++) {
			v = g->u.rankleader[r];
#ifdef DEBUG
			assert(node_in_root_vlist(v));
#endif
			u = furthestnode(g,v,-1);
			w = furthestnode(g,v, 1);
			g->u.rankleader[r] = u;
#ifdef DEBUG
			assert(g->root->u.rank[r].v[u->u.order] == u);
#endif
			g->u.rank[r].v = g->root->u.rank[r].v + u->u.order;
			g->u.rank[r].n = w->u.order - u->u.order + 1;
		}
}

static void init_mincross(graph_t* g)
{
	if (Verbose) start_timer();

	ReMincross = FALSE;
	Root = g;
	TE_list = N_NEW(agnedges(g->root),edge_t*);
	TI_list = N_NEW(agnedges(g->root),int);

	mincross_options(g);
	class2(g);
	decompose(g,1);
	allocate_ranks(g);
	ordered_edges(g);
	GlobalMinRank = g->u.minrank; GlobalMaxRank = g->u.maxrank;
}

static void flat_search(graph_t* g, node_t* v)
{
	int		i,j;
	boolean hascl;
	edge_t	*e,*rev;
	adjmatrix_t	*M = g->u.rank[v->u.rank].flat;

	v->u.mark = TRUE;
	v->u.onstack = TRUE;
	hascl = (g->root->u.n_cluster > 0);
	if (v->u.flat_out.list) for (i = 0; (e = v->u.flat_out.list[i]); i++) {
		if (hascl && NOT(agcontains(g,e->tail) && agcontains(g,e->head)))
			continue;
		if (e->u.weight == 0) continue;
		if (e->head->u.onstack == TRUE)  {
			assert(flatindex(e->head) < M->nrows);
			assert(flatindex(e->tail) < M->ncols);
			ELT(M,flatindex(e->head),flatindex(e->tail)) = 1;
			delete_flat_edge(e);
			i--;
			if (e->u.edge_type == FLATORDER) continue;
			for (j = 0; (rev = e->head->u.flat_out.list[j]); j++)
				if (rev->head == e->tail) break;
			if (rev) {
				merge_oneway(e,rev);
				elist_append(e,e->tail->u.other);
			}
			else {
				rev = new_virtual_edge(e->head,e->tail,e);
				rev->u.edge_type = REVERSED;
				flat_edge(g,rev);
			}
		}
		else {
			assert(flatindex(e->head) < M->nrows);
			assert(flatindex(e->tail) < M->ncols);
			ELT(M,flatindex(e->tail),flatindex(e->head)) = 1;
			if (e->head->u.mark == FALSE) flat_search(g,e->head);
		}
	}
	v->u.onstack = FALSE;
}

static void flat_breakcycles(graph_t* g)
{
	int		i,r,flat;
	node_t	*v;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		flat = 0;
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];
			v->u.mark = v->u.onstack = FALSE;
			flatindex(v) = i;
			if ((v->u.flat_out.size > 0) && (flat == 0)) {
				g->u.rank[r].flat = new_matrix(g->u.rank[r].n,g->u.rank[r].n);
				flat = 1;
			}
		}
		if (flat) {
			for (i = 0; i < g->u.rank[r].n; i++) {
				v = g->u.rank[r].v[i];
				if (v->u.mark == FALSE) flat_search(g,v);
			}
		}
	}
}

int left2right(graph_t *g, node_t *v, node_t *w)
{
	adjmatrix_t	*M;
	int			rv;

	/* CLUSTER indicates orig nodes of clusters, and vnodes of skeletons */
	if (ReMincross == FALSE) {
		if ((v->u.clust != w->u.clust) && (v->u.clust) && (w->u.clust)) {
			return ((v->u.ranktype != CLUSTER) && (w->u.ranktype != CLUSTER));
		}
	}
	else {
		if ((v->u.clust) != (w->u.clust)) return TRUE;
	}
	M = g->u.rank[v->u.rank].flat;
	if (M == NULL) rv = FALSE;
	else {
		if (g->u.left_to_right) {node_t *t = v; v = w; w = t;}
		rv = ELT(M,flatindex(v),flatindex(w));
	}
	return rv;
}

static void clust_count_ranks(graph_t* g, int* count)
{
	node_t		*n;
	edge_t		*e;
	int			low,high,j;
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		assert (n->u.UF_size > 0);
		count[n->u.rank] += n->u.UF_size;
		for (e = agfstout(g->root,n); e; e = agnxtout(g->root,e)) {
			low = e->tail->u.rank; high = e->head->u.rank;
			if (low > high) {int t = low; low = high; high = t;}
			assert(low <= high);
			for (j = low + 1; j <= high - 1; j++) count[j] += e->u.xpenalty;
		}
	}
}

/* should combine with previous, just need to count rankleaders */
static void count_ranks(graph_t* g, int** c0)
{
	static int	*count;
	int			c;
	node_t		*n;
	edge_t		*e;
	int			low,high,i,j;

	count = ALLOC(Root->u.maxrank+1,count,int);
	for (c = 0; c <= g->u.maxrank; c++) count[c] = 0;

	for (c = 0; c < g->u.comp.size; c++) {
		for (n = g->u.comp.list[c]; n; n = n->u.next) {
			assert (n->u.UF_size > 0);
			count[n->u.rank] += n->u.UF_size;
			for (i = 0; (e = n->u.out.list[i]); i++) {
				low = e->tail->u.rank; high = e->head->u.rank;
				assert(low < high);
				for (j = low + 1; j <= high - 1; j++) count[j] += e->u.xpenalty;
			}
		}
	}
	for (c = 1; c <= g->u.n_cluster; c++)
		clust_count_ranks(g->u.clust[c],count);
	*c0 = count;
}

/* allocates ranks, with enough space for all nodes expanded */
void allocate_ranks(graph_t* g)
{
	int		r,*cn;

	count_ranks(g,&cn);
	g->u.rank = N_NEW(g->u.maxrank+2,rank_t);
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		g->u.rank[r].an = g->u.rank[r].n = cn[r];
		g->u.rank[r].av = g->u.rank[r].v = N_NEW(cn[r]+1,node_t*);
	}
}

/* install a node at the current right end of its rank */
void install_in_rank(graph_t* g, node_t* n)
{
	int		i,r;

	r = n->u.rank;
	i = g->u.rank[r].n;
	if (g->u.rank[r].an <= 0) {
		fprintf(stderr,"install_in_rank %s %s rank %d i = %d an = 0\n",
			g->name,n->name,r,i);
		abort();
	}

	g->u.rank[r].v[i] = n;
	n->u.order = i;
	g->u.rank[r].n++;
	assert(g->u.rank[r].n <= g->u.rank[r].an);
#ifdef DEBUG
	{
		node_t		*v;

		for (v = g->u.nlist; v; v = v->u.next)
			if (v == n) break;
		assert(v != NULL);
	}
#endif
	if (n->u.order > Root->u.rank[r].an) abort();
	if ((r < g->u.minrank) || (r > g->u.maxrank)) abort();
	if (g->u.rank[r].v + n->u.order > g->u.rank[r].av + Root->u.rank[r].an) abort();
}

/*	install nodes in ranks. the initial ordering ensure that series-parallel
 *	graphs such as trees are drawn with no crossings.  it tries searching
 *	in- and out-edges and takes the better of the two initial orderings.
 */
void build_ranks(graph_t* g, int pass)
{
	int		i,j;
	node_t	*n,*n0;
	edge_t	**otheredges;
	queue	*q;
	

	q = new_queue(g->u.n_nodes);
	for (n = g->u.nlist; n; n = n->u.next) MARK(n) = FALSE;

#ifdef DEBUG
	{
	edge_t	*e;
	for (n = g->u.nlist; n; n = n->u.next) {
		for (i = 0; (e = n->u.out.list[i]); i++) 
			assert(MARK(e->head) == FALSE);
		for (i = 0; (e = n->u.in.list[i]); i++) 
			assert(MARK(e->tail) == FALSE);
	}
	}
#endif

	for (i = g->u.minrank; i <= g->u.maxrank; i++) g->u.rank[i].n = 0;

	for (n = g->u.nlist; n; n = n->u.next) {
		otheredges = ((pass == 0)? n->u.in.list : n->u.out.list);
		if (otheredges[0] != NULL) continue;
		if (MARK(n) == FALSE) {
			MARK(n) = TRUE;
			enqueue(q,n);
			while ((n0 = dequeue(q))) {
				if (n0->u.ranktype != CLUSTER) {
					install_in_rank(g,n0);
					enqueue_neighbors(q,n0,pass);
				}
				else {
					install_cluster(g,n0,pass,q);
				}
			}
		}
	}
	if (dequeue(q)) fprintf(stderr,"surprise\n");
	for (i = g->u.minrank; i <= g->u.maxrank; i++) {
		g->u.rank[i].valid = FALSE;
		if (g->u.left_to_right && (g->u.rank[i].n > 0)) {
			int		n,ndiv2;
			node_t	**vlist = g->u.rank[i].v;
			n = g->u.rank[i].n - 1;  ndiv2 = n / 2;
			for (j = 0; j <= ndiv2; j++)
				exchange(vlist[j],vlist[n - j]);
		}
	}

	if ((g == g->root) && ncross(g) > 0) transpose(g,FALSE);
	free_queue(q);
}

void enqueue_neighbors(queue* q, node_t* n0, int pass)
{
	int		i;
	edge_t	*e;

	if (pass == 0) {
		for (i = 0; i < n0->u.out.size ; i++) {
			e = n0->u.out.list[i];
			if ((MARK(e->head)) == FALSE) {
				MARK(e->head) = TRUE;
				enqueue(q,e->head);
			}
		}
	}
	else {
		for (i = 0; i < n0->u.in.size ; i++) {
			e = n0->u.in.list[i];
			if ((MARK(e->tail)) == FALSE) {
				MARK(e->tail) = TRUE;
				enqueue(q,e->tail);
			}
		}
	}
}

/* construct nodes reachable from 'here' in post-order.
 * This is the same as doing a topological sort in reverse order.
 */
static int postorder(graph_t *g, node_t *v, node_t **list)
{
    edge_t	*e;
	int		i,cnt = 0;

	MARK(v) = TRUE;
	if (v->u.flat_out.size > 0) {
		for (i = 0; (e = v->u.flat_out.list[i]); i++) {
			if ((e->head->u.node_type == NORMAL) &
				(NOT(agcontains(g,e->head)))) continue;
			if (e->head->u.clust != e->tail->u.clust) continue;

			if (MARK(e->head) == FALSE)
				cnt += postorder(g,e->head,list+cnt);
		}
	}
    list[cnt++] = v;
	return cnt;
}

static void flat_reorder(graph_t* g)
{
	int		i,j,r,pos,n_search,local_in_cnt, local_out_cnt;
	node_t	*v,**left,**right,*t;
	node_t	**temprank = NULL;
	edge_t	*flat_e;

	if (g->u.has_flat_edges == FALSE) return;
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		for (i = 0; i < g->u.rank[r].n; i++) MARK(g->u.rank[r].v[i]) = FALSE;
		temprank = ALLOC(i+1,temprank,node_t*);
		pos = 0;
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];

			local_in_cnt = local_out_cnt = 0;
			for (j = 0; j < v->u.flat_in.size; j++) {
				flat_e = v->u.flat_in.list[j];
				if (inside_cluster(g,flat_e->tail)) local_in_cnt++;
			}
			for (j = 0; j < v->u.flat_out.size; j++) {
				flat_e = v->u.flat_out.list[j];
				if (inside_cluster(g,flat_e->head)) local_out_cnt++;
			}
			if ((local_in_cnt == 0) && (local_out_cnt == 0))
				temprank[pos++] = v;
			else {
				if ((MARK(v) == FALSE) && (local_in_cnt == 0)) {
					left = temprank + pos;
					n_search = postorder(g,v,left);
					if (g->u.left_to_right == FALSE) {
						right = left + n_search - 1;
						while (left < right) {
							t = *left; *left = *right; *right = t;
							left++; right--;
						}
					}
					pos += n_search;
				}
			}
		}
		for (i = 0; i < g->u.rank[r].n; i++){
			v = g->u.rank[r].v[i] = temprank[i];
			v->u.order = i + (g->u.rank[r].v - Root->u.rank[r].v);
		}
		g->u.rank[r].valid = FALSE;
	}
	if (temprank) free(temprank);
}

static void mincross_step(graph_t* g, int pass)
{
	int		r,other,first,last,dir;
	int		hasfixed,reverse;

	if ((pass % 4) < 2) reverse = TRUE; else reverse = FALSE;
	if (pass % 2) 	{ r = g->u.maxrank - 1; dir = -1; }	 /* up pass */
	else			{ r = 1; dir = 1; }				/* down pass */

	if (pass % 2 == 0) {		/* down pass */
		first = g->u.minrank + 1;
		if (g->u.minrank > Root->u.minrank) first--;
		last = g->u.maxrank;
		dir = 1;
	}
	else {						/* up pass */
		first = g->u.maxrank - 1;
		last = g->u.minrank;
		if (g->u.maxrank < Root->u.maxrank) first++;
		dir = -1;
	}

	for (r = first; r != last + dir; r += dir) {
		other = r - dir;
		hasfixed = medians(g,r,other);
		reorder(g,r,reverse,hasfixed);
	}
	transpose(g,NOT(reverse));
}

void transpose(graph_t* g, int reverse)
{
	int		r,delta;

	for (r = g->u.minrank; r <= g->u.maxrank; r++)
		g->u.rank[r].candidate = TRUE;
	do {
		delta = 0;
#ifdef NOTDEF
		for (r = g->u.maxrank; r >= g->u.minrank; r--)
			if (g->u.rank[r].candidate) delta += transpose_step(g,r,reverse);
#endif
		for (r = g->u.minrank; r <= g->u.maxrank; r++)
			if (g->u.rank[r].candidate) delta += transpose_step(g,r,reverse);
	/*} while (delta > ncross(g)*(1.0 - Convergence));*/
	} while (delta >= 1);
}

int local_cross(elist l, int dir)
{
	int		i,j,is_out;
	int		cross = 0;
	edge_t	*e,*f;
	if (dir > 0) is_out = TRUE; else is_out = FALSE;
	for (i = 0; (e = l.list[i]); i++) {
		if (is_out) for (j = i+1; (f = l.list[j]); j++)  {
			if ((f->head->u.order - e->head->u.order) * (f->u.tail_port.p.x - e->u.tail_port.p.x) < 0)
					cross += e->u.xpenalty * f->u.xpenalty;
		}
		else for (j = i+1; (f = l.list[j]); j++)  {
			if ((f->tail->u.order - e->tail->u.order) * (f->u.head_port.p.x - e->u.head_port.p.x) < 0)
					cross += e->u.xpenalty * f->u.xpenalty;
		}
	}
	return cross;
}

int rcross(graph_t* g, int r)
{
	static int *Count,C;
	int		top,bot,cross,max,i,k;
	node_t	**rtop,*v;

	cross = 0;
	max = 0;
	rtop = g->u.rank[r].v;

	if (C <= Root->u.rank[r+1].n) {
		C = Root->u.rank[r+1].n + 1;
		Count = ALLOC(C,Count,int);
	}

	for (i = 0; i < g->u.rank[r+1].n; i++) Count[i] = 0;

	for (top = 0; top < g->u.rank[r].n; top++) {
		register edge_t	*e;
		if (max > 0) {
			for (i = 0; (e = rtop[top]->u.out.list[i]); i++) {
				for (k = e->head->u.order + 1; k <= max; k++)
					cross += Count[k] * e->u.xpenalty;
			}
		}
		for (i = 0; (e = rtop[top]->u.out.list[i]); i++) {
			register int	inv = e->head->u.order;
			if (inv > max) max = inv;
			Count[inv] += e->u.xpenalty;
		}
	}
	for (top = 0; top < g->u.rank[r].n; top++) {
		v = g->u.rank[r].v[top];
		if (v->u.has_port) cross += local_cross(v->u.out,1);
	}
	for (bot = 0; bot < g->u.rank[r+1].n; bot++) {
		v = g->u.rank[r+1].v[bot];
		if (v->u.has_port) cross += local_cross(v->u.in,-1);
	}
	return cross;
}

int ncross(graph_t* g)
{
	int		r,count,nc;

	g = Root;
	count = 0;
	for (r = g->u.minrank; r < g->u.maxrank; r++) {
		if (g->u.rank[r].valid) count += g->u.rank[r].cache_nc;
		else
		{
			nc = g->u.rank[r].cache_nc = rcross(g,r);
			count += nc;
			g->u.rank[r].valid = TRUE;
		}
	}
	return count;
}

int ordercmpf(int *i0, int *i1)
{
	return (*i0) - (*i1);
}

int out_cross(node_t *v, node_t *w)
{
	register edge_t	**e1,**e2;
	register int	inv, cross = 0,t;

	for (e2 = w->u.out.list; *e2; e2++) {
        register int cnt = (*e2)->u.xpenalty;
		inv = ((*e2)->head)->u.order;

        for (e1 = v->u.out.list; *e1; e1++) {
			t = ((*e1)->head)->u.order - inv;
			if ((t > 0)
				|| ((t == 0) && ((*e1)->u.head_port.p.x > (*e2)->u.head_port.p.x)))
					cross += (*e1)->u.xpenalty * cnt;
		}
    }
    return cross;

}

int in_cross(node_t *v,node_t *w)
{
	register edge_t	**e1,**e2;
	register int	inv, cross = 0, t;

	for (e2 = w->u.in.list; *e2; e2++) {
        register int cnt = (*e2)->u.xpenalty;
		inv = ((*e2)->tail)->u.order;

        for (e1 = v->u.in.list; *e1; e1++) {
			t = ((*e1)->tail)->u.order - inv;
			if ((t > 0)
				|| ((t == 0) && ((*e1)->u.tail_port.p.x > (*e2)->u.tail_port.p.x)))
					cross += (*e1)->u.xpenalty * cnt;
		}
    }
    return cross;
}

#define VAL(node,port) (MC_SCALE * (node)->u.order + (port).order)

static boolean medians(graph_t *g, int r0, int r1)
{
	int		i,j,lm,rm,lspan,rspan,*list;
	node_t	*n,**v;
	edge_t	*e;
	boolean	hasfixed = FALSE;

	list = TI_list;
	v = g->u.rank[r0].v;
	for (i = 0; i < g->u.rank[r0].n; i++) {
		n = v[i];
		if (r1 > r0) for (j = 0; (e = n->u.out.list[j]); j++)
			list[j] = VAL(e->head,e->u.head_port);
		else for (j = 0; (e = n->u.in.list[j]); j++)
			list[j] = VAL(e->tail,e->u.tail_port);
		switch(j) {
			case 0:
				n->u.mval = -1;
				break;
			case 1:
				n->u.mval = list[0];
				break;
			case 2:
				n->u.mval = (list[0] + list[1])/2;
				break;
			default:
				qsort(list,j,sizeof(int),(qsort_cmpf)ordercmpf);
				if (j % 2) n->u.mval = list[j/2];
				else {
					/* weighted median */
					rm = j/2;
					lm = rm - 1;
					rspan = list[j-1] - list[rm];
					lspan = list[lm] - list[0];
					if (lspan == rspan)
						n->u.mval = (list[lm] + list[rm])/2;
					else {
						int w = list[lm]*rspan + list[rm]*lspan;
						n->u.mval = w / (lspan + rspan);
					}
				}
		}
	}
	for (i = 0; i < g->u.rank[r0].n; i++) {
		n = v[i];
		if ((n->u.out.size == 0) && (n->u.in.size == 0))
			hasfixed |= flat_mval(n);
	}
	return hasfixed;
}

int transpose_step(graph_t* g, int r, int reverse)
{
	int		i,c0,c1,rv;
	node_t	*v,*w;

	rv = 0;
	g->u.rank[r].candidate = FALSE;
	for (i = 0; i < g->u.rank[r].n - 1; i++) {
		v = g->u.rank[r].v[i];
		w = g->u.rank[r].v[i+1];
assert (v->u.order < w->u.order);
		if (left2right(g,v,w)) continue;
		c0 = c1 = 0;
		if (r > 0) {
			c0 += in_cross(v,w);
			c1 += in_cross(w,v);
		}
		if (g->u.rank[r+1].n > 0) {
			c0 += out_cross(v,w);
			c1 += out_cross(w,v);
		}
		if ((c1 < c0) || ((c0 > 0) && reverse && (c1 == c0))) {
			exchange(v,w);
			rv += (c0 - c1);
			g->u.rank[r].valid = FALSE;
			g->u.rank[r].candidate = TRUE;

			if (r > g->u.minrank) {
				g->u.rank[r-1].valid = FALSE;
				g->u.rank[r-1].candidate = TRUE;
			}
			if (r < g->u.maxrank) {
				g->u.rank[r+1].candidate = TRUE;
				g->u.rank[r].valid = FALSE;
			}
		}
	}
	return rv;
}

void exchange(node_t *v, node_t *w)
{
	int		vi,wi,r;
		
	r = v->u.rank;
	vi = v->u.order;
	wi = w->u.order;
	v->u.order = wi;
	Root->u.rank[r].v[wi] = v;
	w->u.order = vi;
	Root->u.rank[r].v[vi] = w;
}

void reorder(graph_t *g, int r, int reverse, int hasfixed)
{
	int		changed = 0, nelt;
	boolean	muststay,sawclust;
	node_t	**vlist = g->u.rank[r].v;
	node_t	**lp,**rp,**ep = vlist + g->u.rank[r].n;

	for (nelt = g->u.rank[r].n - 1; nelt >= 0; nelt--) {
		lp = vlist;
		while (lp  < ep) {
			/* find leftmost node that can be compared */
			while ((lp < ep) && ((*lp)->u.mval < 0)) lp++;
			if (lp >= ep) break;
			/* find the node that can be compared */
			sawclust = muststay = FALSE;
			for (rp = lp + 1; rp < ep; rp++) {
				if (sawclust && (*rp)->u.clust) continue;	/* ### */
				if (left2right(g,*lp,*rp)) { muststay = TRUE; break; }
				if ((*rp)->u.mval >= 0) break;
				if ((*rp)->u.clust) sawclust = TRUE; /* ### */
			}
			if (rp >= ep) break;
			if (muststay == FALSE) {
				register int	p1 = ((*lp)->u.mval);
				register int	p2 = ((*rp)->u.mval);
				if ((p1 > p2) || ((p1 == p2) && (reverse))) {
					exchange(*lp,*rp);
					changed++;
				}
			}
			lp = rp;
		}
		if ((hasfixed == FALSE) && (reverse == FALSE)) ep--;
	}

	if (changed) {
		g->u.rank[r].valid = FALSE;
		if (r > 0) g->u.rank[r-1].valid = FALSE;
	}
}

static int nodeposcmpf(node_t **n0, node_t **n1)
{
	return ((*n0)->u.order - (*n1)->u.order);
}

static int edgeidcmpf(edge_t **e0, edge_t **e1)
{
	return ((*e0)->id - (*e1)->id);
}

/* following code deals with weights of edges of "virtual" nodes */
#define ORDINARY	0
#define SINGLETON	1
#define	VIRTUALNODE	2
#define NTYPES		3

#define C_EE		1
#define C_VS		2
#define C_SS		2
#define C_VV		4

static int table[NTYPES][NTYPES] = {
	/* ordinary */		{C_EE, C_EE, C_EE},
	/* singleton */		{C_EE, C_SS, C_VS},
	/* virtual */		{C_EE, C_VS, C_VV}
};

static int endpoint_class(node_t* n)
{
	if (n->u.node_type == VIRTUAL) return VIRTUALNODE;
	if (n->u.weight_class <= 1) return SINGLETON;
	return ORDINARY;
}

void virtual_weight(edge_t* e)
{
	int t;
	t = table[endpoint_class(e->tail)][endpoint_class(e->head)];
	e->u.weight *= t;
}

void ordered_edges(graph_t* g)
{
	char		*ordering;

	ordering = agget(g,"ordering");
	if (ordering && streq(ordering,"out")) do_ordering(g);
	else {
			/* search meta-graph to find subgraphs that may be ordered */
		graph_t		*mg,*subg;
		node_t		*mm,*mn;
		edge_t		*me;

		mm = g->meta_node;
		mg = mm->graph;
		for (me = agfstout(mg,mm); me; me = agnxtout(mg,me)) {
			mn = me->head;
			subg = agusergraph(mn);
				/* clusters are processed by seperate calls to ordered_edges */
			if (!is_cluster(subg)) ordered_edges(subg);
		}
	}
}

/* creates flat edges for ordered edges of g.
 * follows virtual edge chains where necessary.
 */
void do_ordering(graph_t* g)
{
	int			i,j,ri,ne,ei,nranks;
	node_t		*n,**lpath,**rpath,*u,*v;
	edge_t		*e,*f,*le,*re,**sortlist;

	sortlist = TE_list;
	lpath = rpath = NULL;

	nranks = g->root->u.maxrank - g->root->u.minrank + 2;
	lpath = N_NEW(nranks,node_t*); rpath = N_NEW(nranks,node_t*);

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		if (n->u.clust) continue;
		le = NULL;

			/* make a list of the forward edges */
		for (ne = 0, e = agfstout(g,n); e; e = agnxtout(g,e)) {
			if (e->head->u.clust) continue;
			if (e->head->u.rank > e->tail->u.rank) sortlist[ne++] = e;
		}

		if (ne == 0) continue;
		qsort(sortlist,ne,sizeof(sortlist[0]),(qsort_cmpf)edgeidcmpf);
		for (ei = 0; ei < ne; ei++) {
			e = sortlist[ei];
			for (re = e; re->u.to_virt; re = re->u.to_virt);

			/* get path of right edge */
			ri = 0;
			while (ri < nranks) {
				rpath[ri++] = re->head;
				if (re->head->u.node_type == NORMAL) break;
				if (re->head->u.out.size != 1) break;
				re = re->head->u.out.list[0];
			}
			rpath[ri] = NULL;

			if (le) {
				for (i = 0; i < ri; i++) {
					if (lpath[i] == NULL) break;
					u = lpath[i]; v = rpath[i];
					f = NULL;
					if ((u->u.node_type == NORMAL)
						&& (v->u.node_type == NORMAL)) {
							for (j = 0; (f = u->u.flat_out.list[j]); j++)
								if (f->head == v) break;
					}
					if (f != NULL) break;
					f = new_virtual_edge(UF_find(lpath[i]),rpath[i],NULL);
					f->u.edge_type = FLATORDER;
					flat_edge(g,f);
				}
			}

			le = re;
			for (i = 0; i <= ri; i++) lpath[i] = rpath[i];
		}
	}
	if (lpath) { free(lpath); free(rpath); }
}

/* merges the connected components of g */
void merge_components(graph_t* g)
{
	int		c;
	node_t	*u,*v;

	if (g->u.comp.size <= 1) return;
	u = NULL;
	for (c = 0; c < g->u.comp.size; c++) {
		v = g->u.comp.list[c];
		if (u) u->u.next = v;
		v->u.prev = u;
		while (v->u.next) {
			v = v->u.next;
		}
		u = v;
	}
	g->u.comp.size = 1;
	g->u.nlist = g->u.comp.list[0];
	g->u.minrank = GlobalMinRank;
	g->u.maxrank = GlobalMaxRank;
}

#ifdef DEBUG
void check_rs(graph_t* g, int null_ok)
{
	int		i,r;
	node_t	*v,*prev;

fprintf(stderr,"\n\n%s:\n",g->name);
	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
fprintf(stderr,"%d: ",r);
		prev = NULL;
		for (i = 0; i < g->u.rank[r].n; i++) {
			v = g->u.rank[r].v[i];
			if (v == NULL) {
fprintf(stderr,"NULL\t");
				if(null_ok==FALSE)abort();
			}
			else {
fprintf(stderr,"%s\t",v->name);
				assert(v->u.rank == r);
				assert(v != prev);
				prev = v;
			}
		}
fprintf(stderr,"\n");
	}
}

void check_order(void)
{
	int		i,r;
	node_t	*v;
	graph_t	*g = Root;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		assert(g->u.rank[r].v[g->u.rank[r].n] == NULL);
		for (i = 0; v = g->u.rank[r].v[i]; i++)
			assert (v->u.order == i);
	}
}
#endif

static void mincross_options(graph_t* g)
{
	char	*p;
	double	f;

	p = agget(g,"mclimit");
	if (p && ((f = atof(p)) > 0.0)) {
		MinQuit = MAX(1,MinQuit * f);
		MaxIter = MAX(1,MaxIter * f);
	}
}

int flat_mval(node_t* n)
{
	int		i;
	edge_t	*e,**fl;
	node_t	*nn;

	if ((n->u.in.size == 0) && (n->u.out.size == 0)) {
		if (n->u.flat_in.size > 0) {
			fl = n->u.flat_in.list;
			nn = fl[0]->tail;
			for (i = 1; (e = fl[i]); i++)
				if (e->tail->u.order > nn->u.order) nn = e->tail;
			n->u.mval = nn->u.mval + 1;
			return FALSE;
		}
		else if (n->u.flat_out.size > 0) {
			fl = n->u.flat_out.list;
			nn = fl[0]->head;
			for (i = 1; (e = fl[i]); i++)
				if (e->head->u.order < nn->u.order) nn = e->head;
			n->u.mval = nn->u.mval - 1;
			return FALSE;
		}
	}
	return TRUE;
}

#ifdef DEBUG
void check_exchange(node_t *v, node_t *w)
{
	int		i,r;
	node_t	*u;

	if ((v->u.clust == NULL) && (w->u.clust == NULL)) return;
	assert ((v->u.clust == NULL) || (w->u.clust == NULL));
	assert(v->u.rank == w->u.rank);
	assert(v->u.order < w->u.order);
	r = v->u.rank;

	for (i = v->u.order + 1; i < w->u.order; i++) {
		u = v->graph->u.rank[r].v[i];
		if (u->u.clust) abort();
	}
}

void check_vlists(graph_t* g)
{
	int		c,i,j,r;
	node_t	*u;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		for (i = 0; i < g->u.rank[r].n; i++) {
			u = g->u.rank[r].v[i];
			j = u->u.order;
			assert (Root->u.rank[r].v[j] == u);
		}
		if (g->u.rankleader) {
			u = g->u.rankleader[r];
			j = u->u.order;
			assert (Root->u.rank[r].v[j] == u);
		}
	}
	for (c = 1; c <= g->u.n_cluster; c++)
		check_vlists(g->u.clust[c]);
}

void node_in_root_vlist(node_t* n)
{
	node_t	**vptr;

	for (vptr = Root->u.rank[n->u.rank].v; *vptr; vptr++)
		if (*vptr == n) break;
	if (*vptr == 0) abort();
}
#endif	/* DEBUG code */
