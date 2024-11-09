/* gdkdrmseat.h: DRM GdkSeat
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#if !defined(__GDKDRM_H_INSIDE__) && !defined(GTK_COMPILATION)
#error "Only <gdk/drm/gdkdrm.h> can be included directly."
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

#ifdef GTK_COMPILATION
typedef struct _GdkDrmSeat GdkDrmSeat;
#else
typedef GdkSeat GdkDrmSeat;
#endif

typedef struct _GdkDrmSeatClass GdkDrmSeatClass;

#define GDK_TYPE_DRM_SEAT (gdk_drm_seat_get_type ())
#define GDK_DRM_SEAT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_DRM_SEAT, GdkDrmSeat))
#define GDK_IS_DRM_SEAT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DRM_SEAT))

GDK_AVAILABLE_IN_ALL
GType gdk_drm_seat_get_type (void) G_GNUC_CONST;

G_END_DECLS
