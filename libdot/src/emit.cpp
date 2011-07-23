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
 *  graphics code generator
 */

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int			N_pages, Page;		/* w.r.t. unrotated coords */
static int			Layer,Nlayers;
static char			**LayerID;
static point		First,Major,Minor;
static point		Pages;
static box			PB;		/* drawable region in device coords */
static pointf		GP;		/* graph page size, in graph coords */
static box			CB;		/* current page box, in graph coords */
static point		PFC;	/* device page box for centering */
static double	    Deffontsize;
static char			*Deffontname;
static char			Layerdelims[] = ":\t ";

void emit_reset(Agraph_t* g)
{
	Agnode_t	*n;

	N_pages = Page = 0;
	Layer = Nlayers = 0;
	LayerID = (char **) 0;
	First.x = First.y = 0;
	Major.x = Major.y = 0;
	Minor.x = Minor.y = 0;
	Pages.x = Pages.y = 0;
	PB.LL.x = PB.LL.y = PB.UR.x = PB.UR.y = 0;
	GP.x = GP.y = 0;
	CB.LL.x = CB.LL.y = CB.UR.x = CB.UR.y = 0;
	PFC.x = PFC.y = 0;
	Deffontsize = 0;
	Deffontname = (char *) 0;

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		n->u.state = 0;
	}

	if(CodeGen) CodeGen->reset();
}

void emit_graph(graph_t* g, int sorted)
{
	point	curpage;
	node_t	*n;
	edge_t	*e;

	emit_header(g);
	Layer = 1;
	do {
		if (Nlayers > 0) emit_layer(Layer);
		for (curpage = First; validpage(curpage); curpage = pageincr(curpage)) {
			setup_page(g,curpage);
			if (g->u.label) emit_label(g->u.label,g);
			emit_clusters(g);
			if (sorted) {
				/* output all nodes, then all edges */
				CodeGen->begin_nodes();
				for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
					emit_node(n);
				}
				CodeGen->end_nodes();
				CodeGen->begin_edges();
				for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
					for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
						emit_edge(e);
					}
				}
				CodeGen->end_edges();
			}
			else {
				/* output in breadth first graph walk order */
            	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
                	emit_node(n);
                	for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
                    	emit_node(e->head);
                    	emit_edge(e);
                	}
            	}    
			}
			CodeGen->end_page();
		}
		Layer++;
	} while (Layer <= Nlayers);
	emit_trailer();
}

void emit_eof(void)
{
	if (Page > 0) CodeGen->end_job();
}

void emit_clusters(graph_t* g)
{
	int			i,c,filled;
	graph_t		*subg;
	point		A[4];
	char		*str,**style;

	for (c = 1; c <= g->u.n_cluster; c++) {
		subg = g->u.clust[c];
		if (clust_in_layer(subg) == FALSE) continue;
		CodeGen->begin_cluster(subg);
		CodeGen->begin_context();
		filled = FALSE;
		if (((str = agget(subg,"style")) != 0) && str[0]) {
			CodeGen->set_style(style = parse_style(str));
			for (i = 0; style[i]; i++) 
				if (strcmp(style[i],"filled")==0) {filled = TRUE; break;}
		}
		emit_background(subg, subg->u.bb.LL, subg->u.bb.UR);
		if (((str = agget(subg,"color")) != 0) && str[0])
			CodeGen->set_color(str);
		A[0] = subg->u.bb.LL;
		A[2] = subg->u.bb.UR;
		A[1].x = A[2].x; A[1].y = A[0].y;
		A[3].x = A[0].x; A[3].y = A[2].y;
		CodeGen->polygon(A,4,filled);
		if (subg->u.label) emit_label(subg->u.label,subg);
		CodeGen->end_context();
		CodeGen->end_cluster();
		emit_clusters(subg);
	}
}

void emit_node(node_t* n)
{
	if (n->u.shape == NULL) return;
	if (node_in_layer(n->graph,n) && node_in_CB(n) && (n->u.state != Page)) {
		n->u.shape->codefn(n);
		n->u.state = Page;
	}
}

