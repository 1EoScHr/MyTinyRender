#include "draw.h"
#include "objreader.h"
#include "defs.h"

#include <math.h>
#include <iostream>
#include <ranges>
#include <algorithm>

std::pair<std::vector<int>, std::vector<int>> getBbox(int ax, int ay, int bx, int by, int cx, int cy);
std::pair<std::vector<int>, std::vector<int>> getBbox(const Pixel& a, const Pixel& b, const Pixel& c); // Pixel封装

void drawOBJ(std::string path, TGAImage& buffer, TGAImage& zbuffer)
{
    std::cout << "绘制中" << std::endl;

    // 读取obj中的点、面信息
    auto vandf = objFileReader(path);
    std::vector<vec4>& v = vandf.first;
    std::vector<face_obj>& f = vandf.second;

    mat4 MV, PV, modelMat, viewMat, projMat, viewportMat;
    Model modelInfo;    // 默认参数
    Camera camInfo;     // 默认参数

    /* TODO：这里后期添加模型相关处理逻辑
    */

    // 模型变换 Model
    modelMat = getModelMat(modelInfo);
    // 视图变换 view/Camera
    viewMat = getViewMat(camInfo);

    MV = viewMat * modelMat;   
    for (auto& iter : v)    // 先进行模型、视图变换，此时物体都不会再变化了，z上也固定；至于开销，是可以容忍的
    {
        iter = MV * iter;
        assert(iter.w == 1);
    }

    // 获取z坐标的最大值与最小值，界定near与far，辅助进行透视
    /*
        这个写法是C++20风味的，简洁优美，但得加配置文件让vscode支持cpp20语法
        &vec4::z是投影参数，让编译器不直接比较结构体，而是统一比较投影，是匿名函数[](const &point_obj p){return p.z}的等价简写
        返回值是最小值与最大值的point_obj迭代器，可以当指针，->来引出
    */
    auto [zfar, znear] = std::ranges::minmax_element(v, {}, &vec4::z);
    Frustum fruInfo(M_PI/2, buffer.width()/static_cast<double>(buffer.height()), znear->z, zfar->z);   // 初始化视锥，可视角90度，宽高比与屏幕相关（不相关会让画面拉伸），使用near与far

    /*  TODO：这里后期添加视锥参数处理逻辑
    
    
    */

    // 投影变换 Projection
    projMat = getProjMat(fruInfo);
    // 视口变换 Viewport
    viewportMat = getViewportMat(buffer);

    PV = viewportMat * projMat;
    for (auto& iter : v)        // 再进行投影、视口变换，把东西先映射到[-1,1]^3，再到屏幕区域。
    {
        iter = PV * iter;
        uintize(iter);
    }

    for(auto& iter : f)
    {
        auto p1 = v[iter.v1], p2 = v[iter.v2], p3 = v[iter.v3];
       
        /*
        auto p1x = std::round((p1.x+1)*w); 
        round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
        使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小
        */
        drawTriangle(buffer, zbuffer,
                    {static_cast<int>(std::lround(p1.x)), static_cast<int>(std::lround(p1.y)), p1.z, getRandomColor()},
                    {static_cast<int>(std::lround(p2.x)), static_cast<int>(std::lround(p2.y)), p2.z, getRandomColor()},
                    {static_cast<int>(std::lround(p3.x)), static_cast<int>(std::lround(p3.y)), p3.z, getRandomColor()}, 
                    true);
    }
    
    // 利用三角形绘制坐标轴
    std::array<vec4, 6> origin;
    origin[0] = {-0.05, 0, 0, 1}, origin[1] = {0.05, 0, 0, 1}, 
    origin[2] = {0, -0.05, 0, 1}, origin[3] = {0, 0.05, 0, 1},
    origin[4] = {0, 0, -0.05, 1}, origin[5] = {0, 0, 0.05, 1};
    std::array<vec4, 3> end;
    end[0] = {1.5, 0, 0, 1}, end[1] = {0, 1.5, 0, 1}, end[2] = {0, 0, 1.5, 1};
    
    for (auto& iter : origin)
    {
        iter = PV * viewMat * iter;
        uintize(iter);
    }
    for (auto& iter : end)
    {
        iter = PV * viewMat * iter;
        uintize(iter);
    }
    for (size_t i = 0; i < end.size(); i ++)
    {
    
    drawTriangle(buffer, zbuffer, 
                {origin[0], getRandomColor()}, 
                {origin[1], getRandomColor()}, 
                {end[i], getRandomColor()}, false);
    drawTriangle(buffer, zbuffer, 
                {origin[2], getRandomColor()}, 
                {origin[3], getRandomColor()}, 
                {end[i], getRandomColor()}, false);
    drawTriangle(buffer, zbuffer, 
                {origin[4], getRandomColor()}, 
                {origin[5], getRandomColor()}, 
                {end[i], getRandomColor()}, false);             
    }

    std::cout << "绘制完毕" << std::endl;
}

