#include "page_rank.h"

#include <stdlib.h>
#include <cmath>
#include <omp.h>
#include <utility>

#include "../common/CycleTimer.h"
#include "../common/graph.h"


// pageRank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is num_nodes(g))
// damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void pageRank(Graph g, double *solution, double damping, double convergence)
{
  //  For PP students: Implement the page rank algorithm here.  You
  //  are expected to parallelize the algorithm using openMP.  Your
  //  solution may need to allocate (and free) temporary arrays.

  int numNodes = num_nodes(g);
  double equal_prob = 1.0 / numNodes;
  double global_diff;
  bool converged = false;
  double *score_old = new double[numNodes];
  double *score_new = new double[numNodes];

  // initialize vertex weights to uniform probability. Double
  // precision scores are used to avoid underflow for large graphs
  #pragma omp parallel for
  for (int i = 0; i < numNodes; ++i)
  {
    score_old[i] = equal_prob;
  }

  while (!converged) {

    // compute score_new[vi] for all nodes vi:
    #pragma omp parallel for
    for (int i = 0; i < numNodes; ++i)
    {
      const Vertex* start = incoming_begin(g, i);
      const Vertex* end = incoming_end(g, i);
      score_new[i] = 0.0;
      for (const Vertex* v = start; v != end; v++)
      {
        score_new[i] += score_old[*v] / outgoing_size(g, *v);
      }

      score_new[i] = (damping * score_new[i]) + (1.0-damping) / numNodes;
    }

    double partial_sum = 0.0;
    #pragma omp parallel for reduction(+:partial_sum)
    for (int i = 0; i < numNodes; ++i)
    {
      if (outgoing_size(g, i) == 0)
        partial_sum += damping * score_old[i] / numNodes;
    }

    #pragma omp parallel for
    for (int i = 0; i < numNodes; ++i)
      score_new[i] += partial_sum;

    // compute how much per-node scores have changed
    // quit once algorithm has converged
    global_diff = 0.0;
    #pragma omp parallel for reduction(+:global_diff)
    for (int i = 0; i < numNodes; ++i)
    {
      global_diff += abs(score_new[i] - score_old[i]);
    }
    
    converged = global_diff < convergence;

    double *tmp;
    tmp = score_old;
    score_old = score_new;
    score_new = tmp;
  }

  #pragma omp parallel for
  for (int i = 0; i < numNodes; ++i)
    solution[i] = score_old[i];

  delete[] score_old;
  delete[] score_new;
}
