/*
    camera.h & model.h
    两者负责存储几何、投影等配置信息，只是MVP矩阵要交给shader，而不是放到管线里
*/

#pragma once

#include "../basic/defs.h"
#include "../basic/homocoor.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <optional>

class Model
{
public:
    void setPos(vec4 newpos);           // 设定模型原点在世界坐标位置
    void addShift(vec4 shift);          // 模型从自身原点位移 // 但似乎用挺少？
    void setRotate(double rad, int axis);   // 改变某轴上的旋转角度
    mat4 getModelMat(void) const;       // 获取模型变换矩阵
    bool getModelDirty(void) const;

    const vec4& getPos(void)
    {
        return pos;
    }
    const std::array<double, 3>&
    getRot(void)
    {
        return rotate;
    }

    const std::vector<face_obj>& getFace(void) const;
    vec4 getVertex(int idx) const;
    vec3f getVertexNormal(int idx) const;
    vec2_f getVertexTexture(int idx) const;

    vec3f getTexture_nm(const vec2_f& uv) const;
    TGAColor getTexture_diff(const vec2_f& uv) const;
    float getTexture_spec(const vec2_f& uv) const;

    Model() = default;
    Model(const std::string& _objFilePath, 
          const std::string& _nmFilePath =   "",
          const std::string& _diffFilePath = "",
          const std::string& _specFilePath = ""); // 构造函数，如果不写就会默认为空

private:
    void objReader(void);       // 模型.obj文件路径读取
    void textureReader(void);   // 纹理/贴图文件路径读取
    const std::string objFilePath;  // .obj文件路径
    const std::string nmFilePath;   // 法线贴图/纹理文件路径（可选）
    const std::string diffFilePath; // 漫反射颜色纹理文件路径（可选）
    const std::string specFilePath; // 高光纹理文件路径（可选）

    vec4 pos;                   // 模型空间坐标原点在世界空间坐标系的位置
    vec4 shift;                 // 模型在模型空间内的位置
    std::array<double, 3> rotate;   // 模型当前绕各轴旋转角度
    mutable bool modelDirty;    // 模型操作脏位    // 脏位这种逻辑上不属于类的值状态的使用

/*////////////////////////////////////////////////3DV小作业
    vec4 x_axis = {1, 0, 0, 0}; // 模型坐标系，默认与世界坐标系对齐
    vec4 y_axis = {0, 1, 0, 0};
    vec4 z_axis = {0, 0, 1, 0};
////////////////////////////////////////////////3DV小作业*/

    std::vector<face_obj> f;    // 面三角索引
    std::vector<vec4>  v;       // 顶点坐标
    std::vector<vec3f> vn;      // 顶点向量
    std::vector<vec2_f> vt;     // 纹理坐标
    
    std::unique_ptr<TGAImage> normalMap;    // 法线纹理（可选），智能指针，终于用到了，其会自动置空
    std::unique_ptr<TGAImage> diffMap;      // 漫反射颜色纹理（可选）
    std::unique_ptr<TGAImage> specMap;      // 高光纹理（可选）
};
