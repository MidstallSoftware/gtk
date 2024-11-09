/* GDK - The GIMP Drawing Kit
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#pragma once

#include "config.h"

#include "gdk/gdkmonitorprivate.h"
#include "gdkdrmmonitor.h"
#include "gdkframeclock.h"
#include <gbm.h>

#define GDK_DRM_MONITOR_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DRM_MONITOR, GdkDrmMonitorClass))
#define GDK_IS_DRM_MONITOR_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DRM_MONITOR))
#define GDK_DRM_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DRM_MONITOR, GdkDrmMonitorClass))

struct _GdkDrmMonitor
{
  GdkMonitor parent_instance;

  uint32_t connector;
  uint32_t crtc_id;
  uint32_t edid;

  gboolean non_desktop;

  GdkFrameClock *frame_clock;
  GList *surfaces;

  struct gbm_surface *gbm_surface;
  struct gbm_bo *gbm_bo;
};

struct _GdkDrmMonitorClass
{
  GdkMonitorClass parent_class;
};

uint32_t _gdk_drm_monitor_get_crtc_id (GdkDrmMonitor *self);
void _gdk_drm_monitor_sync_connector (GdkDrmMonitor *self);
