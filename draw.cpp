#include "draw.h"
#include "objreader.h"
#include "defs.h"

#include <math.h>
#include <iostream>
#include <ranges>
#include <algorithm>

std::pair<std::vector<int>, std::vector<int>> getBbox(int ax, int ay, int bx, int by, int cx, int cy);
std::pair<std::vector<int>, std::vector<int>> getBbox(const Pixel& a, const Pixel& b, const Pixel& c); // Pixel封装

void drawOBJ(std::string path, TGAImage& buffer, TGAImage& zbuffer, Rotate& rot)
{
    // 读取obj中的点、面信息
    auto vandf = objFileReader(path);
    std::vector<vec3>& v = vandf.first;
    std::vector<face_obj>&  f = vandf.second;

    // 获取rot对应的旋转矩阵
    //auto [xrotmat, yrotmat, zrotmat] = getRotMat(rot);
    auto rotmat = getRotMat(rot);

    std::cout << "绘制中" << std::endl;

    // 将所有点旋转
    //for (auto iter = v.begin(); iter != v.end(); iter ++)
    for (auto& iter : v)
    {
        iter = rotmat * iter;
    }

    // 获取z的最大值与最小值，用于给z-buffer的可视黑白确定范围，这样能实现动态分配范围，更优雅
    // 下面这个写法是C++20风味的，十分简洁优美，但是得加一个配置文件让vscode支持cpp20语法
    // 第三个参数是投影参数，告诉编译器不直接比较结构体，而是统一比较投影，是匿名函数[](const &point_obj p){return p.z}的等价简写
    // 返回值是最小值与最大值的point_obj迭代器，可以当指针，->来引出
    auto [minz, maxz] = std::ranges::minmax_element(v, {}, &vec3::z);
    double z_rate = 1.0f / (maxz->z - minz->z);
    std::cout << minz->z << "  " << maxz->z << std::endl;

    for(auto iter = f.begin(); iter != f.end(); iter ++)
    {
        auto p1 = v[iter->v1], p2 = v[iter->v2], p3 = v[iter->v3];
        auto w = buffer.width()/2;

        // auto p1x = std::round((p1.x+1)*w); 
        // round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
        // 使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
        drawTriangle(buffer, zbuffer,
                    {static_cast<int>(std::lround((p1.x+1)*w)), static_cast<int>(std::lround((p1.y+1)*w)), (maxz->z-p1.z)*z_rate, getRandomColor()},
                    {static_cast<int>(std::lround((p2.x+1)*w)), static_cast<int>(std::lround((p2.y+1)*w)), (maxz->z-p2.z)*z_rate, getRandomColor()},
                    {static_cast<int>(std::lround((p3.x+1)*w)), static_cast<int>(std::lround((p3.y+1)*w)), (maxz->z-p3.z)*z_rate, getRandomColor()});
    }
    
    std::cout << "绘制完毕" << std::endl;
}

// 绘制三角形，并计算重心坐标
void drawTriangle  (TGAImage& buffer, TGAImage& zbuffer,
                    Pixel A, Pixel B, Pixel C)
{
    auto bbox = getBbox(A, B, C);
    double s_ABC = computeArea(A, B, C);

    // 背面剔除器，一种优化，原理见drawjusttriangle中的注释
    if(std::signbit(s_ABC)) return;

    // 让编译器把紧跟其后的 for 循环并行化，在多核 CPU 上让不同线程分工执行循环迭代
    #pragma omp parallel for
    for(auto px = bbox.first[0]; px < bbox.second[0]; px ++)
    {
        for(auto py = bbox.first[1]; py < bbox.second[1]; py ++)
        {
            // 计算重心坐标
            double s_PBC = computeArea(Pixel{px,py}, B, C);
            double s_PCA = computeArea(Pixel{px,py}, C, A);
            double s_PAB = computeArea(Pixel{px,py}, A, B);

            double alpha = s_PBC / s_ABC;
            double beta  = s_PCA / s_ABC;
            double gamma = s_PAB / s_ABC;

            // 判断是否在三角形内
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新，这里用方法效率可能比较低
            float d = alpha*A.depth + beta*B.depth + gamma*C.depth; // 计算当前点深度
            if(d > zbuffer.getd(px, py)) continue; // 如果比现有更深，则不画
            auto c = static_cast<std::uint8_t>(255*(1-d));
            zbuffer.set(px, py, {c, c, c, 255});
            
            // 填充正经buffer
            buffer.set(px, py, 
               {static_cast<std::uint8_t>(alpha*A.color[0]+beta*B.color[0]+gamma*C.color[0]),
                static_cast<std::uint8_t>(alpha*A.color[1]+beta*B.color[1]+gamma*C.color[1]),    
                static_cast<std::uint8_t>(alpha*A.color[2]+beta*B.color[2]+gamma*C.color[2]),
                static_cast<std::uint8_t>(alpha*A.color[3]+beta*B.color[3]+gamma*C.color[3])});
        }
    }
}

