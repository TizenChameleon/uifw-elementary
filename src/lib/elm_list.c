#include <Elementary.h>
#include "elm_priv.h"

#define SWIPE_MOVES 12

/**
 * @defgroup List List
 *
 * A list is a very simple type of list widget.  For more robust
 * lists, @ref Genlist should probably be used.
 *
 * Signals that you can add callbacks for are:
 *
 * "clicked,double" - when the user double-clicked an item
 * "selected" - when the user selected an item
 * "unselected" - when the user selected an item
 * "longpressed" - an item in the hoversel list is long-pressed
 * "scroll,edge,top" - the list is scrolled until the top edge
 * "scroll,edge,bottom" - the list is scrolled until the bottom edge
 * "scroll,edge,left" - the list is scrolled until the left edge
 * "scroll,edge,right" - the list is scrolled until the right edge
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *scr, *box, *self;
   Eina_List *items, *selected, *to_delete;
   Elm_List_Item *last_selected_item;
   Elm_List_Mode mode;
   Elm_List_Mode h_mode;
   Evas_Coord minw[2], minh[2];
   Eina_Bool scr_minw : 1;
   Eina_Bool scr_minh : 1;
   int walking;
   int movements;
   struct {
        Evas_Coord x, y;
   } history[SWIPE_MOVES];
   Eina_Bool swipe : 1;
   Eina_Bool fix_pending : 1;
   Eina_Bool on_hold : 1;
   Eina_Bool multi : 1;
   Eina_Bool always_select : 1;
   Eina_Bool longpressed : 1;
   Eina_Bool wasselected : 1;
};

struct _Elm_List_Item
{
   Elm_Widget_Item base;
   Widget_Data *wd;
   Eina_List *node;
   const char *label;
   Evas_Object *icon, *end;
   Evas_Smart_Cb func;
   Ecore_Timer *long_timer;
   Ecore_Timer *swipe_timer;
   Eina_Bool deleted : 1;
   Eina_Bool disabled : 1;
   Eina_Bool even : 1;
   Eina_Bool is_even : 1;
   Eina_Bool is_separator : 1;
   Eina_Bool fixed : 1;
   Eina_Bool selected : 1;
   Eina_Bool hilighted : 1;
   Eina_Bool dummy_icon : 1;
   Eina_Bool dummy_end : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _signal_emit_hook(Evas_Object *obj, const char *emission, const char *source);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _fix_items(Evas_Object *obj);
static void _mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _scroll_edge_left(void *data, Evas_Object *scr, void *event_info);
static void _scroll_edge_right(void *data, Evas_Object *scr, void *event_info);
static void _scroll_edge_top(void *data, Evas_Object *scr, void *event_info);
static void _scroll_edge_bottom(void *data, Evas_Object *scr, void *event_info);
static Eina_Bool _item_multi_select_up(Widget_Data *wd);
static Eina_Bool _item_multi_select_down(Widget_Data *wd);
static Eina_Bool _item_single_select_up(Widget_Data *wd);
static Eina_Bool _item_single_select_down(Widget_Data *wd);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);
static Eina_Bool _deselect_all_items(Widget_Data *wd);

static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_SELECTED[] = "selected";
static const char SIG_UNSELECTED[] = "unselected";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_SCROLL_EDGE_TOP[] = "scroll,edge,top";
static const char SIG_SCROLL_EDGE_BOTTOM[] = "scroll,edge,bottom";
static const char SIG_SCROLL_EDGE_LEFT[] = "scroll,edge,left";
static const char SIG_SCROLL_EDGE_RIGHT[] = "scroll,edge,right";

static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_SELECTED, ""},
   {SIG_UNSELECTED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_SCROLL_EDGE_TOP, ""},
   {SIG_SCROLL_EDGE_BOTTOM, ""},
   {SIG_SCROLL_EDGE_LEFT, ""},
   {SIG_SCROLL_EDGE_RIGHT, ""},
   {NULL, NULL}
};

#define ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, ...)                      \
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, __VA_ARGS__);             \
if (it->deleted)                                                         \
{                                                                        \
   ERR("ERROR: "#it" has been DELETED.\n");                              \
   return __VA_ARGS__;                                                   \
}



static inline void
_elm_list_item_free(Elm_List_Item *it)
{
   evas_object_event_callback_del_full
      (it->base.view, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, it);
   evas_object_event_callback_del_full
      (it->base.view, EVAS_CALLBACK_MOUSE_UP, _mouse_up, it);
   evas_object_event_callback_del_full
      (it->base.view, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, it);

   if (it->icon)
     evas_object_event_callback_del_full
        (it->icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
         _changed_size_hints, it->base.widget);

   if (it->end)
     evas_object_event_callback_del_full
        (it->end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
         _changed_size_hints, it->base.widget);

   eina_stringshare_del(it->label);

   if (it->swipe_timer) ecore_timer_del(it->swipe_timer);
   if (it->long_timer) ecore_timer_del(it->long_timer);
   if (it->icon) evas_object_del(it->icon);
   if (it->end) evas_object_del(it->end);

   elm_widget_item_del(it);
}

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (!wd->items) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   Elm_List_Item *it = NULL;
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;

   elm_smart_scroller_child_pos_get(wd->scr, &x, &y);
   elm_smart_scroller_step_size_get(wd->scr, &step_x, &step_y);
   elm_smart_scroller_page_size_get(wd->scr, &page_x, &page_y);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &v_w, &v_h);

   /* TODO: fix logic for horizontal mode */
   if ((!strcmp(ev->keyname, "Left")) ||
       (!strcmp(ev->keyname, "KP_Left")))
     {
        if ((wd->h_mode) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_up(wd)))
             || (_item_single_select_up(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          x -= step_x;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            (!strcmp(ev->keyname, "KP_Right")))
     {
        if ((wd->h_mode) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_down(wd)))
             || (_item_single_select_down(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          x += step_x;
     }
   else if ((!strcmp(ev->keyname, "Up"))  ||
            (!strcmp(ev->keyname, "KP_Up")))
     {
        if ((!wd->h_mode) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_up(wd)))
             || (_item_single_select_up(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          y -= step_y;
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            (!strcmp(ev->keyname, "KP_Down")))
     {
        if ((!wd->h_mode) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_down(wd)))
             || (_item_single_select_down(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          y += step_y;
     }
   else if ((!strcmp(ev->keyname, "Home")) ||
            (!strcmp(ev->keyname, "KP_Home")))
     {
        it = eina_list_data_get(wd->items);
        elm_list_item_bring_in(it);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "End")) ||
            (!strcmp(ev->keyname, "KP_End")))
     {
        it = eina_list_data_get(eina_list_last(wd->items));
        elm_list_item_bring_in(it);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Prior")) ||
            (!strcmp(ev->keyname, "KP_Prior")))
     {
        if (wd->h_mode)
          {
             if (page_x < 0)
               x -= -(page_x * v_w) / 100;
             else
               x -= page_x;
          }
        else
          {
             if (page_y < 0)
               y -= -(page_y * v_h) / 100;
             else
               y -= page_y;
          }
     }
   else if ((!strcmp(ev->keyname, "Next")) ||
            (!strcmp(ev->keyname, "KP_Next")))
     {
        if (wd->h_mode)
          {
             if (page_x < 0)
               x += -(page_x * v_w) / 100;
             else
               x += page_x;
          }
        else
          {
             if (page_y < 0)
               y += -(page_y * v_h) / 100;
             else
               y += page_y;
          }
     }
   else if (!strcmp(ev->keyname, "Escape"))
     {
        if (!_deselect_all_items(wd)) return EINA_FALSE;
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   elm_smart_scroller_child_pos_set(wd->scr, x, y);
   return EINA_TRUE;
}

static Eina_Bool
_deselect_all_items(Widget_Data *wd)
{
   if (!wd->selected) return EINA_FALSE;
   while (wd->selected)
     elm_list_item_selected_set(wd->selected->data, EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_up(Widget_Data *wd)
{
   if (!wd->selected) return EINA_FALSE;
   if (!wd->multi) return EINA_FALSE;

   Elm_List_Item *prev = elm_list_item_prev(wd->last_selected_item);
   if (!prev) return EINA_TRUE;

   if (elm_list_item_selected_get(prev))
     {
        elm_list_item_selected_set(wd->last_selected_item, EINA_FALSE);
        wd->last_selected_item = prev;
        elm_list_item_show(wd->last_selected_item);
     }
   else
     {
        elm_list_item_selected_set(prev, EINA_TRUE);
        elm_list_item_show(prev);
     }
   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_down(Widget_Data *wd)
{
   if (!wd->selected) return EINA_FALSE;
   if (!wd->multi) return EINA_FALSE;

   Elm_List_Item *next = elm_list_item_next(wd->last_selected_item);
   if (!next) return EINA_TRUE;

   if (elm_list_item_selected_get(next))
     {
        elm_list_item_selected_set(wd->last_selected_item, EINA_FALSE);
        wd->last_selected_item = next;
        elm_list_item_show(wd->last_selected_item);
     }
   else
     {
        elm_list_item_selected_set(next, EINA_TRUE);
        elm_list_item_show(next);
     }
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_up(Widget_Data *wd)
{
   Elm_List_Item *prev;

   if (!wd->selected) prev = eina_list_data_get(eina_list_last(wd->items));
   else prev = elm_list_item_prev(wd->last_selected_item);

   if (!prev) return EINA_FALSE;

   _deselect_all_items(wd);

   elm_list_item_selected_set(prev, EINA_TRUE);
   elm_list_item_show(prev);
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_down(Widget_Data *wd)
{
   Elm_List_Item *next;

   if (!wd->selected) next = eina_list_data_get(wd->items);
   else next = elm_list_item_next(wd->last_selected_item);

   if (!next) return EINA_FALSE;

   _deselect_all_items(wd);

   elm_list_item_selected_set(next, EINA_TRUE);
   elm_list_item_show(next);
   return EINA_TRUE;
}

static void
_elm_list_process_deletions(Widget_Data *wd)
{
   Elm_List_Item *it;

   wd->walking++; // avoid nested deletion and also _sub_del() fix_items

   EINA_LIST_FREE(wd->to_delete, it)
     {
        elm_widget_item_pre_notify_del(it);

        wd->items = eina_list_remove_list(wd->items, it->node);
        _elm_list_item_free(it);
     }

   wd->walking--;
}

static inline void
_elm_list_walk(Widget_Data *wd)
{
   if (wd->walking < 0)
     {
        ERR("ERROR: walking was negative. fixed!\n");
        wd->walking = 0;
     }
   wd->walking++;
}

static inline void
_elm_list_unwalk(Widget_Data *wd)
{
   wd->walking--;
   if (wd->walking < 0)
     {
        ERR("ERROR: walking became negative. fixed!\n");
        wd->walking = 0;
     }

   if (wd->walking)
     return;

   if (wd->to_delete)
     _elm_list_process_deletions(wd);

   if (wd->fix_pending)
     {
        wd->fix_pending = EINA_FALSE;
        _fix_items(wd->self);
        _sizing_eval(wd->self);
     }
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it;
   Eina_List *n;

   if (!wd) return;
   if (wd->walking)
     ERR("ERROR: list deleted while walking.\n");

   _elm_list_walk(wd);
   EINA_LIST_FOREACH(wd->items, n, it) elm_widget_item_pre_notify_del(it);
   _elm_list_unwalk(wd);
   if (wd->to_delete)
     ERR("ERROR: leaking nodes!\n");

   EINA_LIST_FREE(wd->items, it) _elm_list_item_free(it);
   eina_list_free(wd->selected);
   free(wd);
}

static void
_show_region_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   elm_smart_scroller_child_region_set(wd->scr, x, y, w, h);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_disabled_get(obj))
     {
        _signal_emit_hook(obj, "elm,state,disabled", "elm");
        elm_widget_scroll_freeze_push(obj);
        elm_widget_scroll_hold_push(obj);
        /* FIXME: if we get to have a way to only un-hilight items
         * in the future, keeping them selected... */
        _deselect_all_items(wd);
     }
   else
     {
        _signal_emit_hook(obj, "elm,state,enabled", "elm");
        elm_widget_scroll_freeze_pop(obj);
        elm_widget_scroll_hold_pop(obj);
     }
}

static void
_sizing_eval(Evas_Object *obj)
{

   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   Evas_Coord  vw, vh, minw, minh, maxw, maxh, w, h, vmw, vmh;
   double xw, yw;

   evas_object_size_hint_min_get(wd->box, &minw, &minh);
   evas_object_size_hint_max_get(wd->box, &maxw, &maxh);
   evas_object_size_hint_weight_get(wd->box, &xw, &yw);
   if (!wd->scr) return;
   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   if (xw > 0.0)
     {
        if ((minw > 0) && (vw < minw)) vw = minw;
        else if ((maxw > 0) && (vw > maxw)) vw = maxw;
     }
   else if (minw > 0) vw = minw;
   if (yw > 0.0)
     {
        if ((minh > 0) && (vh < minh)) vh = minh;
        else if ((maxh > 0) && (vh > maxh)) vh = maxh;
     }
   else if (minh > 0) vh = minh;
   evas_object_resize(wd->box, vw, vh);
   w = -1;
   h = -1;
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &vmw, &vmh);
   if (wd->scr_minw) w = vmw + minw;
   if (wd->scr_minh) h = vmh + minh;

   evas_object_size_hint_max_get(obj, &maxw, &maxh);
   if ((maxw > 0) && (w > maxw))
     w = maxw;
   if ((maxh > 0) && (h > maxh))
     h = maxh;

   evas_object_size_hint_min_set(obj, w, h);
}

static void
_signal_emit_hook(Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           emission, source);
}

static void
_signal_callback_add_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_signal_callback_add(elm_smart_scroller_edje_object_get(wd->scr),
                                   emission, source, func_cb, data);
}

static void
_signal_callback_del_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_signal_callback_del_full(
      elm_smart_scroller_edje_object_get(wd->scr),
      emission, source, func_cb, data);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it;
   Eina_List *n;

   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_mirrored_set(wd->scr, rtl);

   EINA_LIST_FOREACH(wd->items, n, it)
      edje_object_mirrored_set(it->base.view, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it;
   Eina_List *n;

   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   if (wd->scr)
     {
        Evas_Object *edj;
        const char *str;

        elm_smart_scroller_object_theme_set(obj, wd->scr, "list", "base",
                                            elm_widget_style_get(obj));
        //        edje_object_scale_set(wd->scr, elm_widget_scale_get(obj) * _elm_config->scale);
        edj = elm_smart_scroller_edje_object_get(wd->scr);
        str = edje_object_data_get(edj, "focus_highlight");
        if ((str) && (!strcmp(str, "on")))
          elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
        else
          elm_widget_highlight_in_theme_set(obj, EINA_FALSE);
        elm_object_style_set(wd->scr, elm_widget_style_get(obj));
     }
   EINA_LIST_FOREACH(wd->items, n, it)
     {
        edje_object_scale_set(it->base.view, elm_widget_scale_get(obj) * _elm_config->scale);
        it->fixed = 0;
     }
   _fix_items(obj);
   _sizing_eval(obj);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->self, "elm,action,focus", "elm");
        evas_object_focus_set(wd->self, EINA_TRUE);

        if ((wd->selected) && (!wd->last_selected_item))
          wd->last_selected_item = eina_list_data_get(wd->selected);
     }
   else
     {
        edje_object_signal_emit(wd->self, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->self, EINA_FALSE);
     }
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   _fix_items(data);
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   const Eina_List *l;
   Elm_List_Item *it;

   if (!wd) return;
   if (!sub) abort();
   if (sub == wd->scr)
     wd->scr = NULL;
   else
     {
        EINA_LIST_FOREACH(wd->items, l, it)
          {
             if ((sub == it->icon) || (sub == it->end))
               {
                  if (it->icon == sub) it->icon = NULL;
                  if (it->end == sub) it->end = NULL;
                  evas_object_event_callback_del_full
                     (sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints,
                      obj);
                  if (!wd->walking)
                    {
                       _fix_items(obj);
                       _sizing_eval(obj);
                    }
                  else
                    wd->fix_pending = EINA_TRUE;
                  break;
               }
          }
     }
}

