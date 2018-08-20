#include "rendererSDL.h"

#include <functional>
#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>
#include <map>

#include <CGAL/Aff_transformation_2.h>
#include <CGAL/aff_transformation_tags.h>

#include "utility/utility.h"

namespace {

static constexpr size_t MAX_INSTANCES = 4096;

static const char *glErrorReasons[] = {
    "GL_INVALID_ENUM: Used invalid enum parameter",
    "GL_INVALID_VALUE: Used invalid parameter",
    "GL_INVALID_OPERATION: Invalid state for command or invalid parameter combination",
    "GL_STACK_OVERFLOW: <-",
    "GL_STACK_UNDERFLOW: ... How?",
    "GL_OUT_OF_MEMORY: I can't remember this",
    "GL_INVALID_FRAMEBUFFER_OPERATION: read/write/render with incomplete framebuffer",
    "GL_CONTEXT_LIST: Where did the graphics card go?"
};

static constexpr const char *getGLerrorReason(const size_t e) {
    return (0x500 <= e && e <= 0x507) ? glErrorReasons[e - 0x500] : "Unknown OpenGL error";
}

#define GL_ERROR {\
    GLenum err = glGetError();\
    rassert(GL_NO_ERROR == err, err, std::hex, "0x", err, getGLerrorReason(err));\
}

#define GLEW_ERROR(x) {\
    GLenum err = x;\
    rassert(GLEW_OK == err, "GLEW Error:", glewGetErrorString(err));\
}

static void checkShaderCompile(GLuint id) {
    GLint res;
    glGetShaderiv(id, GL_COMPILE_STATUS, &res);
    if (GL_FALSE == res) {
        int infoLen = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLen);
        GLchar buff[infoLen];
        GLsizei wrote;
        glGetShaderInfoLog(id, infoLen, &wrote, buff);
        rassert(false, "Failed to compile shader:", static_cast< const char * >(buff));
    }
}

static void checkShaderLink(GLuint id) {
    GLint res;
    glGetProgramiv(id, GL_LINK_STATUS, &res);
    if (GL_FALSE == res) {
        int infoLen = 0;
        glGetProgramiv(id, GL_LINK_STATUS, &infoLen);
        GLchar buff[infoLen];
        GLsizei wrote;
        glGetShaderInfoLog(id, infoLen, &wrote, buff);
        rassert(false, "Failed to link OpenGL program:", static_cast< const char * >(buff));
    }
}

static inline void setWhite(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

static GLuint addGLProgram(const GLchar *vertSrc[], const GLchar *fragSrc[]) {
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER); GL_ERROR
    glShaderSource(vertShader, 1, vertSrc, nullptr); GL_ERROR
    glCompileShader(vertShader); GL_ERROR
    checkShaderCompile(vertShader);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER); GL_ERROR
    glShaderSource(fragShader, 1, fragSrc, nullptr); GL_ERROR
    glCompileShader(fragShader); GL_ERROR
    checkShaderCompile(fragShader);

    GLuint prog = glCreateProgram(); GL_ERROR
    glAttachShader(prog, vertShader); GL_ERROR
    glAttachShader(prog, fragShader); GL_ERROR
    glLinkProgram(prog); GL_ERROR
    checkShaderLink(prog);

    glDeleteShader(vertShader); GL_ERROR
    glDeleteShader(fragShader); GL_ERROR

    return prog;
}

static GLuint addPointProgram() {
    static const GLchar *vertShaderSrc[] = {
        "#version 450 core\n"
        "layout (location = 0) in vec3 pos;"
        "layout (location = 1) in vec4 col;"
        "out vec4 fcol;"
        "void main() {"
        "   fcol = col;"
        "   gl_Position = vec4(pos, 1);"
        "}"
    };
    static const GLchar *fragShaderSrc[] = {
        "#version 450 core\n"
        "in vec4 fcol;"
        "out vec4 color;"
        "void main() {"
        "   color = fcol;"
        "}"
    };
    return addGLProgram(vertShaderSrc, fragShaderSrc);
}

