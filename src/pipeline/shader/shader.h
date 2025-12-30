/*
    shader.h
    现代计算机图形学的shader里，包括了vertex与fragment的shader
     - vertex负责处理原始点，输入顶点原始信息，输出顶点的最终位置、要插值的属性（颜色、法线等） 
     - fragment负责利用相关属性，决定每个像素的颜色

    所以我的原始实现是不精确的
*/

#pragma once

#include <algorithm>
#include <array>

#include "../../basic/homocoor.h"
#include "../../basic/tgaimage.h"
#include "../model.h"
#include "../camera.h"
#include "shader.cuh"
#include "../../basic/cuda_math.cuh"

// 抽象着色器接口，定义了“着色器应该能够判断是否渲染、计算fragment的颜色”，但不定义如何计算。并且是一个“无状态”的。
class Shader 
{
public:    
    virtual ~Shader() = default;    // 为接口/抽象基类声明虚构析函数，如果不写这个，删除时就无法定位到具体是哪个实现
                                    // 析构!=delete，delete是先调用析构、再释放内存
                                    // 析构的核心目的是清理资源，比如关文件、断网、释放动态数组等

    virtual vec4   // vertex-shader
    vertex(const Model& model, const face_obj& f, int idx) = 0;

    virtual void getMVP(const mat4& M, const mat4& V, const mat4& P) = 0;  // 获取MVPV矩阵 // 更新：在实现光照模型时，需要分开，这样写更泛用 // 更新：视口变换不归shader管

    // fragment-shader
    virtual // 这是虚函数
    std::pair<bool, TGAColor> // 是否渲染、渲染颜色 
    fragment(const Model& model, const vec3_f abg) // 接受重心坐标 
    const // 纯计算，只读
    = 0; // 表明这是纯虚函数，所在的类是抽象类，必须由派生类实现
};

/*
    这里要加public来修饰shader，默认为private。两者的区别在于：
    - 想要让外界知道新的类可以当成基类用，选public；
    - 只是想复用基类中的东西，选private。
*/ 

// 最经典的随机渲染器，每个顶点随机分配一个颜色，然后面上的点线性插值
class RandomShader : public Shader
{
private:
    mat4 MVP;  // modelMat, viewMat, projMat
    std::array<TGAColor, 3> color;

public:
    vec4 vertex(const Model& model, const face_obj& f, int idx) override;
    std::pair<bool, TGAColor> fragment(const Model& model, const vec3_f abg) const override;
    
    void getMVP(const mat4& M, const mat4& V, const mat4& P) override;
    RandomShader();
};

// B-P着色模型，Flat Shading形式，最经典，面法向量
class BPShader_Flat : public Shader
{
private:
    mat4 MV, P;    // BP光照模型需要分开

    vec4 light;     // 点光源位置(初始的世界坐标)
    vec3f _light;   // 点光源位置(初始世界坐标在相机系中三维形式坐标，减小计算开销)
    TGAColor model_color;   // 模型底色，现在是单色，以后用材质会改这里
    TGAColor light_color;   // 光照颜色
    float I;        // 光源光照强度
    float Ia;       // 环境光照强度
    float kd, ks, ka;   // 漫反射系数、镜面反射系数、环境光系数

    std::array<vec3f, 3> ver;    // 当前面的三个顶点
    TGAColor face_color;

public:
    vec4 vertex(const Model& model, const face_obj& f, int idx) override;
    std::pair<bool, TGAColor> fragment(const Model& model, const vec3_f abg) const override;
    void getMVP(const mat4& M, const mat4& V, const mat4& P) override;

    void setLight(vec4 _light);
    void setLightColor(TGAColor color);

    BPShader_Flat();
};

// B-P着色模型，Phong Shading形式，实际上是读取vn，更精细
class BPShader_Phong : public Shader
{
protected:  // 子类也能使用
    mat4 MV, P;    // BP光照模型需要分开
    mat3f vnMV;

    vec4 light;    // 点光源位置
    vec3f _light;
    TGAColor model_color;   // 模型底色，现在是单色，以后用材质会改这里
    TGAColor light_color;   // 光照颜色
    float I;        // 光源光照强度
    float Ia;       // 环境光照强度
    float kd, ks, ka;   // 漫反射系数、镜面反射系数、环境光系数

    std::array<vec3f, 3> ver;   // view space里当前面的三个顶点
    std::array<vec3f, 3> ver_n; // view space里当前面各顶点的法向量

public:
    vec4 vertex(const Model& model, const face_obj& f, int idx) override;
    std::pair<bool, TGAColor> fragment(const Model& model, const vec3_f abg) const override;
    void getMVP(const mat4& M, const mat4& V, const mat4& P) override;

    void setLight(vec4 _light);
    void setLightColor(TGAColor color);

    BPShader_Phong();
};

// B-P着色模型，全局法线贴图版本
class BPShader_GlobalNormalMap : public Shader
{
protected:
    mat4 MV, P;    // BP光照模型需要分开
    mat3f vnMV;

    vec4 _light;    // 点光源位置（世界坐标）
    vec3f light;    // 点光源位置（view space）
    
    TGAColor modelBaseColor;   // 模型底色，没有漫反射纹理时候用
    TGAColor lightColor;   // 光照颜色
    float I;        // 光源光照强度
    float Ia;       // 环境光照强度
    float kd, ks, ka;   // 漫反射系数、镜面反射系数、环境光系数

    std::array<vec3f, 3> ver;   // view space里当前面的三个顶点
    // std::array<vec3f, 3> ver_n; // view space里当前面各顶点的法向量 // 全局法向量贴图用不着
    std::array<vec2_f, 3> ver_t;// view space里当前面各顶点对应的纹理坐标
    
public:
    vec4 vertex(const Model& model, const face_obj& f, int idx) override;
    std::pair<bool, TGAColor> fragment(const Model& model, const vec3_f abg) const override;
    void getMVP(const mat4& M, const mat4& V, const mat4& P) override;

    void setLight(vec4 _light);
    void setLightColor(TGAColor color);

    BPShader_GlobalNormalMap();
};

// 仅供 BPShader_GnmDiffSpec 上传 GPU 用（CPU侧结构）
struct BP_Gnm_GPU_CPU
{
    mat4 MV;              // 用于 vnMV（左上角3x3）
    vec3f light_view;     // view space 光源位置（你在 getMVP 里算出来的 light）
    TGAColor lightColor;  // lightColor
    float I, Ia, ka;      // 你 fragment 用到的就这些
};

class BPShader_GnmDiffSpec : public BPShader_GlobalNormalMap
{
public:
    BPShader_GnmDiffSpec();
    std::pair<bool, TGAColor> fragment(const Model& model, const vec3_f abg) const override;
    BP_Gnm_GPU_CPU getGPUConstants() const
    {
        BP_Gnm_GPU_CPU c{};
        c.MV = MV;
        c.light_view = light;   // 已经是 view space
        c.lightColor = lightColor;
        c.I  = I;
        c.Ia = Ia;
        c.ka = ka;
        return c;
    }
    vec3f getViewPos(int i) const { return ver[i]; }
    vec2_f getUV(int i) const { return ver_t[i]; }
};
