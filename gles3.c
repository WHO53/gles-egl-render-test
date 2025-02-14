#include "render.h"
#include <stdio.h>
#include <GLES3/gl3.h>

#define TEXTURE_SIZE 512

GLuint texture;
GLuint shader_program;
GLint tex_uniform;
GLuint vao, vbo;
GLsync syncObj;  

const char* vertex_shader_source =
    "#version 300 es\n"
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "out vec2 v_texCoord;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    v_texCoord = texCoord;\n"
    "}\n";

const char* fragment_shader_source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 v_texCoord;\n"
    "uniform sampler2D texSampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(texSampler, v_texCoord);\n"
    "}\n";

GLuint create_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, buffer);
        fprintf(stderr, "Shader compile error:\n%s\n", buffer);
    }

    return shader;
}

void init_gl() {
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint link_status;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        char buffer[512];
        glGetProgramInfoLog(shader_program, 512, NULL, buffer);
        fprintf(stderr, "Program link error:\n%s\n", buffer);
    }

    glUseProgram(shader_program);
    tex_uniform = glGetUniformLocation(shader_program, "texSampler");

    const GLfloat vertices[] = {
        -1.0f, -1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 0.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void create_color_block_texture() {
    unsigned char color_block[TEXTURE_SIZE * TEXTURE_SIZE * 4];
    
    for (int i = 0; i < TEXTURE_SIZE * TEXTURE_SIZE * 4; i += 4) {
        color_block[i] = 255;     
        color_block[i + 1] = 0;   
        color_block[i + 2] = 0;   
        color_block[i + 3] = 255; 
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, color_block);
}

void draw_frame(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(tex_uniform, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    
    if (syncObj) {
        glDeleteSync(syncObj);
    }
    syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    
    GLenum waitReturn = GL_TIMEOUT_EXPIRED;
    while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED) {
        waitReturn = glClientWaitSync(syncObj, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); 
    }

    if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED) {
        printf("GPU Sync: Frame completed!\n");
    } else {
        printf("GPU Sync: Timeout waiting for frame to complete.\n");
    }
}

int main() {
    RenderContext *ctx = render_init(640, 480);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize render context\n");
        return 1;
    }

    init_gl();
    create_color_block_texture();  

    render_loop(ctx, draw_frame);

    glDeleteTextures(1, &texture);
    glDeleteProgram(shader_program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    if (syncObj) {
        glDeleteSync(syncObj);
    }

    render_cleanup(ctx);
    return 0;
}