static GLuint addCircProgram() {
    static const GLchar *vertShaderSrc[] = {
        "#version 450 core\n"
        "layout (location = 0) in vec3 off;"
        "layout (location = 1) in vec4 col;"
        "layout (location = 2) in vec2 pos;"
        "layout (location = 3) in vec2 rad;"
        "out vec2 centre;"
        "out float radius;"
        "out vec4 fcol;"
        "void main() {"
        "   centre = vec2(off);"
        "   radius = rad[0];"
        "   fcol = col;"
        "   gl_Position = vec4("
        "       off[0] + pos[0] * rad[0],"
        "       off[1] + pos[1] * rad[1],"
        "       off[2], 1);"
        "}"
    };
    static const GLchar *fragShaderSrc[] = {
        "#version 450 core\n"
        "uniform vec2 halfScreenDim;"
        "in vec2 centre;"
        "in float radius;"
        "in vec4 fcol;"
        "out vec4 color;"
        "void main() {"
        "   vec2 at = (gl_FragCoord.xy - halfScreenDim) / halfScreenDim;"
        "   vec2 diff = centre - at;"
        "   if (dot(diff, diff) > radius * radius) { discard; }"
        "   color = fcol;"
        "}"
    };
    return addGLProgram(vertShaderSrc, fragShaderSrc);
}

static GLuint addRectProgram() {
    static const GLchar *vertShaderSrc[] = {
        "#version 450 core\n"
        "layout (location = 0) in vec3 off;"
        "layout (location = 1) in vec4 col;"
        "layout (location = 2) in vec2 pos;"
        "layout (location = 3) in vec2 rad;"
        "out vec4 fcol;"
        "void main() {"
        "   fcol = col;"
        "   gl_Position = vec4("
        "       off[0] + pos[0] * rad[0],"
        "       off[1] + pos[1] * rad[1],"
        "       off[2], 1);"
        "}"
    };
    static const GLchar *fragShaderSrc[] = {
        "#version 450 core\n"
        "in vec4 fcol;"
        "out vec4 color;"
        "void main() {"
        "   color = fcol;"
        "}"
    };
    return addGLProgram(vertShaderSrc, fragShaderSrc);
}

};

void RendererSDL::drawPoint(Point pos, Point3 col, double alpha, double depth) {
    commands[0].push_back({ col, pos, { 0, 0 }, alpha, depth });
}

void RendererSDL::drawBox(Point pos, Vec rad, Point3 col, double alpha, double depth) {
    commands[2].push_back({ col, pos, rad, alpha, depth });
}

void RendererSDL::drawCircle(Point pos, Vec rad, Point3 col, double alpha, double depth) {
    commands[1].push_back({ col, pos, rad, alpha, depth });
}

RendererSDL::RendererSDL(size_t width, size_t height)
    : Renderer(width, height)
    , width(width)
    , height(height)
    , window(nullptr) {
    rassert(SDL_Init(SDL_INIT_VIDEO) >= 0, "Failed to initialize SDL", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); GL_ERROR
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5); GL_ERROR
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); GL_ERROR
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); GL_ERROR

    window = SDL_CreateWindow("Hallway",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height, SDL_WINDOW_OPENGL);
    rassert(window, "Failed to create window", SDL_GetError());

    context = SDL_GL_CreateContext(window);
    rassert(context, "Failed to create OpenGL context", SDL_GetError());

    GLEW_ERROR(glewInit());

    GL_ERROR
    SDL_GL_SetSwapInterval(0); GL_ERROR
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); GL_ERROR
    glClearDepth(0.0); GL_ERROR;
    glEnable(GL_BLEND); GL_ERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_ERROR
    glEnable(GL_DEPTH_TEST); GL_ERROR
    glDepthFunc(GL_GEQUAL); GL_ERROR

    programs[0] = addPointProgram();
    programs[1] = addCircProgram();
    programs[2] = addRectProgram();
    hsdLoc = glGetUniformLocation(programs[1], "halfScreenDim"); GL_ERROR

    GLfloat verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    GLuint indices[] = { 0, 1, 2, 3 };

    glGenBuffers(1, &vbo); GL_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, vbo); GL_ERROR
    glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), verts, GL_STATIC_DRAW); GL_ERROR

    glGenBuffers(1, &ibo); GL_ERROR
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); GL_ERROR
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indices, GL_STATIC_DRAW); GL_ERROR

    glGenBuffers(1, &posBuffer); GL_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, posBuffer); GL_ERROR
    // x, y, d
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 3 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &radBuffer); GL_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, radBuffer); GL_ERROR
    // w, h
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 2 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &colBuffer); GL_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, colBuffer); GL_ERROR
    // r, g, b, a
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 4 * sizeof(GLubyte), nullptr, GL_STREAM_DRAW);

    glGenVertexArrays(1, &vao); GL_ERROR
    glBindVertexArray(vao); GL_ERROR

    clear();
    update();
}