void emit_arrowhead (point p, double theta, double scale, int flag)
/* allowable are NORM|INV + DOT|ODOT */
{
  point a[3];
  pointf u,v;
  int r;

  theta = RADIANS(theta);

  /* FIXME: does this have enough precision for ps? */
  if (flag & ARR_NORM) {
    u.x = p.x + ARROW_LENGTH*scale*cos(theta);
    u.y = p.y + ARROW_LENGTH*scale*sin(theta);
    v.x = ARROW_WIDTH/2.*scale*cos(theta+PI/2);
    v.y = ARROW_WIDTH/2.*scale*sin(theta+PI/2);
    a[0].x = ROUND(u.x+v.x); a[0].y = ROUND(u.y+v.y);
    a[1] = p;
    a[2].x = ROUND(u.x-v.x); a[2].y = ROUND(u.y-v.y);
    CodeGen->polygon(a,3,1);
    p.x = ROUND(u.x); p.y = ROUND(u.y);
  } else if (flag & ARR_INV) {
    u.x = p.x + ARROW_INV_LENGTH*scale*cos(theta);
    u.y = p.y + ARROW_INV_LENGTH*scale*sin(theta);
    v.x = ARROW_INV_WIDTH/2.*scale*cos(theta+PI/2);
    v.y = ARROW_INV_WIDTH/2.*scale*sin(theta+PI/2);
    a[0].x = p.x + ROUND(v.x);  a[0].y = p.y + ROUND(v.y);
    a[1].x = ROUND(u.x);        a[1].y = ROUND(u.y);
    a[2].x = p.x + ROUND(-v.x); a[2].y = p.y + ROUND(-v.y);
    CodeGen->polygon(a,3,1);
    p.x = ROUND(u.x); p.y = ROUND(u.y);
  }
  if (flag & (ARR_DOT|ARR_ODOT)) {
    r = ROUND (ARROW_DOT_RADIUS*scale);
    CodeGen->ellipse(p,r,r, flag & ARR_DOT);
  }
}

void emit_edge(edge_t* e)
{
	int		i;
	char	*color,*style;
	bezier	bz;
	boolean	saved = FALSE;
	double	scale;

	if ((edge_in_CB(e) == FALSE) || (edge_in_layer(e->head->graph,e) == FALSE))
		return;

	CodeGen->begin_edge(e);
	style = late_string(e,E_style,"");
	color = late_string(e,E_color,"");
	scale = late_float(e,E_arrowsz,1.0,0.0);
	if (color[0] || style [0]) {
		CodeGen->begin_context();
		if (style[0]) CodeGen->set_style(parse_style(style));
		if (color[0]) CodeGen->set_color(color);
		saved = TRUE;
	}
	if (e->u.spl) {
		for (i = 0; i < e->u.spl->size; i++) {
			bz = e->u.spl->list[i];
			if (NOT(codegen_bezier_has_arrows())) {
				CodeGen->beziercurve(bz.list,bz.size,FALSE,FALSE);
                /* vladimir: added sflag/eflag, arrowheads are made here */
				if (bz.sflag) 
                  emit_arrowhead
                    (bz.sp, DEGREES(atan2pt(bz.list[0],bz.sp)),
                     scale, bz.sflag);
				if (bz.eflag) 
                  emit_arrowhead 
                    (bz.ep, DEGREES(atan2pt(bz.list[bz.size-1],bz.ep)),
                     scale, bz.eflag);
			}
			else CodeGen->beziercurve(bz.list,bz.size,bz.sflag,bz.eflag);
		}
	}
	if (e->u.label) {
		emit_label(e->u.label,e->tail->graph);
		if (mapbool(late_string(e,E_decorate,"false")))
			emit_attachment(e->u.label,e->u.spl);
	}
	if (e->u.head_label) emit_label(e->u.head_label,e->tail->graph); /* vladimir */
	if (e->u.tail_label) emit_label(e->u.tail_label,e->tail->graph); /* vladimir */

	if (saved) CodeGen->end_context();
	CodeGen->end_edge();
}

