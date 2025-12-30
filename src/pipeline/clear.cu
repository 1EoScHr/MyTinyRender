#include <cuda_runtime.h>
#include "clear.cuh"

__global__ void kernel_clear_color(uchar3* framebuffer, int width, int height, uchar3 c)
{
    int x = (int)(blockIdx.x * blockDim.x + threadIdx.x);
    int y = (int)(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= width || y >= height) return;
    framebuffer[y * width + x] = c;
}

__global__ void kernel_clear_depth(float* depth, int width, int height, float v)
{
    int x = (int)(blockIdx.x * blockDim.x + threadIdx.x);
    int y = (int)(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= width || y >= height) return;
    depth[y * width + x] = v;
}

void gpuClearColor(uchar3* framebuffer, int width, int height, uchar3 color)
{
    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x,
              (height + block.y - 1) / block.y);

    kernel_clear_color<<<grid, block>>>(framebuffer, width, height, color);
}

void gpuClearDepth(float* depth, int width, int height, float v)
{
    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x,
              (height + block.y - 1) / block.y);

    kernel_clear_depth<<<grid, block>>>(depth, width, height, v);
}
