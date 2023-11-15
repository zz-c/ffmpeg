add_compile_definitions (__STDC_CONSTANT_MACROS)

include_directories("E:/ffmpeg-3.4.2/include")
link_directories("E:/ffmpeg-3.4.2/lib")

set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup" ) 
set(SRC_LIST "main.cpp" )
add_executable (windows_ffmpeg_test2 ${SRC_LIST})
target_link_libraries(windows_ffmpeg_test2 avutil avcodec avfilter avdevice avformat postproc swresample swscale)


