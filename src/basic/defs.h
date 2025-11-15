#pragma once // 与ifndef define endif作用一样

#include "homocoor.h"
#include "tgaimage.h"

// TGAColor颜色有关
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order // constexpr：告诉编译器在编译器这个东西就能算出来
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

inline 
TGAColor getRandomColor(void) // 获取随机颜色
{
    return {static_cast<unsigned char>(std::rand()%256), 
            static_cast<unsigned char>(std::rand()%256), 
            static_cast<unsigned char>(std::rand()%256),
            255};
}

struct Pixel    // 这里原来是叫Point，但有vec3后，就得更具体，这里就是屏幕上一个像素对应的数据结构
{
    int x;
    int y;
    double depth; // 把[far, near]的z缩放为[-1, 1]的深度
    TGAColor color;

    // 构造函数
    Pixel(int _x, int _y, double _depth = 0, TGAColor _c = white)
        : x(_x), y(_y), depth(_depth), color(_c) {}
    Pixel(vec4 screenPoint, TGAColor _c)
        : color(_c) {   
                this->x = static_cast<int>(std::lround(screenPoint.x)), 
                this->y = static_cast<int>(std::lround(screenPoint.y)), 
                this->depth = screenPoint.z;
                }
};

struct face_obj // 组成一个面的三个点索引
{
    int v1, v2, v3;
};