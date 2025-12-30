/*
    gpu端cuda需要的向量/矩阵定义/基础运算
*/

#pragma once
#include "homocoor.h"
#include "cuda_runtime.h"
// #include <math_functions.h>

/*
    cpu软光栅采用double/float混用，
    但是gpu硬件上用float更合适
*/
struct float4x4u {
    float m[16];   // 行主序 or 列主序，必须固定
};

__host__ __device__ 
inline float4 mul(const float4x4u& M, const float4& v)
{
    return make_float4(
        M.m[0] * v.x + M.m[1] * v.y + M.m[2]  * v.z + M.m[3]  * v.w,
        M.m[4] * v.x + M.m[5] * v.y + M.m[6]  * v.z + M.m[7]  * v.w,
        M.m[8] * v.x + M.m[9] * v.y + M.m[10] * v.z + M.m[11] * v.w,
        M.m[12]* v.x + M.m[13]* v.y + M.m[14] * v.z + M.m[15] * v.w
    );
}

// cuda类型的内联函数运算
// float3 + float3
__device__ inline float3 operator+(const float3& a, const float3& b)
{
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

// float3 - float3
__device__ inline float3 operator-(const float3& a, const float3& b)
{
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

// float3 * float (标量乘法)
__device__ inline float3 operator*(const float3& v, float f)
{
    return make_float3(v.x * f, v.y * f, v.z * f);
}

// float * float3 (标量乘法）
__device__ inline float3 operator*(float f, const float3& v)
{
    return make_float3(v.x * f, v.y * f, v.z * f);
}

// float3 / float (标量除法)
__device__ inline float3 operator/(const float3& v, float f)
{
    return make_float3(v.x / f, v.y / f, v.z / f);
}

// ---- dot：GPU 版本 ----
__host__ __device__ inline float dot3(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 转换函数
inline uchar3 toUChar3(const TGAColor& c)
{
    return {c[0], c[1], c[2]}; // BGR
}

inline float3 toFloat3(const vec4& v)
{
    return {(float)v.x, (float)v.y, (float)v.z}; // 丢掉 w 分量
}

static inline float4x4u toFloat4x4u(const mat4& m)
{
    float4x4u out{};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out.m[r*4 + c] = static_cast<float>(m(r, c));
    return out;
}

__device__
inline float edge(float2 a, float2 b, float2 c) {
    return (c.x - a.x)*(b.y - a.y) - (c.y - a.y)*(b.x - a.x);
}

// 重心坐标
__device__ inline bool barycentric(
    float2 p,
    float2 a,
    float2 b,
    float2 c,
    float3& abg)
{
    float area = edge(a, b, c);
    if (area == 0) return false;

    float alpha = edge(p, b, c) / area;
    float beta  = edge(p, c, a) / area;
    float gamma = edge(p, a, b) / area;

    if (alpha < 0 || beta < 0 || gamma < 0)
        return false;

    abg = make_float3(alpha, beta, gamma);
    return true;
}

// 类型转换
/*
inline vec3f to_vec3f(const float3& f)
{
    return vec3f(f.x, f.y, f.z);
}
*/

inline float3 to_float3(const vec3f& v)
{
    return make_float3(v.x, v.y, v.z);
}

__device__ inline float3 normalize3(float3 v)
{
    float len2 = dot3(v, v);
    if (len2 > 1e-16f)
    {
        float inv = 1.0f / sqrtf(len2);
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
    }
    else
    {
        v.x = v.y = v.z = 0.f;
    }
    return v;
}

__device__ inline float3 halfVec3(float3 a, float3 b)
{
    float3 s = make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
    return normalize3(s);
}

__host__ __device__ inline float3 mul3x3_from4x4(const float4x4u& M, float3 v)
{
    // row-major:
    // [ m0 m1 m2 m3 ]
    // [ m4 m5 m6 m7 ]
    // [ m8 m9 m10 m11]
    // [ ...         ]
    return make_float3(
        M.m[0] * v.x + M.m[1] * v.y + M.m[2]  * v.z,
        M.m[4] * v.x + M.m[5] * v.y + M.m[6]  * v.z,
        M.m[8] * v.x + M.m[9] * v.y + M.m[10] * v.z
    );
}
