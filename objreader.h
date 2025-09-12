#pragma once
#include <vector>
#include <string>

struct point_obj // obj文件中点的坐标
{
    float x; // x坐标
    float y; // y坐标
    float z; // z坐标
};

struct face_obj // 组成一个面的三个点索引
{
    int v1, v2, v3;
};

std::pair<std::vector<point_obj>, std::vector<face_obj>> objFileReader(std::string path);