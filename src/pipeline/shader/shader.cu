#include "shader.h"
#include "shader.cuh"
#include <cuda_runtime.h>

// 把shader的常量上传到显存
void uploadShaderConstants(const BPShader_GnmDiffSpec& shader)
{
    GPUShaderConstants cpu{};

    cpu.lightColor = make_uchar3(255, 255, 255);
    cpu.modelColor = make_uchar3(0, 0, 255);
    cpu.I  = 4.f;
    cpu.Ia = 0.1f;
    cpu.ka = cpu.kd = cpu.ks = 1.f;
    cpu.lightPos = make_float3(0, 2, 2);

    cudaMemcpyToSymbol(
        ::d_shader,
        &cpu,
        sizeof(GPUShaderConstants),
        0,
        cudaMemcpyHostToDevice
    );
}

__constant__ GPUShaderConstants d_shader;