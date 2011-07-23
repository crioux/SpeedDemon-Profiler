/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#ifndef PI
#ifdef M_PI
#define PI M_PI
#else
#define PI 3.14159265358979323846
#endif
#endif

#define SMALLBUF	128
#define LPAREN		'('
#define RPAREN		')'
#define LBRACE		'{'
#define RBRACE		'}'

/*	node,edge types */
#define		NORMAL		0		/* an original input node */
#define		VIRTUAL		1		/* virtual nodes in long edge chains */
#define		SLACKNODE	2		/* encode edges in node position phase */
#define		REVERSED	3		/* reverse of an original edge */
#define		FLATORDER	4		/* for ordered edges */
#define		CLUSTER_EDGE 5		/* for ranking clusters */
#define		IGNORED		6		/* concentrated multi-edges */

/* collapsed node classifications */
#define		NOCMD		0		/* default */
#define		SAMERANK	1		/* place on same rank */
#define		MINRANK		2		/* place on "least" rank */
#define		SOURCERANK	3		/* strict version of MINRANK */
#define		MAXRANK		4		/* place on "greatest" rank */
#define		SINKRANK	5		/* strict version of MAXRANK */
#define		LEAFSET		6		/* set of collapsed leaf nodes */
#define		CLUSTER		7		/* set of clustered nodes */

/* type of cluster rank assignment */
#define		LOCAL		100
#define		GLOBAL		101
#define		NOCLUST		102

/* default attributes */
#define		DEFAULT_COLOR		"black"
#define		DEFAULT_FONTSIZE	14.0
#define		DEFAULT_LABEL_FONTSIZE	11.0 /* for head/taillabel */
#define		MIN_FONTSIZE		1.0
#define		DEFAULT_FONTNAME	"Times-Roman"
#define		DEFAULT_FILL		"lightgrey"
#ifdef _UWIN
#ifndef DEFAULT_FONTPATH
#define		DEFAULT_FONTPATH	"/win/fonts"
#endif
#else
#ifndef MSWIN32
#ifndef DEFAULT_FONTPATH
#define		DEFAULT_FONTPATH	"/usr/share/ttf:/usr/local/share/ttf:/usr/share/fonts/ttf:/usr/local/share/fonts/ttf:/usr/lib/fonts:/usr/local/lib/fonts:/usr/lib/fonts/ttf:/usr/local/lib/fonts/ttf:/usr/common/graphviz/lib/fonts/ttf:/windows/fonts:/dos/windows/fonts:/usr/add-on/share/ttf:."
#endif
#else
#ifndef DEFAULT_FONTPATH
#define		DEFAULT_FONTPATH	"C:/WINDOWS/FONTS;C:/WINNT/Fonts;C:/winnt/fonts"
#endif
#endif
#endif

#define		DEFAULT_NODEHEIGHT	0.5
#define		MIN_NODEHEIGHT		0.02
#define		DEFAULT_NODEWIDTH	0.75
#define		MIN_NODEWIDTH		0.01
#define		DEFAULT_NODESHAPE	"ellipse"
#define		NODENAME_ESC		"\\N"

#define		DEFAULT_NODESEP	0.25
#define		MIN_NODESEP		0.02	
#define		DEFAULT_RANKSEP	0.5
#define		MIN_RANKSEP		0.02

/* default margin for paged formats such as PostScript */
#define		DEFAULT_MARGIN	36

/* default margin for embedded formats such as PNG */
#define		DEFAULT_EMBED_MARGIN_X	10
#define		DEFAULT_EMBED_MARGIN_Y	2

#define		DEFAULT_PAGEHT	792
#define		DEFAULT_PAGEWD	612

#define		SELF_EDGE_SIZE	18
#define		MC_SCALE		256	/* for mincross */

#define		ARROW_LENGTH	10
#define		ARROW_WIDTH		5
/* added by vladimir */
#define		ARROW_INV_LENGTH	6
#define		ARROW_INV_WIDTH		7
#define		ARROW_DOT_RADIUS	2
#define		PORT_LABEL_DISTANCE	10
#define		PORT_LABEL_ANGLE	-25 /* degrees; pos is CCW, neg is CW */

/* added by vladimir */
/* arrow types */
#define ARR_NONE	 0
#define ARR_NORM	 1
#define ARR_INV		 2
#define ARR_DOT		 4
#define ARR_ODOT	 8
#define ARR_INVDOT	 (ARR_INV|ARR_DOT)
#define ARR_INVODOT	 (ARR_INV|ARR_ODOT)

/* sides (e.g. of cluster margins) */
#define		BOTTOM_IX	0
#define		RIGHT_IX	1
#define		TOP_IX		2
#define		LEFT_IX		3

/* sides of boxes for SHAPE_path */
#define		BOTTOM		(1<<BOTTOM_IX)
#define		RIGHT		(1<<RIGHT_IX)
#define		TOP			(1<<TOP_IX)
#define		LEFT		(1<<LEFT_IX)

/* output languages */
#define		ATTRIBUTED_DOT	0		/* default */
#define		POSTSCRIPT	1
#define		HPGL		2
#define		PCL		3
#define		MIF		4
#define		PIC_format	5               /* PIC used by compiler for 
					Position Independent Code */
#define		PLAIN		6

#define		GD		7		/* libgd bitmap format */
#define		GD2		8		/* libgd bitmap format */
#define		GIF		9		/* libgd bitmap format */
#define		JPEG		10		/* libgd bitmap format */
#define		PNG		11		/* libgd bitmap format */
#define		WBMP		12		/* libgd bitmap format */
#define		XBM		13		/* libgd bitmap format */

#define 	ISMAP		14		/* oldstyle map file for httpd servers */
#define 	IMAP		15		/* apache map file for httpd servers */
#define		VRML		16
#define		VTX		17
#define		METAPOST	18
#define		FIG		19
#define		SVG		20
#define		CANONICAL_DOT	21	/* wanted for tcl/tk version */

/* for clusters */
#define		CL_BACK		10		/* cost of backward pointing edge */
#define		CL_OFFSET	8		/* margin of cluster box in PS points */
#ifndef DOS
#define		CL_CROSS	1000	/* cost of cluster skeleton edge crossing */
#else
#define		CL_CROSS	100		/* avoid 16 bit overflow */
#endif

/* for graph server */
#define		SERVER_NN	200
#define		SERVER_NE	500

/* for neato */
#define NDIM            2
#define Spring_coeff    1.0
#define MYHUGE          (1.0e+37)
