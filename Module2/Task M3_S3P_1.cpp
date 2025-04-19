#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <chrono>

#define PRINT 1  

int SZ = 100000000; // Default size of the vectors

// Host-side arrays
int *v1, *v2, *v_out;

// OpenCL memory buffers (device-side)
cl_mem bufV1, bufV2, bufV_out;

// OpenCL object handles
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue queue;
cl_event event = NULL;

int err;

// Function declarations
cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory();
void copy_kernel_args();
void free_memory();
void init(int *&A, int size);
void print(int *A, int size);

int main(int argc, char **argv) {
    // If vector size is given as a command line argument, use it
    if (argc > 1) {
        SZ = atoi(argv[1]); 
    }

    // Allocate and initialize the host vectors with random values
    init(v1, SZ);
    init(v2, SZ);
    init(v_out, SZ); 

    size_t global[1] = {(size_t)SZ}; // Total number of work-items

    // Show initialized data (partial if large)
    print(v1, SZ);
    print(v2, SZ);
   
    // Initialize OpenCL context, device, command queue, and compile the kernel
    setup_openCL_device_context_queue_kernel((char *)"./vector_ops_ocl.cl", (char *)"vector_add_ocl");
    
    // Allocate device memory and copy data to device
    setup_kernel_memory();
    
    // Set arguments for the kernel function
    copy_kernel_args();

    // Start timing the kernel execution
    auto start = std::chrono::high_resolution_clock::now();

    // Run the kernel function on the device
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    // Copy the result back from device to host
    clEnqueueReadBuffer(queue, bufV_out, CL_TRUE, 0, SZ * sizeof(int), &v_out[0], 0, NULL, NULL);

    // Display result (partial if large)
    print(v_out, SZ);

    // Stop timing
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_time = stop - start;

    printf("Kernel Execution Time: %f ms\n", elapsed_time.count());

    // Clean up allocated resources
    free_memory();
}

// Initialize a vector with random numbers from 0 to 99
void init(int *&A, int size) {
    A = (int *)malloc(sizeof(int) * size);
    for (long i = 0; i < size; i++) {
        A[i] = rand() % 100; 
    }
}

// Display the contents of a vector
void print(int *A, int size) {
    if (PRINT == 0) return;

    if (PRINT == 1 && size > 15) {
        // Show the first 5 and last 5 elements for large vectors
        for (long i = 0; i < 5; i++) printf("%d ", A[i]);
        printf(" ..... ");
        for (long i = size - 5; i < size; i++) printf("%d ", A[i]);
    } else {
        // Show the entire vector if small
        for (long i = 0; i < size; i++) printf("%d ", A[i]);
    }
    printf("\n----------------------------\n");
}

// Release OpenCL and host memory
void free_memory() {
    clReleaseMemObject(bufV1);
    clReleaseMemObject(bufV2);
    clReleaseMemObject(bufV_out);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
    free(v1);
    free(v2);
    free(v_out); 
}

// Provide kernel function with necessary inputs and parameters
void copy_kernel_args() {
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV1);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufV2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufV_out);

    if (err < 0) {
        perror("Couldn't set a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

// Allocate buffers on the device and send input data over
void setup_kernel_memory() {
    bufV1 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV2 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV_out = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);

    clEnqueueWriteBuffer(queue, bufV1, CL_TRUE, 0, SZ * sizeof(int), &v1[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, SZ * sizeof(int), &v2[0], 0, NULL, NULL);
}

// Set up OpenCL context, command queue, and compile the kernel program
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname) {
    device_id = create_device();
    cl_int err;

    // Create an OpenCL context for the selected device
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0) {
        perror("Couldn't create a context");
        exit(1);
    }

    // Load and build the OpenCL program (kernel)
    program = build_program(context, device_id, filename);

    // Create a command queue to communicate with the device
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0) {
        perror("Couldn't create a command queue");
        exit(1);
    }

    // Create a kernel object from the compiled program
    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0) {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    }
}

// Load the kernel code from file and build it for the device
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename) {
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    // Open the kernel source file
    program_handle = fopen(filename, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }

    // Read the entire file into a buffer
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    // Create a program object from source code
    program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer, &program_size, &err);
    if (err < 0) {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    // Build the program for the device
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0) {
        // If there are build errors, print the log
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

// Detect and select an OpenCL device (GPU preferred, fallback to CPU)
cl_device_id create_device() {
    cl_platform_id platform;
    cl_device_id dev;
    int err;

    // Get the OpenCL platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Try selecting a GPU device first
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND) {
        printf("GPU not found\n");
        // Fall back to CPU device if no GPU is found
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0) {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}
