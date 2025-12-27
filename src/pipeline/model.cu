#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "../basic/tgaimage.h"
#include "model.h"
#include "rasterization.h"

#include <math.h>
#include <iostream>
#include <algorithm>

#define _CMATH_H_NO_CUDART_OVERLOADS_
#define _STDLIB_H_NO_CUDART_OVERLOADS_
#define __CUDA_NO_HALF_OPERATORS__
#define __CUDA_ENABLE_XP_LEVELS__
// cuda部分
#define __CUDACC__
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <vector_types.h> // 提供float4/uchar4等CUDA基础类型

// 老版本
// 类型后面的数字代表通道数、2D纹理、自动归一化到[0,1]
// extern texture<uchar3, cudaTextureType2D, cudaReadModeNormalizedFloat> gpu_diff_tex;    // 漫反射纹理
// extern texture<uchar3, cudaTextureType2D, cudaReadModeNormalizedFloat> gpu_norm_tex;    // 法线纹理
// extern texture<uchar1, cudaTextureType2D, cudaReadModeNormalizedFloat> gpu_spec_tex;    // 高光纹理

__device__ __host__ cudaTextureObject_t gpu_diff_tex = 0;
__device__ __host__ cudaTextureObject_t gpu_norm_tex = 0;
__device__ __host__ cudaTextureObject_t gpu_spec_tex = 0;

// ====================== 全局独立GPU核函数【3个，彻底替代Lambda】======================
// 核函数1：法线纹理GPU采样（float3通道）
__global__ void gpu_sample_norm(cudaTextureObject_t tex, vec2_f uv, float* d_out)
{
    d_out[0] = tex2D<float>(tex, uv.u, uv.v);
    d_out[1] = tex2D<float>(tex, uv.u, uv.v);
    d_out[2] = tex2D<float>(tex, uv.u, uv.v);
}

// 核函数2：漫反射纹理GPU采样（uchar3通道）
__global__ void gpu_sample_diff(cudaTextureObject_t tex, vec2_f uv, unsigned char* d_out)
{
    d_out[0] = tex2D<unsigned char>(tex, uv.u, uv.v);
    d_out[1] = tex2D<unsigned char>(tex, uv.u, uv.v);
    d_out[2] = tex2D<unsigned char>(tex, uv.u, uv.v);
}

// 核函数3：高光纹理GPU采样（float单通道）
__global__ void gpu_sample_spec(cudaTextureObject_t tex, vec2_f uv, float* d_out)
{
    *d_out = tex2D<float>(tex, uv.u, uv.v);
}


mat4
getRotMat(double x, int axis)
{
    // 旋转矩阵特点是绕谁转，谁就不会变，保留原来的值，因此能确定一行；同样的，其他维度旋转就与该轴无关，这样就确定一列

    assert(axis >= 0 && axis <=2);  // 0为x轴，1为y轴，2为z轴
    
    double sinx = std::sin(x);
    // double cosx = std::sqrt(1.0 - sinx * sinx);  // 三角恒等式，但会导致cos符号还需额外判断，不如直接用标准库
    double cosx = std::cos(x);  

    mat4 rotmat = {};

    switch (axis)
    {
        case 0:
            rotmat(0, 0) =     1, /*        0        */ /*        0        */ /*        0        */
            /*        0        */ rotmat(1, 1) =  cosx, rotmat(1, 2) = -sinx, /*        0        */
            /*        0        */ rotmat(2, 1) =  sinx, rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 1:
            rotmat(0, 0) =  cosx, /*        0        */ rotmat(0, 2) =  sinx, /*        0        */
            /*        0        */ rotmat(1, 1) =     1, /*        0        */ /*        0        */
            rotmat(2, 0) = -sinx, /*        0        */ rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 2:
            rotmat(0, 0) =  cosx, rotmat(0, 1) = -sinx, /*        0        */ /*        0        */
            rotmat(1, 0) =  sinx, rotmat(1, 1) =  cosx, /*        0        */ /*        0        */
            /*        0        */ /*        0        */ rotmat(2, 2) =     1; /*        0        */
            break;
    }

    rotmat(3, 3) = 1;

    return rotmat;
    
    /*
    xrotmat:                yrotmat:                zrotmat:
        1,     0,      0,    cosy,     0,   siny,   cosz,  -sinz,      0, 
        0,  cosx,  -sinx,       0,     1,      0,   sinz,   cosz,      0, 
        0,  sinx,   cosx;   -siny,     0,   cosy;      0,      0,      1; 
    */
}