static void
_item_hilight(Elm_List_Item *it)
{
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *selectraise;

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (it->hilighted) return;

   evas_object_ref(obj);
   _elm_list_walk(wd);

   edje_object_signal_emit(it->base.view, "elm,state,selected", "elm");
   selectraise = edje_object_data_get(it->base.view, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     evas_object_raise(it->base.view);
   it->hilighted = EINA_TRUE;

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

static void
_item_select(Elm_List_Item *it)
{
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (it->selected)
     {
        if (wd->always_select) goto call;
        return;
     }
   it->selected = EINA_TRUE;
   wd->selected = eina_list_append(wd->selected, it);

call:
   evas_object_ref(obj);
   _elm_list_walk(wd);

   if (it->func) it->func((void *)it->base.data, it->base.widget, it);
   evas_object_smart_callback_call(obj, SIG_SELECTED, it);
   it->wd->last_selected_item = it;

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

static void
_item_unselect(Elm_List_Item *it)
{
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *stacking, *selectraise;

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (!it->hilighted) return;

   evas_object_ref(obj);
   _elm_list_walk(wd);

   edje_object_signal_emit(it->base.view, "elm,state,unselected", "elm");
   stacking = edje_object_data_get(it->base.view, "stacking");
   selectraise = edje_object_data_get(it->base.view, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     {
        if ((stacking) && (!strcmp(stacking, "below")))
          evas_object_lower(it->base.view);
     }
   it->hilighted = EINA_FALSE;
   if (it->selected)
     {
        it->selected = EINA_FALSE;
        wd->selected = eina_list_remove(wd->selected, it);
        evas_object_smart_callback_call(it->base.widget, SIG_UNSELECTED, it);
     }

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

static Eina_Bool
_swipe_cancel(void *data)
{
   Elm_List_Item *it = data;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);

   if (!wd) return ECORE_CALLBACK_CANCEL;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, ECORE_CALLBACK_CANCEL);
   wd->swipe = EINA_FALSE;
   wd->movements = 0;
   return ECORE_CALLBACK_RENEW;
}

static void
_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_List_Item *it = data;
   Evas_Object *obj2 = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj2);
   Evas_Event_Mouse_Move *ev = event_info;

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);

   evas_object_ref(obj2);
   _elm_list_walk(wd);

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (!wd->on_hold)
          {
             wd->on_hold = EINA_TRUE;
             if (it->long_timer)
               {
                  ecore_timer_del(it->long_timer);
                  it->long_timer = NULL;
               }
             if (!wd->wasselected)
               _item_unselect(it);
          }
        if (wd->movements == SWIPE_MOVES) wd->swipe = EINA_TRUE;
        else
          {
             wd->history[wd->movements].x = ev->cur.canvas.x;
             wd->history[wd->movements].y = ev->cur.canvas.y;
             if (abs((wd->history[wd->movements].x - wd->history[0].x)) > 40)
               wd->swipe = EINA_TRUE;
             else
               wd->movements++;
          }
     }

   _elm_list_unwalk(wd);
   evas_object_unref(obj2);
}

