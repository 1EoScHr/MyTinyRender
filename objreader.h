#pragma once
#include <vector>
#include <string>

struct face_obj // 组成一个面的三个点索引
{
    int v1, v2, v3;
};

std::pair<std::vector<vec4>, std::vector<face_obj>> objFileReader(std::string path);

// obj文件中点的坐标，已被vec3替代
/*
struct point_obj 
{
    float x; // x坐标
    float y; // y坐标
    float z; // z坐标
};
*/