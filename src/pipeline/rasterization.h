/*
    rasterization.h
    除了vertex和fragment处理外的所有步骤，也就是实际上由图形学api与GPU自动完成的部分
*/

#pragma once

#include "../basic/defs.h"
#include "../basic/tgaimage.h"
#include "../basic/homocoor.h"
#include "vertex.h"
#include "fragment.h"

#include <iostream>

struct ZBuffer
{
    std::vector<float> depth;
    
    int width;
    int height;

    // i是行索引(y)，j是列索引(x)，这是反的!
    float& operator()(const int x, const int y)       { assert(x>=0 && x<width && y>=0 && y<height); return depth[x + y*width];}
    float  operator()(const int x, const int y) const { assert(x>=0 && x<width && y>=0 && y<height); return depth[x + y*width];}

    ZBuffer(int _width, int _height)
        : width(_width), height(_height)
    {
        /*
            FLT_MAX是float能表示的最大值，那么加个负号就是最小值
            // zbuffer.resize(buffer.width()*buffer.height(), -__FLT_MAX__);
            可是cpp不推荐这样，因为宏会污染作用域，
            使用std::numeric_limits<float>类型安全，还支持模板
        */
        depth.resize(width*height, std::numeric_limits<float>::lowest());  // 让深度缓冲默认最远(值最小)
    }
};

class Rasterization 
{
public:
    void renderOBJ(Model& model, Camera& camera, Shader& shader);
    void cheese();  // 最终的转出句柄

    void setAxis(bool axis);
    void setShowZb(bool showzb, TGAImage* _depthbuffer);
    
    Rasterization(TGAImage& _buffer, const std::vector<vec4>& v);

private:
    TGAImage& buffer;
    TGAImage* depthbuffer;  // 可视化的zbuffer
    ZBuffer zbuffer; // zbuffer只要能够体现出相对深度，double精度过剩
    mat4 modelMat, viewMat, projMat, viewPortMat;

    std::vector<vec4_zf> v_copy;
    
    bool viewPortDirty;
    mat4 getViewPortMat();
    bool showAxis;
    bool showZbuffer;

    std::pair<std::pair<int, int>, std::pair<int, int>> getBbox(const std::array<Vertex, 3>& screen); // 获取三角形包围框
    void renderTriangle(const std::array<Vertex, 3>& screen, Shader& shader);
    void renderTriangle_noJudge(const std::array<Vertex, 3>& screen, Shader& shader);
    void zbuffer2tga();
    void renderAxis(Shader& shader);
};

// 计算三像素围成的有向面积
inline
float computeArea(const std::array<Vertex, 3>& screen)
{
    /*
        三角形共面任意一点都可以用三角形的重心坐标来表示，并且任意一值小于0就能判定这一点位于三角形外
        根据证明（没看明白），可知三角形的有向面积与对应的重心坐标成正比，也就是重心坐标为负，对应的有向面积也为负
        
        譬如点P在a、b、c对应的三个重心坐标分别是Area(PBC)/Area(ABC)、Area(APC)/Area(ABC)、Area(ABP)/Area(ABC)，只要一一对应，顺序可变

        向量叉积能够表示两向量组成的平行四边形的有向面积，也就是是三角形有向面积的两倍，
        所以用二维向量叉积u✖v就可以求出三角形的有向面积，其等价于二维标量(u_x*v_y - u_y*v_x)，
        其中，u=ab=(bx-ax, by-ay), v=ac=(cx-ax, cy-ay)。
    */
    return (screen[0].x*(screen[1].y-screen[2].y) + 
            screen[1].x*(screen[2].y-screen[0].y) + 
            screen[2].x*(screen[0].y-screen[1].y)) / 2.0f; 
}
