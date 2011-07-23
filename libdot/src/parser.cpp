
/*  A Bison parser, made from parser.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	T_graph	257
#define	T_digraph	258
#define	T_strict	259
#define	T_node	260
#define	T_edge	261
#define	T_edgeop	262
#define	T_symbol	263
#define	T_subgraph	264

#line 1 "parser.y"

/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include	"libgraph.h"

static char		Port[SMALLBUF],*Symbol;
static char		In_decl,In_edge_stmt;
static int		Current_class,Agraph_type;
static Agraph_t		*G;
static Agnode_t		*N;
static Agedge_t		*E;
static objstack_t	*SP;
static Agraph_t		*Gstack[32];
static int			GSP;

static void push_subg(Agraph_t *g)
{
	G = Gstack[GSP++] = g;
}

static Agraph_t *pop_subg(void)
{
	Agraph_t		*g;
	if (GSP == 0) {
		fprintf(stderr,"Gstack underflow in graph parser\n"); exit(1);
	}
	g = Gstack[--GSP];					/* graph being popped off */
	if (GSP > 0) G = Gstack[GSP - 1];	/* current graph */
	else G = 0;
	return g;
}

static objport_t pop_gobj(void)
{
	objport_t	rv;
	rv.obj = pop_subg();
	rv.port = NULL;
	return rv;
}

static void begin_graph(char *name)
{
	Agraph_t		*g;
	g = AG.parsed_g = agopen(name,Agraph_type);
	push_subg(g);
	In_decl = TRUE;
}

static void end_graph(void)
{
	pop_subg();
}

static Agnode_t *bind_node(char *name)
{
	Agnode_t	*n = agnode(G,name);
	In_decl = FALSE;
	return n;
}

static void anonsubg(void)
{
	static int		anon_id = 0;
	char			buf[SMALLBUF];
	Agraph_t			*subg;

	In_decl = FALSE;
	sprintf(buf,"_anonymous_%d",anon_id++);
	subg = agsubg(G,buf);
	push_subg(subg);
}

static int isanonsubg(Agraph_t *g)
{
	return (strncmp("_anonymous_",g->name,11) == 0);
}

static void begin_edgestmt(objport_t objp)
{
	struct objstack_t	*new_sp;

	new_sp = NEW(objstack_t);
	new_sp->link = SP;
	SP = new_sp;
	SP->list = SP->last = NEW(objlist_t);
	SP->list->data  = objp;
	SP->list->link = NULL;
	SP->in_edge_stmt = In_edge_stmt;
	SP->subg = G;
	agpushproto(G);
	In_edge_stmt = TRUE;
}

static void mid_edgestmt(objport_t objp)
{
	SP->last->link = NEW(objlist_t);
	SP->last = SP->last->link;
	SP->last->data = objp;
	SP->last->link = NULL;
}

static void end_edgestmt(void)
{
	objstack_t	*old_SP;
	objlist_t	*tailptr,*headptr,*freeptr;
	Agraph_t		*t_graph,*h_graph;
	Agnode_t	*t_node,*h_node,*t_first,*h_first;
	Agedge_t	*e;
	char		*tport,*hport;

	for (tailptr = SP->list; tailptr->link; tailptr = tailptr->link) {
		headptr = tailptr->link;
		tport = tailptr->data.port;
		hport = headptr->data.port;
		if (TAG_OF(tailptr->data.obj) == TAG_NODE) {
			t_graph = NULL;
			t_first = (Agnode_t*)(tailptr->data.obj);
		}
		else {
			t_graph = (Agraph_t*)(tailptr->data.obj);
			t_first = agfstnode(t_graph);
		}
		if (TAG_OF(headptr->data.obj) == TAG_NODE) {
			h_graph = NULL;
			h_first = (Agnode_t*)(headptr->data.obj);
		}
		else {
			h_graph = (Agraph_t*)(headptr->data.obj);
			h_first = agfstnode(h_graph);
		}

		for (t_node = t_first; t_node; t_node = t_graph ?
		  agnxtnode(t_graph,t_node) : NULL) {
			for (h_node = h_first; h_node; h_node = h_graph ?
			  agnxtnode(h_graph,h_node) : NULL ) {
				e = agedge(G,t_node,h_node);
				if (e) {
					char	*tp = tport;
					char 	*hp = hport;
					if ((e->tail != e->head) && (e->head == t_node)) {
						/* could happen with an undirected edge */
						char 	*temp;
						temp = tp; tp = hp; hp = temp;
					}
					if (tp && tp[0]) agxset(e,TAILX,tp);
					if (hp && hp[0]) agxset(e,HEADX,hp);
				}
			}
		}
	}
	tailptr = SP->list; 
	while (tailptr) {
		freeptr = tailptr;
		tailptr = tailptr->link;
		if (TAG_OF(freeptr->data.obj) == TAG_NODE)
		free(freeptr->data.port);
		free(freeptr);
	}
	if (G != SP->subg) abort();
	agpopproto(G);
	In_edge_stmt = SP->in_edge_stmt;
	old_SP = SP;
	SP = SP->link;
	In_decl = FALSE;
	free(old_SP);
	Current_class = TAG_GRAPH;
}

