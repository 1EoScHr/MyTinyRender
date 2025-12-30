/*
    homocoor.h
    意为"homogeneous coordinate"，也就是齐次坐标
    算是本项目的线性代数库
*/

#pragma once
#include <cmath>
#include <cassert>
#include <iostream>
#include <array>

// #include "cuda_math.cuh"    // 转换到GPU辅助
#include "tgaimage.h"

// 前向声明，在最后的矩阵x向量里，要实现3x1(4x1)矩阵与三维(四维)向量的转化，必须要选一个做前向声明
template<int m, int n> struct mat;
template<> struct mat<3, 1>;
template<> struct mat<4, 1>;

// n维向量模板
template<int n> 
struct vec 
{
    // double data[n] = {0};
    std::array<double, n> data = {}; // 使用stl的数组结构，开销一致，但是更现代
    double& operator[](const int i)       { assert(i>=0 && i<n); return data[i]; }  // 操作符[]重载，加上断言防止意外访问
    double  operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }  // 分别是针对引用和非引用，能规定是否可以修改值
    vec<n>& operator+=(const vec<n>& other) // +=重载一般都在类内，因为语义上是对类本身进行更改
    {
        for (int i = 0; i < n; i ++)
        {
            data[i] += other[i];
        }
        return *this;   // 返回当前对象的引用
    }
};

// n维向量输出
template<int n> 
std::ostream& operator<<(std::ostream& out, const vec<n>& v)    // 操作符<<重载，向指定输出流输出n维向量的n个维度值
{
    for (int i = 0; i < n; i ++) 
    {
        out << v[i] << " ";
    }
    return out;
}

// n维向量加法
template<int n>
vec<n> operator+(vec<n> a, const vec<n>& b)  // 重载+，参数const表示运行时不可更改，&表示不是拷贝，高效
{
    a += b;     // 这里的a是一个拷贝，不会修改原来的a；复用+=的逻辑
    return a;
}

// n维向量减法
template<int n>
vec<n> operator-(const vec<n>& a, const vec<n>& b)  // 重载-
{
    vec<n> ret;
    for (int i = 0; i < n; i ++)
    {
        ret[i] = a[i] - b[i];
    }
    return ret;
}

// n维向量点乘
template<int n>
double operator*(const vec<n>& a, const vec<n>& b)  // 重载*(点乘)，参数const表示运行时不可更改，&表示不是拷贝，高效
{
    double ret = 0;
    for (int i = 0; i < n; i ++)
    {
        ret += a[i] * b[i];
    }
    return ret;
}

/*
    下面的vec3使用了模板特化（Template Specialization）语法：
    原本的通用模板vec能表示任意向量，但是由于三维向量会频繁使用，每次都用data[i]来调用太过臃肿，
    我们希望能有一个方法能直接用x、y、z来访问。

    所以使用这种“空模板”的写法：template<> struct vec<3>，意思是
    当模板参数n=3时，不使用通用模板，而用这个专门定义的版本（特化版本），
    这里的template<>是特化的语法要求，必须写，表明这是一个 “完全特化”（针对特定参数值的特殊实现）。

    并且还有一些一致性的设计，譬如虽然特化了vec3，但是其对外的接口依然一致，是[]，依然与其他一样通用。
*/

// template<int m, int n> struct mat;  // vec3中有从mat<3,1>构造来的定义，所以要把下面的mat提前放在这里声明