static void
_scroll_edge_left(void *data, Evas_Object *scr __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   evas_object_smart_callback_call(obj, SIG_SCROLL_EDGE_LEFT, NULL);
}

static void
_scroll_edge_right(void *data, Evas_Object *scr __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   evas_object_smart_callback_call(obj, SIG_SCROLL_EDGE_RIGHT, NULL);
}

static void
_scroll_edge_top(void *data, Evas_Object *scr __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   evas_object_smart_callback_call(obj, SIG_SCROLL_EDGE_TOP, NULL);
}

static void
_scroll_edge_bottom(void *data, Evas_Object *scr __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   evas_object_smart_callback_call(obj, SIG_SCROLL_EDGE_BOTTOM, NULL);
}

static Eina_Bool
_long_press(void *data)
{
   Elm_List_Item *it = data;
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) goto end;

   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, ECORE_CALLBACK_CANCEL);
   it->long_timer = NULL;
   if (it->disabled) goto end;

   wd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(it->base.widget, SIG_LONGPRESSED, it);

end:
   return ECORE_CALLBACK_CANCEL;
}

static void
_swipe(Elm_List_Item *it)
{
   int i, sum = 0;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);

   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (!wd) return;
   wd->swipe = EINA_FALSE;
   for (i = 0; i < wd->movements; i++)
     {
        sum += wd->history[i].x;
        if (abs(wd->history[0].y - wd->history[i].y) > 10) return;
     }

   sum /= wd->movements;
   if (abs(sum - wd->history[0].x) <= 10) return;
   evas_object_smart_callback_call(it->base.widget, "swipe", it);
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_List_Item *it = data;
   Evas_Object *obj2 = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj2);
   Evas_Event_Mouse_Down *ev = event_info;

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (wd->on_hold) return;
   wd->wasselected = it->selected;

   evas_object_ref(obj2);
   _elm_list_walk(wd);

   _item_hilight(it);
   wd->longpressed = EINA_FALSE;
   if (it->long_timer) ecore_timer_del(it->long_timer);
   it->long_timer = ecore_timer_add(_elm_config->longpress_timeout, _long_press, it);
   if (it->swipe_timer) ecore_timer_del(it->swipe_timer);
   it->swipe_timer = ecore_timer_add(0.4, _swipe_cancel, it);
   /* Always call the callbacks last - the user may delete our context! */
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        evas_object_smart_callback_call(it->base.widget, SIG_CLICKED_DOUBLE, it);
        evas_object_smart_callback_call(it->base.widget, "clicked", it); // will be removed
     }
   wd->swipe = EINA_FALSE;
   wd->movements = 0;

   _elm_list_unwalk(wd);
   evas_object_unref(obj2);
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_List_Item *it = data;
   Evas_Object *obj2 = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj2);
   Evas_Event_Mouse_Up *ev = event_info;

   if (!wd) return;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   wd->longpressed = EINA_FALSE;
   if (it->long_timer)
     {
        ecore_timer_del(it->long_timer);
        it->long_timer = NULL;
     }
   if (it->swipe_timer)
     {
        ecore_timer_del(it->swipe_timer);
        it->swipe_timer = NULL;
     }
   if (wd->on_hold)
     {
        if (wd->swipe) _swipe(data);
        wd->on_hold = EINA_FALSE;
        return;
     }
   if (wd->longpressed)
     {
        if (!wd->wasselected) _item_unselect(it);
        wd->wasselected = 0;
        return;
     }

   if (it->disabled)
     return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   evas_object_ref(obj2);
   _elm_list_walk(wd);

   if (wd->multi)
     {
        if (!it->selected)
          {
             _item_hilight(it);
             _item_select(it);
          }
        else _item_unselect(it);
     }
   else
     {
        if (!it->selected)
          {
             while (wd->selected)
               _item_unselect(wd->selected->data);
             _item_hilight(it);
             _item_select(it);
          }
        else
          {
             const Eina_List *l, *l_next;
             Elm_List_Item *it2;

             EINA_LIST_FOREACH_SAFE(wd->selected, l, l_next, it2)
                if (it2 != it) _item_unselect(it2);
             _item_hilight(it);
             _item_select(it);
          }
     }

   _elm_list_unwalk(wd);
   evas_object_unref(obj2);
}

