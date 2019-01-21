#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "graph.h"

/* Markers operations */
void set_marker(struct graph* graph,
                uint32 id,
                struct marked_elem** markers,
                bool vertex_or_edge) {
    struct marked_elem* new_elem = NULL;

    if (markers[id] != NULL) {
        /* Already marked */
        return;
    }

    new_elem = malloc(sizeof(struct marked_elem));
    INIT_LIST_ENTRY(&new_elem->entry);

    list_add_tail(&new_elem->entry, &graph->markers[id].marked);
    new_elem->markers = markers;
    new_elem->vertex_or_edge = vertex_or_edge;
    markers[id] = new_elem;
}

void unset_marker(struct graph* graph,
                  uint32 id,
                  struct marked_elem** markers) {
    struct marked_elem* elem = markers[id];

    if (elem == NULL) {
        /* Already unmarked */
        return;
    }

    list_del(&elem->entry);
    elem->markers = NULL;
    markers[id] = NULL;

    free(elem);
}

uint32 alloc_marker(struct graph* graph) {
    uint32 id = INVALID_MARKER;

    for (id = 0; id < MARKER_COUNT; ++id) {
        if (!graph->markers[id].allocated) {
            graph->markers[id].allocated = true;
            break;
        }
    }

    return id;
}

void free_marker(struct graph* graph, uint32 id) {
    struct marked_elem* elem = NULL;
    struct marked_elem* _elem = NULL;

    if (!graph->markers[id].allocated) {
        return;
    }

    list_for_each_entry_safe(elem, _elem, &graph->markers[id].marked, entry) {
        unset_marker(graph, id, elem->markers);
    }

    graph->markers[id].allocated = false;
}

bool check_marker(struct marked_elem** markers, uint32 id) {
    return (markers[id] != NULL);
}

void print_local_markers(struct marked_elem** markers, unsigned char indent) {
    bool no_markers = true;
    uint32 i = 0;

    printf("%*smarkers: ", indent, "");
    for (i = 0; i < MARKER_COUNT; ++i) {
        if (markers[i] != NULL) {
            no_markers = false;
            printf("%u ", i);
        }
    }
    printf("%s\n", no_markers ? "None" : "");

    return;
}

void print_edge(struct edge* edge, unsigned char indent) {
    printf("%*sEdge:\n", indent, "");
    printf("%*ssrc = vertex(%d)\n", indent + 4, "", edge->src->data);
    printf("%*sdst = vertex(%d)\n", indent + 4, "", edge->dst->data);

    print_local_markers(edge->markers, indent + 4);

    return;
}

void print_vertex(struct vertex* vertex, unsigned char indent) {
    struct edge* edge = NULL;

    printf("%*sVertex:\n", indent, "");
    printf("%*sdata = %d\n", indent + 4, "", vertex->data);

    printf("%*sinput:%s\n", indent + 4, "",
           list_is_empty(&vertex->input) ? "EMPTY" : "");
    list_for_each_entry(edge, &vertex->input, input_entry) {
        print_edge(edge, indent + 8);
    }

    printf("%*soutput:%s\n", indent + 4, "",
           list_is_empty(&vertex->output) ? "EMPTY" : "");
    list_for_each_entry(edge, &vertex->output, output_entry) {
        print_edge(edge, indent + 8);
    }

    print_local_markers(vertex->markers, indent + 4);

    return;
}

void print_marker(struct graph* graph, uint32 id, unsigned char indent) {
    struct marker_desc* marker = &graph->markers[id];
    struct vertex* vertex = NULL;
    struct edge* edge = NULL;
    struct marked_elem* elem = NULL;

    printf("%*sMarker:\n", indent, "");
    printf("%*sid: %u\n", indent + 4, "", id);
    printf("%*sallocated: %s\n", indent + 4, "",
           marker->allocated ? "TRUE" : "FALSE");
    printf("%*smarked: %s", indent + 4, "",
           list_is_empty(&marker->marked) ? "EMPTY" : "");
    list_for_each_entry(elem, &marker->marked, entry) {
        printf("\n%*s", indent + 8, "");
        if (elem->vertex_or_edge) {
            vertex = markers_owner(elem->markers, struct vertex);
            printf("vertex(%u)", vertex->data);
        } else {
            edge = markers_owner(elem->markers, struct edge);
            printf("edge(%u, %u)", edge->src->data, edge->dst->data);
        }
        printf(" ");
    }
    printf("\n");

    return;
}

