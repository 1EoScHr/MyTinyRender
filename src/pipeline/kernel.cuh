#pragma once

#include "model.cuh"
#include "../basic/cuda_math.cuh"
#include "rasterization.cuh"
#include "../basic/texture.cuh"

__global__
void kernel_raster_triangle(
    const GPURawVertex* verts,   // 3 个
    uchar3* framebuffer,
    float*  zbuffer
);

__global__ void raster_triangle_kernel(
    const GPUVertexOut* verts, // 每 3 个是一组三角形
    int tri_count,
    uchar3* color,
    float* depth
);