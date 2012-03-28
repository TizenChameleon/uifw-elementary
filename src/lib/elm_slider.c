#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *slider;
   Evas_Object *icon;
   Evas_Object *end;
   Evas_Object *spacer;

   Ecore_Timer *delay;

   Eina_Hash  *labels;
   const char *units;
   const char *indicator;

   char *(*indicator_format_func)(double val);
   void (*indicator_format_free)(char *str);

   char *(*units_format_func)(double val);
   void (*units_format_free)(char *str);

   double val, val_min, val_max, val2;
   Evas_Coord size;
   Evas_Coord downx, downy;

   Eina_Bool horizontal : 1;
   Eina_Bool inverted : 1;
   Eina_Bool indicator_show : 1;
   Eina_Bool spacer_down : 1;
   Eina_Bool frozen : 1;
};

#define ELM_SLIDER_INVERTED_FACTOR (-1.0)

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _units_set(Evas_Object *obj);
static void _val_set(Evas_Object *obj);
static void _indicator_set(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _drag_up(void *data, Evas_Object *obj,
                    const char *emission, const char *source);
static void _drag_down(void *data, Evas_Object *obj,
                    const char *emission, const char *source);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);
static void _spacer_down_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);
static void _spacer_move_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);
static void _spacer_up_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);

