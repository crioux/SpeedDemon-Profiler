/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include		"render.h"
#include		"gd.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

typedef struct context_t {
        char	color_ix, *fontfam, fontopt, font_was_set;
        char	pen, fill, penwidth;
        float	fontsz;
} context_t;

extern char *gd_string(char *s, char *auxbuf);
extern char *get_ttf_fontpath(char *fontreq, int warn);
extern pointf gdpt(pointf p);
extern void gd_font(context_t* cp);

#define		NONE		0
#define		NODE		1
#define		EDGE		2
#define		CLST		3

/* ISMAP font modifiers */
#define REGULAR 0
#define BOLD		1
#define ITALIC		2

/* ISMAP patterns */
#define P_SOLID		0
#define P_NONE  15
#define P_DOTTED 4				/* i wasn't sure about this */
#define P_DASHED 11				/* or this */

#define SCALE (GD_RESOLUTION/72.0)

/* ISMAP bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

static FILE	*Outfile;
static int	  Obj;
static edge_t	*Edge;
/* static int	 N_pages; */
/* static point	Pages; */
static double   Scale;
static pointf   Offset;
static int	  Rot;
static box	  PB;
static int	  onetime = TRUE;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int	  SP;

static  void
ismap_reset(void)
{
	onetime = TRUE;
}

static  void
init_ismap(void)
{
	SP = 0;
	cstk[0].color_ix = 0;		/* ISMAP color index 0-7 */
	cstk[0].fontfam = "Times";		/* font family name */
	cstk[0].fontopt = REGULAR;		/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is sold */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

static		  pointf
ismappt(pointf p)
{
	pointf		  rv;

	if (Rot == 0) {
		rv.x = p.x * Scale + Offset.x;
		rv.y = PB.UR.y - PB.LL.y - p.y * Scale + Offset.y;
	} else {
		rv.x = PB.UR.x - PB.LL.x - p.y * Scale + Offset.x;
		rv.y = PB.UR.y - PB.LL.y - p.x * Scale + Offset.y;
	}
	return rv;
}

static  void
ismap_color(int i)
{
}

static  void
ismap_style(context_t* cp)
{
}

static void
ismap_begin_job(FILE *ofp, graph_t *g, char **lib, char *user, char *info[], point pages)
{
	Outfile = ofp;
	/* Pages = pages; */
	/* N_pages = pages.x * pages.y; */
}

static void
ismap_end_job(void)
{
}

static void
ismap_begin_graph(graph_t* g, box bb, point pb)
{
	char		   *s;

	g = g;
	PB.LL.x = bb.LL.x * SCALE;
	PB.LL.y = bb.LL.y * SCALE;
	PB.UR.x = bb.UR.x * SCALE;
	PB.UR.y = bb.UR.y * SCALE;
	Offset.x = PB.LL.x + 1;
	Offset.y = PB.LL.y + 1;
	if (onetime) {
		init_ismap();
		onetime = FALSE;
	}
	if ((s = agget(g, "URL")) && strlen(s)) 
		fprintf(Outfile,"default %s %s\n",s, g->name);
}

static void
ismap_end_graph(void)
{
}

static  void
ismap_begin_page(point page, double scale, int rot, point offset)
{
	/* int			 page_number; */
	/* point		   sz; */

	Scale = scale * SCALE;
	Rot = rot;
	/* page_number = page.x + page.y * Pages.x + 1; */
	/* sz = sub_points(PB.UR, PB.LL); */
}

static  void
ismap_end_page(void)
{
}

static  void
ismap_begin_cluster(graph_t* g)
{
	Obj = CLST;
}

static  void
ismap_end_cluster(void)
{
	Obj = NONE;
}

static void
ismap_begin_nodes(void)        
{
	Obj = NONE;
}
    
static void
ismap_end_nodes(void)  
{
	Obj = NONE;
}

static void 
ismap_begin_edges(void)        
{
	Obj = NONE;
}            

static void
ismap_end_edges(void)  
{            
	Obj = NONE;
}            

static  void
ismap_begin_node(node_t* n)
{
	char		   *s,*s1,*s2,*lab;
	pointf			p,p1,p2;

	Obj = NODE;
	if ((s = agget(n, "URL")) && strlen(s)) {
		p.x = n->u.coord.x - n->u.lw;
		p.y = n->u.coord.y - (n->u.ht/2);
		p1 = ismappt(p);
		p.x = n->u.coord.x + n->u.rw;
		p.y = n->u.coord.y + (n->u.ht/2);
		p2 = ismappt(p);

		s = strdup(s);
		if ((s2 = strstr(s,NODENAME_ESC))) {
			s1 = n->name;
			*s2 = '\0';
			s2 += 2;
		} else {
			s1 = s2 = "";
		}

		lab = agget(n, "label");
		if ((strcmp(lab,NODENAME_ESC)) == 0) {
			lab = n->name;
		}
		fprintf(Outfile,"rectangle (%d,%d) (%d,%d) %s%s%s %s\n",
			ROUND(p1.x),ROUND(p1.y),ROUND(p2.x),ROUND(p2.y),
			s,s1,s2,lab);

		free(s);
	}
}

static  void
ismap_end_node(void)
{
	Obj = NONE;
}

static  void
ismap_begin_edge(edge_t* e)
{
	Obj = EDGE;
	Edge = e;
}

static  void
ismap_end_edge(void)
{
	Obj = NONE;
}

static  void
ismap_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP + 1] = cstk[SP];
	SP++;
}

