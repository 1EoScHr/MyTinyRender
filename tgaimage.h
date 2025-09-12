#pragma once
#include <cstdint>
#include <fstream>
#include <vector>

/*
下面两个成套的宏是结构体内存对齐控制指令，多用于二进制文件头、网络协议、嵌入式寄存器映射等；

结构体的内存对齐特性会导致其占用的实际内存可能会有空隙，正常不影响使用，但是对于上面提到的特殊情况，内存要求十分严格；

所以有了这两个宏来控制对齐：
#pragma pack(push,1)    告诉编译器下面定义的结构体，内存按照1字节来对齐，不要填充间隙，以此保证内存是紧凑的；
#pragma pack(pop)       告诉编译器恢复之前的对齐方式
*/

#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;         // 字段1：图像id字段长度
    std::uint8_t  colormaptype = 0;     // 字段2：颜色映射类型
    std::uint8_t  datatypecode = 0;     // 字段3：图像类型
    std::uint16_t colormaporigin = 0;   // 字段4：颜色表首地址
    std::uint16_t colormaplength = 0;   //       颜色表长度
    std::uint8_t  colormapdepth = 0;    //       颜色表项位数
    std::uint16_t x_origin = 0;         // 字段5：x位置起始位置
    std::uint16_t y_origin = 0;         //       y位置起始位置
    std::uint16_t width = 0;            //       图像宽度
    std::uint16_t height = 0;           //       图像高度
    std::uint8_t  bitsperpixel = 0;     //       像素位深
    std::uint8_t  imagedescriptor = 0;  //       图像描述符
};
#pragma pack(pop)

/*
表示单个像素颜色的结构体。
*/

struct TGAColor {
    std::uint8_t bgra[4] = {0,0,0,0}; // B、G、R、A(透明度)，默认均为0
    std::uint8_t bytespp = 4;   // bytes per piexl，由于上面有默认的BGRA，所以这里配合默认是4

    // 下面两个是对[]的运算符重载，返回对应的引用
    // 根据TGAColor本身的是否const，分别对应下面两个重载，决定返回的引用是否可更改
    std::uint8_t& operator[](const int i) { return bgra[i]; } 
    const std::uint8_t& operator[](const int i) const { return bgra[i]; }
};

/*
TGA图片结构体，封装了一整张tga格式图像
*/

struct TGAImage {
    enum Format { GRAYSCALE=1, RGB=3, RGBA=4 };

    // 下面的TGAImage()是本类的一个构造函数
    // 其有两种实现，一种是正常的按照参数与逻辑构建；
    // 另一种default的写法，告诉编译器什么都不要动，私有变量置野值或空即可（实际上这里有默认值），使得`TGAImage xx;`的写法合法，理由如下：
    // 如果没有构造函数，可以这么写；但现在有了正经的构造函数，没参数就不行；要想保留这种写法，就得显式的把default标出来
    // 其实这里的核心目的就是保证传参与默认都不会得到垃圾值，有妥善的初始化
    // （不喜欢default的话，另一种可行的方式是在构造函数中每一项都加默认值，更灵活。通过默认值的多种实现方式可见cpp之灵活）
    TGAImage() = default;
    TGAImage(const int w, const int h, const int bpp, TGAColor c = {});
    bool  read_tga_file(const std::string filename);
    bool write_tga_file(const std::string filename, const bool vflip=true, const bool rle=true) const;
    void flip_horizontally();
    void flip_vertically();
    TGAColor get(const int x, const int y) const;
    void set(const int x, const int y, const TGAColor &c);
    int width()  const; // const修饰成员函数本身，是用来获取私有变量w、h值的只读接口
    int height() const;
private:
    bool   load_rle_data(std::ifstream &in);        // rle = run length encoding，是一种压缩连续相同像素的方式，tga常用
    bool unload_rle_data(std::ofstream &out) const; // 因此tga中数据是经过压缩的，所以读写前都要进行相应解码才能用
    int w = 0, h = 0;
    std::uint8_t bpp = 0; // bits per pixel
    std::vector<std::uint8_t> data = {};
};