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

void dotneato_initialize(int argc, char** argv, char *str)
{
    char		*rest,c;
    int			i,nfiles;

    aginit();
    nfiles = 0;
    for (i = 1; i < argc; i++)
	if (argv[i][0] != '-') nfiles++;
    Files = N_NEW(nfiles + 1, char *);
    Files[nfiles++] = str;
    nfiles = 0;
    CmdName = argv[0];
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    rest = &(argv[i][2]);
	    switch (c = argv[i][1]) {
		case 'G': global_def(rest,agraphattr); break;
		case 'N': global_def(rest,agnodeattr); break;
		case 'E': global_def(rest,agedgeattr); break;
		case 'T': Output_lang = lang_select(rest); break;
		case 'V':
			  fprintf(stderr,"%s version %s (%s)\n",
				  Info[0], Info[1], Info[2]);
			  exit (0);
			  break;
		case 'l':
			  use_library(rest[0]?rest:(*argv[i+1]!='-'?argv[++i]:NULL));
			  break;
		case 'n': 
			  Nop = TRUE; 
			  if (isdigit(*rest)) Nop = atoi (rest);
			  break;
		case 'o':
			  Output_file = file_select(rest[0]?rest:argv[++i]);
			  break;
		case 's':
			  PSinputscale = (rest[0]?atof(rest):POINTS_PER_INCH);
			  break;
		case 'v': Verbose = TRUE; break;
		case 'x': Reduce = TRUE; break;
		default:
			  fprintf(stderr,"%s: option -%c unrecognized\n",CmdName,c);
	    }
	}
	else
	    Files[nfiles++] = argv[i];
    }
    if (Output_file == NULL) Output_file = stdout;
    /* set persistent attributes here */ 
    agnodeattr(NULL,"label",NODENAME_ESC);
}

void global_def(char* dcl, attrsym_t*((*dclfun)(Agraph_t*,char*,char*)))
{
	char		*p,*rhs = "";
	if ((p = strchr(dcl,'='))) { *p++ = '\0'; rhs = p; }
	(void) dclfun(NULL,dcl,rhs);
}

void getfloats2pt(graph_t* g, char* name, point* result)
{
    char        *p;
    int         i;
    double      xf,yf;

    if ((p = agget(g,name))) {
        i = sscanf(p,"%lf,%lf",&xf,&yf);
        if ((i > 1) && (xf > 0) && (yf > 0)) {
            result->x = POINTS(xf);
            result->y = POINTS(yf);
        }
    }
}

void getfloat(graph_t* g, char* name, double* result)
{
    char        *p;
    double      f;

    if ((p = agget(g,name))) {
        if (sscanf(p,"%lf",&f) >= 1) *result = f;
    }
}

graph_t* next_input_graph(void)
{
	graph_t *g;
	g = agmemread(Files[0]);
	return g;
}

void graph_init(graph_t* g)
{
	/* node_t		*n; */
	/* edge_t		*e; */

		/* initialize the graph */
		init_ugraph(g);

		/* initialize nodes */
		N_height = agfindattr(g->proto->n,"height");
		N_width = agfindattr(g->proto->n,"width");
		N_shape = agfindattr(g->proto->n,"shape");
		N_color = agfindattr(g->proto->n,"color");
		N_fillcolor = agfindattr(g->proto->n,"fillcolor");
		N_style = agfindattr(g->proto->n,"style");
		N_fontsize = agfindattr(g->proto->n,"fontsize");
		N_fontname = agfindattr(g->proto->n,"fontname");
		N_fontcolor = agfindattr(g->proto->n,"fontcolor");
		N_label = agfindattr(g->proto->n,"label");
		N_showboxes = agfindattr(g->proto->n,"showboxes");
			/* attribs for polygon shapes */
		N_sides = agfindattr(g->proto->n,"sides");
		N_peripheries = agfindattr(g->proto->n,"peripheries");
		N_skew = agfindattr(g->proto->n,"skew");
		N_orientation = agfindattr(g->proto->n,"orientation");
		N_distortion = agfindattr(g->proto->n,"distortion");
		N_fixed = agfindattr(g->proto->n,"fixedsize");
		N_layer = agfindattr(g->proto->n,"layer");
		N_group = agfindattr(g->proto->n,"group");
		N_comment = agfindattr(g->proto->n,"comment");
		N_vertices = agfindattr(g->proto->n,"vertices");
		N_z = agfindattr(g->proto->n,"z");

		/* initialize edges */
		E_weight = agfindattr(g->proto->e,"weight");
		E_color = agfindattr(g->proto->e,"color");
		E_fontsize = agfindattr(g->proto->e,"fontsize");
		E_fontname = agfindattr(g->proto->e,"fontname");
		E_fontcolor = agfindattr(g->proto->e,"fontcolor");
		E_label = agfindattr(g->proto->e,"label");
        /* vladimir */
		E_dir = agfindattr(g->proto->e,"dir");
		E_arrowhead = agfindattr(g->proto->e,"arrowhead");
		E_arrowtail = agfindattr(g->proto->e,"arrowtail");
		E_headlabel = agfindattr(g->proto->e,"headlabel");
		E_taillabel = agfindattr(g->proto->e,"taillabel");
		E_labelfontsize = agfindattr(g->proto->e,"labelfontsize");
		E_labelfontname = agfindattr(g->proto->e,"labelfontname");
		E_labelfontcolor = agfindattr(g->proto->e,"labelfontcolor");
		E_labeldistance = agfindattr(g->proto->e,"labeldistance");
		E_labelangle = agfindattr(g->proto->e,"labelangle");
        /* end vladimir */
		E_minlen = agfindattr(g->proto->e,"minlen");
		E_showboxes = agfindattr(g->proto->e,"showboxes");
		E_style = agfindattr(g->proto->e,"style");
		E_decorate = agfindattr(g->proto->e,"decorate");
		E_arrowsz = agfindattr(g->proto->e,"arrowsize");
		E_constr = agfindattr(g->proto->e,"constraint");
		E_layer = agfindattr(g->proto->e,"layer");
		E_comment = agfindattr(g->proto->e,"comment");
		E_tailclip = agfindattr(g->proto->e,"tailclip");
		E_headclip = agfindattr(g->proto->e,"headclip");
}

