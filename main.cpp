#include <iostream>

#include "defs.h"
#include "tgaimage.h"
#include "draw.h"

void vec_test(void)
{
    // vec test
    {
    vec2 a = {-1.8, 2};
    vec2 b = {3.0, 4};
    std::cout   << a+b << std::endl 
                << a-b << std::endl 
                << a*b << std::endl;
    }
    {
    vec3 a = {1.1, -2.0, 3};
    vec3 b = {3, 4.1, -5.2};
    std::cout   << a+b << std::endl 
                << a-b << std::endl 
                << a*b << std::endl;
    }
    {
    vec4 a = {1.1, -2, -3, -4};
    vec4 b = {-5, 6, 7.1, 8.6};
    std::cout   << a+b << std::endl 
                << a-b << std::endl 
                << a*b << std::endl;
    }
}

int main(int argc, char** argv) {

    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

/*
    drawTriangle(framebuffer, {  7, 45, white}, {35, 100,  blue}, {45,  60,yellow});
    drawTriangle(framebuffer, {120, 35,   red}, {90,   5, green}, {45, 110,  blue});
    drawTriangle(framebuffer, {115, 83}, {80,  90}, {85, 120});
*/

    std::string path1 = "../obj/diablo3_pose/diablo3_pose.obj";
    std::string path2 = "../obj/african_head/african_head.obj";

    drawOBJ(path2, framebuffer, zbuffer);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}
