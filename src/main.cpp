#include <iostream>

#include "basic/defs.h"
#include "basic/tgaimage.h"
#include "pipeline/vertex.h"
#include "pipeline/rasterization.h"

int main(int argc, char** argv) {

    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); // 才发现这里可以直接用灰度图类型，前面都自作聪明用RGB :(

    std::string path1 = "../resource/obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../resource/obj/african_head/african_head.obj";
    std::string path3 = "../resource/obj/bunny/bunny.obj";

    Model model(path1);
    Camera camera(width/height);
    Rasterization raster(framebuffer, zbuffer, model.getVertexCopy());

    raster.renderOBJ(model, camera);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}
