#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/eglext.h>

static void registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                             const char *interface, uint32_t version) {
    RenderContext *ctx = (RenderContext *)data;
    if (strcmp(interface, "wl_compositor") == 0) {
        ctx->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        ctx->xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
    }
}

static void registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    // Handle removal if necessary
}

static const struct wl_registry_listener registry_listener = {
    registry_handler,
    registry_remover
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    xdg_wm_base_ping
};

static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface,
                                         uint32_t serial) {
    RenderContext *ctx = (RenderContext *)data;
    xdg_surface_ack_configure(xdg_surface, serial);
    ctx->configured = 1;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    xdg_surface_handle_configure
};

static void init_egl(RenderContext *ctx) {
    EGLint major, minor, count, n;
    EGLConfig *configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = 
        (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    
    if (get_platform_display) {
        ctx->egl_display = get_platform_display(EGL_PLATFORM_WAYLAND_EXT, ctx->display, NULL);
    } else {
        ctx->egl_display = eglGetDisplay((EGLNativeDisplayType)ctx->display);
    }

    eglInitialize(ctx->egl_display, &major, &minor);
    eglGetConfigs(ctx->egl_display, NULL, 0, &count);
    configs = calloc(count, sizeof *configs);
    
    eglChooseConfig(ctx->egl_display, config_attribs, configs, count, &n);
    ctx->egl_config = configs[0];

    ctx->egl_context = eglCreateContext(ctx->egl_display, ctx->egl_config, 
                                        EGL_NO_CONTEXT, context_attribs);
    free(configs);
}

static void create_window(RenderContext *ctx) {
    ctx->egl_window = wl_egl_window_create(ctx->surface, ctx->width, ctx->height);
    ctx->egl_surface = eglCreateWindowSurface(ctx->egl_display, ctx->egl_config, 
                                              ctx->egl_window, NULL);
    eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context);
}

RenderContext* render_init(int width, int height) {
    RenderContext *ctx = calloc(1, sizeof(RenderContext));
    ctx->width = width;
    ctx->height = height;

    ctx->display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(ctx->display);
    wl_registry_add_listener(registry, &registry_listener, ctx);

    wl_display_dispatch(ctx->display);
    wl_display_roundtrip(ctx->display);

    ctx->surface = wl_compositor_create_surface(ctx->compositor);
    xdg_wm_base_add_listener(ctx->xdg_wm_base, &xdg_wm_base_listener, NULL);

    ctx->xdg_surface = xdg_wm_base_get_xdg_surface(ctx->xdg_wm_base, ctx->surface);
    xdg_surface_add_listener(ctx->xdg_surface, &xdg_surface_listener, ctx);

    ctx->xdg_toplevel = xdg_surface_get_toplevel(ctx->xdg_surface);
    xdg_surface_set_window_geometry(ctx->xdg_surface, 0, 0, width, height);
    wl_surface_commit(ctx->surface);

    init_egl(ctx);

    while (!ctx->configured) {
        wl_display_dispatch(ctx->display);
    }

    create_window(ctx);

    return ctx;
}

void render_loop(RenderContext *ctx, void (*draw_frame)(void)) {
    while (1) {
        wl_display_dispatch_pending(ctx->display);
        draw_frame();
        eglSwapBuffers(ctx->egl_display, ctx->egl_surface);
    }
}

void render_cleanup(RenderContext *ctx) {
    eglDestroySurface(ctx->egl_display, ctx->egl_surface);
    wl_egl_window_destroy(ctx->egl_window);
    eglDestroyContext(ctx->egl_display, ctx->egl_context);
    eglTerminate(ctx->egl_display);
    xdg_toplevel_destroy(ctx->xdg_toplevel);
    xdg_surface_destroy(ctx->xdg_surface);
    wl_surface_destroy(ctx->surface);
    xdg_wm_base_destroy(ctx->xdg_wm_base);
    wl_compositor_destroy(ctx->compositor);
    wl_display_disconnect(ctx->display);
    free(ctx);
}
