#pragma once
#include <cmath>
#include <cassert>
#include <iostream>

// 创建一个n维向量，以模板结构体的形式
template<int n> 
struct vec 
{
    double data[n] = {0};
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

template<>
struct vec<3>
{
    double x = 0, y = 0, z = 0; // 在这一特化版本中没有data[i]，而是x、y、z
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }   // 保留[]接口，保持一致性
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
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