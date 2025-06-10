# 标题
cmake_minimum_required(VERSION 3.10.0)
project(2024011668 VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#设置编译选项(将警告视为错误)
if (MSVC)
  add_compile_options(/WX)
else()
   add_compile_options(-Werror)
endif()

# 设置编译输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

set(TARGET ${PROJECT_NAME}-${CMAKE_SYSTEM_NAME})

# 查找并载入Cmake预设
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(SDL3_ttf REQUIRED)

# 添加资源文件（Windows平台特定）
if(WIN32)
  set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/app_icon.rc")
endif()

# 定义头文件列表
set(HEADER_FILES
    "src/Game.h"
    "src/Scene.h"
    "src/Object.h"
    "src/ObjectPool.h"
    "src/SceneTitle.h"
    "src/SceneMain.h"
    "src/SceneEnd.h"
    "src/SceneSettings.h"
    "src/SceneIntro.h"
    "src/SceneBoss.h"
)

# 定义源文件列表
set(SOURCE_FILES
    "src/main.cpp"
    "src/Game.cpp"
    "src/Scene.cpp"
    "src/SceneTitle.cpp"
    "src/SceneMain.cpp"
    "src/SceneEnd.cpp"
    "src/SceneSettings.cpp"
    "src/SceneIntro.cpp"
    "src/SceneBoss.cpp"
)

# 添加可执行文件
add_executable(${TARGET}
                ${SOURCE_FILES}
                ${HEADER_FILES}
                ${APP_ICON_RESOURCE_WINDOWS})
              
target_link_libraries(${TARGET}
                        ${SDL3_LIBRARIES}
                        SDL3_image::SDL3_image
                        SDL3_mixer::SDL3_mixer
                        SDL3_ttf::SDL3_ttf
                        )

# 不要弹出控制台窗口
if (MSVC)
    target_link_options(${TARGET} PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

# 设置包含目录
target_include_directories(${TARGET} PRIVATE src)

# 为IDE提供更好的项目结构显示
source_group("Header Files" FILES ${HEADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})