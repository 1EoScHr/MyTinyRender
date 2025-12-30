MyTinyrenderer核心pipeline的CUDA重构

# 概述

本分支是课程**GPU并行程序设计**的大作业实现，基于我独立实现的**图形学软光栅小项目**，将其中原本由CPU实现的部分转换为GPU实现，实现从**软光栅**到**硬件光栅**的转换。  

当然，这只是一个理论学习的练手作业，其性能无法做到像各大图形学API一样，只是出于实践的目的。  

# 重构思路

原有的Tinyrenderer是使用c++编写的，整个渲染过程如下：  
1. model对象初始化，同时reader部分读取模型文件(**.obj**)、漫反射纹理/法线纹理/高光纹理(**.tga**)到**model**对象中
2. camera对象初始化；着色器shader对象初始化；软光栅rasterization对象初始化
5. 根据模型参数、相机参数、光栅配置设置model、camera、rasterization对象
6. 渲染

而这全过程并非都适合重构为CUDA版本，因为GPU只适合重构计算密集、数据并行的部分。  
为了分析现有工程的热点，使用**gperftools**工具来找到c++中CPU 时间占比高、数据并行的函数 / 循环（这些是最适合重构为 CUDA 的部分）。  

# 性能分析

## 工具配置

使用`sudo apt install google-perftools libgoogle-perftools-dev`安装，为了满足工具分析的准确性，需要使用O2优化，因此先将原CMake配置改为`set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "RelWithDebInfo default (O2+g)")`。  

为了让分析工具充分采样，将原本800\*800的参数改为8000\*8000，避免瞬间跑完，有充足时间分析。  
但是把分辨率已经改的很大，在udeb上瞬间跑完，小看了硬件的提升（相比于我的x1c），所以是用**SDL2**库先优化为real-time的版本，这样能好点，充分检验性能。  

然后是要把这个库加到cmake里：  
```
find_library(PROFILER_LIB profiler PATHS /usr/lib/x86_64-linux-gnu)
# 2. 链接库（强制检查，确保找到）
if(NOT PROFILER_LIB)
    message(FATAL_ERROR "未找到profiler库，请执行：sudo apt install libgoogle-perftools-dev")
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE profiler) # 在add_executable之后，target_link_libraries处添加
```

仅这样似乎还会有bug，在SDL结束逻辑增加`    ProfilerStop();`强制写入，然后我这里才能显示分析文件。  

## 运行

分析程序的用法是  
`CPUPROFILE=tinyrenderer.prof ./tinyrenderer -r`，成功输出分析文件的话会显示`PROFILE: interrupts/evictions/bytes = 2614/425/84776`这样的结果。  

为了覆盖所有可能的性能开销，分别测了256x256与1024*1024的情况。  

## 结果

这里只分析函数，另外结果的每一列分别是：  
1. 自身采样数，也就是函数调用其他函数之外、只执行自身代码的时间
2. 自身耗时占比，执行自身代码所占程序总时间比例
3. 累积自身耗时占比，也就是从第一行到当前行，自身耗时的占比，用于分析“程序的耗时是否集中在少数几个函数里”
4. 总采样数，也就是自身+调用函数总共时间
5. 总耗时占比
6. 函数名

显然，这几列中最重要的就是自身耗时占比与总耗时占比，可以用于分析单个函数/调用链是否是瓶颈，需要被cuda优化。  

256*256的分析结果：  
```
liuzt@Udebian:~/Documents/MyTinyRender/build$ google-pprof --text ./tinyrenderer tinyrenderer.prof | head -20
Using local file ./tinyrenderer.
Using local file tinyrenderer.prof.
Total: 2614 samples
     283  10.8%  10.8%      294  11.2% TGAImage::get
     162   6.2%  17.0%     1686  64.5% Rasterization::renderTriangle
     151   5.8%  22.8%     1264  48.4% BPShader_GnmDiffSpec::fragment
     143   5.5%  28.3%      151   5.8% TGAImage::clear
     139   5.3%  33.6%      139   5.3% computeArea (inline)
     132   5.0%  38.6%      132   5.0% mat operator* (inline)
     115   4.4%  43.0%      115   4.4% vec::vec (inline)
      97   3.7%  46.7%      107   4.1% normalize (inline)
      85   3.3%  50.0%      234   9.0% operator* (inline)
      79   3.0%  53.0%       79   3.0% std::max (inline)
      65   2.5%  55.5%      159   6.1% Model::getTexture_spec
      62   2.4%  57.9%       62   2.4% uintize (inline)
      57   2.2%  60.1%      333  12.7% BPShader_GlobalNormalMap::vertex
      57   2.2%  62.2%       57   2.2% __lroundf
      56   2.1%  64.4%       56   2.1% vec3f::operator[] (inline)
      53   2.0%  66.4%      277  10.6% Model::getTexture_nm
      46   1.8%  68.2%       46   1.8% std::pair::pair (inline)
      45   1.7%  69.9%       47   1.8% TGAImage::set
      45   1.7%  71.6%       45   1.7% __powf_fma
```

