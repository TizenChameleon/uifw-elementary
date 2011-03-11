#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH

static Evas_Object* btn1 = NULL;
static Evas_Object* btn2 = NULL;
static Evas_Object* btn3 = NULL;


static void
_cb1( void* data, Evas_Object* obj, void* event_info)
{
   Elm_Segment_Item *it1 = elm_segment_control_item_selected_get( data );
   elm_segment_control_item_del(it1);
   return;
}


static void
_cb3( void* data, Evas_Object* obj, void* event_info)
{
   Elm_Segment_Item *it1;
   char buf[PATH_MAX];
   Evas_Object *ic1 = elm_icon_add(obj);
   snprintf(buf, sizeof(buf), "%s/images/icon_00.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic1, buf, NULL);
   evas_object_size_hint_aspect_set(ic1, EVAS_ASPECT_CONTROL_BOTH, 1, 1);


   it1 = elm_segment_control_item_insert_at(data, ic1, "Inserted Item", 0);
   elm_segment_control_item_label_set(it1, "Inserted Item");
   return;
}

static void
_cb2( void* data, Evas_Object* obj, void* event_info)
{
   Elm_Segment_Item *it1;
   char buf[PATH_MAX];
   Evas_Object *ic1;

   it1 = elm_segment_control_item_add(data, NULL, "Added Item");

   ic1 = elm_icon_add(obj);
   snprintf(buf, sizeof(buf), "%s/images/logo.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic1, buf, NULL);
   evas_object_size_hint_aspect_set(ic1, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
   elm_segment_control_item_icon_set(it1, ic1);

   return;
}

void
test_segment_control(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *win, *bg, *bx, *ic, *ic1;
   Elm_Segment_Item *it1, *it2, *it3;


   Evas_Object * in_layout;
   Evas_Object *segment;
   char buf[PATH_MAX];
   char buf1[PATH_MAX];
   char buf2[PATH_MAX];

   win = elm_win_add(NULL, "segmentcontrol", ELM_WIN_BASIC);
   elm_win_title_set(win, "Segment Control");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   in_layout = elm_layout_add( win );
   snprintf(buf, sizeof(buf), "%s/objects/test.edj", PACKAGE_DATA_DIR);
   elm_layout_file_set(in_layout, buf, "segment_test");
   evas_object_size_hint_weight_set(in_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   ic = elm_icon_add(in_layout);
   snprintf(buf1, sizeof(buf1), "%s/images/logo.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf1, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_BOTH, 1, 1);

   ic1 = elm_icon_add(in_layout);
   snprintf(buf2, sizeof(buf2), "%s/images/icon_00.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic1, buf2, NULL);
   evas_object_size_hint_aspect_set(ic1, EVAS_ASPECT_CONTROL_BOTH, 1, 1);

   segment = elm_segment_control_add(win);

   it1 = elm_segment_control_item_add(segment, NULL, "Only Text");
   it2 = elm_segment_control_item_add(segment, ic, NULL);
   elm_segment_control_item_selected_set(it2, EINA_TRUE);
   it3 = elm_segment_control_item_add(segment, ic1, "Text_Icon_test");

   elm_object_disabled_set(segment, EINA_TRUE);
   btn1 = elm_button_add(win);

   elm_button_label_set(btn1, "Delete");
   evas_object_show(btn1);

   btn2 = elm_button_add(win);
   elm_button_label_set(btn2, "Add");
   evas_object_show(btn2);

   btn3 = elm_button_add(win);
   elm_button_label_set(btn3, "Insert");
   evas_object_show(btn3);

   evas_object_smart_callback_add( btn1, "clicked", _cb1, segment);
   evas_object_smart_callback_add( btn2, "clicked", _cb2, segment);
   evas_object_smart_callback_add( btn3, "clicked", _cb3, segment);

   evas_object_show(segment);
   elm_layout_content_set(in_layout, "segment", segment);
   elm_layout_content_set(in_layout, "add", btn2);
   elm_layout_content_set(in_layout, "del", btn1);
   elm_layout_content_set(in_layout, "insert", btn3);

   elm_box_pack_end(bx, in_layout);
   evas_object_show(in_layout);

   evas_object_show(win);
}
#endif

