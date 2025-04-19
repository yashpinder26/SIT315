#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <pthread.h>

#define N 100  // Matrix size
#define MAX_THREADS 4  // Maximum number of threads

int matrix_A[N][N], matrix_B[N][N], matrix_C[N][N];  // Matrices A, B, and C for multiplication

int rows_per_process;  // Rows per process
int sub_matrix_A[N][N], sub_matrix_C[N][N];  // Sub-matrices for each process
int rank;  // MPI rank of the process

// Structure to hold thread-specific data
struct ThreadData {
    int start_row;  // Starting row for the thread
    int end_row;  // Ending row for the thread
};

// Thread function to perform matrix multiplication for a specific range of rows
void* multiply_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;  // Extract thread data

    // Multiply sub-matrix A with matrix B and store the result in sub-matrix C
    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < N; j++) {
            sub_matrix_C[i][j] = 0;  // Initialize result for cell (i, j)
            for (int k = 0; k < N; k++) {
                sub_matrix_C[i][j] += sub_matrix_A[i][k] * matrix_B[k][j];  // Matrix multiplication
            }
        }
    }

    pthread_exit(NULL);  // Exit the thread
}

// Function to initialize matrices A and B with random values
void initialize_matrices() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            matrix_A[i][j] = rand() % 10;  // Random values between 0 and 9
            matrix_B[i][j] = rand() % 10;  // Random values between 0 and 9
        }
}

int main(int argc, char** argv) {
    int total_processes;  // Total number of processes in MPI

    MPI_Init(&argc, &argv);  // Initialize MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get the rank of the process
    MPI_Comm_size(MPI_COMM_WORLD, &total_processes);  // Get the total number of processes

    rows_per_process = N / total_processes;  // Calculate number of rows per process

    // Only rank 0 initializes the matrices A and B
    if (rank == 0) {
        srand(time(NULL));  // Seed random number generator
        initialize_matrices();  // Initialize matrix_A and matrix_B with random values
    }

    // Broadcast matrix B to all processes
    MPI_Bcast(matrix_B, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    // Scatter rows of matrix A to all processes
    MPI_Scatter(matrix_A, rows_per_process * N, MPI_INT, sub_matrix_A, rows_per_process * N, MPI_INT, 0, MPI_COMM_WORLD);

    pthread_t threads[MAX_THREADS];  // Array to store thread IDs
    ThreadData thread_data[MAX_THREADS];  // Array to store thread data

    double start_time = MPI_Wtime();  // Start timing the computation

    // Create threads to perform matrix multiplication in parallel
    for (int t = 0; t < MAX_THREADS; t++) {
        thread_data[t].start_row = t * (rows_per_process / MAX_THREADS);  // Assign start row
        thread_data[t].end_row = (t + 1) * (rows_per_process / MAX_THREADS);  // Assign end row
        pthread_create(&threads[t], NULL, multiply_thread, (void*)&thread_data[t]);  // Create thread
    }

    // Wait for all threads to finish
    for (int t = 0; t < MAX_THREADS; t++) {
        pthread_join(threads[t], NULL);  // Join each thread
    }

    double end_time = MPI_Wtime();  // End timing the computation

    // Gather the result from all processes
    MPI_Gather(sub_matrix_C, rows_per_process * N, MPI_INT, matrix_C, rows_per_process * N, MPI_INT, 0, MPI_COMM_WORLD);

    // Only rank 0 prints the execution time
    if (rank == 0) {
        std::cout << "MPI + Pthreads execution Time - Multithreading: " << (end_time - start_time) << " seconds\n";
    }

    MPI_Finalize();  // Finalize MPI
    return 0;  // Exit the program
}
 
