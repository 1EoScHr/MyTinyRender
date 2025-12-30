#include <cuda_runtime.h>

#include "upload.cuh"
#include "rasterization.cuh"        // d_rt + GPURenderTarget
#include "shader/shader.cuh"      // d_shader + GPUShaderConstants
#include "../basic/cuda_math.cuh"       // float4x4u（以及你放的 math）
#include "../basic/homocoor.h"         // mat4, vec3f, vec4
#include "../basic/tgaimage.h"         // TGAColor

// ----------------------------
// CPU mat4(double) -> GPU float4x4u(float)
// 行主序：out.m[r*4+c] = m(r,c)
// ----------------------------
/*
static inline float4x4u toFloat4x4u(const mat4& m)
{
    float4x4u out{};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out.m[r * 4 + c] = static_cast<float>(m(r, c));
    return out;
}
*/

// ----------------------------
// TGAColor(BGRA) -> uchar3(RGB)
// 你 TGAColor 存的是 B,G,R
// ----------------------------
static inline uchar3 toUchar3RGB(const TGAColor& c)
{
    return make_uchar3(c[2], c[1], c[0]);
}

// ----------------------------
// vec3f -> float3
// ----------------------------
static inline float3 toFloat3(const vec3f& v)
{
    return make_float3(v.x, v.y, v.z);
}

// ----------------------------
// 上传渲染目标尺寸到 d_rt
// ----------------------------
void uploadRenderTarget(int width, int height)
{
    GPURenderTarget h_rt{};
    h_rt.width  = width;
    h_rt.height = height;

    cudaMemcpyToSymbol(d_rt, &h_rt, sizeof(GPURenderTarget), 0, cudaMemcpyHostToDevice);
}

// ----------------------------
// 只上传 BPShader_GnmDiffSpec 的常量到 d_shader
// 需要：
// - shader.getGPUConstants() 返回 CPU侧包（MV, light_view, lightColor, I/Ia/ka）
// - model.getGpu*Tex() 返回 texture object
// ----------------------------
void uploadGnmDiffSpecConstants(const BPShader_GnmDiffSpec& shader, const Model& model)
{
    // 这一步需要你在 BPShader_GnmDiffSpec 里实现：
    // BP_Gnm_GPU_CPU BPShader_GnmDiffSpec::getGPUConstants() const;
    BP_Gnm_GPU_CPU cpu = shader.getGPUConstants();

    GPUShaderConstants gpu{};

    // 光照参数
    gpu.lightPos   = toFloat3(cpu.light_view);     // view space
    gpu.lightColor = toUchar3RGB(cpu.lightColor);
    gpu.I  = cpu.I;
    gpu.Ia = cpu.Ia;
    gpu.ka = cpu.ka;

    // 纹理对象（你 Model 里已经 uploadTexture2GPU 创建好了）
    gpu.diffTex = model.getGpuDiffTex();
    gpu.normTex = model.getGpuNormTex();
    gpu.specTex = model.getGpuSpecTex();

    // MV：用于 vnMV（左上角3x3）
    float4x4u mv = toFloat4x4u(cpu.MV);
    memcpy(&gpu.MV, &mv, sizeof(float4x4u));

    cudaMemcpyToSymbol(d_shader, &gpu, sizeof(GPUShaderConstants), 0, cudaMemcpyHostToDevice);
}

