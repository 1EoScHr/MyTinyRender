#pragma once // 与ifndef define endif作用一样

#include "homocoor.h"
#include "tgaimage.h"

// TGAColor颜色有关
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order // constexpr：告诉编译器在编译器这个东西就能算出来
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

inline 
TGAColor getRandomColor(void) // 获取随机颜色
{
    return {static_cast<unsigned char>(std::rand()%256), 
            static_cast<unsigned char>(std::rand()%256), 
            static_cast<unsigned char>(std::rand()%256),
            255};
}

struct Vertex    // 屏幕顶点信息，只有几何，颜色交给fragment shader获取
{
    int x;
    int y;
    float depth;    

    Vertex() = default;
    Vertex(int _x, int _y, float _depth = 0.f) : x(_x), y(_y), depth(_depth) { }
    /*
        如果使用std::round(xxx)，其会返回double，在这里转int还是调入函数默认转换都有额外开销
        使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
    */
    Vertex(vec4 v): x(static_cast<int>(std::lround(v.x))), y(static_cast<int>(std::lround(std::lround(v.y)))), depth(static_cast<float>(v.z)) { }
    Vertex(vec4_zf v): x(static_cast<int>(std::lround(v.x))), y(static_cast<int>(std::lround(std::lround(v.y)))), depth(v.z) { }

};

struct face_obj // 组成一个面的三个点索引
{
    int v[3];

    int operator[](int i) const
    {
        assert(i>=0 && i<3 && "face_obj's idx not in [0,2]");
        return v[i];
    }
};