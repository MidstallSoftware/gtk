#include "config.h"

#include "gdkwaylandcolor-private.h"
#include "gdksurface-wayland-private.h"
#include <gdk/wayland/xx-color-management-v2-client-protocol.h>


struct _GdkWaylandColor
{
  struct xx_color_manager_v2 *color_manager;
  struct {
    unsigned int intents;
    unsigned int features;
    unsigned int transfers;
    unsigned int primaries;
  } color_manager_supported;
  struct xx_image_description_v2 *srgb;
  struct xx_image_description_v2 *srgb_linear;
  struct xx_image_description_v2 *rec2100_pq;
  struct xx_image_description_v2 *rec2100_linear;
};

static void
xx_color_manager_v2_supported_intent (void                       *data,
                                      struct xx_color_manager_v2 *xx_color_manager_v2,
                                      uint32_t                    render_intent)
{
  GdkWaylandColor *color = data;

  color->color_manager_supported.intents |= (1 << render_intent);
}

static void
xx_color_manager_v2_supported_feature (void                       *data,
                                       struct xx_color_manager_v2 *xx_color_manager_v2,
                                       uint32_t                    feature)
{
  GdkWaylandColor *color = data;

  color->color_manager_supported.features |= (1 << feature);
}

static void
xx_color_manager_v2_supported_tf_named (void                       *data,
                                        struct xx_color_manager_v2 *xx_color_manager_v2,
                                        uint32_t                    tf)
{
  GdkWaylandColor *color = data;

  color->color_manager_supported.transfers |= (1 << tf);
}

static void
xx_color_manager_v2_supported_primaries_named (void                       *data,
                                               struct xx_color_manager_v2 *xx_color_manager_v2,
                                               uint32_t                    primaries)
{
  GdkWaylandColor *color = data;

  color->color_manager_supported.primaries |= (1 << primaries);
}

static struct xx_color_manager_v2_listener color_manager_listener = {
  xx_color_manager_v2_supported_intent,
  xx_color_manager_v2_supported_feature,
  xx_color_manager_v2_supported_tf_named,
  xx_color_manager_v2_supported_primaries_named,
};

GdkWaylandColor *
gdk_wayland_color_new (struct wl_registry *registry,
                       uint32_t            id,
                       uint32_t            version)
{
  GdkWaylandColor *color;

  color = g_new0 (GdkWaylandColor, 1);

  color->color_manager = wl_registry_bind (registry,
                                           id,
                                           &xx_color_manager_v2_interface,
                                           MIN (version, 2));

  xx_color_manager_v2_add_listener (color->color_manager,
                                    &color_manager_listener,
                                    color);

  return color;
}

void
gdk_wayland_color_free (GdkWaylandColor *color)
{
  g_clear_pointer (&color->color_manager, xx_color_manager_v2_destroy);
  g_clear_pointer (&color->srgb, xx_image_description_v2_destroy);
  g_clear_pointer (&color->srgb_linear, xx_image_description_v2_destroy);
  g_clear_pointer (&color->rec2100_pq, xx_image_description_v2_destroy);
  g_clear_pointer (&color->rec2100_linear, xx_image_description_v2_destroy);
  g_free (color);
}

struct wl_proxy *
gdk_wayland_color_get_color_manager (GdkWaylandColor *color)
{
  return (struct wl_proxy *) color->color_manager;
}

static void
std_image_desc_failed (void *data,
                       struct xx_image_description_v2 *desc,
                       uint32_t cause,
                       const char *msg)
{
  g_warning ("Failed to get one of the standard image descriptions: %s", msg);
  xx_image_description_v2_destroy (desc);
}

static void
std_image_desc_ready (void *data,
                      struct xx_image_description_v2 *desc,
                      uint32_t identity)
{
  struct xx_image_description_v2 **ptr = data;

  *ptr = desc;
}

static struct xx_image_description_v2_listener std_image_desc_listener = {
  std_image_desc_failed,
  std_image_desc_ready,
};

static void
create_image_desc (GdkWaylandColor                 *color,
                   uint32_t                         primaries,
                   uint32_t                         tf,
                   struct xx_image_description_v2 **out_desc)
{
  struct xx_image_description_creator_params_v2 *creator;
  struct xx_image_description_v2 *desc;

  creator = xx_color_manager_v2_new_parametric_creator (color->color_manager);

  xx_image_description_creator_params_v2_set_primaries_named (creator, primaries);
  xx_image_description_creator_params_v2_set_tf_named (creator, tf);

  desc = xx_image_description_creator_params_v2_create (creator);

  xx_image_description_v2_add_listener (desc, &std_image_desc_listener, out_desc);
}

