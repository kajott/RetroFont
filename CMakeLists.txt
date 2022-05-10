cmake_minimum_required(VERSION 3.15)

project (retrofont)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

add_executable (rftest
    src/rftest.c
    src/rfcore.c
    src/sys_zx.c
    src/sys_c64.c
    src/fonts.c
)
add_library (retrofont_thirdparty STATIC
    thirdparty/libtmt/tmt.c
)

target_include_directories (rftest PRIVATE
    src
)
target_include_directories (retrofont_thirdparty PUBLIC
    thirdparty/libtmt
)

target_link_libraries (rftest retrofont_thirdparty)

if (NOT MSVC)
    target_compile_options (rftest PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (rftest PRIVATE /W4 /WX)
endif ()

file (GLOB FONTSPECS fonts.in/*.fontspec)
add_custom_command (
    OUTPUT ${CMAKE_SOURCE_DIR}/src/fonts.c
    COMMAND python3 font_import.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/util
    DEPENDS ${FONTSPECS}
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/util/font_import.py
)