void print_graph(struct graph* graph, unsigned char indent) {
    struct vertex* vertex = NULL;
    bool no_allocated_markers = true;
    uint32 i = 0;

    if (graph == NULL) {
        return;
    }

    printf("%*sGraph:\n", indent, "");
    printf("%*svertexes_num = %d\n", indent + 4, "", graph->vertexes_num);
    printf("%*sedges_num = %d\n", indent + 4, "", graph->edges_num);

    printf("%*svertexes:%s\n", indent + 4, "",
           list_is_empty(&graph->vertexes) ? "EMPTY" : "");
    list_for_each_entry(vertex, &graph->vertexes, graph_entry) {
        print_vertex(vertex, indent + 8);
    }

    printf("%*smarkers: ", indent + 4, "");
    for (i = 0; i < MARKER_COUNT; ++i) {
        if (graph->markers[i].allocated) {
            if (no_allocated_markers) {
                printf("\n");
            }
            no_allocated_markers = false;
            print_marker(graph, i, indent + 8);
        }
    }

    if (no_allocated_markers) {
        printf("None\n");
    }
}

struct edge* create_edge(struct graph* graph,
                         struct vertex* src,
                         struct vertex* dst) {
    struct edge* edge = NULL;

    if ((graph == NULL) || (src == NULL) || (dst == NULL)) {
        goto exit;
    }

    /* Allocate memory for the edge */
    edge = malloc(sizeof(struct edge));
    if (edge == NULL) {
        goto exit;
    }

    /* Initialize all edge's data */
    INIT_LIST_ENTRY(&edge->input_entry);
    INIT_LIST_ENTRY(&edge->output_entry);
    INIT_LIST_ENTRY(&edge->graph_entry);
    edge->src = src;
    edge->dst = dst;

    /* Add edge to the graph */
    list_add_tail(&edge->graph_entry, &graph->edges);
    graph->edges_num += 1;

    /* Add edge to the source vertex */
    list_add_tail(&edge->output_entry, &src->output);

    /* Add edge to the destination vertex */
    list_add_tail(&edge->input_entry, &dst->input);

exit:
    return edge;
}

struct vertex* create_vertex(struct graph* graph, unsigned int data) {
    struct vertex* vertex = NULL;

    /* Allocate memory for the vertex */
    vertex = malloc(sizeof(struct vertex));
    if (vertex == NULL) {
        goto exit;
    }

    /* Initialize all vertex's data */
    INIT_LIST_HEAD(&vertex->input);
    vertex->data = data;
    INIT_LIST_HEAD(&vertex->output);
    INIT_LIST_ENTRY(&vertex->graph_entry);

    /* Add vertex to the graph */
    list_add_tail(&vertex->graph_entry, &graph->vertexes);
    graph->vertexes_num += 1;

exit:
    return vertex;
}

struct graph* create_graph(void) {
    struct graph* graph = NULL;
    uint32 i = 0;

    /* Allocate memory for the graph */
    graph = malloc(sizeof(struct graph));
    if (graph == NULL) {
        goto exit;
    }

    /* Initialize all graph's data */
    INIT_LIST_HEAD(&graph->vertexes);
    INIT_LIST_HEAD(&graph->edges);
    graph->vertexes_num = 0;
    graph->edges_num = 0;

    /* Initialize all markers descriptors */
    for (i = 0; i < MARKER_COUNT; ++i) {
        INIT_LIST_HEAD(&graph->markers[i].marked);
        graph->markers[i].allocated = false;
    }

exit:
    return graph;
}

void destroy_edge(struct graph* graph, struct edge* edge) {
    uint32 i = 0;

    if (edge == NULL) {
        return;
    }

    /* Unset all markers */
    for (i = 0; i < MARKER_COUNT; ++i) {
        unset_marker(graph, i, edge->markers);
    }

    /* NULL source vertex */
    edge->src = NULL;

    /* NULL destination vertex */
    edge->dst = NULL;

    /* Delete edge from the vertex input list */
    list_del(&edge->input_entry);

    /* Delete edge from the vertex output list */
    list_del(&edge->output_entry);

    /* Delete edge from the graph edges list */
    list_del(&edge->graph_entry);
    graph->edges_num -= 1;

    /* Free memory */
    free(edge);

    return;
}

