#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <omp.h>

using namespace std;
using namespace std::chrono;

int partition(vector<int>& vec, int low, int high) {
    int pivot = vec[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (vec[j] <= pivot) {
            ++i;
            swap(vec[i], vec[j]);
        }
    }
    swap(vec[i + 1], vec[high]);
    return i + 1;
}

void parallelQuickSort(vector<int>& vec, int low, int high) {
    if (low < high) {
        int pi = partition(vec, low, high);

        #pragma omp parallel sections
        {
            #pragma omp section
            parallelQuickSort(vec, low, pi - 1);

            #pragma omp section
            parallelQuickSort(vec, pi + 1, high);
        }
    }
}

int main() {
    srand(time(0));
    int n;
    cout << "Enter number of elements: ";
    cin >> n;

    vector<int> vec(n);
    for (int& num : vec) {
        num = rand() % 1000;
    }

    auto start = high_resolution_clock::now();

    #pragma omp parallel
    {
        #pragma omp single
        parallelQuickSort(vec, 0, n - 1);
    }

    auto end = high_resolution_clock::now();

    cout << "Parallel Execution Time: " << duration_cast<microseconds>(end - start).count();
    return 0;
}