// 单纯绘制z-buffer深度图
void drawTriangle_zbuffer  (TGAImage& buffer,
                            Pixel A, Pixel B, Pixel C)
{
    auto bbox = getBbox(A, B, C);
    double s_ABC = computeArea(A, B, C);

    #pragma omp parallel for
    for(auto px = bbox.first[0]; px < bbox.second[0]; px ++)
    {
        for(auto py = bbox.first[1]; py < bbox.second[1]; py ++)
        {
            // 计算重心坐标
            double s_PBC = computeArea(Pixel{px,py}, B, C);
            double s_PCA = computeArea(Pixel{px,py}, C, A);
            double s_PAB = computeArea(Pixel{px,py}, A, B);

            double alpha = s_PBC / s_ABC;
            double beta  = s_PCA / s_ABC;
            double gamma = s_PAB / s_ABC;

            // 判断是否在三角形内
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // 绘制
            auto foo = static_cast<std::uint8_t>(255*(alpha*A.depth + beta*B.depth + gamma*C.depth));
            buffer.set(px, py, {foo, foo, foo, 255});
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

// Pixel封装版本
std::pair<std::vector<int>, std::vector<int>>
getBbox(const Pixel& a, const Pixel& b, const Pixel& c) // 获得BoundingBox
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

//std::tuple<mat3, mat3, mat3>
mat3
getRotMat(Rotate& rot)
{
    double sinx = std::sin(rot.x), cosx = std::cos(rot.x);
    double siny = std::sin(rot.y), cosy = std::cos(rot.y);
    double sinz = std::sin(rot.z), cosz = std::cos(rot.z);

    mat3 xrotmat = {}, yrotmat = {}, zrotmat = {};

    // 旋转矩阵特点是绕谁转，谁就不会变，保留原来的值，因此能确定一行；同样的，其他维度旋转就与该轴无关，这样就确定一列
    xrotmat(0, 0) = 1, xrotmat(0, 1) =    0, xrotmat(0, 2) =     0, 
    xrotmat(1, 0) = 0, xrotmat(1, 1) = cosx, xrotmat(1, 2) = -sinx, 
    xrotmat(2, 0) = 0, xrotmat(2, 1) = sinx, xrotmat(2, 2) =  cosx; 

    yrotmat(0, 0) =  cosy, yrotmat(0, 1) = 0, yrotmat(0, 2) = siny, 
    yrotmat(1, 0) =     0, yrotmat(1, 1) = 1, yrotmat(1, 2) =    0, 
    yrotmat(2, 0) = -siny, yrotmat(2, 1) = 0, yrotmat(2, 2) = cosy; 

    zrotmat(0, 0) = cosz, zrotmat(0, 1) = -sinz, zrotmat(0, 2) = 0, 
    zrotmat(1, 0) = sinz, zrotmat(1, 1) =  cosz, zrotmat(1, 2) = 0, 
    zrotmat(2, 0) =    0, zrotmat(2, 1) =     0, zrotmat(2, 2) = 1; 

    // return {xrotmat, yrotmat, zrotmat};
    return zrotmat*yrotmat*xrotmat; // 先x再y再z
}
/*
此处原有绘制三角形中的探索性代码：自制扫描线渲染法、标准扫描线渲染法，可在lec2的commit记录中找到，以供回顾
*/

/*
此处原有绘制直线中的探索性代码：自制直线算法、标准的DDA算法，可在lec1的commit记录中找到，以供回顾
*/
