
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LIB_TYPE "debug")
    set(LIB_SUFFIX "d")
    set(HASH "16BFBB7DA548CF1B79A25EBC21E2FB040DAD9049B61D2C9BF937B0292F038B67")
else ()
    set(LIB_TYPE "release")
    set(HASH "D09FAE867AF40F4F16E4A45BB387CB8F117409FDA815A48E54DCA7B7DF26BC60")
endif ()

FetchContent_Declare(skia
        URL "https://github.com/Yin-Jinlong/skia-prebuild-for-windows/releases/download/v-m130-e415ec/skia-e415ec-msvc-static-${LIB_TYPE}-MD${LIB_SUFFIX}.7z"
        URL_HASH SHA256=${HASH}
)

FetchContent_MakeAvailable(skia)

include_directories(
        ${skia_SOURCE_DIR}
)

link_directories(
        ${skia_SOURCE_DIR}/lib
)
