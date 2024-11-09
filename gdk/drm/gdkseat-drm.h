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

#include "gdk/gdkseatprivate.h"
#include "gdkdrmseat.h"

#ifdef HAS_LIBSEAT
#include <libseat.h>
#endif

#define GDK_DRM_SEAT_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DRM_SEAT, GdkDrmSeatClass))
#define GDK_IS_DRM_SEAT_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DRM_SEAT))
#define GDK_DRM_SEAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DRM_SEAT, GdkDrmSeatClass))

struct _GdkDrmSeat
{
  GdkSeat parent_instance;

#ifdef HAS_LIBSEAT
  struct libseat *libseat_handle;
  int libseat_active;
#endif
};

struct _GdkDrmSeatClass
{
  GdkSeatClass parent_class;
};
