cmake_minimum_required(VERSION 3.31)
project(h-shot CXX C)

set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)


add_definitions(
        -DUNICODE
        -DAPP_NAME=\"h-shot\"
)

if (MSVC)
    # 设置utf-8
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif ()

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(WIN on)
endif ()

find_package(skia)
find_package(glad)

file(GLOB H_SHOT_SRCS
        "src/*.cpp"
        "src/ui/*.cpp"
        "src/window/*.cpp"
)

list(APPEND H_SHOT_SRCS
        "${glad_SOURCE_DIR}/src/glad.c"
        "resource.rc"
)

if (${WIN})
    add_executable(h-shot WIN32 ${H_SHOT_SRCS})
else ()
    add_executable(h-shot ${H_SHOT_SRCS})
endif ()

target_link_libraries(h-shot
        opengl32
        skia
)
