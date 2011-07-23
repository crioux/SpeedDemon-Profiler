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

#define	NONE	0
#define	NODE	1
#define	EDGE	2
#define	CLST	3

/* MIF font modifiers */
#define REGULAR 0
#define BOLD	1
#define ITALIC	2

/* MIF patterns */
#define P_SOLID	0
#define P_NONE  15
#define P_DOTTED 4	/* i wasn't sure about this */
#define P_DASHED 11 /* or this */

/* MIF bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

static	FILE	*Outfile;
/* static	int		Obj; */
static	int		N_pages;
/* static 	point	Pages; */
static	double	Scale;
static	int		Rot;
static	box		PB;
static int		onetime = TRUE;

typedef struct context_t {
	char 	color_ix,*fontfam,fontopt,font_was_set;
	char	pen,fill,penwidth,style_was_set;
	float	fontsz;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int SP;

static char *FillStr = "<Fill 3>";
static char *NoFillStr = "<Fill 15>";

static void mif_reset(void)
{
	onetime = TRUE;
}


static void init_mif(void)
{
	SP = 0;
	cstk[0].color_ix = 0;		/* MIF color index 0-7 */
	cstk[0].fontfam = "Times";	/* font family name */
	cstk[0].fontopt = REGULAR;	/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is solid */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

static pointf
mifpt(pointf p)
{
	pointf	tmp,rv;
	tmp.x = p.x * Scale; tmp.y = Scale * p.y;
	if (Rot == 0) { rv.x = tmp.x; rv.y = PB.UR.y - PB.LL.y - tmp.y; }
	else {rv.x = PB.UR.x - PB.LL.x - tmp.y; rv.y = tmp.x; }
	return rv;
}

static void
mifptarray(point* A, int n)
{
	int		i;
	pointf	p;

	fprintf(Outfile," <NumPoints %d>\n",n);
	for (i = 0; i < n; i++) {
		p.x = A[i].x; p.y = A[i].y;
		p = mifpt(p);
		fprintf(Outfile," <Point %.2f %.2f>\n",p.x,p.y);
	}
}

static void mif_font(context_t* cp)
{
	char	*fw,*fa;

	fw = fa = "Regular";
	switch (cp->fontopt) {
		case BOLD: fw = "Bold"; break;
		case ITALIC: fa = "Italic"; break;
	}
	fprintf(Outfile,"<Font <FFamily `%s'> <FSize %.1f pt> <FWeight %s> <FAngle %s>>\n",cp->fontfam,cp->fontsz,fw,fa);
}

static void mif_color(int i)
{
	static char	*mifcolor[]= {
		"black","white","red","green","blue","cyan",
		"magenta","yellow","comment",
		"aquamarine","plum","peru","pink","mediumpurple","grey", 
		"lightgrey","lightskyblue","lightcoral","yellowgreen",
		(char*)0};
	if (i <= 8) fprintf(Outfile,"<Separation %d>\n",i);
	if (i > 8) fprintf(Outfile, "<ObColor `%s'>\n", mifcolor[i]);
}

static void mif_style(context_t* cp)
{
	fprintf(Outfile,"<Pen %d> <Fill %d> <PenWidth %d>\n",
		cp->pen,cp->fill,cp->penwidth);
}

static void mif_comment(void* obj, attrsym_t* sym)
{
	char	*str;
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"# %s\n",str);
}

static void
mif_begin_job(FILE *ofp, graph_t *g, char **lib, char *user, char *info[], point pages)
{
	Outfile = ofp;
	/* Pages = pages; */
	N_pages = pages.x * pages.y;
	fprintf(Outfile,"<MIFFile 3.00> # Generated by %s version %s (%s)\n",
		info[0],info[1],info[2]);
	fprintf(Outfile,"# For: %s\n",user);
	fprintf(Outfile,"# Title: %s\n",g->name);
	fprintf(Outfile,"# Pages: %d\n",N_pages);
	fprintf(Outfile,"<Units Upt>\n");
	fprintf(Outfile, "<ColorCatalog \n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Black'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  100.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsBlack>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `White'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsWhite>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Red'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  100.000000>\n");
fprintf(Outfile, "  <ColorYellow  100.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsRed>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Green'>\n");
fprintf(Outfile, "  <ColorCyan  100.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  100.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsGreen>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Blue'>\n");
fprintf(Outfile, "  <ColorCyan  100.000000>\n");
fprintf(Outfile, "  <ColorMagenta  100.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsBlue>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Cyan'>\n");
fprintf(Outfile, "  <ColorCyan  100.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsCyan>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Magenta'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  100.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsMagenta>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `Yellow'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  100.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsYellow>\n");
fprintf(Outfile, "  <ColorAttribute ColorIsReserved>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `aquamarine'>\n");
fprintf(Outfile, "  <ColorCyan  100.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  18.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `plum'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  100.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  33.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `peru'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  24.000000>\n");
fprintf(Outfile, "  <ColorYellow  100.000000>\n");
fprintf(Outfile, "  <ColorBlack  32.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `pink'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  50.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `mediumpurple'>\n");
fprintf(Outfile, "  <ColorCyan  40.000000>\n");
fprintf(Outfile, "  <ColorMagenta  100.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `grey'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  50.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `lightgrey'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  25.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `lightskyblue'>\n");
fprintf(Outfile, "  <ColorCyan  38.000000>\n");
fprintf(Outfile, "  <ColorMagenta  33.000000>\n");
fprintf(Outfile, "  <ColorYellow  0.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `lightcoral'>\n");
fprintf(Outfile, "  <ColorCyan  0.000000>\n");
fprintf(Outfile, "  <ColorMagenta  50.000000>\n");
fprintf(Outfile, "  <ColorYellow  60.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, " <Color \n");
fprintf(Outfile, "  <ColorTag `yellowgreen'>\n");
fprintf(Outfile, "  <ColorCyan  31.000000>\n");
fprintf(Outfile, "  <ColorMagenta  0.000000>\n");
fprintf(Outfile, "  <ColorYellow  100.000000>\n");
fprintf(Outfile, "  <ColorBlack  0.000000>\n");
fprintf(Outfile, " > # end of Color\n");
fprintf(Outfile, "> # end of ColorCatalog\n");

}

static  void
mif_end_job(void)
{
	fprintf(Outfile,"# end of MIFFile\n");
}

static void
mif_begin_graph(graph_t* g, box bb, point pb)
{
	g = g;
	PB = bb;
	if (onetime) {
		fprintf(Outfile,"<BRect %d %d %d %d>\n",
			PB.LL.x,PB.UR.y,PB.UR.x - PB.LL.x, PB.UR.y - PB.LL.y);
		init_mif();
		mif_comment(g,agfindattr(g,"comment"));
		onetime = FALSE;
	}
}

static void
mif_end_graph(void)
{
}

static void
mif_begin_page(point page, double scale, int rot, point offset)
{
	/* int		page_number; */
	/* point	sz; */

	Scale = scale;
	Rot = rot;
	/* page_number =  page.x + page.y * Pages.x + 1; */
	/* sz = sub_points(PB.UR,PB.LL); */
	fprintf(Outfile," <ArrowStyle <TipAngle 15> <BaseAngle 90> <Length %.1f> <HeadType Filled>>\n",14*Scale);
}

static  void
mif_end_page(void)
{
}

static  void
mif_begin_cluster(graph_t* g)
{
	/* Obj = CLST; */
}

static  void
mif_end_cluster (void)
{
	/* Obj = NONE; */
}

static void
mif_begin_nodes(void)        
{
	/* Obj = NONE; */
}
    
static void
mif_end_nodes(void)  
{
	/* Obj = NONE; */
}

static void 
mif_begin_edges(void)        
{
	/* Obj = NONE; */
}            

static void
mif_end_edges(void)  
{            
	/* Obj = NONE; */
}            

static  void
mif_begin_node(node_t* n)
{
	/* Obj = NODE; */
	fprintf(Outfile,"# %s\n",n->name);
	mif_comment(n,N_comment);
}

static  void
mif_end_node (void)
{
	/* Obj = NONE; */
}

static  void
mif_begin_edge (edge_t* e)
{
	/* Obj = EDGE; */
	fprintf(Outfile,"# %s -> %s\n",e->tail->name,e->head->name);
	mif_comment(e,E_comment);
}

static  void
mif_end_edge (void)
{
	/* Obj = NONE; */
}

static  void
mif_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP+1] = cstk[SP];
	SP++;
}