gboolean
gdk_wayland_color_prepare (GdkWaylandColor *color)
{
  if (color->color_manager)
    {
      const char *intents[] = {
        "perceptual", "relative", "saturation", "absolute", "relative-bpc"
      };
      const char *features[] = {
        "icc-v2-v4", "parametric", "set-primaries", "set-tf-power",
        "set-mastering-display-primaries", "extended-target-volume"
      };
      const char *tf[] = {
        "bt709", "gamma22", "gamma28", "st240", "linear", "log100",
        "log316", "xvycc", "bt1361", "srgb", "ext-srgb", "pq", "st428", "hlg"
      };
      const char *primaries[] = {
        "srgb", "pal-m", "pal", "ntsc", "generic-film", "bt2020", "xyz",
        "dci-p3", "display-p3", "adobe-rgb",
      };

     for (int i = 0; i < G_N_ELEMENTS (intents); i++)
        {
          GDK_DEBUG (MISC, "Rendering intent %d (%s): %s",
                   i, intents[i], color->color_manager_supported.intents & (1 << i) ? "✓" : "✗");
        }

      for (int i = 0; i < G_N_ELEMENTS (features); i++)
        {
          GDK_DEBUG (MISC, "Feature %d (%s): %s",
                   i, features[i], color->color_manager_supported.features & (1 << i) ? "✓" : "✗");
        }

      for (int i = 0; i < G_N_ELEMENTS (tf); i++)
        {
          GDK_DEBUG (MISC, "Transfer function %d (%s): %s",
                   i, tf[i], color->color_manager_supported.transfers & (1 << i) ? "✓" : "✗");
        }

      for (int i = 0; i < G_N_ELEMENTS (primaries); i++)
        {
          GDK_DEBUG (MISC, "Primaries %d (%s): %s",
                   i, primaries[i], color->color_manager_supported.primaries& (1 << i) ? "✓" : "✗");
        }
    }

  if (color->color_manager &&
      !(color->color_manager_supported.intents & (1 << XX_COLOR_MANAGER_V2_RENDER_INTENT_PERCEPTUAL)))
    {
      GDK_DEBUG (MISC, "Not using color management: Missing perceptual render intent");
      g_clear_pointer (&color->color_manager, xx_color_manager_v2_destroy);
    }

  if (color->color_manager &&
      (!(color->color_manager_supported.features & (1 << XX_COLOR_MANAGER_V2_FEATURE_PARAMETRIC)) ||
       !(color->color_manager_supported.transfers & (1 << XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_SRGB)) ||
       !(color->color_manager_supported.primaries & (1 << XX_COLOR_MANAGER_V2_PRIMARIES_SRGB))))

    {
      GDK_DEBUG (MISC, "Not using color management: Can't create srgb image description");
      g_clear_pointer (&color->color_manager, xx_color_manager_v2_destroy);
    }

  if (color->color_manager)
    {
      create_image_desc (color,
                         XX_COLOR_MANAGER_V2_PRIMARIES_SRGB,
                         XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_SRGB,
                         &color->srgb);

      if (color->color_manager_supported.transfers & (1 << XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR))
        create_image_desc (color,
                           XX_COLOR_MANAGER_V2_PRIMARIES_SRGB,
                           XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR,
                           &color->srgb_linear);

      if (color->color_manager_supported.primaries & (1 << XX_COLOR_MANAGER_V2_PRIMARIES_BT2020))
        {
          if (color->color_manager_supported.transfers & (1 << XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_ST2084_PQ))
            create_image_desc (color,
                               XX_COLOR_MANAGER_V2_PRIMARIES_BT2020,
                               XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_ST2084_PQ,
                               &color->rec2100_pq);

          if (color->color_manager_supported.transfers & (1 << XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR))
            create_image_desc (color,
                               XX_COLOR_MANAGER_V2_PRIMARIES_BT2020,
                               XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR,
                               &color->rec2100_linear);
        }
    }

  return color->color_manager != NULL;
}

struct _GdkWaylandColorSurface
{
  GdkWaylandColor *color;
  struct xx_color_management_surface_v2 *surface;
  GdkColorStateChanged callback;
  gpointer data;
};

typedef struct
{
  GdkWaylandColorSurface *surface;

  struct xx_image_description_v2 *image_desc;
  struct xx_image_description_info_v2 *info;

  int32_t icc;
  uint32_t icc_size;
  uint32_t r_x, r_y, g_x, g_y, b_x, b_y, w_x, w_y;
  uint32_t primaries;
  uint32_t tf_power;
  uint32_t tf_named;
  uint32_t target_r_x, target_r_y, target_g_x, target_g_y, target_b_x, target_b_y, target_w_x, target_w_y;
  uint32_t target_min_lum, target_max_lum;
  uint32_t target_max_cll, target_max_fall;

  unsigned int has_icc : 1;
  unsigned int has_primaries : 1;
  unsigned int has_primaries_named : 1;
  unsigned int has_tf_power : 1;
  unsigned int has_tf_named : 1;
  unsigned int has_target_primaries : 1;
  unsigned int has_target_luminance : 1;
  unsigned int has_target_max_cll : 1;
  unsigned int has_target_max_fall : 1;
} ImageDescription;

