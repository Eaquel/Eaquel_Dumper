cmake_minimum_required(VERSION 4.3.1)

if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME must be set: -DMODULE_NAME=eaquel_dumper")
endif()

project(${MODULE_NAME}
    LANGUAGES    CXX
    VERSION      1.0.0
    DESCRIPTION  "Eaquel IL2CPP Dumper — ReZygisk Stealth Architecture v1.0"
    HOMEPAGE_URL "https://github.com/Eaquel/Eaquel_Dumper"
)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EAQUEL_MIN_API 30)
set(EAQUEL_MAX_API 37)

if(NOT DEFINED ANDROID_PLATFORM OR ANDROID_PLATFORM LESS ${EAQUEL_MIN_API})
    message(FATAL_ERROR
        "Minimum supported Android API is ${EAQUEL_MIN_API} (Android 11). Got: ${ANDROID_PLATFORM}")
endif()
if(ANDROID_PLATFORM GREATER ${EAQUEL_MAX_API})
    message(WARNING
        "Android API ${ANDROID_PLATFORM} exceeds tested maximum (${EAQUEL_MAX_API} / Android 17)")
endif()

set(EAQUEL_ARCH    "unknown")
set(EAQUEL_IS_64BIT OFF)
set(EAQUEL_IS_EMU   OFF)

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(EAQUEL_ARCH    "arm64")
    set(EAQUEL_IS_64BIT ON)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(EAQUEL_ARCH "arm32")
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(EAQUEL_ARCH    "x86_64")
    set(EAQUEL_IS_64BIT ON)
    set(EAQUEL_IS_EMU   ON)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(EAQUEL_ARCH "x86")
    set(EAQUEL_IS_EMU ON)
else()
    message(WARNING "Unknown ABI '${CMAKE_ANDROID_ARCH_ABI}' — proceeding with generic settings")
endif()

message(STATUS "[Eaquel v1] ABI=${CMAKE_ANDROID_ARCH_ABI}  ARCH=${EAQUEL_ARCH}  64BIT=${EAQUEL_IS_64BIT}  EMU=${EAQUEL_IS_EMU}  BUILD=${CMAKE_BUILD_TYPE}")

set(EAQUEL_COMMON_FLAGS
    -Werror=format
    -fdata-sections
    -ffunction-sections
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
    -DEAQUEL_VERSION_MAJOR=1
    -DEAQUEL_VERSION_MINOR=0
    -DANDROID_MIN_API=${EAQUEL_MIN_API}
    -DANDROID_MAX_API=${EAQUEL_MAX_API}
)

set(EAQUEL_CXX_FLAGS
    -fno-exceptions
    -fno-rtti
    -Wnull-dereference
    -Woverloaded-virtual
    -Wno-unused-result
)

if(EAQUEL_ARCH STREQUAL "arm64")
    list(APPEND EAQUEL_COMMON_FLAGS
        -march=armv8-a+simd
        -DEAQUEL_ARM64=1
        -DEAQUEL_64BIT=1
    )
elseif(EAQUEL_ARCH STREQUAL "arm32")
    list(APPEND EAQUEL_COMMON_FLAGS
        -mthumb
        -march=armv7-a
        -mfloat-abi=softfp
        -mfpu=neon
        -DEAQUEL_ARM32=1
        -DEAQUEL_32BIT=1
    )
elseif(EAQUEL_ARCH STREQUAL "x86_64")
    list(APPEND EAQUEL_COMMON_FLAGS
        -march=x86-64
        -DEAQUEL_X86_64=1
        -DEAQUEL_EMULATOR=1
        -DEAQUEL_64BIT=1
    )
elseif(EAQUEL_ARCH STREQUAL "x86")
    list(APPEND EAQUEL_COMMON_FLAGS
        -march=i686
        -msse4.2
        -DEAQUEL_X86=1
        -DEAQUEL_EMULATOR=1
        -DEAQUEL_32BIT=1
    )
endif()

set(EAQUEL_LINKER_FLAGS
    -Wl,--hash-style=both
    -Wl,-z,noexecstack
)

if(NOT EAQUEL_IS_EMU)
    list(APPEND EAQUEL_LINKER_FLAGS -Wl,-z,max-page-size=16384)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND EAQUEL_COMMON_FLAGS
        -O3
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -flto=thin
    )
    if(EAQUEL_ARCH STREQUAL "arm64")
        list(APPEND EAQUEL_COMMON_FLAGS -mbranch-protection=standard)
    endif()
    list(APPEND EAQUEL_LINKER_FLAGS
        -Wl,-exclude-libs,ALL
        -Wl,--gc-sections
        -Wl,--strip-all
        -Wl,-z,relro
        -Wl,-z,now
    )
else()
    list(APPEND EAQUEL_COMMON_FLAGS -O0 -g -DDEBUG_BUILD)
endif()

list(JOIN EAQUEL_COMMON_FLAGS " " _common_str)
list(JOIN EAQUEL_CXX_FLAGS    " " _cxx_str)
list(JOIN EAQUEL_LINKER_FLAGS " " _ld_str)

string(APPEND CMAKE_C_FLAGS             " ${_common_str}")
string(APPEND CMAKE_CXX_FLAGS           " ${_common_str} ${_cxx_str}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${_ld_str}")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${_ld_str}")

add_library(${MODULE_NAME} SHARED
    Dumper.cpp
)

target_include_directories(${MODULE_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        log
        android
)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
    POSITION_INDEPENDENT_CODE ON
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
        COMMENT "Stripping ${MODULE_NAME}"
    )
endif()

message(STATUS "[Eaquel v1] Target      : ${MODULE_NAME}")
message(STATUS "[Eaquel v1] Arch        : ${EAQUEL_ARCH}")
message(STATUS "[Eaquel v1] 64-bit      : ${EAQUEL_IS_64BIT}")
message(STATUS "[Eaquel v1] Emulator    : ${EAQUEL_IS_EMU}")
message(STATUS "[Eaquel v1] Build       : ${CMAKE_BUILD_TYPE}")
message(STATUS "[Eaquel v1] CXX         : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[Eaquel v1] Android API : ${ANDROID_PLATFORM} (range ${EAQUEL_MIN_API}-${EAQUEL_MAX_API})")
