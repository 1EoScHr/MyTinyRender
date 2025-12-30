#pragma once

#include <cuda_runtime.h>
#include "../../basic/cuda_math.cuh"
#include "shader.h"

class BPShader_GnmDiffSpec;

struct GPUShaderConstants {
    uchar3 lightColor;
    uchar3 modelColor;
    float  I;    // 光强
    float  Ia;   // 环境光
    float  ka, kd, ks;
    float3 lightPos;  // 光源位置

    cudaTextureObject_t diffTex;
    cudaTextureObject_t normTex;
    cudaTextureObject_t specTex;

    float4x4u MV;
};

// GPU常量，其实就是shader里面的
extern __constant__ GPUShaderConstants d_shader;

void uploadShaderConstants(const BPShader_GnmDiffSpec& shader);

