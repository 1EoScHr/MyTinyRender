/*
此处原有绘制三角形中的探索性代码：自制扫描线渲染法、标准扫描线渲染法，可在lec2的commit记录中找到，以供回顾
*/

/*
此处原有绘制直线中的探索性代码：自制直线算法、标准的DDA算法，可在lec1的commit记录中找到，以供回顾
*/

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
