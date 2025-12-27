#include <iostream>

#include "realtime.h"
#include "basic/defs.h"
#include "basic/tgaimage.h"
#include "pipeline/camera.h"
#include "pipeline/model.h"
#include "pipeline/shader/shader.h"
#include "pipeline/rasterization.h"

    //                                                  //
    // //                                            // //
    // TODO: 继续完成教程内容 & 模拟GPU裁剪 & 多线程并行// //
    //       & 研究一个支持多模型的接口，能统一处理     // //
    // //                                            // //
    //                                                  //

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
    std::string obj1 = "../resource/obj/diablo3_pose/diablo3_pose.obj";
    std::string obj2_1 = "../resource/obj/african_head/african_head.obj";
    std::string obj2_2 = "../resource/obj/african_head/african_head_eye_inner.obj";

    std::string nm1  = "../resource/obj/diablo3_pose/diablo3_pose_nm.tga";
    std::string nm2_1  = "../resource/obj/african_head/african_head_nm.tga";
    std::string nm2_2  = "../resource/obj/african_head/african_head_eye_inner_nm.tga";

    std::string diff1= "../resource/obj/diablo3_pose/diablo3_pose_diffuse.tga";
    std::string diff2_1= "../resource/obj/african_head/african_head_diffuse.tga";
    std::string diff2_2= "../resource/obj/african_head/african_head_eye_inner_diffuse.tga";

    std::string spec1= "../resource/obj/diablo3_pose/diablo3_pose_spec.tga";
    std::string spec2_1= "../resource/obj/african_head/african_head_spec.tga";
    std::string spec2_2= "../resource/obj/african_head/african_head_eye_inner_spec.tga";

    // Model model1(obj1, nm1, diff1, spec1);
    Model model2_1(obj2_1, nm2_1, diff2_1, spec2_1);
    // Model model2_2(obj2_2, nm2_2, diff2_2, spec2_2);

    Camera camera(static_cast<float>(width)/height);    // fix：保证这里的宽高比是浮点

    // RandomShader shader{};
    // BPShader_Flat shader{};
    // BPShader_Phong shader{};
    // BPShader_GlobalNormalMap shader{};
    BPShader_GnmDiffSpec shader{};

    const TGAColor& bg = yellow;

    if (!realTimeMode)  // 离线渲染模式
    {
        // init：输出的TGA文件初始化
        TGAImage framebuffer(width, height, TGAImage::RGB, yellow);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);   // 才发现这里可以直接用灰度图类型，前面都自作聪明用RGB :(
        
        // init：管线初始化
        Rasterization raster(framebuffer);

        // setting：管线配置
        model2_1.setRotate(M_PI/2, 1);
        // model2_2.setRotate(M_PI/2, 1);
        // model.setPos({0, 0, 0.33, 1});

        // camera.addShift({0, 0, 3, 0});
        // camera.setPos({0, 0, 1, 1});

        raster.setShowZb(true, &zbuffer);
        // raster.setAxis(true);

        // running：管线运行
        raster.renderOBJ(model2_1, camera, shader);
        // raster.renderOBJ(model2_2, camera, shader);
        // raster.renderOBJ(model1, camera, shader);

        // output：写入tga文件
        raster.cheese();    // 茄子！
        return 0;
    }

    realtime(width, height, bg, model2_1, camera, shader);
    return 0;
}
