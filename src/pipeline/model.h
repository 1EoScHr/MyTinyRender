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

class Model
{
public:
    void reader(void);                  // 从模型路径读取
    void setPos(vec4 newpos);           // 设定模型原点在世界坐标位置
    void addShift(vec4 shift);          // 模型从自身原点位移
    void setRotate(double rad, int axis); // 改变某轴上的旋转角度

    vec4 getVertex(int idx) const;
    //std::vector<vec4> getVertexCopy(void);    // 不使用此方法，因为大对象栈上传参要多一次复制开销，直接用auto接const T&就能行
    const std::vector<face_obj>& getFace(void) const;
    bool getModelDirty(void) const;

    mat4 getModelMat(void) const;             // 获取模型变换矩阵
    Model(std::string _path);           // 构造函数

private:
    std::string path;           // 文件路径

    vec4 pos;                   // 模型系坐标原点在世界坐标系的位置
    vec4 shift;                 // 模型
    std::array<double, 3> rotate; // 模型当前绕各轴旋转角度
    mutable bool modelDirty;                 // 模型操作脏位    // 脏位这种逻辑上不属于类的值状态的使用
/*////////////////////////////////////////////////3DV小作业
    vec4 x_axis = {1, 0, 0, 0}; // 模型坐标系，默认与世界坐标系对齐
    vec4 y_axis = {0, 1, 0, 0};
    vec4 z_axis = {0, 0, 1, 0};
////////////////////////////////////////////////3DV小作业*/

    std::vector<vec4> v;        // 顶点坐标信息
    std::vector<face_obj> f;    // 面三角索引
};