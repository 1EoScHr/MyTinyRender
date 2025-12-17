#include "../basic/homocoor.h"
#include "../pipeline/model.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

void
Model::reader(void)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        std::cout << "文件\"" << path << "\"打开失败" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // 要读取的关键信息
    // std::vector<vec4> v; // cpp有类型名字自动提升，相当于直接typedef
    // std::vector<face_obj> f;

    std::string linetype; // 读取第一个字符串，判断该行存放的是什么
    double dummy = 0; // 垃圾值
    v.emplace_back(dummy, dummy, dummy, 1); // 用垃圾值填充v的第一项，这样就能让索引对应了
    vn.emplace_back(dummy, dummy, dummy);   // 同样的垃圾值填充vn第一项

    while (!file.eof())
    {
        file >> linetype;
        // std::cout << linetype << std::endl;
        
        if (linetype == "v") // v double1 double2 double3
        {
            // 使用emplace_back直接在末尾创建一个对象，省一次拷贝
            v.emplace_back(); 
            file >> v.back().x >> v.back().y >> v.back().z;

            // 齐次坐标w值为1，不过这里设成其他值的话，应该就能够在读取文件时控制整个模型的大小
            v.back().w = 1.; 
        }
        else if (linetype == "f")   // f aind1(/bind1/cind1) aind2(/bind2/cind2) aind3(/bind3/cind3)
        {                           // aind1/2/3是几何顶点索引，bxxx未知，cind1/2/3是顶点法线索引
            size_t pos1, pos2;
            std::string triv;       // "aindx/bindx/cindx"
            f.emplace_back();

            file >> triv;
            pos1 = triv.find('/');   // 这里使用find方法，从字符串开头，寻找字符/，返回其第一次出现的索引
            pos2 = triv.find('/', pos1 + 1);    // 从第一个'/'后下一个开始找第二个
            f.back().v[0] = std::stoi(triv.substr(0, pos1));
            f.back().vn[0]= std::stoi(triv.substr(pos2 + 1));

            file >> triv;
            pos1 = triv.find('/');
            pos2 = triv.find('/', pos1 + 1);
            f.back().v[1] = std::stoi(triv.substr(0, pos1));
            f.back().vn[1]= std::stoi(triv.substr(pos2 + 1));

            file >> triv;
            pos1 = triv.find('/');
            pos2 = triv.find('/', pos1 + 1);
            f.back().v[2] = std::stoi(triv.substr(0, pos1));
            f.back().vn[2]= std::stoi(triv.substr(pos2 + 1));
        }  
        else if (linetype == "vn")  // 读取vertex normal信息
        {
            vn.emplace_back();
            file >> vn.back().x >> vn.back().y >> vn.back().z;
        }
        else
        {
            std::string dummy;
            std::getline(file, dummy);
        }
        
    }
    
    // c++11提供的特性，结构体、对象这种东西，想返回就直接返回，不用担心作用域
    // 其会自动把函数体内的变量安全搬到返回的地方
    std::cout << "读取.obj文件成功" << std::endl;
    return;
}
