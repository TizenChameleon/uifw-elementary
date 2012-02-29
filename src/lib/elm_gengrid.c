#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"
#include "els_scroller.h"
#include "elm_gen_common.h"

/* --
 * TODO:
 * Handle non-homogeneous objects too.
 */

#define PRELOAD 1
#define REORDER_EFFECT_TIME 0.5

#define ELM_GEN_SETUP(wd) \
   (wd)->calc_cb = (Ecore_Cb)_calc_job

#define ELM_GEN_ITEM_SETUP(it) \
   (it)->del_cb = (Ecore_Cb)_item_del; \
   (it)->highlight_cb = (Ecore_Cb)_item_highlight; \
   (it)->unsel_cb = (Ecore_Cb)_item_unselect; \
   (it)->unrealize_cb = (Ecore_Cb)_item_unrealize_cb

#define ELM_GENGRID_CHECK_ITC_VER(itc) \
   do \
     { \
        if (!itc) \
          { \
             ERR("Gengrid_Item_Class(itc) is NULL"); \
             return; \
          } \
        if (itc->version != ELM_GENGRID_ITEM_CLASS_VERSION) \
          { \
             ERR("Gengrid_Item_Class version mismatched! required = (%d), current  = (%d)", itc->version, ELM_GENGRID_ITEM_CLASS_VERSION); \
             return; \
          } \
     } \
   while(0)


struct Elm_Gen_Item_Type
{
   Elm_Gen_Item   *it;
   Ecore_Animator *item_moving_effect_timer;
   Evas_Coord   gx, gy, ox, oy, tx, ty, rx, ry;
   unsigned int moving_effect_start_time;
   int          prev_group;

   Eina_Bool   group_realized : 1;
   Eina_Bool   moving : 1;
};

#if 0
struct _Widget_Data
{
   Eina_Inlist_Sorted_State *state;
   Evas_Object      *obj; /* the gengrid object */
   Evas_Object      *scr; /* a smart scroller object which is used internally in genlist */
   Evas_Object      *pan_smart; /* "elm_genlist_pan" evas smart object. this is an extern pan of smart scroller(scr). */
   Eina_List        *selected;
   Eina_List        *group_items; /* list of groups index items */
   Eina_Inlist      *items; /* inlist of all items */
   Elm_Gen_Item     *reorder_it; /* item currently being repositioned */
   Elm_Gen_Item     *last_selected_item;
   Pan              *pan; /* pan_smart object's smart data */
   Ecore_Job        *calc_job;
   int               walking;
   int               item_width, item_height;
   int               group_item_width, group_item_height;
   int               minw, minh;
   long              count;
   Evas_Coord        pan_x, pan_y;
   Eina_Bool         reorder_mode : 1;
   Eina_Bool         on_hold : 1;
   Eina_Bool         multi : 1;
   Eina_Bool         no_select : 1;
   Eina_Bool         wasselected : 1;
   Eina_Bool         always_select : 1;
   Eina_Bool         clear_me : 1;
   Eina_Bool         h_bounce : 1;
   Eina_Bool         v_bounce : 1;
   Ecore_Cb          del_cb, calc_cb, sizing_cb;
   Ecore_Cb          clear_cb;
   ////////////////////////////////////
   double            align_x, align_y;

   Evas_Coord        old_pan_x, old_pan_y;
   Evas_Coord        reorder_item_x, reorder_item_y;
   unsigned int      nmax;
   long              items_lost;

   int               generation;

   Eina_Bool         horizontal : 1;
   Eina_Bool         longpressed : 1;
   Eina_Bool         reorder_item_changed : 1;
   Eina_Bool         move_effect_enabled : 1;
};
#endif

static const char *widtype = NULL;
static void      _item_highlight(Elm_Gen_Item *it);
static void      _item_unrealize_cb(Elm_Gen_Item *it);
static void      _item_unselect(Elm_Gen_Item *it);
static void      _calc_job(void *data);
static void      _on_focus_hook(void        *data,
                                Evas_Object *obj);
static Eina_Bool _item_multi_select_up(Widget_Data *wd);
static Eina_Bool _item_multi_select_down(Widget_Data *wd);
static Eina_Bool _item_multi_select_left(Widget_Data *wd);
static Eina_Bool _item_multi_select_right(Widget_Data *wd);
static Eina_Bool _item_single_select_up(Widget_Data *wd);
static Eina_Bool _item_single_select_down(Widget_Data *wd);
static Eina_Bool _item_single_select_left(Widget_Data *wd);
static Eina_Bool _item_single_select_right(Widget_Data *wd);
static Eina_Bool _event_hook(Evas_Object       *obj,
                             Evas_Object       *src,
                             Evas_Callback_Type type,
                             void              *event_info);
static Eina_Bool _deselect_all_items(Widget_Data *wd);

static Evas_Smart_Class _pan_sc = EVAS_SMART_CLASS_INIT_VERSION;
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);

static const char SIG_ACTIVATED[] = "activated";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_SELECTED[] = "selected";
static const char SIG_UNSELECTED[] = "unselected";
static const char SIG_REALIZED[] = "realized";
static const char SIG_UNREALIZED[] = "unrealized";
static const char SIG_CHANGED[] = "changed";
static const char SIG_DRAG_START_UP[] = "drag,start,up";
static const char SIG_DRAG_START_DOWN[] = "drag,start,down";
static const char SIG_DRAG_START_LEFT[] = "drag,start,left";
static const char SIG_DRAG_START_RIGHT[] = "drag,start,right";
static const char SIG_DRAG_STOP[] = "drag,stop";
static const char SIG_DRAG[] = "drag";
static const char SIG_SCROLL[] = "scroll";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_EDGE_TOP[] = "edge,top";
static const char SIG_EDGE_BOTTOM[] = "edge,bottom";
static const char SIG_EDGE_LEFT[] = "edge,left";
static const char SIG_EDGE_RIGHT[] = "edge,right";
static const char SIG_MOVED[] = "moved";

static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_ACTIVATED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_SELECTED, ""},
   {SIG_UNSELECTED, ""},
   {SIG_REALIZED, ""},
   {SIG_UNREALIZED, ""},
   {SIG_CHANGED, ""},
   {SIG_DRAG_START_UP, ""},
   {SIG_DRAG_START_DOWN, ""},
   {SIG_DRAG_START_LEFT, ""},
   {SIG_DRAG_START_RIGHT, ""},
   {SIG_DRAG_STOP, ""},
   {SIG_DRAG, ""},
   {SIG_SCROLL, ""},
   {SIG_SCROLL_ANIM_START, ""},
   {SIG_SCROLL_ANIM_STOP, ""},
   {SIG_SCROLL_DRAG_START, ""},
   {SIG_SCROLL_DRAG_STOP, ""},
   {SIG_EDGE_TOP, ""},
   {SIG_EDGE_BOTTOM, ""},
   {SIG_EDGE_LEFT, ""},
   {SIG_EDGE_RIGHT, ""},
   {SIG_MOVED, ""},
   {NULL, NULL}
};