static const char SIG_CHANGED[] = "changed";
static const char SIG_DELAY_CHANGED[] = "delay,changed";
static const char SIG_DRAG_START[] = "slider,drag,start";
static const char SIG_DRAG_STOP[] = "slider,drag,stop";
static const Evas_Smart_Cb_Description _signals[] = {
  {SIG_CHANGED, ""},
  {SIG_DELAY_CHANGED, ""},
  {SIG_DRAG_START, ""},
  {SIG_DRAG_STOP, ""},
  {NULL, NULL}
};

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Mouse_Wheel *mev;
   Evas_Event_Key_Down *ev;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (type == EVAS_CALLBACK_KEY_DOWN) goto key_down;
   else if (type != EVAS_CALLBACK_MOUSE_WHEEL) return EINA_FALSE;

   mev = event_info;
   if (mev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   if (mev->z < 0) _drag_up(obj, NULL, NULL, NULL);
   else _drag_down(obj, NULL, NULL, NULL);
   mev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;

  key_down:
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if ((!strcmp(ev->keyname, "Left"))
       || (!strcmp(ev->keyname, "KP_Left")))
     {
        if (!wd->horizontal) return EINA_FALSE;
        if (!wd->inverted) _drag_down(obj, NULL, NULL, NULL);
        else _drag_up(obj, NULL, NULL, NULL);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Right"))
            || (!strcmp(ev->keyname, "KP_Right")))
     {
        if (!wd->horizontal) return EINA_FALSE;
        if (!wd->inverted) _drag_up(obj, NULL, NULL, NULL);
        else _drag_down(obj, NULL, NULL, NULL);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")))
     {
        if (wd->horizontal) return EINA_FALSE;
        if (wd->inverted) _drag_up(obj, NULL, NULL, NULL);
        else _drag_down(obj, NULL, NULL, NULL);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
        if (wd->horizontal) return EINA_FALSE;
        if (wd->inverted) _drag_down(obj, NULL, NULL, NULL);
        else _drag_up(obj, NULL, NULL, NULL);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else return EINA_FALSE;
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->labels) eina_hash_free(wd->labels);
   if (wd->indicator) eina_stringshare_del(wd->units);
   if (wd->delay) ecore_timer_del(wd->delay);
   free(wd);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->slider, "elm,action,focus", "elm");
        evas_object_focus_set(wd->slider, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(wd->slider, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->slider, EINA_FALSE);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_mirrored_set(wd->slider, rtl);
}

static Eina_Bool
_labels_foreach_text_set(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
  Widget_Data *wd = fdata;

  edje_object_part_text_set(wd->slider, key, data);

  return 1;
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   if (wd->horizontal)
     _elm_theme_object_set(obj, wd->slider, "slider", "horizontal", elm_widget_style_get(obj));
   else
     _elm_theme_object_set(obj, wd->slider, "slider", "vertical", elm_widget_style_get(obj));
   if (wd->icon)
     {
        edje_object_part_swallow(wd->slider, "elm.swallow.content", wd->icon);
        edje_object_signal_emit(wd->slider, "elm,state,icon,visible", "elm");
     }
   if (wd->end)
     edje_object_signal_emit(wd->slider, "elm,state,end,visible", "elm");
   else
     edje_object_signal_emit(wd->slider, "elm,state,end,hidden", "elm");
   if (wd->labels)
     {
        eina_hash_foreach(wd->labels, _labels_foreach_text_set, wd);
	edje_object_signal_emit(wd->slider, "elm,state,text,visible", "elm");
     }

   if (wd->units)
     edje_object_signal_emit(wd->slider, "elm,state,units,visible", "elm");

   if (wd->horizontal)
     evas_object_size_hint_min_set(wd->spacer, (double)wd->size * elm_widget_scale_get(obj) * _elm_config->scale, 1);
   else
     evas_object_size_hint_min_set(wd->spacer, 1, (double)wd->size * elm_widget_scale_get(obj) * _elm_config->scale);

   if (wd->inverted)
     edje_object_signal_emit(wd->slider, "elm,state,inverted,on", "elm");

   edje_object_part_swallow(wd->slider, "elm.swallow.bar", wd->spacer);
   _units_set(obj);
   _indicator_set(obj);
   edje_object_message_signal_process(wd->slider);
   edje_object_scale_set(wd->slider, elm_widget_scale_get(obj) * _elm_config->scale);
   _val_set(obj);
   _sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->slider, "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(wd->slider, "elm,state,enabled", "elm");
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->slider, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if ((obj != wd->icon) && (obj != wd->end)) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   if (sub == wd->icon)
     {
        edje_object_signal_emit(wd->slider, "elm,state,icon,hidden", "elm");
        evas_object_event_callback_del_full
           (sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
        wd->icon = NULL;
        edje_object_message_signal_process(wd->slider);
        _sizing_eval(obj);
     }
   if (sub == wd->end)
     {
        edje_object_signal_emit(wd->slider, "elm,state,end,hidden", "elm");
        evas_object_event_callback_del_full(sub,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
        wd->end = NULL;
        edje_object_message_signal_process(wd->slider);
        _sizing_eval(obj);
     }
}

static Eina_Bool
_delay_change(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->delay = NULL;
   evas_object_smart_callback_call(data, SIG_DELAY_CHANGED, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_val_fetch(Evas_Object *obj)
{
   Eina_Bool rtl;
   Widget_Data *wd = elm_widget_data_get(obj);
   double posx = 0.0, posy = 0.0, pos = 0.0, val;
   if (!wd) return;
   edje_object_part_drag_value_get(wd->slider, "elm.dragable.slider",
                                   &posx, &posy);
   if (wd->horizontal) pos = posx;
   else pos = posy;

   rtl = elm_widget_mirrored_get(obj);
   if ((!rtl && wd->inverted) || (rtl &&
                                  ((!wd->horizontal && wd->inverted) ||
                                   (wd->horizontal && !wd->inverted)))) pos = 1.0 - pos;
   val = (pos * (wd->val_max - wd->val_min)) + wd->val_min;
   if (val != wd->val)
     {
        wd->val = val;
        evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
        if (wd->delay) ecore_timer_del(wd->delay);
        wd->delay = ecore_timer_add(0.2, _delay_change, obj);
     }
}

static void
_val_set(Evas_Object *obj)
{
   Eina_Bool rtl;
   Widget_Data *wd = elm_widget_data_get(obj);
   double pos;
   if (!wd) return;
   if (wd->val_max > wd->val_min)
     pos = (wd->val - wd->val_min) / (wd->val_max - wd->val_min);
   else
     pos = 0.0;
   if (pos < 0.0) pos = 0.0;
   else if (pos > 1.0) pos = 1.0;

   rtl = elm_widget_mirrored_get(obj);
   if ((!rtl && wd->inverted) || (rtl &&
                                  ((!wd->horizontal && wd->inverted) ||
                                   (wd->horizontal && !wd->inverted)))) pos = 1.0 - pos;
   edje_object_part_drag_value_set(wd->slider, "elm.dragable.slider", pos, pos);
}

static void
_units_set(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->units_format_func)
     {
        char *buf;
        buf = wd->units_format_func(wd->val);
        edje_object_part_text_set(wd->slider, "elm.units", buf);
        if (wd->units_format_free) wd->units_format_free(buf);
     }
   else if (wd->units)
     {
        char buf[1024];

        snprintf(buf, sizeof(buf), wd->units, wd->val);
        edje_object_part_text_set(wd->slider, "elm.units", buf);
     }
   else
     edje_object_part_text_set(wd->slider, "elm.units", NULL);
}

static void
_indicator_set(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->indicator_format_func)
     {
        char *buf;
        buf = wd->indicator_format_func(wd->val);
        edje_object_part_text_set(wd->slider, "elm.dragable.slider:elm.indicator", buf);
        if (wd->indicator_format_free) wd->indicator_format_free(buf);
     }
   else if (wd->indicator)
     {
        char buf[1024];
        snprintf(buf, sizeof(buf), wd->indicator, wd->val);
        edje_object_part_text_set(wd->slider, "elm.dragable.slider:elm.indicator", buf);
     }
   else
     edje_object_part_text_set(wd->slider, "elm.dragable.slider:elm.indicator", NULL);
}

static void
_drag(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _val_fetch(data);
   _units_set(data);
   _indicator_set(data);
}

static void
_drag_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _val_fetch(data);
   evas_object_smart_callback_call(data, SIG_DRAG_START, NULL);
   _units_set(data);
   _indicator_set(data);
   elm_widget_scroll_freeze_push(data);
}

static void
_drag_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _val_fetch(data);
   evas_object_smart_callback_call(data, SIG_DRAG_STOP, NULL);
   _units_set(data);
   _indicator_set(data);
   elm_widget_scroll_freeze_pop(data);
}

static void
_drag_step(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _val_fetch(data);
   _units_set(data);
   _indicator_set(data);
}

static void
_drag_up(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   double step;
   Widget_Data *wd;

   wd = elm_widget_data_get(data);
   step = 0.05;

   if (wd->inverted) step *= ELM_SLIDER_INVERTED_FACTOR;

   edje_object_part_drag_step(wd->slider, "elm.dragable.slider", step, step);
}

static void
_drag_down(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   double step;
   Widget_Data *wd;

   wd = elm_widget_data_get(data);
   step = -0.05;

   if (wd->inverted) step *= ELM_SLIDER_INVERTED_FACTOR;

   edje_object_part_drag_step(wd->slider, "elm.dragable.slider", step, step);
}

static void
_spacer_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord x, y, w, h;
   double button_x = 0.0, button_y = 0.0;

   wd->spacer_down = EINA_TRUE;
   wd->val2 = wd->val;
   evas_object_geometry_get(wd->spacer, &x, &y, &w, &h);
   wd->downx = ev->canvas.x - x;
   wd->downy = ev->canvas.y - y;
   if (wd->horizontal)
     {
        button_x = ((double)ev->canvas.x - (double)x) / (double)w;
        if (button_x > 1) button_x = 1;
        if (button_x < 0) button_x = 0;
     }
   else
     {
        button_y = ((double)ev->canvas.y - (double)y) / (double)h;
        if (button_y > 1) button_y = 1;
        if (button_y < 0) button_y = 0;
     }
   edje_object_part_drag_value_set(wd->slider, "elm.dragable.slider", button_x, button_y);
   _val_fetch(data);
   evas_object_smart_callback_call(data, SIG_DRAG_START, NULL);
   _units_set(data);
   _indicator_set(data);
   edje_object_signal_emit(wd->slider, "elm,state,indicator,show", "elm");
}

