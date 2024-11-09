gdk_drm_sources = files([
  'gdkdisplay-drm.c',
  'gdkglcontext-drm.c',
  'gdkmonitor-drm.c',
  'gdkpopup-drm.c',
  'gdkseat-drm.c',
  'gdksurface-drm.c',
  'gdktoplevel-drm.c',
])

gdk_drm_public_headers = files([
  'gdkdrmdisplay.h',
  'gdkdrmglcontext.h',
  'gdkdrmmonitor.h',
  'gdkdrmpopup.h',
  'gdkdrmseat.h',
  'gdkdrmsurface.h',
  'gdkdrmtoplevel.h',
])

install_headers(gdk_drm_public_headers, 'gdkdrm.h', subdir: 'gtk-4.0/gdk/drm/')

gdk_drm_deps = [
  libdrm,
  libgbm,
  libudev,
  libdisplay_info,
]

gdk_drm_cflags = []

if libseat.found()
  gdk_drm_deps += [libseat]
  gdk_drm_cflags += ['-DHAS_LIBSEAT=1']
endif

libgdk_drm = static_library('gdk-drm',
  sources: [gdk_drm_sources],
  include_directories: [ confinc, gdkinc ],
  c_args: [
    '-DGTK_COMPILATION',
    '-DG_LOG_DOMAIN="Gdk"',
  ] + common_cflags + gdk_drm_cflags,
  dependencies: [ gdk_deps, gdk_drm_deps ],
)

# Used to generate pkg-config Requires
wayland_public_deps = [libdrm]