int node_in_CB(node_t* n)
{
	box	nb;

	if (N_pages == 1) return TRUE;
	nb.LL.x = n->u.coord.x - n->u.lw;
	nb.LL.y = n->u.coord.y - n->u.ht/2;
	nb.UR.x = n->u.coord.x + n->u.rw;
	nb.UR.y = n->u.coord.y + n->u.ht/2;
	return rect_overlap(CB,nb);
}

int node_in_layer(graph_t* g, node_t* n)
{
	char	*pn,*pe;
	edge_t	*e;

	if (Nlayers <= 0) return TRUE;
	pn = late_string(n,N_layer,"");
	if (selectedlayer(pn)) return TRUE;
	if (pn[0]) return FALSE;
	if ((e = agfstedge(g,n)) == NULL) return TRUE;
	for (e = agfstedge(g,n); e; e = agnxtedge(g,e,n)) {
		pe = late_string(e,E_layer,"");
		if ((pe[0] == '\0') || selectedlayer(pe)) return TRUE;
	}
	return FALSE;
}

int edge_in_layer(graph_t* g, edge_t* e)
{
	char	*pe,*pn;
	int		cnt;

	if (Nlayers <= 0) return TRUE;
	pe = late_string(e,E_layer,"");
	if (selectedlayer(pe)) return TRUE;
	if (pe[0]) return FALSE;
	for (cnt = 0; cnt < 2; cnt++) {
		pn = late_string(cnt < 1? e->tail:e->head,N_layer,"");
		if ((pn[0] == '\0') || selectedlayer(pn)) return TRUE;
	}
	return FALSE;
}

int clust_in_layer(graph_t* subg)
{
	char		*pg;
	node_t		*n;

	if (Nlayers <= 0) return TRUE;
	pg = late_string(subg,agfindattr(subg,"layer"),"");
	if (selectedlayer(pg)) return TRUE;
	if (pg[0]) return FALSE;
	for (n = agfstnode(subg); n; n = agnxtnode(subg,n))
		if (node_in_layer(subg,n)) return TRUE;
	return FALSE;
}

int edge_in_CB(edge_t* e)
{
	int		i,j,np;
	bezier	bz;
	point	*p,pp,sz;
	box		b;
	textlabel_t	*lp;

	if (N_pages == 1) return TRUE;
	if (e->u.spl == NULL) return FALSE;
	for (i = 0; i < e->u.spl->size; i++) {
		bz = e->u.spl->list[i];
		np = bz.size;
		p = bz.list;
		pp = p[0];
		for (j = 0; j < np; j++) {
			if (rect_overlap(CB,mkbox(pp,p[j]))) return TRUE;
			pp = p[j];
		}
	}
	if ((lp = e->u.label) == NULL) return FALSE;
	sz = cvt2pt(lp->dimen);
	b.LL.x = lp->p.x - sz.x / 2; b.UR.x = lp->p.x + sz.x / 2;
	b.LL.y = lp->p.y - sz.y / 2; b.UR.y = lp->p.y + sz.y / 2;
	return rect_overlap(CB,b);
}

void emit_attachment(textlabel_t* lp, splines* spl)
{
	point	sz,A[3];
	char	*s;

	for (s = lp->text; *s; s++) if (isspace(*s) == FALSE) break;
	if (*s == 0) return;

	sz = cvt2pt(lp->dimen);
	A[0] = pointof(lp->p.x + sz.x/2,lp->p.y - sz.y/2);
	A[1] = pointof(A[0].x - sz.x, A[0].y);
	A[2] = dotneato_closest(spl,lp->p);
	CodeGen->polyline(A,3);
}

