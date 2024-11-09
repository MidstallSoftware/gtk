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

#include "gdkdrmpopup.h"
#include "gdkdrmsurface.h"
#include "gdkframeclockidleprivate.h"
#include "gdkpopupprivate.h"
#include "gdksurfaceprivate.h"

#include "gdksurface-drm-private.h"

struct _GdkDrmPopup
{
  GdkDrmSurface parent_instance;
};

typedef struct
{
  GdkDrmSurfaceClass parent_class;
} GdkDrmPopupClass;

static void gdk_drm_popup_iface_init (GdkPopupInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GdkDrmPopup, gdk_drm_popup, GDK_TYPE_DRM_SURFACE, G_IMPLEMENT_INTERFACE (GDK_TYPE_POPUP, gdk_drm_popup_iface_init))

static void
gdk_drm_popup_iface_init (GdkPopupInterface *iface)
{
}

static void
gdk_drm_popup_class_init (GdkDrmPopupClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  gdk_popup_install_properties (object_class, 1);
}

static void
gdk_drm_popup_init (GdkDrmPopup *popup)
{
}
