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

#include <gdk/gdk.h>

G_BEGIN_DECLS

#ifdef GTK_COMPILATION
typedef struct _GdkDrmDisplay GdkDrmDisplay;
#else
typedef GdkDisplay GdkDrmDisplay;
#endif
typedef struct _GdkDrmDisplayClass GdkDrmDisplayClass;

#define GDK_TYPE_DRM_DISPLAY (gdk_drm_display_get_type ())
#define GDK_DRM_DISPLAY(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DRM_DISPLAY, GdkDrmDisplay))
#define GDK_DRM_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DRM_DISPLAY, GdkDrmDisplayClass))
#define GDK_IS_DRM_DISPLAY(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DRM_DISPLAY))
#define GDK_IS_DRM_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DRM_DISPLAY))
#define GDK_DRM_DISPLAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DRM_DISPLAY, GdkDrmDisplayClass))

GDK_AVAILABLE_IN_ALL
GType gdk_drm_display_get_type (void);

G_END_DECLS
