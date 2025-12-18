#include "shader.h"
#include <cmath>

BPShader_Flat::BPShader_Flat(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) 
{ 
    light = {0, 2, 2, 1};    // 默认光源位置
    light_color = white;        // 默认光源颜色
    model_color = blue;         // 模型底色
    I =  4.f;                 // 默认光源强度
    Ia = 0.1f;                  // 环境光照强度，用一个小常数
    ka = 1; kd = 1; ks = 1;     // 默认环境光、漫反射、镜面反射系数
}

void
BPShader_Flat::getMVP(const mat4& _M, const mat4& _V, const mat4& _P)
{
    MV = _V * _M;
    light = _V * light;    // DEBUG：light坐标为直观，应为世界坐标，也需要view变换
    uintize(light);
    _light = static_cast<vec3f>(light);
    P = _P;
}

vec4
BPShader_Flat::vertex(const face_obj& f, int idx)
{ 
    vec4 v = MV * model.getVertex(f[idx].first); // 从model的v中获取原始vertex，进行MV变换；此时镜头位于原点，并且世界还没有变形
    uintize(v);

    ver[idx] = static_cast<vec3f>(v);
    if (idx == 2)   // 凑够三个点了
    {
        vec3f normal = cross(ver[1] - ver[0], ver[2] - ver[1]);   // 计算叉乘
        normalize(normal);  // 正则化为单位向量，即为面的法向量

        vec3f toWatch = (ver[0] + ver[1] + ver[2]) * -0.333333333f;  // -face就是目标面的代表点，多弄个负号是方便用，相机就在原点
        vec3f toLight = _light + toWatch;  // 由面指向光源，即为光源-面，刚好toWatch是-面
        float squareDist = squareMod(toLight); // 面与光源距离平方，球壳模型用

        normalize(toWatch); // toWatch是一个单位向量
        normalize(toLight); // DEBUG：忘记给toLight正则化为单位向量

        float diffuse = kd * (I/squareDist) * std::max(0.f, dot(normal, toLight));  // 漫反射项：漫反射系数 * 球壳模型 * 有效光强
        float specular= ks * (I/squareDist) * std::max(0.f, std::pow(dot(halfVec(toWatch, toLight), normal), 100.f));// 镜面反射项：镜面反射系数 * 球壳模型 * 镜面约束
        float ambient = ka * Ia;

        face_color = model_color*diffuse + model_color*ambient + light_color*specular;
    }

    v = P * v;  // 再补上缺少的P变换，是一个裁剪坐标
    return v;
}

std::pair<bool, TGAColor> 
BPShader_Flat::fragment(const vec3_f abg) const 
{
    return {false, face_color};
}

BPShader_Phong::BPShader_Phong(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) 
{ 
    light = {0, 2, 2, 1};    // 默认光源位置
    light_color = white;        // 默认光源颜色
    model_color = blue;         // 模型底色
    I =  4.f;                   // 默认光源强度
    Ia = 0.1f;                  // 环境光照强度，用一个小常数
    ka = 1; kd = 1; ks = 1;     // 默认环境光、漫反射、镜面反射系数
}

void
BPShader_Phong::getMVP(const mat4& _M, const mat4& _V, const mat4& _P)
{
    MV = _V * _M;
    light = _V * light;
    uintize(light);
    _light = static_cast<vec3f>(light);
    P = _P;
    vnMV = static_cast<mat3f>(MV);
}

vec4
BPShader_Phong::vertex(const face_obj& f, int idx)
{ 
    vec4 v = MV * model.getVertex(f[idx].first); // 从model的v中获取原始vertex，进行MV变换；此时镜头位于原点，并且世界还没有变形
    uintize(v);

    vec3f vn = vnMV * model.getVertexNormal(f[idx].second);
    normalize(vn);
    
    ver[idx] = static_cast<vec3f>(v);
    ver_n[idx] = vn;

    v = P * v;  // 再补上P变换，是裁剪坐标
    return v; 
}

std::pair<bool, TGAColor> 
BPShader_Phong::fragment(const vec3_f abg) const 
{
    /*
        这里插值要注意修正投影空间的畸变
        因为重心坐标来自投影空间，不能够直接套在相机空间内
    */

    // 这里的重心坐标是NDC中的，不等同于view中，不能混用
    // 根据关系，是与1/-z成线性关系，可以到.md中的“关于NDC/screen空间到view空间重心坐标逆变换”帮助理解
    // 根据影响力修正完毕后，还要保证重心坐标之和为1，一个线性变换就好。
    float aa = abg.alpha / ver[0].z;
    float bb = abg.beta  / ver[1].z;
    float gg = abg.gamma / ver[2].z;
    float sum = 1 / (aa + bb + gg);

    vec3f normal = (aa*ver_n[0] + bb*ver_n[1] + gg*ver_n[2]) * sum; 
    normalize(normal);  // 重心坐标插值出fragment的法向量，并重新正则化
    vec3f toWatch = (aa*ver[0] + bb*ver[1] + gg*ver[2]) * -sum; // 重心坐标插值出fragment的反坐标
    vec3f toLight = _light + toWatch;
    float squareDist = squareMod(toLight);

    normalize(toWatch);
    normalize(toLight);

    float diffuse = kd * (I/squareDist) * std::max(0.f, dot(normal, toLight));  // 漫反射项：漫反射系数 * 球壳模型 * 有效光强
    float specular= ks * (I/squareDist) * std::max(0.f, std::pow(dot(halfVec(toWatch, toLight), normal), 100.f));// 镜面反射项：镜面反射系数 * 球壳模型 * 镜面约束
    float ambient = ka * Ia;

    return {false, model_color*diffuse + model_color*ambient + light_color*specular};
}
