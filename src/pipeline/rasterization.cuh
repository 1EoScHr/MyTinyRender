#pragma once

#include "cuda_runtime.h"

#include "model.h"
#include "shader/shader.cuh"
#include "../basic/cuda_math.cuh"
#include "model.cuh"
#include "upload.cuh"
#include "kernel.cuh"

struct GPURenderTarget
{
    int width;
    int height;
};

extern __constant__ GPURenderTarget d_rt;

void gpuInit(int width, int height, const TGAColor& bg, Model& model);
void gpuShutdown();

void gpuRenderFrame(Model& model, Camera& camera, BPShader_GnmDiffSpec& shader,
                    TGAImage& framebuffer, const TGAColor& bg);
