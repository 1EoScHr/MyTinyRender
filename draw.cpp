#include "draw.h"
#include "objreader.h"

#include <math.h>
#include <iostream>

// #define SCALE_RATE (0.95 / 2) // 已废弃

constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor white   = {255, 255, 255, 255};

void drawOBJ(std::string path, TGAImage& buffer)
{
    auto vandf = objFileReader(path);
    std::vector<point_obj>& v = vandf.first;
    std::vector<face_obj>&  f = vandf.second;

    // 已废弃
    //float scale = SCALE_RATE * buffer.width() / v.begin()->x; // 希望最边边的点能缩放到屏幕整体的SCALE_RATE(默认0.95)处，根据比例就可以算出

    std::cout << "绘制中" << std::endl;

    for(auto iter = f.begin(); iter != f.end(); iter ++)
    {
        auto p1 = v[iter->v1], p2 = v[iter->v2], p3 = v[iter->v3];
        auto w = buffer.width()/2;

        auto p1x = std::round((p1.x+1)*w);
        auto p1y = std::round((p1.y+1)*w);
        auto p2x = std::round((p2.x+1)*w);
        auto p2y = std::round((p2.y+1)*w);
        auto p3x = std::round((p3.x+1)*w);
        auto p3y = std::round((p3.y+1)*w);
        
        drawLine(buffer, p1x, p1y, p2x, p2y, red);
        drawLine(buffer, p1x, p1y, p3x, p3y, red);
        drawLine(buffer, p2x, p2y, p3x, p3y, red);
        buffer.set(p1x, p1y, white);
        buffer.set(p2x, p2y, white);
        buffer.set(p3x, p3y, white);
    }
    
    std::cout << "绘制完毕" << std::endl;
}




/*
{
    // 经典的DDA算法，思路很简单，认为要绘制的直线是从左到右、水平偏移大于竖直偏移；
    // 不能保证一定满足条件，但是可以把四个坐标进行一定程度的交换，就能够满足。 

    bool is_steep = std::abs(bx - ax) < std::abs(by - ay); // 如果陡峭，就交换x与y坐标，相当于旋转、变缓
    if(is_steep)
    {
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if(ax > bx) // 如果不是从左到右，那么交换起点终点
    {
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    float k = static_cast<float>(by-ay) / (bx-ax);

    for(int x = ax; x <= bx; x ++)
    {
        if(is_steep) // 如果前面判定是陡峭，那么这里的x与y是反的，绘制时换回来
        {
            buffer.set(std::round(ay + k*(x-ax)), x, color);
        }
        else
        {
            buffer.set(x, std::round(ay + k*(x-ax)), color); // 要加round四舍五入，否则强制截断
        }
    }
}
*/


// my first，粗糙的画直线，但是出于我的直觉
/* 
{
    int x_shift = bx - ax;
    int y_shift = by - ay;
    int steps = std::round(sqrt(x_shift*x_shift + y_shift*y_shift));
    float rate;

    for(int i = 0; i <= steps; i ++)
    {
        rate = static_cast<float>(i) / steps; // cpp风味的类型转换：static_cast<type>(x)，把x当成type来算
        buffer.set(std::round(ax + x_shift*rate), std::round(ay + y_shift*rate), color); // 要加round四舍五入，否则强制截断导致明显的走样
    }
}
*/