cmake_minimum_required(VERSION 3.10)
project(ChartAnalyzer)

set(CMAKE_CXX_STANDARD 17)

find_package(Lua REQUIRED)

add_executable(analyzer FN_diffGuesser.cpp)
target_include_directories(analyzer PRIVATE ${LUA_INCLUDE_DIR})
# Sol2を同梱している場合はそのパスも追加する
# target_include_directories(analyzer PRIVATE ${CMAKE_SOURCE_DIR}/sol)
target_link_libraries(analyzer PRIVATE ${LUA_LIBRARIES})
