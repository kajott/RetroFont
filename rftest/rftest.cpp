#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "gl_header.h"
#include "gl_util.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "retrofont.h"
#include "rftest.h"


int RFTestApp::run(int argc, char *argv[]) {
    uint32_t want_sys_id = 0;
    uint32_t want_font_id = 0;
    if ((argc > 1) && (strlen(argv[1]) == 4)) {  want_sys_id = RF_MAKE_ID_S(argv[1]); }
    if ((argc > 2) && (strlen(argv[2]) == 4)) { want_font_id = RF_MAKE_ID_S(argv[2]); }

    if (!glfwInit()) {
        const char* err = "unknown error";
        glfwGetError(&err);
        fprintf(stderr, "glfwInit failed: %s\n", err);
        return 1;
    }

    glfwWindowHint(GLFW_RED_BITS,     8);
    glfwWindowHint(GLFW_GREEN_BITS,   8);
    glfwWindowHint(GLFW_BLUE_BITS,    8);
    glfwWindowHint(GLFW_ALPHA_BITS,   8);
    glfwWindowHint(GLFW_DEPTH_BITS,   0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    #ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    #endif

    m_window = glfwCreateWindow(
        1024, 768,
        "RetroFont Demo Application",
        nullptr, nullptr);
    if (m_window == nullptr) {
        const char* err = "unknown error";
        glfwGetError(&err);
        fprintf(stderr, "glfwCreateWindow failed: %s\n", err);
        return 1;
    }

    glfwSetWindowUserPointer(m_window, static_cast<void*>(this));
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleKeyEvent(key, scancode, action, mods); });
    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleCharEvent(codepoint); });
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleMouseButtonEvent(button, action, mods); });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleCursorPosEvent(xpos, ypos); });
    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleScrollEvent(xoffset, yoffset); });
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->updateSize(width, height); });

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    #ifdef GL_HEADER_IS_GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            fprintf(stderr, "failed to load OpenGL 3.3 functions\n");
            return 1;
        }
    #else
        #error no valid GL header / loader
    #endif

    if (!GLutil::init()) {
        fprintf(stderr, "OpenGL initialization failed\n");
        return 1;
    }
    GLutil::enableDebugMessages();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    ImGui::CreateContext();
    m_io = &ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(nullptr);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLutil::checkError("texture setup");

    {
        GLutil::Shader vs(GL_VERTEX_SHADER,
             "#version 330 core"
        "\n" "uniform vec4 uArea;"
        "\n" "out vec2 vPos;"
        "\n" "void main() {"
        "\n" "  vec2 pos = vec2(float(gl_VertexID & 1), float((gl_VertexID & 2) >> 1));"
        "\n" "  vPos = pos;"
        "\n" "  gl_Position = vec4(uArea.xy * pos + uArea.zw, 0., 1.);"
        "\n" "}"
        "\n");
        if (!vs.good()) {
            fprintf(stderr, "vertex shader compilation failed\n");
            return 1;
        }
        GLutil::Shader fs(GL_FRAGMENT_SHADER,
             "#version 330 core"
        "\n" "uniform int uMode;"
        "\n" "uniform vec2 uSize;"
        "\n" "uniform sampler2D uTex;"
        "\n" "in vec2 vPos;"
        "\n" "out vec4 oColor;"
        "\n" "void main() {"
        "\n" "  vec2 pos = vPos;"
        "\n" "  if (uMode != 0) {"
        "\n" "    pos *= uSize;\n"
        "\n" "    vec2 i = floor(pos + 0.5);\n"
        "\n" "    pos = (i + clamp((pos - i) / fwidth(pos), -0.5, 0.5)) / uSize;\n"
        "\n" "  }"
        "\n" "  oColor = texture(uTex, pos);"
        "\n" "}"
        "\n");
        if (!fs.good()) {
            fprintf(stderr, "fragment shader compilation failed\n");
            return 1;
        }
        m_prog.link(vs, fs);
        if (!m_prog.good()) {
            fprintf(stderr, "program linking failed\n");
            return 1;
        }
        m_locArea = glGetUniformLocation(m_prog, "uArea");
        m_locMode = glGetUniformLocation(m_prog, "uMode");
        m_locSize = glGetUniformLocation(m_prog, "uSize");
    }

    m_ctx = RF_CreateContext(want_sys_id);
    if (!m_ctx) {
        fprintf(stderr, "ERROR: no such system (sys_id=0x%08X)!\n", want_sys_id);
        return 1;
    }
    m_ctx->insert = true;
    if (want_font_id) {
        if (!RF_SetFont(m_ctx, want_font_id)) {
            fprintf(stderr, "ERROR: no such font (font_id=0x%08X) or incompatible size!\n", want_font_id);
            return 1;
        }
    }
    RF_ResizeScreen(m_ctx, 0, 0, true);
    loadDefaultScreen();

    // main loop
    while (m_active && !glfwWindowShouldClose(m_window)) {
        bool frameRequested = (m_renderFrames > 0);
        if (frameRequested) {
            glfwPollEvents();
            --m_renderFrames;
        } else {
            uint32_t blink = (m_ctx && m_ctx->system) ? m_ctx->system->blink_interval_msec : 0;
            if (blink) {
                uint32_t now = uint32_t(glfwGetTime() * 1000.0);
                uint32_t next = ((now / blink) + 1) * blink;
                glfwWaitEventsTimeout((next - now) * 0.0001);
            } else {
                glfwWaitEvents();
            }
            requestFrames(1);
        }

        // process the UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawUI();
        #ifndef NDEBUG
            if (m_showDemo) {
                ImGui::ShowDemoWindow(&m_showDemo);
            }
        #endif
        ImGui::Render();

        // start display rendering
        GLutil::clearError();
        if (m_ctx) {
            glClearColor(RF_COLOR_R(m_ctx->border_rgb) / 255.0f,
                         RF_COLOR_G(m_ctx->border_rgb) / 255.0f,
                         RF_COLOR_B(m_ctx->border_rgb) / 255.0f,
                         1.0f);
        }
        glViewport(0, 0, int(m_io->DisplaySize.x), int(m_io->DisplaySize.y));
        glClear(GL_COLOR_BUFFER_BIT);

        // draw the screen
        if (m_ctx) {
            // geometry computations
            int x0, y0, x1, y1;
            if (m_borderMode == bmFull) {
                x0 = y0 = 0;
                x1 = m_ctx->bitmap_size.x;
                y1 = m_ctx->bitmap_size.y;
            } else {
                int border = getBorderSize();
                x0 = std::max(m_ctx->main_ul.x - border, 0);
                y0 = std::max(m_ctx->main_ul.y - border, 0);
                x1 = std::min(m_ctx->main_lr.x + border, int(m_ctx->bitmap_size.x));
                y1 = std::min(m_ctx->main_lr.y + border, int(m_ctx->bitmap_size.y));
            }
            double sx, sy, ox, oy;
            if ((m_screenMode == smDynamic) || (m_renderMode == rmIntegral)) {
                // ***** integer scaling *****
                // get (integer) pixel aspect ratio
                int isx = m_ctx->system ? m_ctx->system->coarse_aspect.x : 1;
                int isy = m_ctx->system ? m_ctx->system->coarse_aspect.y : 1;
                // determine zoom (specified upfront, or by zoom-zo-fit)
                int zoom = (m_screenMode == smDynamic) ? m_zoom
                         : std::max(1, std::min(
                                    int(m_io->DisplaySize.x) / ((x1 - x0) * isx),
                                    int(m_io->DisplaySize.y) / ((y1 - y0) * isy)));
                // zoom * aspect = integer size
                isx *= zoom;
                isy *= zoom;
                // integer size -> NDC size
                sx = 2.0 * m_ctx->bitmap_size.x * isx / m_io->DisplaySize.x;
                sy = 2.0 * m_ctx->bitmap_size.y * isy / m_io->DisplaySize.y;
                // compute integer offset
                // (required -- otherwise we might start exactly halfway
                // inside a pixel, causing very nasty scaling artifacts
                // due to critical rounding)
                int iox = int(m_io->DisplaySize.x * 0.5) - isx * ((x0 + x1) >> 1);
                int ioy = int(m_io->DisplaySize.y * 0.5) - isy * ((y0 + y1) >> 1);
                // integer offset -> NDC offset
                ox = 2.0 * iox / m_io->DisplaySize.x - 1.0;
                oy = 2.0 * ioy / m_io->DisplaySize.y - 1.0;
            } else {
                // aspect-correct scaling
                double pixelAspect = m_ctx->pixel_aspect
                                * ((x1 - x0) * m_io->DisplaySize.y)
                                / ((y1 - y0) * m_io->DisplaySize.x);
                if (pixelAspect < 1.0) {
                    // pillarbox
                    sy = 2.0 / (y1 - y0) * m_ctx->bitmap_size.y;
                    sx = sy * pixelAspect;
                } else {
                    // letterbox
                    sx = 2.0 / (x1 - x0) * m_ctx->bitmap_size.x;
                    sy = sx / pixelAspect;
                }
                ox = -0.5 * sx * (x0 + x1) / m_ctx->bitmap_size.x;
                oy = -0.5 * sy * (y0 + y1) / m_ctx->bitmap_size.y;
            }
            m_area[0] =  float(sx);
            m_area[1] = -float(sy);
            m_area[2] =  float(ox);
            m_area[3] = -float(oy);

            // actual drawing
            glUseProgram(m_prog);
            glBindTexture(GL_TEXTURE_2D, m_tex);
            if (RF_Render(m_ctx, uint32_t(glfwGetTime() * 1000.0))) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                             m_ctx->bitmap_size.x, m_ctx->bitmap_size.y,
                             0, GL_RGB, GL_UNSIGNED_BYTE,
                             (const void*) m_ctx->bitmap);
                glUniform2f(m_locSize, float(m_ctx->bitmap_size.x), float(m_ctx->bitmap_size.y));
            }
            GLenum filter = (m_screenMode == smDynamic) || (m_renderMode <= rmUnfiltered) ? GL_NEAREST : GL_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            glUniform1i(m_locMode, ((m_screenMode == smFixed) && (m_renderMode == rmBlocky)) ? 1 : 0);
            glUniform4fv(m_locArea, 1, m_area);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // draw the GUI and finish the frame
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        GLutil::checkError("GUI draw");
        glfwSwapBuffers(m_window);
    }

    // clean up
    #ifndef NDEBUG
        fprintf(stderr, "exiting ...\n");
    #endif
    RF_FreeContext(m_ctx);
    glUseProgram(0);
    m_prog.free();
    GLutil::done();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
    #ifndef NDEBUG
        fprintf(stderr, "bye!\n");
    #endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void RFTestApp::handleKeyEvent(int key, int scancode, int action, int mods) {
    (void)scancode, (void)mods;
    if (((action != GLFW_PRESS) && (action != GLFW_REPEAT)) || m_io->WantCaptureKeyboard) { return; }
    switch (key) {
        case GLFW_KEY_LEFT:
            if (m_ctx) {
                RF_Coord c = m_ctx->cursor_pos;
                if (c.x) { --c.x; } else {
                    c.x = m_ctx->screen_size.x - 1;
                    if (c.y) { --c.y; } else { c.y = m_ctx->screen_size.y - 1; }
                }
                RF_MoveCursor(m_ctx, c.x, c.y);
            }
            break;
        case GLFW_KEY_RIGHT:
            if (m_ctx) {
                RF_Coord c = m_ctx->cursor_pos;
                if ((c.x + 1) < m_ctx->screen_size.x) { ++c.x; } else {
                    c.x = 0;
                    if ((c.y + 1) < m_ctx->screen_size.y) { ++c.y; } else { c.y = 0; }
                }
                RF_MoveCursor(m_ctx, c.x, c.y);
            }
            break;
        case GLFW_KEY_UP:
            if (m_ctx && m_ctx->cursor_pos.y) {
                RF_MoveCursor(m_ctx, m_ctx->cursor_pos.x, m_ctx->cursor_pos.y - 1);
            }
            break;
        case GLFW_KEY_DOWN:
            if (m_ctx && ((m_ctx->cursor_pos.y + 1) < m_ctx->screen_size.y)) {
                RF_MoveCursor(m_ctx, m_ctx->cursor_pos.x, m_ctx->cursor_pos.y + 1);
            }
            break;
        case GLFW_KEY_HOME:
            if (m_ctx) {
                RF_MoveCursor(m_ctx, 0, m_ctx->cursor_pos.y);
            }
            break;
        case GLFW_KEY_END:
            if (m_ctx && m_ctx->screen && (m_ctx->cursor_pos.y < m_ctx->screen_size.y)) {
                uint16_t x = m_ctx->screen_size.x - 1;
                const RF_Cell* pos = &m_ctx->screen[m_ctx->cursor_pos.y * m_ctx->screen_size.x + x];
                while (x && ((pos->codepoint == 32) || (pos->codepoint == 0))) {
                    --x;  --pos;
                }
                if ((x + 1) < m_ctx->screen_size.x) { ++x; }
                RF_MoveCursor(m_ctx, x, m_ctx->cursor_pos.y);
            }
            break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_ENTER);     m_screenContentsChanged = true; }
            break;
        case GLFW_KEY_TAB:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_TAB);       m_screenContentsChanged = true; }
            break;
        case GLFW_KEY_BACKSPACE:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_BACKSPACE); m_screenContentsChanged = true; }
            break;
        case GLFW_KEY_DELETE:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_DELETE);    m_screenContentsChanged = true; }
            break;
        case GLFW_KEY_INSERT:
            if (m_ctx) {
                m_ctx->insert = !m_ctx->insert;
                // force-update cursor after changing insert mode
                RF_MoveCursor(m_ctx, m_ctx->cursor_pos.x, m_ctx->cursor_pos.y);
            }
            #ifndef NDEBUG
                printf("insert mode is %s\n", !m_ctx ? "UNKNOWN" : m_ctx->insert ? "ON" : "OFF");
            #endif
            break;
        case GLFW_KEY_F9:
            m_showDemo = !m_showDemo;
            requestFrames(2);
            break;
        default:
            break;
    }
}

