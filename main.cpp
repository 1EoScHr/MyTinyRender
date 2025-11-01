#include <iostream>

#include "defs.h"
#include "tgaimage.h"
#include "draw.h"

void zmain(void)
{
    Rotate rot(0, 0, _PI/6);
    vec3 point(1, 1, 1);
    std::cout << point << std::endl;
    std::cout << getRotMat(rot) * point;
}

int main(int argc, char** argv) {

    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

    std::string path1 = "../obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../obj/african_head/african_head.obj";

    // 镜头朝向-z轴，上方是y轴，右方是x轴，
    Rotate rot(0, _PI/2, 0);
    drawOBJ(path1, framebuffer, zbuffer, rot, true, 3);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}
