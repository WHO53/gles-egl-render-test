#include "render.h"
#include <stdio.h>
#include <GLES2/gl2.h>

GLuint shader_program;
GLint position_attr;

const char* vertex_shader_source =
    "attribute vec4 position;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "}\n";

const char* fragment_shader_source =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n" 
    "}\n";

GLuint create_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

void init_gl() {
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glUseProgram(shader_program);
    position_attr = glGetAttribLocation(shader_program, "position");
}

void draw_frame(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    static const GLfloat vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    glUseProgram(shader_program);

    glVertexAttribPointer(position_attr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(position_attr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int main() {
    RenderContext *ctx = render_init(640, 480);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize render context\n");
        return 1;
    }

    init_gl();
    render_loop(ctx, draw_frame);

    glDeleteProgram(shader_program);
    render_cleanup(ctx);
    return 0;
}
