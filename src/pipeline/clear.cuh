#pragma once
#include <cuda_runtime.h>

void gpuClearColor(uchar3* framebuffer, int width, int height, uchar3 color);
void gpuClearDepth(float* depth, int width, int height, float v);