void destroy_vertex(struct graph* graph, struct vertex* vertex) {
    struct edge* edge = NULL;
    struct edge* _edge = NULL;
    uint32 i = 0;

    if (vertex == NULL) {
        return;
    }

    /* Unset all markers */
    for (i = 0; i < MARKER_COUNT; ++i) {
        unset_marker(graph, i, vertex->markers);
    }

    /* Destroy all input edges */
    list_for_each_entry_safe(edge, _edge, &vertex->input, input_entry) {
        destroy_edge(graph, edge);
    }

    /* Destroy all output edges */
    list_for_each_entry_safe(edge, _edge, &vertex->output, output_entry) {
        destroy_edge(graph, edge);
    }

    /* NULL associated data */
    vertex->data = 0;

    /* Delete vertex from the graph vertexes list */
    list_del(&vertex->graph_entry);
    graph->vertexes_num -= 1;

    /* Free memory */
    free(vertex);

    return;
}

void destroy_graph(struct graph* graph) {
    struct edge* edge = NULL;
    struct edge* _edge = NULL;
    struct vertex* vertex = NULL;
    struct vertex* _vertex = NULL;
    uint32 i = 0;

    if (graph == NULL) {
        return;
    }

    /* Destroy all markers */
    for (i = 0; i < MARKER_COUNT; ++i) {
        free_marker(graph, i);
    }

    /* Destroy all edges */
    list_for_each_entry_safe(edge, _edge, &graph->edges, graph_entry) {
        destroy_edge(graph, edge);
    }

    /* Destroy all vertexes */
    list_for_each_entry_safe(vertex, _vertex, &graph->vertexes, graph_entry) {
        destroy_vertex(graph, vertex);
    }

    /* Free graph memory */
    free(graph);
}

void redirect_edge(struct edge* edge,
                   struct vertex* new_src,
                   struct vertex* new_dst) {
    if (edge == NULL) {
        return;
    }

    if (new_src != NULL) {
        /* Delete edge from the old source vertex output list */
        list_del(&edge->output_entry);

        /* Add edge to the new source vertex output list */
        list_add_tail(&edge->output_entry, &new_src->output);
        edge->src = new_src;
    }

    if (new_dst != NULL) {
        /* Delete edge from the old destination vertex input list */
        list_del(&edge->input_entry);

        /* Add edge to the new destination vertex input list */
        list_add_tail(&edge->input_entry, &new_dst->input);
        edge->dst = new_dst;
    }
}

