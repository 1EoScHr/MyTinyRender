#include "shader.h"

RandomShader::RandomShader(const Model& _model, const Camera& _camera)
    : model(_model), camera(_camera) { }

void
RandomShader::getMVPV(const mat4& _viewPortMat)
{
    // 模型变换 Model
    modelMat = model.getModelMat();
    // 视图变换 view/Camera
    viewMat = camera.getViewMat();
    // 投影变换 Projection
    projMat = camera.getProjMat();
    // 视口变换 Viewport
    // 传进来的参数就是了
    viewPortMat = _viewPortMat;

    MVPV = viewPortMat * projMat * viewMat * modelMat;
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
    TGAColor c = {color[0][0]*abg.alpha + color[1][0]*abg.beta + color[2][0]*abg.gamma,
                  color[0][1]*abg.alpha + color[1][1]*abg.beta + color[2][1]*abg.gamma,
                  color[0][2]*abg.alpha + color[1][2]*abg.beta + color[2][2]*abg.gamma,
                  color[0][3]*abg.alpha + color[1][3]*abg.beta + color[2][3]*abg.gamma};
    return {false, c};
            
   // const函数要求内部调用也得是const
}