#include <iostream>

#include "tgaimage.h"
#include "draw.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order // constexpr：告诉编译器在编译器这个东西就能算出来
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

int main(int argc, char** argv) {

    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);

/*
    drawTriangle(framebuffer,   7, 45, 35, 100, 45,  60,   red);
    drawTriangle(framebuffer, 120, 35, 90,   5, 45, 110, white);
    drawTriangle(framebuffer, 115, 83, 80,  90, 85, 120, green);
*/
    std::string path1 = "../obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../obj/african_head/african_head.obj";

    drawOBJ(path1, framebuffer);

    framebuffer.write_tga_file("triangle.tga");
    return 0;
}
