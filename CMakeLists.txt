cmake_minimum_required(VERSION 3.15)

project(territory CXX)

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
    GIT_TAG v0.10.3
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
if (UNIX)
    # 内置fmt
    find_package(fmt CONFIG REQUIRED)
endif ()
# 插件源码
endstone_add_plugin(${PROJECT_NAME} src/territory.cpp include/Territory_Action.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# 链接库
if (UNIX)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        SQLite::SQLite3
        fmt::fmt-header-only
)
else()
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            nlohmann_json::nlohmann_json
            SQLite::SQLite3
    )
endif ()