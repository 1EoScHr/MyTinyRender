#include "../basic/homocoor.h"
#include "../pipeline/model.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

void
Model::objReader(void)
{
    std::cout << "读取.obj文件...";

    std::ifstream file(objFilePath);
    if(!file.is_open())
    {
        std::cout << "\"" << objFilePath << "\"打开失败" << std::endl;
        exit(EXIT_FAILURE);
    }

    // cpp有类型名字自动提升，相当于直接typedef

    std::string line;       // 一整行，鲁棒性强
    std::string linetype;   // 行类型
    double dummy = 0; // 垃圾值

    v.emplace_back(dummy, dummy, dummy, 1); // 用垃圾值填充v的第一项，这样就能让索引对应了
    vn.emplace_back(dummy, dummy, dummy);   // 同样的垃圾值填充vn第一项
    vt.emplace_back(dummy, dummy);   // 同上

    /*
        用eof作为while循环状态是不安全的，因为其是一个事后标志，只能说明上一次读是成功的
        并不能指明下一次读。比如上一次是最后一行，读完后就应退出，可是其并没有返回eof
        这一次再读会读到空，从而引发一些未知错误。

        // while (!file.eof())  
    */
    while (std::getline(file, line))    // getline返回流对象file，能隐式转换为bool，eof等情况就会退出
    {
        // 此时已经从输入流file读一整行到line中，不包括\n
        std::istringstream iss(line);   // 把字符串line当作一个虚拟输入流

        iss >> linetype;        
        if (linetype == "v") // v double1 double2 double3
        {
            // 使用emplace_back直接在末尾创建一个对象，省一次拷贝
            v.emplace_back(); 
            iss >> v.back().x >> v.back().y >> v.back().z;

            // 齐次坐标w值为1，不过这里设成其他值的话，应该就能够在读取文件时控制整个模型的大小
            v.back().w = 1.; 
        }
        else if (linetype == "f")   // f aind1(/bind1/cind1) aind2(/bind2/cind2) aind3(/bind3/cind3)
        {                           // aind1/2/3是几何顶点索引，bind1/2/3是纹理索引，cind1/2/3是顶点法线索引
            size_t pos1, pos2;
            std::string triv;       // "aindx/bindx/cindx"
            f.emplace_back();

            iss >> triv;
            pos1 = triv.find('/');   // 这里使用find方法，从字符串开头，寻找字符/，返回其第一次出现的索引
            pos2 = triv.find('/', pos1 + 1);    // 从第一个'/'后下一个开始找第二个
            f.back().v[0] = std::stoi(triv.substr(0, pos1));
            f.back().vt[0]= std::stoi(triv.substr(pos1 + 1, pos2 - pos1 - 1));
            f.back().vn[0]= std::stoi(triv.substr(pos2 + 1));

            iss >> triv;
            pos1 = triv.find('/');
            pos2 = triv.find('/', pos1 + 1);
            f.back().v[1] = std::stoi(triv.substr(0, pos1));
            f.back().vt[1]= std::stoi(triv.substr(pos1 + 1, pos2 - pos1 - 1));
            f.back().vn[1]= std::stoi(triv.substr(pos2 + 1));

            iss >> triv;
            pos1 = triv.find('/');
            pos2 = triv.find('/', pos1 + 1);
            f.back().v[2] = std::stoi(triv.substr(0, pos1));
            f.back().vt[2]= std::stoi(triv.substr(pos1 + 1, pos2 - pos1 - 1));
            f.back().vn[2]= std::stoi(triv.substr(pos2 + 1));
        }  
        else if (linetype == "vn")  // 读取vertex normal信息
        {
            vn.emplace_back();
            iss >> vn.back().x >> vn.back().y >> vn.back().z;
        }
        else if (linetype == "vt")  // 读取纹理信息
        {
            vt.emplace_back();
            iss >> vt.back().u >> vt.back().v;  // 这样就兼容有0.0占位的情况了
        }
        else
        {
            // 空行、注释行则什么都不做
        }
    }
    
    // c++11提供的特性，结构体、对象这种东西，想返回就直接返回，不用担心作用域
    // 其会自动把函数体内的变量安全搬到返回的地方
    std::cout << "成功" << std::endl;
    return;
}

void
Model::textureReader(void)
{
    if (!nmFilePath.empty()) // 空=不存在
    {
        std::cout << "读取nm贴图文件...";
        
        normalMap = std::make_unique<TGAImage>();   // 等价于new，会调用默认构建
        if (!normalMap->read_tga_file(nmFilePath))  // 读取成功会返回true
        {
            exit(EXIT_FAILURE);
        }

        normalMap->flip_vertically();   // tga文件的原点是在左上角，所以脑子与实际是有一个y轴反转的，否则就出鬼图
    }

    if (!diffFilePath.empty())
    {
        std::cout << "读取diff贴图文件...";
        
        diffMap = std::make_unique<TGAImage>();
        if (!diffMap->read_tga_file(diffFilePath))
        {
            exit(EXIT_FAILURE);
        }

        diffMap->flip_vertically();
    }

    if (!specFilePath.empty())
    {
        std::cout << "读取spec贴图文件...";
        
        specMap = std::make_unique<TGAImage>();
        if (!specMap->read_tga_file(specFilePath))
        {
            exit(EXIT_FAILURE);
        }

        specMap->flip_vertically();
    }
}