static Eina_Bool
_event_hook(Evas_Object        *obj,
            Evas_Object        *src __UNUSED__,
            Evas_Callback_Type  type,
            void               *event_info)
{
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (!wd->items) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   Elm_Object_Item *it = NULL;
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

   if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")))
     {
        if ((wd->horizontal) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_up(wd)))
             || (_item_single_select_up(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else if ((!wd->horizontal) &&
                 (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                   (_item_multi_select_left(wd)))
                  || (_item_single_select_left(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          x -= step_x;
     }
   else if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")))
     {
        if ((wd->horizontal) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_down(wd)))
             || (_item_single_select_down(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else if ((!wd->horizontal) &&
                 (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                   (_item_multi_select_right(wd)))
                  || (_item_single_select_right(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else
          x += step_x;
     }
   else if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")))
     {
        if ((wd->horizontal) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_left(wd)))
             || (_item_single_select_left(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else if ((!wd->horizontal) &&
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
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
        if ((wd->horizontal) &&
            (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
              (_item_multi_select_right(wd)))
             || (_item_single_select_right(wd))))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else if ((!wd->horizontal) &&
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
   else if ((!strcmp(ev->keyname, "Home")) || (!strcmp(ev->keyname, "KP_Home")))
     {
        it = elm_gengrid_first_item_get(obj);
        elm_gengrid_item_bring_in(it);
        elm_gengrid_item_selected_set(it, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "End")) || (!strcmp(ev->keyname, "KP_End")))
     {
        it = elm_gengrid_last_item_get(obj);
        elm_gengrid_item_bring_in(it);
        elm_gengrid_item_selected_set(it, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Prior")) || (!strcmp(ev->keyname, "KP_Prior")))
     {
        if (wd->horizontal)
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
   else if ((!strcmp(ev->keyname, "Next")) || (!strcmp(ev->keyname, "KP_Next")))
     {
        if (wd->horizontal)
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
   else if (((!strcmp(ev->keyname, "Return")) ||
             (!strcmp(ev->keyname, "KP_Enter")) ||
             (!strcmp(ev->keyname, "space")))
            && (!wd->multi) && (wd->selected))
     {
        it = elm_gengrid_selected_item_get(obj);
        evas_object_smart_callback_call(WIDGET(it), SIG_ACTIVATED, it);
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
     elm_gengrid_item_selected_set((Elm_Object_Item *) wd->selected->data,
                                   EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_left(Widget_Data *wd)
{
   if (!wd->selected) return EINA_FALSE;

   Elm_Object_Item *prev =
      elm_gengrid_item_prev_get(wd->last_selected_item);
   if (!prev) return EINA_TRUE;
   if (elm_gengrid_item_selected_get(prev))
     {
        elm_gengrid_item_selected_set(wd->last_selected_item, EINA_FALSE);
        wd->last_selected_item = prev;
        elm_gengrid_item_show(wd->last_selected_item);
     }
   else
     {
        elm_gengrid_item_selected_set(prev, EINA_TRUE);
        elm_gengrid_item_show(prev);
     }

   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_right(Widget_Data *wd)
{
   if (!wd->selected) return EINA_FALSE;

   Elm_Object_Item *next =
      elm_gengrid_item_next_get(wd->last_selected_item);
   if (!next) return EINA_TRUE;
   if (elm_gengrid_item_selected_get(next))
     {
        elm_gengrid_item_selected_set(wd->last_selected_item, EINA_FALSE);
        wd->last_selected_item = next;
        elm_gengrid_item_show(wd->last_selected_item);
     }
   else
     {
        elm_gengrid_item_selected_set(next, EINA_TRUE);
        elm_gengrid_item_show(next);
     }

   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_up(Widget_Data *wd)
{
   unsigned int i;
   Eina_Bool r = EINA_TRUE;

   if (!wd->selected) return EINA_FALSE;

   for (i = 0; (r) && (i < wd->nmax); i++)
     r &= _item_multi_select_left(wd);

   return r;
}

static Eina_Bool
_item_multi_select_down(Widget_Data *wd)
{
   unsigned int i;
   Eina_Bool r = EINA_TRUE;

   if (!wd->selected) return EINA_FALSE;

   for (i = 0; (r) && (i < wd->nmax); i++)
     r &= _item_multi_select_right(wd);

   return r;
}

static Eina_Bool
_item_single_select_up(Widget_Data *wd)
{
   unsigned int i;

   Elm_Gen_Item *prev;

   if (!wd->selected)
     {
        prev = ELM_GEN_ITEM_FROM_INLIST(wd->items->last);
        while ((prev) && (prev->generation < wd->generation))
          prev = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(prev)->prev);
        elm_gengrid_item_selected_set((Elm_Object_Item *) prev, EINA_TRUE);
        elm_gengrid_item_show((Elm_Object_Item *) prev);
        return EINA_TRUE;
     }
   else
     prev = (Elm_Gen_Item *) elm_gengrid_item_prev_get(wd->last_selected_item);

   if (!prev) return EINA_FALSE;

   for (i = 1; i < wd->nmax; i++)
     {
        Elm_Object_Item *tmp =
           elm_gengrid_item_prev_get((Elm_Object_Item *) prev);
        if (!tmp) return EINA_FALSE;
        prev = (Elm_Gen_Item *) tmp;
     }

   _deselect_all_items(wd);

   elm_gengrid_item_selected_set((Elm_Object_Item *) prev, EINA_TRUE);
   elm_gengrid_item_show((Elm_Object_Item *) prev);
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_down(Widget_Data *wd)
{
   unsigned int i;

   Elm_Gen_Item *next;

   if (!wd->selected)
     {
        next = ELM_GEN_ITEM_FROM_INLIST(wd->items);
        while ((next) && (next->generation < wd->generation))
          next = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(next)->next);
        elm_gengrid_item_selected_set((Elm_Object_Item *) next, EINA_TRUE);
        elm_gengrid_item_show((Elm_Object_Item *) next);
        return EINA_TRUE;
     }
   else
     next = (Elm_Gen_Item *) elm_gengrid_item_next_get(wd->last_selected_item);

   if (!next) return EINA_FALSE;

   for (i = 1; i < wd->nmax; i++)
     {
        Elm_Object_Item *tmp =
           elm_gengrid_item_next_get((Elm_Object_Item *) next);
        if (!tmp) return EINA_FALSE;
        next = (Elm_Gen_Item *) tmp;
     }

   _deselect_all_items(wd);

   elm_gengrid_item_selected_set((Elm_Object_Item *) next, EINA_TRUE);
   elm_gengrid_item_show((Elm_Object_Item *) next);
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_left(Widget_Data *wd)
{
   Elm_Gen_Item *prev;
   if (!wd->selected)
     {
        prev = ELM_GEN_ITEM_FROM_INLIST(wd->items->last);
        while ((prev) && (prev->generation < wd->generation))
          prev = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(prev)->prev);
     }
   else
     prev = (Elm_Gen_Item *) elm_gengrid_item_prev_get(wd->last_selected_item);

   if (!prev) return EINA_FALSE;

   _deselect_all_items(wd);

   elm_gengrid_item_selected_set((Elm_Object_Item *) prev, EINA_TRUE);
   elm_gengrid_item_show((Elm_Object_Item *) prev);
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_right(Widget_Data *wd)
{
   Elm_Gen_Item *next;
   if (!wd->selected)
     {
        next = ELM_GEN_ITEM_FROM_INLIST(wd->items);
        while ((next) && (next->generation < wd->generation))
          next = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(next)->next);
     }
   else
     next = (Elm_Gen_Item *) elm_gengrid_item_next_get(wd->last_selected_item);

   if (!next) return EINA_FALSE;

   _deselect_all_items(wd);

   elm_gengrid_item_selected_set((Elm_Object_Item *) next, EINA_TRUE);
   elm_gengrid_item_show((Elm_Object_Item *) next);
   return EINA_TRUE;
}

static void
_on_focus_hook(void *data   __UNUSED__,
               Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->obj, "elm,action,focus", "elm");
        evas_object_focus_set(wd->obj, EINA_TRUE);
        if ((wd->selected) && (!wd->last_selected_item))
          wd->last_selected_item = eina_list_data_get(wd->selected);
     }
   else
     {
        edje_object_signal_emit(wd->obj, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->obj, EINA_FALSE);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Gen_Item *it;
   if (!wd) return;
   elm_smart_scroller_mirrored_set(wd->scr, rtl);
   if (!wd->items) return;
   it = ELM_GEN_ITEM_FROM_INLIST(wd->items);

   while (it)
     {
        edje_object_mirrored_set(VIEW(it), rtl);
        elm_gengrid_item_update((Elm_Object_Item *) it);
        it = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
     }
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   elm_smart_scroller_object_theme_set(obj, wd->scr, "gengrid", "base",
                                       elm_widget_style_get(obj));
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_gengrid_clear(obj);
   evas_object_del(wd->pan_smart);
   wd->pan_smart = NULL;
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   free(wd);
}

static void
_signal_emit_hook(Evas_Object *obj,
                  const char  *emission,
                  const char  *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           emission, source);
}

static void
_signal_callback_add_hook(Evas_Object *obj,
                          const char  *emission,
                          const char  *source,
                          Edje_Signal_Cb func_cb,
                          void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_callback_add(elm_smart_scroller_edje_object_get(wd->scr),
                                   emission, source, func_cb, data);
}

static void
_signal_callback_del_hook(Evas_Object *obj,
                          const char  *emission,
                          const char  *source,
                          Edje_Signal_Cb func_cb,
                          void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_callback_del_full(elm_smart_scroller_edje_object_get(wd->scr),
                                        emission, source, func_cb, data);
}

static void
_mouse_move(void        *data,
            Evas *evas   __UNUSED__,
            Evas_Object *obj,
            void        *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord minw = 0, minh = 0, x, y, dx, dy, adx, ady;
   Evas_Coord ox, oy, ow, oh, it_scrl_x, it_scrl_y;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (!it->wd->on_hold)
          {
             it->wd->on_hold = EINA_TRUE;
             if (!it->wd->wasselected)
               _item_unselect(it);
          }
     }
   if ((it->dragging) && (it->down))
     {
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        evas_object_smart_callback_call(WIDGET(it), SIG_DRAG, it);
        return;
     }
   if ((!it->down) || (it->wd->longpressed))
     {
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        if ((it->wd->reorder_mode) && (it->wd->reorder_it))
          {
             evas_object_geometry_get(it->wd->pan_smart, &ox, &oy, &ow, &oh);

             it_scrl_x = ev->cur.canvas.x - it->wd->reorder_it->dx;
             it_scrl_y = ev->cur.canvas.y - it->wd->reorder_it->dy;

             if (it_scrl_x < ox) it->wd->reorder_item_x = ox;
             else if (it_scrl_x + it->wd->item_width > ox + ow)
               it->wd->reorder_item_x = ox + ow - it->wd->item_width;
             else it->wd->reorder_item_x = it_scrl_x;

             if (it_scrl_y < oy) it->wd->reorder_item_y = oy;
             else if (it_scrl_y + it->wd->item_height > oy + oh)
               it->wd->reorder_item_y = oy + oh - it->wd->item_height;
             else it->wd->reorder_item_y = it_scrl_y;

             if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
             it->wd->calc_job = ecore_job_add(_calc_job, it->wd);
          }
        return;
     }
   if (!it->display_only)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   x = ev->cur.canvas.x - x;
   y = ev->cur.canvas.y - y;
   dx = x - it->dx;
   adx = dx;
   if (adx < 0) adx = -dx;
   dy = y - it->dy;
   ady = dy;
   if (ady < 0) ady = -dy;
   minw /= 2;
   minh /= 2;
   if ((adx > minw) || (ady > minh))
     {
        const char *left_drag, *right_drag;
        if (!elm_widget_mirrored_get(WIDGET(it)))
          {
             left_drag = SIG_DRAG_START_LEFT;
             right_drag = SIG_DRAG_START_RIGHT;
          }
        else
          {
             left_drag = SIG_DRAG_START_RIGHT;
             right_drag = SIG_DRAG_START_LEFT;
          }

        it->dragging = 1;
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        if (!it->wd->wasselected)
          _item_unselect(it);
        if (dy < 0)
          {
             if (ady > adx)
               evas_object_smart_callback_call(WIDGET(it), SIG_DRAG_START_UP,
                                               it);
             else
               {
                  if (dx < 0)
                    evas_object_smart_callback_call(WIDGET(it),
                                                    left_drag, it);
               }
          }
        else
          {
             if (ady > adx)
               evas_object_smart_callback_call(WIDGET(it),
                                               SIG_DRAG_START_DOWN, it);
             else
               {
                  if (dx < 0)
                    evas_object_smart_callback_call(WIDGET(it),
                                                    left_drag, it);
                  else
                    evas_object_smart_callback_call(WIDGET(it),
                                                    right_drag, it);
               }
          }
     }
}

static Eina_Bool
_long_press(void *data)
{
   Elm_Gen_Item *it = data;

   it->long_timer = NULL;
   if (elm_widget_item_disabled_get(it)|| (it->dragging))
     return ECORE_CALLBACK_CANCEL;
   it->wd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, it);
   if (it->wd->reorder_mode)
     {
        it->wd->reorder_it = it;
        evas_object_raise(VIEW(it));
        elm_smart_scroller_hold_set(it->wd->scr, EINA_TRUE);
        elm_smart_scroller_bounce_allow_set(it->wd->scr, EINA_FALSE, EINA_FALSE);
        edje_object_signal_emit(VIEW(it), "elm,state,reorder,enabled", "elm");
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(void        *data,
            Evas *evas   __UNUSED__,
            Evas_Object *obj,
            void        *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord x, y;

   if (ev->button != 1) return;
   it->down = 1;
   it->dragging = 0;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   it->dx = ev->canvas.x - x;
   it->dy = ev->canvas.y - y;
   it->wd->longpressed = EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) it->wd->on_hold = EINA_TRUE;
   else it->wd->on_hold = EINA_FALSE;
   if (it->wd->on_hold) return;
   it->wd->wasselected = it->selected;
   _item_highlight(it);
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_DOUBLE, it);
        evas_object_smart_callback_call(WIDGET(it), SIG_ACTIVATED, it);
     }
   if (it->long_timer) ecore_timer_del(it->long_timer);
   if (it->realized)
     it->long_timer = ecore_timer_add(_elm_config->longpress_timeout,
                                        _long_press, it);
   else
     it->long_timer = NULL;
}

static void
_mouse_up(void            *data,
          Evas *evas       __UNUSED__,
          Evas_Object *obj __UNUSED__,
          void            *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Eina_Bool dragged = EINA_FALSE;

   if (ev->button != 1) return;
   it->down = EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) it->wd->on_hold = EINA_TRUE;
   else it->wd->on_hold = EINA_FALSE;
   if (it->long_timer)
     {
        ecore_timer_del(it->long_timer);
        it->long_timer = NULL;
     }
   if (it->dragging)
     {
        it->dragging = EINA_FALSE;
        evas_object_smart_callback_call(WIDGET(it), SIG_DRAG_STOP, it);
        dragged = EINA_TRUE;
     }
   if (it->wd->on_hold)
     {
        it->wd->longpressed = EINA_FALSE;
        it->wd->on_hold = EINA_FALSE;
        return;
     }
   if ((it->wd->reorder_mode) && (it->wd->reorder_it))
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_MOVED, it->wd->reorder_it);
        it->wd->reorder_it = NULL;
        it->wd->move_effect_enabled = EINA_FALSE;
        if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
          it->wd->calc_job = ecore_job_add(_calc_job, it->wd);

        elm_smart_scroller_hold_set(it->wd->scr, EINA_FALSE);
        elm_smart_scroller_bounce_allow_set(it->wd->scr, it->wd->h_bounce, it->wd->v_bounce);
        edje_object_signal_emit(VIEW(it), "elm,state,reorder,disabled", "elm");
     }
   if (it->wd->longpressed)
     {
        it->wd->longpressed = EINA_FALSE;
        if (!it->wd->wasselected) _item_unselect(it);
        it->wd->wasselected = EINA_FALSE;
        return;
     }
   if (dragged)
     {
        if (it->want_unrealize)
          _elm_genlist_item_unrealize(it, EINA_FALSE);
     }
   if (elm_widget_item_disabled_get(it) || (dragged)) return;
   if (it->wd->multi)
     {
        if (!it->selected)
          {
             _item_highlight(it);
             it->sel_cb(it);
          }
        else _item_unselect(it);
     }
   else
     {
        if (!it->selected)
          {
             while (it->wd->selected)
               _item_unselect(it->wd->selected->data);
          }
        else
          {
             const Eina_List *l, *l_next;
             Elm_Gen_Item *item2;

             EINA_LIST_FOREACH_SAFE(it->wd->selected, l, l_next, item2)
                if (item2 != it) _item_unselect(item2);
          }
        _item_highlight(it);
        it->sel_cb(it);
     }
}

static void
_item_highlight(Elm_Gen_Item *it)
{
   if ((it->wd->no_select) || (it->generation < it->wd->generation) || (it->highlighted)) return;
   edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");
   it->highlighted = EINA_TRUE;
}

static void
_item_realize(Elm_Gen_Item *it)
{
   char buf[1024];
   char style[1024];

   if ((it->realized) || (it->generation < it->wd->generation)) return;
   VIEW(it) = edje_object_add(evas_object_evas_get(WIDGET(it)));
   edje_object_scale_set(VIEW(it), elm_widget_scale_get(WIDGET(it)) *
                         _elm_config->scale);
   edje_object_mirrored_set(VIEW(it), elm_widget_mirrored_get(WIDGET(it)));
   evas_object_smart_member_add(VIEW(it), it->wd->pan_smart);
   elm_widget_sub_object_add(WIDGET(it), VIEW(it));
   snprintf(style, sizeof(style), "item/%s",
            it->itc->item_style ? it->itc->item_style : "default");
   _elm_theme_object_set(WIDGET(it), VIEW(it), "gengrid", style,
                         elm_widget_style_get(WIDGET(it)));
   it->spacer =
      evas_object_rectangle_add(evas_object_evas_get(WIDGET(it)));
   evas_object_color_set(it->spacer, 0, 0, 0, 0);
   elm_widget_sub_object_add(WIDGET(it), it->spacer);
   evas_object_size_hint_min_set(it->spacer, 2 * _elm_config->scale, 1);
   edje_object_part_swallow(VIEW(it), "elm.swallow.pad", it->spacer);

   if (it->itc->func.text_get)
     {
        const Eina_List *l;
        const char *key;

        it->texts =
           elm_widget_stringlist_get(edje_object_data_get(VIEW(it),
                                                          "texts"));
        EINA_LIST_FOREACH(it->texts, l, key)
          {
             char *s = it->itc->func.text_get
                ((void *)it->base.data, WIDGET(it), key);
             if (s)
               {
                  edje_object_part_text_set(VIEW(it), key, s);
                  free(s);
               }
          }
     }

   if (it->itc->func.content_get)
     {
        const Eina_List *l;
        const char *key;
        Evas_Object *ic = NULL;

        it->contents =
           elm_widget_stringlist_get(edje_object_data_get(VIEW(it),
                                                          "contents"));
        EINA_LIST_FOREACH(it->contents, l, key)
          {
             if (it->itc->func.content_get)
               ic = it->itc->func.content_get
                  ((void *)it->base.data, WIDGET(it), key);
             if (ic)
               {
                  it->content_objs = eina_list_append(it->content_objs, ic);
                  edje_object_part_swallow(VIEW(it), key, ic);
                  evas_object_show(ic);
                  elm_widget_sub_object_add(WIDGET(it), ic);
               }
          }
     }

   if (it->itc->func.state_get)
     {
        const Eina_List *l;
        const char *key;

        it->states =
           elm_widget_stringlist_get(edje_object_data_get(VIEW(it),
                                                          "states"));
        EINA_LIST_FOREACH(it->states, l, key)
          {
             Eina_Bool on = it->itc->func.state_get
                ((void *)it->base.data, WIDGET(it), l->data);
             if (on)
               {
                  snprintf(buf, sizeof(buf), "elm,state,%s,active", key);
                  edje_object_signal_emit(VIEW(it), buf, "elm");
               }
          }
     }

   if (it->group)
     {
        if ((!it->wd->group_item_width) && (!it->wd->group_item_height))
          {
             edje_object_size_min_restricted_calc(VIEW(it),
                                                  &it->wd->group_item_width,
                                                  &it->wd->group_item_height,
                                                  it->wd->group_item_width,
                                                  it->wd->group_item_height);
          }
     }
   else
     {
        if ((!it->wd->item_width) && (!it->wd->item_height))
          {
             edje_object_size_min_restricted_calc(VIEW(it),
                                                  &it->wd->item_width,
                                                  &it->wd->item_height,
                                                  it->wd->item_width,
                                                  it->wd->item_height);
             elm_coords_finger_size_adjust(1, &it->wd->item_width,
                                           1, &it->wd->item_height);
          }

        evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_DOWN,
                                       _mouse_down, it);
        evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_UP,
                                       _mouse_up, it);
        evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_MOVE,
                                       _mouse_move, it);

        if (it->selected)
          edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");
        if (elm_widget_item_disabled_get(it))
          edje_object_signal_emit(VIEW(it), "elm,state,disabled", "elm");
     }
   evas_object_show(VIEW(it));

   if (it->tooltip.content_cb)
     {
        elm_widget_item_tooltip_content_cb_set(it,
                                               it->tooltip.content_cb,
                                               it->tooltip.data, NULL);
        elm_widget_item_tooltip_style_set(it, it->tooltip.style);
        elm_widget_item_tooltip_window_mode_set(it, it->tooltip.free_size);
     }

   if (it->mouse_cursor)
     elm_widget_item_cursor_set(it, it->mouse_cursor);

   it->realized = EINA_TRUE;
   it->want_unrealize = EINA_FALSE;
}

static void
_item_unrealize_cb(Elm_Gen_Item *it)
{
   evas_object_del(VIEW(it));
   VIEW(it) = NULL;
   evas_object_del(it->spacer);
   it->spacer = NULL;
}

static Eina_Bool
_reorder_item_moving_effect_timer_cb(void *data)
{
   Elm_Gen_Item *it = data;
   double tt, t;
   Evas_Coord dx, dy;

   tt = REORDER_EFFECT_TIME;
   t = ((0.0 > (t = ecore_loop_time_get()-it->item->moving_effect_start_time)) ? 0.0 : t);
   dx = ((it->item->tx - it->item->ox) / 10) * _elm_config->scale;
   dy = ((it->item->ty - it->item->oy) / 10) * _elm_config->scale;

   if (t <= tt)
     {
        it->item->rx += (1 * sin((t / tt) * (M_PI / 2)) * dx);
        it->item->ry += (1 * sin((t / tt) * (M_PI / 2)) * dy);
     }
   else
     {
        it->item->rx += dx;
        it->item->ry += dy;
     }

   if ((((dx > 0) && (it->item->rx >= it->item->tx)) || ((dx <= 0) && (it->item->rx <= it->item->tx))) &&
       (((dy > 0) && (it->item->ry >= it->item->ty)) || ((dy <= 0) && (it->item->ry <= it->item->ty))))
     {
        evas_object_move(VIEW(it), it->item->tx, it->item->ty);
        if (it->group)
          {
             Evas_Coord vw, vh;
             evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &vw, &vh);
             if (it->wd->horizontal)
               evas_object_resize(VIEW(it), it->wd->group_item_width, vh);
             else
               evas_object_resize(VIEW(it), vw, it->wd->group_item_height);
          }
        else
          evas_object_resize(VIEW(it), it->wd->item_width, it->wd->item_height);
        it->item->moving = EINA_FALSE;
        it->item->item_moving_effect_timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   evas_object_move(VIEW(it), it->item->rx, it->item->ry);
   if (it->group)
     {
        Evas_Coord vw, vh;
        evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &vw, &vh);
        if (it->wd->horizontal)
          evas_object_resize(VIEW(it), it->wd->group_item_width, vh);
        else
          evas_object_resize(VIEW(it), vw, it->wd->group_item_height);
     }
   else
     evas_object_resize(VIEW(it), it->wd->item_width, it->wd->item_height);

   return ECORE_CALLBACK_RENEW;
}

static void
_group_item_place(Pan *sd)
{
   Evas_Coord iw, ih, vw, vh;
   Eina_List *l;
   Eina_Bool was_realized;
   Elm_Gen_Item *it;
   evas_object_geometry_get(sd->wd->pan_smart, NULL, NULL, &vw, &vh);
   if (sd->wd->horizontal)
     {
        iw = sd->wd->group_item_width;
        ih = vh;
     }
   else
     {
        iw = vw;
        ih = sd->wd->group_item_height;
     }
   EINA_LIST_FOREACH(sd->wd->group_items, l, it)
     {
        was_realized = it->realized;
        if (it->item->group_realized)
          {
             _item_realize(it);
             if (!was_realized)
               evas_object_smart_callback_call(WIDGET(it), SIG_REALIZED, it);
             evas_object_move(VIEW(it), it->item->gx, it->item->gy);
             evas_object_resize(VIEW(it), iw, ih);
             evas_object_raise(VIEW(it));
          }
        else
          _elm_genlist_item_unrealize(it, EINA_FALSE);
     }
}


static void
_item_place(Elm_Gen_Item *it,
            Evas_Coord        cx,
            Evas_Coord        cy)
{
   Evas_Coord x, y, ox, oy, cvx, cvy, cvw, cvh, iw, ih, ww;
   Evas_Coord tch, tcw, alignw = 0, alignh = 0, vw, vh;
   Eina_Bool reorder_item_move_forward = EINA_FALSE;
   long items_count;
   it->x = cx;
   it->y = cy;
   evas_object_geometry_get(it->wd->pan_smart, &ox, &oy, &vw, &vh);

   /* Preload rows/columns at each side of the Gengrid */
   cvx = ox - PRELOAD * it->wd->item_width;
   cvy = oy - PRELOAD * it->wd->item_height;
   cvw = vw + 2 * PRELOAD * it->wd->item_width;
   cvh = vh + 2 * PRELOAD * it->wd->item_height;

   alignh = 0;
   alignw = 0;

   items_count = it->wd->item_count - eina_list_count(it->wd->group_items) + it->wd->items_lost;
   if (it->wd->horizontal)
     {
        int columns, items_visible = 0, items_row;

        if (it->wd->item_height > 0)
          items_visible = vh / it->wd->item_height;
        if (items_visible < 1)
          items_visible = 1;

        columns = items_count / items_visible;
        if (items_count % items_visible)
          columns++;

        tcw = (it->wd->item_width * columns) + (it->wd->group_item_width * eina_list_count(it->wd->group_items));
        alignw = (vw - tcw) * it->wd->align_x;

        items_row = items_visible;
        if ((unsigned int)items_row > it->wd->item_count)
          items_row = it->wd->item_count;
         if (it->wd->filled
             && (unsigned int)it->wd->nmax > (unsigned int)it->wd->item_count)
           tch = it->wd->nmax * it->wd->item_height;
         else
           tch = items_row * it->wd->item_height;
        alignh = (vh - tch) * it->wd->align_y;
     }
   else
     {
        unsigned int rows, items_visible = 0, items_col;

        if (it->wd->item_width > 0)
          items_visible = vw / it->wd->item_width;
        if (items_visible < 1)
          items_visible = 1;

        rows = items_count / items_visible;
        if (items_count % items_visible)
          rows++;

        tch = (it->wd->item_height * rows) + (it->wd->group_item_height * eina_list_count(it->wd->group_items));
        alignh = (vh - tch) * it->wd->align_y;

        items_col = items_visible;
        if (items_col > it->wd->item_count)
          items_col = it->wd->item_count;
         if (it->wd->filled
             && (unsigned int)it->wd->nmax > (unsigned int)it->wd->item_count)
           tcw = it->wd->nmax * it->wd->item_width;
         else
           tcw = items_col * it->wd->item_width;
        alignw = (vw - tcw) * it->wd->align_x;
     }

   if (it->group)
     {
        if (it->wd->horizontal)
          {
             x = (((cx - it->item->prev_group) * it->wd->item_width) + (it->item->prev_group * it->wd->group_item_width)) - it->wd->pan_x + ox + alignw;
             y = oy;
             iw = it->wd->group_item_width;
             ih = vh;
          }
        else
          {
             x = ox;
             y = (((cy - it->item->prev_group) * it->wd->item_height) + (it->item->prev_group * it->wd->group_item_height)) - it->wd->pan_y + oy + alignh;
             iw = vw;
             ih = it->wd->group_item_height;
          }
        it->item->gx = x;
        it->item->gy = y;
     }
   else
     {
        if (it->wd->horizontal)
          {
             x = (((cx - it->item->prev_group) * it->wd->item_width) + (it->item->prev_group * it->wd->group_item_width)) - it->wd->pan_x + ox + alignw;
             y = (cy * it->wd->item_height) - it->wd->pan_y + oy + alignh;
          }
        else
          {
             x = (cx * it->wd->item_width) - it->wd->pan_x + ox + alignw;
             y = (((cy - it->item->prev_group) * it->wd->item_height) + (it->item->prev_group * it->wd->group_item_height)) - it->wd->pan_y + oy + alignh;
          }
        if (elm_widget_mirrored_get(WIDGET(it)))
          {  /* Switch items side and componsate for pan_x when in RTL mode */
             evas_object_geometry_get(WIDGET(it), NULL, NULL, &ww, NULL);
             x = ww - x - it->wd->item_width - it->wd->pan_x - it->wd->pan_x;
          }
        iw = it->wd->item_width;
        ih = it->wd->item_height;
     }

   Eina_Bool was_realized = it->realized;
   if (ELM_RECTS_INTERSECT(x, y, iw, ih, cvx, cvy, cvw, cvh))
     {
        _item_realize(it);
        if (!was_realized)
          evas_object_smart_callback_call(WIDGET(it), SIG_REALIZED, it);
        if (it->parent)
          {
             if (it->wd->horizontal)
               {
                  if (it->parent->item->gx < ox)
                    {
                       it->parent->item->gx = x + it->wd->item_width - it->wd->group_item_width;
                       if (it->parent->item->gx > ox)
                         it->parent->item->gx = ox;
                    }
                  it->parent->item->group_realized = EINA_TRUE;
               }
             else
               {
                  if (it->parent->item->gy < oy)
                    {
                       it->parent->item->gy = y + it->wd->item_height - it->wd->group_item_height;
                       if (it->parent->item->gy > oy)
                         it->parent->item->gy = oy;
                    }
                  it->parent->item->group_realized = EINA_TRUE;
               }
          }
        if (it->wd->reorder_mode)
          {
             if (it->wd->reorder_it)
               {
                  if (it->item->moving) return;

                  if (!it->wd->move_effect_enabled)
                    {
                       it->item->ox = x;
                       it->item->oy = y;
                    }
                  if (it->wd->reorder_it == it)
                    {
                       evas_object_move(VIEW(it),
                                        it->wd->reorder_item_x, it->wd->reorder_item_y);
                       evas_object_resize(VIEW(it), iw, ih);
                       return;
                    }
                  else
                    {
                       if (it->wd->move_effect_enabled)
                         {
                            if ((it->item->ox != x) || (it->item->oy != y))
                              {
                                 if (((it->wd->old_pan_x == it->wd->pan_x) && (it->wd->old_pan_y == it->wd->pan_y)) ||
                                     ((it->wd->old_pan_x != it->wd->pan_x) && !(it->item->ox - it->wd->pan_x + it->wd->old_pan_x == x)) ||
                                     ((it->wd->old_pan_y != it->wd->pan_y) && !(it->item->oy - it->wd->pan_y + it->wd->old_pan_y == y)))
                                   {
                                      it->item->tx = x;
                                      it->item->ty = y;
                                      it->item->rx = it->item->ox;
                                      it->item->ry = it->item->oy;
                                      it->item->moving = EINA_TRUE;
                                      it->item->moving_effect_start_time = ecore_loop_time_get();
                                      it->item->item_moving_effect_timer = ecore_animator_add(_reorder_item_moving_effect_timer_cb, it);
                                      return;
                                   }
                              }
                         }

                       /* need fix here */
                       Evas_Coord nx, ny, nw, nh;
                       if (it->group)
                         {
                            if (it->wd->horizontal)
                              {
                                 nx = x + (it->wd->group_item_width / 2);
                                 ny = y;
                                 nw = 1;
                                 nh = vh;
                              }
                            else
                              {
                                 nx = x;
                                 ny = y + (it->wd->group_item_height / 2);
                                 nw = vw;
                                 nh = 1;
                              }
                         }
                       else
                         {
                            nx = x + (it->wd->item_width / 2);
                            ny = y + (it->wd->item_height / 2);
                            nw = 1;
                            nh = 1;
                         }

                       if ( ELM_RECTS_INTERSECT(it->wd->reorder_item_x, it->wd->reorder_item_y,
                                                it->wd->item_width, it->wd->item_height,
                                                nx, ny, nw, nh))
                         {
                            if (it->wd->horizontal)
                              {
                                 if ((it->wd->nmax * it->wd->reorder_it->x + it->wd->reorder_it->y) >
                                     (it->wd->nmax * it->x + it->y))
                                   reorder_item_move_forward = EINA_TRUE;
                              }
                            else
                              {
                                 if ((it->wd->nmax * it->wd->reorder_it->y + it->wd->reorder_it->x) >
                                     (it->wd->nmax * it->y + it->x))
                                   reorder_item_move_forward = EINA_TRUE;
                              }

                            it->wd->items = eina_inlist_remove(it->wd->items,
                                                                 EINA_INLIST_GET(it->wd->reorder_it));
                            if (reorder_item_move_forward)
                              it->wd->items = eina_inlist_prepend_relative(it->wd->items,
                                                                             EINA_INLIST_GET(it->wd->reorder_it),
                                                                             EINA_INLIST_GET(it));
                            else
                              it->wd->items = eina_inlist_append_relative(it->wd->items,
                                                                            EINA_INLIST_GET(it->wd->reorder_it),
                                                                            EINA_INLIST_GET(it));

                            it->wd->reorder_item_changed = EINA_TRUE;
                            it->wd->move_effect_enabled = EINA_TRUE;
                            if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
                              it->wd->calc_job = ecore_job_add(_calc_job, it->wd);

                            return;
                         }
                    }
               }
             else if (it->item->item_moving_effect_timer)
               {
                  ecore_animator_del(it->item->item_moving_effect_timer);
                  it->item->item_moving_effect_timer = NULL;
                  it->item->moving = EINA_FALSE;
               }
          }
        if (!it->group)
          {
             evas_object_move(VIEW(it), x, y);
             evas_object_resize(VIEW(it), iw, ih);
          }
        else
          it->item->group_realized = EINA_TRUE;
     }
   else
     {
        if (!it->group)
          _elm_genlist_item_unrealize(it, EINA_FALSE);
        else
          it->item->group_realized = EINA_FALSE;
     }
}

static void
_item_del(Elm_Gen_Item *it)
{
   Evas_Object *obj = WIDGET(it);

   evas_event_freeze(evas_object_evas_get(obj));
   it->wd->selected = eina_list_remove(it->wd->selected, it);
   if (it->realized) _elm_genlist_item_unrealize(it, EINA_FALSE);
   it->wd->item_count--;
   _elm_genlist_item_del_serious(it);
   elm_gengrid_item_class_unref((Elm_Gengrid_Item_Class *)it->itc);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static void
_item_unselect(Elm_Gen_Item *it)
{
   if ((it->generation < it->wd->generation) || (!it->highlighted)) return;
   edje_object_signal_emit(VIEW(it), "elm,state,unselected", "elm");
   it->highlighted = EINA_FALSE;
   if (it->selected)
     {
        it->selected = EINA_FALSE;
        it->wd->selected = eina_list_remove(it->wd->selected, it);
        evas_object_smart_callback_call(WIDGET(it), SIG_UNSELECTED, it);
     }
}

static void
_calc_job(void *data)
{
   Widget_Data *wd = data;
   Evas_Coord minw = 0, minh = 0, nmax = 0, cvw, cvh;
   Elm_Gen_Item *it, *group_item = NULL;
   int count_group = 0;
   long count = 0;
   wd->items_lost = 0;

   evas_object_geometry_get(wd->pan_smart, NULL, NULL, &cvw, &cvh);
   if ((cvw != 0) || (cvh != 0))
     {
        if ((wd->horizontal) && (wd->item_height > 0))
          nmax = cvh / wd->item_height;
        else if (wd->item_width > 0)
          nmax = cvw / wd->item_width;

        if (nmax < 1)
          nmax = 1;

        EINA_INLIST_FOREACH(wd->items, it)
          {
             if (it->item->prev_group != count_group)
               it->item->prev_group = count_group;
             if (it->group)
               {
                  count = count % nmax;
                  if (count)
                    wd->items_lost += nmax - count;
                  //printf("%d items and I lost %d\n", count, wd->items_lost);
                  count_group++;
                  if (count) count = 0;
                  group_item = it;
               }
             else
               {
                  if (it->parent != group_item)
                    it->parent = group_item;
                  count++;
               }
          }
        count = wd->item_count + wd->items_lost - count_group;
        if (wd->horizontal)
          {
             minw = (ceil(count / (float)nmax) * wd->item_width) + (count_group * wd->group_item_width);
             minh = nmax * wd->item_height;
          }
        else
          {
             minw = nmax * wd->item_width;
             minh = (ceil(count / (float)nmax) * wd->item_height) + (count_group * wd->group_item_height);
          }

        if ((minw != wd->minw) || (minh != wd->minh))
          {
             wd->minh = minh;
             wd->minw = minw;
             evas_object_smart_callback_call(wd->pan_smart, "changed", NULL);
          }

        wd->nmax = nmax;
        evas_object_smart_changed(wd->pan_smart);
     }
   wd->calc_job = NULL;
}

static void
_pan_add(Evas_Object *obj)
{
   Pan *sd;
   Evas_Object_Smart_Clipped_Data *cd;

   _pan_sc.add(obj);
   cd = evas_object_smart_data_get(obj);
   sd = ELM_NEW(Pan);
   if (!sd) return;
   sd->__clipped_data = *cd;
   free(cd);
   evas_object_smart_data_set(obj, sd);
}

static void
_pan_del(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   _pan_sc.del(obj);
}

static void
_pan_set(Evas_Object *obj,
         Evas_Coord   x,
         Evas_Coord   y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if ((x == sd->wd->pan_x) && (y == sd->wd->pan_y)) return;
   sd->wd->pan_x = x;
   sd->wd->pan_y = y;
   evas_object_smart_changed(obj);
}

static void
_pan_get(Evas_Object *obj,
         Evas_Coord  *x,
         Evas_Coord  *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (x) *x = sd->wd->pan_x;
   if (y) *y = sd->wd->pan_y;
}

static void
_pan_child_size_get(Evas_Object *obj,
                    Evas_Coord  *w,
                    Evas_Coord  *h)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (w) *w = sd->wd->minw;
   if (h) *h = sd->wd->minh;
}

static void
_pan_max_get(Evas_Object *obj,
             Evas_Coord  *x,
             Evas_Coord  *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;

   if (!sd) return;
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if (x)
     *x = (ow < sd->wd->minw) ? sd->wd->minw - ow : 0;
   if (y)
     *y = (oh < sd->wd->minh) ? sd->wd->minh - oh : 0;
}

static void
_pan_min_get(Evas_Object *obj,
             Evas_Coord  *x,
             Evas_Coord  *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord mx = 0, my = 0;

   if (!sd) return;
   _pan_max_get(obj, &mx, &my);
   if (x)
     *x = -mx * sd->wd->align_x;
   if (y)
     *y = -my * sd->wd->align_y;
}

static void
_pan_resize(Evas_Object *obj,
            Evas_Coord   w,
            Evas_Coord   h)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
   sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_pan_calculate(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord cx = 0, cy = 0;
   Elm_Gen_Item *it;

   if (!sd) return;
   if (!sd->wd->nmax) return;

   sd->wd->reorder_item_changed = EINA_FALSE;

   EINA_INLIST_FOREACH(sd->wd->items, it)
     {
        if (it->group)
          {
             if (sd->wd->horizontal)
               {
                  if (cy)
                    {
                       cx++;
                       cy = 0;
                    }
               }
             else
               {
                  if (cx)
                    {
                       cx = 0;
                       cy++;
                    }
               }
          }
        _item_place(it, cx, cy);
        if (sd->wd->reorder_item_changed) return;
        if (it->group)
          {
             if (sd->wd->horizontal)
               {
                  cx++;
                  cy = 0;
               }
             else
               {
                  cx = 0;
                  cy++;
               }
          }
        else
          {
             if (sd->wd->horizontal)
               {
                  cy = (cy + 1) % sd->wd->nmax;
                  if (!cy) cx++;
               }
             else
               {
                  cx = (cx + 1) % sd->wd->nmax;
                  if (!cx) cy++;
               }
          }
     }
   _group_item_place(sd);


   if ((sd->wd->reorder_mode) && (sd->wd->reorder_it))
     {
        if (!sd->wd->reorder_item_changed)
          {
             sd->wd->old_pan_x = sd->wd->pan_x;
             sd->wd->old_pan_y = sd->wd->pan_y;
          }
        sd->wd->move_effect_enabled = EINA_FALSE;
     }
   evas_object_smart_callback_call(sd->wd->obj, SIG_CHANGED, NULL);
}

static void
_pan_move(Evas_Object *obj,
          Evas_Coord x __UNUSED__,
          Evas_Coord y __UNUSED__)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
   sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_hold_on(void *data       __UNUSED__,
         Evas_Object     *obj,
         void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 1);
}

static void
_hold_off(void *data       __UNUSED__,
          Evas_Object     *obj,
          void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 0);
}

static void
_freeze_on(void *data       __UNUSED__,
           Evas_Object     *obj,
           void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 1);
}

static void
_freeze_off(void *data       __UNUSED__,
            Evas_Object     *obj,
            void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 0);
}

static void
_scr_anim_start(void        *data,
                Evas_Object *obj __UNUSED__,
                void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_ANIM_START, NULL);
}

static void
_scr_anim_stop(void        *data,
                Evas_Object *obj __UNUSED__,
                void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_ANIM_STOP, NULL);
}

