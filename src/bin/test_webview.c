#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH

void
test_webkit(void *data, Evas_Object *obj, void *event_info)
{
#ifdef ELM_EWEBKIT
   Evas_Object *win, *bg, *wv, *ic;

   g_type_init();
   g_thread_init(0);

   printf("#### test_webkit\n");
   win = elm_win_add(NULL, "box-horiz", ELM_WIN_BASIC);
   elm_win_title_set(win, "Box Horiz");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   wv = elm_webview_add(win);
   elm_win_resize_object_add(win, wv);
   evas_object_size_hint_weight_set(wv, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(wv);

   evas_object_resize(win, 320, 300);

   elm_object_focus(win);
   evas_object_show(win);
#endif
}
#endif