static void
_spacer_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord x, y, w, h;
   double button_x = 0.0, button_y = 0.0;

   if  (wd->spacer_down)
     {
        Evas_Coord d = 0;

        evas_object_geometry_get(wd->spacer, &x, &y, &w, &h);
        if (wd->horizontal) d = abs(ev->cur.canvas.x - x - wd->downx);
        else d = abs(ev->cur.canvas.y - y - wd->downy);
        if (d > (_elm_config->thumbscroll_threshold - 1))
          {
             if (!wd->frozen)
               {
                  elm_widget_scroll_freeze_push(data);
                  wd->frozen = 1;
               }
             ev->event_flags &= ~EVAS_EVENT_FLAG_ON_HOLD;
          }

        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          {
             if (wd->spacer_down) wd->spacer_down = EINA_FALSE;
             _val_fetch(data);
             evas_object_smart_callback_call(data, SIG_DRAG_STOP, NULL);
             _units_set(data);
             _indicator_set(data);
             if (wd->frozen)
               {
                  elm_widget_scroll_freeze_pop(data);
                  wd->frozen = 0;
               }
             edje_object_signal_emit(wd->slider, "elm,state,indicator,hide", "elm");
             elm_slider_value_set(data, wd->val2);
             return;
          }
        if (wd->horizontal)
          {
             button_x = ((double)ev->cur.canvas.x - (double)x) / (double)w;
             if (button_x > 1) button_x = 1;
             if (button_x < 0) button_x = 0;
          }
        else
          {
             button_y = ((double)ev->cur.canvas.y - (double)y) / (double)h;
             if (button_y > 1) button_y = 1;
             if (button_y < 0) button_y = 0;
          }
        edje_object_part_drag_value_set(wd->slider, "elm.dragable.slider", button_x, button_y);
        _val_fetch(data);
        _units_set(data);
        _indicator_set(data);
     }
}