/*
    这里是navie camera一节中用三维矩阵、向量完成的“MVP变换”，做了不少小巧思，以及疑惑。
    在引入齐次坐标后，修改了框架，这一navie版本不兼容。

void drawOBJ_navie(std::string path, TGAImage& buffer, TGAImage& zbuffer, Rotate& rot, bool perspective = true, double c_pos = 3)
{
    std::cout << "绘制中" << std::endl;

    // 读取obj中的点、面信息
    auto vandf = objFileReader(path);
    std::vector<vec3>& v = vandf.first;
    std::vector<face_obj>&  f = vandf.second;

    // 获取rot对应的旋转矩阵
    auto rotmat = getRotMat(rot);

    // 这里算是模型变换
    // 将模型的所有点应用旋转
    //for (auto iter = v.begin(); iter != v.end(); iter ++)
    for (auto& iter : v)    // 更现代的写法，自动解引用、需要修改容器值
    {
        iter = rotmat * iter;
    }

    // 这一节还没有引入齐次坐标，无法平移等，所以暂时没有视图变换（或者说是一个默认的视图变换）

    // 接下来就是投影变换
    //
    // 获取实际z坐标的最大值与最小值
    // 1.界定near与far，辅助进行透视（如果选择了透视投影），提供任意空间->[-1, 1]^3空间的映射
    // 2.给z-buffer灰度可视化提供深度->灰度的变换，随着模型的具体情况动态分配
    
    // 下面这个写法是C++20风味的，十分简洁优美，但是得加一个配置文件让vscode支持cpp20语法
    // 第三个参数是投影参数，告诉编译器不直接比较结构体，而是统一比较投影，是匿名函数[](const &point_obj p){return p.z}的等价简写
    // 返回值是最小值与最大值的point_obj迭代器，可以当指针，->来引出    

    auto [zfar, znear] = std::ranges::minmax_element(v, {}, &vec3::z);
    // std::cout << zfar->z << "  " << znear->z << std::endl;
    double z_muti = 2.0 / (znear->z - zfar->z); // 把[zfar,znear]的z映射到[-1, 1]: 2/(znear-zfar)*(z-zfar)-1 = 2z/(znear-zfar)-(znear+zfar)/(znear-zfar)
    double z_plus = (znear->z + zfar->z)/(zfar->z - znear->z);

    // 把xy根据z进行透视变换
    if(perspective) // 如果选择了透视投影
    {
        // 本来结合GAMES的内容，透视变换是在near平面上，而非0；
        // double crate = c_pos - znear->z;
        // double crate = c_pos - zfar->z;
        // 可是如上试了试，画面是越靠近越小，再改成far平面，画面是越靠近越大，但大的诡异
        // 原因好解释，画一张图就能看出来，但是为何与预期不一样？

        // 分析了一下，GAMES是要把视锥压缩为单位立方体，然后进行正交投影，是涉及到齐次坐标的
        // 而目前手搓的版本不涉及到这些，仅仅从最直观的角度来做。

        // 最后自然是想到了取0的时候，画一画会发现，在分布均匀的时候，的确是近的部分变大、远的部分变小，很符合直觉，实际效果也满足预期。
        // （不小心搞出来的这些鬼图，深入来说的话可能也代表了我们所期望的“0”平面？在其之前是会放大的，在其后是缩小的，也是不错的）解释。

        // 还有一点不懂：为什么实例中把z也乘了这个系数？按理说现在应该啥都不变的。

        for (auto& iter : v)
        {
            // 这里不太好放到矩阵里，并且在当前情况下属于比较多余
            double mutirate = 1.0 / (1.0 - iter.z/c_pos);
            iter.x *= mutirate;
            iter.y *= mutirate;
        }
    }

    // 这里就是投影变换了
    //for(auto iter = f.begin(); iter != f.end(); iter ++)
    for(auto& iter : f)
    {
        auto p1 = v[iter.v1], p2 = v[iter.v2], p3 = v[iter.v3];
        auto w = buffer.width()/2;

        // auto p1x = std::round((p1.x+1)*w); 
        // round命令返回double(float)，不管是在这里转int还是调入函数默认转换都有额外开销
        // 使用lround命令，其返回long，能省去这一步，尽管在linux下long是64位，但开销也比float小

        drawTriangle(buffer, zbuffer,
                    {static_cast<int>(std::lround((p1.x+1)*w)), static_cast<int>(std::lround((p1.y+1)*w)), p1.z*z_muti+z_plus, getRandomColor()},
                    {static_cast<int>(std::lround((p2.x+1)*w)), static_cast<int>(std::lround((p2.y+1)*w)), p2.z*z_muti+z_plus, getRandomColor()},
                    {static_cast<int>(std::lround((p3.x+1)*w)), static_cast<int>(std::lround((p3.y+1)*w)), p3.z*z_muti+z_plus, getRandomColor()});
    }
    
    std::cout << "绘制完毕" << std::endl;
}
*/

