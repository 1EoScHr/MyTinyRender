#pragma once

#include <iomanip>

#include "basic/tgaimage.h"
#include "pipeline/camera.h"
#include "pipeline/model.h"
#include "pipeline/shader/shader.h"
#include "pipeline/rasterization.h"

void realtime(int width, int height, const TGAColor& bg, Model& model, Camera& camera, Shader& shader);

inline void
p_fps(float fps)
{
    /*
        这里还真是知识盲区，输出流还能这样用：
        输出固定位数的浮点：std::fixed << std::setprecision(n) << float

        实时要求高的输出要及时 std::flush

        回到行首控制字符：\r
    */
    std::cout << std::fixed << std::setprecision(1) << fps << "       " << '\r' << std::flush;
}