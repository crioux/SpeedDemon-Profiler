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
 * vtxgen.c generates graph diagrams in the format for
 *  Confluents's Visual Thought
 */

/*
 * If this time code is a pain to port, then just comment out the
 * next line.  It only provides an optional information field
 * in the (header...) block 
 */
#define SUPPORT_WRITEDATE

#ifdef SUPPORT_WRITEDATE
#include <time.h>
#endif

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	NONE	0
#define	NODE	1
#define	EDGE	2
#define	CLST	3

/* VTX font modifiers */
#define REGULAR 0
#define BOLD	1
#define ITALIC	2
#define UNDERSORE 4
#define STRIKE 8

/* VTX patterns */
#define P_NONE  0
#define P_SOLID	1
#define P_DOTTED 2
#define P_DASHED 3

/* VTX bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

/* VTX shape mappings */
typedef struct shapemap_s {
	char *shape;
	char *vtxshape;
} shapemap_t;

static shapemap_t shapemap[] = {
	{"box", "\"Rectangle\""},
	{"ellipse", "\"Ellipse\""},
	{"circle", "\"Ellipse\""},
	{"triangle", "\"Triangle\""},
	{"diamond", "\"Diamond\""},
	{"trapezium", "\"Trapezoid\""},
	{"parallelogram", "\"Parallelogram\""},
	{"hexagon", "\"Hexagon\""},
	{NULL,"\"Ellipse\""}				/* default */
};


static	FILE	*Outfile;
static	int		Obj;
static 	point	Pages;
static	int		Rot;
/* static	box		PB; */
static int		onetime = TRUE;