void emit_header(graph_t* g)
{
	char	*user,buf[SMALLBUF];

	setup_graph(g);
	if (Page == 0) {
		user = username(buf);
		CodeGen->begin_job(Output_file,g,Lib,user,Info,Pages);
	}
	CodeGen->begin_graph(g,PB,PFC);
}

void emit_trailer(void)
{
	CodeGen->end_graph();
}

point pagecode(char c)
{
	point		rv;
	rv.x = rv.y = 0;
	switch(c) {
		case 'T': First.y = Pages.y - 1; rv.y = -1; break;
		case 'B': rv.y = 1; break;
		case 'L': rv.x = 1; break;
		case 'R': First.x = Pages.x - 1; rv.x = -1; break;
	}
	return rv;
}

static void set_pagedir(graph_t* g)
{
	char	*str;

	Major.x = Major.y = Minor.x = Minor.y = 0;
	str = agget(g,"pagedir");
	if (str && str[0]) {
		Major = pagecode(str[0]);
		Minor = pagecode(str[1]);
	}
	if ((abs(Major.x + Minor.x) != 1) || (abs(Major.y + Minor.y) != 1)) {
		Major.x = 0; Major.y = 1; Minor.x = 1; Minor.y = 0;
		First.x = First.y = 0;
		if (str) fprintf(stderr,"warning, pagedir=%s ignored\n",str);
	}
}

int validpage(point page)
{
	return ((page.x >= 0) && (page.x < Pages.x) 
		&& (page.y >= 0) && (page.y < Pages.y));
}

int layerindex(char* tok)
{
	int		i;

	for (i = 1; i <= Nlayers; i++) 
		if (streq(tok,LayerID[i])) return i;
	return -1;
}

int is_natural_number(char* str)
{
	while (*str) if (NOT(isdigit(*str++))) return FALSE;
	return TRUE;
}

int layer_index(char* str, int all)
{
	int		i;

	if (streq(str,"all")) return all;
	if (is_natural_number(str)) return atoi(str);
	if (LayerID)
		for (i = 1; i <= Nlayers; i++)
			if (streq(str,LayerID[i])) return i;
	return -1;
}

int selectedlayer(char* spec)
{
	int		n0,n1;
	char	buf[SMALLBUF], *w0, *w1;

	strcpy(buf,spec);
	w1 = w0 = strtok(buf,Layerdelims);
	if (w0) w1 = strtok(NULL,Layerdelims);
	switch((w0 != NULL) + (w1 != NULL)) {
	case 0: return FALSE;
	case 1: n0 = layer_index(w0,Layer);
		return (n0 == Layer);
	case 2: n0 = layer_index(w0,0);  n1 = layer_index(w1,Nlayers);
		if ((n0 < 0) || (n1 < 0)) return TRUE;
		if (n0 > n1) {int t = n0; n0 = n1; n1 = t;}
		return BETWEEN(n0,Layer,n1);
	}
	return FALSE;
}

point
pageincr(point page)
{
	page = add_points(page,Minor);
	if (validpage(page) == FALSE) {
		if (Major.y) page.x = First.x;
		else page.y = First.y;
		page = add_points(page,Major);
	}
	return page;
}

static point exch_xy(point p)
{
	int		t;
	t = p.x; p.x = p.y; p.y = t;
	return p;
}

static pointf exch_xyf(pointf p)
{
	double		t;
	t = p.x; p.x = p.y; p.y = t;
	return p;
}

