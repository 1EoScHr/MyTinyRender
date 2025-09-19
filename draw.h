#include "tgaimage.h"
#include <iostream>

// inline函数的定义必须要写在.h文件中
// 这里用优化写法的Bresenham光栅法
inline
void   drawLine(TGAImage& buffer,                   // TGAImage用引用传参还是指针传参？
                int ax, int ay, int bx, int by,     // 效率上两者基本一致，因为引用在底层的实现就是指针；
                                TGAColor color)     // 逻辑上，指针灵活，传递的可能为空，能代表“可选参数”
{                                                   // 而引用必须绑定一个对象，表示这个对象一定有且要修改
    // 前面的准备与DDA无异
    bool is_steep = std::abs(bx - ax) < std::abs(by - ay);
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

    int x_shift = bx - ax, y_shift = std::abs(by - ay);
    int shift2 = ((by-ay)>0) ? 1 : -1;
    int ierror = 0;
    int y = ay;

    for(int x = ax; x <= bx; x ++)
    {
        if(is_steep) // 先绘制，再累积误差
        {
            buffer.set(y, x, color);
        }
        else
        {
            buffer.set(x, y, color);
        }

        ierror += 2 * y_shift;
        y += shift2 * (ierror > x_shift);
        ierror -= 2 * x_shift * (ierror > x_shift);
    }
}
void drawOBJ(std::string path, TGAImage& buffer);
void drawTriangle  (TGAImage& buffer, int ax, int ay, int bx, int by, int cx, int cy, TGAColor color);

inline
bool isNegtiveArea(int ax, int ay, int bx, int by, int cx, int cy) // 一个布尔量，即三角形的有向面积是否为负
{
    /*
    这里与示例不一样，那里是直接返回有向面积，而我的实现直接返回有向面积符号
    示例这么做是因为其要获得重心坐标，然后再根据重心坐标符号判断，这样后面搞纹理等也能用
    我的这是纯为适配本期目标，因为只画三角形、不考虑后面其他渲染目标的话，根据符号就够了（原理见下面的小证明）
    */

    // 任意一点都可以用一个三角形的重心坐标来表示，并且任意一值小于0就能判定这一点位于三角形外
    // 根据一个证明（没看明白的），可知三角形的有向面积与对应的重心坐标成正比，也就是重心坐标为负，对应的有向面积也为负
    // 譬如点P在a、b、c对应的三个重心坐标分别是Area(PBC)/Area(ABC)、Area(APC)/Area(ABC)、Area(ABP)/Area(ABC)，只要一一对应，顺序可变    
    // 又可知向量叉积能够表示对应的平行四边形的有向面积，也就是是三角形有向面积的两倍

    // 所以根据二维向量叉积u✖v就够用了。其等价于二维标量(u_x*v_y - u_y*v_x)
    // u=a->b=(bx-ax, by-ay), v=a->c=(cx-ax, cy-ay)
    // int area2 = (by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx); // 有几种写法
    int area2 = ax*(by-cy) + bx*(cy-ay) + cx*(ay-by);
    return (area2 < 0); // 负为1，非负为0
}