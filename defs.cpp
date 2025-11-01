#pragma once
#include <cmath>
#include <cassert>
#include <iostream>
#include <array>

// 创建一个n维向量，以模板结构体的形式
template<int n> 
struct vec 
{
    // double data[n] = {0};
    std::array<double, n> data = {}; // 使用stl的数组结构，开销一致，但是更现代
    double& operator[](const int i)       { assert(i>=0 && i<n); return data[i]; }  // 操作符[]重载，加上断言防止意外访问
    double  operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }  // 分别是针对引用和非引用，能规定是否可以修改值
};

template<int n> 
std::ostream& operator<<(std::ostream& out, const vec<n>& v)    // 操作符<<重载，向指定输出流输出n维向量的n个维度值
{
    for (int i = 0; i < n; i ++) 
    {
        out << v[i] << " ";
    }
    return out;
}

template<int n>
vec<n> operator+(const vec<n>& a, const vec<n>& b)  // 重载+，参数const表示运行时不可更改，&表示不是拷贝，高效
{
    vec<n> ret;
    for (int i = 0; i < n; i ++)
    {
        ret[i] = a[i] + b[i];
    }
    return ret;
}

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

template<int n>
double operator*(const vec<n>& a, const vec<n>& b)  // 重载*(点乘)），参数const表示运行时不可更改，&表示不是拷贝，高效
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

template<int m, int n> struct mat;  // vec3中有从mat<3,1>构造来的定义，所以要把下面的mat提前放在这里声明

template<>
struct vec<3>
{
    double x = 0, y = 0, z = 0; // 在这一特化版本中没有data[i]，而是x、y、z
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }

    vec() = default;    // 首先增加默认构造函数
    vec(double x, double y, double z);
    vec(const mat<3, 1>& ma);   // 自定义从三行一列矩阵到三维向量的构造
    // vec<3>(const mat<3, 1>& ma): x(ma(0, 0)), y(ma(1, 0)), z(ma(2, 0)) { } // 这里注意不能这么写，模板要类名统一，同时这里放构造函数声明，因为mat还没有定义
};

template<>
struct vec<2>
{
    double x = 0, y = 0;
    double& operator[](const int i)       { assert(i>=0 && i<2); return i ? y : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<2); return i ? y : x; }
};

template<>
struct vec<4>
{
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i)       { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
};

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

// 以下是照猫画虎，实现一个简陋的矩阵

template<int m, int n>
struct mat
{
    //double data[m*n] = {0};
    std::array<double, m*n> data = {};  // 同样这里使用stl数组
    double& operator()(const int i, const int j)       { assert(i>=0 && i<m && j>=0 && j<n); return data[i*n+j]; }  // 不要写反成i*m+j了
    double  operator()(const int i, const int j) const { assert(i>=0 && i<m && j>=0 && j<n); return data[i*n+j]; }

    mat() = default;
};

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

template<int n>
void trans(mat<n, n>& ma)   // 针对方阵的转置实现
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

template<int n>
mat<n, n> reverse(const mat<n, n>& ma)
{
    std::cerr << "非3x3矩阵求逆暂未实现 :(" << std::endl;
    std::exit(EXIT_FAILURE);    // cpp与python不一样，退回传的是地址，而不是报错信息
}

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

typedef mat<3, 3> mat3;
typedef mat<4, 4> mat4;

inline vec<3>::vec(double _x, double _y, double _z) : x(_x), y(_y), z(_z) { }   // mat<3,1>已经定义，补充构造函数定义
inline vec<3>::vec(const mat<3,1>& ma) : x(ma(0,0)), y(ma(1,0)), z(ma(2,0)) { }

template<int n>
vec<n> operator*(const mat<n, n>& rot, const vec<n>& v)
{
    return static_cast<vec<n>>(rot * mat<n, 1>(v));
}