// OpenCL kernel for performing element-wise addition of two integer vectors
__kernel void vector_add_ocl(const int size, 
                             __global int *v1, 
                             __global int *v2, 
                             __global int *v_out) {
    
    // Calculate the unique global index for this work-item
    const int globalIndex = get_global_id(0);

    // Check if the computed index lies within the valid range of the input arrays
    if (globalIndex < size) {
        
        // Add the corresponding elements from both input vectors
        v_out[globalIndex] = v1[globalIndex] + v2[globalIndex];
        
        // Store the result in the output vector at the same index position
    }

    // Work-items with indices beyond 'size' are safely ignored
}
