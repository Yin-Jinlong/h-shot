
FetchContent_Declare(glad
        GIT_REPOSITORY https://github.com/premake-libs/glad.git
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glad)

include_directories(
        ${glad_SOURCE_DIR}/include
)
