#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "objreader.h"

// 该函数已废弃，因为v中的坐标在没有缩放项时不会大于1，之前看到的是vt项中的
inline float OBJ_max_coordinate(std::vector<point_obj> v) // 返回坐标向量中最大的坐标，用于缩放画面
{
    float ret = 0;

    for(const auto& p : v)
    {
        // ret = std::max({ret, std::abs(p.x), std::abs(p.y), std::abs(p.z)}); // 选择元组中最大的一个

        // 下面这样写可以避免创建元组对象，性能强
        if(std::abs(p.x) > ret) {ret = std::abs(p.x);}
        if(std::abs(p.y) > ret) {ret = std::abs(p.y);}
        if(std::abs(p.z) > ret) {ret = std::abs(p.z);}
    }

    std::cout << "max:" << ret << std::endl;
    return ret;
}

std::pair<std::vector<point_obj>, std::vector<face_obj>> // 暂时只需要顶点坐标、面的组成
objFileReader(std::string path)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        std::cout << "文件打开失败" << std::endl;
        exit(1);
    }
    
    // 要读取的关键信息
    std::vector<point_obj> v; // cpp有类型名字自动提升，相当于直接typedef
    std::vector<face_obj> f;

    std::string linetype; // 读取第一个字符串，判断该行存放的是什么
    float dummy = 0; // 垃圾值

    v.emplace_back(dummy, dummy, dummy); // 用垃圾值填充v的第一项，这样就能让索引对应了

    while (!file.eof())
    {
        file >> linetype;
        // std::cout << linetype << std::endl;
        if(linetype == "v")
        {
            /* // 用更高效的方法替代
            point p;
            file >> p.x >> p.y >> p.z; // cpp的ifstream能够自动按照变量类型解析字符串为数
            v.push_back(p); // 原理是拷贝p到向量的最后，所以不怕循环体            
            */

            v.emplace_back(); // 在末尾构造一个新对象
            file >> v.back().x >> v.back().y >> v.back().z; // 这样能节省一次拷贝开销
        }
        else
        {
            if(linetype == "f")
            {
                std::string triv;
                size_t pos;
                f.emplace_back();
                
                // 这里弄错了面的组成
                /*
                for(uint8_t i = 0; i < 3; i ++)
                {
                    file >> triv; // 包含3个v索引的字符串
                    f.emplace_back();

                    pos1 = triv.find('/'); // find返回从第二个参数往后走，找到第一个参数的索引；如果没有则是std::sting::npos
                    pos2 = triv.find('/', pos1+1);

                    f.back().v1 = std::stoi(triv.substr(0, pos1)); // substr返回从第一个参数往后的第二个参数个字符组成的串
                    f.back().v2 = std::stoi(triv.substr(pos1+1, pos2-pos1-1));
                    f.back().v3 = std::stoi(triv.substr(pos2+1));
                }                
                */

                file >> triv;
                pos = triv.find('/');
                f.back().v1 = std::stoi(triv.substr(0, pos));

                file >> triv;
                pos = triv.find('/');
                f.back().v2 = std::stoi(triv.substr(0, pos));

                file >> triv;
                pos = triv.find('/');
                f.back().v3 = std::stoi(triv.substr(0, pos));
            }

            else
            {
                std::string dummy;
                std::getline(file, dummy);
            }
        }
    }
    
    // v.begin()->x = OBJ_max_coordinate(v); // 悄悄把最大坐标放在索引为0的x坐标处 // 已废弃，用不到

    // c++11提供的特性，结构体、对象这种东西，想返回就直接返回，不用担心作用域
    // 其会自动把函数体内的变量安全搬到返回的地方
    std::cout << "读取.obj文件成功" << std::endl;
    return std::make_pair(v, f);
}