static void
_scr_drag_start(void            *data,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scr_drag_stop(void            *data,
               Evas_Object *obj __UNUSED__,
               void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_STOP, NULL);
}

static void
_edge_left(void        *data,
           Evas_Object *scr __UNUSED__,
           void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_EDGE_LEFT, NULL);
}

static void
_edge_right(void        *data,
            Evas_Object *scr __UNUSED__,
            void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_EDGE_RIGHT, NULL);
}

static void
_edge_top(void        *data,
          Evas_Object *scr __UNUSED__,
          void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_EDGE_TOP, NULL);
}

static void
_edge_bottom(void        *data,
             Evas_Object *scr __UNUSED__,
             void        *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_EDGE_BOTTOM, NULL);
}

static void
_scr_scroll(void            *data,
            Evas_Object *obj __UNUSED__,
            void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL, NULL);
}

static int
_elm_gengrid_item_compare_data(const void *data, const void *data1)
{
   const Elm_Gen_Item *it = data;
   const Elm_Gen_Item *item1 = data1;

   return it->wd->item_compare_data_cb(it->base.data, item1->base.data);
}

static int
_elm_gengrid_item_compare(const void *data, const void *data1)
{
   Elm_Gen_Item *it, *item1;
   it = ELM_GEN_ITEM_FROM_INLIST(data);
   item1 = ELM_GEN_ITEM_FROM_INLIST(data1);
   return it->wd->item_compare_cb(it, item1);
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;

   if (_it->generation < _it->wd->generation) return;

   if (_it->realized)
     {
        if (elm_widget_item_disabled_get(_it))
          edje_object_signal_emit(VIEW(_it), "elm,state,disabled", "elm");
        else
          edje_object_signal_emit(VIEW(_it), "elm,state,enabled", "elm");
     }
}

