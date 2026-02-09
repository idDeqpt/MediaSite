cmake_minimum_required(VERSION 3.5.0)

set(PROJECT_NAME messenger_server)

project(${PROJECT_NAME} CXX)

include(FetchContent)

FetchContent_Declare(
    network
    GIT_REPOSITORY "https://github.com/idDeqpt/Network.git"
    GIT_TAG main  # или конкретная версия
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library" FORCE)

FetchContent_MakeAvailable(network)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE network)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)