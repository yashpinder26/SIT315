#include <iostream>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Args {
    vector<int>* vec;
    int low;
    int high;
};

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

void quickSort(vector<int>& vec, int low, int high) {
    if (low < high) {
        int pi = partition(vec, low, high);
        quickSort(vec, low, pi - 1);
        quickSort(vec, pi + 1, high);
    }
}

void* threadedQuickSort(void* arg) {
    Args* args = (Args*)arg;
    quickSort(*args->vec, args->low, args->high);
    return nullptr;
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

    int pi = partition(vec, 0, n - 1);

    Args left = { &vec, 0, pi - 1 };
    Args right = { &vec, pi + 1, n - 1 };

    pthread_t t1, t2;
    pthread_create(&t1, nullptr, threadedQuickSort, &left);
    pthread_create(&t2, nullptr, threadedQuickSort, &right);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    auto end = high_resolution_clock::now();
    cout << "Execution Time: " << duration_cast<microseconds>(end - start).count();
    return 0;
}