RendererSDL::~RendererSDL() {
    glDeleteVertexArrays(1, &vao);
    for (const auto &p : programs) {
        glDeleteProgram(p);
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void RendererSDL::clear() {
    for (auto &v : commands) { v.clear(); }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_ERROR
}

void RendererSDL::update() {
    static GLfloat posData[MAX_INSTANCES * 3];
    static GLfloat radData[MAX_INSTANCES * 2];
    static GLubyte colData[MAX_INSTANCES * 4];

    const auto scaler = Kernel::Aff_transformation_2(
            2.0 / width, 0.0, 0.0, 0.0, 2.0 / height, 0.0);
    const auto transl = Kernel::Aff_transformation_2(CGAL::TRANSLATION,
            Kernel::Vector_2(-1.0, -1.0));
    const auto toScreen = transl * scaler;

    GL_ERROR

    for (size_t i = 0; i < 3; ++i) {
        glUseProgram(programs[i]);
        if (1 == i) { glUniform2f(hsdLoc, width / 2.0, height / 2.0); }

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colBuffer);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, nullptr);

        if (i > 0) {
            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

            glEnableVertexAttribArray(3);
            glBindBuffer(GL_ARRAY_BUFFER, radBuffer);
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        }

        glVertexAttribDivisor(0, 1);
        glVertexAttribDivisor(1, 1);
        if (i > 0) {
            glVertexAttribDivisor(2, 0);
            glVertexAttribDivisor(3, 1);
        }

        size_t start = 0;
        while (start < commands[i].size()) {
            size_t count = 0;
            const size_t until = std::min(commands[i].size(), start + MAX_INSTANCES);
            for (size_t j = start; j < until; ++j) {
                const auto &dc = commands[i][j];
                const Point p = toScreen.transform(dc.pos);
                const Vec r = toScreen.transform(dc.rad);

                posData[3 * count + 0] = p[0];
                posData[3 * count + 1] = p[1];
                posData[3 * count + 2] = dc.depth / 2.0 + 0.5;

                radData[2 * count + 0] = r[0];
                radData[2 * count + 1] = r[1];

                colData[4 * count + 0] = dc.col[0];
                colData[4 * count + 1] = dc.col[1];
                colData[4 * count + 2] = dc.col[2];
                colData[4 * count + 3] = dc.alpha * 255.0;

                ++count;
            }
            start = until;

            glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
            glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 3 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(GLfloat), posData);

            glBindBuffer(GL_ARRAY_BUFFER, colBuffer);
            glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 4 * sizeof(GLubyte), nullptr, GL_STREAM_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, count * 4 * sizeof(GLubyte), colData);

            if (i > 0) {
                glBindBuffer(GL_ARRAY_BUFFER, radBuffer);
                glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 2 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, count * 2 * sizeof(GLfloat), radData);
            }

            if (0 == i) {
                glDrawArraysInstanced(GL_POINTS, 0, 1, count);
            } else {
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, count);
            }
        }

        if (i > 0) {
            glDisableVertexAttribArray(3);
            glDisableVertexAttribArray(2);
        }
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
    }

    glUseProgram(0);
    SDL_GL_SwapWindow(window);
    
    GL_ERROR
}

size_t RendererSDL::getWidth() const {
    return width;
}

size_t RendererSDL::getHeight() const {
    return height;
}
