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

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE)

FetchContent_Declare(cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.15
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _header_only)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _no_cmake)

FetchContent_Declare(imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG 18cef5e031d8c6973d80284c67f60549fafd78c1
    SOURCE_SUBDIR _no_cmake)

FetchContent_MakeAvailable(glfw miniaudio stb glm cgltf imgui imguizmo)

# OpenGL 4.5 core のローダー(glad2 で生成済みのものを同梱)
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# --- エンジン本体 ---
# main 以外をライブラリにして、実行ファイルとテストで共有する
file(GLOB GOMI_SOURCES CONFIGURE_DEPENDS
    GomiEngine/*.cpp
    GomiEngine/*.h)
list(REMOVE_ITEM GOMI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/Gomi.cpp)

add_library(GomiCore STATIC ${GOMI_SOURCES})
target_include_directories(GomiCore PUBLIC
    GomiEngine
    ${miniaudio_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${cgltf_SOURCE_DIR})
target_link_libraries(GomiCore PUBLIC glfw glad glm::glm)
target_compile_definitions(GomiCore PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS GLM_ENABLE_EXPERIMENTAL)
# 開発中は exe の隣に assets が無ければソースの assets を直接読む(編集がすぐ反映される)
target_compile_definitions(GomiCore PRIVATE GOMI_DEFAULT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
if (WIN32)
    target_link_libraries(GomiCore PUBLIC winmm)
endif()
if (MSVC)
    target_compile_options(GomiCore PUBLIC /utf-8)
    target_compile_options(GomiCore PRIVATE /W3)
endif()

add_executable(GomiEngine GomiEngine/Gomi.cpp)
target_link_libraries(GomiEngine PRIVATE GomiCore)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GomiEngine)
    set_property(TARGET GomiEngine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:GomiEngine>")
endif()

# --- エディタ(Dear ImGui) ---
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imgui_SOURCE_DIR}/misc/cpp ${imguizmo_SOURCE_DIR}/src)
target_link_libraries(imgui PUBLIC glfw)

file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS Editor/*.cpp Editor/*.h)
add_executable(GomiEditor ${EDITOR_SOURCES})
target_link_libraries(GomiEditor PRIVATE GomiCore imgui)
if (MSVC)
    set_property(TARGET GomiEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:GomiEditor>")
endif()

# 設定ファイルを実行ファイルの隣に置く
add_custom_command(TARGET GomiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/settings.json
        $<TARGET_FILE_DIR:GomiEngine>/settings.json)

# --- テスト(ctest で実行) ---
enable_testing()
foreach(test ComponentArrayTest ReflectionTest SceneTest CollisionTest ModelTest EditorTest ReplayTest)
    add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} PRIVATE GomiCore)
    add_test(NAME ${test} COMMAND ${test})
endforeach()

# 操作の記録(tests/replays/*.json)を流し、確認点の画面と状態を基準と比べる。基準を作り直す時は
#   GomiEngine --replay tests/replays/<名前>.json --replay-update
file(GLOB GOMI_REPLAYS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/replays/*.json)
foreach(replay ${GOMI_REPLAYS})
    get_filename_component(name ${replay} NAME_WE)
    add_test(NAME Replay.${name} COMMAND GomiEngine --replay ${replay} --no-wait
        WORKING_DIRECTORY $<TARGET_FILE_DIR:GomiEngine>)
endforeach()
