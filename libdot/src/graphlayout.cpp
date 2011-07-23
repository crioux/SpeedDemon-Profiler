#include "dot.h"
#include <time.h>
#include "libdotgraphlayout.h"

char *Info[] = {
    "dot",              /* Program */
    "1.7",            /* Version */
    "today"                /* Build Date */
};

#include <string>

const char *
GraphLayout::getgraph(const char *str)
{
    static std::string out;

    // clear every time we start
    out = "";

    // init dot
    char *argv[] = { "dot", "-Tplain", NULL };
    dotneato_initialize(2, argv, (char*)str);

    // do graph layout and save
    graph_t *g = next_input_graph();
    dot_layout(g);
    dotneato_write(g, &out);
    emit_eof();

    // cleanup and return static string
    dot_cleanup(g);
	dotneato_terminate();
    return out.c_str();
}

const char *
GraphLayout::getgraphfile(const char *file)
{
    // read file in and pass to getgraph
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
	printf("couldn't open file %s\n", file);
	return NULL;
    }

    char buf[8192];
    std::string s;

    while (fgets(buf, sizeof(buf)-1, fp)) {
	s += buf;
    }
    fclose(fp);

    return getgraph(s.c_str());
}

/*
int
main(int argc, char** argv)
{
    static graph_t *prev;

    char *GR = "digraph states {   size=\"3,2\";   rankdir=LR;   node [shape=ellipse];   empty [label = \"Empty\"];   stolen [label = \"Stolen\"];   waiting [label = \"Waiting\"];   full [label = \"Full\"];   empty -> full [label = \"return\"]   empty -> stolen [label = \"dispatch\", wt=28]   stolen -> full [label = \"return\"];   stolen -> waiting [label = \"touch\"];   waiting -> full [label = \"return\"];   } ";
    char *GR2 = "digraph mike{ size = \"8,8\"; a -> A; a -> m; a -> E; t -> O; r -> V; r -> Q; p -> B; m -> R; l -> C; c -> C; W -> X; W -> D; V -> W; T -> U; Q -> T; Q -> H; Q -> A; O -> K; L -> U; K -> L; K -> J; K -> E; J -> I; R -> B; P -> F; H -> R; H -> P; U -> H; G -> U; E -> G; C -> Z; C -> D; S -> D; B -> N; B -> D; B -> S; M -> B; A -> M; N -> Y; } ";

    printf("%s\n", GraphLayout::getgraph(GR));
    printf("------------------------------------------------\n");
    printf("%s\n", GraphLayout::getgraph(GR2));

    return 0;
}
*/
