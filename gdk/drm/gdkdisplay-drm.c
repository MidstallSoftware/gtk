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

#include "gdkdisplay-drm.h"
#include "gdkdisplay.h"
#include "gdkdrm.h"
#include "gdkglcontext-drm.h"
#include "gdkmonitor-drm.h"
#include "gdkseat-drm.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "gdk/gdkprivate.h"
#include "gdkprivate-drm.h"

G_DEFINE_TYPE (GdkDrmDisplay, gdk_drm_display, GDK_TYPE_DISPLAY)

static gboolean
gdk_drm_display_scan_connectors (GdkDrmDisplay *self)
{
  drmModeResPtr res = drmModeGetResources (self->fd);
  if (res == NULL)
    return FALSE;

  guint num_monitors = g_list_model_get_n_items (G_LIST_MODEL (self->monitors));

  for (guint i = 0; i < num_monitors; i++)
    {
      GdkDrmMonitor *monitor = GDK_DRM_MONITOR (g_list_model_get_item (G_LIST_MODEL (self->monitors), i));

      gboolean has_connector = FALSE;

      for (int x = 0; x < res->count_connectors; x++)
        {
          if (monitor->connector == res->connectors[x])
            {
              has_connector = TRUE;
              break;
            }
        }

      if (!has_connector)
        {
          g_list_store_remove (self->monitors, i);
          gdk_monitor_invalidate (GDK_MONITOR (monitor));
        }

      g_object_unref (monitor);
    }

  for (int i = 0; i < res->count_connectors; i++)
    {
      gboolean has_monitor = FALSE;

      for (guint x = 0; x < num_monitors; x++)
        {
          GdkDrmMonitor *monitor = GDK_DRM_MONITOR (g_list_model_get_item (G_LIST_MODEL (self->monitors), x));

          if (monitor->connector == res->connectors[i])
            {
              has_monitor = TRUE;
              _gdk_drm_monitor_sync_connector (monitor);
              g_object_unref (monitor);
              break;
            }

          g_object_unref (monitor);
        }

      if (has_monitor)
        continue;

      GdkDrmMonitor *monitor = GDK_DRM_MONITOR (g_object_new (GDK_TYPE_DRM_MONITOR, "display", GDK_DISPLAY (self), NULL));

      monitor->connector = res->connectors[i];
      _gdk_drm_monitor_sync_connector (monitor);

      g_list_store_append (self->monitors, monitor);
      g_object_unref (monitor);
    }

  drmModeFreeResources (res);
  return TRUE;
}

static void
gdk_drm_display_dispose (GObject *object)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (object);

  g_clear_pointer (&self->monitors, g_list_store_remove_all);

  G_OBJECT_CLASS (gdk_drm_display_parent_class)->dispose (object);
}

static void
gdk_drm_display_finalize (GObject *object)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (object);

  g_clear_pointer (&self->path, (GDestroyNotify) g_free);
  g_clear_pointer (&self->udev_event, (GDestroyNotify) g_io_channel_unref);
  g_clear_pointer (&self->udev_monitor, (GDestroyNotify) udev_monitor_unref);
  g_clear_pointer (&self->udev, (GDestroyNotify) udev_unref);
  g_clear_pointer (&self->gbm_device, (GDestroyNotify) gbm_device_destroy);
  g_clear_object (&self->monitors);

  if (self->fd > 0)
    {
#ifdef HAS_LIBSEAT
      GdkDrmSeat *seat = GDK_DRM_SEAT (self->seat);
      if (seat->libseat_handle && self->dev_id > 0)
        {
          libseat_close_device (seat->libseat_handle, self->dev_id);
          self->dev_id = -1;
        }
#endif

      close (self->fd);
      self->fd = -1;
    }

  g_clear_object (&self->seat);

  G_OBJECT_CLASS (gdk_drm_display_parent_class)->finalize (object);
}

static gboolean
gdk_drm_display_get_setting (GdkDisplay *display,
                             const char *name,
                             GValue *value)
{
  return FALSE;
}

static GListModel *
gdk_drm_display_get_monitors (GdkDisplay *display)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (display);

  return G_LIST_MODEL (self->monitors);
}

static const char *
gdk_drm_display_get_name (GdkDisplay *display)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (display);
  return self->path;
}