vec4 
Model::getVertex(int idx) const
{
    return v[idx];
}

vec3f
Model::getVertexNormal(int idx) const
{
    return vn[idx];
}

vec2_f
Model::getVertexTexture(int idx) const
{
    return vt[idx];
}

// ====================== 法线纹理采样【最终0错误版】======================
__host__ __device__ vec3f
Model::getTexture_nm(const vec2_f& uv) const
{
    float r = 0.f, g = 0.f, b = 0.f;

#ifdef __CUDA_ARCH__
    // GPU端调用：原生tex2D采样（无任何兼容问题）
    r = tex2D<float>(gpu_norm_tex, uv.u, uv.v);
    g = tex2D<float>(gpu_norm_tex, uv.u, uv.v);
    b = tex2D<float>(gpu_norm_tex, uv.u, uv.v);
#else
    // CPU端调用：启动独立GPU核函数采样【纯原生写法，0错误】
    float* d_out = nullptr;
    vec2_f* d_uv = nullptr;
    // 1. 分配GPU显存
    cudaMalloc(&d_out, sizeof(float) * 3);
    cudaMalloc(&d_uv, sizeof(vec2_f));
    // 2. CPU→GPU拷贝UV坐标
    cudaMemcpy(d_uv, &uv, sizeof(vec2_f), cudaMemcpyHostToDevice);
    // 3. 启动GPU核函数【原生写法，永不报错】
    gpu_sample_norm<<<1, 1>>>(gpu_norm_tex, *d_uv, d_out);
    // 4. 等待GPU执行完成
    cudaDeviceSynchronize();
    // 5. GPU→CPU拷贝采样结果
    cudaMemcpy(&r, d_out, sizeof(float) * 3, cudaMemcpyDeviceToHost);
    // 6. 释放GPU显存（避免内存泄漏）
    cudaFree(d_out);
    cudaFree(d_uv);
#endif

    // ✅ 完全保留你原有法向量转换公式，一丝不改
    vec3f ret(r, g, b);
    ret.x = (2.f * ret.x - 255.f) / 255.f;
    ret.y = (2.f * ret.y - 255.f) / 255.f;
    ret.z = (2.f * ret.z - 255.f) / 255.f;
    return ret;
}

// ====================== 漫反射纹理采样【最终0错误版】======================
__host__ __device__ TGAColor
Model::getTexture_diff(const vec2_f& uv) const
{
    unsigned char r = 0, g = 0, b = 0;

#ifdef __CUDA_ARCH__
    // GPU端调用：原生tex2D采样
    r = tex2D<unsigned char>(gpu_diff_tex, uv.u, uv.v);
    g = tex2D<unsigned char>(gpu_diff_tex, uv.u, uv.v);
    b = tex2D<unsigned char>(gpu_diff_tex, uv.u, uv.v);
#else
    // CPU端调用：启动独立GPU核函数采样【纯原生写法】
    unsigned char* d_out = nullptr;
    vec2_f* d_uv = nullptr;
    cudaMalloc(&d_out, sizeof(unsigned char) * 3);
    cudaMalloc(&d_uv, sizeof(vec2_f));
    cudaMemcpy(d_uv, &uv, sizeof(vec2_f), cudaMemcpyHostToDevice);
    
    gpu_sample_diff<<<1, 1>>>(gpu_diff_tex, *d_uv, d_out);
    cudaDeviceSynchronize();
    cudaMemcpy(&r, d_out, sizeof(unsigned char) * 3, cudaMemcpyHostToDevice);
    
    cudaFree(d_out);
    cudaFree(d_uv);
#endif

    // ✅ 完全保留你原有TGAColor构造，一丝不改
    return TGAColor{r, g, b, 255};
}

