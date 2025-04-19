#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <omp.h>

#define N 100

int matrixA[N][N], matrixB[N][N], resultMatrix[N][N];
int localMatrixA[N][N], localMatrixC[N][N];

// Function to randomly initialize matrixA and matrixB
void initialize_matrices() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            matrixA[i][j] = rand() % 10;
            matrixB[i][j] = rand() % 10;
        }
}

int main(int argc, char** argv) {
    int currentRank, totalProcesses;
    MPI_Init(&argc, &argv);                      // Initialize MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &currentRank);  // Get the current process ID
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses); // Get total number of processes

    int rowsPerProcess = N / totalProcesses;      // Number of rows each process handles

    if (currentRank == 0) {
        srand(time(NULL));                       // Seed random number generator
        initialize_matrices();                   // Initialize matrixA and matrixB with random values
    }

    // Broadcast matrixB to all processes
    MPI_Bcast(matrixB, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    // Distribute rows of matrixA to each process
    MPI_Scatter(matrixA, rowsPerProcess * N, MPI_INT, localMatrixA, rowsPerProcess * N, MPI_INT, 0, MPI_COMM_WORLD);

    double startTime = MPI_Wtime();             // Start the timer

    // Perform matrix multiplication using OpenMP parallel loops
    #pragma omp parallel for
    for (int i = 0; i < rowsPerProcess; i++) {
        for (int j = 0; j < N; j++) {
            localMatrixC[i][j] = 0;
            for (int k = 0; k < N; k++) {
                localMatrixC[i][j] += localMatrixA[i][k] * matrixB[k][j];
            }
        }
    }

    double endTime = MPI_Wtime();               // Stop the timer

    // Gather the results from all processes into the final resultMatrix
    MPI_Gather(localMatrixC, rowsPerProcess * N, MPI_INT, resultMatrix, rowsPerProcess * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (currentRank == 0) {
        std::cout << "Parallel Execution Time (MPI + OpenMP): " << (endTime - startTime) << " seconds\n";
    }

    MPI_Finalize();                              // Close MPI environment
    return 0;
}
