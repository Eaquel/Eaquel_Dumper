cmake_minimum_required(VERSION 4.3.2 FATAL_ERROR)

if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME must be set: cmake -DMODULE_NAME=eaquel_dumper ...")
endif()

project(${MODULE_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

string(REGEX REPLACE "^android-" "" _api_level "${ANDROID_PLATFORM}")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-fstrict-flex-arrays=3 _HAS_STRICT_FLEX_ARRAYS)

set(EAQUEL_BASE_FLAGS
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 -Wformat-security
    -Wnull-dereference -Wimplicit-fallthrough -Wcast-align -Wstrict-aliasing=2
    -Wnon-virtual-dtor -Woverloaded-virtual -Wno-unused-result
    -fstack-protector-strong -fPIC -ffunction-sections -fdata-sections
    -DANDROID_PLATFORM_LEVEL=${_api_level}
)

if(_HAS_STRICT_FLEX_ARRAYS)
    list(APPEND EAQUEL_BASE_FLAGS -fstrict-flex-arrays=3)
endif()

set(EAQUEL_CXX_FLAGS -fno-exceptions -fno-rtti)
set(EAQUEL_RELEASE_FLAGS -O3 -fvisibility=hidden -fvisibility-inlines-hidden -flto=thin -D_FORTIFY_SOURCE=3 -DNDEBUG)
set(EAQUEL_DEBUG_FLAGS -O0 -g3 -DDEBUG_BUILD -fno-omit-frame-pointer)
set(EAQUEL_LINKER_FLAGS -Wl,--hash-style=both -Wl,-z,noexecstack -Wl,--build-id=sha1 -Wl,--no-undefined)

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    list(APPEND EAQUEL_BASE_FLAGS -march=armv8-a)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND EAQUEL_BASE_FLAGS -mbranch-protection=standard)
    endif()
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    list(APPEND EAQUEL_BASE_FLAGS -mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=neon)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    list(APPEND EAQUEL_BASE_FLAGS -march=x86-64)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    list(APPEND EAQUEL_BASE_FLAGS -march=i686 -msse4.2)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND EAQUEL_LINKER_FLAGS -Wl,-z,relro -Wl,-z,now -Wl,--gc-sections -Wl,-exclude-libs,ALL -Wl,--strip-all)
endif()

if(NOT CMAKE_ANDROID_ARCH_ABI MATCHES "^x86")
    list(APPEND EAQUEL_LINKER_FLAGS -Wl,-z,max-page-size=16384)
endif()

add_library(${MODULE_NAME} SHARED Dumper.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)

target_compile_options(${MODULE_NAME} PRIVATE
    ${EAQUEL_BASE_FLAGS}
    ${EAQUEL_CXX_FLAGS}
    $<$<CONFIG:Debug>:${EAQUEL_DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${EAQUEL_RELEASE_FLAGS}>
)

target_link_options(${MODULE_NAME} PRIVATE ${EAQUEL_LINKER_FLAGS})
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${MODULE_NAME} PRIVATE log android)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET ${MODULE_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment --remove-section=.note --remove-section=.gnu.version_d $<TARGET_FILE:${MODULE_NAME}>
        COMMENT "Stripping ${MODULE_NAME} binary"
    )
endif()
