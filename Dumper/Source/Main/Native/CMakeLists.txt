cmake_minimum_required(VERSION 3.22.1)

if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME is not defined")
endif()

project(${MODULE_NAME} LANGUAGES CXX)

# ── Standard ──────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Base flags ────────────────────────────────────────────────────────────────
set(C_FLAGS
    "-Werror=format"
    "-fdata-sections"
    "-ffunction-sections"
    # Crash safety: stack canary + fortify
    "-fstack-protector-strong"
    "-D_FORTIFY_SOURCE=2"
)

set(CXX_FLAGS
    # No exceptions / RTTI (Zygisk modules must not throw)
    "-fno-exceptions"
    "-fno-rtti"
    # Warn on common null-deref patterns
    "-Wnull-dereference"
)

set(LINKER_FLAGS
    "-ffixed-x18"
    "-Wl,--hash-style=both"
)

# ── Release hardening ─────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND C_FLAGS
        "-O3"
        "-fvisibility=hidden"
        "-fvisibility-inlines-hidden"
        # Link-time optimisation — allows inlining across TU boundaries
        "-flto=thin"
    )
    list(APPEND LINKER_FLAGS
        "-Wl,-exclude-libs,ALL"
        "-Wl,--gc-sections"
        "-Wl,--strip-all"
        # Full RELRO + immediate binding = harder to exploit
        "-Wl,-z,relro"
        "-Wl,-z,now"
    )
else()
    list(APPEND C_FLAGS "-O0" "-g")
endif()

# Convert lists to space-separated strings for CMake flag variables
string(JOIN " " C_FLAGS_STR   ${C_FLAGS})
string(JOIN " " CXX_FLAGS_STR ${CXX_FLAGS})
string(JOIN " " LD_FLAGS_STR  ${LINKER_FLAGS})

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${C_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_STR} ${CXX_FLAGS_STR}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LD_FLAGS_STR}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LD_FLAGS_STR}")

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(xdl REQUIRED CONFIG)

# ── Target ────────────────────────────────────────────────────────────────────
add_library(${MODULE_NAME} SHARED
    Dumper.cpp
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${MODULE_NAME}
    xdl::xdl
    log
    # android — for android_get_device_api_level
    android
)

# ── Post-build strip (release only) ──────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP}
            --strip-all
            --remove-section=.comment
            --remove-section=.note
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${MODULE_NAME}.so"
        COMMENT "Stripping ${MODULE_NAME}"
    )
endif()
