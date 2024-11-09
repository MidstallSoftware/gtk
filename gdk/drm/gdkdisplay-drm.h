/*
 * gdkdisplay-drm.h
 *
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

#pragma once

#include <glib.h>

#include <gdk/gdk.h> /* For gdk_get_program_class() */
#include <gdk/gdkkeys.h>
#include <gdk/gdksurface.h>

#include "gdkdisplayprivate.h"
#include "gdkdrmdisplay.h"

#include <gbm.h>

G_BEGIN_DECLS

struct _GdkDrmDisplay
{
  GdkDisplay parent_instance;

  GListStore *monitors;

  GdkSeat *seat;

  char *path;
  int fd;
  bool use_atomics;

  struct udev *udev;
  struct udev_monitor *udev_monitor;
  GIOChannel *udev_event;

  struct gbm_device *gbm_device;

#ifdef HAS_LIBSEAT
  int dev_id;
#endif
};

struct _GdkDrmDisplayClass
{
  GdkDisplayClass parent_class;
};

struct gbm_device *
gdk_drm_display_open_gbm_device (GdkDisplay *display);

G_END_DECLS
