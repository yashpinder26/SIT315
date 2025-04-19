#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>

#define MATRIX_SIZE 100

int matrixA[MATRIX_SIZE][MATRIX_SIZE], matrixB[MATRIX_SIZE][MATRIX_SIZE], resultMatrix[MATRIX_SIZE][MATRIX_SIZE];

void initialize_matrices() {
    for (int row = 0; row < MATRIX_SIZE; row++)
        for (int col = 0; col < MATRIX_SIZE; col++) {
            matrixA[row][col] = rand() % 10;
            matrixB[row][col] = rand() % 10;
        }
}

int main(int argc, char** argv) {
    int currentRank, totalProcesses;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &currentRank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    int rowsPerProcess = MATRIX_SIZE / totalProcesses;
    int partialA[rowsPerProcess][MATRIX_SIZE], partialC[rowsPerProcess][MATRIX_SIZE];

    if (currentRank == 0) {
        srand(time(NULL));
        initialize_matrices();
    }

    MPI_Bcast(matrixB, MATRIX_SIZE * MATRIX_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(matrixA, rowsPerProcess * MATRIX_SIZE, MPI_INT, partialA, rowsPerProcess * MATRIX_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

    double computationStart = MPI_Wtime();

    for (int i = 0; i < rowsPerProcess; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            partialC[i][j] = 0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                partialC[i][j] += partialA[i][k] * matrixB[k][j];
            }
        }
    }

    double computationEnd = MPI_Wtime();

    MPI_Gather(partialC, rowsPerProcess * MATRIX_SIZE, MPI_INT, resultMatrix, rowsPerProcess * MATRIX_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

    if (currentRank == 0) {
        std::cout << "Parallel execution time using MPI: " << (computationEnd - computationStart) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}