static  void
ismap_end_context(void)
{
	int			 psp = SP - 1;
	assert(SP > 0);
	if (cstk[SP].font_was_set)
		gd_font(&(cstk[psp]));
	SP = psp;
}

static  void
ismap_set_font(char* name, double size)
{
        char               *p, *q, buf[SMALLBUF];
        context_t         *cp;

        cp = &(cstk[SP]);
        cp->font_was_set = TRUE;
        cp->fontsz = Scale * size;
        p = strdup(name);
        if ((q = strchr(p, '-'))) {
                *q++ = 0;
                canoncolor(q, buf);
                if (streq(buf, "italic"))
                        cp->fontopt = ITALIC;
                else if (streq(q, "bold"))
                        cp->fontopt = BOLD;
        }
        cp->fontfam = p;
        gd_font(&cstk[SP]);

}

static  void
ismap_arrowhead(point p, double theta, double scale,int flag)
{
}


static  void
ismap_set_color(char* name)
{
}

static  void
ismap_set_style(char** s)
{
}

static  void
ismap_textline(point p, char *str, int width, double fontsz, double align)
{
        char            buf[BUFSIZ];
        char            *fontlist, *err;
	char		*s,*s1,*s1h,*s1t,*s2;
        pointf          mp;
        int             intfontsz, brect[8];
        double          wrongness;
        extern          gdFontPtr       gdFontSmall;


	if (Obj != EDGE) 
		return;
        if (!((s = agget(Edge, "URL")) && strlen(s)))
		return;
	s = strdup(s);
	if ((s2 = strstr(s,NODENAME_ESC))) {
		s1t = Edge->tail->name;
		s1 = "->";
		s1h = Edge->head->name;
		*s2 = '\0';
		s2 += 2;
	} else {
		s1 = s1h = s1t = s2 = "";
	}

        fontlist = gd_alternate_fontlist(cstk[SP].fontfam);

        /*
         * Because the low-level renderer really only deals with integer
         * font sizes, we need to compensate for the roundoff error in
         * the "real" width.  ...maybe this could be done by a different
         * computation of font_scale_adj?
         */
        fontsz *= Scale;
        intfontsz = (int)(fontsz);
        wrongness = ((fontsz - intfontsz)/fontsz) * width;
        gd_string(str,buf);
        if (align == -.5)
		mp.x = p.x - width/2.0;	/* normal - center on p*/
        else if (align < 0.0)
		mp.x = p.x - width;	/* right justify - left of p */
        else
		mp.x = p.x;		/* left justify - right of p */
        	/* mp.x = mp.x + 5; */
        mp.y = p.y - fontsz/2.0 + 2.0;
/*      mp.y = p.y - MAX(fontsz/2.0, 4); */
        mp.y = p.y - fontsz / 4.0 ;
        mp.x = mp.x + wrongness;
        mp = gdpt(mp);

	err = gdImageStringFT(NULL, brect, 0, fontlist,
		(double)(intfontsz), (Rot? 90.0 : 0.0) * PI / 180.0,
		ROUND(mp.x), ROUND(mp.y), gd_string(str,buf));
        if (err)
		gdImageString(NULL, gdFontSmall, ROUND(mp.x), ROUND(mp.y),
                        (unsigned char *)gd_string(str,buf), 0);

	fprintf(Outfile,"rectangle (%d,%d) (%d,%d) %s%s%s%s%s %s\n",
		brect[0],brect[1],brect[4],brect[5],s,s1t,s1,s1h,s2,str);
	free(s);
}

