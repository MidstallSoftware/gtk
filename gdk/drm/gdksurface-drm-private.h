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

#pragma once

#include "gdkprivate-drm.h"
#include "gdksurfaceprivate.h"

#include <gbm.h>

struct _GdkDrmSurface
{
  GdkSurface parent_instance;

  struct gbm_surface *gbm_surface;
};

typedef struct _GdkDrmSurfaceClass GdkDrmSurfaceClass;
struct _GdkDrmSurfaceClass
{
  GdkSurfaceClass parent_class;
};

#define GDK_DRM_SURFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DRM_SURFACE, GdkDrmSurfaceClass))

#define GDK_DRM_SURFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DRM_SURFACE, GdkDrmSurfaceClass))

void
gdk_drm_surface_ensure_egl_window (GdkSurface *surface);

void
gdk_drm_surface_ensure_monitor (GdkSurface *surface);
