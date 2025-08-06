cmake_minimum_required(VERSION 3.15)

project(territory CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
endif()

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v0.10.3
)
FetchContent_MakeAvailable(endstone)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG master
)
FetchContent_MakeAvailable(nlohmann_json)

if (WIN32)
    include_directories(D:/Apps/vcpkg/installed/x64-windows-static/include)
    link_directories(D:/Apps/vcpkg/installed/x64-windows-static/lib)
endif()

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
    include_directories(${SQLITE3_INCLUDE_DIRS})
    link_directories(${SQLITE3_LIBRARY_DIRS})
endif ()

endstone_add_plugin(${PROJECT_NAME} src/territory.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json "D:/Apps/vcpkg/installed/x64-windows-static/lib/sqlite3.lib")
endif ()
if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json ${SQLITE3_LIBRARIES})
endif ()
