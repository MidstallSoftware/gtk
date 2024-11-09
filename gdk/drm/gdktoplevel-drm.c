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

#include "config.h"
#include "gdkprivate-drm.h"

#include "gdkdrmsurface.h"
#include "gdkdrmtoplevel.h"
#include "gdkmonitor-drm.h"
#include "gdksurfaceprivate.h"
#include "gdktoplevelprivate.h"

#include "gdksurface-drm-private.h"

struct _GdkDrmToplevel
{
  GdkDrmSurface parent_instance;
};

typedef struct
{
  GdkDrmSurfaceClass parent_class;
} GdkDrmToplevelClass;

#define LAST_PROP 1

static void gdk_drm_toplevel_iface_init (GdkToplevelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GdkDrmToplevel, gdk_drm_toplevel, GDK_TYPE_DRM_SURFACE, G_IMPLEMENT_INTERFACE (GDK_TYPE_TOPLEVEL, gdk_drm_toplevel_iface_init))

static void
gdk_drm_toplevel_present (GdkToplevel *toplevel, GdkToplevelLayout *layout)
{
  GdkSurface *surface = GDK_SURFACE (toplevel);
  gdk_drm_surface_ensure_monitor (surface);
}

static void
gdk_drm_toplevel_hide (GdkSurface *surface)
{
  GdkMonitor *monitor = gdk_display_get_monitor_at_surface (gdk_surface_get_display (surface), surface);

  if (monitor != NULL)
    {
      GdkDrmMonitor *drm_monitor = GDK_DRM_MONITOR (monitor);

      drm_monitor->surfaces = g_list_remove (drm_monitor->surfaces, surface);
      g_object_unref (surface);
    }
}

static void
gdk_drm_toplevel_constructed (GObject *object)
{
  GdkDrmToplevel *self = GDK_DRM_TOPLEVEL (object);

  gdk_drm_surface_ensure_monitor (GDK_SURFACE (object));

  G_OBJECT_CLASS (gdk_drm_toplevel_parent_class)->constructed (object);
}

static void
gdk_drm_toplevel_iface_init (GdkToplevelInterface *iface)
{
  iface->present = gdk_drm_toplevel_present;
}

static void
gdk_drm_toplevel_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  GdkDrmToplevel *self = GDK_DRM_TOPLEVEL (object);

  switch (prop_id)
    {
    case LAST_PROP + GDK_TOPLEVEL_PROP_TITLE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_STARTUP_ID:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_TRANSIENT_FOR:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_MODAL:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_ICON_LIST:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_DECORATED:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_DELETABLE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_FULLSCREEN_MODE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_SHORTCUTS_INHIBITED:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdk_drm_toplevel_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
  GdkDrmToplevel *self = GDK_DRM_TOPLEVEL (object);

  switch (prop_id)
    {
    case LAST_PROP + GDK_TOPLEVEL_PROP_TITLE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_STARTUP_ID:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_TRANSIENT_FOR:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_MODAL:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_ICON_LIST:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_DECORATED:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_DELETABLE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_FULLSCREEN_MODE:
      break;
    case LAST_PROP + GDK_TOPLEVEL_PROP_SHORTCUTS_INHIBITED:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdk_drm_toplevel_class_init (GdkDrmToplevelClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkSurfaceClass *surface_class = GDK_SURFACE_CLASS (class);

  object_class->constructed = gdk_drm_toplevel_constructed;
  object_class->set_property = gdk_drm_toplevel_set_property;
  object_class->get_property = gdk_drm_toplevel_get_property;

  surface_class->hide = gdk_drm_toplevel_hide;

  gdk_toplevel_install_properties (object_class, 1);
}

static void
gdk_drm_toplevel_init (GdkDrmToplevel *toplevel)
{
}
