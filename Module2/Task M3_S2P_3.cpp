#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono;

void randomVector(int vector[], int size) {
    for (int i = 0; i < size; i++) {
        vector[i] = rand() % 100;
    }
}

int main(int argc, char* argv[]) {
    int rank, size;
    unsigned long total_size = 100000000;
    srand(time(0));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int chunk_size = total_size / size;

    int* v1 = nullptr;
    int* v2 = nullptr;
    int* v3 = (int*)malloc(chunk_size * sizeof(int));

    if (rank == 0) {
        // Allocate full vectors in master
        v1 = (int*)malloc(total_size * sizeof(int));
        v2 = (int*)malloc(total_size * sizeof(int));

        randomVector(v1, total_size);
        randomVector(v2, total_size);
    }

    auto start = high_resolution_clock::now();

    // Scatter v1 and v2 to all processes
    MPI_Scatter(v1, chunk_size, MPI_INT, v1, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(v2, chunk_size, MPI_INT, v2, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Each process computes its part of the addition
    for (int i = 0; i < chunk_size; i++) {
        v3[i] = v1[i] + v2[i];
    }

    // Gather results to master process
    MPI_Gather(v3, chunk_size, MPI_INT, v3, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    auto stop = high_resolution_clock::now();

    if (rank == 0) {
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by MPI vector addition: " << duration.count() << " microseconds" << endl;

        // Total sum using MPI_Reduce
        long long total_sum = 0;
        long long partial_sum = 0;
        for (unsigned long i = 0; i < total_size; i++) {
            partial_sum += v3[i];
        }

        MPI_Reduce(&partial_sum, &total_sum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        cout << "Total sum of v3 elements = " << total_sum << endl;

        free(v1);
        free(v2);
        free(v3);
    }

    MPI_Finalize();
    return 0;
}
