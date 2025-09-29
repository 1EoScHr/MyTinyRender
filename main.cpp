#include <iostream>

#include "defs.h"
#include "tgaimage.h"
#include "draw.h"

int main(int argc, char** argv) {

    constexpr int width  = 200;
    constexpr int height = 200;
    TGAImage framebuffer(width, height, TGAImage::RGB);


    drawTriangle(framebuffer, {  7, 45, white}, {35, 100,  blue}, {45,  60,yellow});
    drawTriangle(framebuffer, {120, 35,   red}, {90,   5, green}, {45, 110,  blue});
    drawTriangle(framebuffer, {115, 83}, {80,  90}, {85, 120});

/*
    std::string path1 = "../obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../obj/african_head/african_head.obj";

    drawOBJ(path1, framebuffer);
*/
    framebuffer.write_tga_file("triangle.tga");
    return 0;
}
