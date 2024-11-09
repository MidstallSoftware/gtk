/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gdkglcontext-drm.h"
#include "gdkdisplay-drm.h"
#include "gdksurface-drm-private.h"

#include <fcntl.h>
#include <xf86drm.h>

G_DEFINE_TYPE (GdkDrmGLContext, gdk_drm_gl_context, GDK_TYPE_GL_CONTEXT)

static void
gdk_drm_gl_context_finalize (GObject *object)
{
  GdkDrmGLContext *self = GDK_DRM_GL_CONTEXT (object);

  G_OBJECT_CLASS (gdk_drm_gl_context_parent_class)->finalize (object);
}

static void
gdk_drm_gl_context_begin_frame (GdkDrawContext *draw_context,
                                GdkMemoryDepth depth,
                                cairo_region_t *region,
                                GdkColorState **out_color_state,
                                GdkMemoryDepth *out_depth)
{
  gdk_drm_surface_ensure_egl_window (gdk_draw_context_get_surface (draw_context));

  GDK_DRAW_CONTEXT_CLASS (gdk_drm_gl_context_parent_class)->begin_frame (draw_context, depth, region, out_color_state, out_depth);
}

static void
gdk_drm_gl_context_end_frame (GdkDrawContext *draw_context,
                              cairo_region_t *painted)
{

  GDK_DRAW_CONTEXT_CLASS (gdk_drm_gl_context_parent_class)->end_frame (draw_context, painted);
}

static void
gdk_drm_gl_context_class_init (GdkDrmGLContextClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDrawContextClass *draw_context_class = GDK_DRAW_CONTEXT_CLASS (class);
  GdkGLContextClass *context_class = GDK_GL_CONTEXT_CLASS (class);

  object_class->finalize = gdk_drm_gl_context_finalize;

  draw_context_class->begin_frame = gdk_drm_gl_context_begin_frame;
  draw_context_class->end_frame = gdk_drm_gl_context_end_frame;

  context_class->backend_type = GDK_GL_EGL;
}

static void
gdk_drm_gl_context_init (GdkDrmGLContext *self)
{
}

GdkGLContext *
gdk_drm_display_init_gl (GdkDisplay *display,
                         GError **error)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (display);

  if (epoxy_has_egl_extension (NULL, "EXT_platform_device"))
    {
      PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = (gpointer) epoxy_eglGetProcAddress ("eglQueryDevicesEXT");
      if (eglQueryDevicesEXT == NULL)
        {
          g_set_error (error, gdk_gl_error_quark (), 1, "Failed to resolve eglQueryDevicesEXT");
          return NULL;
        }

      PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT = (gpointer) epoxy_eglGetProcAddress ("eglQueryDeviceStringEXT");
      if (eglQueryDeviceStringEXT == NULL)
        {
          g_set_error (error, gdk_gl_error_quark (), 1, "Failed to resolve eglQueryDeviceStringEXT");
          return NULL;
        }

      EGLint num_devices = 0;
      if (!eglQueryDevicesEXT (0, NULL, &num_devices))
        {
          g_set_error (error, gdk_gl_error_quark (), 2, "Failed to query devices");
          return NULL;
        }

      EGLDeviceEXT *devices = calloc (num_devices, sizeof (EGLDeviceEXT));
      g_assert (devices != NULL);

      if (!eglQueryDevicesEXT (num_devices, devices, &num_devices))
        {
          g_set_error (error, gdk_gl_error_quark (), 2, "Failed to query devices");
          return NULL;
        }

      drmDevicePtr drm_dev = NULL;
      int ret = drmGetDevice (self->fd, &drm_dev);
      if (ret < 0)
        {
          free (devices);
          g_set_error (error, gdk_gl_error_quark (), 3, "Failed to get DRM device");
          return NULL;
        }

      for (int i = 0; i < num_devices; i++)
        {
          const char *egl_dev_name = eglQueryDeviceStringEXT (devices[i], EGL_DRM_DEVICE_FILE_EXT);
          if (egl_dev_name == NULL)
            continue;

          for (size_t x = 0; x < DRM_NODE_MAX; x++)
            {
              if (!(drm_dev->available_nodes & (1 << x)))
                continue;

              if (g_strcmp0 (drm_dev->nodes[x], egl_dev_name) == 0)
                {
                  EGLDeviceEXT egl_device = devices[i];
                  free (devices);
                  drmFreeDevice (&drm_dev);

                  if (!gdk_display_init_egl (display,
                                             EGL_PLATFORM_DEVICE_EXT,
                                             egl_device,
                                             TRUE,
                                             error))
                    return NULL;

                  GdkDrmGLContext *context = g_object_new (GDK_TYPE_DRM_GL_CONTEXT,
                                                           "display", display,
                                                           NULL);

                  context->egl = egl_device;
                  context->is_gbm = false;
                  return GDK_GL_CONTEXT (context);
                }
            }
        }

      free (devices);
      drmFreeDevice (&drm_dev);
    }

  if (epoxy_has_egl_extension (NULL, "KHR_platform_gbm"))
    {
      if (!gdk_display_init_egl (display,
                                 EGL_PLATFORM_GBM_KHR,
                                 self->gbm_device,
                                 TRUE,
                                 error))
        {
          return NULL;
        }

      GdkDrmGLContext *context = g_object_new (GDK_TYPE_DRM_GL_CONTEXT,
                                               "display", display,
                                               NULL);
      context->is_gbm = true;
      return GDK_GL_CONTEXT (context);
    }

  g_set_error (error, gdk_gl_error_quark (), 5, "Failed to initialize GL");
  return NULL;
}