static GdkColorState *
gdk_color_state_from_image_description_bits (ImageDescription *desc)
{
  GdkColorState *cs = GDK_COLOR_STATE_SRGB;

  if (desc->has_primaries_named && desc->has_tf_named)
    {
      if (desc->primaries == XX_COLOR_MANAGER_V2_PRIMARIES_SRGB &&
          desc->tf_named == XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_SRGB)
        {
          cs = GDK_COLOR_STATE_SRGB;
        }
      else if (desc->primaries == XX_COLOR_MANAGER_V2_PRIMARIES_SRGB &&
               desc->tf_named == XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR)
        {
          cs = GDK_COLOR_STATE_SRGB_LINEAR;
        }
      else if (desc->primaries == XX_COLOR_MANAGER_V2_PRIMARIES_BT2020 &&
               desc->tf_named == XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_ST2084_PQ)
        {
          cs = GDK_COLOR_STATE_REC2100_PQ;
        }
      else if (desc->primaries == XX_COLOR_MANAGER_V2_PRIMARIES_BT2020 &&
               desc->tf_named == XX_COLOR_MANAGER_V2_TRANSFER_FUNCTION_LINEAR)
        {
          cs = GDK_COLOR_STATE_REC2100_LINEAR;
        }
    }

  return cs;
}

static void
image_desc_info_done (void *data,
                      struct xx_image_description_info_v2 *info)
{
  ImageDescription *desc = data;
  GdkWaylandColorSurface *self = desc->surface;
  GdkColorState *cs;

  cs = gdk_color_state_from_image_description_bits (desc);

  if (self->callback)
    self->callback (desc->surface, cs, self->data);

  gdk_color_state_unref (cs);

  xx_image_description_v2_destroy (desc->image_desc);
  xx_image_description_info_v2_destroy (desc->info);
  g_free (desc);
}

static void
image_desc_info_icc_file (void *data,
                          struct xx_image_description_info_v2 *info,
                          int32_t icc,
                          uint32_t icc_size)
{
  ImageDescription *desc = data;

  desc->icc = icc;
  desc->icc_size = icc_size;
  desc->has_icc = 1;
}

static void
image_desc_info_primaries (void *data,
                           struct xx_image_description_info_v2 *info,
                           uint32_t r_x, uint32_t r_y,
                           uint32_t g_x, uint32_t g_y,
                           uint32_t b_x, uint32_t b_y,
                           uint32_t w_x, uint32_t w_y)
{
  ImageDescription *desc = data;

  desc->r_x = r_x; desc->r_y = r_y;
  desc->g_x = g_x; desc->r_y = g_y;
  desc->b_x = b_x; desc->r_y = b_y;
  desc->w_x = w_x; desc->r_y = w_y;
  desc->has_primaries = 1;
}

static void
image_desc_info_primaries_named (void *data,
                                 struct xx_image_description_info_v2 *info,
                                 uint32_t primaries)
{
  ImageDescription *desc = data;

  desc->primaries = primaries;
  desc->has_primaries_named = 1;
}

static void
image_desc_info_tf_power (void *data,
                          struct xx_image_description_info_v2 *info,
                          uint32_t tf_power)
{
  ImageDescription *desc = data;

  desc->tf_power = tf_power;
  desc->has_tf_power = 1;
}

static void
image_desc_info_tf_named (void *data,
                          struct xx_image_description_info_v2 *info,
                          uint32_t tf)
{
  ImageDescription *desc = data;

  desc->tf_named = tf;
  desc->has_tf_named = 1;
}

static void
image_desc_info_target_primaries (void *data,
                                  struct xx_image_description_info_v2 *info,
                                  uint32_t r_x, uint32_t r_y,
                                  uint32_t g_x, uint32_t g_y,
                                  uint32_t b_x, uint32_t b_y,
                                  uint32_t w_x, uint32_t w_y)
{
  ImageDescription *desc = data;

  desc->target_r_x = r_x; desc->target_r_y = r_y;
  desc->target_g_x = g_x; desc->target_r_y = g_y;
  desc->target_b_x = b_x; desc->target_r_y = b_y;
  desc->target_w_x = w_x; desc->target_r_y = w_y;
  desc->has_target_primaries = 1;
}

static void
image_desc_info_target_luminance (void *data,
                                  struct xx_image_description_info_v2 *info,
                                  uint32_t min_lum,
                                  uint32_t max_lum)
{
  ImageDescription *desc = data;

  desc->target_min_lum = min_lum;
  desc->target_max_lum = max_lum;
  desc->has_target_luminance = 1;
}

