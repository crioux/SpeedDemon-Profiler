/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include    "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void dot_nodesize(node_t* n, boolean flip)
{
    float       x,y;
    int         ps;

    if (flip == FALSE) {
               x = n->u.width;
               y = n->u.height;
       }
    else {
               y = n->u.width;
               x = n->u.height;
       }
    ps = POINTS(x)/2;
    if (ps < 1)
               ps = 1;
    n->u.lw = n->u.rw = ps;
    n->u.ht = POINTS(y);
}

void dot_init_node(node_t* n)
{
	char	*str;

	n->u.width = late_float(n,N_width,DEFAULT_NODEWIDTH,MIN_NODEWIDTH);
	n->u.height = late_float(n,N_height,DEFAULT_NODEHEIGHT,MIN_NODEHEIGHT);
	if (N_label == NULL) str = n->name;
	else {
		str = agxget(n,N_label->index);
		if (strcmp(str,NODENAME_ESC) == 0) str = n->name;
	}
	n->u.label = make_label(str,
		late_float(n,N_fontsize,DEFAULT_FONTSIZE,MIN_FONTSIZE),
		late_nnstring(n,N_fontname,DEFAULT_FONTNAME), 
		late_nnstring(n,N_fontcolor,DEFAULT_COLOR), n->graph);
	n->u.shape = bind_shape(late_nnstring(n,N_shape,DEFAULT_NODESHAPE));
	n->u.showboxes = late_int(n,N_showboxes,0,0);
	n->u.shape->initfn(n);
	dot_nodesize(n,n->graph->u.left_to_right);
	alloc_elist(4,n->u.in);	alloc_elist(4,n->u.out);
	alloc_elist(2,n->u.flat_in); alloc_elist(2,n->u.flat_out);
	alloc_elist(2,n->u.other);
	n->u.UF_size = 1;
}

void dot_init_edge(edge_t* e)
{
	char	*p;
	char	*tailgroup,*headgroup;

	e->u.weight = (int)late_float(e,E_weight,1.0,0.0);
	tailgroup = late_string(e->tail,N_group,"");
	headgroup = late_string(e->head,N_group,"");
	e->u.count = e->u.xpenalty = 1;
	if (tailgroup[0] && (tailgroup == headgroup)) {
		e->u.xpenalty = CL_CROSS;  e->u.weight *= 100;
	}
	if (nonconstraint_edge(e)) {
		e->u.xpenalty = 0;
		e->u.weight = 0;
	}

	if (E_label && (p = agxget(e,E_label->index)) && (p[0])) {
		e->u.label = make_label(agxget(e,E_label->index),
			late_float(e,E_fontsize,DEFAULT_FONTSIZE,MIN_FONTSIZE),
			late_nnstring(e,E_fontname,DEFAULT_FONTNAME),
			late_nnstring(e,E_fontcolor,DEFAULT_COLOR),e->tail->graph);
		e->tail->graph->u.has_edge_labels = TRUE;
	}

    /* vladimir */
	if (E_headlabel && (p = agxget(e,E_headlabel->index)) && (p[0])) {
		e->u.head_label = make_label(agxget(e,E_headlabel->index),
			late_float(e,E_labelfontsize,DEFAULT_LABEL_FONTSIZE,MIN_FONTSIZE),
			late_nnstring(e,E_labelfontname,DEFAULT_FONTNAME),
			late_nnstring(e,E_labelfontcolor,DEFAULT_COLOR),e->tail->graph);
	}
	if (E_taillabel && (p = agxget(e,E_taillabel->index)) && (p[0])) {
		e->u.tail_label = make_label(agxget(e,E_taillabel->index),
			late_float(e,E_labelfontsize,DEFAULT_LABEL_FONTSIZE,MIN_FONTSIZE),
			late_nnstring(e,E_labelfontname,DEFAULT_FONTNAME),
			late_nnstring(e,E_labelfontcolor,DEFAULT_COLOR),e->tail->graph);
	}
    /* end vladimir */

	e->u.showboxes = late_int(e,E_showboxes,0,0);
	e->u.minlen = late_int(e,E_minlen,1,0);
	p = agget(e,"tailport");
	if (p[0]) e->tail->u.has_port = TRUE;
	e->u.tail_port = e->tail->u.shape->portfn(e->tail,p);
	p = agget(e,"headport");
	if (p[0]) e->head->u.has_port = TRUE;
	e->u.head_port = e->head->u.shape->portfn(e->head,p);
}

