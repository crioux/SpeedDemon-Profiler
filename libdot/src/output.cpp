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

int        Output_lang;
codegen_t  *CodeGen;
FILE       *Output_file;

#include <string>
void printptf(std::string *str, point pt);
void write_plainstr(graph_t* g, std::string *str);

void dotneato_write(graph_t* g, std::string *output_str)
{
	double	xf, yf;
	char	*p;
	int	i;

	/* margins */
	if ((p = agget(g,"margin"))) {
		i = sscanf(p,"%lf,%lf",&xf,&yf);
		if (i > 0) g->u.drawing->margin.x = g->u.drawing->margin.y = POINTS(xf);
		if (i > 1) g->u.drawing->margin.y = POINTS(yf);
	}
	else {
		/* set default margins depending on format */
		switch (Output_lang) {
			case GIF:
			case PNG:
			case JPEG:
			case GD:
			case GD2:
			case ISMAP:
			case IMAP:
			case VRML:
			case SVG:
	        		g->u.drawing->margin.x = DEFAULT_EMBED_MARGIN_X;
				g->u.drawing->margin.y = DEFAULT_EMBED_MARGIN_Y;
				break;
			case POSTSCRIPT:
			case HPGL:
			case PCL:
			case MIF:
			case METAPOST:
			case FIG:
			case VTX:
			case ATTRIBUTED_DOT:
			case PLAIN:		
	        		g->u.drawing->margin.x = g->u.drawing->margin.y = DEFAULT_MARGIN;
				break;
			case CANONICAL_DOT:
				break;
		}
	}

	switch (Output_lang) {
		case POSTSCRIPT:
		case HPGL:
		case PCL:
		case MIF:
		case GIF:
		case PNG:
		case JPEG:
		case GD:
		case GD2:
		case ISMAP:
		case IMAP:
		case VRML:
		case METAPOST:
		case FIG:
		case SVG:
			/* output in breadth first graph walk order */
			emit_graph(g,0); break;
		case VTX:
			/* output sorted, i.e. all nodes then all edges */
			emit_graph(g,1); break;
		case ATTRIBUTED_DOT:
			attach_attrs(g);
			agwrite(g,Output_file); break;
		case CANONICAL_DOT:
			agwrite(g,Output_file); break;
		case PLAIN:
			attach_attrs(g);
			if (output_str)
			    write_plainstr(g,output_str);
			else
			    write_plain(g,Output_file);
			break;
	}
	fflush(Output_file);
}

static char *rectbufp;
static void set_record_rects (node_t* n, field_t* f)
{
	int             i;

	if (f->n_flds == 0) {
		sprintf(rectbufp, "%d,%d,%d,%d ",
				f->b.LL.x + n->u.coord.x,
				f->b.LL.y + n->u.coord.y,
				f->b.UR.x + n->u.coord.x,
				f->b.UR.y + n->u.coord.y);
		while (*rectbufp) rectbufp++;
	}
	for (i = 0; i < f->n_flds; i++)
		set_record_rects (n, f->fld[i]);
}

static attrsym_t *safe_dcl(graph_t *g, void *obj, char *name, char *def,
	attrsym_t*(*fun)(Agraph_t*, char*, char*))
{
	attrsym_t	*a = agfindattr(obj,name);
	if (a == NULL) a = fun(g,name,def);
	return a;
}