void RFTestApp::handleCharEvent(unsigned int codepoint) {
    RF_AddChar(m_ctx, codepoint);
    m_screenContentsChanged = true;
}

void RFTestApp::handleMouseButtonEvent(int button, int action, int mods) {
    (void)mods;
    if (m_io->WantCaptureMouse) { return; }
    if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_1) && m_ctx) {
        double fx = 0.0, fy = 0.0;
        // transform pixel coordinates to NDC
        glfwGetCursorPos(m_window, &fx, &fy);
        fx = 2.0 * fx / m_io->DisplaySize.x - 1.0;
        fy = 1.0 - 2.0 * fy / m_io->DisplaySize.y;
        // transform to texture (pixel) coordinates
        int x = int((fx - m_area[2]) / m_area[0] * m_ctx->bitmap_size.x);
        int y = int((fy - m_area[3]) / m_area[1] * m_ctx->bitmap_size.y);
        // transform to cell coordinates
        x = (x - m_ctx->main_ul.x) / m_ctx->cell_size.x;
        y = (y - m_ctx->main_ul.y) / m_ctx->cell_size.y;
        // set cursor, if in bounds
        if ((x >= 0) && (x < int(m_ctx->screen_size.x))
        &&  (y >= 0) && (y < int(m_ctx->screen_size.y))) {
            RF_MoveCursor(m_ctx, uint16_t(x), uint16_t(y));
        }
    }
}

