#include <iostream>
#include <vector>
#include <chrono>
#include <pthread.h>
#include <omp.h>

#define N 100 // Matrix size 
#define NUM_THREADS 4

using namespace std;
using namespace chrono;

vector<vector<int>> A(N, vector<int>(N));
vector<vector<int>> B(N, vector<int>(N));
vector<vector<int>> C_seq(N, vector<int>(N, 0));
vector<vector<int>> C_pthread(N, vector<int>(N, 0));
vector<vector<int>> C_openmp(N, vector<int>(N, 0));

// Function to initialize matrices with random values
void initialize_matrices() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }
}

// Sequential Matrix Multiplication
void sequential_multiplication() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C_seq[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Thread function for Pthreads parallel multiplication
struct ThreadData {
    int start_row, end_row;
};

void* pthread_multiplication(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C_pthread[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return NULL;
}

// OpenMP Parallel Matrix Multiplication
void openmp_multiplication() {
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C_openmp[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main() {
    initialize_matrices();

    // Sequential Execution
    auto start = high_resolution_clock::now();
    sequential_multiplication();
    auto stop = high_resolution_clock::now();
    cout << "Sequential Execution Time: " << duration_cast<milliseconds>(stop - start).count() << " ms" << endl;

    // Pthreads Execution
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    int rows_per_thread = N / NUM_THREADS;

    start = high_resolution_clock::now();
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].start_row = i * rows_per_thread;
        thread_data[i].end_row = (i + 1) * rows_per_thread;
        pthread_create(&threads[i], NULL, pthread_multiplication, (void*)&thread_data[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    stop = high_resolution_clock::now();
    cout << "Pthreads Execution Time: " << duration_cast<milliseconds>(stop - start).count() << " ms" << endl;

    // OpenMP Execution
    start = high_resolution_clock::now();
    openmp_multiplication();
    stop = high_resolution_clock::now();
    cout << "OpenMP Execution Time: " << duration_cast<milliseconds>(stop - start).count() << " ms" << endl;

    return 0;
}
