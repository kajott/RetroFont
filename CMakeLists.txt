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
set (CMAKE_CXX_STANDARD 17)

# use the static C library with MSVC builds
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()


###############################################################################
## THIRD-PARTY LIBRARIES                                                    ##
###############################################################################

# add the GLFW library with suitable options
foreach (disable_ GLFW_BUILD_EXAMPLES GLFW_BUILD_TESTS GLFW_BUILD_DOCS GLFW_INSTALL)
    option ("${disable_}" OFF)
endforeach ()
add_subdirectory (thirdparty/glfw)


add_library (rf_thirdparty STATIC
    thirdparty/glad/src/glad.c
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/backends/imgui_impl_glfw.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories (rf_thirdparty PUBLIC
    thirdparty/glad/include
    thirdparty/imgui
    thirdparty/imgui/backends
)

target_link_libraries (rf_thirdparty glfw)
if (WIN32)
    target_link_libraries (rf_thirdparty opengl32)
else ()
    target_link_libraries (rf_thirdparty m dl GL)
endif ()


###############################################################################
## RETROFONT CORE LIBRARY                                                    ##
###############################################################################

set (SYSTEMS
    retrofont/src/sys_generic.c
    retrofont/src/sys_pc.c
    retrofont/src/sys_amiga.c
    retrofont/src/sys_st.c
    retrofont/src/sys_apple2.c
    retrofont/src/sys_cbm.c
    retrofont/src/sys_atari8.c
    retrofont/src/sys_zx.c
    retrofont/src/sys_cpc.c
    retrofont/src/sys_bbc.c
    retrofont/src/sys_robotron.c
)

set (FONTSPECS
    fonts.in/pc.fontspec
    fonts.in/amiga.fontspec
    fonts.in/st.fontspec
    fonts.in/apple2.fontspec
    fonts.in/cbm.fontspec
    fonts.in/atari8.fontspec
    fonts.in/zx.fontspec
    fonts.in/bbc.fontspec
    fonts.in/cpc.fontspec
    fonts.in/robotron.fontspec
)

add_library (retrofont STATIC
    retrofont/src/rfcore.c
    retrofont/src/rfparse_int.c
    retrofont/src/fonts.c
    retrofont/src/systems.c
    retrofont/src/fallbacks.c
    ${SYSTEMS}
)

target_include_directories (retrofont PUBLIC
    retrofont/include
)

add_custom_command (
    OUTPUT ${CMAKE_SOURCE_DIR}/retrofont/src/fonts.c
    COMMAND python3 util/font_import.py ${FONTSPECS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
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

add_custom_command (
    OUTPUT ${CMAKE_SOURCE_DIR}/retrofont/src/fallbacks.c
    COMMAND python3 ${CMAKE_SOURCE_DIR}/util/generate_fallback_map.py -q ${CMAKE_SOURCE_DIR}/retrofont/src/fallbacks.c
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/util/generate_fallback_map.py
)


###############################################################################
## TEST APPLICATION                                                          ##
###############################################################################

add_executable (rftest
    rftest/rftest.cpp
    rftest/gl_util.cpp
    rftest/string_util.cpp
)

target_include_directories (rftest PRIVATE rftest)

target_link_libraries (rftest retrofont rf_thirdparty)

# platform-dependent additional sources and options
if (WIN32)
    # no console in Win32 Release builds
    # (disabled for now -- we need a WinMain() function first!)
    #if (CMAKE_BUILD_TYPE STREQUAL "Release")
    #    set_target_properties (rftest PROPERTIES WIN32_EXECUTABLE ON)
    #endif ()
else ()
    # Linux builds *require* (p)threads, otherwise very weird things can happen
    set (THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package (Threads REQUIRED)
    target_link_libraries (rftest Threads::Threads)
endif ()


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

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
    #                                 ^^^^^^^^^^^^^ ASAN and nVidia's OpenGL driver don't mix well!
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
