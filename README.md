# RetroFont: pixel-accurate rendering of retrocomputing screens

RetroFont is a *library* and an *application* that tries to emulate the font rendering of various 8-bit and 16-bit home computer systems as closely as possible.

It is, however, **not** an emulator! Only the visual aspect of text rendering is replicated, nothing else.

## Features

- many systems supported
  - see the [Systems](Systems.md) document for details
- best-effort emulation of each system's rendering capabilities
  - colors and special styles (bold, underline) are limited to what the system could support
  - on systems that support programmable color palettes, only the default palette of the system's standard OS is simulated
- best-effort emulation of the system's image geometry and pixel aspect ratio
  - on systems that support programmable video timing, only the default modes of the system's standard OS are simulated
  - optionally, rendering in arbitrary screen sizes or with reduced (or fully omitted) borders is possible
    - caveat: arbitrary-size rendering loses aspect ratio accuracy
- pixel-perfect fonts, in most cases extracted straight from the system's ROMs
- realistic startup screens for each system
  - can also load other system's startup screens, for "what whould System A's boot screen look like on System B" simulations
- fully Unicode internally
  - all fancy pseudographics characters are mapped to their Unicode codepoints
  - if a font doesn't support a codepoint (and none supports all of them!), fall-back to suitable replacement characters or other fonts of the same size is possible
- screen can be tinted in green or amber for "realistic" monochrome systems


## Build Prerequisites

Any somewhat modern system with a decent C/C++ compiler, Python >= 3.6, and OpenGL support should do. It has been tested with GCC 10 and 11 as well as Clang 13 on Linux, and Microsoft Visual Studio 2019.

The build system is based on CMake and shouldn't contain any surprises. Just make sure that you clone the repository recursively, otherwise the required third-party libraties ([GLFW](https://www.glfw.org) and [Dear ImGui](https://github.com/ocornut/imgui)) will be missing.
