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

#include "string_util.h"

#include "retrofont.h"
#include "rftest.h"

static const char* DefaultDefaultScreen =
    "Welcome to `fcR`fae`f9t`fer`fbo`fdF`f#80ff00o`f#ff3700n`f#11aafft`0!\n\n"
;

static const GLfloat MonitorTints[_RF_MONITOR_COUNT][4] = {
    { 1.00f, 1.00f, 1.00f, 0.f },  // RF_MONITOR_COLOR
    { 0.00f, 1.00f, 0.00f, 1.f },  // RF_MONITOR_GREEN
    { 0.00f, 1.00f, 0.25f, 1.f },  // RF_MONITOR_LONG
    { 1.25f, 1.00f, 0.25f, 1.f },  // RF_MONITOR_AMBER
    { 1.00f, 1.00f, 0.95f, 1.f },  // RF_MONITOR_WHITE
    { 1.00f, 0.00f, 0.00f, 1.f },  // RF_MONITOR_RED
};

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
    glfwSetDropCallback(m_window, [](GLFWwindow* window, int path_count, const char* paths[])
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleDropEvent(path_count, paths); });

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
        "\n" "uniform vec4 uTint;"
        "\n" "in vec2 vPos;"
        "\n" "out vec4 oColor;"
        "\n" "void main() {"
        "\n" "  vec2 pos = vPos;"
        "\n" "  if (uMode != 0) {"
        "\n" "    pos *= uSize;"
        "\n" "    vec2 i = floor(pos + 0.5);"
        "\n" "    pos = (i + clamp((pos - i) / fwidth(pos), -0.5, 0.5)) / uSize;"
        "\n" "  }"
        "\n" "  vec3 c = texture(uTex, pos).rgb;"
        "\n" "  if (uTint.a > 0.5) {"
        "\n" "    c = vec3(dot(vec3(0.25, 0.5, 0.25), c));"
        "\n" "  }"
        "\n" "  oColor = vec4(c * uTint.rgb, 1.0);"
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
        m_locTint = glGetUniformLocation(m_prog, "uTint");
    }

    m_ctx = RF_CreateContext(want_sys_id);
    if (!m_ctx) {
        fprintf(stderr, "ERROR: no such system (sys_id=0x%08X)!\n", want_sys_id);
        return 1;
    }
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
        if (m_showUI) {
            drawUI();
        }
        #ifndef NDEBUG
            if (m_showDemo) {
                ImGui::ShowDemoWindow(&m_showDemo);
            }
        #endif
        ImGui::Render();

        // update typewriter
        if (m_typerStr) {
            int start = m_typerPos;
            int end = getTyperPos();
            while ((m_typerPos < end) && m_typerStr[m_typerPos]) {
                ++m_typerPos;
            }
            if (m_typerPos > start) {
                char old = m_typerStr[m_typerPos];
                m_typerStr[m_typerPos] = '\0';
                RF_AddText(m_ctx, &m_typerStr[start], m_typerCharset, m_typerType);
                m_typerStr[m_typerPos] = old;
                if (!old) { cancelTyper(); }  // EOS reached
            }
            requestFrames(1);
        }

        // process screen content update from RetroFont library
        if (m_ctx) {
            if (RF_Render(m_ctx, uint32_t(glfwGetTime() * 1000.0))) {
                glBindTexture(GL_TEXTURE_2D, m_tex);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                             m_ctx->bitmap_size.x, m_ctx->bitmap_size.y,
                             0, GL_RGB, GL_UNSIGNED_BYTE,
                             (const void*) m_ctx->bitmap);
                glBindTexture(GL_TEXTURE_2D, 0);
                GLutil::checkError("texture update");
            }
        }

        // determine tint and border color
        const GLfloat *tint = MonitorTints[
            (m_monitorType != mtAuto) ? (m_monitorType - mtOffset) :
            (m_ctx && m_ctx->system)  ? m_ctx->system->monitor :
                                        0  // fall back to white if uncertain
        ];
        GLfloat br = 0.0f, bg = 0.0f, bb = 0.0f;
        if (m_ctx) {
            br = RF_COLOR_R(m_ctx->border_rgb) / 255.0f;
            bg = RF_COLOR_G(m_ctx->border_rgb) / 255.0f;
            bb = RF_COLOR_B(m_ctx->border_rgb) / 255.0f;
        }
        if (tint[3] > 0.5f) {
            // convert to monochrome if necessary
            br = bg = bb = 0.25f * (br + bb) + 0.5f * bg;
        }

        // start display rendering
        GLutil::clearError();
        glClearColor(br * tint[0], bg * tint[1], bb * tint[2], 1.0f);
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
            GLenum filter = (m_screenMode == smDynamic) || (m_renderMode <= rmUnfiltered) ? GL_NEAREST : GL_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            glUniform1i(m_locMode, ((m_screenMode == smFixed) && (m_renderMode == rmBlocky)) ? 1 : 0);
            glUniform2f(m_locSize, float(m_ctx->bitmap_size.x), float(m_ctx->bitmap_size.y));
            glUniform4fv(m_locArea, 1, m_area);
            glUniform4fv(m_locTint, 1, tint);
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
    cancelTyper();
    ::free(m_docData);
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
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_ENTER);     screenChanged(); }
            break;
        case GLFW_KEY_TAB:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_TAB);       screenChanged(); }
            break;
        case GLFW_KEY_BACKSPACE:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_BACKSPACE); screenChanged(); }
            break;
        case GLFW_KEY_DELETE:
            if (m_ctx) { RF_AddChar(m_ctx, RF_CP_DELETE);    screenChanged(); }
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
        case GLFW_KEY_F1:
            m_showUI = !m_showUI;
            requestFrames(2);
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
    if (m_io->WantCaptureKeyboard) { return; }
    cancelTyper();
    RF_AddChar(m_ctx, codepoint);
    screenChanged();
}

