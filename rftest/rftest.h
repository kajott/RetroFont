#pragma once

#include <cstdint>

#include <array>
#include <functional>

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

    // rendering stuff
    GLuint m_tex = 0;
    GLutil::Program m_prog;
    GLfloat m_area[4];
    GLint m_locArea;
    GLint m_locMode;
    GLint m_locSize;
    GLint m_locTint;

    // UI state
    bool m_active = true;
    int m_renderFrames = 2;
    int m_baud = 0;
    bool m_showUI = true;
    bool m_showDemo = false;
    bool m_screenContentsChanged = false;

    // automatic typewriter state
    char* m_typerStr = nullptr;
    int m_typerPos = 0;
    RF_MarkupType m_typerType = RF_MT_NONE;
    double m_typerStartTime = 0.0;
    int m_typerStartPos = 0;

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

    // monitor type
    static constexpr int mtAuto = 0;
    static constexpr int mtOffset = 1;
    static constexpr int mtMax = int(_RF_MONITOR_COUNT) + mtOffset;
    inline static constexpr std::array<const char*, mtMax> MonitorTypeStrings = {{
        "automatic", "color",
        "green monochrome", "long-persistence green",
        "amber monochrome", "white monochrome",
        "red plasma"
    }};
    int m_monitorType = mtAuto;

    // default screen
    static constexpr int dsAsConfigured = -1;
    static constexpr int dsEmpty   = 0;
    static constexpr int dsKeep    = 1;
    static constexpr int dsDefault = 2;
    static constexpr int dsDemo    = 3;
    inline static constexpr std::array<const char*, 4> DefaultScreenStrings = {{ "clear screen", "keep previous contents", "load system default screen", "load attribute test screen" }};
    int m_defaultScreen = dsDefault;

    // UI functions
    void drawUI();
    void colorUI(const char* title, uint32_t color, std::function<void(uint32_t color)> setter);

    // internal functions
    int getBorderSize() const;
    void updateSize(bool force=false, bool forceDefault=false);
    void updateSize(int width, int height, bool force=false, bool forceDefault=false);
    void loadDefaultScreen(int type=dsAsConfigured);
    void loadScreen(const char* text, RF_MarkupType markup=RF_MT_NONE);
    void cancelTyper();
    int getTyperPos();

    // event handling
    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleCharEvent(unsigned int codepoint);
    void handleMouseButtonEvent(int button, int action, int mods);
    void handleCursorPosEvent(double xpos, double ypos);
    void handleScrollEvent(double xoffset, double yoffset);

public:
    inline RFTestApp() {}
    int run(int argc, char* argv[]);

    inline void requestFrames(int n)
        { if (n > m_renderFrames) { m_renderFrames = n; } }
};
