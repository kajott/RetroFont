#pragma once

#include <cstdint>
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

    // UI state
    bool m_active = true;
    int m_renderFrames = 2;
    bool m_showDemo = false;

    // UI functions
    void drawUI();

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
