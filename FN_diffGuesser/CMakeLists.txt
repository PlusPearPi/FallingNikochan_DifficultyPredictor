# CMake 3.11以降が必要
cmake_minimum_required(VERSION 3.11)
project(ChartAnalyzer CXX)

set(CMAKE_CXX_STANDARD 17)

# 1. FetchContentを使ってnlohmann/jsonを自動ダウンロード
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 2. 実行ファイルの定義（ファイル名は実際の環境に合わせること）
add_executable(analyzer FN_diffGuesser.cpp)

# 3. 実行ファイルにダウンロードしたjsonライブラリをリンクさせる
target_link_libraries(analyzer PRIVATE nlohmann_json::nlohmann_json)
# ネットワーク通信用のwininetも必要に応じてリンクする
target_link_libraries(analyzer PRIVATE wininet)