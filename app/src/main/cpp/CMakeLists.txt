cmake_minimum_required(VERSION 3.10.2)
project("glcamerafis")

add_library(
             opengl_renderer
             SHARED
             opengl_camera_renderer.cpp)
find_library(
              log-lib
              log )
find_library(opengl_lib GLESv3)
find_library(opengl2_lib GLESv2)
find_library(egl-lib EGL)
find_library(android-lib android)

target_link_libraries(
        opengl_renderer
        ${opengl-lib}
#        使用Opengl2中的 扩展头文件
        ${opengl2_lib}
        ${egl-lib}
        ${android-lib}
        ${log-lib} )