#include "render.h"
#include <stdio.h>
#include <GLES2/gl2.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#define TEXTURE_SIZE 512

GLuint texture;
GLuint shader_program;
GLint position_attr, tex_coord_attr;
GLint tex_uniform;

const char* vertex_shader_source =
    "attribute vec4 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 v_texCoord;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    v_texCoord = texCoord;\n"
    "}\n";

const char* fragment_shader_source =
    "precision mediump float;\n"
    "varying vec2 v_texCoord;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(texture, v_texCoord);\n"
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
    tex_coord_attr = glGetAttribLocation(shader_program, "texCoord");
    tex_uniform = glGetUniformLocation(shader_program, "texture");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void render_text_to_texture() {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, TEXTURE_SIZE, TEXTURE_SIZE);
    cairo_t* cr = cairo_create(surface);

    // Set background to black
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // Create Pango layout
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* desc = pango_font_description_from_string("Sans 40");
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    // Set text and color
    pango_layout_set_text(layout, "Hello", -1);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  // White text

    // Center the text
    int width, height;
    pango_layout_get_size(layout, &width, &height);
    cairo_move_to(cr, (TEXTURE_SIZE - width / PANGO_SCALE) / 2, (TEXTURE_SIZE - height / PANGO_SCALE) / 2);

    // Render text
    pango_cairo_show_layout(cr, layout);

    // Clean up
    g_object_unref(layout);
    cairo_destroy(cr);

    // Upload texture to OpenGL
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, cairo_image_surface_get_data(surface));

    cairo_surface_destroy(surface);
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

    static const GLfloat tex_coords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    glUseProgram(shader_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(tex_uniform, 0);

    glVertexAttribPointer(position_attr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(position_attr);

    glVertexAttribPointer(tex_coord_attr, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);
    glEnableVertexAttribArray(tex_coord_attr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int main() {
    RenderContext *ctx = render_init(640, 480);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize render context\n");
        return 1;
    }

    init_gl();
    render_text_to_texture();

    render_loop(ctx, draw_frame);

    glDeleteTextures(1, &texture);
    glDeleteProgram(shader_program);

    render_cleanup(ctx);
    return 0;
}
