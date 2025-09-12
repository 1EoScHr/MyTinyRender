#include "tgaimage.h"

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
// 1 32 63 1
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
        // if(ierror > x_shift) // 这里逻辑没问题，但是if会导致流水线预测错误空转，影响性能
        // {                    // 可以更改写法来把if换为其他写法
        //     ierror -= 2 * x_shift;
        //     y += shift2;
        // }

        y += shift2 * (ierror > x_shift);
        ierror -= 2 * x_shift * (ierror > x_shift);
    }
}

void drawOBJ(std::string path, TGAImage& buffer);