static Elm_List_Item *
_item_new(Evas_Object *obj, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it;

   if (!wd) return NULL;
   it = elm_widget_item_new(obj, Elm_List_Item);
   it->wd = wd;
   it->label = eina_stringshare_add(label);
   it->icon = icon;
   it->end = end;
   it->func = func;
   it->base.data = data;
   it->base.view = edje_object_add(evas_object_evas_get(obj));
   edje_object_mirrored_set(it->base.view, elm_widget_mirrored_get(obj));
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, it);
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, it);
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOUSE_MOVE,
                                  _mouse_move, it);
   evas_object_size_hint_weight_set(it->base.view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(it->base.view, EVAS_HINT_FILL, EVAS_HINT_FILL);
   if (it->icon)
     {
        elm_widget_sub_object_add(obj, it->icon);
        evas_object_event_callback_add(it->icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
     }
   if (it->end)
     {
        elm_widget_sub_object_add(obj, it->end);
        evas_object_event_callback_add(it->end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
     }
   return it;
}

static void
_elm_list_mode_set_internal(Widget_Data *wd)
{
   if (!wd->scr)
     return;

   if (wd->mode == ELM_LIST_LIMIT)
     {
        if (!wd->h_mode)
          {
             wd->scr_minw = EINA_TRUE;
             wd->scr_minh = EINA_FALSE;
          }
        else
          {
             wd->scr_minw = EINA_FALSE;
             wd->scr_minh = EINA_TRUE;
          }
     }
   else if (wd->mode == ELM_LIST_EXPAND)
     {
        wd->scr_minw = EINA_TRUE;
        wd->scr_minh = EINA_TRUE;
     }
   else
     {
        wd->scr_minw = EINA_FALSE;
        wd->scr_minh = EINA_FALSE;
     }

   _sizing_eval(wd->self);
}

static void
_fix_items(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   const Eina_List *l;
   Elm_List_Item *it;
   Evas_Coord minw[2] = { 0, 0 }, minh[2] = { 0, 0 };
   Evas_Coord mw, mh;
   int i, redo = 0;
   const char *style = elm_widget_style_get(obj);
   const char *it_plain = wd->h_mode ? "h_item" : "item";
   const char *it_odd = wd->h_mode ? "h_item_odd" : "item_odd";
   const char *it_compress = wd->h_mode ? "h_item_compress" : "item_compress";
   const char *it_compress_odd = wd->h_mode ? "h_item_compress_odd" : "item_compress_odd";

   if (wd->walking)
     {
        wd->fix_pending = EINA_TRUE;
        return;
     }

   evas_object_ref(obj);
   _elm_list_walk(wd); // watch out "return" before unwalk!

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->deleted) continue;
        if (it->icon)
          {
             evas_object_size_hint_min_get(it->icon, &mw, &mh);
             if (mw > minw[0]) minw[0] = mw;
             if (mh > minh[0]) minh[0] = mh;
          }
        if (it->end)
          {
             evas_object_size_hint_min_get(it->end, &mw, &mh);
             if (mw > minw[1]) minw[1] = mw;
             if (mh > minh[1]) minh[1] = mh;
          }
     }

   if ((minw[0] != wd->minw[0]) || (minw[1] != wd->minw[1]) ||
       (minw[0] != wd->minh[0]) || (minh[1] != wd->minh[1]))
     {
        wd->minw[0] = minw[0];
        wd->minw[1] = minw[1];
        wd->minh[0] = minh[0];
        wd->minh[1] = minh[1];
        redo = 1;
     }
   i = 0;
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->deleted)
          continue;

        it->even = i & 0x1;
        if ((it->even != it->is_even) || (!it->fixed) || (redo))
          {
             const char *stacking;

             /* FIXME: separators' themes seem to be b0rked */
             if (it->is_separator)
               _elm_theme_object_set(obj, it->base.view, "separator",
                                     wd->h_mode ? "horizontal" : "vertical",
                                     style);
             else if (wd->mode == ELM_LIST_COMPRESS)
               {
                  if (it->even)
                    _elm_theme_object_set(obj, it->base.view, "list",
                                          it_compress, style);
                  else
                    _elm_theme_object_set(obj, it->base.view, "list",
                                          it_compress_odd, style);
               }
             else
               {
                  if (it->even)
                    _elm_theme_object_set(obj, it->base.view, "list", it_plain,
                                          style);
                  else
                    _elm_theme_object_set(obj, it->base.view, "list", it_odd,
                                          style);
               }
             stacking = edje_object_data_get(it->base.view, "stacking");
             if (stacking)
               {
                  if (!strcmp(stacking, "below"))
                    evas_object_lower(it->base.view);
                  else if (!strcmp(stacking, "above"))
                    evas_object_raise(it->base.view);
               }
             edje_object_part_text_set(it->base.view, "elm.text", it->label);

             if ((!it->icon) && (minh[0] > 0))
               {
                  it->icon = evas_object_rectangle_add(evas_object_evas_get(it->base.view));
                  evas_object_color_set(it->icon, 0, 0, 0, 0);
                  it->dummy_icon = EINA_TRUE;
               }
             if ((!it->end) && (minh[1] > 0))
               {
                  it->end = evas_object_rectangle_add(evas_object_evas_get(it->base.view));
                  evas_object_color_set(it->end, 0, 0, 0, 0);
                  it->dummy_end = EINA_TRUE;
               }
             if (it->icon)
               {
                  evas_object_size_hint_min_set(it->icon, minw[0], minh[0]);
                  evas_object_size_hint_max_set(it->icon, 99999, 99999);
                  edje_object_part_swallow(it->base.view, "elm.swallow.icon", it->icon);
               }
             if (it->end)
               {
                  evas_object_size_hint_min_set(it->end, minw[1], minh[1]);
                  evas_object_size_hint_max_set(it->end, 99999, 99999);
                  edje_object_part_swallow(it->base.view, "elm.swallow.end", it->end);
               }
             if (!it->fixed)
               {
                  // this may call up user and it may modify the list item
                  // but we're safe as we're flagged as walking.
                  // just don't process further
                  edje_object_message_signal_process(it->base.view);
                  if (it->deleted)
                    continue;
                  mw = mh = -1;
                  elm_coords_finger_size_adjust(1, &mw, 1, &mh);
                  edje_object_size_min_restricted_calc(it->base.view, &mw, &mh, mw, mh);
                  elm_coords_finger_size_adjust(1, &mw, 1, &mh);
                  evas_object_size_hint_min_set(it->base.view, mw, mh);
                  evas_object_show(it->base.view);
               }
             if ((it->selected) || (it->hilighted))
               {
                  const char *selectraise;

                  // this may call up user and it may modify the list item
                  // but we're safe as we're flagged as walking.
                  // just don't process further
                  edje_object_signal_emit(it->base.view, "elm,state,selected", "elm");
                  if (it->deleted)
                    continue;

                  selectraise = edje_object_data_get(it->base.view, "selectraise");
                  if ((selectraise) && (!strcmp(selectraise, "on")))
                    evas_object_raise(it->base.view);
               }
             if (it->disabled)
               edje_object_signal_emit(it->base.view, "elm,state,disabled",
                                       "elm");

             it->fixed = EINA_TRUE;
             it->is_even = it->even;
          }
        i++;
     }

   mw = 0; mh = 0;
   evas_object_size_hint_min_get(wd->box, &mw, &mh);

   _elm_list_mode_set_internal(wd);

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

