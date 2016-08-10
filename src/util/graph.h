#ifndef UTIL_GRAPH_H_
#define UTIL_GRAPH_H_

struct arc;
struct graph;
struct iterator;
struct vertex;

struct graph *graph_new(void);
void graph_delete(struct graph *, void (*)(void *), void (*)(void *));
void graph_print_dot(struct graph *, const char *(*)(void *),
						const char *(*)(void *));

int graph_get_vertices_count(struct graph *);
struct vertex *graph_add_vertex(struct graph *, void *, void *);
struct vertex *graph_get_vertex(struct graph *, void *);
struct vertex *graph_get_root(struct graph *);
void vertex_delete(struct graph *, void *, void (*)(void *), void (*)(void *));

void vertex_add_rtgt(struct vertex *, struct arc *);
void vertex_add_rsrc(struct vertex *, struct arc *);
void *vertex_get_info(const struct vertex *);
void vertex_set_info(struct vertex *, void *);
int vertex_has_rsrc(const struct vertex *);
int vertex_has_rtgt(const struct vertex *);
struct iterator *vertex_get_rtgt(const struct vertex *);
struct iterator *vertex_get_rsrc(const struct vertex *);

void vertex_set_mark(struct vertex *, int, int);
int vertex_get_mark(struct vertex *, int);

int graph_has_arc(struct graph *, struct vertex *, struct vertex *);

struct arc *arc_new(struct graph *, struct vertex *, void *, struct vertex *);
void arc_delete(struct graph *, struct arc *, void (*)(void *));
struct vertex *arc_get_src(struct arc *);
void *arc_get_info(const struct arc *);
void arc_set_info(struct arc *, void *);
struct vertex *arc_get_tgt(struct arc *);

void arc_set_mark(struct arc *, int, int);
int arc_get_mark(struct arc *, int);

struct iterator *graph_get_vertex_iterator(struct graph *);

#endif /* UTIL_GRAPH_H_ */
