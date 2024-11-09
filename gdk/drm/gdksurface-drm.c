/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gdkdisplay-drm.h"
#include "gdkdrmsurface.h"
#include "gdkglcontext-drm.h"
#include "gdkmonitor-drm.h"
#include "gdkprivate-drm.h"
#include "gdksurface-drm-private.h"
#include "gdksurfaceprivate.h"
#include "gdktoplevelprivate.h"

G_DEFINE_TYPE (GdkDrmSurface, gdk_drm_surface, GDK_TYPE_SURFACE)

static void
gdk_drm_surface_get_geometry (GdkSurface *surface,
                              int *x,
                              int *y,
                              int *width,
                              int *height)
{
  if (!GDK_SURFACE_DESTROYED (surface))
    {
      if (x)
        *x = surface->x;
      if (y)
        *y = surface->y;
      if (width)
        *width = surface->width;
      if (height)
        *height = surface->height;
    }
}

static void
gdk_drm_surface_get_root_coords (GdkSurface *surface,
                                 int x,
                                 int y,
                                 int *root_x,
                                 int *root_y)
{

  if (root_x)
    *root_x = surface->x + x;

  if (root_y)
    *root_y = surface->y + y;
}

static void
gdk_drm_surface_set_input_region (GdkSurface *surface,
                                  cairo_region_t *input_region)
{
}

static void
gdk_drm_surface_class_init (GdkDrmSurfaceClass *class)
{
  GdkSurfaceClass *surface_class = GDK_SURFACE_CLASS (class);

  surface_class->get_geometry = gdk_drm_surface_get_geometry;
  surface_class->get_root_coords = gdk_drm_surface_get_root_coords;
  surface_class->set_input_region = gdk_drm_surface_set_input_region;
}

static void
gdk_drm_surface_init (GdkDrmSurface *impl)
{
}

static inline void
get_egl_window_size (GdkSurface *surface,
                     int *width,
                     int *height)
{
  GdkDrmSurface *self = GDK_DRM_SURFACE (surface);

  *width = surface->width;
  *height = surface->height;
}

void
gdk_drm_surface_ensure_egl_window (GdkSurface *surface)
{
  GdkDrmSurface *self = GDK_DRM_SURFACE (surface);
  int width, height;

  if (self->gbm_surface != NULL)
    return;

  get_egl_window_size (surface, &width, &height);

  GdkDrmGLContext *drm_gl_context = GDK_DRM_GL_CONTEXT (surface->gl_paint_context);

  if (drm_gl_context->is_gbm)
    {
      GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (gdk_surface_get_display (surface));
      self->gbm_surface = gbm_surface_create (drm_display->gbm_device, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
      gdk_surface_set_egl_native_window (surface, self->gbm_surface);
    }
}

void
gdk_drm_surface_ensure_monitor (GdkSurface *surface)
{
  GdkMonitor *monitor = gdk_display_get_monitor_at_surface (gdk_surface_get_display (surface), surface);

  if (monitor != NULL)
    {
      GdkDrmMonitor *drm_monitor = GDK_DRM_MONITOR (monitor);
      gdk_surface_set_frame_clock (surface, drm_monitor->frame_clock);

      drm_monitor->surfaces = g_list_append (drm_monitor->surfaces, g_object_ref (surface));
    }
}
