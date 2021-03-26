/*
 * wl_client_side_decorations.c
 * Copyright (C) 2021 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#include "wl_client_side_decorations.h"

#include "backend_utils.h"
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#define decs window->wl.decorations
#define tb decs.title_bar
#define eb decs.edges

#define ARGB(a, r, g, b) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

static const uint32_t bg_color = 0xfffefefe;

static void
free_title_bar_resources(_GLFWwindow *window) {
    if (tb.front_buffer) {
        wl_buffer_destroy(tb.front_buffer);
        tb.front_buffer = NULL;
    }
    if (tb.back_buffer) {
        wl_buffer_destroy(tb.back_buffer);
        tb.back_buffer = NULL;
    }
    if (tb.data) {
        munmap(tb.data, tb.buffer_sz * 2);
        tb.data = NULL;
    }
}

static void
free_edge_resources(_GLFWwindow *window) {
    if (eb.left) { wl_buffer_destroy(eb.left); eb.left = NULL; }
    if (eb.right) { wl_buffer_destroy(eb.right); eb.right = NULL; }
    if (eb.bottom) { wl_buffer_destroy(eb.bottom); eb.bottom = NULL; }
}

static bool
create_shm_buffers_for_title_bar(_GLFWwindow* window) {
    free_title_bar_resources(window);
    const size_t stride = 4 * window->wl.width;
    tb.buffer_sz = stride * window->wl.decorations.metrics.top;
    const size_t mapping_sz = tb.buffer_sz * 2;

    int fd = createAnonymousFile(mapping_sz);
    if (fd < 0) {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Wayland: Creating a buffer file for %zu B failed: %s",
                        mapping_sz, strerror(errno));
        return false;
    }
    tb.data = mmap(NULL, mapping_sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (tb.data == MAP_FAILED) {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Wayland: mmap failed: %s", strerror(errno));
        close(fd);
        return false;
    }
    for (uint32_t *px = (uint32_t*)tb.data, *end = (uint32_t*)(tb.data + tb.buffer_sz); px < end; px++) *px = bg_color;
    struct wl_shm_pool* pool = wl_shm_create_pool(_glfw.wl.shm, fd, mapping_sz);
    close(fd);
#define c(offset) wl_shm_pool_create_buffer( \
            pool, offset, window->wl.width, window->wl.decorations.metrics.top, stride, WL_SHM_FORMAT_ARGB8888);
    tb.front_buffer = c(0); tb.back_buffer = c(tb.buffer_sz);
#undef c
    wl_shm_pool_destroy(pool);
    return true;
}

static void
render_left_edge(uint8_t *data, size_t width, size_t height) {
    for (uint32_t *px = (uint32_t*)data, *end = (uint32_t*)(data + 4 * width * height); px < end; px++) *px = bg_color;
}
#define render_right_edge render_left_edge
#define render_bottom_edge render_left_edge

static bool
create_shm_buffers_for_edges(_GLFWwindow* window) {
    free_edge_resources(window);
    int scale = window->wl.scale;
    if (scale < 1) scale = 1;

    size_t vertical_width = decs.metrics.width, vertical_height = window->wl.height + decs.metrics.top;
    size_t horizontal_height = decs.metrics.width, horizontal_width = window->wl.width + 2 * decs.metrics.width;
    vertical_width *= scale; vertical_height *= scale;
    horizontal_width *= scale; horizontal_height *= scale;
    const size_t mapping_sz = 4 * (2 * vertical_width * vertical_height + horizontal_height * horizontal_width);

    int fd = createAnonymousFile(mapping_sz);
    if (fd < 0) {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Wayland: Creating a buffer file for %zu B failed: %s",
                        mapping_sz, strerror(errno));
        return false;
    }
    uint8_t *data = mmap(NULL, mapping_sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Wayland: mmap failed: %s", strerror(errno));
        close(fd);
        return false;
    }
    render_left_edge(data, vertical_width, vertical_height);
    render_right_edge(data + 4 * vertical_width * vertical_height, vertical_width, vertical_height);
    render_bottom_edge(data + 8 * vertical_width * vertical_height, horizontal_width, horizontal_height);
    struct wl_shm_pool* pool = wl_shm_create_pool(_glfw.wl.shm, fd, mapping_sz);
    close(fd);
    eb.left = wl_shm_pool_create_buffer(
            pool, 0, vertical_width, vertical_height, vertical_width * 4, WL_SHM_FORMAT_ARGB8888);
    eb.right = wl_shm_pool_create_buffer(
            pool, 4 * vertical_width * vertical_height, vertical_width, vertical_height, vertical_width * 4, WL_SHM_FORMAT_ARGB8888);
    eb.bottom = wl_shm_pool_create_buffer(
            pool, 8 * vertical_width * vertical_height, horizontal_width, horizontal_height, horizontal_width * 4, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    return true;
}

void
free_csd_surfaces(_GLFWwindow *window) {
#define d(which) {\
    if (decs.subsurfaces.which) wl_subsurface_destroy(decs.subsurfaces.which); decs.subsurfaces.which = NULL; \
    if (decs.surfaces.which) wl_surface_destroy(decs.surfaces.which); decs.surfaces.which = NULL; \
}
    d(left); d(top); d(right); d(bottom);
#undef d
}

#define position_decoration_surfaces(which, x, y) { \
    wl_subsurface_set_position(decs.subsurfaces.which, x, y); \
    wl_surface_commit(decs.surfaces.which); \
}

#define create_decoration_surfaces(which, buffer) { \
    decs.surfaces.which = wl_compositor_create_surface(_glfw.wl.compositor); \
    wl_surface_set_buffer_scale(decs.surfaces.which, window->wl.scale < 1 ? 1 : window->wl.scale); \
    decs.subsurfaces.which = wl_subcompositor_get_subsurface(_glfw.wl.subcompositor, decs.surfaces.which, window->wl.surface); \
    wl_surface_attach(decs.surfaces.which, buffer, 0, 0); \
}

bool
ensure_csd_resources(_GLFWwindow *window) {
    const bool size_changed = (
        decs.for_window_size.width != window->wl.width ||
        decs.for_window_size.height != window->wl.height ||
        decs.for_window_size.scale != window->wl.scale
    );
    if (size_changed) {
        free_title_bar_resources(window);
        free_edge_resources(window);
    }
    if (!decs.edges.left) {
        if (!create_shm_buffers_for_edges(window)) return false;
    }
    if (!decs.title_bar.front_buffer) {
        if (!create_shm_buffers_for_title_bar(window)) return false;
    }
    int x, y;

    x = 0; y = -decs.metrics.top;
    if (!decs.surfaces.top) create_decoration_surfaces(top, decs.title_bar.front_buffer);
    position_decoration_surfaces(top, x, y);

    x = -decs.metrics.width; y = -decs.metrics.top;
    if (!decs.surfaces.left) create_decoration_surfaces(left, decs.edges.left);
    position_decoration_surfaces(left, x, y);

    x = window->wl.width; y = -decs.metrics.top;
    if (!decs.surfaces.right) create_decoration_surfaces(right, decs.edges.right);
    position_decoration_surfaces(right, x, y);

    x = -decs.metrics.width; y = window->wl.height;
    if (!decs.surfaces.bottom) create_decoration_surfaces(bottom, decs.edges.bottom);
    position_decoration_surfaces(bottom, x, y);

    decs.for_window_size.width = window->wl.width;
    decs.for_window_size.height = window->wl.height;
    decs.for_window_size.scale = window->wl.scale;
    return true;
}

void
free_all_csd_resources(_GLFWwindow *window) {
    free_csd_surfaces(window);
    free_title_bar_resources(window);
    free_edge_resources(window);
}

void
resize_csd(_GLFWwindow *window) {
    ensure_csd_resources(window);
}