void RFTestApp::handleMouseButtonEvent(int button, int action, int mods) {
    (void)mods;
    if (m_io->WantCaptureMouse) { return; }
    if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_1) && m_ctx) {
        cancelTyper();
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

void RFTestApp::loadDefaultScreen(int type) {
    cancelTyper();
    m_screenContentsChanged = false;
    m_justLoadedDocument = false;
    if (type == dsAsConfigured) {
        type = m_defaultScreen;
    }
    if (type == dsKeep) {
        return;  // do nothing; don't even touch m_screenContentsChanged!
    }
    if ((type == dsEmpty) || (type == dsDefault) || (type == dsPrevious)) {
        RF_ClearAll(m_ctx);
    }
    RF_MoveCursor(m_ctx, 0, 0);
    if (type == dsDefault) {
        RF_AddText(m_ctx,
            (m_ctx && m_ctx->system && m_ctx->system->default_screen)
                                     ? m_ctx->system->default_screen
                                     : DefaultDefaultScreen, 0, RF_MT_INTERNAL);
    } else if ((type == dsPrevious) && m_docData) {
        RF_AddText(m_ctx, m_docData, m_docCharset ? m_docCharset : m_docAutoCharset, m_docType);
        m_justLoadedDocument = true;
    } else if (type == dsDemo) {
        srand(0x13375EED);
        RF_DemoScreen(m_ctx);
    }
}

void RFTestApp::loadScreen(const char* text, const RF_Charset* charset, RF_MarkupType markup) {
    cancelTyper();
    RF_ClearScreen(m_ctx, nullptr);
    RF_MoveCursor(m_ctx, 0, 0);
    if (m_baud) {
        m_typerStr = StringUtil::copy(text);
        m_typerCharset = charset;
        m_typerType = markup;
        m_typerStartPos = m_typerPos = 0;
        m_typerStartTime = glfwGetTime();
        requestFrames(1);
    } else {
        RF_AddText(m_ctx, text, charset, markup);
    }
    m_screenContentsChanged = true;
}

void RFTestApp::handleDropEvent(int path_count, const char* paths[]) {
    if ((path_count < 1) || !paths || !paths[0] || !paths[0][0]) { return; }
    ::free(m_docData);
    m_docData = StringUtil::loadTextFile(paths[0]);
    m_docAutoCharset = RF_DetectCharset(m_docData);
    m_docType = RF_DetectMarkupType(m_docData);
    loadScreen(m_docData, m_docCharset ? m_docCharset : m_docAutoCharset, m_docType);
    m_justLoadedDocument = true;
}

void RFTestApp::cancelTyper() {
    ::free((void*)m_typerStr);
    m_typerStr = nullptr;
    RF_ResetParser(m_ctx);
}

int RFTestApp::getTyperPos() {
    double now = glfwGetTime();
    return m_typerStartPos + int((now - m_typerStartTime) * 0.1 * m_baud);
}

////////////////////////////////////////////////////////////////////////////////

void RFTestApp::drawUI() {
    static constexpr size_t tempStrSize = 80;
    static char tempStr[tempStrSize] = "";

    // main window begin
    ImGui::SetNextWindowPos(ImVec2(
        ImGui::GetMainViewport()->WorkPos.x,
        ImGui::GetMainViewport()->WorkPos.y +
        ImGui::GetMainViewport()->WorkSize.y),
        ImGuiCond_FirstUseEver, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(470.0f, 262.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, 0)) {

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("on system switch:");
        ImGui::SameLine();
        ImGui::PushItemWidth(234.0f);
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

        // set a sane width for the main controls
        ImGui::PushItemWidth(360.0f);

        if (ImGui::BeginCombo("system", (m_ctx && m_ctx->system) ? m_ctx->system->name : "???", ImGuiComboFlags_HeightLarge)) {
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

        if (ImGui::BeginCombo("font", (m_ctx && m_ctx->font) ? m_ctx->font->name : "???", ImGuiComboFlags_HeightLarge)) {
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

        if ((m_monitorType == mtAuto) && m_ctx && m_ctx->system) {
            snprintf(tempStr, tempStrSize, "automatic (%s)", MonitorTypeStrings[int(m_ctx->system->monitor) + mtOffset]);
        }
        if (ImGui::SliderInt("monitor type", &m_monitorType, mtAuto, mtMax-1, (m_monitorType == mtAuto) ? tempStr : MonitorTypeStrings[m_monitorType])) {
            requestFrames(1);
        }

        int fbmode = 2;
        if (m_ctx) {
            switch (m_ctx->fallback) {
                case RF_FB_FONT_CHAR: fbmode = 0; break;
                case RF_FB_FONT:      fbmode = 1; break;
                case RF_FB_CHAR:      fbmode = 3; break;
                case RF_FB_CHAR_FONT: fbmode = 4; break;
                default:      /* keep fbmode = 2*/break;
            }
        }
        static const char* fmodeStrings[] = { "both (font first)", "other font", "no fallback", "replacement characters", "both (characters first)" };
        if (ImGui::SliderInt("fallback mode", &fbmode, 0, 4, fmodeStrings[fbmode])) {
            switch (fbmode) {
                case 0:  RF_SetFallbackMode(m_ctx, RF_FB_FONT_CHAR); break;
                case 1:  RF_SetFallbackMode(m_ctx, RF_FB_FONT);      break;
                case 3:  RF_SetFallbackMode(m_ctx, RF_FB_CHAR);      break;
                case 4:  RF_SetFallbackMode(m_ctx, RF_FB_CHAR_FONT); break;
                default: RF_SetFallbackMode(m_ctx, RF_FB_NONE);      break;
            }
        }

        // end of main controls
        ImGui::PopItemWidth();

        ImGui::AlignTextToFramePadding();
        if (ImGui::Button("Load screen..."))
            { ImGui::OpenPopup("load_screen_popup"); }
        if (ImGui::BeginPopup("load_screen_popup")) {
            if (ImGui::Selectable("empty screen"))   { loadDefaultScreen(dsEmpty); }
            if (ImGui::Selectable("default screen")) { loadScreen(DefaultDefaultScreen, 0, RF_MT_INTERNAL); }
            if (ImGui::Selectable("attribute demo")) { loadDefaultScreen(dsDemo); }
            if (ImGui::BeginMenu("system default")) {
                const char* scr = nullptr;
                for (const RF_System* const* p_sys = RF_SystemList;  *p_sys;  ++p_sys) {
                    if ((*p_sys)->default_screen == scr) { continue; }
                    scr = (*p_sys)->default_screen;
                    if (scr && ImGui::Selectable((*p_sys)->name)) {
                        loadScreen(scr, 0, RF_MT_INTERNAL);
                    }
                }
                ImGui::EndMenu();
            }
            if (!m_docData) { ImGui::BeginDisabled(); }
            if (ImGui::Selectable("previous loaded document")) {
                loadScreen(m_docData, m_docCharset ? m_docCharset : m_docAutoCharset, m_docType);
                m_justLoadedDocument = true;
            }
            if (!m_docData) { ImGui::EndDisabled(); }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        snprintf(tempStr, tempStrSize, "with %d baud", m_baud);
        ImGui::PushItemWidth(160.0f);
        if (ImGui::BeginCombo("##baud", m_baud ? tempStr : "immediately")) {
            static const int BaudRates[] = { 75, 300, 1200, 9600, 38400, 115200, 0 };
            for (const int *p_rate = BaudRates;  *p_rate;  ++p_rate) {
                bool sel = (m_baud == *p_rate);
                snprintf(tempStr, tempStrSize, "with %d baud", *p_rate);
                if (ImGui::Selectable(tempStr, &sel)) {
                    if (m_typerStr) {
                        m_typerStartTime = glfwGetTime();
                        m_typerStartPos = m_typerPos;
                        m_typerPos = getTyperPos();
                    }
                    m_baud = *p_rate;
                }
            }
            bool sel = !m_baud;
            if (ImGui::Selectable("immediately", &sel)) {
                m_baud = 0;
                if (m_typerStr) {
                    RF_AddText(m_ctx, &m_typerStr[m_typerPos], m_typerCharset, m_typerType);
                }
                cancelTyper();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("loaded document character set:");
        ImGui::SameLine();
        ImGui::PushItemWidth(234.0f);
        auto charsetStr = [=] (const RF_Charset* charset) -> const char* {
            if (charset) {
                snprintf(tempStr, tempStrSize, "%s (%s)", charset->short_name, charset->long_name);
            } else if (m_docAutoCharset) {
                snprintf(tempStr, tempStrSize, "auto-detect (%s)", m_docAutoCharset->short_name);
            } else {
                strcpy(tempStr, "auto-detect");
            }
            return tempStr;
        };
        auto charsetItem = [=] (const RF_Charset* charset) {
            bool sel = (m_docCharset == charset);
            if (ImGui::Selectable(charsetStr(charset), &sel)) {
                m_docCharset = charset;
                if (m_justLoadedDocument) {
                    loadScreen(m_docData, m_docCharset ? m_docCharset : m_docAutoCharset, m_docType);
                }
            }
        };
        if (ImGui::BeginCombo("##charset", charsetStr(m_docCharset))) {
            charsetItem(nullptr);
            for (const RF_Charset* cs = RF_Charsets;  cs->charset_id;  ++cs) {
                charsetItem(cs);
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////

    // attribute window begin
    ImGui::SetNextWindowPos(ImVec2(
        ImGui::GetMainViewport()->WorkPos.x +
        ImGui::GetMainViewport()->WorkSize.x,
        ImGui::GetMainViewport()->WorkPos.y +
        ImGui::GetMainViewport()->WorkSize.y),
        ImGuiCond_FirstUseEver, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(390.0f, 272.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Attributes", nullptr, 0)) {
        colorUI("border color",
                m_ctx ? m_ctx->border_color : RF_COLOR_DEFAULT,
                [=] (uint32_t color) { RF_SetBorderColor(m_ctx, color); });
        colorUI("default background color",
                m_ctx ? m_ctx->default_bg : RF_COLOR_DEFAULT,
                [=] (uint32_t color) { RF_SetBackgroundColor(m_ctx, color); });
        colorUI("default foreground color",
                m_ctx ? m_ctx->default_fg : RF_COLOR_DEFAULT,
                [=] (uint32_t color) { RF_SetForegroundColor(m_ctx, color); });
        colorUI("typed character background color",
                m_ctx ? m_ctx->attrib.bg : RF_COLOR_DEFAULT,
                [=] (uint32_t color) { if (m_ctx) { m_ctx->attrib.bg = color; } });
        colorUI("typed character foreground color",
                m_ctx ? m_ctx->attrib.fg : RF_COLOR_DEFAULT,
                [=] (uint32_t color) { if (m_ctx) { m_ctx->attrib.fg = color; } });

        if (ImGui::TreeNodeEx("typed character attributes", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {
            #define MAKE_TOGGLE(attr,label) do { \
                bool b = m_ctx && !!m_ctx->attrib.attr; \
                if (ImGui::Checkbox(label, &b) && m_ctx) { \
                    m_ctx->attrib.attr = b ? 1 : 0; \
                } \
            } while (0)
            MAKE_TOGGLE(bold,      "bold");   ImGui::SameLine();
            MAKE_TOGGLE(dim,       "dim");    ImGui::SameLine();
            MAKE_TOGGLE(underline, "line");   ImGui::SameLine();
            MAKE_TOGGLE(blink,     "blink");  ImGui::SameLine();
            MAKE_TOGGLE(reverse,   "rev");    ImGui::SameLine();
            MAKE_TOGGLE(invisible, "invis");
            #undef MAKE_TOGGLE
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////

void RFTestApp::colorUI(const char* title, uint32_t color, std::function<void(uint32_t color)> setter) {
    if (!ImGui::TreeNodeEx(title, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
        { return; }

    // Default
    bool active = (color == RF_COLOR_DEFAULT);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    }
    if (ImGui::Button("Default")) {
        setter(RF_COLOR_DEFAULT);
    }
    if (active) {
        ImGui::PopStyleColor(1);
    }

    // Std16
    ImGui::PushItemWidth(16.0f);
    for (int i = 0;  i < 16;  ++i) {
        static const ImU32 colors[16] = {
            0xFF000000, 0xFF800000, 0xFF008000, 0xFF808000, 0xFF000080, 0xFF800080, 0xFF008080, 0xFFC0C0C0,
            0xFF404040, 0xFFFF0000, 0xFF00FF00, 0xFFFFFF00, 0xFF0000FF, 0xFFFF00FF, 0xFF00FFFF, 0xFFFFFFFF,
        };
        ImGui::SameLine(0.0f, i ? 0.0f : -1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        colors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[i]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  colors[i]);
        ImGui::PushStyleColor(ImGuiCol_Text, ((0xFC80 >> i) & 1) ? 0xFF000000 : 0xFFFFFFFF);
        ImGui::PushID(i);
        if (ImGui::Button((color == (RF_COLOR_BLACK + i)) ? "X" : " ")) {
            setter(RF_COLOR_BLACK + i);
        }
        ImGui::PopID();
        ImGui::PopStyleColor(4);
    }
    ImGui::PopItemWidth();

    // RGB
    ImGui::SameLine();
    float rgb[3] = { .0f, .0f, .0f };
    active = RF_IS_RGB_COLOR(color);
    if (active) {
        rgb[0] = RF_COLOR_R(color) / 255.f;
        rgb[1] = RF_COLOR_G(color) / 255.f;
        rgb[2] = RF_COLOR_B(color) / 255.f;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    }
    if (ImGui::ColorEdit3("RGB", rgb, ImGuiColorEditFlags_NoInputs)) {
        setter(RF_COLOR_RGB(
            uint8_t(rgb[0] * 255.f + .5f),
            uint8_t(rgb[1] * 255.f + .5f),
            uint8_t(rgb[2] * 255.f + .5f)
        ));
    }
    if (!active) {
        ImGui::PopStyleColor(1);
    }

    ImGui::TreePop();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    static RFTestApp app;
    return app.run(argc, argv);
}
