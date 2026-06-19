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


set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library" FORCE)

FetchContent_MakeAvailable(network)
FetchContent_MakeAvailable(CLI)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE network)
target_link_libraries(${PROJECT_NAME} PRIVATE CLI)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)