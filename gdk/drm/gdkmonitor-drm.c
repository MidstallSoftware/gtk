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

#include "gdkmonitor-drm.h"
#include "gdkdisplay-drm.h"
#include "gdkframeclockidleprivate.h"
#include "gdkglcontext-drm.h"
#include "gdkmonitor.h"
#include "gdksurface-drm-private.h"

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "gdk/gdkprivate.h"
#include "gdkprivate-drm.h"

G_DEFINE_TYPE (GdkDrmMonitor, gdk_drm_monitor, GDK_TYPE_MONITOR)

struct drm_fb
{
  struct gbm_bo *bo;
  uint32_t fb_id;
};

static void
drm_fb_destroy_callback (struct gbm_bo *bo, void *data)
{
  int drm_fd = gbm_device_get_fd (gbm_bo_get_device (bo));
  struct drm_fb *fb = data;

  if (fb->fb_id)
    drmModeRmFB (drm_fd, fb->fb_id);
  free (fb);
}

static struct drm_fb *
drm_fb_get_from_bo (struct gbm_bo *bo)
{
  int drm_fd = gbm_device_get_fd (gbm_bo_get_device (bo));
  struct drm_fb *fb = gbm_bo_get_user_data (bo);
  uint32_t width, height, format,
      strides[4] = { 0 }, handles[4] = { 0 },
      offsets[4] = { 0 }, flags = 0;
  int ret = -1;

  if (fb)
    return fb;

  fb = malloc (sizeof (struct drm_fb));
  fb->bo = bo;

  width = gbm_bo_get_width (bo);
  height = gbm_bo_get_height (bo);
  format = gbm_bo_get_format (bo);

  uint64_t modifiers[4] = { 0 };
  modifiers[0] = gbm_bo_get_modifier (bo);
  const int num_planes = gbm_bo_get_plane_count (bo);
  for (int i = 0; i < num_planes; i++)
    {
      handles[i] = gbm_bo_get_handle_for_plane (bo, i).u32;
      strides[i] = gbm_bo_get_stride_for_plane (bo, i);
      offsets[i] = gbm_bo_get_offset (bo, i);
      modifiers[i] = modifiers[0];
    }

  if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID)
    {
      flags = DRM_MODE_FB_MODIFIERS;
    }

  ret = drmModeAddFB2WithModifiers (drm_fd, width, height,
                                    format, handles, strides, offsets,
                                    modifiers, &fb->fb_id, flags);

  if (ret)
    {
      memcpy (handles, (uint32_t[4]){ gbm_bo_get_handle (bo).u32, 0, 0, 0 }, 16);
      memcpy (strides, (uint32_t[4]){ gbm_bo_get_stride (bo), 0, 0, 0 }, 16);
      memset (offsets, 0, 16);
      ret = drmModeAddFB2 (drm_fd, width, height, format, handles, strides, offsets, &fb->fb_id, 0);
    }

  if (ret)
    {
      free (fb);
      return NULL;
    }

  gbm_bo_set_user_data (bo, fb, drm_fb_destroy_callback);

  return fb;
}

static void
gdk_drm_monitor_after_paint (GdkFrameClock *frame_clock, GdkDrmMonitor *self)
{
  GdkMonitor *monitor = GDK_MONITOR (self);
  GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (monitor->display);

  g_message ("%p", self->gbm_surface);
  if (self->gbm_surface == NULL)
    return;

  struct gbm_bo *bo = gbm_surface_lock_front_buffer (self->gbm_surface);
  if (bo == NULL)
    {
      g_warning ("Failed to lock the front buffer for connector %u", self->connector);
      return;
    }

  struct drm_fb *fb = drm_fb_get_from_bo (bo);
  if (fb == NULL)
    {
      g_warning ("Failed to create a frame buffer for connector %u", self->connector);
      gbm_surface_release_buffer (self->gbm_surface, bo);
      return;
    }

  uint32_t crtc_id = _gdk_drm_monitor_get_crtc_id (self);
  int ret = drmModePageFlip (drm_display->fd, crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, NULL);

  if (ret < 0)
    {
      g_warning ("Failed to page flip CRTC %u", crtc_id);
      gbm_surface_release_buffer (self->gbm_surface, bo);
      return;
    }

  GDK_DEBUG (MISC, "Page flip occurred on CRTC %u", crtc_id);

  gbm_surface_release_buffer (self->gbm_surface, self->gbm_bo);
  self->gbm_bo = bo;
}