static  void
ismap_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
}

static  void
ismap_polygon(point *A, int n, int filled)
{
	pointf		  p, p0;
	int			 i;

	p0.x = A[0].x; p0.y = A[0].y;
	p0 = ismappt(p0);
	p.x = p0.x; p.y = p0.y;
	for (i = 1; i < n; i++) {
		/* p1.x = p.x; p1.y = p.y; */
		p.x = A[i].x; p.y = A[i].y;
		p = ismappt(p);
/*		gdImageLine(im, ROUND(p1.x), ROUND(p1.y), 
			ROUND(p.x), ROUND(p.y), black); */
	}
/*	gdImageLine(im, ROUND(p.x), ROUND(p.y),
		ROUND(p0.x), ROUND(p0.y), black); */
}

static  void
ismap_ellipse(point p, int rx, int ry, int filled)
{
	pointf		  mp;

	mp.x = p.x; mp.y = p.y;
	mp = ismappt(mp);
/*	gdImageArc(im, ROUND(mp.x), ROUND(mp.y),
		ROUND(Scale*(rx + rx)), ROUND(Scale*(ry + ry)), 0, 360, black); */
}

static  void
ismap_polyline(point* A, int n)
{
	pointf		  p, p1;
	int			 i;

	p.x = A[0].x;
	p.y = A[0].y;
	p = ismappt(p);
	for (i = 1; i < n; i++) {
		p1.x = A[i].x;
		p1.y = A[i].y;
		p1 = ismappt(p1);
/*		gdImageLine(im, ROUND(p.x), ROUND(p.y), ROUND(p1.x), ROUND(p1.y), black); */
		p.x = p1.x;
		p.y = p1.y;
	}
}

static  void
ismap_user_shape(char *name, point *A,int  n, int filled)
{
	static boolean  onetime = TRUE;
	if (onetime) {
		fprintf(stderr, "custom shapes not available with this driver\n");
		onetime = FALSE;
	}
	ismap_polygon(A, n, filled);
}

codegen_t	   ISMAP_CodeGen = {
	ismap_reset,
	ismap_begin_job, ismap_end_job,
	ismap_begin_graph, ismap_end_graph,
	ismap_begin_page, ismap_end_page,
	ismap_begin_cluster, ismap_end_cluster,
	ismap_begin_nodes, ismap_end_nodes,
	ismap_begin_edges, ismap_end_edges,
	ismap_begin_node, ismap_end_node,
	ismap_begin_edge, ismap_end_edge,
	ismap_begin_context, ismap_end_context,
	ismap_set_font, ismap_textline,
	ismap_set_color, ismap_set_style,
	ismap_ellipse, ismap_polygon, ismap_bezier,
	ismap_polyline, 0/* ismap_arrowhead */, ismap_user_shape,
	0, gd_textsize
};