void RFTestApp::handleCursorPosEvent(double xpos, double ypos) {
    (void)xpos, (void)ypos;
}

void RFTestApp::handleScrollEvent(double xoffset, double yoffset) {
    (void)xoffset, (void)yoffset;
    if (m_io->WantCaptureMouse) { return; }
}

int RFTestApp::getBorderSize() const {
    switch (m_borderMode) {
        case bmFull:       return (m_ctx && m_ctx->system) ? ((m_ctx->system->border_ul.x + m_ctx->system->border_ul.y + m_ctx->system->border_lr.x + m_ctx->system->border_lr.y + 2) >> 2) : 8;
        case bmReduced:    return (m_ctx && m_ctx->font) ? std::min(m_ctx->font->font_size.x, m_ctx->font->font_size.y) : 4;
        case bmMinimal:    return 2;
        default/*bmNone*/: return 0;
    }
}

void RFTestApp::updateSize(bool force, bool forceDefault) {
    updateSize(int(m_io->DisplaySize.x), int(m_io->DisplaySize.y), force, forceDefault);
}

void RFTestApp::updateSize(int width, int height, bool force, bool forceDefault) {
    if (!m_ctx || !m_ctx->system || !m_ctx->font) { return; }
    if (m_screenMode == smDynamic) {
        // divide by zoom
        width  /= m_zoom * m_ctx->system->coarse_aspect.x;
        height /= m_zoom * m_ctx->system->coarse_aspect.y;
        // subtract borders
        width  -= (m_borderMode == bmFull) ? (m_ctx->system->border_ul.x + m_ctx->system->border_lr.x) : getBorderSize();
        height -= (m_borderMode == bmFull) ? (m_ctx->system->border_ul.y + m_ctx->system->border_lr.y) : getBorderSize();
        // divide by cell size
        width  /= m_ctx->system->cell_size.x + (m_ctx->system->font_size.x ? 0 : m_ctx->font->font_size.x);
        height /= m_ctx->system->cell_size.y + (m_ctx->system->font_size.y ? 0 : m_ctx->font->font_size.y);
        // clamp to at least one cell
        width  = std::max(1, width);
        height = std::max(1, height);
    } else if (force) {
        width = height = forceDefault ? RF_SIZE_DEFAULT : 0;
    } else {
        return;  // no change
    }
    // activate
    RF_ResizeScreen(m_ctx, uint16_t(width), uint16_t(height), true);
    #ifndef NDEBUG
        printf("set screen size: %dx%d cells -> %dx%d pixels\n", m_ctx->screen_size.x, m_ctx->screen_size.y, m_ctx->bitmap_size.x, m_ctx->bitmap_size.y);
    #endif
}

