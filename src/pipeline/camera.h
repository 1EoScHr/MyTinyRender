/*
    camera.h & model.h
    两者负责存储几何、投影等配置信息，只是MVP矩阵要交给shader，而不是放到管线里
*/

#pragma once

#include "../basic/defs.h"
#include "../basic/homocoor.h"

#include <math.h>
#include <vector>
#include <iostream>
#include <algorithm>

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
    mat4 getViewMat(void) const;
    mat4 getProjMat(void) const;
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

    mutable bool viewDirty;         // 相机操作脏位
    mutable bool projDirty;         // 投影操作脏位

    mat4 getPersp2OrthoMat(void) const;   // 透视转正交
    mat4 getOrthoMat(void) const;         // 正交压缩为立方
};