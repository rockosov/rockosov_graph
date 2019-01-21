#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "list.h"

/* TODO: scale bitmap to support configurable bitness */
#define MARKER_COUNT 64
#define INVALID_MARKER 0xFFFFFFFF

typedef unsigned long long uint64;
typedef unsigned int uint32;

struct marker_desc {
    struct list_head marked; /* List with marked elements (vertexes or edges) */
    bool allocated; /* If marker already allocated for someone */
};

struct marked_elem {
    struct marked_elem** markers; /* Pointer to vertex or edge markers */
    struct list_head entry; /* Entry in list of marked elements in marker descriptor */
    bool vertex_or_edge; /* TRUE - vertex, FALSE - edge */
};

struct graph {
    struct list_head vertexes; /* All vertexes */
    struct list_head edges; /* All edges */
    unsigned int vertexes_num; /* Number of vertexes */
    unsigned int edges_num; /* Number of edges */
    struct marker_desc markers[MARKER_COUNT]; /* All available markers */
};

struct vertex {
    struct list_head input; /* Input edges */
    unsigned int data; /* Data accosiated with the vertex */
    struct list_head output; /* Output edges */
    struct list_head graph_entry; /* Entry in graph vertexes list */
    struct marked_elem* markers[MARKER_COUNT]; /* All markers */
};

struct edge {
    struct list_head input_entry; /* Entry in vertex's input list */
    struct list_head output_entry; /* Entry in vertex's output list */
    struct list_head graph_entry; /* Entry in graph edges list */
    struct vertex* src; /* Pointer to source vertex */
    struct vertex* dst; /* Pointer to destination vertex */
    struct marked_elem* markers[MARKER_COUNT]; /* All markers */
};

/* Get Vertex or Edge which is owner for specific markers map */
#define markers_owner(ptr, type) \
    container_of((struct marked_elem* const(*)[MARKER_COUNT])ptr, type, markers)

#define set_marker_vertex(graph, vertex, id) set_marker(graph, id, (vertex)->markers, true)
#define unset_marker_vertex(graph, vertex, id) unset_marker(graph, id, (vertex)->markers)
#define check_marker_vertex(vertex, id) check_marker((vertex)->markers, id)

#define set_marker_edge(graph, edge, id) set_marker(graph, id, (edge)->markers, false)
#define unset_marker_edge(graph, edge, id) unset_marker(graph, id, (edge)->markers)
#define check_marker_edge(edge, id) check_marker((edge)->markers, id)

#endif /* !__GRAPH_H__ */