static void
gdk_drm_display_class_init (GdkDrmDisplayClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDisplayClass *display_class = GDK_DISPLAY_CLASS (class);

  object_class->dispose = gdk_drm_display_dispose;
  object_class->finalize = gdk_drm_display_finalize;

  display_class->toplevel_type = GDK_TYPE_DRM_TOPLEVEL;
  display_class->popup_type = GDK_TYPE_DRM_POPUP;
  display_class->get_setting = gdk_drm_display_get_setting;
  display_class->get_monitors = gdk_drm_display_get_monitors;
  display_class->init_gl = gdk_drm_display_init_gl;
  display_class->get_name = gdk_drm_display_get_name;
}

static void
gdk_drm_display_init (GdkDrmDisplay *self)
{
  self->monitors = g_list_store_new (GDK_TYPE_MONITOR);
}

static gboolean
handle_udev_event (GIOChannel *channel, GIOCondition cond, GdkDrmDisplay *self)
{
  struct udev_device *udev_dev = udev_monitor_receive_device (self->udev_monitor);
  if (udev_dev == NULL)
    return TRUE;

  if (g_strcmp0 (udev_device_get_devnode (udev_dev), self->path) != 0)
    {
      udev_device_unref (udev_dev);
      return TRUE;
    }

  const char *action = udev_device_get_action (udev_dev);

  if (g_strcmp0 (action, "change") == 0)
    {
      const char *hotplug = udev_device_get_property_value (udev_dev, "HOTPLUG");
      if (g_strcmp0 (hotplug, "1") == 0)
        {
          const char *connector = udev_device_get_property_value (udev_dev, "CONNECTOR");

          if (connector != NULL)
            {
              uint32_t connector_id = strtoul (connector, NULL, 10);
              drmModeConnectorPtr conn = drmModeGetConnector (self->fd, connector_id);
              if (conn != NULL)
                {
                  GdkDrmMonitor *monitor = GDK_DRM_MONITOR (g_object_new (GDK_TYPE_DRM_MONITOR, "display", GDK_DISPLAY (self), NULL));

                  monitor->connector = connector_id;
                  _gdk_drm_monitor_sync_connector (monitor);

                  g_list_store_append (self->monitors, monitor);
                  g_object_unref (monitor);

                  drmModeFreeConnector (conn);
                }
              else
                {
                  guint num_monitors = g_list_model_get_n_items (G_LIST_MODEL (self->monitors));

                  for (guint i = 0; i < num_monitors; i++)
                    {
                      GdkDrmMonitor *monitor = GDK_DRM_MONITOR (g_list_model_get_item (G_LIST_MODEL (self->monitors), i));

                      if (monitor->connector == connector_id)
                        {
                          g_list_store_remove (self->monitors, i);
                          gdk_monitor_invalidate (GDK_MONITOR (monitor));
                        }

                      g_object_unref (monitor);
                    }
                }
            }
        }
    }

  udev_device_unref (udev_dev);
  return TRUE;
}