static void
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;
   if ((_it->relcount > 0) || (_it->walking > 0))
     {
        _elm_genlist_item_del_notserious(_it);
        return;
     }

   _item_del(_it);
}

static Elm_Gen_Item *
_item_new(Widget_Data                  *wd,
          const Elm_Gengrid_Item_Class *itc,
          const void                   *data,
          Evas_Smart_Cb                 func,
          const void                   *func_data)
{
   Elm_Gen_Item *it;

   it = _elm_genlist_item_new(wd, itc, data, NULL, func, func_data);
   if (!it) return NULL;
   elm_widget_item_disable_hook_set(it, _item_disable_hook);
   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_gengrid_item_class_ref((Elm_Gengrid_Item_Class *)itc);
   it->item = ELM_NEW(Elm_Gen_Item_Type);
   wd->item_count++;
   it->group = it->itc->item_style && (!strcmp(it->itc->item_style, "group_index"));
   ELM_GEN_ITEM_SETUP(it);

   return it;
}

EAPI Evas_Object *
elm_gengrid_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   static Evas_Smart *smart = NULL;
   Eina_Bool bounce = _elm_config->thumbscroll_bounce_enable;

   if (!smart)
     {
        static Evas_Smart_Class sc;

        evas_object_smart_clipped_smart_set(&_pan_sc);
        sc = _pan_sc;
        sc.name = "elm_gengrid_pan";
        sc.version = EVAS_SMART_CLASS_VERSION;
        sc.add = _pan_add;
        sc.del = _pan_del;
        sc.resize = _pan_resize;
        sc.move = _pan_move;
        sc.calculate = _pan_calculate;
        if (!(smart = evas_smart_class_new(&sc))) return NULL;
     }

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "gengrid");
   ELM_GEN_SETUP(wd);
   elm_widget_type_set(obj, "gengrid");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_signal_callback_add_hook_set(obj, _signal_callback_add_hook);
   elm_widget_signal_callback_del_hook_set(obj, _signal_callback_del_hook);
   elm_widget_event_hook_set(obj, _event_hook);

   wd->generation = 1;
   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "gengrid", "base",
                                       "default");
   elm_smart_scroller_bounce_allow_set(wd->scr, bounce,
                                       _elm_config->thumbscroll_bounce_enable);
   elm_widget_resize_object_set(obj, wd->scr);

   evas_object_smart_callback_add(wd->scr, "animate,start", _scr_anim_start, obj);
   evas_object_smart_callback_add(wd->scr, "animate,stop", _scr_anim_stop, obj);
   evas_object_smart_callback_add(wd->scr, "drag,start", _scr_drag_start, obj);
   evas_object_smart_callback_add(wd->scr, "drag,stop", _scr_drag_stop, obj);
   evas_object_smart_callback_add(wd->scr, "edge,left", _edge_left, obj);
   evas_object_smart_callback_add(wd->scr, "edge,right", _edge_right, obj);
   evas_object_smart_callback_add(wd->scr, "edge,top", _edge_top, obj);
   evas_object_smart_callback_add(wd->scr, "edge,bottom", _edge_bottom,
                                  obj);
   evas_object_smart_callback_add(wd->scr, "scroll", _scr_scroll, obj);

   wd->obj = obj;
   wd->align_x = 0.5;
   wd->align_y = 0.5;
   wd->h_bounce = bounce;
   wd->v_bounce = bounce;

   evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);

   wd->pan_smart = evas_object_smart_add(e, smart);
   wd->pan = evas_object_smart_data_get(wd->pan_smart);
   wd->pan->wd = wd;

   elm_smart_scroller_extern_pan_set(wd->scr, wd->pan_smart,
                                     _pan_set, _pan_get, _pan_max_get,
                                     _pan_min_get, _pan_child_size_get);

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   return obj;
}

