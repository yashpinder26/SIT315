#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace chrono;

// Partition function for quicksort
int partition(vector<int>& arr, int left, int right) {
    int pivot = arr[right];
    int i = left - 1;
    for(int j = left; j < right; j++) {
        if(arr[j] <= pivot) {
            swap(arr[++i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[right]);
    return i + 1;
}

// Recursive quicksort
void quicksort(vector<int>& arr, int left, int right) {
    if(left < right) {
        int p = partition(arr, left, right);
        quicksort(arr, left, p - 1);
        quicksort(arr, p + 1, right);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int test_rank, test_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &test_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &test_size);

    vector<int> all_data;
    int total_elements;

    steady_clock::time_point start_time, end_time;

    if(test_rank == 0) {
        total_elements = 10000;
        all_data.resize(total_elements);
        for(int i = 0; i < total_elements; ++i) {
            all_data[i] = rand() % 1000000;
        }
        start_time = steady_clock::now();
    }

    MPI_Bcast(&total_elements, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int part_size = total_elements / test_size;
    vector<int> local_data(part_size);

    MPI_Scatter(all_data.data(), part_size, MPI_INT,
                local_data.data(), part_size, MPI_INT, 0, MPI_COMM_WORLD);

    quicksort(local_data, 0, part_size - 1);

    vector<int> final_data;
    if(test_rank == 0) final_data.resize(total_elements);

    MPI_Gather(local_data.data(), part_size, MPI_INT,
               final_data.data(), part_size, MPI_INT, 0, MPI_COMM_WORLD);

    if(test_rank == 0) {
        sort(final_data.begin(), final_data.end());
        end_time = steady_clock::now();
        auto time_taken = duration_cast<milliseconds>(end_time - start_time);

        cout << "Top 10 sorted numbers: ";
        for(int i = 0; i < 10; ++i) cout << final_data[i] << " ";
        cout << "\nExecution Time: " << time_taken.count() << " ms" << endl;
    }

    MPI_Finalize();
    return 0;
}