/* this isn't a pretty sight... */
void setup_graph(graph_t* g)
{
	double		xscale,yscale,scale;
	char		*p;
	point		PFCLM;	/* page for centering less margins */
	point		DS;		/* device drawable region for a page of the graph */

	assert((g->u.bb.LL.x == 0) && (g->u.bb.LL.y == 0));

	if (LayerID) free(LayerID);
	if ((p = agget(g,"layers")) != 0) Nlayers = parse_layers(p);
	else {LayerID = NULL; Nlayers = 0;}

	/* determine final drawing size and scale to apply. */
	/* N.B. magnification could be allowed someday in the next conditional */
	/* N.B. size given by user is not rotated by landscape mode */
	if ((g->u.drawing->size.x > 0)	/* was given by user... */
		&& ((g->u.drawing->size.x < g->u.bb.UR.x) /* drawing is too big... */
			|| (g->u.drawing->size.y < g->u.bb.UR.y))) {
		xscale = ((double)g->u.drawing->size.x) / g->u.bb.UR.x;
		yscale = ((double)g->u.drawing->size.y) / g->u.bb.UR.y;
		scale = MIN(xscale,yscale);
		g->u.drawing->scale = (float)scale;
		g->u.drawing->size.x = (int)(scale * g->u.bb.UR.x);
		g->u.drawing->size.y = (int)(scale * g->u.bb.UR.y);
	}
	else {	/* go with "natural" size of layout */
		g->u.drawing->size = g->u.bb.UR;
		scale = g->u.drawing->scale = 1.0;
	}

	/* determine pagination */
	PB.LL = g->u.drawing->margin;
	if ((g->u.drawing->page.x > 0) && (g->u.drawing->page.y > 0)) {
			/* page was set by user */
		point	tp;
		PFC = g->u.drawing->page;
		PFCLM.x = PFC.x - 2*PB.LL.x; PFCLM.y = PFC.y - 2*PB.LL.y;
		GP.x = PFCLM.x ; GP.y = PFCLM.y;	/* convert to double */
		if (g->u.drawing->landscape) GP = exch_xyf(GP);
		GP.x = GP.x / scale; GP.y = GP.y / scale;
			/* we don't want graph page to exceed its bounding box */
		GP.x = MIN(GP.x,g->u.bb.UR.x); GP.y = MIN(GP.y,g->u.bb.UR.y);
		Pages.x = (int)((GP.x > 0) ? ceil( ((double)g->u.bb.UR.x) / GP.x) : 1);
		Pages.y = (int)((GP.y > 0) ? ceil( ((double)g->u.bb.UR.y) / GP.y) : 1);
		N_pages = Pages.x * Pages.y;

			/* find the drawable size in device coords */
		tp = g->u.drawing->size;
		if (g->u.drawing->landscape) tp = exch_xy(tp);
		DS.x = MIN(tp.x,PFCLM.x);
		DS.y = MIN(tp.y,PFCLM.y);
	}
	else {
			/* page not set by user, assume default when centering,
				but allow infinite page for any other interpretation */
		GP.x = g->u.bb.UR.x; GP.y = g->u.bb.UR.y;
		PFC.x = DEFAULT_PAGEWD; PFC.y = DEFAULT_PAGEHT;
		PFCLM.x = PFC.x - 2*PB.LL.x; PFCLM.y = PFC.y - 2*PB.LL.y;
		DS = g->u.drawing->size;
		if (g->u.drawing->landscape) DS = exch_xy(DS);
		Pages.x = Pages.y = N_pages = 1;
	}

	set_pagedir(g);
	/* determine page box including centering */
	if (g->u.drawing->centered) {
		point	extra;
		if ((extra.x = PFCLM.x - DS.x) < 0) extra.x = 0;
		if ((extra.y = PFCLM.y - DS.y) < 0) extra.y = 0;
		PB.LL.x += extra.x / 2; PB.LL.y += extra.y / 2;
	}
	PB.UR = add_points(PB.LL,DS);
	Deffontname = late_nnstring(g->proto->n,N_fontname,DEFAULT_FONTNAME);
	Deffontsize = late_float(g->proto->n,N_fontsize,DEFAULT_FONTSIZE,MIN_FONTSIZE);
}

void emit_background(graph_t* g, point LL, point UR)
{
	char	*str;
	point	A[4];

	if (((str = agget(g,"bgcolor")) != 0) && str[0]) {
		A[0] = A[1] = LL;
		A[2] = A[3] = UR;
		A[1].y = A[2].y;
		A[3].y = A[0].y;
		CodeGen->set_color(str);
		CodeGen->polygon(A,4,TRUE);	/* filled */
	}
}