1024*1024的结果：  
```
liuzt@Udebian:~/Documents/MyTinyRender/build$ google-pprof --text ./tinyrenderer tinyrenderer.prof | head -20
Using local file ./tinyrenderer.
Using local file tinyrenderer.prof.
Total: 6953 samples
     944  13.6%  13.6%     1001  14.4% TGAImage::get
     640   9.2%  22.8%     4977  71.6% BPShader_GnmDiffSpec::fragment
     570   8.2%  31.0%      612   8.8% TGAImage::clear
     379   5.5%  36.4%      412   5.9% normalize (inline)
     336   4.8%  41.3%      888  12.8% operator* (inline)
     314   4.5%  45.8%      314   4.5% std::max (inline)
     305   4.4%  50.2%      305   4.4% computeArea (inline)
     289   4.2%  54.3%     5861  84.3% Rasterization::renderTriangle
     261   3.8%  58.1%      707  10.2% Model::getTexture_spec
     237   3.4%  61.5%      237   3.4% __powf_fma
     230   3.3%  64.8%      230   3.3% __lroundf
     218   3.1%  67.9%      218   3.1% std::pair::pair (inline)
     190   2.7%  70.7%      209   3.0% TGAImage::set
     188   2.7%  73.4%      188   2.7% vec3f::operator[] (inline)
     173   2.5%  75.9%      173   2.5% log2_inline
     142   2.0%  77.9%      142   2.0% exp2_inline
     140   2.0%  79.9%      264   3.8% SDL_DYNAPI_entry
     126   1.8%  81.7%      690   9.9% Model::getTexture_nm
     124   1.8%  83.5%      124   1.8% TGAImage::width
```

# 分析

简单从上往下看，排名第一的是TGAImage::get函数，分析调用关系，其主要就是法线纹理、漫反射纹理、高光纹理的读取，而CPU对这种工作是串行的，因此成为了一个大拖累。而GPU上有纹理内存，应该可以用的上？  

然后是两个复杂的调用链，分别是**Rasterization::renderTriangle**与**BPShader_GnmDiffSpec::fragment**，分别是光栅化的绘制三角形与片元着色器。  

其余倒是也有可优化之处，但是时间经历也不足够分析这么多，先就把这三个先弄好吧。  


# ~~优化实现~~（已放弃）

## TGAImage::get

这里的核心就是只让CPU做从tga图像读取到内存这一步，而具体的采样由GPU来做。  

所以就主要在修改我的model对象，这些操作都是在主机端完成的，因此只需要定义好api，完全可以在cpp文件内进行。  

### 涉及到的陌生api
+ `extern texture<uchar3, cudaTextureType2D, cudaReadModeNormalizedFloat> gpu_diff_tex;    // 漫反射纹理
`，这是一个全局纹理对象，类别uchar3是三通道，然后是说明二维纹理，最后是自动执行归一化到[0,1]。


### 修改/新增的部分

在model对象增加了**uploadTex2GPU**方法，用于把内存上的纹理上传到纹理内存  

略

### 最坑人的地方

我的cuda版本比较老：  
```
liuzt@Udebian:~/Documents/MyTinyRender/build$ nvcc -V
nvcc: NVIDIA (R) Cuda compiler driver
Copyright (c) 2005-2022 NVIDIA Corporation
Built on Wed_Sep_21_10:33:58_PDT_2022
Cuda compilation tools, release 11.8, V11.8.89
Build cuda_11.8.r11.8/compiler.31833905_0
```
其不支持gcc12，十分逆天。  
配了半天，最后的办法就是升级cuda版本。  

跑起来后，竟然还全跑在cpu上，无语。  

# 从头再来，优化实现

前面都是唐氏豆包干的，现在换gemini，比较优雅的从头开始。  

## get

这次就比较优雅了。  

CPU 做法：你的 get 函数需要：计算内存偏移 -> 访问内存 -> 处理边界补齐 -> (如果要平滑) 手动做双线性插值计算。这些全是指令开销。  

GPU 做法：当你调用 tex2D 时，GPU 内部有专门的硬件电路（Texture Mapping Units）来做这件事。它不占通用算力，而且自带双线性插值和平滑效果，几乎是免费的。  

CPU 内存：是线性的。当你访问像素 (x, y) 和 (x, y+1) 时，内存地址其实隔得很远（差了一整行）。这会导致 CPU 缓存命中率极低。  

CUDA Array：它在显存里是按 Z-order（莫顿曲线） 排列的块状存储。这意味着像素 (x, y) 物理上就紧挨着它的上下左右邻居。当你采样贴图时，GPU 的 Texture Cache 命中率极高。  
  

cuda程序一般需要用cuda的基本类型，如果要使用自定义类型，需要为他们增加`__device__`与`__host__`的标记。  
但是这样过于麻烦，要用**数据解耦**思路来做，只在cpu与gpu的交界做一次转换。  

## 剩余链条部分

本来是上面的get实现了，想先试一试，但是这个是`__device__`函数，只能由`__global__`（核函数）或另一个`__device__`来调用。  

所以现在光重构了链条上的一环，还需要完整的整个链条都重新构建一下才可以。  

### 逻辑重组

原来的cpu版本是一个大循环，按面串行处理，逐个进行变换、裁剪、判断，再交给光栅化，消耗大量时间；  
而这个地方要搬到GPU上就要进行逻辑重组，顶点、光栅化都一口气处理。  

这就要整两个核函数，一个是顶点处理，另一个是光栅化处理。  

根据AI的分析，需要以每个三角形为block、每个像素为thread。  

### model

略略略

这文档要写吐了。疯狂ai重构。  

终于在与屎山搏斗一整晚一整凌晨，至少是能跑了。  

哎，太难了！  