static void
_spacer_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd->spacer_down) return;
   if (wd->spacer_down) wd->spacer_down = EINA_FALSE;
   _val_fetch(data);
   evas_object_smart_callback_call(data, SIG_DRAG_STOP, NULL);
   _units_set(data);
   _indicator_set(data);
   if (wd->frozen)
     {
        elm_widget_scroll_freeze_pop(data);
        wd->frozen = 0;
     }
   edje_object_signal_emit(wd->slider, "elm,state,indicator,hide", "elm");
}

static void
_elm_slider_label_set(Evas_Object *obj, const char *part, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   const char* default_part = "elm.text";
   const char* real_part;

   if (!wd) return;

   if (!part)
     real_part = default_part;
   else
     real_part = part;

   if (wd->labels)
     {
       const char* old_label;

       old_label = eina_hash_find(wd->labels, real_part);
       if (!old_label)
	   eina_hash_add(wd->labels, real_part, eina_stringshare_add(label));
       else
	 {
	   eina_stringshare_ref(old_label);
	   eina_hash_modify(wd->labels, real_part, eina_stringshare_add(label));
	   eina_stringshare_del(old_label);
	 }
     }

   if (label)
     {
        edje_object_signal_emit(wd->slider, "elm,state,text,visible", "elm");
        edje_object_message_signal_process(wd->slider);
     }
   else
     {
        edje_object_signal_emit(wd->slider, "elm,state,text,hidden", "elm");
        edje_object_message_signal_process(wd->slider);
     }

   edje_object_part_text_set(wd->slider, real_part, label);
   _sizing_eval(obj);
}

static const char *
_elm_slider_label_get(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!wd->labels) return NULL;

   if (!part)
     return eina_hash_find(wd->labels, "elm.text");
   return eina_hash_find(wd->labels, part);
}

static void
_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->icon == icon) return;
   if (wd->icon) evas_object_del(wd->icon);
   wd->icon = icon;
   if (icon)
     {
        elm_widget_sub_object_add(obj, icon);
        evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_part_swallow(wd->slider, "elm.swallow.icon", icon);
        edje_object_signal_emit(wd->slider, "elm,state,icon,visible", "elm");
        edje_object_message_signal_process(wd->slider);
     }
   _sizing_eval(obj);
}

static Evas_Object *
_icon_unset(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *ret = NULL;
   if (!wd) return NULL;
   if (wd->icon)
     {
        elm_widget_sub_object_del(obj, wd->icon);
        evas_object_event_callback_del_full(wd->icon,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
        ret = wd->icon;
        edje_object_part_unswallow(wd->slider, wd->icon);
        edje_object_signal_emit(wd->slider, "elm,state,icon,hidden", "elm");
        wd->icon = NULL;
        _sizing_eval(obj);
     }
   return ret;
}

static void
_end_set(Evas_Object *obj, Evas_Object *end)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->end == end) return;
   if (wd->end) evas_object_del(wd->end);
   wd->end = end;
   if (end)
     {
        elm_widget_sub_object_add(obj, end);
        evas_object_event_callback_add(end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_part_swallow(wd->slider, "elm.swallow.end", end);
        edje_object_signal_emit(wd->slider, "elm,state,end,visible", "elm");
        edje_object_message_signal_process(wd->slider);
     }
   _sizing_eval(obj);
}

static Evas_Object *
_end_unset(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *ret = NULL;
   if (!wd) return NULL;
   if (wd->end)
     {
        elm_widget_sub_object_del(obj, wd->end);
        evas_object_event_callback_del_full(wd->end,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
        ret = wd->end;
        edje_object_part_unswallow(wd->slider, wd->end);
        edje_object_signal_emit(wd->slider, "elm,state,end,hidden", "elm");
        wd->end = NULL;
        _sizing_eval(obj);
     }
   return ret;
}

static void
_content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!part || !strcmp(part, "icon"))
     _icon_set(obj, content);
   else if (!strcmp(part, "end"))
     _end_set(obj, content);
}

