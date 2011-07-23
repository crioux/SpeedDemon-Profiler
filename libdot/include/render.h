/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#ifndef               RENDER_H
#define               RENDER_H

#ifdef _PACKAGE_AST
#include <ast.h>
#endif

#ifdef HAVE_CONFIG_H
#include "gvconfig.h"
#endif

#include <signal.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "const.h"   /* must follow gvconfig.h */
#include "types.h"
#include "graph.h"   /* must follow types.h */
#include "globals.h"
#include "renderprocs.h"

#ifndef NIL
#define NIL(type)       ((type)0)
#endif /*NIL*/

#endif /* RENDER_H */