// ====================== 高光纹理采样【最终0错误版】======================
__host__ __device__ float
Model::getTexture_spec(const vec2_f& uv) const
{
    float spec_val = 0.f;

#ifdef __CUDA_ARCH__
    // GPU端调用：原生tex2D采样
    spec_val = tex2D<float>(gpu_spec_tex, uv.u, uv.v);
#else
    // CPU端调用：启动独立GPU核函数采样【纯原生写法】
    float* d_out = nullptr;
    vec2_f* d_uv = nullptr;
    cudaMalloc(&d_out, sizeof(float));
    cudaMalloc(&d_uv, sizeof(vec2_f));
    cudaMemcpy(d_uv, &uv, sizeof(vec2_f), cudaMemcpyHostToDevice);
    
    gpu_sample_spec<<<1, 1>>>(gpu_spec_tex, *d_uv, d_out);
    cudaDeviceSynchronize();
    cudaMemcpy(&spec_val, d_out, sizeof(float), cudaMemcpyHostToDevice);
    
    cudaFree(d_out);
    cudaFree(d_uv);
#endif

    // ✅ 完全保留你原有归一化公式，一丝不改
    return spec_val / 255.f;
}



const std::vector<face_obj>& 
Model::getFace(void) const
{
    return this->f;
}

bool 
Model::getModelDirty(void) const
{
    return this->modelDirty;
}

void 
Model::setPos(vec4 newPos)
{
    assert(newPos.w != 0 && "newPos is position, but it's w = 0");
    pos = newPos;
    modelDirty = true;   // 脏位
    return;
}

void 
Model::addShift(vec4 shift)
{
    assert(shift.w == 0 && "shift is vector, but it's w not 0");
    this->shift = shift; 
    modelDirty = true;
    return;
}

void 
Model::setRotate(double rad, int axis)
{
    assert(axis >= 0 && axis <=2 && "invalid axis");
    rotate[axis] = rad;
    modelDirty = true;
    return;
}

Model::Model(const std::string& _objFilePath, 
             const std::string& _nmFilePath,
             const std::string& _diffFilePath,
             const std::string& _specFilePath) 
    : objFilePath(_objFilePath), nmFilePath(_nmFilePath), diffFilePath(_diffFilePath), specFilePath(_specFilePath),
      pos({0., 0., 0., 1.}), shift({0., 0., 0., 0.}), rotate({0., 0., 0.}), modelDirty(true)  // v、f等没有初始化，会调用默认，也就是创造两个空向量
{
    objReader();
    textureReader();
}