static void
_hold_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_hold_set(wd->scr, EINA_TRUE);
}

static void
_hold_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_hold_set(wd->scr, EINA_FALSE);
}

static void
_freeze_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_freeze_set(wd->scr, EINA_TRUE);
}

static void
_freeze_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_freeze_set(wd->scr, EINA_FALSE);
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

/**
 * Adds a list object.
 *
 * @param parent The parent object
 * @return The created object or NULL upon failure
 *
 * @ingroup List
 */
EAPI Evas_Object *
elm_list_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_Coord minw, minh;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "list");
   elm_widget_type_set(obj, "list");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);
   elm_widget_signal_callback_add_hook_set(obj, _signal_callback_add_hook);
   elm_widget_signal_callback_del_hook_set(obj, _signal_callback_del_hook);
   elm_widget_event_hook_set(obj, _event_hook);

   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_widget_resize_object_set(obj, wd->scr);
   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr), &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);

   elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE,
                                       _elm_config->thumbscroll_bounce_enable);

   wd->box = elm_box_add(parent);
   elm_box_homogeneous_set(wd->box, 1);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, 0.0);
   elm_widget_on_show_region_hook_set(wd->box, _show_region_hook, obj);
   elm_widget_sub_object_add(obj, wd->box);
   elm_smart_scroller_child_set(wd->scr, wd->box);
   evas_object_event_callback_add(wd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);

   evas_object_show(wd->box);

   _theme_hook(obj);

   wd->mode = ELM_LIST_SCROLL;

   evas_object_smart_callback_add(wd->scr, "edge,left", _scroll_edge_left, obj);
   evas_object_smart_callback_add(wd->scr, "edge,right", _scroll_edge_right, obj);
   evas_object_smart_callback_add(wd->scr, "edge,top", _scroll_edge_top, obj);
   evas_object_smart_callback_add(wd->scr, "edge,bottom", _scroll_edge_bottom, obj);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);
   return obj;
}

/**
 * Appends an item to the list object.
 *
 * @param obj The list object
 * @param label The label of the list item
 * @param icon The icon object to use for the left side of the item
 * @param end The icon object to use for the right side of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item for related callbacks
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_append(Evas_Object *obj, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it = _item_new(obj, label, icon, end, func, data);

   wd->items = eina_list_append(wd->items, it);
   it->node = eina_list_last(wd->items);
   elm_box_pack_end(wd->box, it->base.view);
   return it;
}

/**
 * Prepends an item to the list object.
 *
 * @param obj The list object
 * @param label The label of the list item
 * @param icon The icon object to use for the left side of the item
 * @param end The icon object to use for the right side of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item for related callbacks
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_prepend(Evas_Object *obj, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it = _item_new(obj, label, icon, end, func, data);

   wd->items = eina_list_prepend(wd->items, it);
   it->node = wd->items;
   elm_box_pack_start(wd->box, it->base.view);
   return it;
}

/**
 * Inserts an item into the list object before @p before.
 *
 * @param obj The list object
 * @param before The list item to insert before
 * @param label The label of the list item
 * @param icon The icon object to use for the left side of the item
 * @param end The icon object to use for the right side of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item for related callbacks
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_insert_before(Evas_Object *obj, Elm_List_Item *before, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data)
{
   Widget_Data *wd;
   Elm_List_Item *it;

   EINA_SAFETY_ON_NULL_RETURN_VAL(before, NULL);
   if (!before->node) return NULL;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(before, NULL);

   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   it = _item_new(obj, label, icon, end, func, data);
   wd->items = eina_list_prepend_relative_list(wd->items, it, before->node);
   it->node = before->node->prev;
   elm_box_pack_before(wd->box, it->base.view, before->base.view);
   return it;
}

/**
 * Inserts an item into the list object after @p after.
 *
 * @param obj The list object
 * @param after The list item to insert after
 * @param label The label of the list item
 * @param icon The icon object to use for the left side of the item
 * @param end The icon object to use for the right side of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item for related callbacks
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_insert_after(Evas_Object *obj, Elm_List_Item *after, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data)
{
   Widget_Data *wd;
   Elm_List_Item *it;

   EINA_SAFETY_ON_NULL_RETURN_VAL(after, NULL);
   if (!after->node) return NULL;
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(after, NULL);

   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   it = _item_new(obj, label, icon, end, func, data);
   wd->items = eina_list_append_relative_list(wd->items, it, after->node);
   it->node = after->node->next;
   elm_box_pack_after(wd->box, it->base.view, after->base.view);
   return it;
}

/**
 * Insert a new item into the sorted list object.
 *
 * @param obj The list object
 * @param label The label of the list item
 * @param icon The icon object to use for the left side of the item
 * @param end The icon object to use for the right side of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item for related callbacks
 * @param cmp_func The function called for the sort.
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_sorted_insert(Evas_Object *obj, const char *label, Evas_Object *icon, Evas_Object *end, Evas_Smart_Cb func, const void *data, Eina_Compare_Cb cmp_func)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it = _item_new(obj, label, icon, end, func, data);
   Eina_List *l;

   wd->items = eina_list_sorted_insert(wd->items, cmp_func, it);
   l = eina_list_data_find_list(wd->items, it);
   l = eina_list_next(l);
   if (!l)
     {
        it->node = eina_list_last(wd->items);
        elm_box_pack_end(wd->box, it->base.view);
     }
   else
     {
        Elm_List_Item *before = eina_list_data_get(l);
        it->node = before->node->prev;
        elm_box_pack_before(wd->box, it->base.view, before->base.view);
     }
   return it;
}

/**
 * Clears a list of all items.
 *
 * @param obj The list object
 *
 * @ingroup List
 */