void init_ugraph(graph_t* g)
{
	char		*p;
	int			i;
	double		xf;
	static char *rankname[] = {"local","global","none",NULL};
	static int	rankcode[] =  {LOCAL, GLOBAL, NOCLUST, LOCAL};
	
	g->u.drawing = NEW(layout_t);

	/* set this up fairly early in case any string sizes are needed */
	if ((p = agget(g,"fontpath")) || (p = getenv("DOTFONTPATH"))) {
		/* overide GDFONTPATH in local environment if dot
		 * wants its own */
#ifdef HAVE_SETENV
		setenv("GDFONTPATH", p, 1);
#else
		static char *buf=0;

		buf=(char *)realloc(buf,strlen("GDFONTPATH=")+strlen(p)+1);
		strcpy(buf,"GDFONTPATH=");
		strcat(buf,p);
		putenv(buf);
#endif
	}

	g->u.drawing->quantum = late_float(g,agfindattr(g,"quantum"),0.0,0.0);
 	g->u.drawing->font_scale_adj = 1.0;
	g->u.left_to_right = ((p = agget(g,"rankdir")) && streq(p,"LR"));
	do_graph_label(g);
	xf = late_float(g,agfindattr(g,"nodesep"),DEFAULT_NODESEP,MIN_NODESEP);
	g->u.nodesep = POINTS(xf);

	p = late_string(g,agfindattr(g,"ranksep"),NULL);
	if (p) {
			if (sscanf(p,"%lf",&xf) == 0) xf = DEFAULT_RANKSEP;
			else {if (xf < MIN_RANKSEP) xf = MIN_RANKSEP;}
			if (strstr(p,"equally")) g->u.exact_ranksep = TRUE;
	}
	else xf = DEFAULT_RANKSEP;
	g->u.ranksep = POINTS(xf);

	g->u.showboxes = late_int(g,agfindattr(g,"showboxes"),0,0);

	Epsilon = .0001 * agnnodes(g);
	getfloats2pt(g,"size",&(g->u.drawing->size));
	getfloats2pt(g,"page",&(g->u.drawing->page));
	getfloat(g,"epsilon",&Epsilon);
	getfloat(g,"nodesep",&Nodesep);
	getfloat(g,"nodefactor",&Nodefactor);

	g->u.drawing->centered = mapbool(agget(g,"center"));
	if ((p = agget(g,"rotate"))) g->u.drawing->landscape = (atoi(p) == 90);
	else {		/* today we learned the importance of backward compatibilty */
		if ((p = agget(g,"orientation")))
			g->u.drawing->landscape = ((p[0] == 'l') || (p[0] == 'L'));
	}

	p = agget(g,"clusterrank");
	CL_type = maptoken(p,rankname,rankcode);
	p = agget(g,"concentrate");
	Concentrate = mapbool(p);
	
	Nodesep = 1.0;
	Nodefactor = 1.0;
	Initial_dist = MYHUGE;
}

void free_ugraph(graph_t* g)
{
	free(g->u.drawing);
	g->u.drawing = NULL;
}

void do_graph_label(graph_t* g)
{
    char    *p, *pos;
	int		pos_ix;

	/* it would be nice to allow multiple graph labels in the future */
    if ((p = agget(g,"label"))) {
        g->u.label = make_label(p,
		late_float(g,agfindattr(g,"fontsize"),DEFAULT_FONTSIZE,MIN_FONTSIZE),
		late_nnstring(g,agfindattr(g,"fontname"),DEFAULT_FONTNAME),
		late_nnstring(g,agfindattr(g,"fontcolor"),DEFAULT_COLOR),g);

		pos = agget(g,"labelloc");
		if (!g->u.left_to_right) {
			if (!pos || (pos[0] != 'b')) pos_ix = TOP_IX;
			else pos_ix = BOTTOM_IX;
			g->u.border[pos_ix] = cvt2pt(g->u.label->dimen);
		}
		else {
			/* when rotated, the labels will be restored to TOP or BOTTOM */
			if (!pos || (pos[0] != 'b')) pos_ix = RIGHT_IX;
			else pos_ix = LEFT_IX;
			g->u.border[pos_ix].x = g->u.label->dimen.y;
			g->u.border[pos_ix].y = g->u.label->dimen.x;
		}
	}
}

void dotneato_terminate(void)
{
//    emit_eof();
//    exit(agerrors());
	free(Files);
}

