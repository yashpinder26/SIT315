#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

const int NUM_THREADS = 8;  // Adjust based on your system

void randomVector(int vector[], int start, int end) {
  for (int i = start; i < end; i++) {
    vector[i] = rand() % 100;  // Assign random numbers
  }
}

void vectorAdd(int v1[], int v2[], int v3[], int start, int end) {
  for (int i = start; i < end; i++) {
    v3[i] = v1[i] + v2[i];
  }
}

int main() {
  unsigned long size = 100000000;
  srand(time(0));

  int *v1, *v2, *v3;
  v1 = (int *)malloc(size * sizeof(int));
  v2 = (int *)malloc(size * sizeof(int));
  v3 = (int *)malloc(size * sizeof(int));

  auto start = high_resolution_clock::now();

  // Create threads to fill vectors in parallel
  vector<thread> threads;
  int chunk_size = size / NUM_THREADS;

  for (int i = 0; i < NUM_THREADS; i++) {
    int start_idx = i * chunk_size;
    int end_idx = (i == NUM_THREADS - 1) ? size : start_idx + chunk_size;
    threads.emplace_back(randomVector, v1, start_idx, end_idx);
  }

  for (auto &t : threads) t.join();
  threads.clear();

  for (int i = 0; i < NUM_THREADS; i++) {
    int start_idx = i * chunk_size;
    int end_idx = (i == NUM_THREADS - 1) ? size : start_idx + chunk_size;
    threads.emplace_back(randomVector, v2, start_idx, end_idx);
  }

  for (auto &t : threads) t.join();
  threads.clear();

  // Create threads to perform parallel vector addition
  for (int i = 0; i < NUM_THREADS; i++) {
    int start_idx = i * chunk_size;
    int end_idx = (i == NUM_THREADS - 1) ? size : start_idx + chunk_size;
    threads.emplace_back(vectorAdd, v1, v2, v3, start_idx, end_idx);
  }

  for (auto &t : threads) t.join();

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);

  cout << "Time taken by parallel function: " << duration.count() << " microseconds" << endl;

  free(v1);
  free(v2);
  free(v3);

  return 0;
}