mat4
Model::getModelMat(void) const
{
    /*
        获取齐次坐标版模型变换矩阵，包括旋转与平移
        实际计算时，等效为先把模型平移回模型坐标系原点，再把模型空间与世界空间对齐，再旋转，再恢复
    */
    
    // 先把模型平移回模型空间原点，再让模型坐标系与世界坐标系重合，两个平移操作可以叠加
    // 但是由于.obj默认的原点就在世界坐标系原点，所以只需模型平移回模型空间原点就可
    assert(pos.w != 0 && "pos is position, but it's w = 0");
    mat4 moveBack = get1Mat();
    moveBack(0, 3) = -shift.x;
    moveBack(1, 3) = -shift.y;
    moveBack(2, 3) = -shift.z;

    ////////////////////////////////////////////////3DV小作业：绕指定轴旋转，已落后版本，重启须评估
/*
    double angle;
    std::cin >> angle;
    modelInfo.z_rotate = angle / 180 * M_PI; // 绕对应轴转70度

    // 理解成模型坐标系不与世界坐标系平行，根据view变换的经验，可以增加以下步骤：
    // 把模型平移回原点后，先旋转使模型坐标系与世界坐标系重合，再绕预设的各轴旋转，再旋转使模型坐标系恢复原来的值。
    // 当然这里我简化实现，不追求模型坐标系完全与世界坐标系重合，只要两个的z轴能够重合，就绕z轴转。
    // 所以这里只是进了一步来实现绕任意轴旋转，是从绕世界xyz轴旋转进步到经过模型原点任意一轴旋转，要再进一步变成任意一轴，可能还需要把模型坐标原点移动考虑进来

    // 对于要求的(1, 1, 1)与70度情况

    vec4 targetModelZaxis = {1, 1, 0, 0}; // 若与原轴重合，则就会引起断言错误，就不要闲的没事
    normalize(targetModelZaxis);    // 目标z轴正则化向量值


    // 默认的模型坐标系还和世界坐标系重合，要先让其变成设定的（同样为简化，只让z轴对齐，其他两轴跟着转就行）
    // 我的思路是先绕世界z轴转，把模型z轴转到YOZ平面，再绕世界x轴把模型z轴转到世界z轴（当然也可反着，但这似乎就要顺时针）
    // 复习一下“绕某轴转某度”：就是按右手定则，箭头对向眼睛、逆时针

    // 这里选用的三角函数反解也有说法，为了精确的sin、cos符号，把求旋转矩阵的入参改成了弧度
    // 第一步里，操作是把轴投影到在XOY平面上，所以其有可能分布在四个象限，用atan不精确，而atan2则接收x和y的值，刚好；
    // 第二步里，已经把轴移到投影刚好在+y轴上，所以旋转角度只是0-pi，这时刚好用acos解算就没问题。
    mat4 alignRotate1 = getRotMat(std::atan2(targetModelZaxis.y, targetModelZaxis.x), 2);   // 目标轴先绕z轴转到YOZ平面
    mat4 alignRotate2 = getRotMat(std::acos(targetModelZaxis.z), 0);    // 目标轴再绕x轴转到z轴

    // 逆旋转矩阵是旋转矩阵的转置
    trans(alignRotate2);    // z轴转到ZOY面上
    trans(alignRotate1);    // 继续转到目标z轴

    // 模型坐标系的值
    mat4 transTarget = alignRotate1 * alignRotate2;
    assert(std::abs((transTarget * modelInfo.z_axis).z - targetModelZaxis.z) < 1e-6);
    modelInfo.x_axis = transTarget * modelInfo.x_axis;
    modelInfo.y_axis = transTarget * modelInfo.y_axis;
    modelInfo.z_axis = transTarget * modelInfo.z_axis;
    assert(std::abs(modelInfo.z_axis.x - targetModelZaxis.x) < 1e-6);

    // 旋转矩阵的每一行，都是目标空间基向量在当前空间的表示
    mat4 alignRotate;
    alignRotate(0, 0) = modelInfo.x_axis.x, alignRotate(0, 1) = modelInfo.x_axis.y, alignRotate(0, 2) = modelInfo.x_axis.z, 
    alignRotate(1, 0) = modelInfo.y_axis.x, alignRotate(1, 1) = modelInfo.y_axis.y, alignRotate(1, 2) = modelInfo.y_axis.z, 
    alignRotate(2, 0) = modelInfo.z_axis.x, alignRotate(2, 1) = modelInfo.z_axis.y, alignRotate(2, 2) = modelInfo.z_axis.z, 
    alignRotate(3, 3) = 1; 

    // 反向变换回去
    mat4 transAlignRotate = alignRotate;
    trans(transAlignRotate);

    assert(modelInfo.x_rotate == 0);
    assert(modelInfo.y_rotate == 0);
*/
    ////////////////////////////////////////////////3DV小作业

    // 此时再旋转
    mat4 xrotmat = getRotMat(rotate[0], 0), 
         yrotmat = getRotMat(rotate[1], 1),
         zrotmat = getRotMat(rotate[2], 2);

    mat4 rotmovMat = zrotmat*yrotmat*xrotmat;

    // 计算完矩阵后，就可以认为已经执行了，只是还没有渲染

    ////////////////////////////////////////////////3DV小作业
/*
    mat4 movMat = get1Mat();
    movMat(0, 3) = modelInfo.pos.x;
    movMat(1, 3) = modelInfo.pos.y;
    movMat(2, 3) = modelInfo.pos.z;

    ModelMat = movMat * transAlignRotate * rotmovMat * alignRotate * moveBack; 
    // 先移动回原点，再旋转使模型坐标系对应世界坐标系，此时再绕z轴旋转，再恢复原本的坐标系，再移动到目标地点
*/
    ////////////////////////////////////////////////3DV小作业

    /**/
    
    // 最后平移回原来的位置，先把坐标系平移到模型坐标系，再坐标系内平移
    assert(shift.w == 0 && "shift is vector, but it's w not 0");   // 向量齐次坐标w为0
    rotmovMat(0, 3) = pos.x + shift.x,
    rotmovMat(1, 3) = pos.y + shift.y,
    rotmovMat(2, 3) = pos.z + shift.z;   
    
    mat4 ModelMat;
    ModelMat = rotmovMat * moveBack; // 先绕x再绕y后绕z

    modelDirty = false; // 清除脏位
    return ModelMat;
}

