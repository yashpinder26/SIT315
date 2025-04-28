__kernel void quick_sort(__global int* arr, int chunk_size) {
    int id = get_global_id(0);
    int left = id * chunk_size;
    int right = left + chunk_size - 1;

    if (left >= right) return;

    int stack[300000];
    int top = -1;

    stack[++top] = left;
    stack[++top] = right;

    while (top >= 0) {
        right = stack[top--];
        left = stack[top--];

        int pivot = arr[right];
        int i = left - 1;

        for (int j = left; j < right; j++) {
            if (arr[j] <= pivot) {
                i++;
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }

        int temp = arr[i + 1];
        arr[i + 1] = arr[right];
        arr[right] = temp;

        int pivot_index = i + 1;

        // Push left side to stack if it exists
        if (pivot_index - 1 > left) {
            stack[++top] = left;
            stack[++top] = pivot_index - 1;
        }

        // Push right side to stack if it exists
        if (pivot_index + 1 < right) {
            stack[++top] = pivot_index + 1;
            stack[++top] = right;
        }
    }
}
