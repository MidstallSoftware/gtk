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
 */

#pragma once

#if !defined(__GDKDRM_H_INSIDE__) && !defined(GTK_COMPILATION)
#error "Only <gdk/drm/gdkdrm.h> can be included directly."
#endif

#include <gdk/gdkmonitor.h>
#include <libdisplay-info/edid.h>
#include <libdisplay-info/info.h>

G_BEGIN_DECLS

#define GDK_TYPE_DRM_MONITOR (gdk_drm_monitor_get_type ())
#define GDK_DRM_MONITOR(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DRM_MONITOR, GdkDrmMonitor))
#define GDK_IS_DRM_MONITOR(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DRM_MONITOR))

#ifdef GTK_COMPILATION
typedef struct _GdkDrmMonitor GdkDrmMonitor;
#else
typedef GdkMonitor GdkDrmMonitor;
#endif
typedef struct _GdkDrmMonitorClass GdkDrmMonitorClass;

GDK_AVAILABLE_IN_ALL
GType gdk_drm_monitor_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void gdk_drm_monitor_set_position (GdkDrmMonitor *self, int x, int y);

GDK_AVAILABLE_IN_ALL
struct di_info *gdk_drm_monitor_get_info (GdkDrmMonitor *self);

G_END_DECLS
