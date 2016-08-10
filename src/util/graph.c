
#include <util/graph.h>

#include <stdio.h>

#include <util/bitvector.h>
#include <util/bsd-queue.h>
#include <util/exception.h>
#include <util/hash.h>
#include <util/iterator.h>
#include <util/list.h>
#include <util/memory.h>
#include <util/message.h>


struct graph {
    t_hash vertices;		/* void * -> vertex */
};

struct vertex {
    void *info;
    t_bitvector marks;

    /* lists of arcs this vertex is a target or a source of */
    SLIST_HEAD(, arc) rtgt_head;
    SLIST_HEAD(, arc) rsrc_head;
};

struct arc {
    void *info;
    struct vertex *src;
    struct vertex *tgt;

    t_bitvector marks;

    /* chain pointers used for lists in struct vertex */
    SLIST_ENTRY(arc) rtgt_entry;
    SLIST_ENTRY(arc) rsrc_entry;
};


struct graph *
graph_new(void) {
    struct graph *g = memory_alloc(sizeof *g);

    g->vertices = hash_new(hash_hashfun_pointer, hash_eqfun_pointer);

    return g;
}

void
graph_delete(struct graph *graph,
	      void vertex_info_delete(void *), void arc_info_delete(void *)) {
    t_list vertices = NULL;
    struct iterator *it = hash_iterator(graph->vertices);
    while (!iterator_is_at_end(it))
	vertices = list_cons(iterator_next(it), vertices);
    iterator_delete(it);

    while (vertices != NULL) {
	t_list tmp = list_cdr(vertices);
	vertex_delete(graph, vertex_get_info(list_car(vertices)),
		vertex_info_delete, arc_info_delete);
	list_delete(vertices);
	vertices = tmp;
    }

    hash_delete(graph->vertices, memory_pair_delete_noop);
    memory_free(graph);
}

void
graph_print_dot(struct graph *g,
		const char *vertex_info_to_string(void *),
		const char *arc_info_to_string(void *)) {
    struct iterator *iter;

    message_printf("digraph untitled {\n");

    for (iter = graph_get_vertex_iterator(g); !iterator_is_at_end(iter); ) {
	struct vertex *v = iterator_next(iter);
	struct iterator *i = vertex_get_rsrc(v);

	message_printf("\tnode%p", v);
	if (vertex_info_to_string != NULL)
	    message_printf(" [ label=\"%s\" ]", vertex_info_to_string(v->info));
	message_printf(";\n");

	while (!iterator_is_at_end(i)) {
	    struct arc *arc = iterator_next(i);

	    message_printf("\tnode%p -> node%p", v, arc->tgt);
	    if (arc_info_to_string != NULL)
		message_printf(" [ label=\"%s\" ]",
			arc_info_to_string(arc->info));
	    message_printf(";\n");
	}
	iterator_delete(i);
    }
    iterator_delete(iter);

    message_printf("}\n");
}

int
graph_get_vertices_count(struct graph *graph){

    return hash_get_size(graph->vertices);
}

struct vertex *
graph_add_vertex(struct graph *graph, void *id, void *info) {
    volatile struct vertex *vertex = NULL;
    volatile t_bitvector vertex_marks = NULL;

    EXCEPTION_CATCH(runtime_error, {
	vertex = memory_alloc(sizeof *vertex);
	vertex_marks = bitvec_new();
    });

    if (EXCEPTION_IS_RAISED(runtime_error)) {
	if (vertex_marks != NULL)
	    bitvec_delete(vertex_marks);

	if (vertex != NULL)
	    memory_free((void *) vertex);

	EXCEPTION_RAISE_SAME();
    }

    vertex->info = info;
    vertex->marks = vertex_marks;
    SLIST_INIT(&vertex->rtgt_head);
    SLIST_INIT(&vertex->rsrc_head);

    hash_put(graph->vertices, id, (struct vertex *) vertex);
    return (struct vertex *) vertex;
}

struct vertex *
graph_get_vertex(struct graph *graph, void *id) {
    return hash_get(graph->vertices, id);
}

struct vertex *
graph_get_root(struct graph *graph) {
    struct iterator *iter;
    struct vertex *v = NULL;

    for (iter = graph_get_vertex_iterator(graph); !iterator_is_at_end(iter); )
	if (!vertex_has_rtgt(v = iterator_next(iter)))
	    break;

    if (v == NULL)
	EXCEPTION_RAISE(runtime_error, "graph has not root node");

    iterator_delete(iter);
    return v;
}

void
vertex_delete(struct graph *graph, void *id,
	      void vertex_info_delete(void *), void arc_info_delete(void *)) {
    struct vertex *vertex = hash_get(graph->vertices, id);
    struct arc *arc;

    for (arc = SLIST_FIRST(&vertex->rsrc_head); arc != NULL; ) {
	struct arc *next_arc = SLIST_NEXT(arc, rsrc_entry);

	arc_delete(graph, arc, arc_info_delete);
	arc = next_arc;
    }

    for (arc = SLIST_FIRST(&vertex->rtgt_head); arc != NULL; ) {
	struct arc *next_arc = SLIST_NEXT(arc, rtgt_entry);

	arc_delete(graph, arc, arc_info_delete);
	arc = next_arc;
    }

    hash_remove(graph->vertices, id);

    if (vertex_info_delete != NULL)
	vertex_info_delete(vertex->info);

    bitvec_delete(vertex->marks);
    memory_free(vertex);
}

