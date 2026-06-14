cmake_minimum_required(VERSION 3.15)

project(territory CXX)
set(BASE_VERSION "0.4.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
endif()

include(FetchContent)

# Endstone
FetchContent_Declare(
    endstone
    GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
    GIT_TAG v0.11.2
)
FetchContent_MakeAvailable(endstone)

# nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# SQLite3
find_package(SQLite3 REQUIRED)

# fmt (header-only or static)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        endstone_money_connect
        GIT_REPOSITORY https://github.com/yuhangle/endstone-money-connect.git
        GIT_TAG main
)
FetchContent_MakeAvailable(endstone_money_connect)

# 定义并获取 BinaryStream
FetchContent_Declare(
        BinaryStream
        GIT_REPOSITORY https://github.com/GlacieTeam/BinaryStream.git
        GIT_TAG        main # 或者指定具体的 commit hash 以保证稳定性
)

# 确保项目已下载并可用
FetchContent_MakeAvailable(BinaryStream)

add_library(bstream_obj OBJECT
        "${binarystream_SOURCE_DIR}/src/binarystream/BinaryStream.cpp"
        "${binarystream_SOURCE_DIR}/src/binarystream/ReadOnlyBinaryStream.cpp"
        "${binarystream_SOURCE_DIR}/src/binarystream-c/bstream.cpp"
)

target_compile_options(bstream_obj PRIVATE -fPIC)
target_include_directories(bstream_obj PUBLIC "${binarystream_SOURCE_DIR}/include")

# 插件源码
file(GLOB PLUGIN_SOURCES
        CONFIGURE_DEPENDS
        "src/*.cpp"
)
endstone_add_plugin(${PROJECT_NAME} ${PLUGIN_SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        bstream_obj
        nlohmann_json::nlohmann_json
        SQLite::SQLite3
        fmt::fmt-header-only
        endstone_money_connect
)

find_package(Git QUIET)
if(GIT_FOUND)
    # 1. 查找最近的【纯净】正式版本 Tag (排除带 - 的 Tag)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 --match "v[0-9]*" --exclude "*-*" HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE LAST_STABLE_TAG
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(LAST_STABLE_TAG)
        # 有稳定版基准：计算距离该 Tag 的提交数
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count "${LAST_STABLE_TAG}..HEAD"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        # 无稳定版基准：计算总提交数
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    # 获取短哈希
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_SHORT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_COMMIT_COUNT "0")
    set(GIT_SHORT_HASH "unknown")
endif()

# 决定最终版本号
if(RELEASE_VERSION)
    set(PROJECT_VERSION "${BASE_VERSION}")
else()
    set(PROJECT_VERSION "${BASE_VERSION}-alpha.${GIT_COMMIT_COUNT}+${GIT_SHORT_HASH}")
endif()

# 生成 version.h
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
)

# 将生成目录加入包含路径
target_include_directories(${PROJECT_NAME} PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/generated" "${binarystream_SOURCE_DIR}/include")