void attach_attrs(graph_t* g)
{
	int		i,j,sides;
	char	buf[BUFSIZ],*p;
	node_t	*n;
	edge_t	*e;
	point	pt;

	safe_dcl(g,g->proto->n,"pos","",agnodeattr);
	safe_dcl(g,g->proto->n,"rects","",agnodeattr);
	N_width = safe_dcl(g,g->proto->n,"width","",agnodeattr);
	N_height = safe_dcl(g,g->proto->n,"height","",agnodeattr);
	safe_dcl(g,g->proto->e,"pos","",agedgeattr);
	if (g->u.has_edge_labels) safe_dcl(g,g->proto->e,"lp","",agedgeattr);
	if (g->u.label) {
		safe_dcl(g,g,"lp","",agraphattr);
		pt = g->u.label->p;
		sprintf(buf,"%d,%d",pt.x,pt.y);
		agset(g,"lp",buf);
	}
	safe_dcl(g,g,"bb","",agraphattr);
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		sprintf(buf,"%d,%d",n->u.coord.x,n->u.coord.y);
		agset(n,"pos",buf);
		sprintf(buf,"%.2f",PS2INCH(n->u.ht));
		agxset(n,N_height->index,buf);
		sprintf(buf,"%.2f",PS2INCH(n->u.lw + n->u.rw));
		agxset(n,N_width->index,buf);
		if (strcmp (n->u.shape->name, "record") == 0) {
			buf[0] = '\000', rectbufp = &buf[0];
			set_record_rects (n, (field_t *)(n->u.shape_info));
			if (rectbufp > &buf[0]) /* get rid of last space */
				*(--rectbufp) = '\000';
			agset(n,"rects",buf);
		}
		else {
			extern void	poly_init(node_t *);
			polygon_t *poly;
			int i;
			if (N_vertices && (n->u.shape->initfn == poly_init)) {
				poly = (polygon_t*) n->u.shape_info;
				p = buf; 
				sides = poly->sides;
				if (sides < 3) {
					char *p = agget(n,"samplepoints");
					if (p) sides = atoi(p);
					else sides = 8;
					if (sides < 3) sides = 8;
				}
				for (i = 0; i < sides; i++) {
					if (i > 0) {*p++ = ' ';}
					if (poly->sides >= 3)
						sprintf(p,"%.3lf %.3lf",
							poly->vertices[i].x,poly->vertices[i].y);
					else
						sprintf(p,"%.3lf %.3lf",
							n->u.width/2.0 * cos(i/(double)sides * PI * 2.0),
							n->u.height/2.0 * sin(i/(double)sides * PI * 2.0));
					while (*p) p++;
				}
				agxset(n,N_vertices->index,buf);
			}
		}
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
			p = buf;
if (e->u.spl == NULL)
	{fprintf(stderr,"lost spline of %s %s\n",e->tail->name,e->head->name); continue;}
			for (i = 0; i < e->u.spl->size; i++) {
				if (i > 0) *p++ = ';';
				if (e->u.spl->list[i].sflag) {
					sprintf (p, "s,%d,%d ",e->u.spl->list[i].sp.x,e->u.spl->list[i].sp.y);
					while (*p) p++;
				}
				if (e->u.spl->list[i].eflag) {
					sprintf (p, "e,%d,%d ",e->u.spl->list[i].ep.x,e->u.spl->list[i].ep.y);
					while (*p) p++;
				}
				for (j = 0; j < e->u.spl->list[i].size; j++) {
					if (j > 0) *p++ = ' ';
					pt = e->u.spl->list[i].list[j];
					sprintf(p,"%d,%d",pt.x,pt.y);
					while (*p) p++;
				}
				*p = '\0';
			}
			agset(e,"pos",buf);
			if (e->u.label) {
				pt = e->u.label->p;
				sprintf(buf,"%d,%d",pt.x,pt.y);
				agset(e,"lp",buf);
			}
		}
	}
	rec_attach_bb(g);
}

void rec_attach_bb(graph_t* g)
{
	int		c;
	char	buf[32];

	sprintf(buf,"%d,%d,%d,%d", g->u.bb.LL.x, g->u.bb.LL.y,
		g->u.bb.UR.x, g->u.bb.UR.y);
	agset(g,"bb",buf);
	for (c = 1; c <= g->u.n_cluster; c++) rec_attach_bb(g->u.clust[c]);
}

void write_plain(graph_t* g, FILE* f)
{
    std::string s;
    write_plainstr(g, &s);
    fprintf(f, "%s", s.c_str());
}

void write_plainstr(graph_t* g, std::string *str)
{
	int			i;
	node_t		*n;
	edge_t		*e;
	bezier		bz;
	char		buf[SMALLBUF],buf1[SMALLBUF];

	char tmpbuf[1000];

	setup_graph(g);

	sprintf(tmpbuf, "graph %.3f", g->u.drawing->scale);
	str->append(tmpbuf);
	printptf(str, g->u.bb.UR);
	str->append("\n");

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
	    str->append("node ");
	    str->append(agstrcanon(n->name,buf));
	    printptf(str,n->u.coord);
	    sprintf(tmpbuf, " %.3f", n->u.width);
	    str->append(tmpbuf);
	    sprintf(tmpbuf, " %.3f ", n->u.height);
	    str->append(tmpbuf);
	    str->append(agstrcanon(n->u.label->text,buf));
	    str->append(" ");
	    str->append(late_nnstring(n,N_style,"solid"));
	    str->append(" ");
	    str->append(n->u.shape->name);
	    str->append(" ");
	    str->append(late_nnstring(n,N_color,DEFAULT_COLOR));
	    str->append(" ");
	    str->append(late_nnstring(n,N_fillcolor,DEFAULT_FILL));
	    str->append("\n");
	}
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
			bz = e->u.spl->list[0];
			str->append("edge ");
			str->append(agstrcanon(e->tail->name,buf));
			str->append(" ");
			str->append(agstrcanon(e->head->name,buf1));
			str->append(" ");
			sprintf(tmpbuf, " %d", bz.size);
			str->append(tmpbuf);

			for (i = 0; i < bz.size; i++) printptf(str,bz.list[i]);
			if (e->u.label) {
			    str->append(" ");
			    str->append(agstrcanon(e->u.label->text,buf));
			    printptf(str,e->u.label->p);
			}
			str->append(" ");
			str->append(late_nnstring(e,E_style,"solid"));
			str->append(" ");
			str->append(late_nnstring(e,E_color,DEFAULT_COLOR));
			str->append("\n");
		}
	}
	str->append("stop\n");
}


void printptf(std::string *str, point pt)
{
	char tmpbuf[1000];
	sprintf(tmpbuf," %.3f %.3f",PS2INCH(pt.x),PS2INCH(pt.y));
	str->append(tmpbuf);
}

void printptf(FILE* f, point pt)
{
	fprintf(f," %.3f %.3f",PS2INCH(pt.x),PS2INCH(pt.y));
}

int codegen_bezier_has_arrows(void)
{
	return CodeGen && 
      CodeGen->bezier_has_arrows
      /* (CodeGen->arrowhead == 0)) */;
}
