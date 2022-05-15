cmake_minimum_required(VERSION 3.15)

project (retrofont)

# set default build type
set (default_build_type "Debug")
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set (CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "build type" FORCE)
        set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    endif ()
    message (STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
endif ()
message (STATUS "Compiler Type: ${CMAKE_CXX_COMPILER_ID}")

# set C/C++ language standards for compilers that respect them
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# use the static C library with MSVC builds
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()


###############################################################################
## THIRD-PARTY LIBRARIES                                                    ##
###############################################################################

add_library (rf_thirdparty STATIC
    thirdparty/libtmt/tmt.c
)

target_include_directories (rf_thirdparty PUBLIC
    thirdparty/libtmt
)


###############################################################################
## RETROFONT CORE LIBRARY                                                    ##
###############################################################################

set (SYSTEMS
    retrofont/src/sys_generic.c
    retrofont/src/sys_pc.c
    retrofont/src/sys_c64.c
    retrofont/src/sys_zx.c
)
file (GLOB FONTSPECS fonts.in/*.fontspec)

add_library (retrofont STATIC
    retrofont/src/rfcore.c
    retrofont/src/fonts.c
    retrofont/src/systems.c
    ${SYSTEMS}
)

target_include_directories (retrofont PUBLIC
    retrofont/include
)

add_custom_command (
    OUTPUT ${CMAKE_SOURCE_DIR}/retrofont/src/fonts.c
    COMMAND python3 font_import.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/util
    DEPENDS ${FONTSPECS}
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/util/font_import.py
)

add_custom_command (
    OUTPUT ${CMAKE_SOURCE_DIR}/retrofont/src/systems.c
    COMMAND python3 update_systems.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/util
    DEPENDS ${SYSTEMS}
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/util/update_systems.py
)


###############################################################################
## TEST APPLICATION                                                          ##
###############################################################################

add_executable (rftest
    rftest/rftest.c
)

target_link_libraries (rftest retrofont rf_thirdparty)


###############################################################################
## COMPILER OPTIONS                                                          ##
###############################################################################

if (NOT MSVC)
    target_compile_options (retrofont PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
    target_compile_options (rftest PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (retrofont PRIVATE /W4 /WX)
    target_compile_options (rftest PRIVATE /W4 /WX)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT MSVC)
        message (STATUS "Debug build, enabling Address Sanitizer")
        target_compile_options (rftest PUBLIC "-fsanitize=address")
        target_compile_options (retrofont PRIVATE "-fsanitize=address")
        target_compile_options (rf_thirdparty PRIVATE "-fsanitize=address")
        target_link_options (rftest PRIVATE "-fsanitize=address")
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            message (STATUS "Clang Debug build, enabling Undefined Behavior Sanitizer")
            target_compile_options (rftest PRIVATE "-fsanitize=undefined")
        endif ()
    elseif (MSVC_VERSION GREATER 1627 AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message (STATUS "Debug build and MSVC 16.8 or greater detected, enabling Address Sanitizer")
        target_compile_options (rftest PRIVATE "/fsanitize=address")
        target_compile_options (retrofont PRIVATE "/fsanitize=address")
        target_compile_options (rf_thirdparty PUBLIC "/fsanitize=address")
        target_link_options (rftest PRIVATE "/DEBUG")
        # ASAN isn't compatible with the /RTC switch and incremental linking,
        # both of which CMake enables by default
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string (REGEX REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    endif ()
endif ()