static GdkDisplay *
_gdk_drm_display_device_open (GdkDrmSeat *seat, struct udev *udev, const char *path)
{
  g_debug ("Trying display device %s", path);

  int fd = -1;
  int dev_id = -1;

#ifdef HAS_LIBSEAT
  if (seat->libseat_handle)
    {
      dev_id = libseat_open_device (seat->libseat_handle, path, &fd);
      if (dev_id == -1)
        {
          fd = -1;
          g_critical ("libseat_open_device \"%s\": %s", path, strerror (errno));
        }
      else
        {
          struct stat st;
          if (fstat (fd, &st) < 0)
            {
              libseat_close_device (seat->libseat_handle, dev_id);
              close (fd);
              fd = -1;
            }
        }
    }
#endif

  if (fd == -1)
    fd = open (path, O_RDWR);
  if (fd == -1)
    return NULL;

  GdkDisplay *display = GDK_DISPLAY (g_object_new (GDK_TYPE_DRM_DISPLAY, NULL));
  GdkDrmDisplay *drm_display = GDK_DRM_DISPLAY (display);

  drm_display->seat = GDK_SEAT (g_object_ref (seat));
  drm_display->fd = fd;
  drm_display->path = g_strdup (path);

#ifdef HAS_LIBSEAT
  drm_display->dev_id = dev_id;
#endif

  drm_display->udev = udev_ref (udev);
  drm_display->udev_monitor = udev_monitor_new_from_netlink (drm_display->udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype (drm_display->udev_monitor, "drm", NULL);

  udev_monitor_enable_receiving (drm_display->udev_monitor);

  drm_display->udev_event = g_io_channel_unix_new (udev_monitor_get_fd (drm_display->udev_monitor));
  g_io_add_watch (drm_display->udev_event, G_IO_IN, (GIOFunc) handle_udev_event, drm_display);

  drm_display->use_atomics = drmSetClientCap (fd, DRM_CLIENT_CAP_ATOMIC, 1) == 0;

  drm_display->gbm_device = gdk_drm_display_open_gbm_device (display);
  if (drm_display->gbm_device == NULL)
    {
      g_object_unref (drm_display);
      return NULL;
    }

  if (!gdk_drm_display_scan_connectors (drm_display))
    {
      g_object_unref (drm_display);
      return NULL;
    }

  gdk_display_emit_opened (display);
  return GDK_DISPLAY (display);
}

static GdkDisplay *
_gdk_drm_display_find_from_udev (GdkDrmSeat *seat, struct udev *udev)
{
  struct udev_enumerate *en = udev_enumerate_new (udev);
  if (!en)
    return NULL;

  udev_enumerate_add_match_subsystem (en, "drm");
  udev_enumerate_add_match_sysname (en, DRM_PRIMARY_MINOR_NAME "[0-9]*");

  if (udev_enumerate_scan_devices (en) != 0)
    {
      udev_enumerate_unref (en);
      return NULL;
    }

  struct udev_list_entry *entry;
  udev_list_entry_foreach (entry, udev_enumerate_get_list_entry (en))
  {
    const char *path = udev_list_entry_get_name (entry);

    struct udev_device *dev = udev_device_new_from_syspath (udev, path);
    if (!dev)
      continue;

    GdkDisplay *display = _gdk_drm_display_device_open (seat, udev, path);
    if (display != NULL)
      {
        udev_device_unref (dev);
        udev_enumerate_unref (en);
        return display;
      }

    udev_device_unref (dev);
  }

  udev_enumerate_unref (en);
  return NULL;
}

static GdkDisplay *
_gdk_drm_display_find (GdkDrmSeat *seat, struct udev *udev)
{
#define MAX_DRM_DEVICES 8
  drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };

  int num_devices = drmGetDevices2 (0, devices, MAX_DRM_DEVICES);

  for (int i = 0; i < num_devices; i++)
    {
      drmDevicePtr device = devices[i];

      if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
        continue;

      GdkDisplay *display = _gdk_drm_display_device_open (seat, udev, device->nodes[DRM_NODE_PRIMARY]);
      if (display != NULL)
        {
          drmFreeDevices (devices, num_devices);
          return display;
        }
    }

  drmFreeDevices (devices, num_devices);
  return NULL;
}

GdkDisplay *
_gdk_drm_display_open (const char *display_name)
{
  struct udev *udev = udev_new ();
  if (!udev)
    return NULL;

  GdkDrmSeat *seat = GDK_DRM_SEAT (g_object_new (GDK_TYPE_DRM_SEAT, NULL));

  GdkDisplay *device = NULL;
  if (display_name == NULL)
    display_name = g_getenv ("DRM_DEVICE");
  if (display_name != NULL)
    device = _gdk_drm_display_device_open (seat, udev, display_name);

  if (device == NULL)
    device = _gdk_drm_display_find_from_udev (seat, udev);
  if (device == NULL)
    device = _gdk_drm_display_find (seat, udev);

  udev_unref (udev);
  g_object_unref (seat);
  return device;
}

struct gbm_device *
gdk_drm_display_open_gbm_device (GdkDisplay *display)
{
  GdkDrmDisplay *self = GDK_DRM_DISPLAY (display);

  char *render_name = drmGetRenderDeviceNameFromFd (self->fd);
  if (render_name == NULL)
    {
      render_name = drmGetPrimaryDeviceNameFromFd (self->fd);
      if (render_name == NULL)
        return NULL;
    }

  int render_fd = open (render_name, O_RDWR | O_CLOEXEC);
  if (render_fd < 0)
    {
      free (render_name);
      return NULL;
    }

  free (render_name);

  struct gbm_device *gbm_dev = gbm_create_device (render_fd);
  if (gbm_dev == NULL)
    {
      close (render_fd);
      return NULL;
    }

  return gbm_dev;
}
