#pragma once
#include <cuda_runtime.h>
#include "shader/shader.cuh"

struct GPURawVertex {
    float4 clip;      // MVP 后的裁剪空间坐标
    float3 view_pos;  // view space 位置（给光照）
    float3 normal;    // 法线
    float2 uv;        // 纹理坐标
};

struct GPUFramebuffer {
    uchar3* color;  // width * height
    float*  depth;  // z-buffer
    int width;
    int height;
};

struct GPUVertexOut
{
    float4 clip;      // 裁剪空间坐标
    float3 view_pos;  // view space
    float3 normal;    // view space normal
    float2 uv;
};

inline void createTexObj(
    cudaTextureObject_t& texObj,
    cudaArray_t texArray,
    bool normalized,   // readMode: NormalizedFloat or ElementType
    bool linear        // filterMode: Linear or Point
){
    cudaResourceDesc resDesc{};
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = texArray;

    cudaTextureDesc texDesc{};
    texDesc.addressMode[0] = cudaAddressModeWrap;
    texDesc.addressMode[1] = cudaAddressModeWrap;
    texDesc.normalizedCoords = 1;

    texDesc.readMode = normalized ? cudaReadModeNormalizedFloat
                                  : cudaReadModeElementType;

    // 关键：ElementType + Linear 不允许 → 强制 Point
    if (!normalized && linear) linear = false;
    texDesc.filterMode = linear ? cudaFilterModeLinear : cudaFilterModePoint;

    cudaCreateTextureObject(&texObj, &resDesc, &texDesc, nullptr);
}