/* even if this makes you cringe, at least it's short */
void setup_page(graph_t* g, point page)
{
	point		offset;
	int			theta;

	Page++;
	theta = (g->u.drawing->landscape ? 90 : 0);

	/* establish current box in graph coordinates */
	CB.LL.x = (int)(page.x  * GP.x);
	CB.LL.y = (int)(page.y * GP.y);
	CB.UR.x = (int)(CB.LL.x + GP.x);
	CB.UR.y = (int)(CB.LL.y + GP.y);

	/* establish offset to be applied, in graph coordinates */
	if (g->u.drawing->landscape == FALSE) offset = pointof(-CB.LL.x,-CB.LL.y);
	else { offset.x = (page.y + 1) * GP.y; offset.y = -page.x * GP.x; }
	CodeGen->begin_page(page,g->u.drawing->scale,theta,offset);
	emit_background(g, CB.LL, CB.UR);
	emit_defaults(g);
}

void emit_label(textlabel_t* lp, Agraph_t *g)
{
	int		i, left_x, center_x, right_x, width_x;
	point	ref;
	double	align;

	g = g->root;
	width_x = POINTS(lp->dimen.x);
	center_x = lp->p.x;
	left_x = center_x - width_x/2;
	right_x = center_x + width_x/2;
	ref.y = ROUND(lp->p.y + ((POINTS_PER_INCH*lp->dimen.y) - lp->fontsize)/2.0);

	if (lp->nlines < 1) return;
	CodeGen->begin_context();
		CodeGen->set_color(lp->fontcolor);
		CodeGen->set_font(lp->fontname,lp->fontsize*g->u.drawing->font_scale_adj);

		for (i = 0; i < lp->nlines; i++) {
			switch(lp->line[i].just) {
				case 'l':	align = 0.0;	ref.x = left_x;		break;
				case 'r':	align = -1.0;	ref.x = right_x;	break;
				default:
				case 'n':	align = -0.5;	ref.x =	center_x;	break;
			}
			CodeGen->textline(ref,lp->line[i].str,lp->line[i].width,
				lp->fontsize*g->u.drawing->font_scale_adj,align);
			ref.y = ref.y - lp->fontsize - 2;
		}

	CodeGen->end_context();
}

void emit_defaults(graph_t* g)
{
	CodeGen->set_color(DEFAULT_COLOR);
	CodeGen->set_font(Deffontname,Deffontsize);
}

int parse_layers(char* p)
{
	int		ntok,c;
	char	*pcopy,*tok;

	ntok = strccnt(p,':')+1;
	pcopy = strdup(p);
	if (LayerID) free(LayerID);
	LayerID = N_NEW(ntok+2,char*);
	c = 1;
	for (tok = strtok(pcopy,Layerdelims); tok; tok = strtok(NULL,Layerdelims)) 
		LayerID[c++] = tok;
	return ntok;
}

void emit_layer(int n)
{
	char	buf[BUFSIZ],*fake[2];
	char	null = 0;

	if (LayerID) {
		/* create compatible char array structure for set_style call */
		sprintf(buf,"setlayer%c%d%c%d%c",null,n,null,Nlayers,null);
		fake[0] = buf;
		fake[1] = NULL;
		CodeGen->set_style(fake);
	}
}

static int style_delim(int c)
{
	switch (c) {
		case '(': case ')': case ',': case '\0': return TRUE;
		default : return FALSE;
	}
}

static char *
style_token(char * p, char *out)
{
	while (*p && (isspace(*p) || (*p ==','))) p++;
	switch (*p) {
		case '\0': return NULL;
		case '(': case ')': *out++ = *p++; break;
		default: while (style_delim(*p) == FALSE) *out++ = *p++;
	}
	*out = '\0';
	return p;
}

