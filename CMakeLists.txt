cmake_minimum_required(VERSION 3.24)
project(GomiEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- 外部ライブラリ(初回の構成時に自動で取得) ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)

# ヘッダだけ使うものは SOURCE_SUBDIR を存在しない場所にして add_subdirectory させない
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _header_only)
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
    SOURCE_SUBDIR _header_only)

FetchContent_MakeAvailable(glfw miniaudio stb)

# OpenGL 4.5 core のローダー(glad2 で生成済みのものを同梱)
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# --- エンジン本体 ---
file(GLOB GOMI_SOURCES CONFIGURE_DEPENDS
    GomiEngine/*.cpp
    GomiEngine/*.h)

add_executable(GomiEngine ${GOMI_SOURCES} GomiEngine/resource/game.rc)
target_include_directories(GomiEngine PRIVATE
    GomiEngine
    ${miniaudio_SOURCE_DIR}
    ${stb_SOURCE_DIR})
target_link_libraries(GomiEngine PRIVATE glfw glad)
target_compile_definitions(GomiEngine PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)

if (WIN32)
    target_link_libraries(GomiEngine PRIVATE winmm)
endif()
if (MSVC)
    target_compile_options(GomiEngine PRIVATE /utf-8 /W3)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GomiEngine)
    set_property(TARGET GomiEngine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:GomiEngine>")
endif()

# 設定ファイルを実行ファイルの隣に置く
add_custom_command(TARGET GomiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/settings.json
        $<TARGET_FILE_DIR:GomiEngine>/settings.json)