typedef struct context_t {
	int 	color_r,color_g,color_b;
	char	*fontfam,fontopt,font_was_set;
	char	pen,fill,penwidth,style_was_set;
	float	fontsz;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int SP;

static void vtx_reset(void)
{
	onetime = TRUE;
}


static void init_vtx(void)
{
	SP = 0;
	cstk[0].color_r = cstk[0].color_g = cstk[0].color_b = 0; 
	cstk[0].fontfam = "Times";	/* font family name */
	cstk[0].fontopt = REGULAR;	/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is solid */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

static pointf
vtx_pt(pointf p)
{
	pointf	rv;

	if (Rot == 0) { 
		rv.x = p.x;
		rv.y = p.y;
	}
	else {
		rv.x = p.y;
		rv.y = p.x;
	}
	return rv;
}

static void
vtx_ptarray(point* A, int n)
{
	int		i;
	pointf	p;

	fprintf(Outfile,"    (points\n");
	for (i = 0; i < n; i++) {
		p.x = (double)A[i].x; p.y = (double)A[i].y;
		p = vtx_pt(p);
		fprintf(Outfile,"      (%g %g)\n",p.x,p.y);
	}
	fprintf(Outfile,"    )\n");
}

static void
vtx_bzptarray(point* A, int start, int end)
{
	pointf	p;
	int 	qx, qy;
	int		i,j,incr=(start>end)?-1:1;

	fprintf(Outfile,"    (points\n");
	for (i = start, j=1; i != end; i += incr, j++) {
		switch (j % 3) {
			case 0:
				p.x = (double)A[i].x; p.y = (double)A[i].y;
				p = vtx_pt(p);
				fprintf(Outfile,"      (%g %g)\n",p.x,p.y);
				break;
			case 1:
#if 1
				qx = A[i].x; qy = A[i].y;
#else
				p.x = (double)A[i].x; p.y = (double)A[i].y;
				p = vtx_pt(p);
				fprintf(Outfile,"      (%g %g)\n",p.x,p.y);
#endif
				break;
			case 2:
#if 1
				/* undo EK's strange coding of straight segments */
				if (A[i].x == qx && A[i].y == qy) {
					if ((A[i-2].x == qx && A[i-2].y == qy)
					  || (A[i+1].x == qx && A[i+1].y == qy)) {
						p.x = (A[i+1].x + A[i-2].x)/2.0;
						p.y = (A[i+1].y + A[i-2].y)/2.0;
					}
					else {
						p.x = (double)qx; p.y = (double)qy;
					}
				}
				else {
					p.x = (A[i].x + qx)/2.0;
					p.y = (A[i].y + qy)/2.0;
				}
#else
				p.x = (double)A[i].x; p.y = (double)A[i].y;
#endif
				p = vtx_pt(p);
				fprintf(Outfile,"      (%g %g)\n",p.x,p.y);
				break;
		}
	}
	fprintf(Outfile,"    )\n");
}

static void vtx_font(context_t* cp)
{
/* FIX
	char	*fw,*fa;

	fw = fa = "Regular";
	switch (cp->fontopt) {
		case BOLD: fw = "Bold"; break;
		case ITALIC: fa = "Italic"; break;
	}
*/
}

static void vtx_comment(void* obj, attrsym_t* sym)
{
	char	*str;
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"; %s\n",str);
}

static void
vtx_begin_job(FILE *ofp, graph_t *g, char **lib, char *user,
char *info[], point pages)
{
	char *date = "";
#ifdef SUPPORT_WRITEDATE
	time_t when;
	struct tm *tm;
	size_t date_length = 200;

	time(&when);
	tm = localtime(&when);
	date = (char *)malloc(date_length);
	strftime(date, date_length, "%a %b %e %H:%M:%S %Z %Y", tm);
#endif

	Outfile = ofp;
	Pages = pages;
	/* N_pages = pages.x * pages.y; */

    fprintf(Outfile,"; Visual Thought 1.0\n"
					"\n"
					"(header\n"
					"  (program \"%s\")\n"
					"  (version \"%s\")\n"
					"  (buildDate \"%s\")\n"
					"  (writeDate \"%s\")\n"
					"  (documentPath \"\")\n"
					")\n"
					"\n",
		info[0],info[1],info[2],date);

	free (date);
}

static  void
vtx_end_job(void) { }

static void
vtx_begin_graph(graph_t* g, box bb, point pb)
{
	g = g;
	/* PB = bb; */
	if (onetime) {
		init_vtx();
		vtx_comment(g,agfindattr(g,"comment"));
		onetime = FALSE;
	}
}

static void
vtx_end_graph(void) { }

static void
vtx_begin_page(point page, double scale, int rot, point offset)
{
	int		page_number;
	/* point	sz; */

	Rot = rot;
	page_number =  page.x + page.y * Pages.x + 1;
	/* sz = sub_points(PB.UR,PB.LL); */

    fprintf(Outfile,"(document\n"
					"  (palette F)\n"
					"  (layout\n"
					"    (page \"Letter\")\n"
					"    (units \"Inches\")\n"
					"    (orientation \"portrait\")\n"
					"    (numberOfPages %d %d)\n"
					"    (scale %g)\n"
					"    (margins 18 18 18 18)\n"
					"  )\n"
					")\n"
					"\n"
					"(views\n"
					"  (view\n"
					"    (location 269 49)\n"
					"    (size 632 723)\n"
					"    (zoom %g)\n"
					"    (documentLocation 0 119)\n"
					"    (gridSnap T)\n"
					"    (gridVisibility F)\n"
					"    (gridSpacing 9)\n"
					"    (pageBreaks T)\n"
					"    (toolVisibility T)\n"
					"    (rulerVisibility T)\n"
					"  )\n"
					")\n"
					"\n",
		page_number, Pages.x*Pages.y, scale*100, scale);
}

static  void
vtx_end_page(void) { }

static  void
vtx_begin_cluster(graph_t* g)
{
	Obj = CLST;
}

static  void
vtx_end_cluster (void)
{
	Obj = NONE;
}

static void
vtx_begin_nodes(void)        
{
	Obj = NONE;
	fprintf(Outfile,"(shapes\n");
}
    
static void
vtx_end_nodes(void)  
{
    Obj = NONE;
	fprintf(Outfile,")\n"
					"\n");
}

static void 
vtx_begin_edges(void)        
{
    Obj = NONE;
	fprintf(Outfile,"(connections\n");
}            

static void
vtx_end_edges(void)  
{            
    Obj = NONE;
	fprintf(Outfile,")\n"
					"\n"
					"(groups\n"
					")\n");
}            

static  void
vtx_begin_node(node_t* n)
{
	shapemap_t *p;

	Obj = NODE;
	for (p = shapemap; p->shape; p++) {
		if (streq(n->u.shape->name,p->shape)) {
			break;
		}
	}
	fprintf(Outfile,"  (shape\n"
					"    (id %d)\n"
					"    (layer %d)\n"
					"    (type %s)\n",
		n->id + 1, n->id, p->vtxshape);
}

static  void
vtx_end_node (void)
{
	Obj = NONE;
	fprintf(Outfile,"  )\n");
}

static  void
vtx_begin_edge (edge_t* e)
{
	Obj = EDGE;
	fprintf(Outfile,"  (connection\n"
					"    (id %d)\n"
					"    (layer %d)\n"
					"    (rotation 0)\n"
					"    (textRotation 0)\n"
					"    (locked F)\n"
					"    (start %d)\n"
					"    (end %d)\n",
		e->id + 1, e->id, e->tail->id + 1, e->head->id + 1);
}

static  void
vtx_end_edge (void)
{
	Obj = NONE;
	fprintf(Outfile,"  )\n");
}

static  void
vtx_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP+1] = cstk[SP];
	SP++;
}

