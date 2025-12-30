#pragma once

#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif

#ifdef __CUDACC__
/*
 * 约定（非常重要）：
 * 1. 所有纹理都使用 normalizedCoords = true
 *    → uv ∈ [0,1]
 * 2. addressMode = wrap
 * 3. filterMode  = linear
 *
 * diff  : uchar4  → 手动 /255
 * normal: float4  → readModeNormalizedFloat
 * spec  : float   → readModeNormalizedFloat
 */

// =======================
// 漫反射贴图：返回 uchar4 (BGRA)
// =======================
__device__ __forceinline__ uchar4
sample_diffuse(cudaTextureObject_t tex, float2 uv)
{
    // tex2D<uchar4>：要求 readMode = cudaReadModeElementType
    return tex2D<uchar4>(tex, uv.x, uv.y);
}

// =======================
// 法线贴图：返回 float3 ∈ [-1,1]
// =======================
__device__ __forceinline__  float3
sample_normal(cudaTextureObject_t tex, float2 uv)
{
    // tex2D<float4>：要求 readMode = cudaReadModeNormalizedFloat
    float4 n = tex2D<float4>(tex, uv.x, uv.y);

    // 你 CPU 里的逻辑是：
    // (R,G,B) ∈ [0,1] → [-1,1]
    // 并且你用的是 BGR 顺序
    return make_float3(
        n.z * 2.0f - 1.0f,   // R → x
        n.y * 2.0f - 1.0f,   // G → y
        n.x * 2.0f - 1.0f    // B → z
    );
}

// =======================
// 高光贴图：返回 float ∈ [0,1]
// =======================
__device__ __forceinline__ float
sample_specular(cudaTextureObject_t tex, float2 uv)
{
    // tex2D<float>：单通道 normalized
    return tex2D<float>(tex, uv.x, uv.y);
}
#endif // __CUDACC__