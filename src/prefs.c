/*
 * Copyright (c) 2017 Jean Guyomarc'h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "eovim/prefs.h"
#include "eovim/config.h"
#include "eovim/nvim.h"
#include "eovim/log.h"
#include "eovim/gui.h"
#include "contrib/contrib.h"
#include <Elementary.h>

typedef struct
{
   Eina_Stringshare *name;
   Eina_Stringshare *fancy;
} s_font;

static const double _font_min = 4.0;
static const double _font_max = 72.0;
static Elm_Genlist_Item_Class *_font_itc = NULL;
static const char *const _nvim_data_key = "nvim";

/* This example test comples from the EFL (terminology). It allows to easily
 * spot the differences between similar characters */
static const char *const _text_example = "oislOIS.015!|,";

static inline s_nvim *
_nvim_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, _nvim_data_key);
}

static Evas_Object *
_frame_add(Evas_Object *parent,
           const char *text)
{
   Evas_Object *f;

   f = elm_frame_add(parent);
   elm_frame_autocollapse_set(f, EINA_FALSE);
   elm_object_text_set(f, text);
   evas_object_size_hint_weight_set(f, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(f, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(f);

   return f;
}


/*============================================================================*
 *                            Bell Config Handling                            *
 *============================================================================*/

static void
_bell_mute_changed_cb(void *data,
                      Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   s_gui *const gui = data;
   const Eina_Bool mute = elm_check_state_get(obj);
   config_bell_mute_set(gui->nvim->config, mute);
}

static Evas_Object *
_config_bell_add(s_gui *gui,
                 Evas_Object *parent)
{
   const s_config *const config = gui->nvim->config;

   /* Frame container */
   Evas_Object *const f = _frame_add(parent, "Bell Settings");

   /* Settings box */
   Evas_Object *const box = elm_box_add(f);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);
   evas_object_show(box);

   /* Bell mute checkbox */
   Evas_Object *const mute = elm_check_add(box);
   evas_object_size_hint_align_set(mute, 0.0, 0.0);
   elm_object_text_set(mute, "Mute the audible bell");
   elm_check_state_set(mute, config->mute_bell);
   evas_object_smart_callback_add(mute, "changed", _bell_mute_changed_cb, gui);
   evas_object_show(mute);
   elm_box_pack_end(box, mute);

   elm_object_content_set(f, box);
   return f;
}


/*============================================================================*
 *                             Font Size Handling                             *
 *============================================================================*/

static void
_font_size_cb(void *data,
              Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   s_gui *const gui = data;
   s_config *const config = gui->nvim->config;
   const unsigned int size = (unsigned int)elm_slider_value_get(obj);

   config_font_size_set(config, size);
   termview_font_set(gui->termview, config->font_name, size);
   gui_size_recalculate(gui);
}

static Evas_Object *
_config_font_size_add(s_gui *gui,
                      Evas_Object *parent)
{
   const s_config *const config = gui->nvim->config;

   /* Frame container */
   Evas_Object *const f = _frame_add(parent, "Font Size Settings");

   /* Slider */
   Evas_Object *const sl = elm_slider_add(f);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_slider_span_size_set(sl, 40);
   elm_slider_unit_format_set(sl, "%1.0f");
   elm_slider_indicator_format_set(sl, "%1.0f");
   elm_slider_min_max_set(sl, _font_min, _font_max);
   elm_slider_value_set(sl, config->font_size);
   evas_object_smart_callback_add(sl, "delay,changed", _font_size_cb, gui);
   evas_object_show(sl);

   elm_object_content_set(f, sl);
   return f;
}

/*============================================================================*
 *                             Font Name Handling                             *
 *============================================================================*/

static int
_font_sort_cb(const void *a, const void *b)
{
   return strcasecmp(a, b);
}

static void
_font_item_del(void *data,
               Evas_Object *obj EINA_UNUSED)
{
   s_font *const font = data;

   eina_stringshare_del(font->fancy);
   eina_stringshare_del(font->name);
   free(font);
}

static char *
_font_text_get(void *data,
               Evas_Object *obj EINA_UNUSED,
               const char *part EINA_UNUSED)
{
   const s_font *const font = data;
   return strndup(font->fancy, (size_t)eina_stringshare_strlen(font->fancy));
}

static Evas_Object *
_font_content_get(void *data,
                  Evas_Object *obj,
                  const char *part)
{
   const s_font *const font = data;
   if (strcmp(part, "elm.swallow.end") != 0) { return NULL; }

   const s_nvim *const nvim = _nvim_get(obj);
   char buf[1024];
   Evas_Object *const label = elm_label_add(obj);

   snprintf(buf, sizeof(buf), "<font='%s' font_size=%u align=right>%s<tab>",
            font->name, nvim->config->font_size, _text_example);
   elm_object_text_set(label, buf);
   return label;
}

static void
_font_sel_cb(void *data,
             Evas_Object *obj EINA_UNUSED,
             void *event)
{
   s_gui *const gui = data;
   s_config *const config = gui->nvim->config;
   const Elm_Genlist_Item *const item = event;
   const s_font *const font = elm_object_item_data_get(item);

   /*
    * Write the font name in the config and change the font of the termview.
    */
   config_font_name_set(config, font->name);
   termview_font_set(gui->termview, config->font_name, config->font_size);
   gui_size_recalculate(gui);
}

static Evas_Object *
_config_font_name_add(s_gui *gui,
                      Evas_Object *parent)
{
   const s_config *const config = gui->nvim->config;
   Elm_Genlist_Item *sel_item = NULL;

   /* Frame container */
   Evas_Object *const f = _frame_add(parent, "Font Name Settings");
   evas_object_size_hint_weight_set(f, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   /* Fonts list */
   Evas_Object *const gl = elm_genlist_add(f);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_data_set(gl, _nvim_data_key, gui->nvim);
   elm_object_content_set(f, gl);
   evas_object_show(gl);

   /* Get a sorted list of all available fonts */
   Evas *const evas = evas_object_evas_get(parent);
   Eina_List *fonts = evas_font_available_list(evas);
   fonts = eina_list_sort(fonts, eina_list_count(fonts), _font_sort_cb);

   /* Add each font to the list of possible fonts */
   Eina_List *l;
   const char *fname;
   EINA_LIST_FOREACH(fonts, l, fname)
     {
        s_font *const font = malloc(sizeof(s_font));
        if (EINA_UNLIKELY(! font))
          {
             CRI("Failed to allocate memory");
             goto fail;
          }
        const int ret = contrib_parse_font_name(fname, &font->name, &font->fancy);
        if (EINA_UNLIKELY(ret < 0))
          {
             WRN("Failed to parse font '%s'", fname);
             free(font);
             continue;
          }

        Elm_Genlist_Item *const item = elm_genlist_item_append(
           gl, _font_itc, font, NULL, ELM_GENLIST_ITEM_NONE, _font_sel_cb, gui
        );

        /* Keep track of the item that has the config's font selection */
        if (font->name == config->font_name)
          sel_item = item;
     }

   /* Select in the genlist the currently used font */
   if (EINA_LIKELY(sel_item != NULL))
     {
        elm_genlist_item_selected_set(sel_item, EINA_TRUE);
        elm_genlist_item_bring_in(sel_item, ELM_GENLIST_ITEM_SCROLLTO_IN);
     }

   evas_font_available_list_free(evas, fonts);

   return f;

fail:
   evas_font_available_list_free(evas, fonts);
   evas_object_del(f);
   return NULL;
}


static void
_page_change_cb(void *data EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *info)
{
   /* Promote the naviframe page item, that is stored as a data of the
    * item selected in the segmentcontrol */

   const Elm_Object_Item *const sec_item = info;
   Elm_Object_Item *const item = elm_object_item_data_get(sec_item);

   elm_naviframe_item_promote(item);
}

static Evas_Object *
_prefs_box_new(Evas_Object *parent)
{
   Evas_Object *const box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_align_set(box, 0.5, 0.0);
   evas_object_show(box);
   return box;
}

static Evas_Object *
_font_prefs_new(s_gui *gui)
{
   Evas_Object *const box = _prefs_box_new(gui->prefs.nav);

   Evas_Object *const fsize = _config_font_size_add(gui, box);
   elm_box_pack_end(box, fsize);

   Evas_Object *const fname = _config_font_name_add(gui, box);
   elm_box_pack_end(box, fname);
   return box;
}

static Evas_Object *
_theme_prefs_new(s_gui *gui)
{
   Evas_Object *const box = _prefs_box_new(gui->prefs.nav);
   Evas_Object *const bell = _config_bell_add(gui, box);
   elm_box_pack_end(box, bell);
   return box;
}

static Elm_Object_Item *
_push_nav_item(s_gui *gui, Evas_Object *contents)
{
   Elm_Object_Item *const p = elm_naviframe_item_push(
      gui->prefs.nav, NULL, NULL, NULL, contents, NULL);
   elm_naviframe_item_title_enabled_set(p, EINA_FALSE, EINA_FALSE);
   return p;
}

void
prefs_show(s_gui *gui)
{
   if (gui->prefs.box) { return; }

   /* Create the main box that holds the prefs together */
   Evas_Object *const box = _prefs_box_new(gui->layout);
   gui->prefs.box = box;

   /* Add a segment control that keeps track of all the pages */
   Evas_Object *const sec = elm_segment_control_add(box);
   evas_object_size_hint_align_set(sec, 0.0, EVAS_HINT_FILL);
   evas_object_smart_callback_add(sec, "changed", _page_change_cb, gui);
   evas_object_show(sec);
   elm_box_pack_start(box, sec);

   /* Naviframe: the widget that will manage prefs pages */
   Evas_Object *const nav = elm_naviframe_add(box);
   evas_object_size_hint_weight_set(nav, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nav, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(nav);
   elm_box_pack_end(box, nav);
   gui->prefs.nav = nav;

   /* Create the pages */
   Elm_Object_Item *const p1 = _push_nav_item(gui, _theme_prefs_new(gui));
   Elm_Object_Item *const p2 = _push_nav_item(gui, _font_prefs_new(gui));

   /* Associate one item data in the segment control to each page */
   Elm_Object_Item *it, *it_init;
   it_init = elm_segment_control_item_add(sec, NULL, "Theme");
   elm_object_item_data_set(it_init, p1);
   it = elm_segment_control_item_add(sec, NULL, "Font");
   elm_object_item_data_set(it, p2);

   /* We select theme by default */
   elm_segment_control_item_selected_set(it_init, EINA_TRUE);

   /* Update edje and fire up the popup */
   elm_layout_content_set(gui->layout, "eovim.config.box", box);
   elm_layout_signal_emit(gui->layout, "config,show", "eovim");
   evas_object_focus_set(gui->termview, EINA_FALSE);
}

void
prefs_hide(s_gui *gui)
{
   elm_layout_signal_emit(gui->layout, "config,hide", "eovim");
   elm_layout_content_unset(gui->layout, "eovim.config.box");
   evas_object_del(gui->prefs.box);
   gui->prefs.box = NULL;
}

Eina_Bool
prefs_init(void)
{
   /* Font list item class */
   _font_itc = elm_genlist_item_class_new();
   if (EINA_UNLIKELY(! _font_itc))
     {
        CRI("Failed to create genlist item class");
        return EINA_FALSE;
     }
   _font_itc->item_style = "default";
   _font_itc->func.text_get = _font_text_get;
   _font_itc->func.content_get = _font_content_get;
   _font_itc->func.del = _font_item_del;

   return EINA_TRUE;
}

void
prefs_shutdown(void)
{
   elm_genlist_item_class_free(_font_itc);
}