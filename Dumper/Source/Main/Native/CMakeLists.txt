cmake_minimum_required(VERSION 3.22.1)

if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME is not defined")
endif()

project(${MODULE_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections")
set(CXX_FLAGS "-fno-exceptions -fno-rtti")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND C_FLAGS " -O3 -fvisibility=hidden -fvisibility-inlines-hidden")
    string(APPEND LINKER_FLAGS " -Wl,-exclude-libs,ALL -Wl,--gc-sections -Wl,--strip-all")
else()
    string(APPEND C_FLAGS " -O0")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS} ${CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

find_package(xdl REQUIRED CONFIG)

add_library(${MODULE_NAME} SHARED
    Main.cpp
    Hack.cpp
    Dumper.cpp
)

target_link_libraries(${MODULE_NAME}
    xdl::xdl
    log
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP}
            --strip-all
            --remove-section=.comment
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${MODULE_NAME}.so"
    )
endif()
