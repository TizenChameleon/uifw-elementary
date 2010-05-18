#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH

#define IND_NUM 20

static Evas_Object *indicator[IND_NUM];

static void
_mouse_down(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
//   Evas_Object *win = data;
   
   if (ev->button != 1) return;
   printf("MOUSE: down @ %4i %4i\n", ev->canvas.x, ev->canvas.y);
   evas_object_move(indicator[0], ev->canvas.x, ev->canvas.y);
   evas_object_resize(indicator[0], 1, 1);
   evas_object_show(indicator[0]);
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
//   Evas_Object *win = data;
   if (ev->button != 1) return;
   printf("MOUSE: up   @ %4i %4i\n", ev->canvas.x, ev->canvas.y);
   evas_object_hide(indicator[0]);
}

static void
_mouse_move(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
//   Evas_Object *win = data;
   printf("MOUSE: move @ %4i %4i\n", ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_move(indicator[0], ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_resize(indicator[0], 1, 1);
}




static void
_multi_down(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Multi_Down *ev = event_info;
//   Evas_Object *win = data;
   printf("MULTI: down @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_move(indicator[ev->device], ev->canvas.x, ev->canvas.y);
   evas_object_resize(indicator[ev->device], 1, 1);
   evas_object_show(indicator[ev->device]);
}

static void
_multi_up(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Multi_Up *ev = event_info;
//   Evas_Object *win = data;
   printf("MULTI: up    @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_hide(indicator[ev->device]);
}

static void
_multi_move(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Multi_Move *ev = event_info;
//   Evas_Object *win = data;
   printf("MULTI: move @ %4i %4i | dev: %i\n", ev->cur.canvas.x, ev->cur.canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_move(indicator[ev->device], ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_resize(indicator[ev->device], 1, 1);
}




void
test_multi(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *r;
   int i;

   win = elm_win_add(NULL, "bg-plain", ELM_WIN_BASIC);
   elm_win_title_set(win, "Bg Plain");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(r, 0, 0, 0, 0);
   elm_win_resize_object_add(win, r);
   evas_object_show(r);
   
   for (i = 0; i < IND_NUM; i++)
     {
        char buf[PATH_MAX];
        
        snprintf(buf, sizeof(buf), "%s/objects/multip.edj", PACKAGE_DATA_DIR);
        indicator[i] = edje_object_add(evas_object_evas_get(win));
        edje_object_file_set(indicator[i], buf, "point");
     }
   
   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_UP, _mouse_up, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_DOWN, _multi_down, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_UP, _multi_up, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_MOVE, _multi_move, win);

   evas_object_size_hint_min_set(bg, 160, 160);
   evas_object_resize(win, 480, 800);
   
   evas_object_show(win);
}

#endif