EAPI void
elm_gengrid_item_size_set(Evas_Object *obj,
                          Evas_Coord   w,
                          Evas_Coord   h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((wd->item_width == w) && (wd->item_height == h)) return;
   wd->item_width = w;
   wd->item_height = h;
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
}

EAPI void
elm_gengrid_item_size_get(const Evas_Object *obj,
                          Evas_Coord        *w,
                          Evas_Coord        *h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (w) *w = wd->item_width;
   if (h) *h = wd->item_height;
}

EAPI void
elm_gengrid_group_item_size_set(Evas_Object *obj,
                          Evas_Coord   w,
                          Evas_Coord   h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((wd->group_item_width == w) && (wd->group_item_height == h)) return;
   wd->group_item_width = w;
   wd->group_item_height = h;
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
}

EAPI void
elm_gengrid_group_item_size_get(const Evas_Object *obj,
                          Evas_Coord        *w,
                          Evas_Coord        *h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (w) *w = wd->group_item_width;
   if (h) *h = wd->group_item_height;
}

EAPI void
elm_gengrid_align_set(Evas_Object *obj,
                      double       align_x,
                      double       align_y)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd = elm_widget_data_get(obj);
   double old_h = wd->align_x, old_y = wd->align_y;

   if (align_x > 1.0)
     align_x = 1.0;
   else if (align_x < 0.0)
     align_x = 0.0;
   wd->align_x = align_x;

   if (align_y > 1.0)
     align_y = 1.0;
   else if (align_y < 0.0)
     align_y = 0.0;
   wd->align_y = align_y;

   if ((old_h != wd->align_x) || (old_y != wd->align_y))
     evas_object_smart_calculate(wd->pan_smart);
}

