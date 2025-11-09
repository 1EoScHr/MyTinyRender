#include <iostream>

#include "defs.h"
#include "tgaimage.h"
#include "draw.h"


int main(int argc, char** argv) {

    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); // 才发现这里可以直接用灰度图类型，前面都自作聪明用RGB :(

    std::string path1 = "../obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../obj/african_head/african_head.obj";

    drawOBJ(path2, framebuffer, zbuffer);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}
