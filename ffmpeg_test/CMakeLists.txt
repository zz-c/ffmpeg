# CMakeList.txt: ffmpeg_test 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 设置ffmpeg头文件的查找位置
include_directories("E:/ffmpeg-3.4.2/include")
# 设置ffmpeg库的查找目录 jar
link_directories("E:/ffmpeg-3.4.2/lib")
# 将源代码添加到此项目的可执行文件。
add_executable (ffmpeg_test "main.cpp"  "test.h" "test.cpp")
#设置编译链接的库ffmpeg
target_link_libraries(ffmpeg_test avutil avcodec avfilter avdevice avformat postproc swresample swscale)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ffmpeg_test PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
