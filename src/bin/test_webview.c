#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH

static void
click_through(void *data, Evas_Object *obj, void *event_info)
{
   printf("click went through on %p\n", obj);
}

void
test_webview_in_scroller(void *data, Evas_Object *obj, void *event_info)
{
#ifdef ELM_EWEBKIT
   Evas_Object *win, *bg, *sc, *bx, *bt, *wv;
   printf("#### test_webview2\n");
   int i;

   win = elm_win_add(NULL, "webview2", ELM_WIN_BASIC);
   elm_win_title_set(win, "webview in scroller");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   sc = elm_scroller_add(win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
   elm_scroller_bounce_set(sc, 1, 0);
   elm_win_resize_object_add(win, sc);
   evas_object_show(sc);


   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.0);
   elm_scroller_content_set(sc, bx);
   evas_object_show(bx);

   for (i = 0; i < 10; i++)
     {
        bt = elm_button_add(win);
        elm_button_label_set(bt, "Vertical");
        evas_object_smart_callback_add(bt, "clicked", click_through, NULL);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
     }

   //create webview
   wv = elm_webview_add(win, EINA_TRUE);
   evas_object_size_hint_weight_set(wv, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_min_set(wv, 480, 400);
   elm_box_pack_end(bx, wv);
   evas_object_show(wv);
   elm_webview_uri_set(wv, "http://www.google.com");

   for (i = 0; i < 10; i++)
     {
        bt = elm_button_add(win);
        elm_button_label_set(bt, "Vertical");
        evas_object_smart_callback_add(bt, "clicked", click_through, NULL);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
     }

   evas_object_resize(win, 480, 300);
   elm_object_focus(win);
   evas_object_show(win);
#endif
}

void
test_webview(void *data, Evas_Object *obj, void *event_info)
{
#ifdef ELM_EWEBKIT
   Evas_Object *win, *bg, *wv;

   win = elm_win_add(NULL, "webview", ELM_WIN_BASIC);
   elm_win_title_set(win, "webview");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   wv = elm_webview_add(win, EINA_TRUE);
   evas_object_size_hint_weight_set(wv, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, wv);
   elm_webview_uri_set(wv, "http://www.google.com");
   evas_object_show(wv);

   evas_object_resize(win, 320, 300);
   elm_object_focus(win);
   evas_object_show(win);
#endif
}
#endif
