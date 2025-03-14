#include <chrono>
#include <cstdlib>
#include <iostream>
#include <time.h>

using namespace std::chrono;
using namespace std;

// Function to fill a vector with random integers
void randomVector(int vector[], int size) {
  for (int i = 0; i < size; i++) {
    vector[i] = rand() % 100;  // Assigning a random number between 0-99
  }
}

int main() {
  unsigned long size = 100000000;  // Size of vectors (100 million elements)
  srand(time(0));  // Seed random number generator

  int *v1, *v2, *v3;
  
  // Measure execution time
  auto start = high_resolution_clock::now();

  // Dynamically allocate memory for large vectors
  v1 = (int *)malloc(size * sizeof(int));
  v2 = (int *)malloc(size * sizeof(int));
  v3 = (int *)malloc(size * sizeof(int));

  // Fill vectors with random values
  randomVector(v1, size);
  randomVector(v2, size);

  // Perform vector addition
  for (int i = 0; i < size; i++) {
    v3[i] = v1[i] + v2[i];
  }

  // Stop timing
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);

  // Output execution time
  cout << "Time taken by function: " << duration.count() << " microseconds" << endl;

  // Free allocated memory
  free(v1);
  free(v2);
  free(v3);

  return 0;
}
