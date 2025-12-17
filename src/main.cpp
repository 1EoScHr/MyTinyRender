#include <iostream>

#include "basic/defs.h"
#include "basic/tgaimage.h"
#include "pipeline/camera.h"
#include "pipeline/model.h"
#include "pipeline/shader/shader.h"
#include "pipeline/rasterization.h"

    //                                   //
    // //                             // //
    // TODO: ??????????????????????????? //
    // //                             // //
    //                                   //

int main(int argc, char** argv) {

// output：TGA文件初始化
    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB, yellow);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); // 才发现这里可以直接用灰度图类型，前面都自作聪明用RGB :(

// input：模型路径初始化
    std::string path1 = "../resource/obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../resource/obj/african_head/african_head.obj";
    std::string path3 = "../resource/obj/bunny/bunny.obj";

// init：管线初始化
    Model model(path2);
    Camera camera(width/height);
    // RandomShader shader(model, camera);
    BPShader_Flat shader(model, camera);
    // BPShader_Phong shader(model, camera);
    Rasterization raster(framebuffer);

// setting：管线配置
    model.setRotate(M_PI/2, 1);
    // model.setPos({0, 0, -1, 1});

    // camera.addShift({0, 0, 3, 0});
    // camera.setPos({0, 0, 1, 1});

    raster.setShowZb(true, &zbuffer);
    // raster.setAxis(true);

// running：管线运行
    raster.renderOBJ(model, camera, shader);
    raster.cheese();    // 茄子！

    return 0;
}
