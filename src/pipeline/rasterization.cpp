#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "vertex.h"
#include "rasterization.h"

#include <math.h>
#include <limits>
#include <ranges>
#include <iostream>
#include <algorithm>

Rasterization::Rasterization(TGAImage& _buffer, const std::vector<vec4>& v)
    : buffer(_buffer), depthbuffer(nullptr), zbuffer(_buffer.width(), _buffer.height()), 
      viewPortDirty(true), showAxis(false), showZbuffer(false)
{
    v_copy.resize(v.size());   // 让rasterization里面的vertex副本与原值一致
}

void 
Rasterization::renderOBJ(Model& model, Camera& camera)
{
    std::cout << "绘制中" << std::endl;

    // 获取模型的点、面信息，顶点由于要被变换，为满足其不可变，直接使用其副本
    // v实际上是vertexCopy
    const auto& f = model.getFace();

    // 模型变换 Model
    modelMat = model.getModelMat();
    // 视图变换 view/Camera
    viewMat = camera.getViewMat();
    // 投影变换 Projection
    projMat = camera.getProjMat();
    // 视口变换 Viewport
    viewPortMat = getViewPortMat();

    mat4 MVPV = viewPortMat * projMat * viewMat * modelMat;
    /*
        // 下面这是cpp23引入的新特性，用zip结构化绑定，同步访问；但是现在用的debian12，没升级，用不了www
        for (auto& [iter, rawiter] : std::ranges::views::zip(v_copy, model.getVertex())) // 再进行投影、视口变换，把东西先映射到[-1,1]^3，再到屏幕区域。
        {
            iter = MVPV * rawiter;
            uintize(iter);
        }
    */
    // 就用这个简陋手动方法 
    const auto& v_raw = model.getVertex();
    assert(v_copy.size() == v_raw.size() && "vertex's raw and copy not same size");
    for (size_t i = 0; i < v_copy.size(); i ++)
    {
        v_copy[i] = MVPV * v_raw[i];
        uintize(v_copy[i]);
    }

    for (auto& iter : f)
    {
        auto p1 = v_copy[iter.v1], p2 = v_copy[iter.v2], p3 = v_copy[iter.v3];
       
        /*
            //auto p1x = std::round((p1.x+1)*w); 
            round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
            使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
        */
        renderTriangle( {static_cast<int>(std::lround(p1.x)), static_cast<int>(std::lround(p1.y)), p1.z, getRandomColor()},
                        {static_cast<int>(std::lround(p2.x)), static_cast<int>(std::lround(p2.y)), p2.z, getRandomColor()},
                        {static_cast<int>(std::lround(p3.x)), static_cast<int>(std::lround(p3.y)), p3.z, getRandomColor()});
    }
}

// 计算重心坐标，在原图与zbuffer上绘制三角形
void 
Rasterization::renderTriangle(const Pixel& A, const Pixel& B, const Pixel& C)
{
    // 首先通过bbox判断是否在屏幕内，不在屏幕直接跳过
    auto [lb, rt] = getBbox(A, B, C);
    if (lb.first > rt.first || lb.second > rt.second) return;

    // 然后利用有向面积背面剔除器，一种优化，原理见历史commit的drawjusttriangle中的注释
    float s_ABC = computeArea(A, B, C);
    if (std::signbit(s_ABC)) return;
    
    #pragma omp parallel for    // 让编译器把其后的for循环并行化，在多核CPU上让不同线程分工执行循环迭代
    for(auto px = lb.first; px <= rt.first; px ++)
    {
        for(auto py = lb.second; py <= rt.second; py ++)
        {    
            // 计算重心坐标，用float足够
            float s_PBC = computeArea(Pixel{px,py}, B, C);
            float s_PCA = computeArea(Pixel{px,py}, C, A);
            float s_PAB = computeArea(Pixel{px,py}, A, B);

            float alpha = s_PBC / s_ABC;
            float beta  = s_PCA / s_ABC;
            float gamma = s_PAB / s_ABC;

            // 判断是否在三角形内，根据重心坐标符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新
            float d = (alpha*A.depth + beta*B.depth + gamma*C.depth); // 计算当前点深度

            if(d <= zbuffer(px, py)) // 如果比现有更深，则不画，注意z越小、depth越小、越在后
            {
                continue; 
            }
// auto g = static_cast<std::uint8_t>(127.5*((d>1?:d)+1)); // 从[-1, 1]映射到[0, 255]
            zbuffer(px, py) = d;
            
            // 填充正经buffer
            buffer.set(px, py, 
               {static_cast<std::uint8_t>(alpha*A.color[0]+beta*B.color[0]+gamma*C.color[0]),
                static_cast<std::uint8_t>(alpha*A.color[1]+beta*B.color[1]+gamma*C.color[1]),    
                static_cast<std::uint8_t>(alpha*A.color[2]+beta*B.color[2]+gamma*C.color[2]),
                static_cast<std::uint8_t>(alpha*A.color[3]+beta*B.color[3]+gamma*C.color[3])});
        }
    }
}