static void
image_desc_info_target_max_cll (void *data,
                                struct xx_image_description_info_v2 *info,
                                uint32_t max_cll)
{
  ImageDescription *desc = data;

  desc->target_max_cll = max_cll;
  desc->has_target_max_cll = 1;
}

static void
image_desc_info_target_max_fall (void *data,
                                 struct xx_image_description_info_v2 *info,
                                 uint32_t max_fall)
{
  ImageDescription *desc = data;

  desc->target_max_fall = max_fall;
  desc->has_target_max_fall = 1;
}

static struct xx_image_description_info_v2_listener info_listener = {
  image_desc_info_done,
  image_desc_info_icc_file,
  image_desc_info_primaries,
  image_desc_info_primaries_named,
  image_desc_info_tf_power,
  image_desc_info_tf_named,
  image_desc_info_target_primaries,
  image_desc_info_target_luminance,
  image_desc_info_target_max_cll,
  image_desc_info_target_max_fall,
};

static void
image_desc_failed (void                           *data,
                   struct xx_image_description_v2 *image_desc,
                   uint32_t                        cause,
                   const char                     *msg)
{
  ImageDescription *desc = data;
  GdkWaylandColorSurface *self = desc->surface;

  self->callback (self, GDK_COLOR_STATE_SRGB, self->data);

  xx_image_description_v2_destroy (desc->image_desc);
  g_free (desc);
}

static void
image_desc_ready (void                           *data,
                  struct xx_image_description_v2 *image_desc,
                  uint32_t                        identity)
{
  ImageDescription *desc = data;

  desc->info = xx_image_description_v2_get_information (image_desc);

  xx_image_description_info_v2_add_listener (desc->info, &info_listener, desc);
}

static const struct xx_image_description_v2_listener image_desc_listener = {
  image_desc_failed,
  image_desc_ready
};

static void
preferred_changed (void *data,
                   struct xx_color_management_surface_v2 *color_mgmt_surface)
{
  GdkWaylandColorSurface *self = data;
  ImageDescription *desc;

  desc = g_new0 (ImageDescription, 1);

  desc->surface = self;

  desc->image_desc = xx_color_management_surface_v2_get_preferred (self->surface);

  xx_image_description_v2_add_listener (desc->image_desc, &image_desc_listener, desc);
}

static const struct xx_color_management_surface_v2_listener color_listener = {
  preferred_changed,
};

GdkWaylandColorSurface *
gdk_wayland_color_surface_new (GdkWaylandColor          *color,
                               struct wl_surface        *wl_surface,
                               GdkColorStateChanged      callback,
                               gpointer                  data)
{
  GdkWaylandColorSurface *self;

  self = g_new0 (GdkWaylandColorSurface, 1);

  self->color = color;

  self->surface = xx_color_manager_v2_get_surface (color->color_manager, wl_surface);

  self->callback = callback;
  self->data = data;

  xx_color_management_surface_v2_add_listener (self->surface, &color_listener, self);

  return self;
}

void
gdk_wayland_color_surface_free (GdkWaylandColorSurface *self)
{
  xx_color_management_surface_v2_destroy (self->surface);

  g_free (self);
}


static struct xx_image_description_v2 *
gdk_wayland_color_get_image_description (GdkWaylandColor *color,
                                         GdkColorState   *cs)
{
  if (gdk_color_state_equal (cs, GDK_COLOR_STATE_SRGB))
    return color->srgb;
  else if (gdk_color_state_equal (cs, GDK_COLOR_STATE_SRGB_LINEAR))
    return color->srgb_linear;
  else if (gdk_color_state_equal (cs, GDK_COLOR_STATE_REC2100_PQ))
    return color->rec2100_pq;
  else if (gdk_color_state_equal (cs, GDK_COLOR_STATE_REC2100_LINEAR))
    return color->rec2100_linear;
  else
    return color->srgb;
}

void
gdk_wayland_color_surface_set_color_state (GdkWaylandColorSurface *self,
                                           GdkColorState          *cs)
{
  struct xx_image_description_v2 *desc;

  desc = gdk_wayland_color_get_image_description (self->color, cs);

  if (desc)
    xx_color_management_surface_v2_set_image_description (self->surface,
                                                          desc,
                                                          XX_COLOR_MANAGER_V2_RENDER_INTENT_PERCEPTUAL);
  else
    xx_color_management_surface_v2_unset_image_description (self->surface);
}

gboolean
gdk_wayland_color_surface_can_set_color_state (GdkWaylandColorSurface *self,
                                               GdkColorState          *cs)
{
  return gdk_wayland_color_get_image_description (self->color, cs) != NULL;
}