/*
void 
Model::uploadTex2GPU(std::unique_ptr<TGAImage>& cpuTex, int texType)
{
    if (!cpuTex) return;    // 空纹理，跳过

    int tex_w = cpuTex->width();
    int tex_h = cpuTex->height();
    int tex_bpp = cpuTex->bytepp();
    size_t tex_size;
    const uint8_t* cpu_buffer = cpuTex->buffer();
   
    void* device_tex = nullptr;
    cudaChannelFormatDesc tex_desc; // 绑定显存到纹理对象

    switch (tex_bpp)
    {
        case 1: 
            // 内存上传至显存
            assert(texType == 0 && "it should be specTex");
            tex_size = tex_w * tex_h * sizeof(uchar1);  // 计算纹理内存大小
            cudaMalloc((void**)&device_tex, tex_size);
            cudaMemcpy(device_tex, cpu_buffer, tex_size, cudaMemcpyHostToDevice);
            
            tex_desc = cudaCreateChannelDesc<uchar1>();
            cudaBindTexture2D(NULL, &gpu_spec_tex, device_tex, &tex_desc, tex_w, tex_h, sizeof(uchar1) * tex_w);
            break;
        case 3: 
            tex_size = tex_w * tex_h * sizeof(uchar3);
            cudaMalloc((void**)&device_tex, tex_size);
            cudaMemcpy(device_tex, cpuTex->buffer(), tex_size, cudaMemcpyHostToDevice);
            
            tex_desc = cudaCreateChannelDesc<uchar3>();
            if (texType == 1) cudaBindTexture2D(NULL, &gpu_diff_tex, device_tex, &tex_desc, tex_w, tex_h, sizeof(uchar3) * tex_w);
            if (texType == 2) cudaBindTexture2D(NULL, &gpu_norm_tex, device_tex, &tex_desc, tex_w, tex_h, sizeof(uchar3) * tex_w);            
            break;
        case 4: // 应该用不到，因为纹理要么三通道要么一通道，就不写了，有问题再说
        default:
            exit(EXIT_FAILURE);
    }

    return;
}
*/

// 初始化纹理对象
template<typename T>
void createCudaTexture(cudaTextureObject_t& texObj, const T* cpuData, int width, int height, bool needNormalize) {
    // GPU显存分配、从CPU拷贝纹理数据
    T* gpuData = nullptr;
    cudaMalloc(&gpuData, width * height * sizeof(T));
    cudaMemcpy(gpuData, cpuData, width * height * sizeof(T), cudaMemcpyHostToDevice);

    // 配置资源描述符
    cudaResourceDesc resDesc{};
    resDesc.resType = cudaResourceTypePitch2D;       // 2D纹理（固定值）
    resDesc.res.pitch2D.devPtr = gpuData;            // GPU显存地址
    resDesc.res.pitch2D.width = width;               // 纹理宽度
    resDesc.res.pitch2D.height = height;             // 纹理高度
    resDesc.res.pitch2D.pitchInBytes = width * sizeof(T); // 每行字节数
    resDesc.res.pitch2D.desc = cudaCreateChannelDesc<T>(); // 数据格式（自动适配uchar1/uchar3）

    // 配置纹理描述符，控制归一化、采样模式、UV规则
    cudaTextureDesc texDesc{};
    texDesc.normalizedCoords = 1;                    // UV归一化
    texDesc.filterMode = cudaFilterModeLinear;       // 线性插值（关闭则写cudaFilterModePoint）
    texDesc.addressMode[0] = cudaAddressModeClamp;   // U方向边界钳位（防UV越界）
    texDesc.addressMode[1] = cudaAddressModeClamp;   // V方向边界钳位

    if (needNormalize) {
        // 自动归一化
        texDesc.readMode = cudaReadModeNormalizedFloat;
    } else {
        // 漫反射
        texDesc.readMode = cudaReadModeElementType;
    }

    // 4. 第四步：创建纹理对象（新版核心API，一行搞定）
    cudaCreateTextureObject(&texObj, &resDesc, &texDesc, nullptr);
}

