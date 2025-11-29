#pragma once

#include <algorithm>

#include "../basic/homocoor.h"
#include "../basic/tgaimage.h"
#include "vertex.h"

// 抽象着色器接口，定义了“着色器应该能够判断是否渲染、计算fragment的颜色”，但不定义如何计算。
class Shader 
{
public:    
    virtual ~Shader() = default;    // 为接口/抽象基类声明虚构析函数，如果不写这个，删除时就无法定位到具体是哪个实现
                                    // 析构!=delete，delete是先调用析构、再释放内存
                                    // 析构的核心目的是清理资源，比如关文件、断网、释放动态数组等

    virtual std::array<Vertex, 3>   // 获取对应面三角的屏幕几何信息
    getTriVertex(const std::vector<vec4_zf>& v, const face_obj& f) = 0;

    virtual // 这是虚函数
    std::pair<bool, TGAColor> // 是否渲染、渲染颜色 
    fragment(const vec3_f abg) // 接受重心坐标 
    const // 纯计算，只读
    = 0; // 表明这是纯虚函数，所在的类是抽象类，必须由派生类实现
};

/*
    这里要加public来修饰shader，默认为private。两者的区别在于：
    - 想要让外界知道新的类可以当成基类用，选public；
    - 只是想复用基类中的东西，选public。
*/ 
class RandomShader : public Shader
{
public:
    std::array<Vertex, 3> getTriVertex(const std::vector<vec4_zf>& v, const face_obj& f) override
    {
        return {v[f.v1], v[f.v2], v[f.v3]}; // std::array构造时需要两层大括号，但直接返回只需要一层
    }

    std::pair<bool, TGAColor> fragment(const vec3_f abg) const override 
    {
        return {false, getRandomColor()};   // const函数要求内部调用也得是const
    }

};