EAPI void
elm_list_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_List_Item *it;

   if (!wd) return;
   if (!wd->items) return;

   eina_list_free(wd->selected);
   wd->selected = NULL;

   if (wd->walking > 0)
     {
        Eina_List *n;

        EINA_LIST_FOREACH(wd->items, n, it)
          {
             if (it->deleted) continue;
             it->deleted = EINA_TRUE;
             wd->to_delete = eina_list_append(wd->to_delete, it);
          }
        return;
     }

   evas_object_ref(obj);
   _elm_list_walk(wd);

   EINA_LIST_FREE(wd->items, it)
     {
        elm_widget_item_pre_notify_del(it);
        _elm_list_item_free(it);
     }

   _elm_list_unwalk(wd);

   _fix_items(obj);
   _sizing_eval(obj);
   evas_object_unref(obj);
}

/**
 * Starts the list.  Call before running show() on the list object.
 *
 * @param obj The list object
 *
 * @ingroup List
 */
EAPI void
elm_list_go(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _fix_items(obj);
}

/**
 * Enables/disables the state of multi-select on the list object.
 *
 * @param obj The list object
 * @param multi If true, multi-select is enabled
 *
 * @ingroup List
 */
EAPI void
elm_list_multi_select_set(Evas_Object *obj, Eina_Bool multi)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->multi = multi;
}

/**
 * Gets the state of multi-select on the list object.
 *
 * @param obj The list object
 * @return If true, multi-select is enabled
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_multi_select_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->multi;
}

/**
 * Set which mode to use for the list with.
 *
 * @param obj The list object
 * @param mode One of @c ELM_LIST_COMPRESS, @c ELM_LIST_SCROLL, @c
 *             ELM_LIST_LIMIT or @c ELM_LIST_EXPAND.
 *
 * @note Default value is @c ELM_LIST_SCROLL. At this mode, the list
 * object won't set any of its size hints to inform how a possible
 * container should resize it. Then, if it's not created as a "resize
 * object", it might end with zero dimensions. The list will respect
 * the container's geometry and, if any of its items won't fit into
 * its transverse axis, one will be able to scroll it in that
 * direction. @c ELM_LIST_COMPRESS is the same as the previous, except
 * that it <b>won't</b> let one scroll in the transverse axis, on
 * those cases (large items will get cropped). @c ELM_LIST_LIMIT will
 * actually set a minimun size hint on the list object, so that
 * containers may respect it (and resize itself to fit the child
 * properly). More specifically, a minimum size hint will be set for
 * its transverse axis, so that the <b>largest</b> item in that
 * direction fits well. @c ELM_LIST_EXPAND, besides setting a minimum
 * size on the transverse axis, just like the previous mode, will set
 * a minimum size on the longitudinal axis too, trying to reserve
 * space to all its children to be visible at a time. The last two
 * modes can always have effects bounded by setting the list object's
 * maximum size hints, though.
 *
 * @ingroup List
 */
EAPI void
elm_list_mode_set(Evas_Object *obj, Elm_List_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd)
     return;
   if (wd->mode == mode)
     return;
   wd->mode = mode;

   _elm_list_mode_set_internal(wd);
}

/**
 * Get the mode the list is at.
 *
 * @param obj The list object
 * @return mode One of @c ELM_LIST_COMPRESS, @c ELM_LIST_SCROLL or @c
 *         ELM_LIST_LIMIT (@c ELM_LIST_LAST on errors).
 *
 * @note see elm_list_mode_set() for more information.
 *
 * @ingroup List
 */
EAPI Elm_List_Mode
elm_list_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_LIST_LAST;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_LIST_LAST;
   return wd->mode;
}

/**
 * Enables/disables horizontal mode of the list.
 *
 * @param obj The list object
 * @param mode If true, horizontale mode is enabled
 *
 * @note Bounce options for the list will be reset to default values
 * with this funcion. Re-call elm_list_bounce_set() once more after
 * this one, if you had custom values.
 *
 * @ingroup List
 */
EAPI void
elm_list_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd;
   Eina_Bool bounce = _elm_config->thumbscroll_bounce_enable;

   wd = elm_widget_data_get(obj);
   if (!wd)
     return;

   if (wd->h_mode == horizontal)
     return;

   wd->h_mode = horizontal;
   elm_box_horizontal_set(wd->box, horizontal);

   if (horizontal)
     {
        evas_object_size_hint_weight_set(wd->box, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(wd->box, 0.0, EVAS_HINT_FILL);
        elm_smart_scroller_bounce_allow_set(wd->scr, bounce, EINA_FALSE);
     }
   else
     {
        evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, 0.0);
        elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, bounce);
     }

   _elm_list_mode_set_internal(wd);
}

/**
 * Retrieve whether horizontal mode is enabled for a list.
 *
 * @param obj The list object
 * @return @c EINA_TRUE, if horizontal mode is enabled and @c
 *            EINA_FALSE, otherwise.
 *
 * @note see elm_list_horizontal_set() for more information.
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;

   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd)
     return EINA_FALSE;

   return wd->h_mode;
}

/**
 * Enables/disables the state of always_select, meaning that
 * an item will always be selected.
 *
 * @param obj The list object
 * @param always_select If true, always_select is enabled
 *
 * @ingroup List
 */
EAPI void
elm_list_always_select_mode_set(Evas_Object *obj, Eina_Bool always_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->always_select = always_select;
}

/**
 * Gets the state of always_select.
 * See also elm_list_always_select_mode_set()
 *
 * @param obj The list object
 * @return If true, always_select is enabled
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_always_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->always_select;
}

/**
 * Returns a list of all the list items.
 *
 * @param obj The list object
 * @return An Eina_List* of the list items, or NULL on failure
 *
 * @ingroup List
 */
EAPI const Eina_List *
elm_list_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}

/**
 * Returns the currently selected list item.
 *
 * @param obj The list object
 * @return The selected list item, or NULL on failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->selected) return wd->selected->data;
   return NULL;
}

/**
 * Returns a list of the currently selected list items.
 *
 * @param obj The list object
 * @return An Eina_List* of the selected list items, or NULL on failure
 *
 * @ingroup List
 */
EAPI const Eina_List *
elm_list_selected_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->selected;
}

/**
 * Sets if item is a separator.
 *
 * @param it The list item object
 * @param setting
 */
EAPI void
elm_list_item_separator_set(Elm_List_Item *it, Eina_Bool setting)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   it->is_separator = !!setting;
}

/**
 * Returns EINA_TRUE if Elm_List_Item is a separator.
 *
 * @param it The list item object
 */
EAPI Eina_Bool
elm_list_item_separator_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, EINA_FALSE);
   return it->is_separator;
}


/**
 * Sets the selected state of @p it.
 *
 * @param it The list item
 * @param selected Enables/disables the selected state
 *
 * @ingroup List
 */
EAPI void
elm_list_item_selected_set(Elm_List_Item *it, Eina_Bool selected)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   selected = !!selected;
   if (it->selected == selected) return;

   evas_object_ref(obj);
   _elm_list_walk(wd);

   if (selected)
     {
        if (!wd->multi)
          {
             while (wd->selected)
               _item_unselect(wd->selected->data);
          }
        _item_hilight(it);
        _item_select(it);
     }
   else
     _item_unselect(it);

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

