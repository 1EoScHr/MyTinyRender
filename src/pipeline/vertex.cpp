#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "vertex.h"
#include "rasterization.h"

#include <math.h>
#include <iostream>
#include <algorithm>

const std::vector<vec4>& 
Model::getVertex(void) const
{
    return this->v;
}

std::vector<vec4> 
Model::getVertexCopy(void)
{
    return this->v;
}

const std::vector<face_obj>& 
Model::getFace(void) const
{
    return this->f;
}

void 
Model::setPos(vec4 newPos)
{
    assert(newPos.w != 0 && "newPos is position, but it's w = 0");
    pos = newPos;
    modelDirty = true;   // 脏位
    return;
}

void 
Model::addShift(vec4 shift)
{
    assert(shift.w == 0 && "shift is vector, but it's w not 0");
    this->shift = shift; 
    modelDirty = true;
    return;
}

void 
Model::setRotate(double rad, int axis)
{
    assert(axis >= 0 && axis <=2 && "invalid axis");
    rotate[axis] = rad;
    modelDirty = true;
    return;
}

Model::Model(std::string _path) 
    : path(_path), pos({0., 0., 0., 1.}), shift({0., 0., 0., 0.}), 
    rotate({0., 0., 0.}), modelDirty(true)
{
    // v和f由于没有初始化，会调用默认，也就是创造两个空向量
    reader();
}

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
    this->near = newNear;
    this->far = newFar;
    projDirty = true;
    return;
}

