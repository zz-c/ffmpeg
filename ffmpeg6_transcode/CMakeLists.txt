# 设置ffmpeg头文件的查找位置
include_directories("E:/clib/ffmpeg-6.0-full_build-shared/include")

link_directories("E:/clib/ffmpeg-6.0-full_build-shared/lib")
# 将源代码添加到此项目的可执行文件。
add_executable (ffmpeg6_transcode "main4.cpp")
#设置编译链接的库ffmpeg
target_link_libraries(ffmpeg6_transcode avutil avcodec avfilter avdevice avformat postproc swresample swscale)

