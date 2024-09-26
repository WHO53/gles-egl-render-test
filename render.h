#ifndef RENDER_H
#define RENDER_H

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "proto/xdg-shell-client-protocol.h"

typedef struct {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_egl_window *egl_window;
    struct xdg_wm_base *xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    int width;
    int height;
    int configured;
} RenderContext;

RenderContext* render_init(int width, int height);
void render_loop(RenderContext *ctx, void (*draw_frame)(void));
void render_cleanup(RenderContext *ctx);

#endif // RENDER_H
