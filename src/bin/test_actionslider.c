#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH

static void _pos_selected_cb(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   printf("Selection: %s\n", (char *)event_info);
   printf("Label selected: %s\n", elm_actionslider_selected_label_get(obj));
}

static void
_position_change_magnetic_cb(void *data __UNUSED__, Evas_Object * obj, void *event_info)
{
   if (!strcmp((char *)event_info, "left"))
     elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_MAGNET_LEFT);
   else if (!strcmp((char *)event_info, "right"))
     elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_MAGNET_RIGHT);
}

static void
_magnet_enable_disable_cb(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   if (!strcmp((char *)event_info, "left"))
      elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_MAGNET_CENTER);
   else if (!strcmp((char *)event_info, "right"))
      elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_MAGNET_NONE);
}

void
test_actionslider(void *data __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *win, *bg, *bx, *as;

   win = elm_win_add(NULL, "actionslider", ELM_WIN_BASIC);
   elm_win_title_set(win, "Actionslider");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_RIGHT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_RIGHT);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,"Snooze");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,NULL);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Stop");
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_MAGNET_LEFT |
                                    ELM_ACTIONSLIDER_MAGNET_RIGHT);
   evas_object_smart_callback_add(as, "pos_changed",
                                  _position_change_magnetic_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_CENTER);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_CENTER);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,"Snooze");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,NULL);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Stop");
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_MAGNET_LEFT |
                                    ELM_ACTIONSLIDER_MAGNET_RIGHT);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   elm_object_style_set(as, "bar");
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL,EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_CENTER|
                                   ELM_ACTIONSLIDER_MAGNET_RIGHT);
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_MAGNET_CENTER |
                                    ELM_ACTIONSLIDER_MAGNET_RIGHT);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,NULL);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,"Accept");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Reject");
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   elm_object_style_set(as, "bar");
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_LEFT);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,NULL);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,"Accept");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Reject");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_BUTTON,"Go");
   evas_object_smart_callback_add(as, "pos_changed",
                                  _position_change_magnetic_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);


   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_ALL);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,"Left");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,"Center");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Right");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_BUTTON,"Go");
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_INDICATOR_CENTER);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_MAGNET_CENTER);
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_LEFT,"Enable");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_CENTER,"Magnet");
   elm_actionslider_label_set(as, ELM_ACTIONSLIDER_LABEL_RIGHT,"Disable");
   evas_object_smart_callback_add(as, "pos_changed",
                                  _magnet_enable_disable_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   evas_object_resize(win, 240, 240);
   evas_object_show(win);
}
#endif
