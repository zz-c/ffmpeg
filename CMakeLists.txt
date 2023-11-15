# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CMAKE" VERSION 1.2.3.4)

message( STATUS "CMAKE_PROJECT_NAME    = ${CMAKE_PROJECT_NAME}" )
message( STATUS "PROJECT_VERSION_MAJOR = ${PROJECT_VERSION_MAJOR}" )
message( STATUS "PROJECT_VERSION_MINOR = ${PROJECT_VERSION_MINOR}" )
message( STATUS "PROJECT_VERSION_PATCH = ${PROJECT_VERSION_PATCH}" )
message( STATUS "PROJECT_VERSION_TWEAK = ${PROJECT_VERSION_TWEAK}" )
message( STATUS "PROJECT_HOMEPAGE_URL = ${PROJECT_HOMEPAGE_URL}")
message( STATUS "CMAKE_PROJECT_DESCRIPTION = ${CMAKE_PROJECT_DESCRIPTION}" )
message( STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}" )
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "current platform: Windows")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Android")
    MESSAGE(STATUS "current platform: Android")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    MESSAGE(STATUS "current platform: Darwin")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    MESSAGE(STATUS "current platform: Emscripten")
ELSE ()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

# 包含子项目。
#add_subdirectory ("ffmpeg_test")
#add_subdirectory ("ffmpeg_sdl_test")
#add_subdirectory ("ffmpeg6_sdl_test")
#add_subdirectory ("ffmpeg6_transcode")
#add_subdirectory ("ffmpeg6_mp4_wav")
add_subdirectory ("windows_ffmpeg_test")
add_subdirectory ("windows_ffmpeg_test2")