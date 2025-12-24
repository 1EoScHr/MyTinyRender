#include <iostream>

#include "realtime.h"
#include "basic/defs.h"
#include "basic/tgaimage.h"
#include "pipeline/camera.h"
#include "pipeline/model.h"
#include "pipeline/shader/shader.h"
#include "pipeline/rasterization.h"

#include <SDL2/SDL.h>

void realtime(int width, int height, const TGAColor& bg, Model& model, Camera& camera, Shader& shader)
{
    // init：初始化SDL子系统，表示需要图形/窗口
    if (SDL_Init(SDL_INIT_VIDEO) != 0)  
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

// init：创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "MyTinyrenderer: Real-Time",    // 窗口标题
        SDL_WINDOWPOS_CENTERED,         // x位置，居中
        SDL_WINDOWPOS_CENTERED,         // y位置，居中
        width, height,                  // 宽高(像素)
        SDL_WINDOW_SHOWN                // 标志位，立即显示
    );

// init：创建渲染上下文，用于向window写
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,                     // 关联的窗口
        -1,                         // 显卡索引，-1为默认
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC    // 标志位：使用GPU硬件加速、开启垂直同步 
    );

// init：创建一块GPU纹理内存，存储帧缓冲图像
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,                       // 所属的renderer
        SDL_PIXELFORMAT_BGR24,          // 像素格式
        SDL_TEXTUREACCESS_STREAMING,    // 访问模式，每帧更新
        width, height                   // 纹理尺寸
    );

    TGAImage framebuffer(width, height, TGAImage::RGB, bg);
    Rasterization raster(framebuffer);

    uint32_t lastTime = SDL_GetTicks();
    int frameCount = 0;
    float fps = 0.f;

    vec4 new_model_pos = model.getPos();
    bool pos_dirty = false;
    auto new_model_rot = model.getRot();
    bool rot_xd = false;
    bool rot_yd = false;
    bool rot_zd = false;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event))   // 从事件队列取出一个事件
        {
            if (event.type == SDL_QUIT) running = false;    // 点击叉号会有SDL_QUIT信号
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_w     : new_model_pos.y += 1; pos_dirty = true; break;
                    case SDLK_s     : new_model_pos.y -= 1; pos_dirty = true; break;
                    case SDLK_a     : new_model_pos.x -= 1; pos_dirty = true; break;
                    case SDLK_d     : new_model_pos.x += 1; pos_dirty = true; break;
                    case SDLK_SPACE : new_model_pos.z += 1; pos_dirty = true; break;
                    case SDLK_LSHIFT: new_model_pos.z -= 1; pos_dirty = true; break;

                    case SDLK_UP    : new_model_rot[0] += 1; rot_xd = true; break;
                    case SDLK_DOWN  : new_model_rot[0] -= 1; rot_xd = true; break;
                    case SDLK_LEFT  : new_model_rot[1] -= 1; rot_yd = true; break;
                    case SDLK_RIGHT : new_model_rot[1] += 1; rot_yd = true; break;
                    case SDLK_PAGEUP: new_model_rot[2] += 1; rot_zd = true; break;
                    case SDLK_PAGEDOWN:new_model_rot[2] -= 1;rot_zd = true; break;

                    default: break;
                }
            }
        }

        // 帧间刷新
        framebuffer.clear(bg);
        raster.clearZb();

        // 判断是否更新
        if (pos_dirty)
        {
            model.setPos(new_model_pos);
            pos_dirty = false;
        }
        if (rot_xd)
        {
            model.setRotate(new_model_rot[0], 0);
            rot_xd = false;
        }
        if (rot_yd)
        {
            model.setRotate(new_model_rot[1], 1);
            rot_yd = false;
        }        
        if (rot_zd)
        {
            model.setRotate(new_model_rot[2], 2);
            rot_zd = false;
        }

        // 渲染
        raster.renderOBJ(model, camera, shader);

        // 上传到 SDL
        SDL_UpdateTexture(texture, nullptr, framebuffer.buffer(), width * 3);

        // 循环渲染套餐
        // SDL_RenderClear(renderer);   // 清屏，但是因为实际的“信号源”是我自己renderer，已经在大循环清屏过了，所以不用
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);    // 用renderer把texture放到window
        SDL_RenderPresent(renderer);    // 翻页，show出来

        frameCount ++;
        uint32_t currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 1000)
        {
            fps = frameCount * 1000.f / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
            p_fps(fps);
        }
    }

    // 清理
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return;
}