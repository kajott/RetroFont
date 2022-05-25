#pragma once

#include <cstdint>

#include <array>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "gl_util.h"
#include "gl_header.h"
#include "imgui.h"

#include "retrofont.h"

class RFTestApp {
    // GLFW and ImGui stuff
    GLFWwindow* m_window = nullptr;
    ImGuiIO* m_io = nullptr;

    // RetroFont stuff
    RF_Context* m_ctx = nullptr;
    GLuint m_tex = 0;
    GLutil::Program m_prog;
    GLint m_locArea;
    GLint m_locMode;
    GLint m_locSize;

    // UI state
    bool m_active = true;
    int m_renderFrames = 2;
    bool m_showDemo = false;

    // border mode
    static constexpr int bmNone    = 0;
    static constexpr int bmMinimal = 1;
    static constexpr int bmReduced = 2;
    static constexpr int bmFull    = 3;
    inline static constexpr std::array<const char*, 4> BorderModeStrings = {{ "no", "minimal", "reduced", "full" }};
    int m_borderMode = bmFull;

    // screen mode
    static constexpr int smFixed   = 0;
    static constexpr int smDynamic = 1;
    inline static constexpr std::array<const char*, 2> ScreenModeStrings = {{ "fixed", "dynamic" }};
    int m_screenMode = smFixed;

    // render modes / zoom
    static constexpr int rmIntegral   = 0;
    static constexpr int rmUnfiltered = 1;
    static constexpr int rmBlocky     = 2;
    static constexpr int rmSmooth     = 3;
    inline static constexpr std::array<const char*, 4> RenderModeStrings = {{ "integer (w/o aspect ratio)", "nearest-neighbor", "antialiased", "smooth" }};
    int m_renderMode = rmBlocky;
    int m_zoom = 1;

    // UI functions
    void drawUI();

    // internal functions
    int getBorderSize() const;
    void updateSize(bool force=false, bool forceDefault=false);
    void updateSize(int width, int height, bool force=false, bool forceDefault=false);

    // event handling
    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleMouseButtonEvent(int button, int action, int mods);
    void handleCursorPosEvent(double xpos, double ypos);
    void handleScrollEvent(double xoffset, double yoffset);

public:
    inline RFTestApp() {}
    int run(int argc, char* argv[]);

    inline void requestFrames(int n)
        { if (n > m_renderFrames) { m_renderFrames = n; } }
};
