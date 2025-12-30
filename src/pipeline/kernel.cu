#include "model.cuh"
#include "../basic/cuda_math.cuh"
#include "rasterization.cuh"
#include "../basic/texture.cuh"

__global__
void kernel_raster_triangle(
    const GPURawVertex* verts,   // 3 个
    uchar3* framebuffer,
    float*  zbuffer
)
{
    // blockIdx.x = triangle id（现在我们先一次只画 1 个三角形）
    // threadIdx.x / y = 像素偏移

    // ---------- 1. 取三角形 ----------
    const GPURawVertex& v0 = verts[0];
    const GPURawVertex& v1 = verts[1];
    const GPURawVertex& v2 = verts[2];

    // ---------- 2. NDC → Screen ----------
    float2 p0 = {
        (v0.clip.x / v0.clip.w * 0.5f + 0.5f) * d_rt.width,
        (v0.clip.y / v0.clip.w * 0.5f + 0.5f) * d_rt.height
    };
    float2 p1 = {
        (v1.clip.x / v1.clip.w * 0.5f + 0.5f) * d_rt.width,
        (v1.clip.y / v1.clip.w * 0.5f + 0.5f) * d_rt.height
    };
    float2 p2 = {
        (v2.clip.x / v2.clip.w * 0.5f + 0.5f) * d_rt.width,
        (v2.clip.y / v2.clip.w * 0.5f + 0.5f) * d_rt.height
    };

    // ---------- 3. BBox ----------
    float minx = fminf(p0.x, fminf(p1.x, p2.x));
    float maxx = fmaxf(p0.x, fmaxf(p1.x, p2.x));
    float miny = fminf(p0.y, fminf(p1.y, p2.y));
    float maxy = fmaxf(p0.y, fmaxf(p1.y, p2.y));

    int x = int(minx) + threadIdx.x;
    int y = int(miny) + threadIdx.y;

    if (x < 0 || x >= d_rt.width ||
        y < 0 || y >= d_rt.height)
        return;

    if (x > maxx || y > maxy) return;

    // ---------- 4. 重心坐标 ----------
    float2 p = { x + 0.5f, y + 0.5f };

    float area = edge(p0, p1, p2);
    if (fabsf(area) < 1e-6f) return;

    float w0 = edge(p1, p2, p) / area;
    float w1 = edge(p2, p0, p) / area;
    float w2 = edge(p0, p1, p) / area;

    if (w0 < 0 || w1 < 0 || w2 < 0) return;

    // ---------- 5. 深度插值 ----------
    float z =
        w0 * (v0.clip.z / v0.clip.w) +
        w1 * (v1.clip.z / v1.clip.w) +
        w2 * (v2.clip.z / v2.clip.w);

    int idx = y * d_rt.width + x;

    // ---------- 6. Z-test ----------
    if (z >= zbuffer[idx]) return;
    zbuffer[idx] = z;

    // ---------- 7. 暂时写死颜色 ----------
    framebuffer[idx] = make_uchar3(255, 255, 255);
}

