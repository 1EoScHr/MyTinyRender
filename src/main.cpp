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

// init：管线初始化
    Model model1(obj2_1, nm2_1, diff2_1, spec2_1);
    Model model2(obj2_2, nm2_2, diff2_2, spec2_2);
    
    Camera camera(width/height);

    // RandomShader shader{};
    // BPShader_Flat shader{};
    // BPShader_Phong shader{};
    // BPShader_GlobalNormalMap shader{};
    BPShader_GnmDiffSpec shader{};

    Rasterization raster(framebuffer);

// setting：管线配置
    model1.setRotate(M_PI/2, 1);
    model2.setRotate(M_PI/2, 1);
    
    // model.setPos({0, 0, 0.33, 1});
    // camera.addShift({0, 0, 3, 0});
    // camera.setPos({0, 0, 1, 1});

    raster.setShowZb(true, &zbuffer);
    // raster.setAxis(true);

// running：管线运行
    raster.renderOBJ(model1, camera, shader);
    raster.renderOBJ(model2, camera, shader);

    raster.cheese();    // 茄子！

    return 0;
}
