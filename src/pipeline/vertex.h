/*
    vertex.h
    顶点操作，作为pipeline中可编程的一步
*/
#pragma once

#include "../basic/defs.h"
#include "../basic/homocoor.h"

#include <math.h>
#include <vector>
#include <iostream>
#include <algorithm>

class Model
{
public:
    void reader(void);                  // 模型路径
    void setPos(vec4 newpos);           // 设定模型原点在世界坐标位置
    void addShift(vec4 shift);          // 模型从自身原点位移
    void setRotate(double rad, int axis); // 改变某轴上的旋转角度

    const std::vector<vec4>& getVertex(void) const;
    //std::vector<vec4> getVertexCopy(void);    // 不使用此方法，因为大对象栈上传参要多一次复制开销，直接用auto接const T&就能行
    const std::vector<face_obj>& getFace(void) const;
    bool getModelDirty(void) const;

    mat4 getModelMat(void);             // 获取模型变换矩阵
    Model(std::string _path);           // 构造函数

private:
    std::string path;           // 文件路径

    vec4 pos;                   // 模型系坐标原点在世界坐标系的位置
    vec4 shift;                 // 模型
    std::array<double, 3> rotate; // 模型当前绕各轴旋转角度
    bool modelDirty;                 // 模型操作脏位
/*////////////////////////////////////////////////3DV小作业
    vec4 x_axis = {1, 0, 0, 0}; // 模型坐标系，默认与世界坐标系对齐
    vec4 y_axis = {0, 1, 0, 0};
    vec4 z_axis = {0, 0, 1, 0};
////////////////////////////////////////////////3DV小作业*/

    std::vector<vec4> v;        // 顶点坐标信息
    std::vector<face_obj> f;    // 面三角索引
};

class Camera
{
public:
    // view相关
    void setPos(vec4 newPos);       // 直接改变相机位置
    void addShift(vec4 shift);      // 对相机施加位移
    void setGaze(vec4 newGaze);
    void setT(vec4 newT);

    // proj相关
    void setNearAndFar(double newNear, double newFar);
    void setPersp(bool perspective);
    void setFov(double newFov);
    void setAspect(double newAspect);

    // 获取脏位
    bool getViewDirty(void) const;
    bool getProjDirty(void) const;

    // 主功能
    mat4 getViewMat(void);
    mat4 getProjMat(void);
    Camera(double _aspect);

private:
    // 相机位置信息
    vec4 e;             // 相机位置
    vec4 g;             // 相机朝向，gaze
    vec4 t;             // 相机上方
    vec4 shift;

    // 视锥信息
    double fov;         // Y方向上可视角度，可转换为X上
    double aspect;      // 宽高比，一般要与屏幕相同，否则会变形
    double near, far;   // 相机最近/最远能看到的距离
    double left, right;
    double bottom, top;
    bool perspective;       // 是否进行透视投影

    bool viewDirty;         // 相机操作脏位
    bool projDirty;         // 投影操作脏位

    mat4 getPersp2OrthoMat(void);   // 透视转正交
    mat4 getOrthoMat(void);         // 正交压缩为立方
};