static  void 
mif_end_context(void)
{
	int			c, psp = SP - 1;
	assert(SP > 0);
	if (cstk[SP].color_ix != (c = cstk[psp].color_ix)) mif_color(c);
	if (cstk[SP].font_was_set) mif_font(&(cstk[psp]));
	if (cstk[SP].style_was_set) mif_style(&(cstk[psp]));
	/*free(cstk[psp].fontfam);*/
	SP = psp;
}

static void 
mif_set_font(char* name, double size)
{
	char	*p,*q,buf[SMALLBUF];
	context_t	*cp;

	cp = &(cstk[SP]);
	cp->font_was_set = TRUE;
	cp->fontsz = Scale * size;
	p = strdup(name);
	if ((q = strchr(p,'-'))) {
		*q++ = 0;
		canoncolor(q,buf);
		if (streq(buf,"italic")) cp->fontopt = ITALIC;
		else if (streq(q,"bold")) cp->fontopt = BOLD;
	}
	cp->fontfam = p;
	mif_font(&cstk[SP]);
}

/***** Arrowheads now centralized in emit.c
static  void
mif_arrowhead(point p, double theta, double scale,int flag)
{
	pointf		other,mp;
	double		v;
	v = cos(RADIANS(theta)) * ARROW_LENGTH; other.x = v + p.x;
	v = sin(RADIANS(theta)) * ARROW_LENGTH; other.y = v + p.y;
	fprintf(Outfile,"<ArrowStyle <ScaleFactor %.2f>>\n",scale);
	fprintf(Outfile,"<PolyLine <HeadCap ArrowHead> <NumPoints 2>\n");
		mp = mifpt(other);
		fprintf(Outfile," <Point %.2f %.2f>\n",mp.x,mp.y);
		mp.x = p.x; mp.y = p.y;
		mp = mifpt(mp);
		fprintf(Outfile," <Point %.2f %.2f>\n",mp.x,mp.y);
	fprintf(Outfile,">\n");
}
**********/

