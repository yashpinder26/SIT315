#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <CL/cl.h>

const int N = 1000;

int matrixA[N][N], matrixB[N][N], resultMatrix[N][N];
int localMatrixA[N][N], localMatrixC[N][N];

const char* kernelSource = R"(
__kernel void mat_mul(
    __global int* matrixA,
    __global int* matrixB,
    __global int* resultMatrix,
    int N,
    int rows_per_proc
) {
    int row = get_global_id(0);
    for (int j = 0; j < N; j++) {
        int sum = 0;
        for (int k = 0; k < N; k++) {
            sum += matrixA[row * N + k] * matrixB[k * N + j];
        }
        resultMatrix[row * N + j] = sum;
    }
})";

// Function to randomly initialize matrixA and matrixB
void initialize_matrices() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            matrixA[i][j] = rand() % 10;
            matrixB[i][j] = rand() % 10;
        }

int main(int argc, char** argv) {
    int currentRank, totalProcesses;
    MPI_Init(&argc, &argv);                      // Initialize MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &currentRank);  // Get the current process ID
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses); // Get total number of processes

    if (currentRank == 0) {
        std::cout << "[MPI] Running with " << totalProcesses << " process(es)\n";
    }

    int rowsPerProcess = N / totalProcesses;      // Number of rows each process handles

    if (currentRank == 0) {
        srand(time(NULL));                       // Seed random number generator
        initialize_matrices();                   // Initialize matrixA and matrixB with random values
    }

    // Broadcast matrixB to all processes
    MPI_Bcast(matrixB, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    // Distribute rows of matrixA to each process
    MPI_Scatter(matrixA, rowsPerProcess * N, MPI_INT, localMatrixA, rowsPerProcess * N, MPI_INT, 0, MPI_COMM_WORLD);

    // OpenCL setup
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    cl_mem bufA, bufB, bufC;
    cl_int err;

    // Get platform and device
    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);

    // Create buffers
    bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * rowsPerProcess * N, NULL, &err);
    bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * N * N, NULL, &err);
    bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * rowsPerProcess * N, NULL, &err);

    // Write data to buffers
    err = clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, sizeof(int) * rowsPerProcess * N, localMatrixA, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, sizeof(int) * N * N, matrixB, 0, NULL, NULL);

    // Build OpenCL program
    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "mat_mul", &err);

    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &N);
    err |= clSetKernelArg(kernel, 4, sizeof(int), &rowsPerProcess);

    // Set global size for OpenCL kernel
    size_t global[1] = { static_cast<size_t>(rowsPerProcess) };

    double startTime = MPI_Wtime();             // Start the timer
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, NULL);
    clFinish(queue);
    double endTime = MPI_Wtime();               // Stop the timer

    // Read results from buffer
    err = clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, sizeof(int) * rowsPerProcess * N, localMatrixC, 0, NULL, NULL);

    // Release OpenCL resources
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    // Gather the results from all processes into the final resultMatrix
    MPI_Gather(localMatrixC, rowsPerProcess * N, MPI_INT, resultMatrix, rowsPerProcess * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (currentRank == 0) {
        std::cout << "MPI + OpenCL execution Time: " << (endTime - startTime) << " seconds\n";
    }

    MPI_Finalize();                              // Close MPI environment
    return 0;
}
 