void RFTestApp::loadDefaultScreen() {
    if (m_defaultScreen == dsKeep) {
        return;  // do nothing; don't even touch m_screenContentsChanged!
    }
    if ((m_defaultScreen == dsEmpty) || (m_defaultScreen == dsDefault)) {
        RF_ClearAll(m_ctx);
    }
    RF_MoveCursor(m_ctx, 0, 0);
    if (m_defaultScreen == dsDemo) {
        srand(0x13375EED);
        RF_DemoScreen(m_ctx);
    }
    m_screenContentsChanged = false;
}

////////////////////////////////////////////////////////////////////////////////

void RFTestApp::drawUI() {
    // main window begin
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(556.0f, 170.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, 0)) {

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("on system switch:");
        ImGui::SameLine();
        ImGui::PushItemWidth(236.0f);
        if (ImGui::BeginCombo("##defscreen", DefaultScreenStrings[m_defaultScreen])) {
            for (int i = 0;  i < int(DefaultScreenStrings.size());  ++i) {
                bool sel = (i == m_defaultScreen);
                if (ImGui::Selectable(DefaultScreenStrings[i], &sel)) {
                    m_defaultScreen = i;
                    if (!m_screenContentsChanged) {
                        loadDefaultScreen();
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if (ImGui::BeginCombo("system", (m_ctx && m_ctx->system) ? m_ctx->system->name : "???", 0)) {
            for (const RF_System* const* p_sys = RF_SystemList;  *p_sys;  ++p_sys) {
                if (ImGui::Selectable((*p_sys)->name, m_ctx && (m_ctx->system == *p_sys))) {
                    if (RF_SetSystem(m_ctx, (*p_sys)->sys_id)) {
                        updateSize(true, true);
                        loadDefaultScreen();
                    }
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("font", (m_ctx && m_ctx->font) ? m_ctx->font->name : "???", 0)) {
            for (const RF_Font* font = RF_FontList;  font->font_id;  ++font) {
                if (!RF_CanUseFont(m_ctx, font)) { continue; }
                if (ImGui::Selectable(font->name, m_ctx && (m_ctx->font == font))) {
                    if (RF_SetFont(m_ctx, font->font_id)) {
                        updateSize(true, (m_screenMode == smFixed));
                    }
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::SliderInt("borders", &m_borderMode, bmNone, bmFull, BorderModeStrings[m_borderMode]))
            { updateSize(); }
        if (ImGui::SliderInt("screen size", &m_screenMode, smFixed, smDynamic, ScreenModeStrings[m_screenMode])) {
            bool backToFixed = (m_screenMode == smFixed);
            updateSize(backToFixed, backToFixed);
        }
        if (m_screenMode == smFixed) {
            ImGui::SliderInt("scaling mode", &m_renderMode, rmIntegral, rmSmooth, RenderModeStrings[m_renderMode]);
        } else {
            if (ImGui::SliderInt("zoom", &m_zoom, 1, 4, "%dx")) { updateSize(); }
        }
    }
    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    static RFTestApp app;
    return app.run(argc, argv);
}
