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

#include "gdkseat-drm.h"
#include "gdkdebugprivate.h"
#include "gdkdrm.h"
#include "gdkprivate-drm.h"
#include "gdkprivate.h"
#include "gdkseatprivate.h"
#include "gdktypes.h"

#include <fcntl.h>

#ifdef HAS_LIBSEAT
#include <libseat.h>
#endif

G_DEFINE_TYPE (GdkDrmSeat, gdk_drm_seat, GDK_TYPE_SEAT)

#ifdef HAS_LIBSEAT
static void
handle_enable_seat (struct libseat *seat_handle, void *data)
{
  GdkDrmSeat *seat = GDK_DRM_SEAT (data);
  seat->libseat_active++;

  GDK_DEBUG (MISC, "seat enable %d", seat->libseat_active);
}

static void
handle_disable_seat (struct libseat *seat_handle, void *data)
{
  GdkDrmSeat *seat = GDK_DRM_SEAT (data);
  seat->libseat_active--;

  GDK_DEBUG (MISC, "seat disable %d", seat->libseat_active);
}

static struct libseat_seat_listener seat_listener = {
  .enable_seat = handle_enable_seat,
  .disable_seat = handle_disable_seat,
};

G_GNUC_PRINTF (2, 0)
static void
log_handler (enum libseat_log_level level, const char *fmt, va_list args)
{
  switch (level)
    {
    case LIBSEAT_LOG_LEVEL_ERROR:
      g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, fmt, args);
      break;
    case LIBSEAT_LOG_LEVEL_INFO:
      g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, fmt, args);
      break;
    default:
      g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, fmt, args);
      break;
    }
}
#endif

static void
gdk_drm_seat_finalize (GObject *object)
{
  GdkDrmSeat *seat = GDK_DRM_SEAT (object);

#ifdef HAS_LIBSEAT
  g_clear_pointer (&seat->libseat_handle, libseat_close_seat);
#endif

  G_OBJECT_CLASS (gdk_drm_seat_parent_class)->finalize (object);
}

static void
gdk_drm_seat_class_init (GdkDrmSeatClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkSeatClass *seat_class = GDK_SEAT_CLASS (klass);

  object_class->finalize = gdk_drm_seat_finalize;
}

static void
gdk_drm_seat_init (GdkDrmSeat *seat)
{
#ifdef HAS_LIBSEAT
  libseat_set_log_handler (log_handler);
  libseat_set_log_level (LIBSEAT_LOG_LEVEL_DEBUG);

  seat->libseat_active = 0;
  seat->libseat_handle = libseat_open_seat (&seat_listener, seat);
  if (!seat->libseat_handle)
    {
      g_warning ("Could not open a seat with libseat");
    }
#endif
}
