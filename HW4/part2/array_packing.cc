#include <iostream>
#include <mpi.h>

void construct_matrices(int *__restrict__ n_ptr, int * __restrict__ m_ptr, int * __restrict__ l_ptr,
                        int ** __restrict__ a_mat_ptr, int ** __restrict__ b_mat_ptr){

    std::ios_base::sync_with_stdio(false);
    
    int rank, num_nodes;
    int n_m_l[3], n, m ,l;
    int chunk_n;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    if (rank == 0)
        std::cin >> n_m_l[0] >> n_m_l[1] >> n_m_l[2];

    MPI_Bcast(n_m_l, 3, MPI_INT, 0, MPI_COMM_WORLD);
    *n_ptr = n = n_m_l[0];
    *m_ptr = m = n_m_l[1];
    *l_ptr = l = n_m_l[2];

    if (rank == 0)
    {
        *a_mat_ptr = new int[n * m];
        *b_mat_ptr = new int[m * l];

        for (int i = 0; i < n * m; ++i)
            std::cin >> (*a_mat_ptr)[i];
        // array packing
        for (int i = 0; i < m ; ++i)
            for (int j = 0; j < l; ++j)
                std::cin >> (*b_mat_ptr)[j * m + i];
    }

    chunk_n = n / num_nodes;
    chunk_n = rank == num_nodes - 1? chunk_n + n % num_nodes : chunk_n;

    if (rank != 0)
    {
        *a_mat_ptr = new int[chunk_n * m];
        *b_mat_ptr = new int[m * l];
    }

    if (rank == 0)
    {
        for (int i = 1; i < num_nodes - 1; ++i)
            MPI_Send(&(*a_mat_ptr)[i * chunk_n * m], chunk_n * m, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&(*a_mat_ptr)[(num_nodes - 1) * (n / num_nodes) * m], (n - chunk_n * (num_nodes - 1)) * m, MPI_INT, num_nodes - 1, 0, MPI_COMM_WORLD);
    }
    else
        MPI_Recv(*a_mat_ptr, chunk_n * m, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Bcast(*b_mat_ptr, m * l, MPI_INT, 0, MPI_COMM_WORLD);
}


void matrix_multiply(const int n, const int m, const int l,
                     const int * __restrict__ a_mat, const int * __restrict__ b_mat){
    int rank, num_nodes;
    int chunk_n;
    int* output;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    chunk_n = n / num_nodes;
    chunk_n = rank == num_nodes - 1? chunk_n + n % num_nodes : chunk_n;

    if (rank == 0)
        output = new int[n * l];
    else
        output = new int[chunk_n * l];
    
    for (int i = 0; i < chunk_n * l; ++i)
        output[i] = 0;

    // multiply
    // array packing
    for (int i = 0; i < chunk_n; ++i)
    {   
        const int output_ii = i * l;
        const a_ii = i * m;
        for (int j = 0; j < l; ++j)
        {
            const int output_index = output_ii + j;
            const b_jj = j * m;
            for (int k = 0; k < m; ++k)
            {
                output[output_index] += a_mat[a_ii + k] * b_mat[b_jj + k];
            }
        }
    }

    // send result
    if (rank == 0)
    {      
        for (int i = 1; i < num_nodes - 1; i++)
            MPI_Recv(&output[chunk_n * i * l], chunk_n * l, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&output[(num_nodes - 1) * (n / num_nodes) * l], (n - chunk_n * (num_nodes - 1)) * l, MPI_INT, num_nodes - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else
        MPI_Send(output, chunk_n * l, MPI_INT, 0, 0, MPI_COMM_WORLD);

    /* print out the result */
    if (rank == 0)
    {  
        // for (int i = 0; i < n; ++i)
        // {
        //     for (int j = 0; j < l; ++j)
        //         std::cout << output[i * l + j] << " ";
        //     std::cout << "\n";
        // }
    }

    delete[] output;
}


void destruct_matrices(int *a_mat, int *b_mat){
    delete[] a_mat;
    delete[] b_mat;
}