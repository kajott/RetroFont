cmake_minimum_required(VERSION 3.15)

project (retrofont)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)


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

if (NOT MSVC)
    target_compile_options (retrofont PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (retrofont PRIVATE /W4 /WX)
endif ()


###############################################################################
## TEST APPLICATION                                                          ##
###############################################################################

add_executable (rftest
    rftest/rftest.c
)

target_link_libraries (rftest retrofont rf_thirdparty)

if (NOT MSVC)
    target_compile_options (rftest PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (rftest PRIVATE /W4 /WX)
endif ()
