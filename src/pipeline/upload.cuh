#pragma once

#include "model.h"
#include "shader/shader.h"   // BPShader_GnmDiffSpec 声明

// 上传屏幕宽高到 constant memory（d_rt）
void uploadRenderTarget(int width, int height);

// 只上传 BPShader_GnmDiffSpec 所需的 shader 常量到 constant memory（d_shader）
void uploadGnmDiffSpecConstants(const BPShader_GnmDiffSpec& shader, const Model& model);

// cpu发到gpu
inline float4x4u
to_float4x4u(const mat4& m)
{
    float4x4u out;
    for (int i = 0; i < 16; ++i)
    {
        out.m[i] = static_cast<float>(m.data[i]);
    }
    return out;
}