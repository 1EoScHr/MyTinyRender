#include "cuda_runtime.h"

#include "model.h"
#include "rasterization.cuh"
#include "shader/shader.cuh"
#include "../basic/cuda_math.cuh"
#include "model.cuh"
#include "upload.cuh"
#include "kernel.cuh"

__constant__ GPURenderTarget d_rt;

static uchar3* d_color = nullptr;
static float*  d_depth = nullptr;
static GPUVertexOut* d_verts = nullptr;
static int g_max_tris = 0;

static inline void cudaCheck(cudaError_t e, const char* msg)
{
    if (e != cudaSuccess) {
        fprintf(stderr, "CUDA error %s: %s\n", msg, cudaGetErrorString(e));
        std::exit(1);
    }
}

// 简单 clear kernel
__global__ void clear_color_depth_kernel(uchar3* color, float* depth, int w, int h, uchar3 bg)
{
    int idx = (int)(blockIdx.x * blockDim.x + threadIdx.x);
    int n = w * h;
    if (idx >= n) return;
    color[idx] = bg;
    depth[idx] = 1e30f; // “远”
}

void gpuInit(int width, int height, const TGAColor& bg, Model& model)
{
    // 1) render target 常量
    uploadRenderTarget(width, height);

    // 2) model 纹理上 GPU（你已经写了）
    model.uploadTexture2GPU();

    // 3) framebuffer
    cudaCheck(cudaMalloc(&d_color, width * height * sizeof(uchar3)), "malloc d_color");
    cudaCheck(cudaMalloc(&d_depth, width * height * sizeof(float)),  "malloc d_depth");

    // 4) verts buffer：先给一个上限（后面每帧按需要 realloc）
    g_max_tris = 200000; // 先拍个大数，后面你可以按 model.face.size()
    cudaCheck(cudaMalloc(&d_verts, g_max_tris * 3 * sizeof(GPUVertexOut)), "malloc d_verts");
}

void gpuShutdown()
{
    if (d_verts) cudaFree(d_verts);
    if (d_color) cudaFree(d_color);
    if (d_depth) cudaFree(d_depth);
    d_verts = nullptr;
    d_color = nullptr;
    d_depth = nullptr;
    g_max_tris = 0;
}

// 把 TGAColor(BGR) 转 uchar3(BGR)
static inline uchar3 toU3(const TGAColor& c)
{
    return make_uchar3(c[0], c[1], c[2]);
}

void gpuRenderFrame(Model& model, Camera& camera, BPShader_GnmDiffSpec& shader,
                    TGAImage& framebuffer, const TGAColor& bg)
{
    const int width  = framebuffer.width();
    const int height = framebuffer.height();

    // === 1) 每帧 shader 的 MVP/常量上传 ===
    // 这一步严格沿你 CPU 流程：model/camera dirty 的逻辑仍在 realtime 里做
    shader.getMVP(model.getModelMat(), camera.getViewMat(), camera.getProjMat());
    uploadGnmDiffSpecConstants(shader, model); // 内部 cudaMemcpyToSymbol(d_shader,...)

    // === 2) clear GPU framebuffer ===
    {
        int n = width * height;
        int threads = 256;
        int blocks = (n + threads - 1) / threads;

        // TGAColor 是 BGR
        uchar3 bg3 = make_uchar3(bg[0], bg[1], bg[2]);

        clear_color_depth_kernel<<<blocks, threads>>>(
            d_color, d_depth, width, height, bg3
        );
        cudaCheck(cudaGetLastError(), "clear kernel launch");
    }


    // === 3) CPU 组装每个三角形的 GPUVertexOut（最小版本）===
    // 你之后可以改成 GPU vertex kernel + index buffer
    const auto& faces = model.getFace();
    int tri_count = (int)faces.size();

    if (tri_count > g_max_tris) {
        cudaFree(d_verts);
        g_max_tris = tri_count;
        cudaCheck(cudaMalloc(&d_verts, g_max_tris * 3 * sizeof(GPUVertexOut)), "realloc d_verts");
    }

    std::vector<GPUVertexOut> h_verts(tri_count * 3);

    for (int t = 0; t < tri_count; ++t) {
        const face_obj& f = faces[t];

        for (int k = 0; k < 3; ++k) {
            int vid = std::get<0>(f[k]);
            int tid = std::get<1>(f[k]);
            int nid = std::get<2>(f[k]);

            // 位置：走你 shader.vertex 得到 clip（你 CPU shader 已经能算）
            // 但 shader.vertex 会写内部数组 ver/ver_t 等，不适合并行。
            // 最小版本：直接用 CPU 算三顶点并填入（能跑）
            vec4 clip = shader.vertex(model, f, k);

            // view pos / uv / normal：你 CPU shader 内部就有 ver/ver_t/vnMV 等
            // 这里先从 shader 的缓存里取（你得提供 getter，或者直接重算）
            // —— 为了最小改动，建议你在 BPShader_GnmDiffSpec 增加只读 getter：
            // getViewPos(k), getUV(k), getVN_MV() 之类
            vec3f vp = shader.getViewPos(k);
            vec2_f uv = shader.getUV(k);

            // 这里 normal map 在 fragment 采样，所以 per-vertex normal 其实可不填
            // 但你的 raster kernel 里用了 v.normal 插值做了个 n（我们已经删掉那段极简了）
            // 因此这里可以填 0，后面完全用 normal map
            GPUVertexOut out{};
            out.clip = make_float4((float)clip.x, (float)clip.y, (float)clip.z, (float)clip.w);
            out.view_pos = make_float3(vp.x, vp.y, vp.z);
            out.uv = make_float2(uv.u, uv.v);
            out.normal = make_float3(0.f, 0.f, 1.f);

            h_verts[t*3 + k] = out;
        }
    }

    cudaCheck(cudaMemcpy(d_verts, h_verts.data(), tri_count * 3 * sizeof(GPUVertexOut), cudaMemcpyHostToDevice),
              "memcpy verts H2D");

    // === 4) raster kernel ===
    {
        dim3 block(16, 16);
        dim3 grid(tri_count);
        raster_triangle_kernel<<<grid, block>>>(d_verts, tri_count, d_color, d_depth);
        cudaCheck(cudaGetLastError(), "raster_triangle_kernel launch");
    }

    // === 5) 拷回 CPU framebuffer（uchar3 BGR -> 你的 framebuffer buffer）===
    // framebuffer.buffer() 是 uint8_t*，每像素 3字节 BGR
    std::vector<uchar3> h_color(width * height);
    cudaCheck(cudaMemcpy(h_color.data(), d_color, width * height * sizeof(uchar3), cudaMemcpyDeviceToHost),
              "memcpy color D2H");

    uint8_t* dst = framebuffer.buffer();
    for (int i = 0; i < width * height; ++i) {
        dst[i*3 + 0] = h_color[i].x; // B
        dst[i*3 + 1] = h_color[i].y; // G
        dst[i*3 + 2] = h_color[i].z; // R
    }

    cudaDeviceSynchronize();
}


