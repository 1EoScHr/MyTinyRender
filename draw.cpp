#include "draw.h"
#include "objreader.h"

#include <math.h>
#include <iostream>
#include <algorithm>

constexpr TGAColor white   = {255, 255, 255, 255};
std::pair<std::vector<int>, std::vector<int>> getBbox(int ax, int ay, int bx, int by, int cx, int cy);

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

        // auto p1x = std::round((p1.x+1)*w); // round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
        auto p1x = std::lround((p1.x+1)*w);   // 使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
        auto p1y = std::lround((p1.y+1)*w);
        auto p2x = std::lround((p2.x+1)*w);
        auto p2y = std::lround((p2.y+1)*w);
        auto p3x = std::lround((p3.x+1)*w);
        auto p3y = std::lround((p3.y+1)*w);
        
        /*
        drawLine(buffer, p1x, p1y, p2x, p2y, red);
        drawLine(buffer, p1x, p1y, p3x, p3y, red);
        drawLine(buffer, p2x, p2y, p3x, p3y, red);
        */
        drawTriangle(buffer, p1x, p1y, p2x, p2y, p3x, p3y, 
                    {static_cast<unsigned char>(std::rand()%256), 
                     static_cast<unsigned char>(std::rand()%256), 
                     static_cast<unsigned char>(std::rand()%256)});

        buffer.set(p1x, p1y, white);
        buffer.set(p2x, p2y, white);
        buffer.set(p3x, p3y, white);
    }
    
    std::cout << "绘制完毕" << std::endl;
}

void drawTriangle  (TGAImage& buffer,
                    int ax, int ay, int bx, int by, int cx, int cy,
                    TGAColor color)
{
    auto bbox = getBbox(ax, ay, bx, by, cx, cy);
    bool isNegtive = isNegtiveArea(ax, ay, bx, by, cx, cy);
    if(isNegtive) return; // 一个粗糙的正反面过滤器，从二维出发，发现朝向一面的线段在屏幕的投影大多都是一个方向
                          // 同样的，对于.obj文件中的三维对象，正面的那些面与背面的那些面三角形的有向面积符号不同
                          // 原因是一般.obj文件中一个三角形面的三个点都是逆时针排序的，但是仅限于正面对着的时候
                          // 可以想象在一张纸上画满三角形，点的顺序都是逆时针排序，其有向面积都是正；
                          // 但是将这张纸卷成圆柱，这是正面的那些不变，有向面积还是正；但是背面那些的投影就变成顺时针
                          // 有向面积为负了，因此能够基本的区分开正反面
                          // 当然，这仅限于简单情况，当很复杂时就会有些错乱了（比如按这个方法画那个头，嘴的表现不好，
                          // 因为相当于大球里套了个小球，会干扰计算），所以这个方法对于简单情况（比如天空盒？？？）好用
                          // 复杂的就考虑其他现代化方法了。

    #pragma omp parallel for // 让编译器把紧跟其后的 for 循环并行化，在多核 CPU 上让不同线程分工执行循环迭代
    for(auto px = bbox.first[0]; px <= bbox.second[0]; px ++) 
    {
        for(auto py = bbox.first[1]; py <= bbox.second[1]; py ++)
        {
            // 尽管根据isNegtiveArea函数中的推导，能得到为负就退出
            // 但是事实上正负还要考虑原本三角形本身的有向面积（因为要除，如果原本也是负，那么负负得正）// 推翻，直接判断是否全部同号
            bool pbc = isNegtiveArea(px, py, bx, by, cx, cy);
            bool pca = isNegtiveArea(px, py, cx, cy, ax, ay); // B：APC->PCA(按照A-B-C-A的循环顺序替换，不交换顺序结果就一样)
            bool pab = isNegtiveArea(px, py, ax, ay, bx, by);// C：ABP->PAB，理由同上
             
            if((pbc == pca)&&(pca == pab)) buffer.set(px, py, color);
        }
    }

    buffer.set(ax, ay, white);
    buffer.set(bx, by, white);
    buffer.set(cx, cy, white);
}

std::pair<std::vector<int>, std::vector<int>>
getBbox(int ax, int ay, int bx, int by, int cx, int cy) // 获得BoundingBox
{
    std::vector<int> lb_bbox, rt_bbox;
    
    auto mm = std::minmax({ax, bx, cx});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);

    mm = std::minmax({ay, by, cy});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);
    
    return std::make_pair(lb_bbox, rt_bbox);
}