EAPI void
elm_gengrid_align_get(const Evas_Object *obj,
                      double            *align_x,
                      double            *align_y)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (align_x) *align_x = wd->align_x;
   if (align_y) *align_y = wd->align_y;
}

EAPI Elm_Object_Item *
elm_gengrid_item_append(Evas_Object                  *obj,
                        const Elm_Gengrid_Item_Class *itc,
                        const void                   *data,
                        Evas_Smart_Cb                 func,
                        const void                   *func_data)
{
   Elm_Gen_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(wd, itc, data, func, func_data);
   if (!it) return NULL;
   wd->items = eina_inlist_append(wd->items, EINA_INLIST_GET(it));

   if (it->group)
     wd->group_items = eina_list_prepend(wd->group_items, it);

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_gengrid_item_prepend(Evas_Object                  *obj,
                         const Elm_Gengrid_Item_Class *itc,
                         const void                   *data,
                         Evas_Smart_Cb                 func,
                         const void                   *func_data)
{
   Elm_Gen_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(wd, itc, data, func, func_data);
   if (!it) return NULL;
   wd->items = eina_inlist_prepend(wd->items, EINA_INLIST_GET(it));
   if (it->group)
     wd->group_items = eina_list_append(wd->group_items, it);

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_gengrid_item_insert_before(Evas_Object                  *obj,
                               const Elm_Gengrid_Item_Class *itc,
                               const void                   *data,
                               Elm_Object_Item              *relative,
                               Evas_Smart_Cb                 func,
                               const void                   *func_data)
{
   Elm_Gen_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_OBJ_ITEM_CHECK_OR_RETURN(relative, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(wd, itc, data, func, func_data);
   if (!it) return NULL;
   wd->items = eina_inlist_prepend_relative
      (wd->items, EINA_INLIST_GET(it), EINA_INLIST_GET((Elm_Gen_Item *) relative));
   if (it->group)
     wd->group_items = eina_list_append_relative(wd->group_items, it, ((Elm_Gen_Item *) relative)->parent);

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_gengrid_item_insert_after(Evas_Object                  *obj,
                              const Elm_Gengrid_Item_Class *itc,
                              const void                   *data,
                              Elm_Object_Item              *relative,
                              Evas_Smart_Cb                 func,
                              const void                   *func_data)
{
   Elm_Gen_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_OBJ_ITEM_CHECK_OR_RETURN(relative, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(wd, itc, data, func, func_data);
   if (!it) return NULL;
   wd->items = eina_inlist_append_relative
      (wd->items, EINA_INLIST_GET(it), EINA_INLIST_GET((Elm_Gen_Item *) relative));
   if (it->group)
     wd->group_items = eina_list_prepend_relative(wd->group_items, it, ((Elm_Gen_Item *) relative)->parent);

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_gengrid_item_direct_sorted_insert(Evas_Object                  *obj,
                                      const Elm_Gengrid_Item_Class *itc,
                                      const void                   *data,
                                      Eina_Compare_Cb               comp,
                                      Evas_Smart_Cb                 func,
                                      const void                   *func_data)
{
   Elm_Gen_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(wd, itc, data, func, func_data);
   if (!it) return NULL;

   if (!wd->state)
     wd->state = eina_inlist_sorted_state_new();

   wd->item_compare_cb = comp;
   wd->items = eina_inlist_sorted_state_insert(wd->items, EINA_INLIST_GET(it),
                                         _elm_gengrid_item_compare, wd->state);
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_gengrid_item_sorted_insert(Evas_Object                  *obj,
                               const Elm_Gengrid_Item_Class *itc,
                               const void                   *data,
                               Eina_Compare_Cb               comp,
                               Evas_Smart_Cb                 func,
                               const void                   *func_data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->item_compare_data_cb = comp;

   return elm_gengrid_item_direct_sorted_insert(obj, itc, data, _elm_gengrid_item_compare_data, func, func_data);
}

EAPI void
elm_gengrid_item_del(Elm_Object_Item *it)
{
   elm_object_item_del(it);
}

EAPI void
elm_gengrid_horizontal_set(Evas_Object *obj,
                           Eina_Bool    horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   horizontal = !!horizontal;
   if (horizontal == wd->horizontal) return;
   wd->horizontal = horizontal;

   /* Update the items to conform to the new layout */
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
}

EAPI Eina_Bool
elm_gengrid_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->horizontal;
}

EAPI void
elm_gengrid_clear(Evas_Object *obj)
{
   elm_genlist_clear(obj);
}

EINA_DEPRECATED EAPI const Evas_Object *
elm_gengrid_item_object_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return VIEW(it);
}

EAPI void
elm_gengrid_item_update(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;
   if (!_it->realized) return;
   if (_it->want_unrealize) return;
   _elm_genlist_item_unrealize(_it, EINA_FALSE);
   _item_realize(_it);
   _item_place(_it, _it->x, _it->y);
}

EAPI void *
elm_gengrid_item_data_get(const Elm_Object_Item *it)
{
   return elm_object_item_data_get(it);
}

EAPI void
elm_gengrid_item_data_set(Elm_Object_Item  *it,
                          const void       *data)
{
   elm_object_item_data_set(it, (void *) data);
}

EAPI const Elm_Gengrid_Item_Class *
elm_gengrid_item_item_class_get(const Elm_Object_Item *it)
{
   return (Elm_Gengrid_Item_Class *) elm_genlist_item_item_class_get(it);
}

EAPI void
elm_gengrid_item_item_class_set(Elm_Object_Item *it,
                                const Elm_Gengrid_Item_Class *itc)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   EINA_SAFETY_ON_NULL_RETURN(itc);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;
   if (_it->generation < _it->wd->generation) return;
   _it->itc = itc;
   elm_gengrid_item_update(it);
}

EAPI void
elm_gengrid_item_pos_get(const Elm_Object_Item *it,
                         unsigned int           *x,
                         unsigned int           *y)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   if (x) *x = ((Elm_Gen_Item *) it)->x;
   if (y) *y = ((Elm_Gen_Item *) it)->y;
}

EAPI void
elm_gengrid_multi_select_set(Evas_Object *obj,
                             Eina_Bool    multi)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->multi = !!multi;
}

EAPI Eina_Bool
elm_gengrid_multi_select_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->multi;
}

EAPI Elm_Object_Item *
elm_gengrid_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->selected) return wd->selected->data;
   return NULL;
}

