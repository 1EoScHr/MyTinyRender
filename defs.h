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
    double depth; // 把[far, near]的z缩放为[-1, 1]的深度
    TGAColor color;

    // 构造函数
    Pixel(int _x, int _y, double _depth = 0, TGAColor _c = white)
        : x(_x), y(_y), depth(_depth), color(_c) {}
    Pixel(vec4 screenPoint, TGAColor _c = white)
        : color(_c) {   
                this->x = static_cast<int>(std::lround(screenPoint.x)), 
                this->y = static_cast<int>(std::lround(screenPoint.y)), 
                this->depth = screenPoint.z;
                }
};

struct Model    // 模型变换所用旋转、平移信息
{
    double x_rotate;   // 角度用弧度制
    double y_rotate;
    double z_rotate;

    vec4 pos = {0, 0, 0, 1};   // 模型系坐标原点在世界坐标系的位置，默认是 0,0,0
    vec4 shift = {0, 0, 0, 0}; // 模型的位移，默认也是0,0,0

    ////////////////////////////////////////////////3DV小作业
    vec4 x_axis = {1, 0, 0, 0}; // 模型坐标系，默认与世界坐标系对齐
    vec4 y_axis = {0, 1, 0, 0};
    vec4 z_axis = {0, 0, 1, 0};
    ////////////////////////////////////////////////3DV小作业


    Model(double _x_rotate = 0, double _y_rotate = 0, double _z_rotate = 0)
        : x_rotate(_x_rotate), y_rotate(_y_rotate), z_rotate(_z_rotate) { }  // 位置是个坐标
};

struct Camera   // 视口变换所用相机信息
{
    vec4 e; // 相机位置，是坐标
    vec4 g; // 相机朝向，默认-z
    vec4 t; // 相机上方，默认y

    Camera(vec4 _e = {0, 0, 2, 1})
        : e(_e) { this->g.z = -1, this->t.y = 1; }
};

struct Frustum  // 投影变换所用视锥信息
{
    // 核心参数
    bool perspective = true;// 是否启用正交投影，默认使用
    double fov;             // 弧度，可视角，这里用Y方向的，可以与X方向互相转化
    double aspect;          // 宽高比
    double near, far;       // 近、远平面（z方向）

    // 可根据核心参数解算
    double left, right;     // 近平面左、右（x方向）
    double bottom, top;     // 近平面下、上（y方向）

    Frustum(double _fov, double _aspect, double _near, double _far)
        : fov(_fov), aspect(_aspect), near(_near), far(_far) 
        { 
            double hd2 = std::abs(_near) * std::tan(_fov/2);

            this->top = hd2;
            this->bottom = -hd2;
            this->right = _aspect * hd2;
            this->left = -this->right;
        }
};


#endif