int main(int argc, char** argv) {
    struct graph* graph = NULL;
    struct vertex* vertex_1 = NULL;
    struct vertex* vertex_3 = NULL;
    struct vertex* vertex_5 = NULL;
    struct vertex* vertex_6 = NULL;
    struct vertex* vertex_8 = NULL;
    struct edge* edge_3_1 = NULL;
    struct edge* edge_1_6 = NULL;
    struct edge* edge_1_8 = NULL;
    struct edge* edge_8_5 = NULL;
    struct edge* edge_5_6 = NULL;
    uint32 marker_0 = INVALID_MARKER;
    uint32 marker_1 = INVALID_MARKER;
    int err = -1;

    graph = create_graph();
    if (graph == NULL) {
        goto fail_create_graph;
    }

    vertex_1 = create_vertex(graph, 1);
    if (vertex_1 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    vertex_3 = create_vertex(graph, 3);
    if (vertex_3 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    vertex_5 = create_vertex(graph, 5);
    if (vertex_5 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    vertex_6 = create_vertex(graph, 6);
    if (vertex_6 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    vertex_8 = create_vertex(graph, 8);
    if (vertex_8 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    edge_3_1 = create_edge(graph, vertex_3, vertex_1);
    if (edge_3_1 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    edge_1_6 = create_edge(graph, vertex_1, vertex_6);
    if (edge_1_6 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    edge_1_8 = create_edge(graph, vertex_1, vertex_8);
    if (edge_1_8 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    edge_8_5 = create_edge(graph, vertex_8, vertex_5);
    if (edge_8_5 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    edge_5_6 = create_edge(graph, vertex_5, vertex_6);
    if (edge_5_6 == NULL) {
        goto destroy_graph_due_to_fail;
    }

    printf("Graph after creation:\n");
    print_graph(graph, 0);

    printf("Redirect edge(1, 6) to edge(1, 1):\n");
    print_edge(edge_1_6, 0);
    redirect_edge(edge_1_6, NULL, vertex_1);
    printf("Graph after redirection:\n");
    print_graph(graph, 0);

    printf("Destroy edge(1, 1):\n");
    print_edge(edge_1_6, 0);
    destroy_edge(graph, edge_1_6);
    printf("Graph after destruction:\n");
    print_graph(graph, 0);

    marker_0 = alloc_marker(graph);
    printf("Created marker with id = %u\n", marker_0);
    print_marker(graph, marker_0, 0);
    marker_1 = alloc_marker(graph);
    printf("Created marker with id = %u\n", marker_1);
    print_marker(graph, marker_1, 0);

    printf("Set vertex 1 with marker 0\n");
    set_marker_vertex(graph, vertex_1, marker_0);
    printf("Set vertex 5 with marker 0\n");
    set_marker_vertex(graph, vertex_5, marker_0);
    printf("Set vertex 1 with marker 1\n");
    set_marker_vertex(graph, vertex_1, marker_1);
    printf("Set vertex 5 with marker 1\n");
    set_marker_vertex(graph, vertex_5, marker_1);
    printf("Set edge (1, 8) with marker 0\n");
    set_marker_edge(graph, edge_1_8, marker_0);
    printf("Set edge (5, 6) with marker 0\n");
    set_marker_edge(graph, edge_5_6, marker_0);
    printf("Set edge (1, 8) with marker 1\n");
    set_marker_edge(graph, edge_1_8, marker_1);
    printf("Set edge (5, 6) with marker 1\n");
    set_marker_edge(graph, edge_5_6, marker_1);
    printf("Vertexes after set:\n");
    print_vertex(vertex_1, 0);
    print_vertex(vertex_5, 0);
    printf("Edges after set:\n");
    print_edge(edge_1_8, 0);
    print_edge(edge_5_6, 0);
    printf("Markers after set:\n");
    print_marker(graph, marker_0, 0);
    print_marker(graph, marker_1, 0);
    printf("Graph after set:\n");
    print_graph(graph, 0);

    printf("Unset vertex 1 with marker 0\n");
    unset_marker_vertex(graph, vertex_1, marker_0);
    printf("Unset edge (5, 6) with marker 1\n");
    unset_marker_edge(graph, edge_5_6, marker_1);
    printf("Vertexes after unset:\n");
    print_vertex(vertex_1, 0);
    print_vertex(vertex_5, 0);
    printf("Edges after unset:\n");
    print_edge(edge_1_8, 0);
    print_edge(edge_5_6, 0);
    printf("Markers after unset:\n");
    print_marker(graph, marker_0, 0);
    print_marker(graph, marker_1, 0);
    printf("Graph after unset:\n");
    print_graph(graph, 0);

    printf("Check vertex 1 set with marker 0: %s\n",
           check_marker_vertex(vertex_1, marker_0) ? "TRUE" : "FALSE");
    printf("Check vertex 5 set with marker 0: %s\n",
           check_marker_vertex(vertex_5, marker_0) ? "TRUE" : "FALSE");
    printf("Check vertex 1 set with marker 1: %s\n",
           check_marker_vertex(vertex_1, marker_1) ? "TRUE" : "FALSE");
    printf("Check vertex 5 set with marker 1: %s\n",
           check_marker_vertex(vertex_5, marker_1) ? "TRUE" : "FALSE");
    printf("Check edge (1, 8) set with marker 0: %s\n",
           check_marker_edge(edge_1_8, marker_0) ? "TRUE" : "FALSE");
    printf("Check edge (5, 6) set with marker 0: %s\n",
           check_marker_edge(edge_5_6, marker_0) ? "TRUE" : "FALSE");
    printf("Check edge (1, 8) set with marker 1: %s\n",
           check_marker_edge(edge_1_8, marker_1) ? "TRUE" : "FALSE");
    printf("Check edge (5, 6) set with marker 1: %s\n",
           check_marker_edge(edge_5_6, marker_1) ? "TRUE" : "FALSE");

    printf("Free marker_1\n");
    free_marker(graph, marker_1);
    printf("Graph after free marker_1\n");
    print_graph(graph, 0);

    err = 0;

destroy_graph_due_to_fail:
    destroy_graph(graph);

fail_create_graph:
    return err;
}