EAPI const Eina_List *
elm_gengrid_selected_items_get(const Evas_Object *obj)
{
   return elm_gengrid_selected_items_get(obj);
}

EAPI void
elm_gengrid_item_selected_set(Elm_Object_Item  *it,
                              Eina_Bool         selected)
{
   elm_genlist_item_selected_set(it, selected);
}

EAPI Eina_Bool
elm_gengrid_item_selected_get(const Elm_Object_Item *it)
{
   return elm_genlist_item_selected_get(it);
}

EAPI void
elm_gengrid_item_disabled_set(Elm_Object_Item  *it,
                              Eina_Bool         disabled)
{
   elm_object_item_disabled_set(it, disabled);
}

EAPI Eina_Bool
elm_gengrid_item_disabled_get(const Elm_Object_Item *it)
{
   return elm_object_item_disabled_get(it);
}

static Evas_Object *
_elm_gengrid_item_label_create(void        *data,
                               Evas_Object *obj __UNUSED__,
                               Evas_Object *tooltip,
                               void *it   __UNUSED__)
{
   Evas_Object *label = elm_label_add(tooltip);
   if (!label)
     return NULL;
   elm_object_style_set(label, "tooltip");
   elm_object_text_set(label, data);
   return label;
}

static void
_elm_gengrid_item_label_del_cb(void            *data,
                               Evas_Object *obj __UNUSED__,
                               void *event_info __UNUSED__)
{
   eina_stringshare_del(data);
}

EAPI void
elm_gengrid_item_tooltip_text_set(Elm_Object_Item  *it,
                                  const char       *text)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   text = eina_stringshare_add(text);
   elm_gengrid_item_tooltip_content_cb_set(it, _elm_gengrid_item_label_create,
                                           text,
                                           _elm_gengrid_item_label_del_cb);
}

EAPI void
elm_gengrid_item_tooltip_content_cb_set(Elm_Object_Item            *it,
                                        Elm_Tooltip_Item_Content_Cb func,
                                        const void                 *data,
                                        Evas_Smart_Cb               del_cb)
{
   ELM_OBJ_ITEM_CHECK_OR_GOTO(it, error);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;

   if ((_it->tooltip.content_cb == func) && (_it->tooltip.data == data))
     return;

   if (_it->tooltip.del_cb)
     _it->tooltip.del_cb((void *)_it->tooltip.data, WIDGET(_it), _it);
   _it->tooltip.content_cb = func;
   _it->tooltip.data = data;
   _it->tooltip.del_cb = del_cb;
   if (VIEW(_it))
     {
        elm_widget_item_tooltip_content_cb_set(_it,
                                               _it->tooltip.content_cb,
                                               _it->tooltip.data, NULL);
        elm_widget_item_tooltip_style_set(_it, _it->tooltip.style);
        elm_widget_item_tooltip_window_mode_set(_it, _it->tooltip.free_size);
     }

   return;

error:
   if (del_cb) del_cb((void *)data, NULL, NULL);
}

EAPI void
elm_gengrid_item_tooltip_unset(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;

   if ((VIEW(_it)) && (_it->tooltip.content_cb))
     elm_widget_item_tooltip_unset(_it);

   if (_it->tooltip.del_cb)
     _it->tooltip.del_cb((void *) _it->tooltip.data, WIDGET(_it), _it);
   _it->tooltip.del_cb = NULL;
   _it->tooltip.content_cb = NULL;
   _it->tooltip.data = NULL;
   _it->tooltip.free_size = EINA_FALSE;
   if (_it->tooltip.style)
     elm_gengrid_item_tooltip_style_set(it, NULL);
}

