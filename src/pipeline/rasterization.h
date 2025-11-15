/*
    rasterization.h
    除了vertex和fragment处理外的所有步骤，也就是实际上由图形学api与GPU自动完成的部分
*/

#pragma once

#include "../basic/defs.h"
#include "../basic/tgaimage.h"
#include "../basic/homocoor.h"
#include "vertex.h"

#include <iostream>

class Rasterization 
{
public:
    void renderOBJ(Model& model, Camera& camera);
    
    Rasterization(TGAImage& _buffer, TGAImage& _zbuffer, std::vector<vec4> v);

private:
    TGAImage buffer, zbuffer;
    mat4 modelMat, viewMat, projMat, viewPortMat;

    std::vector<vec4> v_copy;
    
    bool viewPortDirty;
    mat4 getViewPortMat();

    std::pair<std::vector<int>, std::vector<int>> getBbox(const Pixel& a, const Pixel& b, const Pixel& c); // Pixel封装，获取三角形包围框
    void renderTriangle(const Pixel& a, const Pixel& b, const Pixel& c);
    void renderTriangle_noJudge(const Pixel& a, const Pixel& b, const Pixel& c);

    bool showAxis;
    void renderAxis();
};

// 计算三像素围成的有向面积
inline
double computeArea(const Pixel& a, const Pixel& b, const Pixel& c)
{
    /*
        三角形共面任意一点都可以用三角形的重心坐标来表示，并且任意一值小于0就能判定这一点位于三角形外
        根据证明（没看明白），可知三角形的有向面积与对应的重心坐标成正比，也就是重心坐标为负，对应的有向面积也为负
        
        譬如点P在a、b、c对应的三个重心坐标分别是Area(PBC)/Area(ABC)、Area(APC)/Area(ABC)、Area(ABP)/Area(ABC)，只要一一对应，顺序可变

        向量叉积能够表示两向量组成的平行四边形的有向面积，也就是是三角形有向面积的两倍，
        所以用二维向量叉积u✖v就可以求出三角形的有向面积，其等价于二维标量(u_x*v_y - u_y*v_x)，
        其中，u=ab=(bx-ax, by-ay), v=ac=(cx-ax, cy-ay)。
    */
    return (a.x*(b.y-c.y) + b.x*(c.y-a.y) + c.x*(a.y-b.y)) / 2.0; 
}
