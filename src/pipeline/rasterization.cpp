#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "model.h"
#include "camera.h"
#include "./shader/shader.h"
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

    // std::cout << "绘制...";

    if (model.getModelDirty()) modelMat = model.getModelMat();  // 模型变换 Model
    if (camera.getViewDirty()) viewMat  = camera.getViewMat();  // 视图变换 view/Camera
    if (camera.getProjDirty()) projMat  = camera.getProjMat();  // 投影变换 Projection
    if (getViewPortDirty())    viewPortMat = getViewPortMat();  // 视口变换 Viewport

    shader.getMVP(modelMat, viewMat, projMat);

    const auto& f = model.getFace();    // 面信息获取
    faceTaskOnGPU(model, f, shader);
}

// 计算重心坐标，绘制三角形
void 
Rasterization::renderTriangle(const std::array<Vertex, 3>& screen, Shader& shader, const Model& model)
{
    // 首先通过bbox判断是否在屏幕内，不在屏幕直接跳过
    auto [lb, rt] = getBbox(screen);
    if (lb.first > rt.first || lb.second > rt.second) return;

    // 然后利用有向面积背面剔除器，一种优化，原理见历史commit的drawjusttriangle中的注释
    float s_ABC = computeArea(screen);
    if (std::signbit(s_ABC)) return;        
    
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
            auto [discard, color] = shader.fragment(model, abg);

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

// 计算重心坐标，在原图与zbuffer上绘制三角形，不进行背面剔除测试，用于坐标轴、固定信息等
void 
Rasterization::renderTriangle_noJudge(const std::array<Vertex, 3>& screen, Shader& shader, const Model& model)
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
            auto [discard, color] = shader.fragment(model, abg);

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
Rasterization::zbuffer2tga()
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
    renderAxis(); 
    zbuffer2tga();
    buffer.write_tga_file("framebuffer.tga");
    if(showZbuffer) depthbuffer->write_tga_file("zbuffer.tga");
    // std::cout << "完毕" << std::endl;
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
Rasterization::clearZb(void)
{
    zbuffer.clear();
}

void
Rasterization::renderAxis(void)
{
    if (showAxis == 0) return;

    RandomShader axisShader{};
    Model emptyModel{};

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
        }}, axisShader, emptyModel);
        renderTriangle_noJudge({{
            origin[2], origin[3], end[i]
        }}, axisShader, emptyModel);
        renderTriangle_noJudge({{
            origin[4], origin[5], end[i]
        }}, axisShader, emptyModel);
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

void 
Rasterization::faceTaskOnGPU(const Model& model, const std::vector<face_obj>& f, Shader& shader)
{
    // #pragma omp parallel for    // 让编译器把其后的for循环并行化，在多核CPU上让不同线程分工执行循环迭代
    for (auto& iter : f)    // 遍历模型所有面
    {   
        // 使用vertex shader获取面的各顶点，但是是裁剪坐标下
        vec4 clip0 = shader.vertex(model, iter, 0);
        vec4 clip1 = shader.vertex(model, iter, 1);
        vec4 clip2 = shader.vertex(model, iter, 2);

        // 先判断是否在视锥内，裁剪相关
        bool in0 = in_frustum(clip0);
        bool in1 = in_frustum(clip1);
        bool in2 = in_frustum(clip2);
    
        int total = in0 + in1 + in2;
        switch (total)
        {
            case 0: continue;  // 整个都不在，直接去下一个面
            case 3: otherGPUWork(clip0, clip1, clip2, shader, model); break;   // 整个都在，无事发生，继续往下
//                       //
// //                 // //
// TODO： 用逐平面裁剪算法 //
// //                 // //
//                       //
            case 1: otherGPUWork(clip0, clip1, clip2, shader, model); break;   // 卧槽，好复杂，先空下
            case 2: otherGPUWork(clip0, clip1, clip2, shader, model); break;
        }
    }
}

