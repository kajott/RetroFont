#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

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
    static const uint32_t cp_offsets[] = {
        0x0020, 0x0040, 0x0060,          // Basic Latin (a.k.a. standard ASCII)
        0x00A0, 0x00C0, 0x00E0,          // Latin-1 Supplement
        0x2580,                          // Block Elements
        0x1FB70, 0x1FB90,                // (some) Symbols for Legacy Computing
        0x2500, 0x2520, 0x2540, 0x2560,  // Box Drawing
        0x25A0, 0x25C0, 0x25E0,          // Geometric Shapes
        0x2190,                          // Arrows (most basic ones only)
    };
    uint16_t demo_row_count = (uint16_t)(sizeof(cp_offsets) / sizeof(*cp_offsets));
    uint16_t attribute_start_row = (m_ctx->screen_size.y > demo_row_count) || (m_ctx->screen_size.x > 32) ? demo_row_count : 9;
    RF_Cell *c = m_ctx->screen;
    for (uint16_t y = 0;  y < m_ctx->screen_size.y;  ++y) {
        for (uint16_t x = 0;  x < m_ctx->screen_size.x;  ++x) {
            uint32_t row_mod = 3;
            if (y >= attribute_start_row) {
                uint16_t r = (uint16_t) rand();
                c->fg = RF_COLOR_BLACK | (rand() & 15);
                do { c->bg = RF_COLOR_BLACK | (rand() & 15); } while (c->bg == c->fg);
                c->bold      = r >> 0;
                c->dim       = r >> 1;
                c->underline = r >> 2;
                c->blink     = r >> 3;
                c->reverse   = r >> 4;
                c->invisible = (r & 0x3F00) ? 0 : 1;  // make this very rare
            } else if (x >= 64) {
                c->bold      = y >> 0;
                c->dim       = x >> 0;
                c->underline = y >> 1;
                c->blink     = x >> 1;
                c->reverse   = y >> 2;
                c->invisible = x >> 2;
            } else if (x >= 32) {
                c->fg = RF_COLOR_BLACK | (x & 15);
                c->bg = RF_COLOR_BLACK | (y & 15);
                c->reverse = x >> 4;
            } else {
                row_mod = demo_row_count;
            }
            c->codepoint = (x & 31) + cp_offsets[y % row_mod];
            ++c;
        }
    }
    RF_Invalidate(m_ctx, true);

    // main loop
    while (m_active && !glfwWindowShouldClose(m_window)) {
        bool frameRequested = (m_renderFrames > 0);
        if (frameRequested) {
            glfwPollEvents();
            --m_renderFrames;
        } else {
            uint32_t blink = m_ctx ? m_ctx->system->blink_interval_msec : 0;
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
        ImGui::Render();

        // start display rendering
        GLutil::clearError();
        if (m_ctx) {
            glClearColor(RF_COLOR_R(m_ctx->border_color) / 255.0f,
                         RF_COLOR_G(m_ctx->border_color) / 255.0f,
                         RF_COLOR_B(m_ctx->border_color) / 255.0f,
                         1.0f);
        }
        glViewport(0, 0, int(m_io->DisplaySize.x), int(m_io->DisplaySize.y));
        glClear(GL_COLOR_BUFFER_BIT);

        // draw the screen
        if (m_ctx) {
            glBindTexture(GL_TEXTURE_2D, m_tex);
            if (RF_Render(m_ctx, uint32_t(glfwGetTime() * 1000.0))) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                             m_ctx->bitmap_size.x, m_ctx->bitmap_size.y,
                             0, GL_RGB, GL_UNSIGNED_BYTE,
                             (const void*) m_ctx->bitmap);
            }
            glUseProgram(m_prog);
            glUniform4f(m_locArea, 2.0f, -2.0f, -1.0f, 1.0f);
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
    (void)scancode;
    if ((action != GLFW_PRESS) || m_io->WantCaptureKeyboard) { return; }
    (void)key, (void)mods;
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
    ImGui::SetNextWindowSize(ImVec2(320.0f, 480.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, 0)) {

    }
    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    static RFTestApp app;
    return app.run(argc, argv);
}