static  void 
vtx_end_context(void)
{
	int		psp = SP - 1;

	assert(SP > 0);
	SP = psp;
}

static void 
vtx_set_font(char* name, double size)
{
	char	*p,*q,buf[SMALLBUF];
	context_t	*cp;

	cp = &(cstk[SP]);
	cp->font_was_set = TRUE;
	cp->fontsz = (float)size;
	p = strdup(name);
	if ((q = strchr(p,'-'))) {
		*q++ = 0;
		canoncolor(q,buf);
		if (streq(buf,"italic")) cp->fontopt = ITALIC;
		else if (streq(q,"bold")) cp->fontopt = BOLD;
	}
	cp->fontfam = p;
	vtx_font(&cstk[SP]);
}

static void
vtx_style()
{
	context_t   *cp;

	cp = &(cstk[SP]);
	fprintf(Outfile,"    (style\n"
					"      (filled %s)\n"
					"      (fillColor %d %d %d)\n"
					"      (stroked T)\n"
					"      (strokeColor %d %d %d)\n"
					"      (lineWidth %d)\n"
					"      (shadowed F)\n"
					"      (shadowColor 39321 39321 39321)\n"
					"    )\n",
		cp->fill?"T":"F",
		cp->color_r, cp->color_g, cp->color_b,
		cp->color_r, cp->color_g, cp->color_b,
		cp->penwidth);
}
	
static void
vtx_node_style()
{
	fprintf(Outfile,"    (rotation 0)\n"
					"    (locked F)\n");
	vtx_style();
	fprintf(Outfile,"    (flipHorizontal F)\n"
					"    (flipVertical F)\n");
}

static  void
vtx_set_color(char* name)
{
    double r,g,b;   
    double h,s,v;   
    char   result[SMALLBUF];
	context_t   *cp;

	cp = &(cstk[SP]);
    colorxlate(name,result);
    if ((sscanf(result,"%lf %lf %lf", &h, &s, &v)) != 3) {
      fprintf(stderr, "Unknown color %s; using black\n", name);
      h = s = v = 0.0;
    }
    hsv2rgb(&r,&g,&b,h,s,v);
    cp->color_r=(int)(r*65535);
    cp->color_g=(int)(g*65535);
    cp->color_b=(int)(b*65535);
}

static  void
vtx_set_style(char** s)
{
	char		*line;
	context_t	*cp;

	cp = &(cstk[SP]);
	while ((line = *s++)) {
		if (streq(line,"solid")) { /* no-op */ }
		else if (streq(line,"dashed")) cp->pen = P_DASHED;
		else if (streq(line,"dotted")) cp->pen = P_DOTTED;
		else if (streq(line,"invis")) cp->pen = P_NONE;
		else if (streq(line,"bold")) cp->penwidth = WIDTH_BOLD;
		else if (streq(line,"filled")) cp->fill = P_SOLID;
		else if (streq(line,"unfilled")) { /* no-op */ }
		else {
            fprintf(stderr, "vtx_set_style: unsupported style %s - ignoring\n",
                line); 
        }
		cp->style_was_set = TRUE;
	}
}

static char *
vtx_string(char *s, char *auxbuf)
{
	char			*p = auxbuf,esc;
	while (*s) {
		esc = 0;
		switch (*s) {
			case '\t':	esc = 't'; break;
			case '{': case '}': case '\\': esc = *s; break;
		}
		if (esc) {*p++ = '\\'; *p++ = esc;}
		else *p++ = *s;
		s++;
	}
	*p = '\0';
	return auxbuf;
}

static void vtx_textline(point  p, char *str, int width, double fontsz, double align)
{
	char	buf[BUFSIZ],*astr;
	pointf	mp;

	mp.x = (double)p.x;
	mp.y = (double)(p.y - fontsz/2 + 2);
	mp = vtx_pt(mp);
	if (Obj == EDGE) {
		fprintf(Outfile,"    (showText T)\n"
						"    (textDistancePercentage 0.5)\n"
						"    (textWidth 72)\n"
						"    (textOffset 0)\n"
						"    (rtfText{\\rtf1\\ansi\\deff0\n"
						"{\\fonttbl{\\f0\\fnil helvetica medium;}}\n"
						"{\\colortbl\\red0\\green0\\blue0;}\n"
						"\\cf0\\plain\\pard {\\fs%d %s}})\n",
			(int)((fontsz*2)-8), vtx_string(str,buf));
	}
	else {
		if (align == -.5) {
			astr = "center";
		}
		else if (align < 0) {
			astr = "right"; 
		}
		else {
			astr = "left";
		}
		fprintf(Outfile,"    (showText T)\n"
						"    (textVerticalAlignment \"%s\")\n"
						"    (rtfText{\\rtf1\\ansi\\deff0\n"
						"{\\fonttbl{\\f0\\fnil helvetica medium;}}\n"
						"{\\colortbl\\red0\\green0\\blue0;}\n"
						"\\cf0\\plain\\pard {\\fs%d %s}})\n",
			astr, (int)((fontsz*2)-8), vtx_string(str,buf));
	}
}