void dot_init_node_edge(graph_t *g)
{
	node_t *n;
	edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g,n)) dot_init_node(n);
    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
        for (e = agfstout(g,n); e; e = agnxtout(g,e)) dot_init_edge(e);
    }
}

static void free_edge_list(elist L)
{
	edge_t	*e;
	int	i;

	for (i=0; i<L.size; i++) {
		e=L.list[i];
		free(e);
	}
}

void dot_cleanup_node(node_t* n)
{
	free_list(n->u.in);
	free_list(n->u.out);
	free_list(n->u.flat_out);
	free_list(n->u.flat_in);
	free_list(n->u.other);
	free_label(n->u.label);
	if (n->u.shape)
		n->u.shape->freefn(n);
	memset(&(n->u),0,sizeof(Agnodeinfo_t));
}

void dot_free_splines(edge_t* e)
{
	int		i;
	if (e->u.spl) {
		for (i = 0; i < e->u.spl->size; i++) free(e->u.spl->list[i].list);
		free(e->u.spl->list);
		free(e->u.spl);
	}
	e->u.spl = NULL;
}

void dot_cleanup_edge(edge_t* e)
{
	dot_free_splines(e);
	free_label(e->u.label);
	memset(&(e->u),0,sizeof(Agedgeinfo_t));
}

static void free_virtual_edge_list(node_t *n)
{
	edge_t *e;
	int	i;

	for (i=n->u.in.size - 1; i >= 0; i--) {
		e = n->u.in.list[i];
		delete_fast_edge(e);
		free(e);
	}
	for (i=n->u.out.size - 1; i >= 0; i--) {
		e = n->u.out.list[i];
		delete_fast_edge(e);
		free(e);
	}
}

static void free_virtual_node_list(node_t *vn)
{
	node_t	*next_vn;

	while (vn) {
		next_vn = vn->u.next;
		free_virtual_edge_list(vn);
		if (vn->u.node_type == VIRTUAL) {
			free_list(vn->u.out);
			free_list(vn->u.in);
			free(vn);
		}
		vn = next_vn;
	}
}

void dot_cleanup_graph(graph_t* g)
{
	int		i, c;
	graph_t	*clust;

	for (c = 1; c <= g->u.n_cluster; c++) {
		clust= g->u.clust[c];
		dot_cleanup(clust);
	}

	free_list(g->u.comp);
	if ((g == g->root) && g->u.rank) {
		for (i = g->u.minrank; i <= g->u.maxrank; i++)
			free(g->u.rank[i].v);
		free(g->u.rank);
	}
	free_ugraph(g);
	free_label(g->u.label);
	memset(&(g->u),0,sizeof(Agraphinfo_t));
}

/* delete the layout (but not the underlying graph) */
void dot_cleanup(graph_t* g)
{
	node_t  *n;
	edge_t  *e;

	free_virtual_node_list(g->u.nlist);
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
			dot_cleanup_edge(e);
		}
		dot_cleanup_node(n);
	}
	dot_cleanup_graph(g);
}

void dot_layout(Agraph_t *g)
{
    graph_init(g);
    g->u.drawing->engine = DOT;
    dot_init_node_edge(g);
    dot_rank(g);
    dot_mincross(g);
    dot_position(g);
    dot_sameports(g);
    dot_splines(g);
    if (mapbool(agget(g,"compound"))) makeCompoundEdges (g);
    dotneato_postprocess(g, dot_nodesize);
}   