char **
parse_style(char* s)
{
	static char	*parse[64];
	static char	outbuf[SMALLBUF];

	int			fun = 0;
	boolean		in_parens = FALSE;
	char		buf[SMALLBUF],*p,*q,*out;


	p = s;
	out = outbuf;
	while ((p = style_token(p,buf)) != 0) {
		switch (buf[0]) {
		case '(':
			if (in_parens) {
				fprintf(stderr,"nesting not allowed in style: %s\n",s);
				parse[0] = (char*)0;
				return parse;
			}
			in_parens = TRUE;
			break;

		case ')':
			if (in_parens == FALSE) {
				fprintf(stderr,"unmatched ')' in style: %s\n",s);
				parse[0] = (char*)0;
				return parse;
			}
			in_parens = FALSE; 
			break;

		default:
			if (in_parens == FALSE) {
				*out++ = '\0';	/* terminate previous */
				parse[fun++] = out;
			}
			q = buf;
			while ((*out++ = *q++) != 0);
		}
	}
	*out = '\0';

	if (in_parens) {
		fprintf(stderr,"unmatched '(' in style: %s\n",s);
		parse[0] = (char*)0;
		return parse;
	}
	parse[fun] = (char*)0;
	return parse;
}

static struct cg_s {
        codegen_t   *cg;
        char        *name;
        int         id;
    }
    gens[] = {
//        {&PS_CodeGen,"ps",POSTSCRIPT},
//        {&HPGL_CodeGen,"hpgl",HPGL},
//        {&HPGL_CodeGen,"pcl",PCL},
//        {&MIF_CodeGen,"mif",MIF},
//        {&PIC_CodeGen,"pic",PIC_format},
//        {&GD_CodeGen,"gd",GD},
//        {&GD_CodeGen,"gd2",GD2},
//        {&GD_CodeGen,"gif",GIF},
#ifdef HAVE_LIBJPEG
//        {&GD_CodeGen,"jpg",JPEG},
//        {&GD_CodeGen,"jpeg",JPEG},
#endif
#ifdef HAVE_LIBPNG
#ifdef HAVE_LIBZ
//        {&GD_CodeGen,"png",PNG},
#endif
#endif
//        {&GD_CodeGen,"wbmp",WBMP},
#ifdef HAVE_LIBXPM
//        {&GD_CodeGen,"xbm",XBM},
#endif
//        {&ISMAP_CodeGen,"ismap",ISMAP},
//        {&IMAP_CodeGen,"imap",IMAP},
#ifdef HAVE_LIBPNG
//        {&VRML_CodeGen,"vrml",VRML},
#endif
//        {&VTX_CodeGen,"vtx",VTX},
//        {&MP_CodeGen,"mp",METAPOST},
        /* {&FIG_CodeGen,"fig",FIG}, */
//        {&SVG_CodeGen,"svg",SVG},
        {(codegen_t*)0,"dot",ATTRIBUTED_DOT},
        {(codegen_t*)0,"canon",CANONICAL_DOT},
        {(codegen_t*)0,"plain",PLAIN},
        {(codegen_t*)0,(char*)0,0}
    };

int lang_select(char* str)
{
    struct  cg_s *p;
    for (p = gens; p->name; p++) {
        if (stricmp(str,p->name) == 0) {
            CodeGen = p->cg;
            return p->id;
        }
    }
    fprintf(stderr,"warning, language %s not recognized, use one of:\n",str);
    for (p = gens; p->name; p++) fprintf(stderr," %s",p->name);
    fprintf(stderr,"\n");
    return ATTRIBUTED_DOT;
}

FILE *
file_select(char* str)
{
    FILE    *rv;
    rv = fopen(str,"wb");
    if (rv == NULL) { perror(str); exit(1); }
    return rv;
}

void use_library(char* name)
{
    static int  cnt = 0;
    if (name) {
        Lib = ALLOC(cnt+2,Lib,char*);
        Lib[cnt++] = name;
        Lib[cnt] = NULL;
    }
}
