cmake_minimum_required(VERSION 3.24)
# エンジンのバージョン。配布物(tools/release.sh)・タグ(v<番号>)・エディタの表示・sdk の GomiEngine.cmake はここから決まる
project(GomiEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# exe・DLL を全部 build/<構成>/ に出す(GomiCore.dll・glfw3.dll を exe の隣に置くため)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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

# glfw はエンジン(GomiCore.dll)とエディタの両方が呼ぶので DLL にして1つにする
set(BUILD_SHARED_LIBS ON)
FetchContent_MakeAvailable(glfw)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(miniaudio stb glm cgltf imgui imguizmo)

# OpenGL 4.5 core のローダー(glad2 で生成済みのものを同梱)
# エンジンとエディタの両方が GL を呼ぶので、関数ポインタを1つにするため DLL にする
add_library(glad SHARED third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_compile_definitions(glad PUBLIC GLAD_API_CALL_EXPORT PRIVATE GLAD_API_CALL_EXPORT_BUILD)

# --- エンジン本体 ---
# main 以外を DLL にして、実行ファイル・エディタ・テスト・ゲームのコード(GomiGame.dll)で共有する。
# 静的ライブラリだと登録表などのシングルトンがモジュールごとに別物になる
file(GLOB GOMI_SOURCES CONFIGURE_DEPENDS
    GomiEngine/*.cpp
    GomiEngine/*.h)
list(REMOVE_ITEM GOMI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/Gomi.cpp)

# GOMI_VERSION(project の VERSION)を C++ から使うヘッダ
configure_file(GomiEngine/GomiVersion.h.in ${CMAKE_BINARY_DIR}/generated/GomiVersion.h @ONLY)

add_library(GomiCore SHARED ${GOMI_SOURCES})
set_target_properties(GomiCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(GomiCore PUBLIC
    GomiEngine
    ${CMAKE_BINARY_DIR}/generated
    ${miniaudio_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${cgltf_SOURCE_DIR})
target_link_libraries(GomiCore PUBLIC glfw glad glm::glm)
target_compile_definitions(GomiCore PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS GLM_ENABLE_EXPERIMENTAL)
# 開発中は --project が無く exe の隣に project.json も無ければ、見本のプロジェクトを直接開く(編集がすぐ反映される)
set(GOMI_SAMPLE_PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/samples/Demo)
target_compile_definitions(GomiCore PRIVATE GOMI_DEFAULT_PROJECT_DIR="${GOMI_SAMPLE_PROJECT}")
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
# プロジェクトのゲームのコードは、エディタ自身と同じ構成で作る(Debug と Release の実行時ライブラリは混ぜられない)
target_compile_definitions(GomiEditor PRIVATE GOMI_BUILD_CONFIG="$<CONFIG>")
if (MSVC)
    set_property(TARGET GomiEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:GomiEditor>")
endif()

# 設定ファイルを実行ファイルの隣に置く
add_custom_command(TARGET GomiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/settings.json
        $<TARGET_FILE_DIR:GomiEngine>/settings.json)

# --- ゲームのコードを作るための sdk と、インストールした形 ---
# エディタは exe の隣の sdk/game/CMakeLists.txt で、プロジェクトの game/ を <プロジェクト>/build に作る。
# 開発中のビルド: build/<構成>/sdk/(GomiCore はソースのヘッダとビルドの .lib を指す)
# インストールした形(cmake --install build --config <構成> --prefix <先>。B の配布物と同じ):
#   <先>/GomiEditor.exe・GomiEngine.exe・*.dll・settings.json
#   <先>/sdk/GomiEngine.cmake・game/(CMakeLists.txt・GameEntry.cpp)・include/(エンジンと glm のヘッダ)・lib/GomiCore.lib
set(GOMI_SDK_CONFIG "$<CONFIG>")
set(GOMI_SDK_GAME_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine/game_entry/GameEntry.cpp")
set(GOMI_SDK_CORE_LIB "$<TARGET_LINKER_FILE:GomiCore>")
set(GOMI_SDK_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/GomiEngine;${glm_SOURCE_DIR}")
configure_file(sdk/GomiEngine.cmake.in ${CMAKE_BINARY_DIR}/sdk-build/GomiEngine.cmake.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/$<CONFIG>/sdk/GomiEngine.cmake INPUT ${CMAKE_BINARY_DIR}/sdk-build/GomiEngine.cmake.in)
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/$<CONFIG>/sdk/game/CMakeLists.txt INPUT ${CMAKE_CURRENT_SOURCE_DIR}/sdk/game/CMakeLists.txt)

# インストールした形では sdk の中の相対パスにする(${CMAKE_CURRENT_LIST_DIR} は読む時に決まる)
set(GOMI_SDK_GAME_ENTRY "\${CMAKE_CURRENT_LIST_DIR}/game/GameEntry.cpp")
set(GOMI_SDK_CORE_LIB "\${CMAKE_CURRENT_LIST_DIR}/lib/GomiCore.lib")
set(GOMI_SDK_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/include")
configure_file(sdk/GomiEngine.cmake.in ${CMAKE_BINARY_DIR}/sdk-install/GomiEngine.cmake.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/sdk-install/$<CONFIG>/GomiEngine.cmake INPUT ${CMAKE_BINARY_DIR}/sdk-install/GomiEngine.cmake.in)

install(TARGETS GomiEngine GomiEditor glfw glad RUNTIME DESTINATION .)
install(TARGETS GomiCore RUNTIME DESTINATION . ARCHIVE DESTINATION sdk/lib)
install(FILES GomiEngine/settings.json DESTINATION .)
install(FILES ${CMAKE_BINARY_DIR}/sdk-install/$<CONFIG>/GomiEngine.cmake DESTINATION sdk)
install(FILES sdk/game/CMakeLists.txt GomiEngine/game_entry/GameEntry.cpp DESTINATION sdk/game)
install(DIRECTORY GomiEngine/ DESTINATION sdk/include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "game_entry" EXCLUDE)
install(FILES ${CMAKE_BINARY_DIR}/generated/GomiVersion.h DESTINATION sdk/include)
install(DIRECTORY ${glm_SOURCE_DIR}/glm DESTINATION sdk/include FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inl" PATTERN "*.h")

# --- ランチャー(バージョンを入れる・プロジェクトを作る・開く) ---
# バージョンに依らない1本の exe なので、配布物(install)には入れない。置くのは tools/install-launcher.sh
add_library(LauncherCore STATIC Launcher/LauncherCore.cpp Launcher/LauncherCore.h)
target_include_directories(LauncherCore PUBLIC Launcher GomiEngine/include)
target_compile_definitions(LauncherCore PUBLIC NOMINMAX UNICODE _UNICODE)
target_link_libraries(LauncherCore PUBLIC shell32 ole32)
if (MSVC)
    target_compile_options(LauncherCore PUBLIC /utf-8)
endif()

add_executable(GomiLauncher Launcher/LauncherMain.cpp Launcher/LauncherApp.cpp Launcher/LauncherOptions.h)
target_link_libraries(GomiLauncher PRIVATE LauncherCore imgui opengl32)
target_include_directories(GomiLauncher PRIVATE ${stb_SOURCE_DIR})

add_executable(LauncherTest tests/LauncherTest.cpp)
target_link_libraries(LauncherTest PRIVATE LauncherCore)
target_compile_definitions(LauncherTest PRIVATE GOMI_LAUNCHER_TEST_DIR="${CMAKE_BINARY_DIR}/launcher-test")

# --- テスト(ctest で実行) ---
enable_testing()
foreach(test ComponentArrayTest ReflectionTest SceneTest CollisionTest ModelTest EditorTest ReplayTest GameModuleTest)
    add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} PRIVATE GomiCore)
    add_test(NAME ${test} COMMAND ${test})
endforeach()

add_test(NAME LauncherTest COMMAND LauncherTest)

# GameModuleTest が読む見本のゲームのコード
add_library(GameSampleModule SHARED GomiEngine/game_entry/GameEntry.cpp tests/game_sample/Sample.cpp)
target_link_libraries(GameSampleModule PRIVATE GomiCore)
add_dependencies(GameModuleTest GameSampleModule)

# 見本のプロジェクトの操作の記録(samples/Demo/tests/replays/*.json)を流し、確認点の画面と状態を基準と比べる。基準を作り直す時は
#   GomiEngine --project samples/Demo --replay samples/Demo/tests/replays/<名前>.json --replay-update
file(GLOB GOMI_REPLAYS CONFIGURE_DEPENDS ${GOMI_SAMPLE_PROJECT}/tests/replays/*.json)
foreach(replay ${GOMI_REPLAYS})
    get_filename_component(name ${replay} NAME_WE)
    add_test(NAME Replay.${name} COMMAND GomiEngine --project ${GOMI_SAMPLE_PROJECT} --replay ${replay} --no-wait
        WORKING_DIRECTORY $<TARGET_FILE_DIR:GomiEngine>)
endforeach()
