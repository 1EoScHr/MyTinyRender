#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "model.h"
#include "camera.h"
#include "shader.h"
#include "rasterization.h"

#include <math.h>
#include <ranges>
#include <limits>
#include <iostream>
#include <algorithm>

Rasterization::Rasterization(TGAImage& _buffer)
    : buffer(_buffer), depthbuffer(nullptr), zbuffer(_buffer.width(), _buffer.height()), 
      viewPortDirty(true), showAxis(false), showZbuffer(false) { }

void 
Rasterization::renderOBJ(Model& model, Camera& camera, Shader& shader)
{
    /* 一条渲染管线开始 */

    std::cout << "绘制中" << std::endl;

    mat4 MV = camera.getViewMat() * model.getModelMat();// 模型变换 Model + 视图变换 view/Camera
    mat4 PV = getViewPortMat() * camera.getProjMat();   // 投影变换 Projection + 视口变换 Viewport
    shader.getMVPV(MV, PV);

    float zfar, znear;
    zfar = std::numeric_limits<float>::max();
    znear= std::numeric_limits<float>::lowest();
    
    const auto& f = model.getFace();    // 面信息获取
    for (auto& iter : f)    // 遍历模型所有面
    {   
        // 使用vertex shader依次处理顶点，可以加特殊需求
        Vertex v1 = shader.vertex(iter, 0);
        Vertex v2 = shader.vertex(iter, 1);
        Vertex v3 = shader.vertex(iter, 2);

        // 为zbuffer2tga准备数据
        auto [zzfar, zznear] = std::minmax({v1.depth, v2.depth, v3.depth});
        znear= znear>zznear? znear:zznear;
        zfar = zfar <zzfar ? zfar :zzfar;

        // fragment shader在渲染三角形内
        renderTriangle({v1, v2, v3}, shader);
    }

    zbuffer2tga(znear, zfar);
    renderAxis(shader);
}

// 计算重心坐标，绘制三角形
void 
Rasterization::renderTriangle(const std::array<Vertex, 3>& screen, Shader& shader)
{
    // 首先通过bbox判断是否在屏幕内，不在屏幕直接跳过
    auto [lb, rt] = getBbox(screen);
    if (lb.first > rt.first || lb.second > rt.second) return;

    // 然后利用有向面积背面剔除器，一种优化，原理见历史commit的drawjusttriangle中的注释
    float s_ABC = computeArea(screen);
    if (std::signbit(s_ABC)) return;
    
    #pragma omp parallel for    // 让编译器把其后的for循环并行化，在多核CPU上让不同线程分工执行循环迭代
    for(auto px = lb.first; px <= rt.first; px ++)
    {
        for(auto py = lb.second; py <= rt.second; py ++)
        {    
            // 计算重心坐标，用float足够
            float s_PBC = computeArea({Vertex{px,py}, screen[1], screen[2]});
            float s_PCA = computeArea({Vertex{px,py}, screen[2], screen[0]});
            float s_PAB = computeArea({Vertex{px,py}, screen[0], screen[1]});

            vec3_f abg = {s_PBC / s_ABC, s_PCA / s_ABC, s_PAB / s_ABC};

            // 判断是否在三角形内，根据重心坐标符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(abg.alpha) || std::signbit(abg.beta) || std::signbit(abg.gamma))
                continue;

            // z-buffer更新
            float d = (abg.alpha*screen[0].depth + 
                        abg.beta*screen[1].depth + 
                       abg.gamma*screen[2].depth); // 计算当前点深度

            if(d <= zbuffer(px, py)) // 深度测试：如果比现有更深，则不画，注意z越小、depth越小、越在后
            {
                continue; 
            }

            // 调用shader获取是否丢弃、颜色
            auto [discard, color] = shader.fragment(abg);

            if (discard)    // 用于一些高级纹理，哪怕通过了所有测试，也会放弃
            {
                continue;
            }

            // buffer填充
            zbuffer(px, py) = d;
            buffer.set(px, py, color);
        }
    }
}

// 计算重心坐标，在原图与zbuffer上绘制三角形
void 
Rasterization::renderTriangle_noJudge(const std::array<Vertex, 3>& screen, Shader& shader)
{
    auto [lb, rt] = getBbox(screen);
    if (lb.first > rt.first || lb.second > rt.second) return;

    float s_ABC = computeArea(screen);
    
    // 让编译器把紧跟其后的 for 循环并行化，在多核 CPU 上让不同线程分工执行循环迭代
    #pragma omp parallel for
    for(auto px = lb.first; px < rt.first; px ++)
    {
        for(auto py = lb.second; py < rt.second; py ++)
        {
            // 计算重心坐标
            float s_PBC = computeArea({Vertex{px,py}, screen[1], screen[2]});
            float s_PCA = computeArea({Vertex{px,py}, screen[2], screen[0]});
            float s_PAB = computeArea({Vertex{px,py}, screen[0], screen[1]});

            vec3_f abg = {s_PBC / s_ABC, s_PCA / s_ABC, s_PAB / s_ABC};

            // 判断是否在三角形内，根据重心坐标符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(abg.alpha) || std::signbit(abg.beta) || std::signbit(abg.gamma))
                continue;

            // z-buffer更新
            float d = (abg.alpha*screen[0].depth + 
                        abg.beta*screen[1].depth + 
                       abg.gamma*screen[2].depth); // 计算当前点深度

            if(d <= zbuffer(px, py)) // 深度测试：如果比现有更深，则不画，注意z越小、depth越小、越在后
            {
                continue; 
            }

            // 调用shader获取是否丢弃、颜色
            auto [discard, color] = shader.fragment(abg);

            if (discard)    // 用于一些高级纹理，哪怕通过了所有测试，也会放弃
            {
                continue;
            }

            // buffer填充
            zbuffer(px, py) = d;
            buffer.set(px, py, color);
        }
    }
}