static Agraph_t *parent_of(Agraph_t *g)
{
	Agraph_t		*rv;
	rv = agusergraph(agfstin(g->meta_node->graph,g->meta_node)->tail);
	return rv;
}

static void attr_set(char *name, char *value)
{
	Agsym_t		*ap = NULL;
	char		*defval = "";

	if (In_decl && (G->root == G)) defval = value;
	switch (Current_class) {
		case TAG_NODE:
			ap = agfindattr(G->proto->n,name);
			if (ap == NULL)
				ap = agnodeattr(AG.parsed_g,name,defval);
			agxset(N,ap->index,value);
			break;
		case TAG_EDGE:
			ap = agfindattr(G->proto->e,name);
			if (ap == NULL)
				ap = agedgeattr(AG.parsed_g,name,defval);
			agxset(E,ap->index,value);
			break;
		case 0:		/* default */
		case TAG_GRAPH:
			ap = agfindattr(G,name);
			if (ap == NULL) 
				ap = agraphattr(AG.parsed_g,name,defval);
			agxset(G,ap->index,value);
			break;
	}
}


#line 216 "parser.y"
typedef union	{
			int					i;
			char				*str;
			struct objport_t	obj;
			struct Agnode_t		*n;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		90
#define	YYFLAG		-32768
#define	YYNTBASE	22

#define YYTRANSLATE(x) ((unsigned)(x) <= 264 ? agtranslate[x] : 57)

static const char agtranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    19,
    20,     2,     2,    13,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    18,    17,     2,
    16,     2,     2,    21,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    14,     2,    15,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    11,     2,    12,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10
};

#if YYDEBUG != 0
static const short agprhs[] = {     0,
     0,     1,     8,    10,    11,    13,    16,    18,    21,    23,
    25,    27,    31,    32,    33,    35,    39,    42,    43,    45,
    49,    51,    52,    54,    57,    59,    62,    64,    66,    68,
    70,    72,    75,    77,    80,    82,    83,    85,    87,    90,
    93,    96,    97,   105,   108,   109,   113,   114,   115,   121,
   122,   123,   129,   132,   133,   138,   141,   142,   147,   152,
   153,   158,   160,   163
};

static const short agrhs[] = {    -1,
    24,    56,    23,    11,    32,    12,     0,     1,     0,     0,
     3,     0,     5,     3,     0,     4,     0,     5,     4,     0,
     3,     0,     6,     0,     7,     0,    31,    27,    26,     0,
     0,     0,    13,     0,    14,    26,    15,     0,    29,    28,
     0,     0,    29,     0,    56,    16,    56,     0,    33,     0,
     0,    34,     0,    33,    34,     0,    35,     0,    35,    17,
     0,     1,     0,    43,     0,    45,     0,    36,     0,    53,
     0,    25,    28,     0,    31,     0,    38,    39,     0,    56,
     0,     0,    40,     0,    42,     0,    42,    40,     0,    40,
    42,     0,    18,    56,     0,     0,    18,    19,    56,    41,
    13,    56,    20,     0,    21,    56,     0,     0,    37,    44,
    30,     0,     0,     0,    37,    46,    50,    47,    30,     0,
     0,     0,    53,    48,    50,    49,    30,     0,     8,    37,
     0,     0,     8,    37,    51,    50,     0,     8,    53,     0,
     0,     8,    53,    52,    50,     0,    55,    11,    32,    12,
     0,     0,    11,    54,    32,    12,     0,    55,     0,    10,
    56,     0,     9,     0
};

#endif