static void
gdk_drm_monitor_finalize (GObject *object)
{
  GdkDrmMonitor *self = GDK_DRM_MONITOR (object);

  g_clear_object (&self->frame_clock);
  g_clear_list (&self->surfaces, (GDestroyNotify) g_object_unref);

  G_OBJECT_CLASS (gdk_drm_monitor_parent_class)->finalize (object);
}

static void
gdk_drm_monitor_class_init (GdkDrmMonitorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gdk_drm_monitor_finalize;
}

static void
gdk_drm_monitor_init (GdkDrmMonitor *self)
{
  self->frame_clock = _gdk_frame_clock_idle_new ();

  g_signal_connect (self->frame_clock, "after-paint", G_CALLBACK (gdk_drm_monitor_after_paint), self);
}

/**
 * gdk_drm_monitor_set_position:
 * @self: The monitor instance
 * @x: The X position
 * @y: The Y position
 *
 * Places the monitor onto a specific X, Y position within
 * th monitor layout.
 */
void
gdk_drm_monitor_set_position (GdkDrmMonitor *self, int x, int y)
{
  GdkMonitor *monitor = GDK_MONITOR (self);

  GdkRectangle rect = {
    .x = x,
    .y = y,
    .width = monitor->geometry.width,
    .height = monitor->geometry.height,
  };

  gdk_monitor_set_geometry (monitor, &rect);
}

/**
 * gdk_drm_monitor_get_info:
 * @self: The monitor instance
 *
 * Reads the EDID and gets the display info of
 * the monitor.
 *
 * Returns: (transfer full): a libdisplay-info info
 */
struct di_info *
gdk_drm_monitor_get_info (GdkDrmMonitor *self)
{
  if (self->edid == 0)
    return NULL;

  GdkMonitor *monitor = GDK_MONITOR (self);
  GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (monitor->display);

  drmModeObjectPropertiesPtr props = drmModeObjectGetProperties (drm_display->fd, self->connector, DRM_MODE_OBJECT_CONNECTOR);
  g_assert (props != NULL);

  for (uint32_t i = 0; i < props->count_props; i++)
    {
      if (props->props[i] == self->edid)
        {
          drmModePropertyBlobPtr blob = drmModeGetPropertyBlob (drm_display->fd, props->prop_values[i]);
          if (blob == NULL)
            break;

          struct di_info *info = di_info_parse_edid (blob->data, blob->length);

          drmModeFreePropertyBlob (blob);
          drmModeFreeObjectProperties (props);
          return info;
        }
    }

  drmModeFreeObjectProperties (props);
  return NULL;
}

uint32_t
_gdk_drm_monitor_get_crtc_id (GdkDrmMonitor *self)
{
  GdkMonitor *monitor = GDK_MONITOR (self);
  GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (monitor->display);

  drmModeConnectorPtr conn = drmModeGetConnector (drm_display->fd, self->connector);
  g_assert (conn != NULL);

  uint32_t crtc_id = 0;

  if (self->crtc_id != 0)
    {
      for (uint32_t i = 0; i < conn->count_props; i++)
        {
          if (conn->props[i] == self->crtc_id)
            {
              crtc_id = conn->prop_values[i];
              break;
            }
        }
    }
  else if (conn->encoder_id != 0)
    {
      drmModeEncoderPtr encoder = drmModeGetEncoder (drm_display->fd, conn->encoder_id);
      g_assert (encoder != NULL);

      crtc_id = encoder->crtc_id;

      drmModeFreeEncoder (encoder);
    }

  drmModeFreeConnector (conn);

  return crtc_id;
}

