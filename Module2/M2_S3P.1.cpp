#include <omp.h>                        // Added OpenMP library
#include <iostream>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main() {
  unsigned long size = 100000000;

  // Changed from malloc to C++ style new
  int *v1 = new int[size];
  int *v2 = new int[size];
  int *v3 = new int[size];

  // Moved random value assignment here (removed randomVector function)
  for (unsigned long i = 0; i < size; i++) {
    v1[i] = rand() % 100;
    v2[i] = rand() % 100;
  }

  auto start = high_resolution_clock::now();

  // Added OpenMP parallel for loop here
  #pragma omp parallel for
  for (unsigned long i = 0; i < size; i++) {
    v3[i] = v1[i] + v2[i];
  }

  auto stop = high_resolution_clock::now();
  cout << "Time taken: " << duration_cast<microseconds>(stop - start).count() << " microseconds" << endl;

  // Changed from free() to delete[]
  delete[] v1;
  delete[] v2;
  delete[] v3;

  return 0;
}
