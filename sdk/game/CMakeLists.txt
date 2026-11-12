# プロジェクトのゲームのコード(<プロジェクト>/game/*.cpp / *.h)を GomiGame.dll にする。
# プロジェクトには CMakeLists を置かず、エディタがエンジンのこのファイルを使って
#   cmake -S <エンジン>/sdk/game -B <プロジェクト>/build -DGOMI_PROJECT=<プロジェクト>
#   cmake --build <プロジェクト>/build --config <エンジンの構成>
# で作る。出来た DLL は <プロジェクト>/build/bin/GomiGame.dll。
# GomiCore の場所は隣の ../GomiEngine.cmake(インストールした形と開発中のビルドで中身が違う)が決める。
# 生成式はコメントにも書かない(開発中のビルドではこのファイルを file(GENERATE) で写すので、評価されてしまう)
cmake_minimum_required(VERSION 3.24)

include(${CMAKE_CURRENT_LIST_DIR}/../GomiEngine.cmake)
# エンジンと同じ構成でしか作れない(Debug と Release で C++ の実行時ライブラリが違い、DLL をまたいで std::string などを渡せない)
set(CMAKE_CONFIGURATION_TYPES ${GOMI_ENGINE_CONFIG} CACHE STRING "" FORCE)

project(GomiGame LANGUAGES CXX)

if (NOT GOMI_PROJECT)
    message(FATAL_ERROR "-DGOMI_PROJECT=<プロジェクトのフォルダ> を付けてください")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${GOMI_PROJECT}/game/*.cpp ${GOMI_PROJECT}/game/*.h)
add_library(GomiGame SHARED ${GOMI_GAME_ENTRY} ${GAME_SOURCES})
target_include_directories(GomiGame PRIVATE ${GOMI_PROJECT}/game)
target_link_libraries(GomiGame PRIVATE GomiCore)
string(TOUPPER ${GOMI_ENGINE_CONFIG} GOMI_CONFIG_UPPER)
set_target_properties(GomiGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${GOMI_CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
if (MSVC)
    target_compile_options(GomiGame PRIVATE /W3)
endif()