/*
inline
void   drawLine(TGAImage& buffer,
                int y, int x1, int x2,
                TGAColor color) // 填充时用，绘制横线
{    
    if(x1 > x2)
    {
        std::swap(x1, x2);
    }

    for(int x = x1; x <= x2; x ++)
    {
        buffer.set(x, y, color);
    }
}

{
    // 标准扫描线渲染法
    if(ay < by) { std::swap(ax, bx); std::swap(ay, by); }
    if(ay < cy) { std::swap(ax, cx); std::swap(ay, cy); }
    if(by < cy) { std::swap(bx, cx); std::swap(by, cy); }

    int x1, x2;
    //if(cy != ay) // cy 一定不等于 ay，否则坍缩成为一条直线，而by是可以与cy、ay相等的，效果就是有个平顶
    
    if(by != ay) // 具体判断原因见下方原理
    for(int y = ay; y >= by; y --) // a->b
    {
        x1 = ax + (cx-ax)*(y-ay)/(cy-ay); // x2 = cx + (ax-cx)*(y-cy)/(ay-cy);
        x2 = ax + (bx-ax)*(y-ay)/(by-ay); // 为何用整数除法？这里有妙处，注意是把k^-1拆开来的
                                          // 先乘，与浮点等价；再除，获得整数，与用浮点除完截取整数部分是一样的
                                          // 如果不先算乘法，精度会丢失
        drawLine(buffer, y, x1, x2, color);
    }
    
    if(cy != by)    
    for(int y = cy; y < by; y ++) // c->b
    {
        x1 = cx + (ax-cx)*(y-cy)/(ay-cy);
        x2 = cx + (bx-cx)*(y-cy)/(by-cy);
        drawLine(buffer, y, x1, x2, color);
    }

    buffer.set(ax, ay, white);
    buffer.set(bx, by, white);
    buffer.set(cx, cy, white);
}
*/


/* // homemade扫描线渲染法
inline 
bool drawLine(TGAImage& buffer,
                int ax, int ay, int bx, int by,
                TGAColor color, std::vector<int>& x_step) // 填充时用
{
    // 前面的准备与DDA无异
    bool is_steep = std::abs(bx - ax) < std::abs(by - ay);
    if(is_steep)
    {
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    bool is_swap = ax > bx;
    if(is_swap) // 如果不是从左到右，那么交换起点终点
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
            x_step.emplace_back(y);
        }
        else
        {
            buffer.set(x, y, color);
            x_step.emplace_back(x);
        }
        
        // bug原因：当不陡峭时，x是绝对不会突变的，所以一个一个写进向量，但是对应的y是在变化的，等不起
        // 这里的解决方法就是y没有变化时，就覆盖，这样就能保证
        // if(ierror > x_shift) { x_step.pop_back(); } // 性能负优化：有if，和之前一样需要优化，见下
        
        ierror += 2 * y_shift; 
        const bool if_shift = ierror > x_shift;
        // 只在非陡峭时适用
        if(!is_steep) { x_step.resize(x_step.size() - !if_shift); } // 减少分支的方法(来自GPT)，在软光栅直线中，if+pop 一定慢于 resize+内联判断 
        y += shift2 * if_shift;
        ierror -= 2 * x_shift * if_shift;
    }

    return (is_swap);
}

void drawTriangle  (TGAImage& buffer, 
                    int ax, int ay, int bx, int by, int cx, int cy,
                    TGAColor color)
{
    // 自制扫描线渲染法
    if(ay < by) // 保证ay >= by >= cy
    {
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    if(ay < cy)
    {
        std::swap(ax, cx);
        std::swap(ay, cy);
    }
    if(by < cy)
    {
        std::swap(bx, cx);
        std::swap(by, cy);
    }

    std::vector<int> a2b_x, a2c_x, b2c_x; // 本以为用bool与初始值就可以描述像素点的位置，但发现要考虑水平上一下走很多距离，bool用不了，干脆上int
    if(drawLine(buffer, ax, ay, bx, by, color, a2b_x)) { std::reverse(a2b_x.begin(), a2b_x.end());} // 发现在又陡峭交换、又左右交换的情况下，实际的绘制是从右到左、从下到上，需要颠倒一下
    if(drawLine(buffer, ax, ay, cx, cy, color, a2c_x)) { std::reverse(a2c_x.begin(), a2c_x.end());} // 上面错了，实践证明，只要有左右交换，就需要颠倒
    if(drawLine(buffer, bx, by, cx, cy, color, b2c_x)) { std::reverse(b2c_x.begin(), b2c_x.end());}

    auto iter_a2b = a2b_x.begin(), iter_a2c = a2c_x.begin();
    for(int y = ay; y > by; y --)
    {
        // drawLine(buffer, y, *(++iter_a2b), *(++iter_a2c), color); // 脑子糊涂了，*(++iter)是先自增
        drawLine(buffer, y, *(iter_a2b++), *(iter_a2c++), color);
    }

    auto iter_b2c = b2c_x.begin();
    for(int y = by; y > cy; y --)
    {
        drawLine(buffer, y, *(iter_b2c++), *(iter_a2c++), color);
    }

    buffer.set(ax, ay, white);
    buffer.set(bx, by, white);
    buffer.set(cx, cy, white);
}
*/


/*
此处原有绘制直线中的探索性代码：自制直线算法、标准的DDA算法，可在一开始的commit记录中找到，以供回顾
*/
