#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH

static void
_response_cb(void *data, Evas_Object *obj __UNUSED__,
             void *event_info __UNUSED__)
{
   evas_object_hide(data);
   evas_object_del(data);
}

static void
_block_clicked_cb(void *data __UNUSED__, Evas_Object *obj,
                  void *event_info __UNUSED__)
{
   printf("\nblock,clicked callback\n");
   evas_object_del(obj);
}

static void
_item_selected_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
                  void *event_info)
{
   printf("popup item selected: %s\n", elm_object_item_text_get(event_info));
}

static void
_list_click(void *data __UNUSED__, Evas_Object *obj,
            void *event_info __UNUSED__)
{
   Elm_Object_Item *it = elm_list_selected_item_get(obj);
   if (!it) return;
   elm_list_item_selected_set(it, EINA_FALSE);
}

static void
_popup_center_text_cb(void *data, Evas_Object *obj __UNUSED__,
                      void *event_info __UNUSED__)
{
   Evas_Object *popup;

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set(popup, "This Popup has content area and "
                       "timeout value is 3 seconds");
   elm_popup_timeout_set(popup, 3.0);
   evas_object_smart_callback_add(popup, "timeout", _response_cb, popup);
   evas_object_show(popup);
}

static void
_popup_center_text_1button_cb(void *data, Evas_Object *obj __UNUSED__,
                              void *event_info __UNUSED__)
{
   Evas_Object *popup;
   Evas_Object *btn;

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set(popup, "This Popup has content area and "
                       "action area set, action area has one button Close");
   btn = elm_button_add(popup);
   elm_object_text_set(btn, "Close");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", _response_cb, popup);
   evas_object_show(popup);
}

static void
_popup_center_title_text_1button_cb(void *data, Evas_Object *obj __UNUSED__,
                                    void *event_info __UNUSED__)
{
   Evas_Object *popup;
   Evas_Object *btn;

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set(popup, "This Popup has title area, content area and "
                       "action area set, action area has one button Close");
   elm_object_part_text_set(popup, "title,text", "Title");
   btn = elm_button_add(popup);
   elm_object_text_set(btn, "Close");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", _response_cb, popup);
   evas_object_show(popup);
}

static void
_popup_center_title_text_block_clicked_event_cb(void *data,
                                                Evas_Object *obj __UNUSED__,
                                                void *event_info __UNUSED__)
{
   Evas_Object *popup;

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set(popup, "This Popup has title area and content area. "
                       "When clicked on blocked event region, popup gets "
                       "deleted");
   elm_object_part_text_set(popup, "title,text", "Title");
   evas_object_smart_callback_add(popup, "block,clicked", _block_clicked_cb,
                                  NULL);
   evas_object_show(popup);
}

static void
_popup_bottom_title_text_3button_cb(void *data, Evas_Object *obj __UNUSED__,
                                    void *event_info __UNUSED__)
{
   Evas_Object *popup;
   Evas_Object *icon, *btn1, *btn2, *btn3;
   char buf[256];

   popup = elm_popup_add(data);
   elm_popup_orient_set(popup, ELM_POPUP_ORIENT_BOTTOM);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set(popup, "This Popup has title area, content area and "
                       "action area set with content being character wrapped. "
                       "action area has three buttons OK, Cancel and Close");
   elm_popup_content_text_wrap_type_set(popup, ELM_WRAP_CHAR);
   elm_object_part_text_set(popup, "title,text", "Title");
   icon = elm_icon_add(popup);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_icon_file_set(icon, buf, NULL);
   elm_object_part_content_set(popup, "title,icon", icon);
   btn1 = elm_button_add(popup);
   elm_object_text_set(btn1, "OK");
   elm_object_part_content_set(popup, "button1", btn1);
   btn2 = elm_button_add(popup);
   elm_object_text_set(btn2, "Cancel");
   elm_object_part_content_set(popup, "button2", btn2);
   btn3 = elm_button_add(popup);
   elm_object_text_set(btn3, "Close");
   elm_object_part_content_set(popup, "button3", btn3);
   evas_object_smart_callback_add(btn1, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn2, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn3, "clicked", _response_cb, popup);
   evas_object_show(popup);
}