EAPI void
elm_gengrid_item_tooltip_style_set(Elm_Object_Item  *it,
                                   const char       *style)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   eina_stringshare_replace(&((Elm_Gen_Item *) it)->tooltip.style, style);
   if (VIEW(it)) elm_widget_item_tooltip_style_set(it, style);
}

EAPI const char *
elm_gengrid_item_tooltip_style_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Gen_Item *) it)->tooltip.style;
}

EAPI Eina_Bool
elm_gengrid_item_tooltip_window_mode_set(Elm_Object_Item *it, Eina_Bool disable)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   ((Elm_Gen_Item *) it)->tooltip.free_size = disable;
   if (VIEW(it)) return elm_widget_item_tooltip_window_mode_set(it, disable);
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_gengrid_item_tooltip_window_mode_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Gen_Item *) it)->tooltip.free_size;
}

EAPI void
elm_gengrid_item_cursor_set(Elm_Object_Item  *it,
                            const char       *cursor)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   eina_stringshare_replace(&((Elm_Gen_Item *) it)->mouse_cursor, cursor);
   if (VIEW(it)) elm_widget_item_cursor_set(it, cursor);
}

EAPI const char *
elm_gengrid_item_cursor_get(const Elm_Object_Item *it)
{
   return elm_widget_item_cursor_get(it);
}

EAPI void
elm_gengrid_item_cursor_unset(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;
   if (!_it->mouse_cursor)
     return;

   if (VIEW(_it))
     elm_widget_item_cursor_unset(_it);

   eina_stringshare_del(_it->mouse_cursor);
   _it->mouse_cursor = NULL;
}

EAPI void
elm_gengrid_item_cursor_style_set(Elm_Object_Item  *it,
                                  const char       *style)
{
   elm_widget_item_cursor_style_set(it, style);
}

EAPI const char *
elm_gengrid_item_cursor_style_get(const Elm_Object_Item *it)
{
   return elm_widget_item_cursor_style_get(it);
}

EAPI void
elm_gengrid_item_cursor_engine_only_set(Elm_Object_Item  *it,
                                        Eina_Bool         engine_only)
{
   elm_widget_item_cursor_engine_only_set(it, engine_only);
}

EAPI Eina_Bool
elm_gengrid_item_cursor_engine_only_get(const Elm_Object_Item *it)
{
   return elm_widget_item_cursor_engine_only_get(it);
}

EAPI void
elm_gengrid_reorder_mode_set(Evas_Object *obj,
                             Eina_Bool    reorder_mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->reorder_mode = !!reorder_mode;
}

EAPI Eina_Bool
elm_gengrid_reorder_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->reorder_mode;
}

EAPI void
elm_gengrid_always_select_mode_set(Evas_Object *obj,
                                   Eina_Bool    always_select)
{
   elm_genlist_always_select_mode_set(obj, always_select);
}

EAPI Eina_Bool
elm_gengrid_always_select_mode_get(const Evas_Object *obj)
{
   return elm_genlist_always_select_mode_get(obj);
}

EAPI void
elm_gengrid_no_select_mode_set(Evas_Object *obj,
                               Eina_Bool    no_select)
{
   elm_genlist_no_select_mode_set(obj, no_select);
}

EAPI Eina_Bool
elm_gengrid_no_select_mode_get(const Evas_Object *obj)
{
   return elm_genlist_no_select_mode_get(obj);
}

EAPI void
elm_gengrid_bounce_set(Evas_Object *obj,
                       Eina_Bool    h_bounce,
                       Eina_Bool    v_bounce)
{
   elm_genlist_bounce_set(obj, h_bounce, v_bounce);
}

EAPI void
elm_gengrid_bounce_get(const Evas_Object *obj,
                       Eina_Bool         *h_bounce,
                       Eina_Bool         *v_bounce)
{
   elm_genlist_bounce_get(obj, h_bounce, v_bounce);
}

EAPI void
elm_gengrid_page_relative_set(Evas_Object *obj,
                              double       h_pagerel,
                              double       v_pagerel)
{
   _elm_genlist_page_relative_set(obj, h_pagerel, v_pagerel);
}

EAPI void
elm_gengrid_page_relative_get(const Evas_Object *obj, double *h_pagerel, double *v_pagerel)
{
   _elm_genlist_page_relative_get(obj, h_pagerel, v_pagerel);
}

EAPI void
elm_gengrid_page_size_set(Evas_Object *obj,
                          Evas_Coord   h_pagesize,
                          Evas_Coord   v_pagesize)
{
   _elm_genlist_page_size_set(obj, h_pagesize, v_pagesize);
}

EAPI void
elm_gengrid_current_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber)
{
   _elm_genlist_current_page_get(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_gengrid_last_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber)
{
   _elm_genlist_last_page_get(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_gengrid_page_show(const Evas_Object *obj, int h_pagenumber, int v_pagenumber)
{
   _elm_genlist_page_show(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_gengrid_page_bring_in(const Evas_Object *obj, int h_pagenumber, int v_pagenumber)
{
   _elm_genlist_page_bring_in(obj, h_pagenumber, v_pagenumber);
}

EAPI Elm_Object_Item *
elm_gengrid_first_item_get(const Evas_Object *obj)
{
   return elm_genlist_first_item_get(obj);
}

EAPI Elm_Object_Item *
elm_gengrid_last_item_get(const Evas_Object *obj)
{
   return elm_genlist_last_item_get(obj);
}

EAPI Elm_Object_Item *
elm_gengrid_item_next_get(const Elm_Object_Item *it)
{
   return elm_genlist_item_next_get(it);
}

EAPI Elm_Object_Item *
elm_gengrid_item_prev_get(const Elm_Object_Item *it)
{
   return elm_genlist_item_prev_get(it);
}

EAPI Evas_Object *
elm_gengrid_item_gengrid_get(const Elm_Object_Item *it)
{
   return elm_object_item_widget_get(it);
}

EAPI void
elm_gengrid_item_show(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;
   Widget_Data *wd = _it->wd;
   Evas_Coord minx = 0, miny = 0;

   if ((_it->generation < _it->wd->generation)) return;
   _pan_min_get(wd->pan_smart, &minx, &miny);

   if (wd->horizontal)
     elm_smart_scroller_region_bring_in(_it->wd->scr,
                                        ((_it->x - _it->item->prev_group) * wd->item_width) + (_it->item->prev_group * _it->wd->group_item_width) + minx,
                                        _it->y * wd->item_height + miny,
                                        _it->wd->item_width,
                                        _it->wd->item_height);
   else
     elm_smart_scroller_region_bring_in(_it->wd->scr,
                                        _it->x * wd->item_width + minx,
                                        ((_it->y - _it->item->prev_group) * wd->item_height) + (_it->item->prev_group * _it->wd->group_item_height) + miny,
                                        _it->wd->item_width,
                                        _it->wd->item_height);
}

EAPI void
elm_gengrid_item_bring_in(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Gen_Item *_it = (Elm_Gen_Item *) it;

   if (_it->generation < _it->wd->generation) return;

   Evas_Coord minx = 0, miny = 0;
   Widget_Data *wd = _it->wd;
   _pan_min_get(wd->pan_smart, &minx, &miny);

   if (wd->horizontal)
     elm_smart_scroller_region_bring_in(_it->wd->scr,
                                        ((_it->x - _it->item->prev_group) * wd->item_width) + (_it->item->prev_group * _it->wd->group_item_width) + minx,
                                        _it->y * wd->item_height + miny,
                                        _it->wd->item_width,
                                        _it->wd->item_height);
   else
     elm_smart_scroller_region_bring_in(_it->wd->scr,
                                        _it->x * wd->item_width + minx,
                                        ((_it->y - _it->item->prev_group)* wd->item_height) + (_it->item->prev_group * _it->wd->group_item_height) + miny,
                                        _it->wd->item_width,
                                        _it->wd->item_height);
}

EAPI void
elm_gengrid_filled_set(Evas_Object *obj, Eina_Bool fill)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   fill = !!fill;
   if (wd->filled != fill)
     wd->filled = fill;
}

EAPI Eina_Bool
elm_gengrid_filled_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->filled;
}

EAPI Elm_Gengrid_Item_Class *
elm_gengrid_item_class_new(void)
{
   Elm_Gengrid_Item_Class *itc;

   itc = calloc(1, sizeof(Elm_Gengrid_Item_Class));
   if (!itc)
     return NULL;
   itc->version = ELM_GENGRID_ITEM_CLASS_VERSION;
   itc->refcount = 1;
   itc->delete_me = EINA_FALSE;

   return itc;
}

EAPI void
elm_gengrid_item_class_free(Elm_Gengrid_Item_Class *itc)
{
   ELM_GENGRID_CHECK_ITC_VER(itc);
   if (!itc->delete_me) itc->delete_me = EINA_TRUE;
   if (itc->refcount > 0) elm_gengrid_item_class_unref(itc);
   else
     {
        itc->version = 0;
        free(itc);
     }
}

EAPI void
elm_gengrid_item_class_ref(Elm_Gengrid_Item_Class *itc)
{
   ELM_GENGRID_CHECK_ITC_VER(itc);

   itc->refcount++;
   if (itc->refcount == 0) itc->refcount--;
}

EAPI void
elm_gengrid_item_class_unref(Elm_Gengrid_Item_Class *itc)
{
   ELM_GENGRID_CHECK_ITC_VER(itc);

   if (itc->refcount > 0) itc->refcount--;
   if (itc->delete_me && (!itc->refcount))
     elm_gengrid_item_class_free(itc);
}