void
_gdk_drm_monitor_sync_connector (GdkDrmMonitor *self)
{
  GdkMonitor *monitor = GDK_MONITOR (self);
  GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (monitor->display);

  drmModeConnectorPtr conn = drmModeGetConnector (drm_display->fd, self->connector);
  g_assert (conn != NULL);

  gdk_monitor_set_connector (monitor, g_strdup_printf ("%s-%d", drmModeGetConnectorTypeName (conn->connector_type), conn->connector_type_id));

  gdk_monitor_set_physical_size (monitor, conn->mmWidth, conn->mmHeight);

  switch (conn->subpixel)
    {
    case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB);
      break;
    case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR);
      break;
    case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_VERTICAL_RGB);
      break;
    case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_VERTICAL_BGR);
      break;
    case DRM_MODE_SUBPIXEL_NONE:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_NONE);
      break;
    case DRM_MODE_SUBPIXEL_UNKNOWN:
      gdk_monitor_set_subpixel_layout (monitor, GDK_SUBPIXEL_LAYOUT_UNKNOWN);
      break;
    }

  for (uint32_t i = 0; i < conn->count_props; i++)
    {
      drmModePropertyRes *prop = drmModeGetProperty (drm_display->fd, conn->props[i]);
      if (prop == NULL)
        continue;

      if (g_strcmp0 (prop->name, "CRTC_ID") == 0)
        {
          self->crtc_id = prop->prop_id;
        }
      else if (g_strcmp0 (prop->name, "EDID") == 0)
        {
          self->edid = prop->prop_id;
        }
      else if (g_strcmp0 (prop->name, "non-desktop") == 0)
        {
          self->non_desktop = prop->values[0] != 0;
        }

      drmModeFreeProperty (prop);
    }

  uint32_t crtc_id = _gdk_drm_monitor_get_crtc_id (self);

  if (crtc_id != 0)
    {
      drmModeCrtcPtr crtc = drmModeGetCrtc (drm_display->fd, crtc_id);
      g_assert (crtc != NULL);

      GdkRectangle rect = {
        .x = monitor->geometry.x,
        .y = monitor->geometry.y,
        .width = crtc->mode.hdisplay,
        .height = crtc->mode.vdisplay,
      };

      gboolean has_changed = !gdk_rectangle_equal (&monitor->geometry, &rect);
      gdk_monitor_set_geometry (monitor, &rect);

      if (has_changed)
        {
          g_clear_pointer (&self->gbm_surface, gbm_surface_destroy);

          g_assert (drm_display->gbm_device != NULL);
          self->gbm_surface = gbm_surface_create (drm_display->gbm_device, rect.width, rect.height, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
          g_assert (self->gbm_surface != NULL);

          self->gbm_bo = gbm_surface_lock_front_buffer (self->gbm_surface);
          g_assert (self->gbm_bo != NULL);

          struct drm_fb *fb = drm_fb_get_from_bo (self->gbm_bo);
          g_assert (fb != NULL);

          int ret = drmModeSetCrtc (drm_display->fd, crtc_id, fb->fb_id, 0, 0, &self->connector, 1, &crtc->mode);
          if (ret < 0)
            {
              g_warning ("Failed to set CRTC %u with connector %u", crtc_id, self->connector);
            }
        }

      int32_t refresh = (crtc->mode.clock * 1000000LL / crtc->mode.htotal + crtc->mode.vtotal / 2) / crtc->mode.vtotal;

      if (crtc->mode.flags & DRM_MODE_FLAG_INTERLACE)
        {
          refresh *= 2;
        }

      if (crtc->mode.flags & DRM_MODE_FLAG_DBLSCAN)
        {
          refresh /= 2;
        }

      if (crtc->mode.vscan > 1)
        {
          refresh /= crtc->mode.vscan;
        }

      gdk_monitor_set_refresh_rate (monitor, refresh);

      drmModeFreeCrtc (crtc);
    }
  else
    {
      g_clear_pointer (&self->gbm_surface, gbm_surface_destroy);
    }

  struct di_info *info = gdk_drm_monitor_get_info (self);
  if (info != NULL)
    {
      char *make = di_info_get_make (info);
      gdk_monitor_set_manufacturer (monitor, make);
      g_clear_pointer (&make, g_free);

      char *model = di_info_get_model (info);
      gdk_monitor_set_model (monitor, model);
      g_clear_pointer (&model, g_free);

      di_info_destroy (info);
    }

  drmModeFreeConnector (conn);
}