static void
_popup_center_title_content_3button_cb(void *data, Evas_Object *obj __UNUSED__,
                                       void *event_info __UNUSED__)
{
   Evas_Object *popup;
   Evas_Object *icon, *btn, *btn1, *btn2, *btn3;
   char buf[256];

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   btn = elm_button_add(popup);
   elm_object_text_set(btn, "Content");
   icon = elm_icon_add(btn);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_icon_file_set(icon, buf, NULL);
   elm_object_content_set(btn, icon);
   elm_object_content_set(popup, btn);
   elm_object_part_text_set(popup, "title,text", "Title");
   btn1 = elm_button_add(popup);
   elm_object_text_set(btn1, "OK");
   elm_object_part_content_set(popup, "button1", btn1);
   btn2 = elm_button_add(popup);
   elm_object_text_set(btn2, "Cancel");
   elm_object_part_content_set(popup, "button2", btn2);
   btn3 = elm_button_add(popup);
   elm_object_text_set(btn3, "Close");
   elm_object_part_content_set(popup, "button3", btn3);
   evas_object_smart_callback_add(btn1, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn2, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn3, "clicked", _response_cb, popup);
   evas_object_show(popup);
}

static void
_popup_center_title_item_3button_cb(void *data, Evas_Object *obj __UNUSED__,
                                    void *event_info __UNUSED__)
{
   char buf[256];
   unsigned int i;
   Evas_Object *popup, *icon1, *btn1, *btn2, *btn3;

   popup = elm_popup_add(data);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   icon1 = elm_icon_add(popup);
   elm_object_part_text_set(popup, "title,text", "Title");
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_icon_file_set(icon1, buf, NULL);
   for (i = 0; i < 10; i++)
     {
        snprintf(buf, sizeof(buf), "Item%u", i+1);
        if (i == 3)
          elm_popup_item_append(popup, buf, icon1, _item_selected_cb, NULL);
        else
          elm_popup_item_append(popup, buf, NULL, _item_selected_cb, NULL);
     }
   btn1 = elm_button_add(popup);
   elm_object_text_set(btn1, "OK");
   elm_object_part_content_set(popup, "button1", btn1);
   btn2 = elm_button_add(popup);
   elm_object_text_set(btn2, "Cancel");
   elm_object_part_content_set(popup, "button2", btn2);
   btn3 = elm_button_add(popup);
   elm_object_text_set(btn3, "Close");
   elm_object_part_content_set(popup, "button3", btn3);
   evas_object_smart_callback_add(btn1, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn2, "clicked", _response_cb, popup);
   evas_object_smart_callback_add(btn3, "clicked", _response_cb, popup);
   evas_object_show(popup);
}

void
test_popup(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   Evas_Object *win, *bg, *list;

   win = elm_win_add(NULL, "popup", ELM_WIN_BASIC);
   elm_win_title_set(win, "popup");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   list = elm_list_add(win);
   elm_win_resize_object_add(win, list);
   elm_list_mode_set(list, ELM_LIST_LIMIT);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(list, "selected", _list_click, NULL);

   elm_list_item_append(list, "popup-center-text", NULL, NULL,
                        _popup_center_text_cb, win);
   elm_list_item_append(list, "popup-center-text + 1 button", NULL, NULL,
                        _popup_center_text_1button_cb, win);
   elm_list_item_append(list, "popup-center-title + text + 1 button", NULL,
                        NULL, _popup_center_title_text_1button_cb, win);

   elm_list_item_append(list,
                        "popup-center-title + text (block,clicked handling)",
                        NULL, NULL,
                        _popup_center_title_text_block_clicked_event_cb, win);
   elm_list_item_append(list, "popup-bottom-title + text + 3 buttons", NULL,
                        NULL, _popup_bottom_title_text_3button_cb, win);
   elm_list_item_append(list, "popup-center-title + content + 3 buttons", NULL,
                        NULL, _popup_center_title_content_3button_cb, win);
   elm_list_item_append(list, "popup-center-title + items + 3 buttons", NULL,
                        NULL, _popup_center_title_item_3button_cb, win);
   elm_list_go(list);
   evas_object_show(list);
   evas_object_show(win);
   evas_object_resize(win, 480, 800);
}

#endif

