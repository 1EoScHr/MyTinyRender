#include "shader.h"
#include <cmath>

RandomShader::RandomShader(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) { }

void
RandomShader::getMVP(const mat4& _M, const mat4& _V, const mat4& _P)
{
    MVP = _P * _V * _M;
}

/*
    override是声明标识符，在声明中使用即可，不必在实现中也加，会报错
    std::array构造时需要两层大括号，但直接返回只需要一层
*/

vec4
RandomShader::vertex(const face_obj& f, int idx)
{
    //                                    |  这里用std::get<>方法来提取tuple，第0项是vertex索引
    //                                   \|/
    vec4 v = MVP * model.getVertex(std::get<0>(f[idx]));  // 从model的v中获取原始vertex，然后立刻用MVPV变换，得到裁剪空间坐标
/*
目前就只完成了固定的变换，如果有其他想做的可以继续写下去，这就是vertex shader的用法
*/
    color[idx] = getRandomColor();
    return v;   // 返回裁剪坐标，交给GPU进行处理
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
