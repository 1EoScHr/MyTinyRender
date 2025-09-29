#ifndef __DEFS_H
#define __DEFS_H

#include "tgaimage.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order // constexpr：告诉编译器在编译器这个东西就能算出来
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

struct Point
{
    int x;
    int y;
    TGAColor color = white;

    Point(int _x, int _y, TGAColor _c = white) // cpp的结构体也能有构造函数，因为其就是一个特殊的类（或者类是特殊的结构体？）
        : x(_x), y(_y), color(_c) {}
};

#endif