// 计算重心坐标，在原图与zbuffer上绘制三角形
void 
Rasterization::renderTriangle_noJudge(const Pixel& A, const Pixel& B, const Pixel& C)
{
    auto [lb, rt] = getBbox(A, B, C);
    if (lb.first > rt.first || lb.second > rt.second) return;

    float s_ABC = computeArea(A, B, C);
    
    // 让编译器把紧跟其后的 for 循环并行化，在多核 CPU 上让不同线程分工执行循环迭代
    #pragma omp parallel for
    for(auto px = lb.first; px < rt.first; px ++)
    {
        for(auto py = lb.second; py < rt.second; py ++)
        {
            // 计算重心坐标
            float s_PBC = computeArea(Pixel{px,py}, B, C);
            float s_PCA = computeArea(Pixel{px,py}, C, A);
            float s_PAB = computeArea(Pixel{px,py}, A, B);

            float alpha = s_PBC / s_ABC;
            float beta  = s_PCA / s_ABC;
            float gamma = s_PAB / s_ABC;

            // 判断是否在三角形内，根据符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新
            float d = alpha*A.depth + beta*B.depth + gamma*C.depth; // 计算当前点深度
            if(d <= zbuffer(px, py)) continue; // 如果比现有更深，则不画，注意z越小、depth越小、越在后
            //auto g = static_cast<std::uint8_t>(127.5*(d+1)); // 从[-1, 1]映射到[0, 255]
            zbuffer(px, py) = d;
            
            // 填充正经buffer
            buffer.set(px, py, 
               {static_cast<std::uint8_t>(alpha*A.color[0]+beta*B.color[0]+gamma*C.color[0]),
                static_cast<std::uint8_t>(alpha*A.color[1]+beta*B.color[1]+gamma*C.color[1]),    
                static_cast<std::uint8_t>(alpha*A.color[2]+beta*B.color[2]+gamma*C.color[2]),
                static_cast<std::uint8_t>(alpha*A.color[3]+beta*B.color[3]+gamma*C.color[3])});
        }
    }
}

void
Rasterization::zbuffer2tga(void)
{
    // 刚开始想多了，觉得要遍历一遍zbuffer，或者在渲染时就挑出来，但是由于三角形特质，直接用minmax来接MVP算完的点云即可
    /*
        ~~以下是之前基于错误理解实现的，已经不再需要，但是有一点学习意义。~~
        并非错误，这不还要用回来

        获取z坐标的最大值与最小值，界定near与far，辅助进行透视
        这个写法是C++20风味的，简洁优美，但得加配置文件让vscode支持cpp20语法
        &vec4::z是投影参数，让编译器不直接比较结构体，而是统一比较投影，是匿名函数[](const &point_obj p){return p.z}的等价简写
        返回值是最小值与最大值的point_obj迭代器，可以当指针，->来引出
    */

    auto [zfar, znear] = std::ranges::minmax_element(v_copy, {}, &vec4_zf::z);

    // zfar-znear : 0 - 255
    float k = 255.f/(znear->z-zfar->z), b = 255.f*zfar->z/(zfar->z-znear->z);

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
    if (showAxis) renderAxis();
    buffer.write_tga_file("framebuffer.tga");

    if (showZbuffer)
    {
        assert(depthbuffer != nullptr && "zbufferTGA invalid");
        assert(depthbuffer-> width() == zbuffer.width && 
               depthbuffer->height() == zbuffer.height && 
               "zbufferTGA not fit zbuffer in memory!");
        zbuffer2tga();
        depthbuffer->write_tga_file("zbuffer.tga");
    }

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
Rasterization::renderAxis()
{
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
        renderTriangle_noJudge( 
                {origin[0], getRandomColor()}, 
                {origin[1], getRandomColor()}, 
                {end[i], getRandomColor()});
        renderTriangle_noJudge( 
                {origin[2], getRandomColor()}, 
                {origin[3], getRandomColor()}, 
                {end[i], getRandomColor()});
        renderTriangle_noJudge(
                {origin[4], getRandomColor()}, 
                {origin[5], getRandomColor()}, 
                {end[i], getRandomColor()});
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
Rasterization::getBbox(const Pixel& A, const Pixel& B, const Pixel& C) // 获得BoundingBox
{
    // left back和right top
    auto [l, r] = std::minmax({A.x, B.x, C.x});
    auto [b, t] = std::minmax({A.y, B.y, C.y});
    
    // 加一个限制，防止bbox越界，进行裁剪
    return std::make_pair(std::make_pair(std::max(l, 0), std::max(b, 0)),
                          std::make_pair(std::min(r, buffer.width()-1), std::min(t, buffer.height()-1)));
}
