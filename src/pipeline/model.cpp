#include "../basic/defs.h"
#include "../basic/homocoor.h"
#include "model.h"
#include "rasterization.h"

#include <math.h>
#include <iostream>
#include <algorithm>

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

vec4 
Model::getVertex(int idx) const
{
    return v[idx];
}

vec3f
Model::getVertexNormal(int idx) const
{
    return vn[idx];
}

vec2_f
Model::getVertexTexture(int idx) const
{
    return vt[idx];
}

vec3f
Model::getTexture_nm(const vec2_f& uv) const
{
    // 从贴图的对应uv坐标获取RGB值，并转换为法向量

    /*
        //static_cast<int>(std::lround(uv.v * normalMap->height()));
        这个写法会带来隐形bug，当uv为1时，实际上是height，而实际范围应该是[0,height-1]
    */
    int x = std::min(static_cast<int>(std::lround(uv.u * normalMap->width())), normalMap->width() - 1);
    int y = static_cast<int>(std::lround(uv.v * normalMap->height()));

    vec3f ret(normalMap->get(x, y));
    
// 未来可顺手优化成乘法
    ret.x = (2.f * ret.x - 255.f) / 255.f;
    ret.y = (2.f * ret.y - 255.f) / 255.f;
    ret.z = (2.f * ret.z - 255.f) / 255.f;

    return ret;
}

TGAColor
Model::getTexture_diff(const vec2_f& uv) const
{
    int x = std::min(static_cast<int>(std::lround(uv.u * diffMap->width())), diffMap->width() - 1);
    int y = static_cast<int>(std::lround(uv.v * diffMap->height()));

    return diffMap->get(x, y);
}

float
Model::getTexture_spec(const vec2_f& uv) const
{
    int x = std::min(static_cast<int>(std::lround(uv.u * specMap->width())), specMap->width() - 1);
    int y = static_cast<int>(std::lround(uv.v * specMap->height()));

// 未来可顺手优化成乘法
    return specMap->get(x, y).bgra[0] / 255.f;  // spec贴图是一个灰度图，直接除255来映射到[0,1]
}

const std::vector<face_obj>& 
Model::getFace(void) const
{
    return this->f;
}

bool 
Model::getModelDirty(void) const
{
    return this->modelDirty;
}

void 
Model::setPos(vec4 newPos)
{
    assert(newPos.w != 0 && "newPos is position, but it's w = 0");
    pos = newPos;
    modelDirty = true;   // 脏位
    return;
}

void 
Model::addShift(vec4 shift)
{
    assert(shift.w == 0 && "shift is vector, but it's w not 0");
    this->shift = shift; 
    modelDirty = true;
    return;
}

void 
Model::setRotate(double rad, int axis)
{
    assert(axis >= 0 && axis <=2 && "invalid axis");
    rotate[axis] = rad;
    modelDirty = true;
    return;
}

Model::Model(const std::string& _objFilePath, 
             const std::string& _nmFilePath,
             const std::string& _diffFilePath,
             const std::string& _specFilePath) 
    : objFilePath(_objFilePath), nmFilePath(_nmFilePath), diffFilePath(_diffFilePath), specFilePath(_specFilePath),
      pos({0., 0., 0., 1.}), shift({0., 0., 0., 0.}), rotate({0., 0., 0.}), modelDirty(true)  // v、f等没有初始化，会调用默认，也就是创造两个空向量
{
    objReader();
    textureReader();
}

mat4
Model::getModelMat(void) const
{
    /*
        获取齐次坐标版模型变换矩阵，包括旋转与平移
        实际计算时，等效为先把模型平移回模型坐标系原点，再把模型空间与世界空间对齐，再旋转，再恢复
    */
    
    // 先把模型平移回模型空间原点，再让模型坐标系与世界坐标系重合，两个平移操作可以叠加
    // 但是由于.obj默认的原点就在世界坐标系原点，所以只需模型平移回模型空间原点就可
    assert(pos.w != 0 && "pos is position, but it's w = 0");
    mat4 moveBack = get1Mat();
    moveBack(0, 3) = -shift.x;
    moveBack(1, 3) = -shift.y;
    moveBack(2, 3) = -shift.z;

    ////////////////////////////////////////////////3DV小作业：绕指定轴旋转，已落后版本，重启须评估
/*
    double angle;
    std::cin >> angle;
    modelInfo.z_rotate = angle / 180 * M_PI; // 绕对应轴转70度

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
*/
    ////////////////////////////////////////////////3DV小作业

    // 此时再旋转
    mat4 xrotmat = getRotMat(rotate[0], 0), 
         yrotmat = getRotMat(rotate[1], 1),
         zrotmat = getRotMat(rotate[2], 2);

    mat4 rotmovMat = zrotmat*yrotmat*xrotmat;

    // 计算完矩阵后，就可以认为已经执行了，只是还没有渲染

    ////////////////////////////////////////////////3DV小作业
/*
    mat4 movMat = get1Mat();
    movMat(0, 3) = modelInfo.pos.x;
    movMat(1, 3) = modelInfo.pos.y;
    movMat(2, 3) = modelInfo.pos.z;

    ModelMat = movMat * transAlignRotate * rotmovMat * alignRotate * moveBack; 
    // 先移动回原点，再旋转使模型坐标系对应世界坐标系，此时再绕z轴旋转，再恢复原本的坐标系，再移动到目标地点
*/
    ////////////////////////////////////////////////3DV小作业

    /**/
    
    // 最后平移回原来的位置，先把坐标系平移到模型坐标系，再坐标系内平移
    assert(shift.w == 0 && "shift is vector, but it's w not 0");   // 向量齐次坐标w为0
    rotmovMat(0, 3) = pos.x + shift.x,
    rotmovMat(1, 3) = pos.y + shift.y,
    rotmovMat(2, 3) = pos.z + shift.z;   
    
    mat4 ModelMat;
    ModelMat = rotmovMat * moveBack; // 先绕x再绕y后绕z

    modelDirty = false; // 清除脏位
    return ModelMat;
}
