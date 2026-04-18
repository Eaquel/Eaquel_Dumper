cmake_minimum_required(VERSION 4.3.1)

if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME must be set, e.g. -DMODULE_NAME=eaqueldumper")
endif()

project(${MODULE_NAME}
    LANGUAGES    CXX
    VERSION      1.0.0
    DESCRIPTION  "Eaquel IL2CPP Dumper — Zygisk-based"
)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(EAQUEL_ARCH "arm64")
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(EAQUEL_ARCH "arm32")
else()
    message(WARNING "Unsupported ABI '${CMAKE_ANDROID_ARCH_ABI}'")
    set(EAQUEL_ARCH "unknown")
endif()

message(STATUS "[Eaquel] ABI=${CMAKE_ANDROID_ARCH_ABI}  ARCH=${EAQUEL_ARCH}  BUILD=${CMAKE_BUILD_TYPE}")

set(EAQUEL_C_FLAGS
    -Werror=format
    -fdata-sections
    -ffunction-sections
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
    -DEAQUEL_VERSION_MAJOR=1
    -DEAQUEL_VERSION_MINOR=0
)

set(EAQUEL_CXX_FLAGS
    -fno-exceptions
    -fno-rtti
    -Wnull-dereference
    -Woverloaded-virtual
)

if(EAQUEL_ARCH STREQUAL "arm64")
    list(APPEND EAQUEL_C_FLAGS -march=armv8-a)
elseif(EAQUEL_ARCH STREQUAL "arm32")
    list(APPEND EAQUEL_C_FLAGS
        -mthumb
        -march=armv7-a
        -mfloat-abi=softfp
        -mfpu=neon
    )
endif()

set(EAQUEL_LINKER_FLAGS
    -Wl,--hash-style=both
    -Wl,-z,max-page-size=16384
    -Wl,-z,noexecstack
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND EAQUEL_C_FLAGS
        -O3
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -flto=thin
    )
    if(EAQUEL_ARCH STREQUAL "arm64")
        list(APPEND EAQUEL_C_FLAGS -mbranch-protection=standard)
    endif()
    list(APPEND EAQUEL_LINKER_FLAGS
        -Wl,-exclude-libs,ALL
        -Wl,--gc-sections
        -Wl,--strip-all
        -Wl,-z,relro
        -Wl,-z,now
    )
else()
    list(APPEND EAQUEL_C_FLAGS -O0 -g -DDEBUG_BUILD)
endif()

list(JOIN EAQUEL_C_FLAGS      " " _c_str)
list(JOIN EAQUEL_CXX_FLAGS    " " _cxx_str)
list(JOIN EAQUEL_LINKER_FLAGS " " _ld_str)

string(APPEND CMAKE_C_FLAGS             " ${_c_str}")
string(APPEND CMAKE_CXX_FLAGS           " ${_c_str} ${_cxx_str}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${_ld_str}")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${_ld_str}")

add_library(${MODULE_NAME} SHARED
    Dumper.cpp
)

target_include_directories(${MODULE_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${MODULE_NAME}
    PRIVATE
        $<$<STREQUAL:${EAQUEL_ARCH},arm64>:EAQUEL_ARM64=1>
        $<$<STREQUAL:${EAQUEL_ARCH},arm32>:EAQUEL_ARM32=1>
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        log
        android
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET  ${MODULE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_STRIP}
                --strip-all
                --remove-section=.comment
                --remove-section=.note
                $<TARGET_FILE:${MODULE_NAME}>
    )
endif()

message(STATUS "[Eaquel] Target : ${MODULE_NAME}")
message(STATUS "[Eaquel] Arch   : ${EAQUEL_ARCH}")
message(STATUS "[Eaquel] Build  : ${CMAKE_BUILD_TYPE}")
message(STATUS "[Eaquel] CXX    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[Eaquel] Deps   : NONE (custom inline-hook + GOT-patch engine)")
