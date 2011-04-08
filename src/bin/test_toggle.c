#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH
void
test_toggle(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *win, *bg, *bx, *ic, *tg;
   char buf[PATH_MAX];

   win = elm_win_add(NULL, "toggles", ELM_WIN_BASIC);
   elm_win_title_set(win, "Toggles");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

   tg = elm_toggle_add(win);
   evas_object_size_hint_weight_set(tg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tg, EVAS_HINT_FILL, 0.5);
   elm_toggle_label_set(tg, "Icon sized to toggle");
   elm_toggle_icon_set(tg, ic);
   elm_toggle_state_set(tg, 1);
   elm_toggle_states_labels_set(tg, "Yes", "No");
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   elm_icon_scale_set(ic, 0, 0);

   tg = elm_toggle_add(win);
   elm_toggle_label_set(tg, "Icon no scale");
   elm_toggle_icon_set(tg, ic);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   elm_icon_scale_set(ic, 0, 0);

   tg = elm_toggle_add(win);
   elm_toggle_label_set(tg, "Icon no scale");
   elm_toggle_icon_set(tg, ic);
   elm_object_disabled_set(tg, 1);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   tg = elm_toggle_add(win);
   elm_toggle_label_set(tg, "Label Only");
   elm_toggle_states_labels_set(tg, "Big long fun times label",
                                "Small long happy fun label");
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   elm_icon_scale_set(ic, 0, 0);

   tg = elm_toggle_add(win);
   elm_toggle_icon_set(tg, ic);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   evas_object_show(win);
}
#endif
