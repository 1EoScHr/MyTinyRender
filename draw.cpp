#include "draw.h"
#include "objreader.h"
#include "defs.h"

#include <math.h>
#include <iostream>
#include <algorithm>

std::pair<std::vector<int>, std::vector<int>> getBbox(int ax, int ay, int bx, int by, int cx, int cy);
std::pair<std::vector<int>, std::vector<int>> getBbox(const Point& a, const Point& b, const Point& c); // Point封装

void drawOBJ(std::string path, TGAImage& buffer)
{
    auto vandf = objFileReader(path);
    std::vector<point_obj>& v = vandf.first;
    std::vector<face_obj>&  f = vandf.second;

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
        drawJustTriangle(buffer, p1x, p1y, p2x, p2y, p3x, p3y, 
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
                    Point A, Point B, Point C)
{
    auto bbox = getBbox(A, B, C);
    double s_ABC = computeArea(A, B, C);

    #pragma omp parallel for
    for(auto px = bbox.first[0]; px < bbox.second[0]; px ++)
    {
        for(auto py = bbox.first[1]; py < bbox.second[1]; py ++)
        {
            double s_PBC = computeArea(Point{px,py}, B, C);
            double s_PCA = computeArea(Point{px,py}, C, A);
            double s_PAB = computeArea(Point{px,py}, A, B);

            double alpha = s_PBC / s_ABC;
            double beta  = s_PCA / s_ABC;
            double gamma = s_PAB / s_ABC;

            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            buffer.set(px, py, 
               {static_cast<std::uint8_t>(alpha*A.color[0]+beta*B.color[0]+gamma*C.color[0]),
                static_cast<std::uint8_t>(alpha*A.color[1]+beta*B.color[1]+gamma*C.color[1]),    
                static_cast<std::uint8_t>(alpha*A.color[2]+beta*B.color[2]+gamma*C.color[2]),
                static_cast<std::uint8_t>(alpha*A.color[3]+beta*B.color[3]+gamma*C.color[3])});
        }
    }
}

// 绘制纯三角形，不计算重心坐标
void drawJustTriangle  (TGAImage& buffer,
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

// Point封装版本
std::pair<std::vector<int>, std::vector<int>>
getBbox(const Point& a, const Point& b, const Point& c) // 获得BoundingBox
{
    std::vector<int> lb_bbox, rt_bbox;
    
    auto mm = std::minmax({a.x, b.x, c.x});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);

    mm = std::minmax({a.y, b.y, c.y});
    lb_bbox.emplace_back(mm.first);
    rt_bbox.emplace_back(mm.second);
    
    return std::make_pair(lb_bbox, rt_bbox);
}

std::pair<std::vector<int>, std::vector<int>>
getBbox(int ax, int ay, int bx, int by, int cx, int cy) // 获得BoundingBox，格式是first[0]为xmin，second[0]为xmax，同理1为y
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
此处原有绘制三角形中的探索性代码：自制扫描线渲染法、标准扫描线渲染法，可在lec2的commit记录中找到，以供回顾
*/

/*
此处原有绘制直线中的探索性代码：自制直线算法、标准的DDA算法，可在lec1的commit记录中找到，以供回顾
*/
