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
 * Written by Stephen North and Eleftherios Koutsofios.
 */

#include	"neato.h"
#ifdef HAVE_CONFIG_H
#include "gvconfig.h"
#endif
#include	<time.h>
#ifndef MSWIN32
#include	<unistd.h>
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef STANDALONE
int main(int argc, char** argv)
{
	graph_t	*g;
	static graph_t *prev;

	dotneato_initialize(argc,argv);
#ifndef MSWIN32
	signal (SIGUSR1, toggle);
	signal (SIGINT, intr);
#endif

	while ((g = next_input_graph())) {
		if (prev) neato_cleanup(prev);
		prev = g;
		neato_layout(g);
		dotneato_write(g);
	}
	dotneato_terminate();
	return 1;
}	

#endif