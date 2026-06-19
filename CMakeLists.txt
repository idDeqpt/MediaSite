cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME MediaSite)

project(${PROJECT_NAME} CXX)


include(FetchContent)

FetchContent_Declare(
    network
    GIT_REPOSITORY "https://github.com/idDeqpt/Network.git"
    GIT_TAG 3c822f3ef08ea95de3e8e669f9e311432f2708c9
)

FetchContent_Declare(
    CLI
    GIT_REPOSITORY "https://github.com/idDeqpt/CommandLineInterface.git"
    GIT_TAG 7a63897b3b96b63574c35b12fd08edbc115c137e
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
FetchContent_MakeAvailable(CLI)
FetchContent_MakeAvailable(wolfssl)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE network)
target_link_libraries(${PROJECT_NAME} PRIVATE CLI)
target_link_libraries(${PROJECT_NAME} PRIVATE wolfssl)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)