/**
 * Gets the selected state of @p it.
 *
 * @param it The list item
 * @return If true, the item is selected
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_item_selected_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, EINA_FALSE);
   return it->selected;
}

/**
 * Brings @p it to the center of the list view.
 *
 * @param it The list item
 *
 * @ingroup List
 */
EAPI void
elm_list_item_show(Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   Evas_Coord bx, by, bw, bh;
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(wd->box, &bx, &by, &bw, &bh);
   evas_object_geometry_get(it->base.view, &x, &y, &w, &h);
   x -= bx;
   y -= by;
   if (wd->scr)
     elm_smart_scroller_child_region_show(wd->scr, x, y, w, h);
}

/**
 * Bring in the given item
 *
 * This causes list to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible. This may use animation to do so and take a
 * period of time
 *
 * @param it The item
 *
 * @ingroup List
 */
EAPI void
elm_list_item_bring_in(Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   Evas_Coord bx, by, bw, bh;
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(wd->box, &bx, &by, &bw, &bh);
   evas_object_geometry_get(it->base.view, &x, &y, &w, &h);
   x -= bx;
   y -= by;
   if (wd->scr)
     elm_smart_scroller_region_bring_in(wd->scr, x, y, w, h);
}

/**
 * Deletes item @p it from the list.
 *
 * @param it The list item to delete
 *
 * @ingroup List
 */
EAPI void
elm_list_item_del(Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   Evas_Object *obj = it->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (it->selected) _item_unselect(it);

   if (wd->walking > 0)
     {
        if (it->deleted) return;
        it->deleted = EINA_TRUE;
        wd->to_delete = eina_list_append(wd->to_delete, it);
        return;
     }

   wd->items = eina_list_remove_list(wd->items, it->node);

   evas_object_ref(obj);
   _elm_list_walk(wd);

   elm_widget_item_pre_notify_del(it);
   _elm_list_item_free(it);

   _elm_list_unwalk(wd);
   evas_object_unref(obj);
}

/**
 * Set the function called when a list item is freed.
 *
 * @param it The item to set the callback on
 * @param func The function called
 *
 * @ingroup List
 */
EAPI void
elm_list_item_del_cb_set(Elm_List_Item *it, Evas_Smart_Cb func)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   elm_widget_item_del_cb_set(it, func);
}

/**
 * Returns the data associated with the item.
 *
 * @param it The list item
 * @return The data associated with @p it
 *
 * @ingroup List
 */
EAPI void *
elm_list_item_data_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   return elm_widget_item_data_get(it);
}

/**
 * Returns the left side icon associated with the item.
 *
 * @param it The list item
 * @return The left side icon associated with @p it
 *
 * @ingroup List
 */
EAPI Evas_Object *
elm_list_item_icon_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   if (it->dummy_icon) return NULL;
   return it->icon;
}

/**
 * Sets the left side icon associated with the item.
 *
 * Once the icon object is set, a previously set one will be deleted.
 * You probably don't want, then, to have the <b>same</b> icon object set
 * for more than one item of the list.
 *
 * @param it The list item
 * @param icon The left side icon object to associate with @p it
 *
 * @ingroup List
 */
EAPI void
elm_list_item_icon_set(Elm_List_Item *it, Evas_Object *icon)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (it->icon == icon) return;
   if ((it->dummy_icon) && (!icon)) return;
   if (it->dummy_icon)
     {
        evas_object_del(it->icon);
        it->dummy_icon = EINA_FALSE;
     }
   if (!icon)
     {
        icon = evas_object_rectangle_add(evas_object_evas_get(it->base.widget));
        evas_object_color_set(icon, 0, 0, 0, 0);
        it->dummy_icon = EINA_TRUE;
     }
   if (it->icon)
     {
        evas_object_del(it->icon);
        it->icon = NULL;
     }
   it->icon = icon;
   if (it->base.view)
     edje_object_part_swallow(it->base.view, "elm.swallow.icon", icon);
}

/**
 * Gets the right side icon associated with the item.
 *
 * @param it The list item
 * @return The right side icon object associated with @p it
 *
 * @ingroup List
 */
EAPI Evas_Object *
elm_list_item_end_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   if (it->dummy_end) return NULL;
   return it->end;
}

/**
 * Sets the right side icon associated with the item.
 *
 * Once the icon object is set, a previously set one will be deleted.
 * You probably don't want, then, to have the <b>same</b> icon object set
 * for more than one item of the list.
 *
 * @param it The list item
 * @param icon The right side icon object to associate with @p it
 *
 * @ingroup List
 */
EAPI void
elm_list_item_end_set(Elm_List_Item *it, Evas_Object *end)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (it->end == end) return;
   if ((it->dummy_end) && (!end)) return;
   if (it->dummy_end)
     {
        evas_object_del(it->end);
        it->dummy_icon = EINA_FALSE;
     }
   if (!end)
     {
        end = evas_object_rectangle_add(evas_object_evas_get(it->base.widget));
        evas_object_color_set(end, 0, 0, 0, 0);
        it->dummy_end = EINA_TRUE;
     }
   if (it->end)
     {
        evas_object_del(it->end);
        it->end = NULL;
     }
   it->end = end;
   if (it->base.view)
     edje_object_part_swallow(it->base.view, "elm.swallow.end", end);
}

/**
 * Gets the base object of the item.
 *
 * @param it The list item
 * @return The base object associated with @p it
 *
 * @ingroup List
 */
EAPI Evas_Object *
elm_list_item_base_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   return it->base.view;
}

/**
 * Gets the label of the item.
 *
 * @param it The list item
 * @return The label of @p it
 *
 * @ingroup List
 */
EAPI const char *
elm_list_item_label_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   return it->label;
}

/**
 * Sets the label of the item.
 *
 * @param it The list item
 * @param text The label of @p it
 *
 * @ingroup List
 */
EAPI void
elm_list_item_label_set(Elm_List_Item *it, const char *text)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);
   if (!eina_stringshare_replace(&it->label, text)) return;
   if (it->base.view)
     edje_object_part_text_set(it->base.view, "elm.text", it->label);
}

/**
 * Gets the item before @p it in the list.
 *
 * @param it The list item
 * @return The item before @p it, or NULL on failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_prev(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   if (it->node->prev) return it->node->prev->data;
   else return NULL;
}

/**
 * Gets the item after @p it in the list.
 *
 * @param it The list item
 * @return The item after @p it, or NULL on failure
 *
 * @ingroup List
 */
EAPI Elm_List_Item *
elm_list_item_next(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, NULL);
   if (it->node->next) return it->node->next->data;
   else return NULL;
}

/**
 * Set the text to be shown in the list item.
 *
 * @param item Target item
 * @param text The text to set in the content
 *
 * Setup the text as tooltip to object. The item can have only one tooltip,
 * so any previous tooltip data is removed.
 *
 * @ingroup List
 */
EAPI void
elm_list_item_tooltip_text_set(Elm_List_Item *item, const char *text)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_tooltip_text_set(item, text);
}