// 计算重心坐标，在原图与zbuffer上绘制三角形
void drawTriangle  (TGAImage& buffer, TGAImage& zbuffer,
                    Pixel A, Pixel B, Pixel C, bool rmBack)
{
    auto bbox = getBbox(A, B, C);
    double s_ABC = computeArea(A, B, C);

    // 背面剔除器，一种优化，原理见drawjusttriangle中的注释
    if (rmBack)
    {
        if (std::signbit(s_ABC)) return;
    }

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

            // 判断是否在三角形内，根据符号位来判断
            //if(std::signbit(alpha) == std::signbit(beta) && std::signbit(beta) == std::signbit(gamma))
            if(std::signbit(alpha) || std::signbit(beta) || std::signbit(gamma))
                continue;

            // z-buffer更新，这里是基于灰度图的方式，感觉还能再优化？
            double d = alpha*A.depth + beta*B.depth + gamma*C.depth; // 计算当前点深度
            if(d < zbuffer.getdepth(px, py)) continue; // 如果比现有更深，则不画，注意z越小、depth越小、越在后
            auto g = static_cast<std::uint8_t>(127.5*(d+1)); // 从[-1, 1]映射到[0, 255]
            zbuffer.set(px, py, {g});
            
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

mat4
getRotMat(double x, int axis)
{
    // 旋转矩阵特点是绕谁转，谁就不会变，保留原来的值，因此能确定一行；同样的，其他维度旋转就与该轴无关，这样就确定一列

    assert(axis >= 0 && axis <=2);  // 0为x轴，1为y轴，2为z轴
    
    double sinx = std::sin(x);
    // double cosx = std::sqrt(1.0 - sinx * sinx);  // 三角恒等式，但会导致cos符号还需额外判断，不如直接用标准库
    double cosx = std::cos(x);  

    mat4 rotmat = {};

    switch (axis)
    {
        case 0:
            rotmat(0, 0) =     1, /*        0        */ /*        0        */ /*        0        */
            /*        0        */ rotmat(1, 1) =  cosx, rotmat(1, 2) = -sinx, /*        0        */
            /*        0        */ rotmat(2, 1) =  sinx, rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 1:
            rotmat(0, 0) =  cosx, /*        0        */ rotmat(0, 2) =  sinx, /*        0        */
            /*        0        */ rotmat(1, 1) =     1, /*        0        */ /*        0        */
            rotmat(2, 0) = -sinx, /*        0        */ rotmat(2, 2) =  cosx; /*        0        */
            break;

        case 2:
            rotmat(0, 0) =  cosx, rotmat(0, 1) = -sinx, /*        0        */ /*        0        */
            rotmat(1, 0) =  sinx, rotmat(1, 1) =  cosx, /*        0        */ /*        0        */
            /*        0        */ /*        0        */ rotmat(2, 2) =     1; /*        0        */
            break;
    }

    rotmat(3, 3) = 1;

    return rotmat;
    
    /*
    xrotmat:                yrotmat:                zrotmat:
        1,     0,      0,    cosy,     0,   siny,   cosz,  -sinz,      0, 
        0,  cosx,  -sinx,       0,     1,      0,   sinz,   cosz,      0, 
        0,  sinx,   cosx;   -siny,     0,   cosy;      0,      0,      1; 
    */
}

mat4
getPersp2OrthoMat(Frustum& fruInfo)
{
    if (fruInfo.perspective)
    {
        /*
            根据相似三角形（cam - near - z）可得到x、y上的映射关系（相机坐标系）：
            x'(y')= x(y) * n/z
            但这时z也会变，未知，就要靠“近平面完全不变”与“远平面只有中点z不变”两个来构造、解方程

            n   0   0   0
            0   n   0   0
            0   0 n+f -nf
            0   0   1   0

            */ 
        mat4 persp2orthoMat;
        persp2orthoMat(0, 0) = fruInfo.near;
        persp2orthoMat(1, 1) = fruInfo.near;
        persp2orthoMat(2, 2) = fruInfo.near + fruInfo.far;
        persp2orthoMat(2, 3) = -fruInfo.near * fruInfo.far;
        persp2orthoMat(3, 2) = 1;

        return persp2orthoMat;
    }

    else    // 如果坚持正交则返回单位矩阵
    {
        return get1Mat();
    }
}

mat4
getOrthoMat(Frustum& fruInfo)
{
    mat4 moveBack = get1Mat();  // 把视长方体中心移动到坐标原点
    moveBack(0, 3) = -0.5 * (fruInfo.left + fruInfo.right);
    moveBack(1, 3) = -0.5 * (fruInfo.bottom + fruInfo.top);
    moveBack(2, 3) = -0.5 * (fruInfo.near + fruInfo.far);

    mat4 transMat;  // 从视长方体转换为[-1, 1]^3正立方体
    transMat(0, 0) = 2 / (fruInfo.right - fruInfo.left);
    transMat(1, 1) = 2 / (fruInfo.top - fruInfo.bottom);
    transMat(2, 2) = 2 / (fruInfo.near - fruInfo.far);
    transMat(3, 3) = 1;

    mat4 OrthoMat = transMat * moveBack;    // 先平移回原点再变换
    return OrthoMat;
}

mat4
getModelMat(Model& modelInfo)
{
    /*
        获取齐次坐标版模型变换矩阵，包括旋转与平移
        实际计算时，等效为先平移回原点、再旋转、最后平移到目标处
    */
    mat4 ModelMat;
    
    // 先平移回原点
    mat4 moveBack = get1Mat();
    moveBack(0, 3) = -modelInfo.pos.x;
    moveBack(1, 3) = -modelInfo.pos.y;
    moveBack(2, 3) = -modelInfo.pos.z;

    ////////////////////////////////////////////////3DV小作业

    double angle;
    std::cin >> angle;
    modelInfo.z_rotate = angle / 180 * M_PI; // 绕对应轴转70度
/**/
    // 理解成模型坐标系不与世界坐标系平行，根据view变换的经验，可以增加以下步骤：
    // 把模型平移回原点后，先旋转使模型坐标系与世界坐标系重合，再绕预设的各轴旋转，再旋转使模型坐标系恢复原来的值。
    // 当然这里我简化实现，不追求模型坐标系完全与世界坐标系重合，只要两个的z轴能够重合，就绕z轴转。
    // 所以这里只是进了一步来实现绕任意轴旋转，是从绕世界xyz轴旋转进步到经过模型原点任意一轴旋转，要再进一步变成任意一轴，可能还需要把模型坐标原点移动考虑进来

    // 对于要求的(1, 1, 1)与70度情况

    vec4 targetModelZaxis = {1, 1, 0, 0}; // 若与原轴重合，则就会引起断言错误，就不要闲的没事
    normalize(targetModelZaxis);    // 目标z轴正则化向量值


    // 默认的模型坐标系还和世界坐标系重合，要先让其变成设定的（同样为简化，只让z轴对齐，其他两轴跟着转就行）
    // 我的思路是先绕世界z轴转，把模型z轴转到YOZ平面，再绕世界x轴把模型z轴转到世界z轴（当然也可反着，但这似乎就要顺时针）
    // 复习一下“绕某轴转某度”：就是按右手定则，箭头对向眼睛、逆时针

    // 这里选用的三角函数反解也有说法，为了精确的sin、cos符号，把求旋转矩阵的入参改成了弧度
    // 第一步里，操作是把轴投影到在XOY平面上，所以其有可能分布在四个象限，用atan不精确，而atan2则接收x和y的值，刚好；
    // 第二步里，已经把轴移到投影刚好在+y轴上，所以旋转角度只是0-pi，这时刚好用acos解算就没问题。
    mat4 alignRotate1 = getRotMat(std::atan2(targetModelZaxis.y, targetModelZaxis.x), 2);   // 目标轴先绕z轴转到YOZ平面
    mat4 alignRotate2 = getRotMat(std::acos(targetModelZaxis.z), 0);    // 目标轴再绕x轴转到z轴

    // 逆旋转矩阵是旋转矩阵的转置
    trans(alignRotate2);    // z轴转到ZOY面上
    trans(alignRotate1);    // 继续转到目标z轴

    // 模型坐标系的值
    mat4 transTarget = alignRotate1 * alignRotate2;
    assert(std::abs((transTarget * modelInfo.z_axis).z - targetModelZaxis.z) < 1e-6);
    modelInfo.x_axis = transTarget * modelInfo.x_axis;
    modelInfo.y_axis = transTarget * modelInfo.y_axis;
    modelInfo.z_axis = transTarget * modelInfo.z_axis;
    assert(std::abs(modelInfo.z_axis.x - targetModelZaxis.x) < 1e-6);

    // 旋转矩阵的每一行，都是目标空间基向量在当前空间的表示
    mat4 alignRotate;
    alignRotate(0, 0) = modelInfo.x_axis.x, alignRotate(0, 1) = modelInfo.x_axis.y, alignRotate(0, 2) = modelInfo.x_axis.z, 
    alignRotate(1, 0) = modelInfo.y_axis.x, alignRotate(1, 1) = modelInfo.y_axis.y, alignRotate(1, 2) = modelInfo.y_axis.z, 
    alignRotate(2, 0) = modelInfo.z_axis.x, alignRotate(2, 1) = modelInfo.z_axis.y, alignRotate(2, 2) = modelInfo.z_axis.z, 
    alignRotate(3, 3) = 1; 

    // 反向变换回去
    mat4 transAlignRotate = alignRotate;
    trans(transAlignRotate);

    assert(modelInfo.x_rotate == 0);
    assert(modelInfo.y_rotate == 0);

    ////////////////////////////////////////////////3DV小作业

    // 再旋转
    mat4 xrotmat = getRotMat(modelInfo.x_rotate, 0), 
         yrotmat = getRotMat(modelInfo.y_rotate, 1),
         zrotmat = getRotMat(modelInfo.z_rotate, 2);

    mat4 rotmovMat = zrotmat*yrotmat*xrotmat;

    // 计算完矩阵后，就可以认为已经执行了，只是还没有渲染
    assert(modelInfo.shift.w == 0);   // 向量齐次坐标w为0
    modelInfo.pos += modelInfo.shift;   // 更新坐标
    modelInfo.shift = {0, 0, 0, 0};     // 清零shift

    ////////////////////////////////////////////////3DV小作业
/**/
    mat4 movMat = get1Mat();
    movMat(0, 3) = modelInfo.pos.x;
    movMat(1, 3) = modelInfo.pos.y;
    movMat(2, 3) = modelInfo.pos.z;

    ModelMat = movMat * transAlignRotate * rotmovMat * alignRotate * moveBack; 
    // 先移动回原点，再旋转使模型坐标系对应世界坐标系，此时再绕z轴旋转，再恢复原本的坐标系，再移动到目标地点

    ////////////////////////////////////////////////3DV小作业

    /*
    
    // 最后平移到目标
    rotmovMat(0, 3) = modelInfo.pos.x,
    rotmovMat(1, 3) = modelInfo.pos.y,
    rotmovMat(2, 3) = modelInfo.pos.z;   
    
    ModelMat = rotmovMat * moveBack; // 先绕x再绕y后绕z

    */
   
    return ModelMat;
}

mat4
getViewMat(Camera& camInfo)
{
    // 这里要做的其实是把世界坐标系与相机坐标系对齐，想想相机带着一堆东西，平移，旋转，和世界坐标系对齐
    mat4 ViewMat;
    mat4 shift = get1Mat(), rotate;

    // 先获取平移回原点矩阵
    shift(0, 3) = -camInfo.e.x;
    shift(1, 3) = -camInfo.e.y;
    shift(2, 3) = -camInfo.e.z;

    /*
        再获取摆正相机旋转矩阵，从“旋转矩阵其实就是基坐标变换”的角度比较好理解：
        旋转矩阵的每一行都是目标坐标系的基向量在当前坐标系中的表示
        比如单位矩阵之所以代表“1”，就是其做了一个从当前坐标系到当前坐标系的映射

        而我们的g、t、gxt都是单位向量，可以直接代入
    */
    vec4 gxt = cross(camInfo.g, camInfo.t); // 单位向量，相互垂直，结果的模就还是1

    rotate(0, 0) = gxt.x,       rotate(0, 1) = gxt.y,       rotate(0, 2) = gxt.z, 
    rotate(1, 0) = camInfo.t.x, rotate(1, 1) = camInfo.t.y, rotate(1, 2) = camInfo.t.z, 
    rotate(2, 0) =-camInfo.g.x, rotate(2, 1) =-camInfo.g.y, rotate(2, 2) =-camInfo.g.z, 
    rotate(3, 3) = 1; 


    ViewMat = rotate * shift; // 先移回原点，再旋转对齐，很直观

    return ViewMat;
}

mat4
getProjMat(Frustum& fruInfo)
{
    /*
        投影变换还没懂的地方：
        1.视锥按理来说是与cam同轴的，为何平移时的写法似乎是还考虑到视锥中心不在z轴上
        2.视锥的可视角度是基于远平面定义的，从这个角度看，近平面似乎就是一个成像的平面，近平面的长宽就是起到裁切的作用
          但是从这个角度看，用相机位置与近平面就完全能够得到最大可视角度的限制，此时若设定的可视角在此范围内可以理解为裁切已经成的像
          但超过范围外（也就是定义的视锥比物理法则决定的视锥大）时，在两集合差集的物体从物理上看是不会成像的，但视锥压缩却能做到这一点。
          这一块应该怎么处理。
          看了下当时的笔记，fov最后还是由n与t定义，可依旧没有提出解决方法。
          但是再观察变换式，其同样的也没有规定实际的far范围，那么似乎明白了卧槽！
          near和far本身就只是规定了“最近/远能看到哪里”，然后加上一个最后的窗口范围，这样规定near的同时也隐性的划分好了视锥，
          变换矩阵是对所有点变换的，原本在视锥外面的，变换后也仍是在视锥外面，不必担忧。
          
          牛逼！

          可是相机位置怎么影响呢？view变换中已经做了这些！太严丝合缝了nb
    */

    /*
        此时镜头已经对准想要看的东西，接下来把视锥里的投影即可
        若采用透视投影，就是把视锥整个压缩为长方体
        然后利用正交投影把长方体变换到单位立方体内
    */
    mat4 persp2orthoMat = getPersp2OrthoMat(fruInfo);
    mat4 orthoMat = getOrthoMat(fruInfo);
    mat4 ProjMat = orthoMat * persp2orthoMat; // 先把透视的视锥变换为正交的长方体，再缩放到[-1, 1]^3

    return ProjMat;
}

mat4
getViewportMat(const TGAImage& buffer)
{
    mat4 ViewportMat;
    ViewportMat(0, 0) = buffer.width() / 2;
    ViewportMat(0, 3) = buffer.width() / 2;
    ViewportMat(1, 1) = buffer.height()/ 2;
    ViewportMat(1, 3) = buffer.height()/ 2;
    ViewportMat(2, 2) = 1;
    ViewportMat(3, 3) = 1;
    return ViewportMat;
}
/*
此处原有绘制三角形中的探索性代码：自制扫描线渲染法、标准扫描线渲染法，可在lec2的commit记录中找到，以供回顾
*/

/*
此处原有绘制直线中的探索性代码：自制直线算法、标准的DDA算法，可在lec1的commit记录中找到，以供回顾
*/
