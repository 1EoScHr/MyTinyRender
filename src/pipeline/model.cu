// pipeline/model.cu
#include <cuda_runtime.h>
#include <vector>
#include <cstdint>

#include "model.h"
#include "model.cuh"
#include "../basic/tgaimage.h"

// ------------------------
// 创建 GPU texture object（支持：灰度 uchar1 / 彩色 uchar4）
// normalized == true  -> readModeNormalizedFloat（0..1）
// normalized == false -> readModeElementType（原始 uchar/uchar4）
// channel: 1 or 3（你的 tga RGB 是 3）
// ------------------------
void Model::setupGpuTexture(cudaTextureObject_t& texObj,
                            cudaArray_t& texArray,
                            const TGAImage* tex,
                            bool normalized,
                            int channel)
{
    if (!tex) return;

    int w = tex->width();
    int h = tex->height();
    const uint8_t* texBuffer = tex->buffer();

    cudaChannelFormatDesc channelDesc{};
    size_t hostPitch = 0;
    const void* hostPtr = nullptr;

    std::vector<uchar4> bgraBuffer;

    if (channel == 1)
    {
        // 灰度：uchar1
        channelDesc = cudaCreateChannelDesc<uchar1>();
        hostPtr = texBuffer;
        hostPitch = (size_t)w * sizeof(uint8_t);
    }
    else if (channel == 4)
    {
        // 直接当 BGRA 读（你的 TGA 32bit 通常就是 BGRA）
        channelDesc = cudaCreateChannelDesc<uchar4>();
        hostPtr = texBuffer;
        hostPitch = (size_t)w * sizeof(uchar4);
    }
    else
    {
        // 彩色 3 通道：BGR -> uchar4(BGRA)
        channelDesc = cudaCreateChannelDesc<uchar4>();
        bgraBuffer.resize((size_t)w * (size_t)h);

        for (int i = 0; i < w * h; ++i)
        {
            bgraBuffer[i].x = texBuffer[i * 3 + 0]; // B
            bgraBuffer[i].y = texBuffer[i * 3 + 1]; // G
            bgraBuffer[i].z = texBuffer[i * 3 + 2]; // R
            bgraBuffer[i].w = 255;
        }

        hostPtr = bgraBuffer.data();
        hostPitch = (size_t)w * sizeof(uchar4);
    }

    cudaMallocArray(&texArray, &channelDesc, w, h);

    cudaMemcpy2DToArray(texArray, 0, 0,
                        hostPtr, hostPitch,
                        hostPitch, h,
                        cudaMemcpyHostToDevice);

    cudaResourceDesc resDesc{};
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = texArray;

    cudaTextureDesc texDesc{};
    texDesc.addressMode[0] = cudaAddressModeWrap;
    texDesc.addressMode[1] = cudaAddressModeWrap;
    texDesc.normalizedCoords = 1;

    // 关键：ElementType 不能 linear
    texDesc.readMode  = normalized ? cudaReadModeNormalizedFloat
                                   : cudaReadModeElementType;
    texDesc.filterMode = normalized ? cudaFilterModeLinear
                                    : cudaFilterModePoint;

    cudaCreateTextureObject(&texObj, &resDesc, &texDesc, nullptr);
}


// ------------------------
// 你要找回的：uploadTexture2GPU
// ------------------------
void Model::uploadTexture2GPU()
{
    // 防止重复上传造成泄漏（可选，但很实用）
    if (gpuDiffTex) { cudaDestroyTextureObject(gpuDiffTex); gpuDiffTex = 0; }
    if (gpuNormTex) { cudaDestroyTextureObject(gpuNormTex); gpuNormTex = 0; }
    if (gpuSpecTex) { cudaDestroyTextureObject(gpuSpecTex); gpuSpecTex = 0; }

    if (diffArray) { cudaFreeArray(diffArray); diffArray = nullptr; }
    if (normArray) { cudaFreeArray(normArray); normArray = nullptr; }
    if (specArray) { cudaFreeArray(specArray); specArray = nullptr; }

    // 漫反射：RGB -> uchar4，elementType（自己 /255）
    if (diffMap)   setupGpuTexture(gpuDiffTex, diffArray, diffMap.get(),   false, 3);

    // 法线：RGB -> uchar4，但用 normalizedFloat（tex2D<float4> 得到 0..1）
    if (normalMap) setupGpuTexture(gpuNormTex, normArray, normalMap.get(), true,  3);

    // 高光：灰度 uchar1，用 normalizedFloat（tex2D<float> 得到 0..1）
    if (specMap)   setupGpuTexture(gpuSpecTex, specArray, specMap.get(),   true,  1);
}

// ------------------------
// 析构：释放 GPU 资源
// ------------------------
Model::~Model()
{
    if (gpuDiffTex) cudaDestroyTextureObject(gpuDiffTex);
    if (gpuNormTex) cudaDestroyTextureObject(gpuNormTex);
    if (gpuSpecTex) cudaDestroyTextureObject(gpuSpecTex);

    if (diffArray) cudaFreeArray(diffArray);
    if (normArray) cudaFreeArray(normArray);
    if (specArray) cudaFreeArray(specArray);
}
