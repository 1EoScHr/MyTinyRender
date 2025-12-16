#include "shader.h"
#include <cmath>

RandomShader::RandomShader(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) { }

void
RandomShader::getMVPV(const mat4& _MV, const mat4& _PV)
{
    MVPV = _PV * _MV;
}

/*
    override是声明标识符，在声明中使用即可，不必在实现中也加，会报错
    std::array构造时需要两层大括号，但直接返回只需要一层
*/

Vertex
RandomShader::vertex(const face_obj& f, int idx)
{
    vec4_zf v = MVPV * model.getVertex(f[idx]);   // 从model的v中获取原始vertex，然后立刻用MVPV变换，得到结果时再降低z轴精度
    uintize(v);
/*
目前就只完成了固定的变换，如果有其他想做的可以继续写下去，这就是vertex shader的用法
*/
    color[idx] = getRandomColor();

    return v;   // return时完成vec4_zf到Vertex的变换 
}

std::pair<bool, TGAColor> 
RandomShader::fragment(const vec3_f abg) const 
{
    TGAColor c = {static_cast<uint8_t>(color[0][0]*abg.alpha + color[1][0]*abg.beta + color[2][0]*abg.gamma),
                  static_cast<uint8_t>(color[0][1]*abg.alpha + color[1][1]*abg.beta + color[2][1]*abg.gamma),
                  static_cast<uint8_t>(color[0][2]*abg.alpha + color[1][2]*abg.beta + color[2][2]*abg.gamma),
                  static_cast<uint8_t>(color[0][3]*abg.alpha + color[1][3]*abg.beta + color[2][3]*abg.gamma)};
    return {false, c};
            
   // const函数要求内部调用也得是const
}

BPShader::BPShader(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) 
{ 
    light = {0.f, 2.f, 2.f};    // 默认光源位置
    light_color = white;        // 默认光源颜色
    model_color = blue;         // 模型底色
    I =  2.1f;                  // 默认光源强度
    Ia = 0.1f;                  // 环境光照强度，用一个小常数
    ka = 1; kd = 1; ks = 1;     // 默认环境光、漫反射、镜面反射系数
}

void
BPShader::getMVPV(const mat4& _MV, const mat4& _PV)
{
    MV = _MV;
    PV = _PV;
}

Vertex
BPShader::vertex(const face_obj& f, int idx)
{ 
    vec4 tmpv = MV * model.getVertex(f[idx]); // 从model的v中获取原始vertex，进行MV变换；此时镜头位于原点，并且世界还没有变形
    uintize(tmpv);

    ver[idx] = tmpv;
    if (idx == 2)   // 凑够三个点了
    {
        normal = cross(ver[1] - ver[0], ver[2] - ver[1]);   // 计算叉乘
        normalize(normal);  // 正则化为单位向量，即为面的法向量

        toWatch = (ver[0] + ver[1] + ver[2]) * -0.333333333;  // -face就是目标面的代表点，多弄个负号是方便用，相机就在原点
        toLight = light + toWatch;  // 由面指向光源，即为光源-面，刚好toWatch是-面
        squareDist = squareMod(toLight); // 面与光源距离平方，球壳模型用

        normalize(toWatch); // toWatch是一个单位向量
    }

    vec4_zf v = PV * tmpv;  // 再补上缺少的PV变换
    uintize(v);

    return v;   // return时完成vec4_zf到Vertex的变换 
}

std::pair<bool, TGAColor> 
BPShader::fragment(const vec3_f abg) const 
{
    /*
        BP模型主要是fragment上的操作
    */
    float diffuse = kd * (I/squareDist) * std::max(0.f, dot(normal, toLight));  // 漫反射项：漫反射系数 * 球壳模型 * 有效光强
    float specular= ks * (I/squareDist) * std::max(0.f, std::pow(dot(halfVec(toWatch, toLight), normal), 50.f));// 镜面反射项：镜面反射系数 * 球壳模型 * 镜面约束
    float ambient = ka * Ia;

    return {false, model_color*diffuse + model_color*ambient + light_color*specular};
}