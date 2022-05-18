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
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleMouseButtonEvent(button, action, mods); });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleCursorPosEvent(xpos, ypos); });
    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset)
        { static_cast<RFTestApp*>(glfwGetWindowUserPointer(window))->handleScrollEvent(xoffset, yoffset); });

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

    ImGui::CreateContext();
    m_io = &ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(nullptr);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
        "\n" "uniform sampler2D uTex;"
        "\n" "in vec2 vPos;"
        "\n" "out vec4 oColor;"
        "\n" "void main() {"
        "\n" "  vec2 pos = vPos;"
        "\n" "  if (uMode != 0) {"
        "\n" "    // TODO"
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
    srand(0x13375EED);
    RF_DemoScreen(m_ctx);
    RF_Invalidate(m_ctx, true);

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
                int border = ((m_borderMode == bmNone) || !m_ctx->font) ? 0
                           : std::min(m_ctx->font->font_size.x, m_ctx->font->font_size.y);
                x0 = std::max(m_ctx->main_ul.x - border, 0);
                y0 = std::max(m_ctx->main_ul.y - border, 0);
                x1 = std::min(m_ctx->main_lr.x + border, int(m_ctx->bitmap_size.x));
                y1 = std::min(m_ctx->main_lr.y + border, int(m_ctx->bitmap_size.y));
            }
            double pixelAspect = m_ctx->pixel_aspect
                               * ((x1 - x0) * m_io->DisplaySize.y)
                               / ((y1 - y0) * m_io->DisplaySize.x);
            double sx, sy, ox, oy;
            if (pixelAspect < 1.0) {
                // Pillarbox
                sy = 2.0 / (y1 - y0) * m_ctx->bitmap_size.y;
                sx = sy * pixelAspect;
                oy = -1.0 - sy * y0 / m_ctx->bitmap_size.y;
                ox = -0.5 * (x0 + x1) * sx / m_ctx->bitmap_size.x;
            } else {
                // Letterbox
                sx = 2.0 / (x1 - x0) * m_ctx->bitmap_size.x;
                sy = sx / pixelAspect;
                ox = -1.0 - sx * x0 / m_ctx->bitmap_size.x;
                oy = -0.5 * (y0 + y1) * sy / m_ctx->bitmap_size.y;
            }

            // actual drawing
            glBindTexture(GL_TEXTURE_2D, m_tex);
            if (RF_Render(m_ctx, uint32_t(glfwGetTime() * 1000.0))) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                             m_ctx->bitmap_size.x, m_ctx->bitmap_size.y,
                             0, GL_RGB, GL_UNSIGNED_BYTE,
                             (const void*) m_ctx->bitmap);
            }
            glUseProgram(m_prog);
            glUniform4f(m_locArea, float(sx), -float(sy), float(ox), -float(oy));
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
    if ((action != GLFW_PRESS) || m_io->WantCaptureKeyboard) { return; }
    switch (key) {
        case GLFW_KEY_F9:
            m_showDemo = !m_showDemo;
            requestFrames(2);
            break;
        default:
            break;
    }
}

void RFTestApp::handleMouseButtonEvent(int button, int action, int mods) {
    (void)mods, (void)action, (void)button;
    if (m_io->WantCaptureMouse) { return; }
}

void RFTestApp::handleCursorPosEvent(double xpos, double ypos) {
    (void)xpos, (void)ypos;
}

void RFTestApp::handleScrollEvent(double xoffset, double yoffset) {
    (void)xoffset, (void)yoffset;
    if (m_io->WantCaptureMouse) { return; }
}

////////////////////////////////////////////////////////////////////////////////

void RFTestApp::drawUI() {
    // main window begin
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(344.0f, 128.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, 0)) {

        if (ImGui::BeginCombo("system", (m_ctx && m_ctx->system) ? m_ctx->system->name : "???", 0)) {
            for (const RF_System* const* p_sys = RF_SystemList;  *p_sys;  ++p_sys) {
                if (ImGui::Selectable((*p_sys)->name, m_ctx && (m_ctx->system == *p_sys))) {
                    if (RF_SetSystem(m_ctx, (*p_sys)->sys_id)) {
                        RF_ResizeScreen(m_ctx, RF_SIZE_DEFAULT, RF_SIZE_DEFAULT, true);
                    }
                }
            }
            ImGui::EndCombo();
        }

        RF_Coord matchSize = {0, 0};
        if (m_ctx && m_ctx->system) { matchSize = m_ctx->system->font_size; }
        if (ImGui::BeginCombo("font", (m_ctx && m_ctx->font) ? m_ctx->font->name : "???", 0)) {
            for (const RF_Font* font = RF_FontList;  font->font_id;  ++font) {
                if ((matchSize.x && (matchSize.x != font->font_size.x))
                ||  (matchSize.y && (matchSize.y != font->font_size.y))) {
                    continue;
                }
                if (ImGui::Selectable(font->name, m_ctx && (m_ctx->font == font))) {
                    if (RF_SetFont(m_ctx, font->font_id)) {
                        RF_ResizeScreen(m_ctx, 0,0, true);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("borders:"); ImGui::SameLine();
        ImGui::RadioButton("full",    &m_borderMode, bmFull);    ImGui::SameLine();
        ImGui::RadioButton("reduced", &m_borderMode, bmReduced); ImGui::SameLine();
        ImGui::RadioButton("none",    &m_borderMode, bmNone);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("size:"); ImGui::SameLine();
        int dynaSize = 0;
        ImGui::RadioButton("fixed",   &dynaSize, 0); ImGui::SameLine();
        ImGui::RadioButton("dynamic", &dynaSize, 1);
    }
    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    static RFTestApp app;
    return app.run(argc, argv);
}
