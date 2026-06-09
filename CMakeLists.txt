cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME messenger_server)

project(${PROJECT_NAME} CXX)


include(FetchContent)

FetchContent_Declare(
    network
    GIT_REPOSITORY "https://github.com/idDeqpt/Network.git"
    GIT_TAG 5c9261fa81cd6a9e2f2fde49e835c2d3b151ebba
)


set(WOLFSSL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WOLFSSL_TESTS    OFF CACHE BOOL "" FORCE)
set(WOLFSSL_OLD_TLS  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    wolfssl
    GIT_REPOSITORY "https://github.com/wolfSSL/wolfssl"
    GIT_TAG v5.9.1-stable
    GIT_SHALLOW TRUE
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library" FORCE)

FetchContent_MakeAvailable(network)
FetchContent_MakeAvailable(wolfssl)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Server.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE network)
target_link_libraries(${PROJECT_NAME} PRIVATE wolfssl)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)