__global__ void raster_triangle_kernel(
    const GPUVertexOut* verts, // 每 3 个是一组三角形
    int tri_count,
    uchar3* color,
    float* depth)
{
    int tri_id = (int)blockIdx.x;
    if (tri_id >= tri_count) return;

    // 取三角形
    const GPUVertexOut v0 = verts[tri_id * 3 + 0];
    const GPUVertexOut v1 = verts[tri_id * 3 + 1];
    const GPUVertexOut v2 = verts[tri_id * 3 + 2];

    // === NDC -> screen ===
    float invw0 = 1.0f / v0.clip.w;
    float invw1 = 1.0f / v1.clip.w;
    float invw2 = 1.0f / v2.clip.w;

    float2 p0 = make_float2((v0.clip.x * invw0 * 0.5f + 0.5f) * (float)d_rt.width,
                            (v0.clip.y * invw0 * 0.5f + 0.5f) * (float)d_rt.height);
    float2 p1 = make_float2((v1.clip.x * invw1 * 0.5f + 0.5f) * (float)d_rt.width,
                            (v1.clip.y * invw1 * 0.5f + 0.5f) * (float)d_rt.height);
    float2 p2 = make_float2((v2.clip.x * invw2 * 0.5f + 0.5f) * (float)d_rt.width,
                            (v2.clip.y * invw2 * 0.5f + 0.5f) * (float)d_rt.height);

    // === bbox ===
    int minx = max(0, (int)floorf(fminf(p0.x, fminf(p1.x, p2.x))));
    int maxx = min(d_rt.width  - 1, (int)ceilf (fmaxf(p0.x, fmaxf(p1.x, p2.x))));
    int miny = max(0, (int)floorf(fminf(p0.y, fminf(p1.y, p2.y))));
    int maxy = min(d_rt.height - 1, (int)ceilf (fmaxf(p0.y, fmaxf(p1.y, p2.y))));

    float area = edge(p0, p1, p2);
    if (fabsf(area) < 1e-8f) return;

    // 如果你要背面剔除：你 CPU 是 signbit(area) return;
    // 这里按同样逻辑：面积为负就剔除
    if (area < 0.f) return;

    // === block 内像素并行 ===
    for (int y = miny + (int)threadIdx.y; y <= maxy; y += (int)blockDim.y)
    {
        for (int x = minx + (int)threadIdx.x; x <= maxx; x += (int)blockDim.x)
        {
            float2 p = make_float2((float)x + 0.5f, (float)y + 0.5f);

            float w0 = edge(p1, p2, p);
            float w1 = edge(p2, p0, p);
            float w2 = edge(p0, p1, p);

            if (w0 < 0.f || w1 < 0.f || w2 < 0.f) continue;

            w0 /= area;
            w1 /= area;
            w2 /= area;

            // === depth（用 NDC z 插值）===
            float zndc0 = v0.clip.z * invw0;
            float zndc1 = v1.clip.z * invw1;
            float zndc2 = v2.clip.z * invw2;

            float z = zndc0 * w0 + zndc1 * w1 + zndc2 * w2;

            int idx = y * d_rt.width + x;

            // 简单 z-test（非原子，先这样）
            if (z >= depth[idx]) continue;
            depth[idx] = z;

            // =========================
            // 透视修正插值（严格对齐你 CPU：aa=abg/ver.z）
            // 这里 ver[i].z 是 view space z（你的 CPU ver[idx] 是 uintize 后的 view）
            // =========================
            float z0 = v0.view_pos.z;
            float z1 = v1.view_pos.z;
            float z2 = v2.view_pos.z;

            // 防止除 0（极端情况）
            if (fabsf(z0) < 1e-8f || fabsf(z1) < 1e-8f || fabsf(z2) < 1e-8f) continue;

            float aa = w0 / z0;
            float bb = w1 / z1;
            float gg = w2 / z2;
            float invSum = 1.0f / (aa + bb + gg);

            // uv（透视修正）
            float2 uv = make_float2(
                (aa * v0.uv.x + bb * v1.uv.x + gg * v2.uv.x) * invSum,
                (aa * v0.uv.y + bb * v1.uv.y + gg * v2.uv.y) * invSum
            );

            // fragPos(view space)（透视修正，对齐 CPU 的 ver 插值）
            float3 fragPos = make_float3(
                (aa * v0.view_pos.x + bb * v1.view_pos.x + gg * v2.view_pos.x) * invSum,
                (aa * v0.view_pos.y + bb * v1.view_pos.y + gg * v2.view_pos.y) * invSum,
                (aa * v0.view_pos.z + bb * v1.view_pos.z + gg * v2.view_pos.z) * invSum
            );

            // =========================
            // 纹理采样（GnmDiffSpec）
            // =========================
            uchar4 diff = sample_diffuse(d_shader.diffTex, uv);          // uchar4 BGRA
            float3 n_ts = sample_normal(d_shader.normTex, uv);           // [-1,1]（按你 sample_normal）
            float  ks_tex = sample_specular(d_shader.specTex, uv);       // [0,1]

            // baseColor: BGR -> RGB in [0,1]
            float3 baseColor = make_float3(
                (float)diff.z * (1.0f / 255.0f),
                (float)diff.y * (1.0f / 255.0f),
                (float)diff.x * (1.0f / 255.0f)
            );

            // 你的 CPU：normal = vnMV * model.getTexture_nm(uv)
            // 这里先按“全局法线贴图”假设：用 MV 的左上 3x3 把 n_ts 变到 view space
            // 如果你还没写 mul3x3_from4x4，就先直接用 n_ts（先跑通）
            float3 n_view = n_ts;
            // 如果你已有 mul3x3_from4x4：
            // float3 n_view = mul3x3_from4x4(d_shader.MV, n_ts);
            n_view = normalize3(n_view);

            // =========================
            // 光照：严格对齐 CPU
            // toWatch = normalize(-fragPos)
            // toLightVec = lightPos - fragPos
            // squareDist = |toLightVec|^2
            // diffuse = (I/squareDist) * max(0, dot(n, toLight))
            // specular = ks_tex*(I/squareDist)*pow(max(0, dot(halfVec, n)), 100)
            // ambient = ka*Ia
            // final = baseColor*(diffuse+ambient) + lightColor*specular
            // =========================
            float3 toWatch = normalize3(make_float3(-fragPos.x, -fragPos.y, -fragPos.z));

            float3 toLightVec = make_float3(
                d_shader.lightPos.x - fragPos.x,
                d_shader.lightPos.y - fragPos.y,
                d_shader.lightPos.z - fragPos.z
            );

            float squareDist = dot3(toLightVec, toLightVec);
            if (squareDist < 1e-12f) squareDist = 1e-12f; // 防止爆

            float3 toLight = normalize3(toLightVec);

            float invDist = d_shader.I / squareDist;
            float diffuse = invDist * fmaxf(0.f, dot3(n_view, toLight));
            float ambient = d_shader.ka * d_shader.Ia;

            float3 h = halfVec3(toWatch, toLight);
            float specular = ks_tex * invDist * powf(fmaxf(0.f, dot3(h, n_view)), 100.0f);

            float3 lc = make_float3(
                (float)d_shader.lightColor.x * (1.0f / 255.0f),
                (float)d_shader.lightColor.y * (1.0f / 255.0f),
                (float)d_shader.lightColor.z * (1.0f / 255.0f)
            );

            float3 rgb = baseColor * (diffuse + ambient) + lc * specular;

            // clamp & write
            rgb.x = fminf(fmaxf(rgb.x, 0.f), 1.f);
            rgb.y = fminf(fmaxf(rgb.y, 0.f), 1.f);
            rgb.z = fminf(fmaxf(rgb.z, 0.f), 1.f);

            color[idx] = make_uchar3(
                (unsigned char)(rgb.x * 255.0f),
                (unsigned char)(rgb.y * 255.0f),
                (unsigned char)(rgb.z * 255.0f)
            );
        }
    }
}
