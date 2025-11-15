#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "vertex.h"
#include "rasterization.h"

#include <math.h>
#include <iostream>
#include <algorithm>

Rasterization::Rasterization(TGAImage& _buffer, TGAImage& _zbuffer, std::vector<vec4> v)
    : buffer(_buffer), zbuffer(_zbuffer), 
      v_copy(v), viewPortDirty(true), showAxis(false){}

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

    // 以下是之前基于错误理解实现的，已经不再需要，但是有一点学习意义。
    /*
        // 获取z坐标的最大值与最小值，界定near与far，辅助进行透视
        这个写法是C++20风味的，简洁优美，但得加配置文件让vscode支持cpp20语法
        &vec4::z是投影参数，让编译器不直接比较结构体，而是统一比较投影，是匿名函数[](const &point_obj p){return p.z}的等价简写
        返回值是最小值与最大值的point_obj迭代器，可以当指针，->来引出
        // auto [zfar, znear] = std::ranges::minmax_element(v, {}, &vec4::z);
        // Frustum fruInfo(M_PI/2, buffer.width()/static_cast<double>(buffer.height()), znear->z, zfar->z);   // 初始化视锥，可视角90度，宽高比与屏幕相关（不相关会让画面拉伸），使用near与far
    */

    // 投影变换 Projection
    projMat = camera.getProjMat();
    // 视口变换 Viewport
    viewPortMat = getViewPortMat();

    mat4 MVPV = viewPortMat * projMat * viewMat * modelMat;
    for (auto& iter : v_copy)        // 再进行投影、视口变换，把东西先映射到[-1,1]^3，再到屏幕区域。
    {
        iter = MVPV * iter;
        uintize(iter);
    }

    for(auto& iter : f)
    {
        auto p1 = v_copy[iter.v1], p2 = v_copy[iter.v2], p3 = v_copy[iter.v3];
       
        /*
            //auto p1x = std::round((p1.x+1)*w); 
            round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
            使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
        */
        renderTriangle( {static_cast<int>(std::lround(p1.x)), static_cast<int>(std::lround(p1.y)), p1.z, getRandomColor()},
                        {static_cast<int>(std::lround(p2.x)), static_cast<int>(std::lround(p2.y)), p2.z, getRandomColor()},
                        {static_cast<int>(std::lround(p3.x)), static_cast<int>(std::lround(p3.y)), p3.z, getRandomColor()}
                        );
    }

    std::cout << "绘制完毕" << std::endl;
}

// 计算重心坐标，在原图与zbuffer上绘制三角形
void 
Rasterization::renderTriangle(const Pixel& A, const Pixel& B, const Pixel& C)
{
    
    double s_ABC = computeArea(A, B, C);

    // 背面剔除器，一种优化，原理见drawjusttriangle中的注释
    if (std::signbit(s_ABC)) return;

    auto bbox = getBbox(A, B, C);
    #pragma omp parallel for    // 让编译器把其后的for循环并行化，在多核CPU上让不同线程分工执行循环迭代
    for(auto px = bbox.first[0]; px < bbox.second[0]; px ++)
    {
        for(auto py = bbox.first[1]; py < bbox.second[1]; py ++)
        {
            // 计算重心坐标
            double s_PBC = computeArea(Pixel{px,py}, B, C);
            double s_PCA = computeArea(Pixel{px,py}, C, A);
            double s_PAB = computeArea(Pixel{px,py}, A, B);

            double alpha = s_PBC / s_ABC;
            double beta  = s_PCA / s_ABC;
            double gamma = s_PAB / s_ABC;

            // 判断是否在三角形内，根据符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新，这里是基于灰度图的方式，感觉还能再优化？
            double d = alpha*A.depth + beta*B.depth + gamma*C.depth; // 计算当前点深度
            if(d < zbuffer.getdepth(px, py)) continue; // 如果比现有更深，则不画，注意z越小、depth越小、越在后
            auto g = static_cast<std::uint8_t>(127.5*(d+1)); // 从[-1, 1]映射到[0, 255]
            zbuffer.set(px, py, {g});
            
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
    auto bbox = getBbox(A, B, C);
    double s_ABC = computeArea(A, B, C);

    // 让编译器把紧跟其后的 for 循环并行化，在多核 CPU 上让不同线程分工执行循环迭代
    #pragma omp parallel for
    for(auto px = bbox.first[0]; px < bbox.second[0]; px ++)
    {
        for(auto py = bbox.first[1]; py < bbox.second[1]; py ++)
        {
            // 计算重心坐标
            double s_PBC = computeArea(Pixel{px,py}, B, C);
            double s_PCA = computeArea(Pixel{px,py}, C, A);
            double s_PAB = computeArea(Pixel{px,py}, A, B);

            double alpha = s_PBC / s_ABC;
            double beta  = s_PCA / s_ABC;
            double gamma = s_PAB / s_ABC;

            // 判断是否在三角形内，根据符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新，这里是基于灰度图的方式，感觉还能再优化？
            double d = alpha*A.depth + beta*B.depth + gamma*C.depth; // 计算当前点深度
            if(d < zbuffer.getdepth(px, py)) continue; // 如果比现有更深，则不画，注意z越小、depth越小、越在后
            auto g = static_cast<std::uint8_t>(127.5*(d+1)); // 从[-1, 1]映射到[0, 255]
            zbuffer.set(px, py, {g});
            
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

// Pixel封装版本
std::pair<std::vector<int>, std::vector<int>>
Rasterization::getBbox(const Pixel& a, const Pixel& b, const Pixel& c) // 获得BoundingBox
{
    std::vector<int> lb_bbox, rt_bbox;
    
    auto mm = std::minmax({a.x, b.x, c.x});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);

    mm = std::minmax({a.y, b.y, c.y});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);
    
    return std::make_pair(lb_bbox, rt_bbox);
}