void
Rasterization::otherGPUWork(vec4& clip0, vec4& clip1, vec4& clip2, Shader& shader, const Model& model)
{
    /*
        vertex shader得到裁剪坐标，接下来要通过GPU的另一个模块进行处理
        会进行坐标裁剪（即把视锥之外的点排除）、透视除法（也就是我的uintize），
        最后返回Vertex类型。
    
        坐标裁剪比较难理解，其是使用裁剪坐标来排除视锥之外的点。
        1.为何不直接在MV变换之后的真实几何空间来裁剪？
            真实几何空间里，视锥是最直观的，但是其是一个台型，进行裁剪时要受到深度的制约，不是很简洁。
        2.为何不用透视除法之后的NDC空间（[-1,1]），明明其更直观？
            透视除法是基于每个点自己的w值，而每一个点的w都不一样（在推导透视投影时对w乘了z值）
            那么透视除法就要把z除掉，这就引入了非线性。所以真实几何中的一条直线，在NDC中就可能会变成曲线。
            继续推进，坐标裁剪还有一个关键的步骤，就是在视锥边缘补上另外合法的值，像用剪刀减去三角形的一个角
            可以十分直观的想到在真实几何空间内，一个三角形跨越了视锥，两平面交线必为直线，那么直线代表线性，
            所以用旧值可以为边界插值出合法值。但是由于NDC引入了非线性，所以这个边界映射到NDC就是一个曲线，
            插值就苦难；若直接在NDC中连直线剪三角形，这本来就不对。
        因此就只能使用裁剪坐标（怪不得叫裁剪坐标，后面的条件简直证明了其天生为裁剪而生）：
            裁剪空间使用的是经过投影变换的齐次坐标，投影变换这一操作在几何上压缩了深度，这的确也是一个非线性，
            但是由于其四维齐次的性质，原本的线性关系却并没有被丢弃（这里其实我也是知其然不知其所以然，但确实妙）
            直线仍是直线、平面仍是平面（在齐次意义上），相当于兼顾真实几何空间的线性与透视变换后裁剪条件的判断
        
        接下来是裁剪坐标“怎么用”的问题，这个更难。
        首先找一个特例来研究，选择fov=90度、aspect=1时，视锥就是一个很漂亮的等腰直角三角形，
        这个视锥的上下左右边界都与深度z相等（或为负）。
        这时考虑投影矩阵做了什么，手算一下投影矩阵的表达式，另外这里还改了一下near与far的意义：
            2n/(r-l)        0 (r+l)/(r-l)         0   
                   0 2n/(t-b) (t+b)/(t-b)         0
                   0        0 (f+n)/(f-n) 2nf/(f-n)
                   0        0          -1         0
        与(x, y, z, w)相乘，得到w为-z，那么由此就能进行筛除了。当然这是一个特例，那么其他fov、aspect情况下呢？
        这里有些反直觉，但就是挺厉害，投影矩阵会把所有情况都通通压缩成一个以w为分界线的锥，先这么用，再细究再说。
    */ 

    // 接下来就是透视除法与画三角形
    uintize(clip0);
    uintize(clip1);
    uintize(clip2);

    Vertex v0 = static_cast<Vertex>(viewPortMat * clip0);
    Vertex v1 = static_cast<Vertex>(viewPortMat * clip1);
    Vertex v2 = static_cast<Vertex>(viewPortMat * clip2);

    // 为zbuffer2tga准备数据
    auto [zzfar, zznear] = std::minmax({v0.depth, v1.depth, v2.depth});
    znear= znear>zznear? znear:zznear;
    zfar = zfar <zzfar ? zfar :zzfar;

    // fragment shader在渲染三角形内
    renderTriangle({v0, v1, v2}, shader, model);
}

// 判断一个裁剪坐标是否在视锥内，属于实际GPU上的固件，所以这里if多没事
inline bool
Rasterization::in_frustum(const vec4& clip)
{
    /*
        这里还突然想到远期的一个东西，景深、聚焦、失焦类似的
        但千问说这些东西还是在视锥内的，景深是再往后的处理。
    */

    // 也是六个面判断
    if (clip.w < 0) return false;   // 在相机背面
    if (clip.x < -clip.w || clip.x > clip.w) return false;  // x方向，左右
    if (clip.y < -clip.w || clip.y > clip.w) return false;  // y方向，上下
    if (clip.z < -clip.w || clip.z > clip.w) return false;  // z方向，near/far，这里也有点难懂，但也勉强意会，先这样吧……
    
    return true;    // 在视锥内，返回true
}