void 
Camera::setPersp(bool perspective)
{
    this->perspective = perspective;
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
    : e({0, 0, 2, 1}), g({0, 0, -1, 0}), t({0, 1, 0, 0}), shift({0, 0, 0, 0}),
    fov(M_PI / 2.), aspect(_aspect), near(0.1), far(0.9), viewDirty(true), projDirty(true)
{
    double hdiv2 = std::abs(near) * std::tan(fov/2);    // 几何关系推导
    this->top = hdiv2;
    this->bottom = -hdiv2;
    this->right = aspect * hdiv2;
    this->left = -this->right;
}

mat4
getRotMat(double x, int axis)
{
    // 旋转矩阵特点是绕谁转，谁就不会变，保留原来的值，因此能确定一行；同样的，其他维度旋转就与该轴无关，这样就确定一列

    assert(axis >= 0 && axis <=2);  // 0为x轴，1为y轴，2为z轴
    
    double sinx = std::sin(x);
    // double cosx = std::sqrt(1.0 - sinx * sinx);  // 三角恒等式，但会导致cos符号还需额外判断，不如直接用标准库
    double cosx = std::cos(x);  

    mat4 rotmat = {};

    switch (axis)
    {
        case 0:
            rotmat(0, 0) =     1, /*        0        */ /*        0        */ /*        0        */
            /*        0        */ rotmat(1, 1) =  cosx, rotmat(1, 2) = -sinx, /*        0        */
            /*        0        */ rotmat(2, 1) =  sinx, rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 1:
            rotmat(0, 0) =  cosx, /*        0        */ rotmat(0, 2) =  sinx, /*        0        */
            /*        0        */ rotmat(1, 1) =     1, /*        0        */ /*        0        */
            rotmat(2, 0) = -sinx, /*        0        */ rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 2:
            rotmat(0, 0) =  cosx, rotmat(0, 1) = -sinx, /*        0        */ /*        0        */
            rotmat(1, 0) =  sinx, rotmat(1, 1) =  cosx, /*        0        */ /*        0        */
            /*        0        */ /*        0        */ rotmat(2, 2) =     1; /*        0        */
            break;
    }

    rotmat(3, 3) = 1;

    return rotmat;
    
    /*
    xrotmat:                yrotmat:                zrotmat:
        1,     0,      0,    cosy,     0,   siny,   cosz,  -sinz,      0, 
        0,  cosx,  -sinx,       0,     1,      0,   sinz,   cosz,      0, 
        0,  sinx,   cosx;   -siny,     0,   cosy;      0,      0,      1; 
    */
}

mat4 
Camera::getPersp2OrthoMat(void)
{
    if (perspective)
    {
        /*
            根据相似三角形（cam - near - z）可得到x、y上的映射关系（相机坐标系）：
            x'(y')= x(y) * n/z
            但这时z也会变，未知，就要靠“近平面完全不变”与“远平面只有中点z不变”两个来构造、解方程

            n   0   0   0
            0   n   0   0
            0   0 n+f -nf
            0   0   1   0

            */ 
        mat4 persp2orthoMat;
        persp2orthoMat(0, 0) = near;
        persp2orthoMat(1, 1) = near;
        persp2orthoMat(2, 2) = near + far;
        persp2orthoMat(2, 3) = -near * far;
        persp2orthoMat(3, 2) = 1;

        return persp2orthoMat;
    }

    else    // 如果坚持正交则返回单位矩阵
    {
        return get1Mat();
    }
}

mat4
Camera::getOrthoMat(void)
{
    mat4 moveBack = get1Mat();  // 把视长方体中心移动到坐标原点
    moveBack(0, 3) = -0.5 * (left  + right);
    moveBack(1, 3) = -0.5 * (bottom+ top);
    moveBack(2, 3) = -0.5 * (near  + far);

    mat4 transMat;  // 从视长方体转换为[-1, 1]^3正立方体
    transMat(0, 0) = 2 / (right- left);
    transMat(1, 1) = 2 / (top  - bottom);
    transMat(2, 2) = 2 / (near - far);
    transMat(3, 3) = 1;

    mat4 OrthoMat = transMat * moveBack;    // 先平移回原点再变换
    return OrthoMat;
}

mat4
Model::getModelMat(void)
{
    /*
        获取齐次坐标版模型变换矩阵，包括旋转与平移
        实际计算时，等效为先平移回原点、再旋转、最后平移到目标处
    */
    
    // 先平移回原点/让模型坐标系与世界坐标系重合/站在模型坐标系考虑
    assert(pos.w != 0 && "pos is position, but it's w = 0");
    mat4 moveBack = get1Mat();
    moveBack(0, 3) = -pos.x;
    moveBack(1, 3) = -pos.y;
    moveBack(2, 3) = -pos.z;

    ////////////////////////////////////////////////3DV小作业：绕指定轴旋转
/*
    double angle;
    std::cin >> angle;
    modelInfo.z_rotate = angle / 180 * M_PI; // 绕对应轴转70度

    // 理解成模型坐标系不与世界坐标系平行，根据view变换的经验，可以增加以下步骤：
    // 把模型平移回原点后，先旋转使模型坐标系与世界坐标系重合，再绕预设的各轴旋转，再旋转使模型坐标系恢复原来的值。
    // 当然这里我简化实现，不追求模型坐标系完全与世界坐标系重合，只要两个的z轴能够重合，就绕z轴转。
    // 所以这里只是进了一步来实现绕任意轴旋转，是从绕世界xyz轴旋转进步到经过模型原点任意一轴旋转，要再进一步变成任意一轴，可能还需要把模型坐标原点移动考虑进来

    // 对于要求的(1, 1, 1)与70度情况

    vec4 targetModelZaxis = {1, 1, 0, 0}; // 若与原轴重合，则就会引起断言错误，就不要闲的没事
    normalize(targetModelZaxis);    // 目标z轴正则化向量值


    // 默认的模型坐标系还和世界坐标系重合，要先让其变成设定的（同样为简化，只让z轴对齐，其他两轴跟着转就行）
    // 我的思路是先绕世界z轴转，把模型z轴转到YOZ平面，再绕世界x轴把模型z轴转到世界z轴（当然也可反着，但这似乎就要顺时针）
    // 复习一下“绕某轴转某度”：就是按右手定则，箭头对向眼睛、逆时针

    // 这里选用的三角函数反解也有说法，为了精确的sin、cos符号，把求旋转矩阵的入参改成了弧度
    // 第一步里，操作是把轴投影到在XOY平面上，所以其有可能分布在四个象限，用atan不精确，而atan2则接收x和y的值，刚好；
    // 第二步里，已经把轴移到投影刚好在+y轴上，所以旋转角度只是0-pi，这时刚好用acos解算就没问题。
    mat4 alignRotate1 = getRotMat(std::atan2(targetModelZaxis.y, targetModelZaxis.x), 2);   // 目标轴先绕z轴转到YOZ平面
    mat4 alignRotate2 = getRotMat(std::acos(targetModelZaxis.z), 0);    // 目标轴再绕x轴转到z轴

    // 逆旋转矩阵是旋转矩阵的转置
    trans(alignRotate2);    // z轴转到ZOY面上
    trans(alignRotate1);    // 继续转到目标z轴

    // 模型坐标系的值
    mat4 transTarget = alignRotate1 * alignRotate2;
    assert(std::abs((transTarget * modelInfo.z_axis).z - targetModelZaxis.z) < 1e-6);
    modelInfo.x_axis = transTarget * modelInfo.x_axis;
    modelInfo.y_axis = transTarget * modelInfo.y_axis;
    modelInfo.z_axis = transTarget * modelInfo.z_axis;
    assert(std::abs(modelInfo.z_axis.x - targetModelZaxis.x) < 1e-6);

    // 旋转矩阵的每一行，都是目标空间基向量在当前空间的表示
    mat4 alignRotate;
    alignRotate(0, 0) = modelInfo.x_axis.x, alignRotate(0, 1) = modelInfo.x_axis.y, alignRotate(0, 2) = modelInfo.x_axis.z, 
    alignRotate(1, 0) = modelInfo.y_axis.x, alignRotate(1, 1) = modelInfo.y_axis.y, alignRotate(1, 2) = modelInfo.y_axis.z, 
    alignRotate(2, 0) = modelInfo.z_axis.x, alignRotate(2, 1) = modelInfo.z_axis.y, alignRotate(2, 2) = modelInfo.z_axis.z, 
    alignRotate(3, 3) = 1; 

    // 反向变换回去
    mat4 transAlignRotate = alignRotate;
    trans(transAlignRotate);

    assert(modelInfo.x_rotate == 0);
    assert(modelInfo.y_rotate == 0);
*/
    ////////////////////////////////////////////////3DV小作业

    // 再旋转
    mat4 xrotmat = getRotMat(rotate[0], 0), 
         yrotmat = getRotMat(rotate[1], 1),
         zrotmat = getRotMat(rotate[2], 2);

    mat4 rotmovMat = zrotmat*yrotmat*xrotmat;

    // 计算完矩阵后，就可以认为已经执行了，只是还没有渲染

    ////////////////////////////////////////////////3DV小作业
/*
    mat4 movMat = get1Mat();
    movMat(0, 3) = modelInfo.pos.x;
    movMat(1, 3) = modelInfo.pos.y;
    movMat(2, 3) = modelInfo.pos.z;

    ModelMat = movMat * transAlignRotate * rotmovMat * alignRotate * moveBack; 
    // 先移动回原点，再旋转使模型坐标系对应世界坐标系，此时再绕z轴旋转，再恢复原本的坐标系，再移动到目标地点
*/
    ////////////////////////////////////////////////3DV小作业

    /**/
    
    // 最后平移到目标
    assert(shift.w == 0 && "shift is vector, but it's w not 0");   // 向量齐次坐标w为0
    rotmovMat(0, 3) = pos.x + shift.x,
    rotmovMat(1, 3) = pos.y + shift.y,
    rotmovMat(2, 3) = pos.z + shift.z;   
    
    mat4 ModelMat;
    ModelMat = rotmovMat * moveBack; // 先绕x再绕y后绕z

    modelDirty = false; // 清除脏位
    return ModelMat;
}

mat4
Camera::getViewMat(void)
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
Camera::getProjMat(void)
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