/**
 * Set the content to be shown in the tooltip item
 *
 * Setup the tooltip to item. The item can have only one tooltip,
 * so any previous tooltip data is removed. @p func(with @p data) will
 * be called every time that need show the tooltip and it should
 * return a valid Evas_Object. This object is then managed fully by
 * tooltip system and is deleted when the tooltip is gone.
 *
 * @param item the list item being attached a tooltip.
 * @param func the function used to create the tooltip contents.
 * @param data what to provide to @a func as callback data/context.
 * @param del_cb called when data is not needed anymore, either when
 *        another callback replaces @func, the tooltip is unset with
 *        elm_list_item_tooltip_unset() or the owner @a item
 *        dies. This callback receives as the first parameter the
 *        given @a data, and @c event_info is the item.
 *
 * @ingroup List
 */
EAPI void
elm_list_item_tooltip_content_cb_set(Elm_List_Item *item, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_tooltip_content_cb_set(item, func, data, del_cb);
}

/**
 * Unset tooltip from item
 *
 * @param item list item to remove previously set tooltip.
 *
 * Remove tooltip from item. The callback provided as del_cb to
 * elm_list_item_tooltip_content_cb_set() will be called to notify
 * it is not used anymore.
 *
 * @see elm_list_item_tooltip_content_cb_set()
 *
 * @ingroup List
 */
EAPI void
elm_list_item_tooltip_unset(Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_tooltip_unset(item);
}

/**
 * Sets a different style for this item tooltip.
 *
 * @note before you set a style you should define a tooltip with
 *       elm_list_item_tooltip_content_cb_set() or
 *       elm_list_item_tooltip_text_set()
 *
 * @param item list item with tooltip already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup List
 */
EAPI void
elm_list_item_tooltip_style_set(Elm_List_Item *item, const char *style)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_tooltip_style_set(item, style);
}

/**
 * Get the style for this item tooltip.
 *
 * @param item list item with tooltip already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a tooltip set, then NULL is returned.
 *
 * @ingroup List
 */
EAPI const char *
elm_list_item_tooltip_style_get(const Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item, NULL);
   return elm_widget_item_tooltip_style_get(item);
}

/**
 * Set the cursor to be shown when mouse is over the list item
 *
 * @param item Target item
 * @param cursor the cursor name to be used.
 *
 * @see elm_object_cursor_set()
 * @ingroup List
 */
EAPI void
elm_list_item_cursor_set(Elm_List_Item *item, const char *cursor)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_cursor_set(item, cursor);
}

/**
 * Get the cursor to be shown when mouse is over the list item
 *
 * @param item list item with cursor already set.
 * @return the cursor name.
 *
 * @ingroup List
 */
EAPI const char *
elm_list_item_cursor_get(const Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item, NULL);
   return elm_widget_item_cursor_get(item);
}

/**
 * Unset the cursor to be shown when mouse is over the list item
 *
 * @param item Target item
 *
 * @see elm_object_cursor_unset()
 * @ingroup List
 */
EAPI void
elm_list_item_cursor_unset(Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_cursor_unset(item);
}

/**
 * Sets a different style for this item cursor.
 *
 * @note before you set a style you should define a cursor with
 *       elm_list_item_cursor_set()
 *
 * @param item list item with cursor already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup List
 */
EAPI void
elm_list_item_cursor_style_set(Elm_List_Item *item, const char *style)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_cursor_style_set(item, style);
}

/**
 * Get the style for this item cursor.
 *
 * @param item list item with cursor already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a cursor set, then NULL is returned.
 *
 * @ingroup List
 */
EAPI const char *
elm_list_item_cursor_style_get(const Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item, NULL);
   return elm_widget_item_cursor_style_get(item);
}

/**
 * Set if the cursor set should be searched on the theme or should use
 * the provided by the engine, only.
 *
 * @note before you set if should look on theme you should define a cursor
 * with elm_object_cursor_set(). By default it will only look for cursors
 * provided by the engine.
 *
 * @param item widget item with cursor already set.
 * @param engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well.
 *
 * @ingroup List
 */
EAPI void
elm_list_item_cursor_engine_only_set(Elm_List_Item *item, Eina_Bool engine_only)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item);
   elm_widget_item_cursor_engine_only_set(item, engine_only);
}

/**
 * Get the cursor engine only usage for this item cursor.
 *
 * @param item widget item with cursor already set.
 * @return engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well. If
 *         the object does not have a cursor set, then EINA_FALSE is returned.
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_item_cursor_engine_only_get(const Elm_List_Item *item)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(item, EINA_FALSE);
   return elm_widget_item_cursor_engine_only_get(item);
}

/**
 * Set bounce mode
 *
 * This will enable or disable the scroller bounce mode for the list. See
 * elm_scroller_bounce_set() for details
 *
 * @param obj The list object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup List
 */
EAPI void
elm_list_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scr)
     elm_smart_scroller_bounce_allow_set(wd->scr, h_bounce, v_bounce);
}

/**
 * Get the bounce mode
 *
 * @param obj The List object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup List
 */
EAPI void
elm_list_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_get(wd->scr, h_bounce, v_bounce);
}

/**
 * Set the scrollbar policy
 *
 * This sets the scrollbar visibility policy for the given list scroller.
 * ELM_SMART_SCROLLER_POLICY_AUTO means the scrollber is made visible if it
 * is needed, and otherwise kept hidden. ELM_SMART_SCROLLER_POLICY_ON turns
 * it on all the time, and ELM_SMART_SCROLLER_POLICY_OFF always keeps it off.
 * This applies respectively for the horizontal and vertical scrollbars.
 *
 * @param obj The list object
 * @param policy_h Horizontal scrollbar policy
 * @param policy_v Vertical scrollbar policy
 *
 * @ingroup List
 */
EAPI void
elm_list_scroller_policy_set(Evas_Object *obj, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((policy_h >= ELM_SCROLLER_POLICY_LAST) ||
       (policy_v >= ELM_SCROLLER_POLICY_LAST))
     if (wd->scr)
       elm_smart_scroller_policy_set(wd->scr, policy_h, policy_v);
}

EAPI void
elm_list_scroller_policy_get(const Evas_Object *obj, Elm_Scroller_Policy *policy_h, Elm_Scroller_Policy *policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Smart_Scroller_Policy s_policy_h, s_policy_v;
   if ((!wd) || (!wd->scr)) return;
   elm_smart_scroller_policy_get(wd->scr, &s_policy_h, &s_policy_v);
   if (policy_h) *policy_h = (Elm_Scroller_Policy) s_policy_h;
   if (policy_v) *policy_v = (Elm_Scroller_Policy) s_policy_v;
}

/**
 * Sets the disabled/enabled state of a list item.
 *
 * A disabled item cannot be selected or unselected. It will also
 * change its appearance (generally greyed out). This sets the
 * disabled state (@c EINA_TRUE for disabled, @c EINA_FALSE for
 * enabled).
 *
 * @param it The item
 * @param disabled The disabled state
 *
 * @ingroup List
 */
EAPI void
elm_list_item_disabled_set(Elm_List_Item *it, Eina_Bool disabled)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it);

   if (it->disabled == disabled)
     return;

   it->disabled = !!disabled;

   if (it->disabled)
     edje_object_signal_emit(it->base.view, "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(it->base.view, "elm,state,enabled", "elm");
}

/**
 * Get the disabled/enabled state of a list item
 *
 * @param it The item
 * @return The disabled state
 *
 * See elm_list_item_disabled_set().
 *
 * @ingroup List
 */
EAPI Eina_Bool
elm_list_item_disabled_get(const Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_DELETED_RETURN(it, EINA_FALSE);

   return it->disabled;
}