// 特化：三维向量
template<>
struct vec<3>
{
    double x, y, z; // 在这一特化版本中没有data[i]，而是x、y、z
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    vec<3>& operator+=(const vec<3> other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    vec() = default;                    // 默认构造函数，不传参数时默认选择
    vec(double x, double y, double z);  // 自定义参数构造，这里只声明是因为mat还没有定义完
    vec(const mat<3, 1>& ma);           // 自定义
    // vec<3>(const mat<3, 1>& ma): x(ma(0, 0)), y(ma(1, 0)), z(ma(2, 0)) { } // 这里注意不能这么写，模板要类名统一，同时这里放构造函数声明，因为mat还没有定义
};

// 特化：二维向量
template<>
struct vec<2>
{
    double x = 0, y = 0;
    double& operator[](const int i)       { assert(i>=0 && i<2); return i ? y : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<2); return i ? y : x; }
    vec<2>& operator+=(const vec<2> other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

// 特化：四维向量
template<>
struct vec<4>
{
    double x, y, z, w;
    
    double& operator[](const int i)       { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
    vec<4>& operator+=(const vec<4> other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    vec() = default;
    vec(double _x, double _y, double _z, double _w);
    vec(const mat<4, 1>& ma);
};

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

// 深度缓冲使用，z退化为够用的float
struct vec4_zf
{
    double x, y;
    float z;
    double w;

    vec4_zf() = default;    // 在zbuffer的初始化里，调用了resize，其需要用默认构造函数
    vec4_zf(vec4 v) : x(v.x), y(v.y), z(static_cast<float>(v.z)), w(v.w) { }    // 构造实现要与声明一致
    // 用到的时候都是来接一个mat<4,1>的，如果不加一个直接的转换接口，就会多一次开销
    vec4_zf(const mat<4, 1>& ma);
};

// 重心坐标，float精度足够
struct vec3_f
{
    float alpha = 0.f;
    float beta = 0.f;
    float gamma = 0.f;
};

// 纹理坐标，float精度足够
struct vec2_f
{
    float u = 0.f;
    float v = 0.f;
};

struct vec3f
{
    float x = 0.f;  // DEBUG:惨痛教训，即使有默认构造，其也不会自动设为0！
    float y = 0.f;
    float z = 0.f;

    float& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }   // 保留[]接口，保持一致性
    float  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }

    vec3f& operator+=(const vec3f other)    // 沿袭“利用+=重载”的思路
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    vec3f& operator-=(const vec3f other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    vec3f& operator*=(float i)
    {
        x *= i;
        y *= i;
        z *= i;
        return *this;
    }

    vec3f() = default;
    vec3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) { }
    vec3f(const TGAColor& bgra) : x(static_cast<float>(bgra[2])), y(static_cast<float>(bgra[1])), z(static_cast<float>(bgra[0])) { }
    vec3f(const vec4_zf& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(v.z) { }
    vec3f(const vec4& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)) { }
};

inline vec3f 
operator+(vec3f a, const vec3f& b)
{
    a += b;
    return a;
};

inline vec3f 
operator-(vec3f a, const vec3f& b)
{
    a -= b;
    return a;
};

inline vec3f 
operator*(vec3f v, float f)
{
    v *= f;
    return v;
};

inline vec3f
operator*(float f, vec3f v)
{
    v *= f;
    return v;
}

inline float 
squareMod(vec3f v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

// 点乘的重载版，下面还有个函数版，虽然不一定有用……
// 据称，并不推荐这么重定义，老实用dot吧
/*
float operator*(vec3f a, vec3f b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
*/

// 以下是照猫画虎，实现一个简陋的矩阵

// m*n矩阵模板
template<int m, int n>
struct mat
{
    //double data[m*n] = {0};
    std::array<double, m*n> data = {};  // 同样这里使用stl数组
    double& operator()(const int i, const int j)       { assert(i>=0 && i<m && j>=0 && j<n); return data[i*n+j]; }  // 不要写反成i*m+j了
    double  operator()(const int i, const int j) const { assert(i>=0 && i<m && j>=0 && j<n); return data[i*n+j]; }

    mat() = default;
};

// 矩阵通用输出
template<int m, int n>
std::ostream& operator<<(std::ostream& out, const mat<m, n>& ma)
{
    for (int i = 0; i < m; i ++)
    {
        for (int j = 0; j < n; j ++)
        {
            out << ma(i, j) << " ";
        }
        out << std::endl;
    }
    return out;
}

// 矩阵乘法实现
template<int p, int q, int r>
mat<p, r> operator*(const mat<p, q>& a, const mat<q, r>& b) // 矩阵乘法
{
    mat<p, r> ret;

    /*
    矩阵乘法的这复杂度基本就定死在这O(n^3)了，或许学界有优化方案，但工程上仍是基于这个上做优化
    除了分块乘法之类的优化，这里剩下需要考虑的就是三层for的顺序：i、k、j该如何安排？
    这是体系结构这部分的一个很经典的点，涉及到缓存友好。
    
    假设现在a与b都是一个巨大的矩阵，但在内存中两者其实都还是一维数组，
    每次访问a与b中的元素(x, y)，会先到缓存中寻找，能够命中的话就很快，是理想状态；
    但如果在缓存中没有命中，则会到内存中找(x, y)，并将其周围的所有数据一并拷贝到缓存中（空间连续访问概率很大），这就很慢，要避免。

    为了优化性能，就要尽可能的保证能命中缓存，尽量减少内存访存，
    这里要知道，缓存内部会分块，每次从内存（或下级缓存）中会搬一块，替换掉最长时间没有用的那一块（LRU策略）
    每当取块时，而由于行优先策略，实际上取的是data[(row*x+y)±size]，是一行
    那么也就是说尽量让x固定、y连续，我们的式子里y是j与k，所以先把i循环放到最外层。

    接下来考虑k、j循环，发现如果把k放到最内层，那么每次访问b时都会miss！
    所以应该把k放到中层、j放到外层。

    这样每次到内存取到的ret、a、b，都能用好久才会下一次miss。

    至于所说的这样能连续访问a的行、b的列，我认为就是不严谨，
    我们直观上计算矩阵乘法的顺序是a的第i行的对应元素与b的i列对应元素相乘求和，
    这样一一算出ret的一个元素，这才叫a按行、b按列，但是这里的写法其实就根本不是，更像是把计算拆开了
    但是反正结果一样，只是让程序更优化，即使和人类思维不同。
    */
    for (int i = 0; i < p; i ++)
    {
        for (int k = 0; k < q; k ++)
        {
            for (int j = 0; j < r; j ++)
            {
                ret(i, j) += a(i, k) * b(k, j);
            }
        }   
    }

    return ret;
}

// m*n矩阵转置
template<int m, int n>
mat<n, m> trans(const mat<m, n>& ma)    // 矩阵求转置的一般写法
{
    mat<n, m> ret = {};

    /*
    但是像矩阵求转置就不能像上面一样搞出缓存友好的操作了，
    因为一个按列，一个按行，天生就冲突
    可以用分块之类的优化
    但是对我们的软光栅而言，这根本不是问题——因为顶多4x4，嘻嘻
    */
    for (int i = 0; i < m; i ++)
    {
        for (int j = 0; j < n; j ++)
        {
            ret(j, i) = ma(i, j);
        }
    }

    return ret;
}

// 特化：方阵转置
template<int n>
void trans(mat<n, n>& ma)   // 针对方阵的转置实现，更高效
{
    for (int i = 0; i < n; i++)
    {
        for (int j = i+1; j < n; j ++)
        {
            std::swap(ma(i, j), ma(j, i));
        }
    }
    return;
}

// 方阵求逆（未完成）
template<int n>
mat<n, n> reverse(const mat<n, n>& ma)
{
    std::cerr << "非3x3矩阵求逆暂未实现 :(" << std::endl;
    std::exit(EXIT_FAILURE);    // cpp与python不一样，退回传的是地址，而不是报错信息
}

// 特化：3x3矩阵求逆
template<>
inline mat<3, 3> reverse(const mat<3, 3>& ma)  // 针对3x3矩阵的专门求逆
{
    double  a = ma(0, 0), b = ma(0, 1), c = ma(0, 2),
            d = ma(1, 0), e = ma(1, 1), f = ma(1, 2),
            g = ma(2, 0), h = ma(2, 1), i = ma(2, 2);
        
    double det = a*(e*i-h*f) - b*(d*i-f*g) + c*(d*h-e*g);
    assert(std::abs(det) > 1e-9);
    det = 1/det;
    
    mat<3, 3> adj;
    adj(0, 0) =  det*(e*i-f*h), adj(0, 1) = det*(-b*i+c*h), adj(0, 2) =  det*(b*f-c*e),
    adj(1, 0) = det*(-d*i+f*g), adj(1, 1) =  det*(a*i-c*g), adj(1, 2) = det*(-a*f+c*d),
    adj(2, 0) =  det*(d*h-e*g), adj(2, 1) = det*(-a*h+b*g), adj(2, 2) =  det*(a*e-b*d);

    return adj;
}

// 特化：3x1矩阵（也就是三维向量）
template<>
struct mat<3, 1>
{
    //double data[m*n] = {0};
    std::array<double, 3> data = {};  // 同样这里使用stl数组
    double& operator()(const int i, const int j)       { assert(i>=0 && i<3 && j==0); return data[i]; }  // 不要写反成i*m+j了
    double  operator()(const int i, const int j) const { assert(i>=0 && i<3 && j==0); return data[i]; }

    mat() = default; // 默认构造函数，因为在矩阵乘法的实现中
    mat(const vec3& v): data{v.x, v.y, v.z} { } // vec<3>到mat<3,1>的转换
};

// 特化：4x1矩阵
template<>
struct mat<4, 1>
{
    std::array<double, 4> data = {};
    double& operator()(const int i, const int j)       { assert(i>=0 && i<4 && j==0); return data[i]; }  // 不要写反成i*m+j了
    double  operator()(const int i, const int j) const { assert(i>=0 && i<4 && j==0); return data[i]; }

    mat() = default; // 默认构造函数
    mat(const vec4& v): data{v.x, v.y, v.z, v.w} { } // vec<4>到mat<4,1>的转换
};

typedef mat<3, 3> mat3;
typedef mat<4, 4> mat4;

struct mat3f
{
    std::array<float, 9> data = {};

    float& operator()(const int i, const int j)       { assert(i>=0 && i<3 && j>=0 && j<3); return data[i*3+j]; }  // 不要写反成i*m+j了
    float  operator()(const int i, const int j) const { assert(i>=0 && i<3 && j>=0 && j<3); return data[i*3+j]; }

    mat3f() = default;
    mat3f(const mat4& m4) 
    {
        data[0] = static_cast<float>(m4(0,0));
        data[1] = static_cast<float>(m4(0,1)); 
        data[2] = static_cast<float>(m4(0,2)); 
        data[3] = static_cast<float>(m4(1,0)); 
        data[4] = static_cast<float>(m4(1,1)); 
        data[5] = static_cast<float>(m4(1,2)); 
        data[6] = static_cast<float>(m4(2,0)); 
        data[7] = static_cast<float>(m4(2,1)); 
        data[8] = static_cast<float>(m4(2,2)); 
    }
};

inline vec3f
operator*(const mat3f& m, const vec3f& v)
{
    vec3f ret;
    
    for (int i = 0; i < 3; i ++)
    {
        for (int k = 0; k < 3; k ++)
        {
            ret[i] += m(i, k) * v[k];
        }   
    }

    return ret;
}

// 内联：获取四维单位矩阵
inline mat4
get1Mat(void)   // 得到四阶单位矩阵
{
    mat4 ret;
    ret(0, 0) = 1;
    ret(1, 1) = 1;
    ret(2, 2) = 1;
    ret(3, 3) = 1;
    return ret;
}

// 内联：齐次坐标规范化
inline void 
uintize(vec<4>& v)    // 齐次坐标规范化
{
    assert(v.w != 0);
    v.x /= v.w;
    v.y /= v.w;
    v.z /= v.w;
    v.w = 1.0;
}

inline void 
uintize(vec4_zf& v)    // 齐次坐标规范化
{
    assert(v.w != 0);
    v.x /= v.w;
    v.y /= v.w;
    v.z /= static_cast<float>(v.w);
    v.w = 1.0;
}

// 向量归一化（in-place写法）
inline void
normalize(vec<4>& v)
{
    assert(v.w == 0);
    double mod = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    v.x /= mod;
    v.y /= mod;
    v.z /= mod;
}

inline void
normalize(vec3f& v)
{
    float mod = 1.f / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    v.x *= mod;
    v.y *= mod;
    v.z *= mod;
}

// 四维齐次坐标向量叉乘（实际为三维）
inline vec4 
cross(vec4 v1, vec4 v2)    // 两向量叉乘
{
    /*
        i    j    k
     v1.x v1.y v1.z
     v2.x v2.y v2.z
    */
    assert(v1.w == 0 && v1.w == 0 && "vec4 cross, but w not 0");
    vec4 ret;
    ret[0] = v1.y*v2.z - v1.z*v2.y;
    ret[1] = v1.z*v2.x - v1.x*v2.z;
    ret[2] = v1.x*v2.y - v1.y*v2.x; // fix bug：属于手误，之前没有发现是因为没有乱动过相机!
    ret[3] = 0;
    return ret;
}

// 三维向量叉乘
inline vec3f
cross(vec3f v1, vec3f v2)
{
    vec3f ret;
    ret[0] = v1.y*v2.z - v1.z*v2.y;
    ret[1] = v1.z*v2.x - v1.x*v2.z;
    ret[2] = v1.x*v2.y - v1.y*v2.x;
    return ret;
}

// 三维向量点乘
inline float
dot(vec3f v1, vec3f v2)
{
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

// 计算半程向量
inline vec3f
halfVec(vec3f v1, vec3f v2)
{
    vec3f ret = v1 + v2;
    float dist = std::sqrt(squareMod(ret));
    if (dist < 1e-8f) return {0.f, 0.f, 0.f};
    ret *= 1.f / dist;
    return ret;
}

// 补齐构造函数
inline vec<3>::vec(double _x, double _y, double _z) : x(_x), y(_y), z(_z) { }   // mat<3,1>已经定义，补充构造函数定义
inline vec<3>::vec(const mat<3,1>& ma) : x(ma(0,0)), y(ma(1,0)), z(ma(2,0)) { }

inline vec<4>::vec(double _x, double _y, double _z, double _w) : x(_x), y(_y), z(_z), w(_w) { }
inline vec<4>::vec(const mat<4,1>& ma) : x(ma(0,0)), y(ma(1,0)), z(ma(2,0)), w(ma(3,0)) { }

inline vec4_zf::vec4_zf(const mat<4,1>& ma) : x(ma(0,0)), y(ma(1,0)), z(static_cast<float>(ma(2,0))), w(ma(3,0)) { }

// n维方阵与n维向量相乘
template<int n>
vec<n> operator*(const mat<n, n>& ma, const vec<n>& v)
{
    return static_cast<vec<n>>(ma * mat<n, 1>(v));
}

