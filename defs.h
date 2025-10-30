// #pragma once // 与ifndef define endif作用一样

#ifndef __DEFS_H
#define __DEFS_H

#include "tgaimage.h"
#include "defs.cpp" // 由于模板先占位、用时解析的特性，所以要在.h中include.cpp

// TGAColor颜色宏
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order // constexpr：告诉编译器在编译器这个东西就能算出来
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

struct Pixel    // 这里原来是叫Point，但有vec3后，就得更具体，这里就是屏幕上一个像素对应的数据结构
{
    int x;
    int y;
    double depth = 0; // 把(-1)~1的z缩放为0-1的深度
    TGAColor color = white;

    // 构造函数
    Pixel(int _x, int _y, double _depth = 0, TGAColor _c = white)
        : x(_x), y(_y), depth(_depth), color(_c) {}
};

#endif