void
Rasterization::zbuffer2tga(float znear, float zfar)
{
    if (showZbuffer == 0) return;

    assert(depthbuffer != nullptr && "zbufferTGA invalid");
    assert(depthbuffer-> width() == zbuffer.width && 
           depthbuffer->height() == zbuffer.height && 
           "zbufferTGA not fit zbuffer in memory!");

    // zfar-znear : 0 - 255
    float k = 255.f/(znear-zfar), b = 255.f*zfar/(zfar-znear);

    // 这也是c++23特性，debian12用不了……
    // for (auto [idx, z] = std::views::enumerate(zbuffer.depth))
    // 下面这种写法共享的idx会在并行时造成索引错乱
    // for (auto iter = zbuffer.depth.begin(); iter != zbuffer.depth.end(); ++iter)

    #pragma omp parallel for 
    for (size_t i = 0; i < zbuffer.depth.size(); i ++)
    {
        auto d = zbuffer.depth[i];
        if (d == std::numeric_limits<float>::lowest()) continue;
        depthbuffer->set(i, {static_cast<uint8_t>(k*d+b)});
    }
}

void
Rasterization::cheese(void)
{    
    buffer.write_tga_file("framebuffer.tga");
    if(showZbuffer) depthbuffer->write_tga_file("zbuffer.tga");
    std::cout << "绘制完毕" << std::endl;
}

void 
Rasterization::setAxis(bool axis)
{
    this->showAxis = axis;
    return;
}

void 
Rasterization::setShowZb(bool showzb, TGAImage* _depthbuffer)
{
    this->showZbuffer = showzb;
    this->depthbuffer = _depthbuffer;
    return;
}

void
Rasterization::renderAxis(Shader& shader)
{
    if (showAxis == 0) return;

    // 利用三角形绘制坐标轴，在最后进行
    std::array<vec4, 6> origin; // 原点族，分别在x轴、y轴、z轴
    origin[0] = {-0.05, 0, 0, 1}, origin[1] = {0.05, 0, 0, 1}, 
    origin[2] = {0, -0.05, 0, 1}, origin[3] = {0, 0.05, 0, 1},
    origin[4] = {0, 0, -0.05, 1}, origin[5] = {0, 0, 0.05, 1};

    std::array<vec4, 3> end;    // 终点族
    end[0] = {1.5, 0, 0, 1}, end[1] = {0, 1.5, 0, 1}, end[2] = {0, 0, 1.5, 1};
    
    mat4 VPV = viewPortMat * projMat * viewMat;
    for (auto& iter : origin)
    {
        iter = VPV * iter;
        uintize(iter);
    }
    for (auto& iter : end)
    {
        iter = VPV * iter;
        uintize(iter);
    }
    for (size_t i = 0; i < end.size(); i ++)
    {
        renderTriangle_noJudge({{
            origin[0], origin[1], end[i]
        }}, shader);
        renderTriangle_noJudge({{
            origin[2], origin[3], end[i]
        }}, shader);
        renderTriangle_noJudge({{
            origin[4], origin[5], end[i]
        }}, shader);
    }
}

mat4
Rasterization::getViewPortMat()
{
    mat4 ViewportMat;
    ViewportMat(0, 0) = buffer.width() / 2;
    ViewportMat(0, 3) = buffer.width() / 2;
    ViewportMat(1, 1) = buffer.height()/ 2;
    ViewportMat(1, 3) = buffer.height()/ 2;
    ViewportMat(2, 2) = 1;
    ViewportMat(3, 3) = 1;

    viewPortDirty = false;
    return ViewportMat;
}

// 获取三角形的包围盒，Pixel封装版本，使用结构化绑定比较高效
std::pair<std::pair<int, int>, std::pair<int, int>>
Rasterization::getBbox(const std::array<Vertex, 3>& screen) // 获得BoundingBox
{
    // left back和right top
    auto [l, r] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [b, t] = std::minmax({screen[0].y, screen[1].y, screen[2].y});
    
    // 加一个限制，防止bbox越界，进行裁剪
    return std::make_pair(std::make_pair(std::max(l, 0), std::max(b, 0)),
                          std::make_pair(std::min(r, buffer.width()-1), std::min(t, buffer.height()-1)));
}