static  void
mif_set_color(char* name)
{
	int		i;
	char	buf[SMALLBUF];
	
	static char	*mifcolor[]= {
		"black","white","red","green","blue","cyan",
		"magenta","yellow","comment",
		"aquamarine","plum","peru","pink","mediumpurple","grey", 
		"lightgrey", "lightskyblue","lightcoral","yellowgreen",
		(char*)0};
	canoncolor(name,buf);
	for (i = 0; mifcolor[i]; i++)
		if (streq(mifcolor[i],buf))
			{ cstk[SP].color_ix = i; mif_color(i); return; }
	fprintf(stderr,"color %s not supported in MIF\n",name);
}

static  void
mif_set_style(char** s)
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
            fprintf(stderr, "mif_set_style: unsupported style %s - ignoring\n",
                line); 
        }
		cp->style_was_set = TRUE;
	}
	if (cp->style_was_set) mif_style(cp);
}

static char *
mif_string(char *s, char *auxbuf)
{
	char			*p = auxbuf,esc;
	while (*s) {
		esc = 0;
		switch (*s) {
			case '\t':	esc = 't'; break;
			case '>': case '\'': case '`': case '\\': esc = *s; break;
		}
		if (esc) {*p++ = '\\'; *p++ = esc;}
		else *p++ = *s;
		s++;
	}
	*p = '\0';
	return auxbuf;
}

static void mif_textline(point  p, char *str, int width, double fontsz, double align)
{
	char	buf[BUFSIZ],*astr;
	pointf	mp;

	mp.x = p.x;
	mp.y = p.y - fontsz/2 + 2;
	mp = mifpt(mp);
	if (align == -.5) astr = "Center";
	else {if (align < 0) astr = "Right"; else astr = "Left";}
	fprintf(Outfile,"<TextLine <Angle %d> <TLOrigin %.2f %.2f> <TLAlignment %s>",Rot,mp.x,mp.y,astr);
	fprintf(Outfile," <String `%s'>>\n",mif_string(str,buf));
}

static void mif_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
	fprintf(Outfile,"<PolyLine <Fill 15> <Smoothed Yes> <HeadCap Square>\n");
	mifptarray(A,n);
	fprintf(Outfile,">\n");
}

static void mif_polygon(point *A, int n, int filled)
{
	fprintf(Outfile,"<Polygon %s\n",(filled? FillStr : NoFillStr));
	mifptarray(A,n);
        fprintf(Outfile,">\n");
}

static void mif_ellipse(point p, int rx, int ry, int filled)
{
	pointf		tl,mp;
	tl.x = p.x - rx; tl.y = p.y + ry;
	if (Rot) {int t; t = rx; rx = ry; ry = t;}
	mp = mifpt(tl);
	fprintf(Outfile,"<Ellipse %s <BRect %.2f %.2f %.1f %.1f>>\n",
		filled?FillStr:NoFillStr,mp.x,mp.y,Scale*(rx + rx),Scale*(ry + ry));
}

static void mif_polyline(point* A, int n)
{
	fprintf(Outfile,"<PolyLine <HeadCap Square>\n");
	mifptarray(A,n);
	fprintf(Outfile,">\n");
}

static void mif_user_shape(char *name, point *A, int n, int filled)
{
	static boolean onetime = TRUE;
	if (onetime) {fprintf(stderr,"custom shapes not available with this driver\n"); onetime = FALSE;}
	mif_polygon(A,n,filled);
}

codegen_t	MIF_CodeGen = {
	mif_reset,
	mif_begin_job, mif_end_job,
	mif_begin_graph, mif_end_graph,
	mif_begin_page, mif_end_page,
	mif_begin_cluster, mif_end_cluster,
        mif_begin_nodes, mif_end_nodes,
        mif_begin_edges, mif_end_edges,
	mif_begin_node, mif_end_node,
	mif_begin_edge, mif_end_edge,
	mif_begin_context, mif_end_context,
	mif_set_font, mif_textline,
	mif_set_color, mif_set_style,
	mif_ellipse, mif_polygon,
	mif_bezier, mif_polyline,
    0/* mif_arrowhead */, mif_user_shape,
	0, 0
};