static Evas_Object *
_content_get_hook(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!part || !strcmp(part, "icon"))
     return wd->icon;
   else if (!strcmp(part, "end"))
     return wd->end;
   return NULL;
}

static Evas_Object *
_content_unset_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!part || !strcmp(part, "icon"))
     return _icon_unset(obj);
   else if (!strcmp(part, "end"))
     return _end_unset(obj);
   return NULL;
}

static void
_hash_labels_free_cb(void* label)
{
  if (label)
    eina_stringshare_del(label);
}

static void
_min_max_set(Evas_Object *obj)
{
   char *buf_min = NULL;
   char *buf_max = NULL;

   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->units_format_func)
     {
        buf_min = wd->units_format_func(wd->val_min);
        buf_max = wd->units_format_func(wd->val_max);
     }
   else if (wd->units)
     {
        int length = strlen(wd->units);

        buf_min = alloca(length + 128);
        buf_max = alloca(length + 128);

        snprintf((char*) buf_min, length + 128, wd->units, wd->val_min);
        snprintf((char*) buf_max, length + 128, wd->units, wd->val_max);
     }

   edje_object_part_text_set(wd->slider, "elm.units.min", buf_min);
   edje_object_part_text_set(wd->slider, "elm.units.max", buf_max);

   if (wd->units_format_func && wd->units_format_free)
     {
        wd->units_format_free(buf_min);
        wd->units_format_free(buf_max);
     }
}


EAPI Evas_Object *
elm_slider_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "slider");
   elm_widget_type_set(obj, "slider");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_event_hook_set(obj, _event_hook);
   elm_widget_text_set_hook_set(obj, _elm_slider_label_set);
   elm_widget_text_get_hook_set(obj, _elm_slider_label_get);
   elm_widget_content_set_hook_set(obj, _content_set_hook);
   elm_widget_content_get_hook_set(obj, _content_get_hook);
   elm_widget_content_unset_hook_set(obj, _content_unset_hook);

   wd->horizontal = EINA_TRUE;
   wd->indicator_show = EINA_TRUE;
   wd->val = 0.0;
   wd->val_min = 0.0;
   wd->val_max = 1.0;
   wd->labels = eina_hash_string_superfast_new(_hash_labels_free_cb);

   wd->slider = edje_object_add(e);
   _elm_theme_object_set(obj, wd->slider, "slider", "horizontal", "default");
   elm_widget_resize_object_set(obj, wd->slider);
   edje_object_signal_callback_add(wd->slider, "drag", "*", _drag, obj);
   edje_object_signal_callback_add(wd->slider, "drag,start", "*", _drag_start, obj);
   edje_object_signal_callback_add(wd->slider, "drag,stop", "*", _drag_stop, obj);
   edje_object_signal_callback_add(wd->slider, "drag,step", "*", _drag_step, obj);
   edje_object_signal_callback_add(wd->slider, "drag,page", "*", _drag_stop, obj);
   //   edje_object_signal_callback_add(wd->slider, "drag,set", "*", _drag_stop, obj);
   edje_object_part_drag_value_set(wd->slider, "elm.dragable.slider", 0.0, 0.0);

   wd->spacer = evas_object_rectangle_add(e);
   evas_object_color_set(wd->spacer, 0, 0, 0, 0);
   evas_object_pass_events_set(wd->spacer, EINA_TRUE);
   elm_widget_sub_object_add(obj, wd->spacer);
   edje_object_part_swallow(wd->slider, "elm.swallow.bar", wd->spacer);
   evas_object_event_callback_add(wd->spacer, EVAS_CALLBACK_MOUSE_DOWN, _spacer_down_cb, obj);
   evas_object_event_callback_add(wd->spacer, EVAS_CALLBACK_MOUSE_MOVE, _spacer_move_cb, obj);
   evas_object_event_callback_add(wd->spacer, EVAS_CALLBACK_MOUSE_UP, _spacer_up_cb, obj);
   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);

   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);
   return obj;
}

