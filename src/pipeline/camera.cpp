#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "camera.h"
#include "rasterization.h"

#include <math.h>
#include <iostream>
#include <algorithm>

void 
Camera::setPos(vec4 newPos)
{
    assert(newPos.w != 0 && "newPos is position, but it's w = 0");
    e = newPos;
    viewDirty = true;
    return;
}

void 
Camera::addShift(vec4 shift)
{
    assert(shift.w == 0 && "shift is vector, but it's w not 0");
    this->shift = shift;
    viewDirty = true;
    return;
}

void 
Camera::setGaze(vec4 newGaze)
{
    assert(newGaze.w == 0 && "gaze is vector, but it's w not 0");
    this->g = newGaze;
    viewDirty = true;
    return;
}

void 
Camera::setT(vec4 newT)
{
    assert(newT.w == 0 && "t is vector, but it's w not 0");
    this->t = newT;
    viewDirty = true;
    return;
}

void 
Camera::setNearAndFar(double newNear, double newFar)
{
    assert(newFar > 0 && newNear > 0 && "near & far should be postive, i'll trans it, no worry");

    this->near = newNear;
    this->far = newFar;
    projDirty = true;
    return;
}

void 
Camera::setFov(double newFov)
{
    this->fov = newFov;
    projDirty = true;
    return;
}

void 
Camera::setAspect(double newAspect)
{
    this->aspect = newAspect;
    projDirty = true;
    return;
}

Camera::Camera(double _aspect)
    : e({0, 0, 2., 1}), g({0, 0, -1, 0}), t({0, 1, 0, 0}), shift({0, 0, 0, 0}),
    fov(M_PI / 2.), aspect(_aspect), near(1.), far(100.), perspective(true),
    viewDirty(true), projDirty(true)
{
    double hdiv2 = std::abs(near) * std::tan(fov/2);    // 几何关系推导
    this->top = hdiv2;
    this->bottom = -hdiv2;
    this->right = aspect * hdiv2;
    this->left = -this->right;
}

mat4 
Camera::getPersp2OrthoMat(void) const
{
    if (perspective)
    {
        /*
            根据相似三角形（cam - near - z）可得到x、y上的映射关系（相机坐标系，认为n、f为正值）：
            x'= x * n/(-z), y'= y * n/(-z)
            注：这里课程ppt说的其实是一个全是正的情况，所以实践来才发现有错漏之处，所以修改

            但这时z也会变，未知，就要靠“近平面完全不变”与“远平面只有中点z不变”两个来构造、解方程。

            n   0   0   0
            0   n   0   0
            0   0 n+f  nf
            0   0  -1   0
        */

        mat4 persp2orthoMat;
        persp2orthoMat(0, 0) = near;
        persp2orthoMat(1, 1) = near;
        persp2orthoMat(2, 2) = near + far;
        persp2orthoMat(2, 3) = near * far;
        persp2orthoMat(3, 2) = -1.;

        return persp2orthoMat;
    }

    else    // 如果坚持正交则返回单位矩阵
    {
        return get1Mat();
    }
}

mat4
Camera::getOrthoMat(void) const
{
    mat4 moveBack = get1Mat();  // 把视长方体中心移动到坐标原点
    moveBack(0, 3) = -0.5 * (left  + right);
    moveBack(1, 3) = -0.5 * (bottom+ top);
    moveBack(2, 3) =  0.5 * (near  + far);  // 更新：near与far现在为绝对值，要加负号转换为原来的坐标，实则负负得正

    mat4 transMat;  // 从视长方体转换为[-1, 1]^3正立方体
    transMat(0, 0) = 2 / (right- left);
    transMat(1, 1) = 2 / (top  - bottom);
    transMat(2, 2) = 2 / (far  - near);  // 更新：同上，near与far含义变化
    transMat(3, 3) = 1;

    mat4 OrthoMat = transMat * moveBack;    // 先平移回原点再变换
    return OrthoMat;
}

mat4
Camera::getViewMat(void) const
{
    // 这里要做的其实是把世界坐标系与相机坐标系对齐，想想相机带着一堆东西，平移，旋转，和世界坐标系对齐

    // 把相机移回原点/站在相机坐标系原点    
    mat4 moveBack = get1Mat();
    moveBack(0, 3) = -e.x -shift.x;
    moveBack(1, 3) = -e.y -shift.y;
    moveBack(2, 3) = -e.z -shift.z;

    /*
        再获取摆正相机旋转矩阵，从“旋转矩阵其实就是基坐标变换”的角度比较好理解：
        旋转矩阵的每一行都是目标坐标系的基向量在当前坐标系中的表示
        比如单位矩阵之所以代表“1”，就是其做了一个从当前坐标系到当前坐标系的映射

        而我们的g、t、gxt都是单位向量，可以直接代入
    */
    mat4 align; // “对齐”之意
    vec4 gxt = cross(g, t); // 单位向量，相互垂直，结果的模就还是1

    align(0, 0) = gxt.x,    align(0, 1) = gxt.y,    align(0, 2) = gxt.z, 
    align(1, 0) = t.x,      align(1, 1) = t.y,      align(1, 2) = t.z, 
    align(2, 0) =-g.x,      align(2, 1) =-g.y,      align(2, 2) =-g.z, 
    align(3, 3) = 1; 

    mat4 ViewMat;
    ViewMat = align * moveBack; // 先移回原点，再旋转对齐，很直观

    viewDirty = false;  // 清除脏位
    return ViewMat;
}

mat4
Camera::getProjMat(void) const
{
    /*
        投影变换还没懂的地方：
        1.视锥按理来说是与cam同轴的，为何平移时的写法似乎是还考虑到视锥中心不在z轴上。
        明白的妙处：
        2.near和far本身就只是规定了“最近/远能看到哪里”，再加上fov与aspect，这样在规定near与far的同时也实际的划分好了视锥，
          变换矩阵是对所有点变换的，原本在视锥外面的，变换后也仍是在视锥外面，不必担忧。
    */

    /*
        此时镜头已经对准想要看的区域，接下来把视锥里的东西进行投影变换
        若采用透视投影，就是把视锥整个压缩为长方体
        然后利用正交投影把长方体变换到单位立方体内
    */
    mat4 persp2orthoMat = getPersp2OrthoMat();
    mat4 orthoMat = getOrthoMat();

    mat4 ProjMat = orthoMat * persp2orthoMat; // 先把透视的视锥变换为正交的长方体，再缩放到[-1, 1]^3
    projDirty = false;
    return ProjMat;
}
