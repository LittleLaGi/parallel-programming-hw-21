#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

void partial_pi(long long int* partial_sum, long long int tosses);

int main(int argc, char **argv)
{
    // --- DON'T TOUCH ---
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();
    double pi_result;
    long long int tosses = atoi(argv[1]);
    int world_rank, world_size;
    // ---
    
    // TODO: init MPI
    long long int partial_sum = 0;

    // TODO
    partial_pi(&partial_sum, tosses);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if (world_rank == 0)
    {
        // TODO: process PI result
        long long int* partial_sums = new long long int[world_size];
        partial_sums[0] = partial_sum;
        MPI_Request requests[world_size - 1];
        MPI_Status status[world_size - 1];

        for (int i = 1; i < world_size; ++i)
            MPI_Irecv(partial_sums + i, 1, MPI_LONG_LONG, i, 0, MPI_COMM_WORLD, requests + i - 1);
        MPI_Waitall(world_size - 1, requests, status);


        long long int number_in_circle = 0;
        for (int i = 0; i < world_size; ++i)
            number_in_circle += partial_sums[i];
        pi_result = (double)4 * number_in_circle / tosses;
        delete[] partial_sums;

        // --- DON'T TOUCH ---
        double end_time = MPI_Wtime();
        printf("%lf\n", pi_result);
        printf("MPI running time: %lf Seconds\n", end_time - start_time);
        // ---
    }

    MPI_Finalize();
    return 0;
}

void partial_pi(long long int* partial_sum, long long int tosses){
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    unsigned int seed;
    srand(time(0));
    seed = rand() * rank;

    double x;
    double y;
    double distance_squared;
    long long int num_iter = tosses / world_size;
    long long int toss = 0;
    for (; toss < num_iter; ++toss) {
        x = (double)rand_r(&seed)/RAND_MAX;
        y = (double)rand_r(&seed)/RAND_MAX;
        distance_squared = x * x + y * y;
        if ( distance_squared <= 1)
            ++(*partial_sum);
    }

    if (rank > 0)
        MPI_Send(partial_sum, 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
}