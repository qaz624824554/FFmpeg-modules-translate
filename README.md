# 前言

众所周知，FFmpeg 的代码开发上手难度较高，源于官方提供的文档很少有包含代码教程相关的。要想熟练掌握 FFmpeg 的代码库开发，需要借助它的头文件，FFmpeg 把很多代码库教程都写在头文件里面。因此，熟读头文件的内容很重要，为此，我对 FFmpeg 6.x 版本的头文件进行了翻译，方便大家阅读理解。相信我，通读一遍头文件的注释后，你的 FFmpeg 的代码库开发技能将更上一层。

本仓库适用于有 FFmpeg 代码库开发基础，但想深入熟练使用的同学。

FFmpeg 有如下模块：

- libavcodec
- libavformat
- libavutil
- libavdevice
- libavfilter
- libswresample
- libswscale
- libpostproc
