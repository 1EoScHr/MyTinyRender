#include <cuda_runtime.h>

#include <iostream>

#include "realtime.h"
#include "basic/defs.h"
#include "basic/tgaimage.h"
#include "pipeline/camera.h"
#include "pipeline/model.h"
#include "pipeline/shader/shader.h"
#include "pipeline/rasterization.h"
#include "pipeline/rasterization.cuh"
#include "pipeline/shader/shader.cuh"

void uploadShaderConstants(const Shader& shader);

int main(int argc, char** argv) 
{
    constexpr int width  = 1024;
    constexpr int height = 1024;

// setting：渲染模式设置
    bool realTimeMode = false;  // 默认为离线模式
    for (int i = 1; i < argc; i ++) 
    {
        if (std::string(argv[i]) == "--realtime" || std::string(argv[i]) == "-r") 
        {
            realTimeMode = true;
            break;
        }
    }

// init：资源路径、Model、Camera、Shader初始化
    std::string obj = "../resource/obj/african_head/african_head.obj";
    std::string nm  = "../resource/obj/african_head/african_head_nm.tga";
    std::string diff= "../resource/obj/african_head/african_head_diffuse.tga";
    std::string spec= "../resource/obj/african_head/african_head_spec.tga";

    Model model(obj, nm, diff, spec);
    Camera camera(static_cast<float>(width)/height);    // fix：保证这里的宽高比是浮点
    BPShader_GnmDiffSpec shader{};
    const TGAColor& bg = yellow;

    if (!realTimeMode)  // 离线渲染模式 // 保留cpu版本，减小工作量
    {
        // init：输出的TGA文件初始化
        TGAImage framebuffer(width, height, TGAImage::RGB, yellow);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);   // 才发现这里可以直接用灰度图类型，前面都自作聪明用RGB :(

        // init：管线初始化
        Rasterization raster(framebuffer);

        // setting：管线配置
        model.setRotate(M_PI/2, 1);
        // model.setRotate(M_PI/2, 1);
        // model.setPos({0, 0, 0.33, 1});
        // camera.addShift({0, 0, 3, 0});
        // camera.setPos({0, 0, 1, 1});

        raster.setShowZb(true, &zbuffer);
        // raster.setAxis(true);

        // running：管线运行
        raster.renderOBJ(model, camera, shader);

        // output：写入tga文件
        raster.cheese();    // 茄子！
        return 0;
    }

    uploadShaderConstants(shader);  // 上传shader常量
    realtime(width, height, bg, model, camera, shader);
    return 0;
}