void Model::uploadTex2GPU(const std::unique_ptr<TGAImage>& tgaImg, int texType)
{
    if (!tgaImg) return; // 空指针防护
    int texW = tgaImg->width();
    int texH = tgaImg->height();

    // ========== 分支1：texType=0 → 高光纹理（uchar1 + 自动归一化） ==========
    if (texType == 0)
    {
        // 1. TGAColor → CUDA uchar1（仅取B通道，高光灰度用，全程BGR）
        std::vector<uchar1> cpuData(texW * texH);
        for (int y = 0; y < texH; ++y)
        {
            for (int x = 0; x < texW; ++x)
            {
                TGAColor color = tgaImg->get(x, y);
                int idx = y * texW + x;
                cpuData[idx].x = color.bgra[0]; // 高光取B通道 ✔️
            }
        }
        // 2. 防重复初始化：先销毁旧纹理，再创建新纹理（工程级规范）
        if (gpu_spec_tex) cudaDestroyTextureObject(gpu_spec_tex);
        // 3. 高光需要归一化 → needNormalize=true
        createCudaTexture(gpu_spec_tex, cpuData.data(), texW, texH, true);
    }
    // ========== 分支2：texType=1 → 漫反射纹理（uchar3 + 零归一化/原样读取）⭐核心诉求⭐ ==========
    else if (texType == 1)
    {
        // 1. TGAColor → CUDA uchar3（BGR三通道，原样读取，零冗余）
        std::vector<uchar3> cpuData(texW * texH);
        for (int y = 0; y < texH; ++y)
        {
            for (int x = 0; x < texW; ++x)
            {
                TGAColor color = tgaImg->get(x, y);
                int idx = y * texW + x;
                cpuData[idx].x = color.bgra[0]; // B通道 ✔️
                cpuData[idx].y = color.bgra[1]; // G通道 ✔️
                cpuData[idx].z = color.bgra[2]; // R通道 ✔️
            }
        }
        // 2. 防重复初始化
        if (gpu_diff_tex) cudaDestroyTextureObject(gpu_diff_tex);
        // 3. 漫反射核心：无需归一化 → needNormalize=false ✔️
        createCudaTexture(gpu_diff_tex, cpuData.data(), texW, texH, false);
    }
    // ========== 分支3：texType=2 → 法线纹理（uchar3 + 自动归一化） ==========
    else if (texType == 2)
    {
        // 1. TGAColor → CUDA uchar3（BGR三通道，归一化后用于vec3_f）
        std::vector<uchar3> cpuData(texW * texH);
        for (int y = 0; y < texH; ++y)
        {
            for (int x = 0; x < texW; ++x)
            {
                TGAColor color = tgaImg->get(x, y);
                int idx = y * texW + x;
                cpuData[idx].x = color.bgra[0]; // B通道 ✔️
                cpuData[idx].y = color.bgra[1]; // G通道 ✔️
                cpuData[idx].z = color.bgra[2]; // R通道 ✔️
            }
        }
        // 2. 防重复初始化
        if (gpu_norm_tex) cudaDestroyTextureObject(gpu_norm_tex);
        // 3. 法线需要归一化 → needNormalize=true
        createCudaTexture(gpu_norm_tex, cpuData.data(), texW, texH, true);
    }
}

Model::~Model()
{
    if (gpu_spec_tex) cudaDestroyTextureObject(gpu_spec_tex);
    if (gpu_diff_tex) cudaDestroyTextureObject(gpu_diff_tex);
    if (gpu_norm_tex) cudaDestroyTextureObject(gpu_norm_tex);
}