static void vtx_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
	if (arrow_at_start) {
		vtx_bzptarray(A,n-2,0);
		fprintf(Outfile,"    (curved T)\n");
		vtx_style();
		fprintf(Outfile,"    (drawStartArrowhead %s)\n"
						"    (drawEndArrowhead %s)\n"
						"    (startArrowhead \"StandardArrow\")\n"
						"    (endArrowhead \"StandardArrow\")\n",
			arrow_at_end?"T":"F", arrow_at_start?"T":"F");
	}
	else {
		vtx_bzptarray(A,1,n-1);
		fprintf(Outfile,"    (curved T)\n");
		vtx_style();
		fprintf(Outfile,"    (drawStartArrowhead %s)\n"
						"    (drawEndArrowhead %s)\n"
						"    (startArrowhead \"StandardArrow\")\n"
						"    (endArrowhead \"StandardArrow\")\n",
			arrow_at_start?"T":"F", arrow_at_end?"T":"F");
	}
}

static void vtx_polygon(point *A, int n, int filled)
{
	int i;
	pointf		mp, max, min;

	mp.x  = 0; mp.y = 0;
	max.x = min.x = (double)A[0].x; max.y = min.y = (double)A[0].y;
	for (i=0; i<n; i++) {
		mp.x += (double)A[i].x; mp.y += (double)A[i].y;
		max.x = MAX(max.x, (double)A[i].x); max.y = MAX(max.y, (double)A[i].y);
		min.x = MIN(min.x, (double)A[i].x); min.y = MIN(min.y, (double)A[i].y);
	}
	mp.x /= n; mp.y /= n;
	mp = vtx_pt(mp);
	max = vtx_pt(max);
	min = vtx_pt(min);
	fprintf(Outfile,"    (location %g %g)\n"
					"    (size %g %g)\n", 
		mp.x, mp.y, max.x-min.x, max.y-min.y);
	vtx_node_style();
}

static void vtx_ellipse(point p, int rx, int ry, int filled)
{
	pointf		mp;

	mp.x = (double)p.x; mp.y = (double)p.y;
	mp = vtx_pt(mp);
	fprintf(Outfile,"    (location %g %g)\n"
					"    (size %g %g)\n", 
		mp.x, mp.y, (double)(rx+rx), (double)(ry+ry));
	vtx_node_style();
}

static void vtx_polyline(point *A, int n)
{
	vtx_ptarray(A,n);
	fprintf(Outfile,"    (curved F)\n");
	vtx_style();
}

static void vtx_user_shape(char *name, point *A, int n, int filled)
{
	int i;
	pointf		mp, max, min;

	mp.x  = 0; mp.y = 0;
	max.x = min.x = (double)A[0].x; max.y = min.y = (double)A[0].y;
	for (i=0; i<n; i++) {
		mp.x += (double)A[i].x; mp.y += (double)A[i].y;
		max.x = MAX(max.x, (double)A[i].x); max.y = MAX(max.y, (double)A[i].y);
		min.x = MIN(min.x, (double)A[i].x); min.y = MIN(min.y, (double)A[i].y);
	}
	mp.x /= n; mp.y /= n;
	mp = vtx_pt(mp);
	max = vtx_pt(max);
	min = vtx_pt(min);
	fprintf(Outfile,"    (location %g %g)\n"
					"    (size %g %g)\n", 
		mp.x, mp.y, max.x-min.x, max.y-min.y);
	vtx_node_style();
}

codegen_t	VTX_CodeGen = {
	vtx_reset,
	vtx_begin_job, vtx_end_job,
	vtx_begin_graph, vtx_end_graph,
	vtx_begin_page, vtx_end_page,
	vtx_begin_cluster, vtx_end_cluster,
	vtx_begin_nodes, vtx_end_nodes,
	vtx_begin_edges, vtx_end_edges,
	vtx_begin_node, vtx_end_node,
	vtx_begin_edge, vtx_end_edge,
	vtx_begin_context, vtx_end_context,
	vtx_set_font, vtx_textline,
	vtx_set_color, vtx_set_style,
	vtx_ellipse, vtx_polygon,
	vtx_bezier, vtx_polyline,
    1, vtx_user_shape,
	vtx_comment, 0
};