EAPI void
elm_slider_span_size_set(Evas_Object *obj, Evas_Coord size)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->size == size) return;
   wd->size = size;
   if (wd->horizontal)
     evas_object_size_hint_min_set(wd->spacer, (double)wd->size * elm_widget_scale_get(obj) * _elm_config->scale, 1);
   else
     evas_object_size_hint_min_set(wd->spacer, 1, (double)wd->size * elm_widget_scale_get(obj) * _elm_config->scale);
   if (wd->indicator_show)
     edje_object_signal_emit(wd->slider, "elm,state,val,show", "elm");
   else
     edje_object_signal_emit(wd->slider, "elm,state,val,hide", "elm");
   edje_object_part_swallow(wd->slider, "elm.swallow.bar", wd->spacer);
   _sizing_eval(obj);
}

EAPI Evas_Coord
elm_slider_span_size_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->size;
}

EAPI void
elm_slider_unit_format_set(Evas_Object *obj, const char *units)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->units, units);
   if (units)
     {
        edje_object_signal_emit(wd->slider, "elm,state,units,visible", "elm");
        edje_object_message_signal_process(wd->slider);
     }
   else
     {
        edje_object_signal_emit(wd->slider, "elm,state,units,hidden", "elm");
        edje_object_message_signal_process(wd->slider);
     }
   _min_max_set(obj);
   _units_set(obj);
   _sizing_eval(obj);
}

EAPI const char *
elm_slider_unit_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->units;
}

EAPI void
elm_slider_indicator_format_set(Evas_Object *obj, const char *indicator)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->indicator, indicator);
   _indicator_set(obj);
}

EAPI const char *
elm_slider_indicator_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->indicator;
}

EAPI void
elm_slider_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   horizontal = !!horizontal;
   if (wd->horizontal == horizontal) return;
   wd->horizontal = horizontal;
   _theme_hook(obj);
}

EAPI Eina_Bool
elm_slider_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->horizontal;
}

EAPI void
elm_slider_min_max_set(Evas_Object *obj, double min, double max)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((wd->val_min == min) && (wd->val_max == max)) return;
   wd->val_min = min;
   wd->val_max = max;
   if (wd->val < wd->val_min) wd->val = wd->val_min;
   if (wd->val > wd->val_max) wd->val = wd->val_max;
   _min_max_set(obj);
   _val_set(obj);
   _units_set(obj);
   _indicator_set(obj);
}

EAPI void
elm_slider_min_max_get(const Evas_Object *obj, double *min, double *max)
{
   if (min) *min = 0.0;
   if (max) *max = 0.0;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (min) *min = wd->val_min;
   if (max) *max = wd->val_max;
}

EAPI void
elm_slider_value_set(Evas_Object *obj, double val)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->val == val) return;
   wd->val = val;
   if (wd->val < wd->val_min) wd->val = wd->val_min;
   if (wd->val > wd->val_max) wd->val = wd->val_max;
   _val_set(obj);
   _units_set(obj);
   _indicator_set(obj);
}

EAPI double
elm_slider_value_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0.0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0.0;
   return wd->val;
}

EAPI void
elm_slider_inverted_set(Evas_Object *obj, Eina_Bool inverted)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   inverted = !!inverted;
   if (wd->inverted == inverted) return;
   wd->inverted = inverted;
   if (wd->inverted)
     edje_object_signal_emit(wd->slider, "elm,state,inverted,on", "elm");
   else
     edje_object_signal_emit(wd->slider, "elm,state,inverted,off", "elm");
   edje_object_message_signal_process(wd->slider);
   _val_set(obj);
   _units_set(obj);
   _indicator_set(obj);
}

EAPI Eina_Bool
elm_slider_inverted_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->inverted;
}

EAPI void
elm_slider_indicator_format_function_set(Evas_Object *obj, char *(*func)(double val), void (*free_func)(char *str))
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->indicator_format_func = func;
   wd->indicator_format_free = free_func;
   _indicator_set(obj);
}

EAPI void
elm_slider_units_format_function_set(Evas_Object *obj, char *(*func)(double val), void (*free_func)(char *str))
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->units_format_func = func;
   wd->units_format_free = free_func;
   _min_max_set(obj);
   _units_set(obj);
}

EAPI void
elm_slider_indicator_show_set(Evas_Object *obj, Eina_Bool show)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (show) {
        wd->indicator_show = EINA_TRUE;
        edje_object_signal_emit(wd->slider, "elm,state,val,show", "elm");
   }
   else {
        wd->indicator_show = EINA_FALSE;
        edje_object_signal_emit(wd->slider, "elm,state,val,hide", "elm");
   }
}

EAPI Eina_Bool
elm_slider_indicator_show_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->indicator_show;
}

