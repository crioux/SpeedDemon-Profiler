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
    J$: added `pdfmark' URL embedding.  PostScript rendered from
        dot files with URL attributes will get active PDF links
        from Adobe's Distiller.
 */
#define	PDFMAX	3240	/*  Maximum size of Distiller's PDF canvas  */

#include	"render.h"
#include	"ps.h"
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
static	int		Obj,N_pages,Cur_page;
/* static 	point	Pages; */
static	box		PB;
static int		onetime = TRUE;

static char	*Fill = "fill\n";
static char	*Stroke = "stroke\n";
static char	*Newpath_Moveto = "newpath %d %d moveto\n";
static char	**U_lib;

typedef struct grcontext_t {
	char	*color,*font;
	double	size;
} grcontext_t;

#define STACKSIZE 8
static grcontext_t S[STACKSIZE];
static int SP = 0;

static void
ps_reset(void)
{
	onetime = TRUE;
}

static void
ps_begin_job(FILE *ofp,graph_t *g, char **lib, char *user, char *info[],
point pages)
{
	Outfile = ofp;
	/* Pages = pages; */
	U_lib = lib;
		/* wrong when drawing more than one than one graph - use (atend) */
	N_pages = pages.x * pages.y;
	Cur_page = 0;
	fprintf(Outfile,"%%!PS-Adobe-2.0\n");
	fprintf(Outfile,"%%%%Creator: %s version %s (%s)\n",
		info[0], info[1],info[2]);
	fprintf(Outfile,"%%%%For: %s\n",user);
	fprintf(Outfile,"%%%%Title: %s\n",g->name);
	fprintf(Outfile,"%%%%Pages: (atend)\n");
		/* remainder is emitted by first begin_graph */
}

static  void
ps_end_job(void)
{
	fprintf(Outfile,"%%%%Trailer\n");
	fprintf(Outfile,"%%%%Pages: %d\n",Cur_page);
	fprintf(Outfile,"end\nrestore\n");
	fprintf(Outfile,"%%%%EOF\n");
}

static void
ps_comment(void* obj, attrsym_t* sym)
{
	char	*str;
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"%% %s\n",str);
}

static void
ps_begin_graph(graph_t* g, box bb, point pb)
{
	char *s;

	PB = bb;
	if (onetime) {
		fprintf(Outfile,"%%%%BoundingBox: %d %d %d %d\n",
			bb.LL.x-1,bb.LL.y-1,bb.UR.x+1,bb.UR.y+1);
		ps_comment(g,agfindattr(g,"comment"));
		fprintf(Outfile,"%%%%EndComments\n");
		cat_libfile(Outfile,U_lib,ps_txt);
		epsf_define();

 		/*  Set base URL for relative links (for Distiller >= 3.0)  */
 		if ((s = agget(g, "URL")) && strlen(s)) {
 			fprintf(Outfile,
				"[ {Catalog} << /URI << /Base (%s) >> >>\n"
 				"/PUT pdfmark\n", s);

		}
	}
}

static void
ps_end_graph(void)
{
	onetime = FALSE;
}

static void
ps_begin_page(point page, double scale, int rot, point offset)
{
	point	sz;

	Cur_page++;
	sz = sub_points(PB.UR,PB.LL);
    fprintf(Outfile,"%%%%Page: %d %d\n",Cur_page,Cur_page);
    fprintf(Outfile,"%%%%PageBoundingBox: %d %d %d %d\n",
		PB.LL.x,PB.LL.y,PB.UR.x+1,PB.UR.y+1);
	fprintf(Outfile,"%%%%PageOrientation: %s\n",(rot?"Landscape":"Portrait"));
    fprintf(Outfile,"gsave\n%d %d %d %d boxprim clip newpath\n",
		PB.LL.x-1, PB.LL.y-1, sz.x + 2, sz.y + 2);
	fprintf(Outfile,"%d %d translate\n",PB.LL.x,PB.LL.y);
	if (rot) fprintf(Outfile,"gsave %d %d translate %d rotate\n",
		PB.UR.x-PB.LL.x,0,rot);
	fprintf(Outfile,"%d %d %d beginpage\n",page.x,page.y,N_pages);
	if (rot) fprintf(Outfile,"grestore\n");
	if (scale != 1.0) fprintf(Outfile,"%.4f set_scale\n",scale);
	fprintf(Outfile,"%d %d translate %d rotate\n",offset.x,offset.y,rot);
	assert(SP == 0);
	S[SP].font = S[SP].color = ""; S[SP].size = 0.0;

 	/*  Define the size of the PS canvas  */
 	if (PB.UR.x >= PDFMAX || PB.UR.y >= PDFMAX)
 		fprintf(stderr,
		 "dot: warning, canvas size (%d,%d) exceeds PDF limit (%d)\n"
 			"\t(suggest setting a bounding box size, see dot(1))\n",
 			PB.UR.x, PB.UR.y, PDFMAX);
 	fprintf(Outfile,"[ /CropBox [%d %d %d %d] /PAGES pdfmark\n",
 		PB.LL.x, PB.LL.y, PB.UR.x+1, PB.UR.y+1);
}

static void
ps_end_page(void)
{
	fprintf(Outfile,"endpage\ngrestore\n");
	fprintf(Outfile,"%%%%PageTrailer\n");
    fprintf(Outfile,"%%%%EndPage: %d\n",Cur_page);
	assert(SP == 0);
}

static void
ps_begin_cluster(graph_t* g)
{
	fprintf(Outfile,"%% %s\n",g->name);
	Obj = CLST;
}

static void
ps_end_cluster(void)
{
	Obj = NONE;
}

static void
ps_begin_nodes(void)        
{
    Obj = NONE;
}
    
static void
ps_end_nodes(void)  
{
    Obj = NONE;
}

static void 
ps_begin_edges(void)        
{
    Obj = NONE;
}            

static void
ps_end_edges(void)  
{            
    Obj = NONE;
}            

static void
ps_begin_node(node_t* n)
{
        char    *s,*s1,*s2;
        pointf   p1,p2;

	Obj = NODE;
	fprintf(Outfile,"\n%%\t%s\n",n->name);
	ps_comment(n,N_comment);
 
 	/*  Embed information for Distiller, so it can generate hyperactive PDF  */
 	if ((s = agget(n, "URL")) && strlen(s)) {
 		p1.x = n->u.coord.x - n->u.lw;
 		p1.y = n->u.coord.y - (n->u.ht/2);
 		p2.x = n->u.coord.x + n->u.rw;
 		p2.y = n->u.coord.y + (n->u.ht/2);
 
 		/*
 		  Substitute first occurrence of NODENAME_ESC by this node's name
 		 */
 		s = strdup(s);
 		if ((s2 = strstr(s,NODENAME_ESC))) {
 			s1 = n->name;
 			*s2 = '\0';
 			s2 += 2;
 		} else {
 			s1 = s2 = "";
 		}
 
		fprintf(Outfile,"[ /Rect [ %d %d %d %d ]\n"
 				"  /Border [ 0 0 0 ]\n"
 				" /Action << /Subtype /URI /URI (%s%s%s) >>\n"
 				"  /Subtype /Link\n"
 				"/ANN pdfmark\n",
 			ROUND(p1.x), ROUND(p1.y),
 			ROUND(p2.x), ROUND(p2.y),s, s1, s2);
 
 		free(s);
 	}

}

static void
ps_end_node (void)
{
	Obj = NONE;
}

static void
ps_begin_edge (edge_t* e)
{
	Obj = EDGE;
	fprintf(Outfile,"\n%%\t%s -> %s\n",e->tail->name,e->head->name);
	ps_comment(e,E_comment);
}

static void
ps_end_edge (void)
{
	Obj = NONE;
}

static void
ps_begin_context(void)
{
	fprintf(Outfile,"gsave 10 dict begin\n");
	if (SP == STACKSIZE - 1) fprintf(stderr,"warning: psgen stk ovfl\n");
	else {SP++; S[SP] = S[SP-1];}
}

static void
ps_end_context(void)
{
	if (SP == 0) fprintf(stderr,"warning: psgen stk undfl\n");
	else SP--;
	fprintf(Outfile,"end grestore\n");
}

static void
ps_set_font(char* name, double size)
{
	if (strcmp(S[SP].font,name) || (size != S[SP].size)) {
		fprintf(Outfile,"%.2f /%s set_font\n",size,name);
		S[SP].font = name;
		S[SP].size = size;
	}
}

/***** Arrowheads now centralized in emit.c
static void
ps_arrowhead(point p, double theta, double scale, int type)
{
  fprintf(Outfile,"%d %d %.2f ", p.x,p.y,theta);
  switch (type) {
  case ARR_NORM:
    fprintf(Outfile,"%.2f %.2f arrowhead\n",
            scale*ARROW_LENGTH,scale*ARROW_WIDTH);
    break;
  case ARR_INV:
    fprintf(Outfile,"%.2f %.2f arrowinv\n",
            scale*ARROW_INV_LENGTH,scale*ARROW_INV_WIDTH);
    break;
  case ARR_DOT:
    fprintf(Outfile,"%.2f arrowdot\n",
            scale*ARROW_DOT_RADIUS);
    break;
  case ARR_ODOT:
    fprintf(Outfile,"%.2f arrowodot\n",
            scale*ARROW_DOT_RADIUS);
    break;
  case ARR_INVDOT:
    fprintf(Outfile,"%.2f %.2f %.2f arrowinvdot\n",
            scale*ARROW_INV_LENGTH,scale*ARROW_INV_WIDTH,
            scale*ARROW_DOT_RADIUS);
    break;
  case ARR_INVODOT:
    fprintf(Outfile,"%.2f %.2f %.2f arrowinvodot\n",
            scale*ARROW_INV_LENGTH,scale*ARROW_INV_WIDTH,
            scale*ARROW_DOT_RADIUS);
  }
}
**********/

static void
ps_set_color(char* name)
{
	static char *op[] = {"graph","node","edge","sethsb"};
	char	buf[SMALLBUF];

	if (strcmp(name,S[SP].color))
		fprintf(Outfile,"%s %scolor\n",colorxlate(name,buf),op[Obj]);
	S[SP].color = name;
}

static void
ps_set_style(char** s)
{
	char	*line,*p;

	while ((p = line = *s++)) {
		while (*p) p++; p++;
		while (*p) {
			fprintf(Outfile,"%s ",p);
			while (*p) p++; p++;
		}
		fprintf(Outfile,"%s\n",line);
	}
}

static char *
ps_string(char *s, char *auxbuf)
{
	char			*p = auxbuf;
	*p++ = LPAREN;
	while (*s)  {
		if ((*s == LPAREN) || (*s == RPAREN)) *p++ = '\\';
		*p++ = *s++;
	}
	*p++ = RPAREN;
	*p = '\0';
	return auxbuf;
}

static void
ps_textline(point p, char *str, int width, double fontsz, double align)
{
	int		len;
	char	sbuf[2*1024],*buf;

	if ((len = strlen(str)) < sizeof(sbuf)/2) buf = sbuf;
	else buf = malloc(2*len+1);

	ps_string(str,buf);
	fprintf(Outfile,"%d %d moveto %s %d %.2f %.2f alignedtext\n",
		p.x,p.y,buf,width,fontsz,align);
	if (buf != sbuf) free(buf);
}

static void
ps_bezier(point *A, int n, int arrow_at_start, int arrow_at_end)
{
	int		j;
	if (arrow_at_start || arrow_at_end)
		fprintf(stderr,"ps_bezier illegal arrow args\n");
	fprintf(Outfile,Newpath_Moveto,A[0].x,A[0].y);
	for (j = 1; j < n; j += 3)
		fprintf(Outfile,"%d %d %d %d %d %d curveto\n",
			A[j].x,A[j].y,A[j+1].x,A[j+1].y,A[j+2].x,A[j+2].y);
	fprintf(Outfile,Stroke);
}

static void
ps_polygon(point *A, int n, int filled)
{
	int		j;
	fprintf(Outfile,Newpath_Moveto,A[0].x,A[0].y);
	for (j = 1; j < n; j++) fprintf(Outfile,"%d %d lineto\n",A[j].x,A[j].y);
	fprintf(Outfile,"closepath\n");
	fprintf(Outfile, filled? Fill : Stroke);
}

static void
ps_ellipse(point p, int rx, int ry, int filled)
{
	fprintf(Outfile,"%d %d %d %d ellipse_path\n",p.x,p.y,rx,ry);
	fprintf(Outfile, filled? Fill : Stroke);
}

static void
ps_polyline(point* A, int n)
{
	int		j;

	fprintf(Outfile,Newpath_Moveto,A[0].x,A[0].y);
	for (j = 1; j < n; j ++) fprintf(Outfile,"%d %d lineto\n",A[j].x,A[j].y);
	fprintf(Outfile,Stroke);
}

static void
ps_user_shape(char *name, point *A, int sides, int filled)
{
	int		j;
	fprintf(Outfile,"[ ");
	for (j = 0; j < sides; j++) fprintf(Outfile,"%d %d ",A[j].x,A[j].y);
	fprintf(Outfile,"%d %d ",A[0].x,A[0].y);
	fprintf(Outfile,"]  %d %s %s\n",sides,(filled?"true":"false"),name);
}

#define N_EPSF 32
static int  N_EPSF_files;
static char *EPSF_contents[N_EPSF];

typedef struct epsf_s {
	int		macro_id;
	point	offset;
} epsf_t;

void epsf_init(node_t* n)
{
	char	*str,*contents;
	char	line[BUFSIZ];
	FILE	*fp;
	struct stat statbuf;
	int		i, saw_bb;
	int		lx,ly,ux,uy;
	epsf_t	*desc;

	str = agget(n,"shapefile");
	if (str && str[0] && (fp = fopen(str,"r"))) {
		/* try to find size */
		saw_bb = FALSE;
		while (fgets(line, sizeof(line), fp)) {
		  if (sscanf(line,"%%%%BoundingBox: %d %d %d %d",&lx,&ly,&ux,&uy) == 4) {
			saw_bb = TRUE;
			break;
		  }
		}

		if (saw_bb) {
			n->u.width = PS2INCH(ux - lx);
			n->u.height = PS2INCH(uy - ly);
			fstat(fileno(fp),&statbuf);
			i = N_EPSF_files++;
			n->u.shape_info = desc = NEW(epsf_t);
			desc->macro_id = i;
			desc->offset.x = -lx - (ux - lx)/2;
			desc->offset.y = -ly - (uy - ly)/2;
			contents = EPSF_contents[i] = malloc(statbuf.st_size);
			fseek(fp,0,SEEK_SET);
			fread(contents,statbuf.st_size,1,fp);
			fclose(fp);
		}
	}
}

void epsf_free(node_t* n)
{
/* FIXME  What about the allocated EPSF_contents[i] ? */

	free(n->u.shape_info);
}

void epsf_gencode(node_t *n)
{
	epsf_t	*desc;

	desc = (epsf_t*)(n->u.shape_info);
	CodeGen->begin_node(n);
	CodeGen->begin_context();
	fprintf(Outfile,"%d %d translate newpath user_shape_%d\n",
		n->u.coord.x+desc->offset.x,n->u.coord.y+desc->offset.y,
		desc->macro_id);
	CodeGen->end_context();
	CodeGen->end_node();
}

void epsf_define(void)
{
	int		i;
	char	*p;

	for (i = 0; i < N_EPSF_files; i++) {
		fprintf(Outfile,"/user_shape_%d {",i);
		p = EPSF_contents[i];
		while (*p) {
			if ((p[0] == '%') && (p[1] == '%') && (p[1] != '!')) {
				while (*p++ != '\n');
				continue;
			}
			do {fputc(*p,Outfile);} while (*p++ != '\n');
		}
		free(EPSF_contents[i]);
		fprintf(Outfile,"} bind def\n");
	}
	N_EPSF_files = 0;
}

codegen_t	PS_CodeGen = {
	ps_reset,
	ps_begin_job, ps_end_job,
	ps_begin_graph, ps_end_graph,
	ps_begin_page, ps_end_page,
	ps_begin_cluster, ps_end_cluster,
	ps_begin_nodes, ps_end_nodes,
	ps_begin_edges, ps_end_edges,
	ps_begin_node, ps_end_node,
	ps_begin_edge, ps_end_edge,
	ps_begin_context, ps_end_context,
	ps_set_font, ps_textline,
	ps_set_color, ps_set_style,
	ps_ellipse, ps_polygon,
	ps_bezier, ps_polyline,
	0/* ps_arrowhead */, ps_user_shape,
	ps_comment, 0	/* textsize */
};
