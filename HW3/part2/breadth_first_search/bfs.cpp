#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <omp.h>
#include <vector>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set *list)
{
    list->count = 0;
}

void vertex_set_init(vertex_set *list, int count)
{
    list->max_vertices = count;
    list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
    vertex_set_clear(list);
}

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// new_frontier.
void top_down_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *new_frontier,
    int *distances)
{
    int curr_distance = distances[frontier->vertices[0]];

    #pragma omp parallel
    {   
        std::vector<int> local_frontier;
        int frontier_count = frontier->count;

        #pragma omp for
        for (int i = 0; i < frontier_count; i++)
        {

            int node = frontier->vertices[i];

            int start_edge = g->outgoing_starts[node];
            int end_edge = (node == g->num_nodes - 1)
                            ? g->num_edges
                            : g->outgoing_starts[node + 1];

            // attempt to add all neighbors to the new frontier
            for (int neighbor = start_edge; neighbor < end_edge; ++neighbor)
            {
                int outgoing = g->outgoing_edges[neighbor];
                
                if (distances[outgoing] != NOT_VISITED_MARKER)
                    continue;

                __sync_val_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, curr_distance + 1);
                local_frontier.push_back(outgoing);
            }
        }


        size_t count = local_frontier.size();
        int *vertices = new_frontier->vertices;
        int offset = __sync_fetch_and_add(&new_frontier->count, count);

        for (int j = 0; j < count; ++j)
            vertices[offset + j] = local_frontier[j];
    }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol)
{

    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        top_down_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        std::swap(frontier, new_frontier);
    }
}


bool *visited;
void bottom_up_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *new_frontier,
    int *distances)
{
    int curr_distance = distances[frontier->vertices[0]];
    
    #pragma omp parallel
    {   
        std::vector<int> local_frontier;
        int num_nodes = g->num_nodes;
        int *incoming_edges = g->incoming_edges;

        #pragma omp for
        for (auto i = 0; i < num_nodes; ++i)
        {
            if (visited[i])
                continue;

            int start_edge = g->incoming_starts[i];
            int end_edge = (i == g->num_nodes - 1)
                                ? g->num_edges
                                : g->incoming_starts[i + 1];

            for (int neighbor = start_edge; neighbor < end_edge; ++neighbor)
            {   
                int incoming = incoming_edges[neighbor];
                if (visited[incoming])
                {
                    distances[i] = curr_distance + 1;
                    local_frontier.push_back(i);
                    break;
                }
            } 
        }


        size_t count = local_frontier.size();
        int *vertices = new_frontier->vertices;
        int offset = __sync_fetch_and_add(&new_frontier->count, count);

        for (int j = 0; j < count; ++j)
            vertices[offset + j] = local_frontier[j];
    }
}

void bfs_bottom_up(Graph graph, solution *sol)
{
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    visited = new bool[graph->num_nodes];

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
    {
        sol->distances[i] = NOT_VISITED_MARKER;
        visited[i] = false;
    }

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    visited[ROOT_NODE_ID] = true;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        bottom_up_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        #pragma omp parallel for
        for (int i = 0; i < new_frontier->count; ++i)
        {
            visited[new_frontier->vertices[i]] = true;
        }

        // swap pointers
        std::swap(frontier, new_frontier);
    }

    delete[] visited;
}


int num_unvisited;
void bfs_hybrid(Graph graph, solution *sol)
{
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    visited = new bool[graph->num_nodes];
    num_unvisited = graph->num_nodes - 1;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
    {
        sol->distances[i] = NOT_VISITED_MARKER;
        visited[i] = false;
    }

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    visited[ROOT_NODE_ID] = true;

    bool method;
    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif
        
        vertex_set_clear(new_frontier);

        //method = frontier->count < num_unvisited? 1 : 0;
        method = frontier->count < graph->num_nodes * 0.05? 1 : 0;
        if (method)
            top_down_step(graph, frontier, new_frontier, sol->distances);
        else
            bottom_up_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        //num_unvisited -= new_frontier->count;
        #pragma omp parallel for
        for (int i = 0; i < new_frontier->count; ++i)
        {
            visited[new_frontier->vertices[i]] = true;
        }

        // swap pointers
        std::swap(frontier, new_frontier);
    }

    delete[] visited;
}