#if YYDEBUG != 0
static const short agrline[] = { 0,
   233,   235,   237,   244,   248,   250,   252,   254,   258,   260,
   262,   266,   267,   270,   271,   273,   275,   276,   279,   282,
   286,   287,   290,   291,   294,   295,   296,   299,   300,   301,
   302,   305,   307,   311,   321,   324,   325,   326,   327,   328,
   331,   332,   332,   339,   346,   349,   352,   355,   358,   359,
   362,   365,   368,   369,   372,   372,   374,   377,   380,   381,
   381,   382,   385,   395
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const agtname[] = {   "$","error","$undefined.","T_graph",
"T_digraph","T_strict","T_node","T_edge","T_edgeop","T_symbol","T_subgraph",
"'{'","'}'","','","'['","']'","'='","';'","':'","'('","')'","'@'","file","@1",
"graph_type","attr_class","inside_attr_list","optcomma","attr_list","rec_attr_list",
"opt_attr_list","attr_set","stmt_list","stmt_list1","stmt","stmt1","attr_stmt",
"node_id","node_name","node_port","port_location","@2","port_angle","node_stmt",
"@3","edge_stmt","@4","@5","@6","@7","edgeRHS","@8","@9","subg_stmt","@10","subg_hdr",
"symbol", NULL
};
#endif

static const short agr1[] = {     0,
    23,    22,    22,    22,    24,    24,    24,    24,    25,    25,
    25,    26,    26,    27,    27,    28,    29,    29,    30,    31,
    32,    32,    33,    33,    34,    34,    34,    35,    35,    35,
    35,    36,    36,    37,    38,    39,    39,    39,    39,    39,
    40,    41,    40,    42,    44,    43,    46,    47,    45,    48,
    49,    45,    50,    51,    50,    50,    52,    50,    53,    54,
    53,    53,    55,    56
};

static const short agr2[] = {     0,
     0,     6,     1,     0,     1,     2,     1,     2,     1,     1,
     1,     3,     0,     0,     1,     3,     2,     0,     1,     3,
     1,     0,     1,     2,     1,     2,     1,     1,     1,     1,
     1,     2,     1,     2,     1,     0,     1,     1,     2,     2,
     2,     0,     7,     2,     0,     3,     0,     0,     5,     0,
     0,     5,     2,     0,     4,     2,     0,     4,     4,     0,
     4,     1,     2,     1
};

static const short agdefact[] = {     0,
     3,     5,     7,     0,     0,     6,     8,    64,     1,     0,
     0,    27,     9,    10,    11,     0,    60,     0,    33,     0,
     0,    23,    25,    30,    45,    36,    28,    29,    31,    62,
    35,    63,     0,    13,    32,     2,    24,    26,    18,     0,
     0,     0,    34,    37,    38,     0,     0,     0,     0,     0,
    14,     0,    19,    46,     0,    48,     0,    41,    44,    40,
    39,    51,     0,    20,    61,    16,    15,    13,    17,    53,
    56,    35,    18,    42,    18,    59,    12,     0,     0,    49,
     0,    52,    55,    58,     0,     0,    43,     0,     0,     0
};

static const short agdefgoto[] = {    88,
    10,     5,    18,    50,    68,    35,    53,    54,    19,    20,
    21,    22,    23,    24,    25,    26,    43,    44,    81,    45,
    27,    39,    28,    40,    73,    46,    75,    56,    78,    79,
    29,    33,    30,    31
};

static const short agpact[] = {     4,
-32768,-32768,-32768,    30,     1,-32768,-32768,-32768,-32768,     8,
    11,-32768,-32768,-32768,-32768,     1,-32768,    26,-32768,     3,
    48,-32768,    24,-32768,    17,     6,-32768,-32768,    34,    33,
    29,-32768,    11,     1,-32768,-32768,-32768,-32768,-32768,    38,
    -6,     1,-32768,    27,    35,    38,    11,     1,    44,    32,
    49,    29,    26,-32768,    21,-32768,     1,-32768,-32768,-32768,
-32768,-32768,    52,-32768,-32768,-32768,-32768,     1,-32768,    53,
    57,-32768,-32768,-32768,-32768,-32768,-32768,    38,    38,-32768,
    54,-32768,-32768,-32768,     1,    46,-32768,    68,    69,-32768
};

static const short agpgoto[] = {-32768,
-32768,-32768,-32768,     2,-32768,    18,-32768,   -47,   -33,   -31,
-32768,    51,-32768,-32768,    19,-32768,-32768,    28,-32768,    31,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -40,-32768,-32768,
    22,-32768,-32768,    -5
};


#define	YYLAST		80


static const short agtable[] = {     9,
    51,    49,     8,    -4,     1,    62,     2,     3,     4,     8,
    32,    12,    57,    13,    36,    63,    14,    15,    11,     8,
    16,    17,   -22,    41,   -47,    80,    42,    82,    52,     8,
    16,    17,     6,     7,    51,    58,    59,    83,    84,    34,
    38,   -50,    64,    47,    48,    55,    66,    42,    12,    72,
    13,    74,    41,    14,    15,    65,     8,    16,    17,   -21,
   -54,    67,    52,    76,   -57,    87,    85,    89,    90,    77,
    69,    37,    61,    70,    60,     0,    71,     0,     0,    86
};

static const short agcheck[] = {     5,
    34,    33,     9,     0,     1,    46,     3,     4,     5,     9,
    16,     1,    19,     3,    12,    47,     6,     7,    11,     9,
    10,    11,    12,    18,     8,    73,    21,    75,    34,     9,
    10,    11,     3,     4,    68,    41,    42,    78,    79,    14,
    17,     8,    48,    11,    16,     8,    15,    21,     1,    55,
     3,    57,    18,     6,     7,    12,     9,    10,    11,    12,
     8,    13,    68,    12,     8,    20,    13,     0,     0,    68,
    53,    21,    45,    55,    44,    -1,    55,    -1,    -1,    85
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define agerrok		(agerrstatus = 0)
#define agclearin	(agchar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto agacceptlab
#define YYABORT 	goto agabortlab
#define YYERROR		goto agerrlab1
/* Like YYERROR except do call agerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto agerrlab
#define YYRECOVERING()  (!!agerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (agchar == YYEMPTY && aglen == 1)				\
    { agchar = (token), aglval = (value);			\
      agchar1 = YYTRANSLATE (agchar);				\
      YYPOPSTACK;						\
      goto agbackup;						\
    }								\
  else								\
    { agerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		aglex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		aglex(&aglval, &aglloc, YYLEX_PARAM)
#else
#define YYLEX		aglex(&aglval, &aglloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		aglex(&aglval, YYLEX_PARAM)
#else
#define YYLEX		aglex(&aglval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	agchar;			/*  the lookahead symbol		*/
YYSTYPE	aglval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE aglloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int agnerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int agdebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __ag_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __ag_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__ag_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__ag_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into agparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int agparse (void *);
#else
int agparse (void);
#endif
#endif

int
agparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int agstate;
  register int agn;
  register short *agssp;
  register YYSTYPE *agvsp;
  int agerrstatus;	/*  number of tokens to shift before error messages enabled */
  int agchar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	agssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE agvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *agss = agssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *agvs = agvsa;	/*  to allow agoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE aglsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *agls = aglsa;
  YYLTYPE *aglsp;

#define YYPOPSTACK   (agvsp--, agssp--, aglsp--)
#else
#define YYPOPSTACK   (agvsp--, agssp--)
#endif

  int agstacksize = YYINITDEPTH;
  int agfree_stacks = 0;

#ifdef YYPURE
  int agchar;
  YYSTYPE aglval;
  int agnerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE aglloc;
#endif
#endif

  YYSTYPE agval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int aglen;

#if YYDEBUG != 0
  if (agdebug)
    fprintf(stderr, "Starting parse\n");
#endif

  agstate = 0;
  agerrstatus = 0;
  agnerrs = 0;
  agchar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  agssp = agss - 1;
  agvsp = agvs;
#ifdef YYLSP_NEEDED
  aglsp = agls;
#endif

/* Push a new state, which is found in  agstate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
agnewstate:

  *++agssp = agstate;

  if (agssp >= agss + agstacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *agvs1 = agvs;
      short *agss1 = agss;
#ifdef YYLSP_NEEDED
      YYLTYPE *agls1 = agls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = agssp - agss + 1;

#ifdef agoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if agoverflow is a macro.  */
      agoverflow("parser stack overflow",
		 &agss1, size * sizeof (*agssp),
		 &agvs1, size * sizeof (*agvsp),
		 &agls1, size * sizeof (*aglsp),
		 &agstacksize);
#else
      agoverflow("parser stack overflow",
		 &agss1, size * sizeof (*agssp),
		 &agvs1, size * sizeof (*agvsp),
		 &agstacksize);
#endif

      agss = agss1; agvs = agvs1;
#ifdef YYLSP_NEEDED
      agls = agls1;
#endif
#else /* no agoverflow */
      /* Extend the stack our own way.  */
      if (agstacksize >= YYMAXDEPTH)
	{
	  agerror("parser stack overflow");
	  if (agfree_stacks)
	    {
	      free (agss);
	      free (agvs);
#ifdef YYLSP_NEEDED
	      free (agls);
#endif
	    }
	  return 2;
	}
      agstacksize *= 2;
      if (agstacksize > YYMAXDEPTH)
	agstacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      agfree_stacks = 1;
#endif
      agss = (short *) YYSTACK_ALLOC (agstacksize * sizeof (*agssp));
      __ag_memcpy ((char *)agss, (char *)agss1,
		   size * (unsigned int) sizeof (*agssp));
      agvs = (YYSTYPE *) YYSTACK_ALLOC (agstacksize * sizeof (*agvsp));
      __ag_memcpy ((char *)agvs, (char *)agvs1,
		   size * (unsigned int) sizeof (*agvsp));
#ifdef YYLSP_NEEDED
      agls = (YYLTYPE *) YYSTACK_ALLOC (agstacksize * sizeof (*aglsp));
      __ag_memcpy ((char *)agls, (char *)agls1,
		   size * (unsigned int) sizeof (*aglsp));
#endif
#endif /* no agoverflow */

      agssp = agss + size - 1;
      agvsp = agvs + size - 1;
#ifdef YYLSP_NEEDED
      aglsp = agls + size - 1;
#endif

#if YYDEBUG != 0
      if (agdebug)
	fprintf(stderr, "Stack size increased to %d\n", agstacksize);
#endif

      if (agssp >= agss + agstacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (agdebug)
    fprintf(stderr, "Entering state %d\n", agstate);
#endif

  goto agbackup;
 agbackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* agresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  agn = agpact[agstate];
  if (agn == YYFLAG)
    goto agdefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* agchar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (agchar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (agdebug)
	fprintf(stderr, "Reading a token: ");
#endif
      agchar = YYLEX;
    }

  /* Convert token to internal form (in agchar1) for indexing tables with */

  if (agchar <= 0)		/* This means end of input. */
    {
      agchar1 = 0;
      agchar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (agdebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      agchar1 = YYTRANSLATE(agchar);

#if YYDEBUG != 0
      if (agdebug)
	{
	  fprintf (stderr, "Next token is %d (%s", agchar, agtname[agchar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, agchar, aglval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  agn += agchar1;
  if (agn < 0 || agn > YYLAST || agcheck[agn] != agchar1)
    goto agdefault;

  agn = agtable[agn];

  /* agn is what to do for this token type in this state.
     Negative => reduce, -agn is rule number.
     Positive => shift, agn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (agn < 0)
    {
      if (agn == YYFLAG)
	goto agerrlab;
      agn = -agn;
      goto agreduce;
    }
  else if (agn == 0)
    goto agerrlab;

  if (agn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (agdebug)
    fprintf(stderr, "Shifting token %d (%s), ", agchar, agtname[agchar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (agchar != YYEOF)
    agchar = YYEMPTY;

  *++agvsp = aglval;
#ifdef YYLSP_NEEDED
  *++aglsp = aglloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (agerrstatus) agerrstatus--;

  agstate = agn;
  goto agnewstate;

/* Do the default action for the current state.  */
agdefault:

  agn = agdefact[agstate];
  if (agn == 0)
    goto agerrlab;

/* Do a reduction.  agn is the number of a rule to reduce with.  */
agreduce:
  aglen = agr2[agn];
  if (aglen > 0)
    agval = agvsp[1-aglen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (agdebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       agn, agrline[agn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = agprhs[agn]; agrhs[i] > 0; i++)
	fprintf (stderr, "%s ", agtname[agrhs[i]]);
      fprintf (stderr, " -> %s\n", agtname[agr1[agn]]);
    }
#endif


  switch (agn) {

case 1:
#line 234 "parser.y"
{begin_graph(agvsp[0].str); agstrfree(agvsp[0].str);;
    break;}
case 2:
#line 236 "parser.y"
{AG.accepting_state = TRUE; end_graph();;
    break;}
case 3:
#line 238 "parser.y"
{
					if (AG.parsed_g)
						agclose(AG.parsed_g);
					AG.parsed_g = NULL;
					/*exit(1);*/
				;
    break;}
case 4:
#line 244 "parser.y"
{AG.parsed_g = NULL;;
    break;}
case 5:
#line 249 "parser.y"
{Agraph_type = AGRAPH; AG.edge_op = "--";;
    break;}
case 6:
#line 251 "parser.y"
{Agraph_type = AGRAPHSTRICT; AG.edge_op = "--";;
    break;}
case 7:
#line 253 "parser.y"
{Agraph_type = AGDIGRAPH; AG.edge_op = "->";;
    break;}
case 8:
#line 255 "parser.y"
{Agraph_type = AGDIGRAPHSTRICT; AG.edge_op = "->";;
    break;}
case 9:
#line 259 "parser.y"
{Current_class = TAG_GRAPH;;
    break;}
case 10:
#line 261 "parser.y"
{Current_class = TAG_NODE; N = G->proto->n;;
    break;}
case 11:
#line 263 "parser.y"
{Current_class = TAG_EDGE; E = G->proto->e;;
    break;}
case 20:
#line 283 "parser.y"
{attr_set(agvsp[-2].str,agvsp[0].str); agstrfree(agvsp[-2].str); agstrfree(agvsp[0].str);;
    break;}
case 27:
#line 296 "parser.y"
{agerror("syntax error, statement skipped");;
    break;}
case 31:
#line 302 "parser.y"
{;
    break;}
case 32:
#line 306 "parser.y"
{Current_class = TAG_GRAPH; /* reset */;
    break;}
case 33:
#line 308 "parser.y"
{Current_class = TAG_GRAPH;;
    break;}
case 34:
#line 312 "parser.y"
{
					objport_t		rv;
					rv.obj = agvsp[-1].n;
					rv.port = strdup(Port);
					Port[0] = '\0';
					agval.obj = rv;
				;
    break;}
case 35:
#line 321 "parser.y"
{agval.n = bind_node(agvsp[0].str); agstrfree(agvsp[0].str);;
    break;}
case 41:
#line 331 "parser.y"
{strcat(Port,":"); strcat(Port,agvsp[0].str);;
    break;}
case 42:
#line 332 "parser.y"
{Symbol = strdup(agvsp[0].str);;
    break;}
case 43:
#line 333 "parser.y"
{	char buf[SMALLBUF];
					sprintf(buf,":(%s,%s)",Symbol,agvsp[-1].str);
					strcat(Port,buf); free(Symbol);
				;
    break;}
case 44:
#line 340 "parser.y"
{	char buf[SMALLBUF];
					sprintf(buf,"@%s",agvsp[0].str);
					strcat(Port,buf);
				;
    break;}
case 45:
#line 347 "parser.y"
{Current_class = TAG_NODE; N = (Agnode_t*)(agvsp[0].obj.obj);;
    break;}
case 46:
#line 349 "parser.y"
{Current_class = TAG_GRAPH; /* reset */;
    break;}
case 47:
#line 353 "parser.y"
{begin_edgestmt(agvsp[0].obj);;
    break;}
case 48:
#line 355 "parser.y"
{ E = SP->subg->proto->e;
				  Current_class = TAG_EDGE; ;
    break;}
case 49:
#line 358 "parser.y"
{end_edgestmt();;
    break;}
case 50:
#line 360 "parser.y"
{begin_edgestmt(agvsp[0].obj);;
    break;}
case 51:
#line 362 "parser.y"
{ E = SP->subg->proto->e;
				  Current_class = TAG_EDGE; ;
    break;}
case 52:
#line 365 "parser.y"
{end_edgestmt();;
    break;}
case 53:
#line 368 "parser.y"
{mid_edgestmt(agvsp[0].obj);;
    break;}
case 54:
#line 370 "parser.y"
{mid_edgestmt(agvsp[0].obj);;
    break;}
case 56:
#line 373 "parser.y"
{mid_edgestmt(agvsp[0].obj);;
    break;}
case 57:
#line 375 "parser.y"
{mid_edgestmt(agvsp[0].obj);;
    break;}
case 59:
#line 380 "parser.y"
{agval.obj = pop_gobj();;
    break;}
case 60:
#line 381 "parser.y"
{ anonsubg(); ;
    break;}
case 61:
#line 381 "parser.y"
{agval.obj = pop_gobj();;
    break;}
case 62:
#line 382 "parser.y"
{agval.obj = pop_gobj();;
    break;}
case 63:
#line 386 "parser.y"
{ Agraph_t	 *subg;
				if ((subg = agfindsubg(AG.parsed_g,agvsp[0].str))) aginsert(G,subg);
				else subg = agsubg(G,agvsp[0].str); 
				push_subg(subg);
				In_decl = FALSE;
				agstrfree(agvsp[0].str);
				;
    break;}
case 64:
#line 395 "parser.y"
{agval.str = agstrdup(agvsp[0].str); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  agvsp -= aglen;
  agssp -= aglen;
#ifdef YYLSP_NEEDED
  aglsp -= aglen;
#endif

#if YYDEBUG != 0
  if (agdebug)
    {
      short *ssp1 = agss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != agssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++agvsp = agval;

#ifdef YYLSP_NEEDED
  aglsp++;
  if (aglen == 0)
    {
      aglsp->first_line = aglloc.first_line;
      aglsp->first_column = aglloc.first_column;
      aglsp->last_line = (aglsp-1)->last_line;
      aglsp->last_column = (aglsp-1)->last_column;
      aglsp->text = 0;
    }
  else
    {
      aglsp->last_line = (aglsp+aglen-1)->last_line;
      aglsp->last_column = (aglsp+aglen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  agn = agr1[agn];

  agstate = agpgoto[agn - YYNTBASE] + *agssp;
  if (agstate >= 0 && agstate <= YYLAST && agcheck[agstate] == *agssp)
    agstate = agtable[agstate];
  else
    agstate = agdefgoto[agn - YYNTBASE];

  goto agnewstate;

agerrlab:   /* here on detecting error */

  if (! agerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++agnerrs;

#ifdef YYERROR_VERBOSE
      agn = agpact[agstate];

      if (agn > YYFLAG && agn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -agn if nec to avoid negative indexes in agcheck.  */
	  for (x = (agn < 0 ? -agn : 0);
	       x < (sizeof(agtname) / sizeof(char *)); x++)
	    if (agcheck[x + agn] == x)
	      size += strlen(agtname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (agn < 0 ? -agn : 0);
		       x < (sizeof(agtname) / sizeof(char *)); x++)
		    if (agcheck[x + agn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, agtname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      agerror(msg);
	      free(msg);
	    }
	  else
	    agerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	agerror("parse error");
    }

  goto agerrlab1;
agerrlab1:   /* here on error raised explicitly by an action */

  if (agerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (agchar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (agdebug)
	fprintf(stderr, "Discarding token %d (%s).\n", agchar, agtname[agchar1]);
#endif

      agchar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  agerrstatus = 3;		/* Each real token shifted decrements this */

  goto agerrhandle;

agerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  agn = agdefact[agstate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (agn) goto agdefault;
#endif

agerrpop:   /* pop the current state because it cannot handle the error token */

  if (agssp == agss) YYABORT;
  agvsp--;
  agstate = *--agssp;
#ifdef YYLSP_NEEDED
  aglsp--;
#endif

#if YYDEBUG != 0
  if (agdebug)
    {
      short *ssp1 = agss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != agssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

agerrhandle:

  agn = agpact[agstate];
  if (agn == YYFLAG)
    goto agerrdefault;

  agn += YYTERROR;
  if (agn < 0 || agn > YYLAST || agcheck[agn] != YYTERROR)
    goto agerrdefault;

  agn = agtable[agn];
  if (agn < 0)
    {
      if (agn == YYFLAG)
	goto agerrpop;
      agn = -agn;
      goto agreduce;
    }
  else if (agn == 0)
    goto agerrpop;

  if (agn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (agdebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++agvsp = aglval;
#ifdef YYLSP_NEEDED
  *++aglsp = aglloc;
#endif

  agstate = agn;
  goto agnewstate;

 agacceptlab:
  /* YYACCEPT comes here.  */
  if (agfree_stacks)
    {
      free (agss);
      free (agvs);
#ifdef YYLSP_NEEDED
      free (agls);
#endif
    }
  return 0;

 agabortlab:
  /* YYABORT comes here.  */
  if (agfree_stacks)
    {
      free (agss);
      free (agvs);
#ifdef YYLSP_NEEDED
      free (agls);
#endif
    }
  return 1;
}
#line 396 "parser.y"
