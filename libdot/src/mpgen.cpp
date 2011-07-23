/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
/* mpgen.c 1999-Feb-23 Jim Hefferon jim@joshua.smcvt.edu 
 *  Adapted from psgen.c.  See 1st_read.mp.
 */
#include	"render.h"
#ifndef MSWIN32
#include <unistd.h>
#endif
#include <sys/stat.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	NONE	0
#define	NODE	1
#define	EDGE	2
#define	CLST	3

static	FILE	*Outfile;
static	int		Obj;
/* static 	point	Pages; */
/* static	box		PB; */
static int		onetime = TRUE;

/* static char	**U_lib; */

typedef struct grcontext_t {
	char	*color,*font;
	double	size;
} grcontext_t;

#define STACKSIZE 32  /* essentially infinite? */
static grcontext_t S[STACKSIZE];
static int SP = 0;

static void
mp_reset(void)
{
	onetime = TRUE;
}

static void
mp_begin_job(FILE *ofp,graph_t *g, char **lib, char *user, char *info[],
point pages)
{
	Outfile = ofp;
        /* pages and libraries not here (yet?) */
	/* Pages = pages; */
	/* U_lib = lib; */
	/* N_pages = pages.x * pages.y; */
	/* Cur_page = 0; */

	fprintf(Outfile,"%%--- graphviz MetaPost input\n");
	fprintf(Outfile,"%% Created by program: %s version %s (%s)\n",info[0],info[1], info[2]);
	fprintf(Outfile,"%% For user: %s\n",user);
	fprintf(Outfile,"%% Title: %s\n",g->name);
	fprintf(Outfile,"%%  Put this between beginfig and endfig.  See 1st_read.mp.\n");
	fprintf(Outfile,"%% \n");
}

static  void
mp_end_job(void)
{
	fprintf(Outfile,"%%  End of graphviz MetaPost input\n");
	fprintf(Outfile,"%%  \n");
}

static void
mp_comment(void* obj, attrsym_t* sym)
{
	char	*str;
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"%% %s\n",str);
}

static void
mp_begin_graph(graph_t* g, box bb, point pb)
{
	/* PB = bb; */
	if (onetime) {
		fprintf(Outfile,"%% BoundingBox: %d %d %d %d\n",
			bb.LL.x,bb.LL.y,bb.UR.x+1,bb.UR.y+1);
		mp_comment(g,agfindattr(g,"comment"));
		/*	cat_libfile(Outfile,U_lib,mp_lib); */
		onetime = FALSE;
	}
}

static void
mp_end_graph(void)
{
}

static void
mp_begin_page(point page, double scale, int rot, point offset)
{
	assert(SP == 0);
	S[SP].font = "";
        S[SP].color = "black"; 
        S[SP].size = 0.0;
}

static void
mp_end_page(void)
{
}

static void
mp_begin_cluster(graph_t* g)
{
	Obj = CLST;
}

static void
mp_end_cluster(void)
{
	Obj = NONE;
}

static void
mp_begin_nodes(void)        
{
    Obj = NONE;
}
    
static void
mp_end_nodes(void)  
{
    Obj = NONE;
}

static void 
mp_begin_edges(void)        
{
    Obj = NONE;
}            

static void
mp_end_edges(void)  
{            
    Obj = NONE;
}            


static void
mp_begin_node(node_t* n)
{
	Obj = NODE;
	fprintf(Outfile,"%% GV node: \n%%  %s\n",n->name); 
	mp_comment(n,N_comment);
}

static void
mp_end_node (void)
{
	Obj = NONE;
}

static void
mp_begin_edge (edge_t* e)
{
	Obj = EDGE;
	fprintf(Outfile,"%% GV edge: \n%%  %s -> %s\n",e->tail->name,e->head->name); 
	mp_comment(e,E_comment);
}

static void
mp_end_edge (void)
{
	Obj = NONE;
}

static void
mp_begin_context(void)
{
	if (SP == STACKSIZE - 1) fprintf(stderr,"warning: mpgen stack overflow\n");
	else {SP++; S[SP] = S[SP-1];}
}

static void
mp_end_context(void)
{
	if (SP == 0) fprintf(stderr,"warning: mpgen stack underflow\n");
	else SP--;
}

static void
mp_set_font(char* name, double size)
{
	if (strcmp(S[SP].font,name) || (size != S[SP].size)) {
	        fprintf(Outfile,"%% GV set font: %.2f /%s ignored\n",size,name); 
		S[SP].font = name;
		S[SP].size = size;
	}
}

static void
mp_set_color(char* name)
{
	static char *op[] = {"graph","node","edge","sethsb"};
	char	buf[SMALLBUF];

	if (strcmp(name,S[SP].color))
	       fprintf(Outfile,"%% GV set color: %s %scolor\n",colorxlate(name,buf),op[Obj]); 
	S[SP].color = name;
}

