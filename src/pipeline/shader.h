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

#include "../basic/homocoor.h"
#include "../basic/tgaimage.h"
#include "model.h"
#include "camera.h"

// 抽象着色器接口，定义了“着色器应该能够判断是否渲染、计算fragment的颜色”，但不定义如何计算。
class Shader 
{
public:    
    virtual ~Shader() = default;    // 为接口/抽象基类声明虚构析函数，如果不写这个，删除时就无法定位到具体是哪个实现
                                    // 析构!=delete，delete是先调用析构、再释放内存
                                    // 析构的核心目的是清理资源，比如关文件、断网、释放动态数组等

    virtual Vertex   // vertex-shader
    vertex(const face_obj& f, int idx) = 0;

    virtual void getMVPV(const mat4& viewPortMat) = 0;  // 获取MVPV矩阵

    // fragment-shader
    virtual // 这是虚函数
    std::pair<bool, TGAColor> // 是否渲染、渲染颜色 
    fragment(const vec3_f abg) // 接受重心坐标 
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
    const Model& model;
    const Camera& camera;

    mat4 modelMat, viewMat, projMat, viewPortMat;
    mat4 MVPV;

    std::array<TGAColor, 3> color;

public:
    Vertex vertex(const face_obj& f, int idx) override;
    std::pair<bool, TGAColor> fragment(const vec3_f abg) const override;
    
    void getMVPV(const mat4& viewPortMat) override;
    RandomShader() = default;
    RandomShader(const Model& _model, const Camera& _camera);
};