void
vertex_add_rtgt(struct vertex *vertex, struct arc *arc) {
    SLIST_INSERT_HEAD(&vertex->rtgt_head, arc, rtgt_entry);
}

void
vertex_add_rsrc(struct vertex *vertex, struct arc *arc) {
    SLIST_INSERT_HEAD(&vertex->rsrc_head, arc, rsrc_entry);
}

void
vertex_set_mark(struct vertex *vertex, int index, int value) {
    bitvec_set(vertex->marks, index, value);
}

int
vertex_get_mark(struct vertex *vertex, int index) {
    return bitvec_get(vertex->marks, index);
}

void *
vertex_get_info(const struct vertex *vertex) {
    return vertex->info;
}

void
vertex_set_info(struct vertex *vertex, void *info) {
    vertex->info = info;
}

int
vertex_has_rsrc(const struct vertex *vertex) {
    return SLIST_FIRST(&vertex->rsrc_head) != NULL;
}

int
vertex_has_rtgt(const struct vertex *vertex) {
    return SLIST_FIRST(&vertex->rtgt_head) != NULL;
}

struct arc_iterator {
    struct iterator iter;
    struct arc *next_arc;
};

static int
arc_iterator_is_at_end(struct iterator *i) {
    struct arc_iterator *iter = (struct arc_iterator *) i;

    return iter->next_arc == NULL;
}

static void
arc_iterator_delete(struct iterator *i) {
    memory_free(i);
}

static struct iterator *
vertex_new_arc_iterator(struct arc *first_arc, void *next(struct iterator *)) {
    struct arc_iterator *i = memory_alloc(sizeof *i);

    i->iter.is_at_end = arc_iterator_is_at_end;
    i->iter.next_pair = NULL;
    i->iter.next = next;
    i->iter.free = arc_iterator_delete;

    i->next_arc = first_arc;

    return (struct iterator *) i;
}

static void *
arc_iterator_next_in_rtgt(struct iterator *i) {
    struct arc_iterator *iter = (struct arc_iterator *) i;
    struct arc *arc = iter->next_arc;

    iter->next_arc = SLIST_NEXT(arc, rtgt_entry);

    return arc;
}

static void *
arc_iterator_next_in_rsrc(struct iterator *i) {
    struct arc_iterator *iter = (struct arc_iterator *) i;
    struct arc *arc = iter->next_arc;

    iter->next_arc = SLIST_NEXT(arc, rsrc_entry);

    return arc;
}

struct iterator *
vertex_get_rtgt(const struct vertex *vertex) {
    return vertex_new_arc_iterator(SLIST_FIRST(&vertex->rtgt_head),
	arc_iterator_next_in_rtgt);
}

struct iterator *
vertex_get_rsrc(const struct vertex *vertex) {
    return vertex_new_arc_iterator(SLIST_FIRST(&vertex->rsrc_head),
	arc_iterator_next_in_rsrc);
}



struct arc *
arc_new(struct graph *graph,
	struct vertex *src, void *info, struct vertex *tgt) {
    struct arc *arc = memory_alloc(sizeof *arc);
    (void) graph;

    arc->src = src;
    arc->info = info;
    arc->tgt = tgt;

    arc->marks = NULL;

    return arc;
}

void
arc_delete(struct graph *graph, struct arc *arc, void arc_delete_info(void *)) {
    struct arc **ap;
    (void) graph;

    /* reaching NULL means the data structure is broken. dump core. */

    for (ap = &SLIST_FIRST(&arc->src->rsrc_head); *ap != arc && *ap != NULL; )
	ap = &SLIST_NEXT(*ap, rsrc_entry);
    *ap = SLIST_NEXT(*ap, rsrc_entry);

    for (ap = &SLIST_FIRST(&arc->tgt->rtgt_head); *ap != arc && *ap != NULL; )
	ap = &SLIST_NEXT(*ap, rtgt_entry);
    *ap = SLIST_NEXT(*ap, rtgt_entry);

    if (arc_delete_info != NULL)
	arc_delete_info(arc->info);

    if (arc->marks != NULL)
	bitvec_delete(arc->marks);

    memory_free(arc);
}

int
graph_has_arc(struct graph *graph, struct vertex *src, struct vertex *tgt) {
    struct arc *arc = SLIST_FIRST(&src->rsrc_head);
    (void) graph;

    while (arc != NULL) {
	if (arc->tgt == tgt)
	    return 1;
	arc = SLIST_NEXT(arc, rsrc_entry);
    }

    return 0;
}

void
arc_set_mark(struct arc *arc, int index, int value) {
    bitvec_set(arc->marks, index, value);
}

int
arc_get_mark(struct arc *arc, int index) {
    return bitvec_get(arc->marks, index);
}

struct vertex *
arc_get_src(struct arc *arc) {
    return arc->src;
}

void *
arc_get_info(const struct arc *arc) {
    return arc->info;
}

void
arc_set_info(struct arc *arc, void *info) {
    arc->info = info;
}

struct vertex *
arc_get_tgt(struct arc *arc) {
    return arc->tgt;
}


struct iterator *
graph_get_vertex_iterator(struct graph *g) {
    return hash_iterator(g->vertices);
}