static void
mp_set_style(char** s)
{
	char	*line,*p;

	while ((p = line = *s++)) {
		while (*p) p++; p++;
		while (*p) {
			fprintf(Outfile,"%% GV set style: %s \n",p); 
			while (*p) p++; p++;
		}
		fprintf(Outfile,"%% GV set style:: %s\n",line);
	}
}

static char *
mp_string(char *s, char *auxbuf)
{
	char			*p = auxbuf;
	/* *p++ = LPAREN; */
	while (*s)  {
		if ((*s == LPAREN) || (*s == RPAREN)) *p++ = '\\';
		*p++ = *s++;
	}
	/* *p++ = RPAREN; */
	*p = '\0';
	return auxbuf;
}

static void
mp_textline(point p, char *str, int width, double fontsz, double align)
{
	int		len;
	char	sbuf[2*1024],*buf;

	if ((len = strlen(str)) < sizeof(sbuf)/2) buf = sbuf;
	else buf = (char *)malloc(2*len+1);

	mp_string(str,buf);
	fprintf(Outfile,"label(btex %s etex,(%dbp,%dbp)) withcolor %s;\n",
		buf,p.x,p.y,S[SP].color);
	if (buf != sbuf) free(buf);
}

static void
mp_bezier(point *A, int n, int arrow_at_start, int arrow_at_end)
{
	int		j;
	if (arrow_at_start || arrow_at_end)
		fprintf(stderr," mp_bezier illegal arrow args\n");
	fprintf(Outfile,"draw (%dbp,%dbp) ",A[0].x,A[0].y);
	for (j = 1; j < n; j += 3)
		fprintf(Outfile,"\n  ..controls (%dbp,%dbp) and (%dbp,%dbp).. (%dbp,%dbp)",
			A[j].x,A[j].y,A[j+1].x,A[j+1].y,A[j+2].x,A[j+2].y);
	fprintf(Outfile," withcolor %s;\n",S[SP].color);
}

static void
mp_polygon(point *A, int n, int filled)
{
	int		j;
        if (filled) {
       	  fprintf(Outfile,"  fill (%dbp,%dbp)",A[0].x,A[0].y);
	  for (j = 1; j < n; j++) 
            fprintf(Outfile,"\n  --(%dbp,%dbp)",A[j].x,A[j].y);
	  fprintf(Outfile,"\n  --cycle withcolor %s;\n",S[SP].color);
        }
       	fprintf(Outfile,"draw (%dbp,%dbp)  ",A[0].x,A[0].y);
	for (j = 1; j < n; j++) 
          fprintf(Outfile,"\n  --(%dbp,%dbp)",A[j].x,A[j].y);
	fprintf(Outfile,"\n  --cycle withcolor %s;\n",S[SP].color);
}

static void
mp_ellipse(point p, int rx, int ry, int filled)
{
        if (filled) 
	   fprintf(Outfile,"  fill fullcircle xscaled %dbp yscaled %dbp shifted (%dbp,%dbp) withcolor %s;\n",
                 2*rx,2*ry,p.x,p.y,S[SP].color);
	fprintf(Outfile,"draw fullcircle xscaled %dbp yscaled %dbp shifted (%dbp,%dbp);\n",
                 2*rx,2*ry,p.x,p.y);
}

static void
mp_polyline(point* A, int n)
{
	int		j;

	fprintf(Outfile,"draw (%dbp,%dbp) ",A[0].x,A[0].y);
	for (j = 1; j < n; j ++) fprintf(Outfile,"\n  --(%dbp,%dbp)",A[j].x,A[j].y);
	fprintf(Outfile," withcolor %s;\n",S[SP].color);
}

static void
mp_user_shape(char *name, point *A, int sides, int filled)
{
	int		j;
	fprintf(Outfile,"%%GV USER SHAPE [ ");
	for (j = 0; j < sides; j++) fprintf(Outfile,"%d %d ",A[j].x,A[j].y);
	fprintf(Outfile,"%d %d ",A[0].x,A[0].y);
	fprintf(Outfile,"]  %d %s %s ignored\n",sides,(filled?"true":"false"),name);
}


codegen_t	MP_CodeGen = {
	mp_reset,
	mp_begin_job, mp_end_job,
	mp_begin_graph, mp_end_graph,
	mp_begin_page, mp_end_page,
	mp_begin_cluster, mp_end_cluster,
	mp_begin_nodes, mp_end_nodes,
	mp_begin_edges, mp_end_edges,
	mp_begin_node, mp_end_node,
	mp_begin_edge, mp_end_edge,
	mp_begin_context, mp_end_context,
	mp_set_font, mp_textline,
	mp_set_color, mp_set_style,
	mp_ellipse, mp_polygon,
	mp_bezier, mp_polyline,
	0/* ps_arrowhead */, mp_user_shape,
	mp_comment, 0	/* textsize */
};
