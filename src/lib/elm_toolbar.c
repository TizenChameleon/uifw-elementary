#include <Elementary.h>
#include "elm_priv.h"
#include "els_scroller.h"
#include "els_box.h"
#include "els_icon.h"

typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Toolbar_Item Elm_Toolbar_Item;

struct _Widget_Data
{
   Evas_Object *scr, *bx;
   Evas_Object *menu_parent;
   Eina_Inlist *items;
   Elm_Toolbar_Item *more_item, *selected_item;
   Elm_Toolbar_Shrink_Mode shrink_mode;
   Elm_Icon_Lookup_Order lookup_order;
   int icon_size;
   double align;
   Eina_Bool homogeneous : 1;
   Eina_Bool no_select : 1;
   Eina_Bool always_select : 1;
   Eina_Bool vertical : 1;
   Eina_Bool long_press : 1;
   Ecore_Timer *long_timer;
   Ecore_Job *resize_job;
};

struct _Elm_Toolbar_Item
{
   ELM_WIDGET_ITEM;
   EINA_INLIST;
   const char *label;
   const char *icon_str;
   Evas_Object *icon;
   Evas_Object *o_menu;
   Evas_Smart_Cb func;
   struct {
      int priority;
      Eina_Bool visible : 1;
   } prio;
   Eina_Bool selected : 1;
   Eina_Bool disabled : 1;
   Eina_Bool separator : 1;
   Eina_Bool menu : 1;
   Eina_List *states;
   Eina_List *current_state;
};

#define ELM_TOOLBAR_ITEM_FROM_INLIST(item)      \
  ((item) ? EINA_INLIST_CONTAINER_GET(item, Elm_Toolbar_Item) : NULL)

struct _Elm_Toolbar_Item_State
{
   const char *label;
   const char *icon_str;
   Evas_Object *icon;
   Evas_Smart_Cb func;
   const void *data;
};

static const char *widtype = NULL;
static void _item_show(Elm_Toolbar_Item *it);
static void _item_select(Elm_Toolbar_Item *it);
static void _item_unselect(Elm_Toolbar_Item *it);
static void _item_disable(Elm_Toolbar_Item *it, Eina_Bool disabled);
static void _del_pre_hook(Evas_Object *obj);
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool mirrored);
static void _mirrored_set_item(Evas_Object *obj, Elm_Toolbar_Item *it, Eina_Bool mirrored);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_move_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data);
static void _elm_toolbar_item_icon_obj_set(Evas_Object *obj, Elm_Toolbar_Item *item, Evas_Object *icon_obj, const char *icon_str, double icon_size, const char *signal);
static void _item_label_set(Elm_Toolbar_Item *item, const char *label, const char *signal);

static const char SIG_CLICKED[] = "clicked";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";

static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_CLICKED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {NULL, NULL}
};


static Eina_Bool
_item_icon_set(Evas_Object *icon_obj, const char *type, const char *icon)
{
   char icon_str[512];

   if ((!type) || (!*type)) goto end;
   if ((!icon) || (!*icon)) return EINA_FALSE;
   if ((snprintf(icon_str, sizeof(icon_str), "%s%s", type, icon) > 0)
       && (elm_icon_standard_set(icon_obj, icon_str)))
     return EINA_TRUE;
end:
   if (elm_icon_standard_set(icon_obj, icon))
     return EINA_TRUE;
   WRN("couldn't find icon definition for '%s'", icon);
   return EINA_FALSE;
}

static int
_elm_toolbar_icon_size_get(Widget_Data *wd)
{
   const char *icon_size = edje_object_data_get(
      elm_smart_scroller_edje_object_get(wd->scr), "icon_size");
   if (icon_size)
     return atoi(icon_size);
   return _elm_config->icon_size;
}

static void
_item_show(Elm_Toolbar_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   Evas_Coord x, y, w, h, bx, by;

   if (!wd) return;
   evas_object_geometry_get(wd->bx, &bx, &by, NULL, NULL);
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scr, x - bx, y - by, w, h);
}

static void
_item_unselect(Elm_Toolbar_Item *item)
{
   Widget_Data *wd;
   if ((!item) || (!item->selected)) return;
   wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;
   item->selected = EINA_FALSE;
   wd->selected_item = NULL;
   edje_object_signal_emit(VIEW(item), "elm,state,unselected", "elm");
   elm_widget_signal_emit(item->icon, "elm,state,unselected", "elm");
}

static void
_item_select(Elm_Toolbar_Item *it)
{
   Elm_Toolbar_Item *it2;
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   Evas_Object *obj2;
   Eina_Bool sel;

   if (!wd) return;
   if ((it->disabled) || (it->separator)) return;
   sel = it->selected;

   if (!wd->no_select)
     {
        if (sel)
          {
             if (wd->always_select) return;
             _item_unselect(it);
          }
        else
          {
             it2 = (Elm_Toolbar_Item *)
                elm_toolbar_selected_item_get(WIDGET(it));
             _item_unselect(it2);

             it->selected = EINA_TRUE;
             wd->selected_item = it;
             edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,selected", "elm");
             _item_show(it);
          }
     }
   obj2 = WIDGET(it);
   if (it->menu && (!sel))
     {
        evas_object_show(it->o_menu);
        evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_RESIZE,
                                       _menu_move_resize, it);
        evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOVE,
                                       _menu_move_resize, it);

        _menu_move_resize(it, NULL, NULL, NULL);
     }
   if (it->func) it->func((void *)(it->base.data), WIDGET(it), it);
   evas_object_smart_callback_call(obj2, SIG_CLICKED, it);
}

static void
_menu_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *selected;
   Elm_Toolbar_Item *it = data;
   selected = (Elm_Toolbar_Item *) elm_toolbar_selected_item_get(WIDGET(it));
   _item_unselect(selected);
}

static void
_menu_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   // avoid hide being emitted during object deletion
   evas_object_event_callback_del_full
      (obj, EVAS_CALLBACK_HIDE, _menu_hide, data);
}

static void
_menu_move_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   Evas_Coord x,y,w,h;
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));

   if ((!wd) || (!wd->menu_parent)) return;
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   elm_menu_move(it->o_menu, x, y+h);
}

static void
_item_disable(Elm_Toolbar_Item *it, Eina_Bool disabled)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));

   if (!wd) return;
   if (it->disabled == disabled) return;
   it->disabled = !!disabled;
   if (it->disabled)
     {
        edje_object_signal_emit(VIEW(it), "elm,state,disabled", "elm");
        elm_widget_signal_emit(it->icon, "elm,state,disabled", "elm");
     }
   else
     {
        edje_object_signal_emit(VIEW(it), "elm,state,enabled", "elm");
        elm_widget_signal_emit(it->icon, "elm,state,enabled", "elm");
     }
}

static void
_item_del(Elm_Toolbar_Item *it)
{
   Elm_Toolbar_Item_State *it_state;
   _item_unselect(it);
   elm_widget_item_pre_notify_del(it);
   EINA_LIST_FREE(it->states, it_state)
     {
        if (it->icon == it_state->icon)
          it->icon = NULL;
        eina_stringshare_del(it_state->label);
        eina_stringshare_del(it_state->icon_str);
        if (it_state->icon) evas_object_del(it_state->icon);
        free(it_state);
     }
   eina_stringshare_del(it->label);
   eina_stringshare_del(it->icon_str);
   if (it->icon) evas_object_del(it->icon);
   //TODO: See if checking for wd->menu_parent is necessary before deleting menu
   if (it->o_menu) evas_object_del(it->o_menu);
   elm_widget_item_del(it);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Toolbar_Item *it, *next;

   if (!wd) return;
   it = ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items);
   while(it)
     {
        next = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
        _item_del(it);
        it = next;
     }
   if (wd->more_item)
     _item_del(wd->more_item);
   if (wd->long_timer)
     {
        ecore_timer_del(wd->long_timer);
        wd->long_timer = NULL;
     }
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   free(wd);
}


static void
_mirrored_set_item(Evas_Object *obj __UNUSED__, Elm_Toolbar_Item *it, Eina_Bool mirrored)
{
   edje_object_mirrored_set(VIEW(it), mirrored);
   elm_widget_mirrored_set(it->o_menu, mirrored);
}

static void
_theme_hook_item(Evas_Object *obj, Elm_Toolbar_Item *it, double scale, int icon_size)
{
   Evas_Object *view = VIEW(it);
   Evas_Coord mw, mh;
   const char *style = elm_widget_style_get(obj);

   _mirrored_set_item(obj, it, elm_widget_mirrored_get(obj));
   edje_object_scale_set(view, scale);
   if (!it->separator)
     {
        _elm_theme_object_set(obj, view, "toolbar", "item", style);
        if (it->selected)
          {
             edje_object_signal_emit(view, "elm,state,selected", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,selected", "elm");
          }
        if (it->disabled)
          {
             edje_object_signal_emit(view, "elm,state,disabled", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,disabled", "elm");
          }
        if (it->icon)
          {
             int ms = 0;

             ms = ((double)icon_size * scale);
             evas_object_size_hint_min_set(it->icon, ms, ms);
             evas_object_size_hint_max_set(it->icon, ms, ms);
             edje_object_part_swallow(view, "elm.swallow.icon",
                                      it->icon);
          }
        edje_object_part_text_set(view, "elm.text", it->label);
     }
   else
     _elm_theme_object_set(obj, view, "toolbar", "separator", style);

   mw = mh = -1;
   if (!it->separator)
     elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(view, &mw, &mh, mw, mh);
   if (!it->separator)
     elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   evas_object_size_hint_min_set(view, mw, mh);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool mirrored)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Toolbar_Item *it;

   EINA_INLIST_FOREACH(wd->items, it)
      _mirrored_set_item(obj, it, mirrored);
   if (wd->more_item)
     _mirrored_set_item(obj, wd->more_item, mirrored);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Toolbar_Item *it;
   double scale = 0;

   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "toolbar", "base", elm_widget_style_get(obj));
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   scale = (elm_widget_scale_get(obj) * _elm_config->scale);
   edje_object_scale_set(wd->scr, scale);
   wd->icon_size = _elm_toolbar_icon_size_get(wd);
   EINA_INLIST_FOREACH(wd->items, it)
      _theme_hook_item(obj, it, scale, wd->icon_size);
   if (wd->more_item)
     _theme_hook_item(obj, wd->more_item, scale, wd->icon_size);
   _sizing_eval(obj);
}

static void
_translate_hook(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, "language,changed", NULL);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, minw_bx, minh_bx;
   Evas_Coord vw = 0, vh = 0;
   Evas_Coord w, h;

   if (!wd) return;
   evas_object_smart_calculate(wd->bx);
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &minw, &minh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   evas_object_resize(wd->scr, w, h);

   evas_object_size_hint_min_get(wd->bx, &minw, &minh);
   minw_bx = minw;
   minh_bx = minh;
   if (wd->vertical && (h > minh)) minh = h;
   if ((!wd->vertical) && (w > minw)) minw = w;
   evas_object_resize(wd->bx, minw, minh);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   if (wd->shrink_mode == ELM_TOOLBAR_SHRINK_NONE)
     {
        if (wd->vertical) minh = minh_bx + (h - vh);
        else minw = minw_bx + (w - vw);
     }
   else
     {
        if (wd->vertical) minh = h - vh;
        else minw = w - vw;
     }
   minh = minh + (h - vh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_item_menu_create(Widget_Data *wd, Elm_Toolbar_Item *item)
{
   item->o_menu = elm_menu_add(VIEW(item));
   item->menu = EINA_TRUE;
   if (wd->menu_parent)
     elm_menu_parent_set(item->o_menu, wd->menu_parent);
   evas_object_event_callback_add(item->o_menu, EVAS_CALLBACK_HIDE,
                                  _menu_hide, item);
   evas_object_event_callback_add(item->o_menu, EVAS_CALLBACK_DEL,
                                  _menu_del, item);
}

static void
_item_menu_destroy(Elm_Toolbar_Item *item)
{
   if (item->o_menu)
     {
        evas_object_del(item->o_menu);
        item->o_menu = NULL;
     }
   item->menu = EINA_FALSE;
}

static int
_toolbar_item_prio_compare_cb(const void *i1, const void *i2)
{
   const Elm_Toolbar_Item *eti1 = i1;
   const Elm_Toolbar_Item *eti2 = i2;

   if (!eti2) return 1;
   if (!eti1) return -1;

   return eti2->prio.priority - eti1->prio.priority;
}

static void
_fix_items_visibility(Widget_Data *wd, Evas_Coord *iw, Evas_Coord vw)
{
   Elm_Toolbar_Item *it;
   Eina_List *sorted = NULL;
   Evas_Coord ciw;

   EINA_INLIST_FOREACH(wd->items, it)
     {
        sorted = eina_list_sorted_insert(sorted,
                                         _toolbar_item_prio_compare_cb, it);
     }

   if (wd->more_item)
     {
        evas_object_geometry_get(wd->VIEW(more_item), NULL, NULL, &ciw, NULL);
        *iw += ciw;
     }
   EINA_LIST_FREE(sorted, it)
     {
        evas_object_geometry_get(VIEW(it), NULL, NULL, &ciw, NULL);
        *iw += ciw;
        it->prio.visible = (*iw <= vw);
     }
}

static void
_elm_toolbar_item_menu_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   if (it->func) it->func((void *)(it->base.data), WIDGET(it), it);
}

static void
_resize_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord mw, mh, vw, vh, w, h;
   Elm_Toolbar_Item *it;
   Evas_Object *obj = (Evas_Object *) data;

   if (!wd) return;
   wd->resize_job = NULL;
   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   evas_object_size_hint_min_get(wd->bx, &mw, &mh);
   evas_object_geometry_get(wd->bx, NULL, NULL, &w, &h);
   if (wd->shrink_mode == ELM_TOOLBAR_SHRINK_MENU)
     {
        Evas_Coord iw = 0, more_w;

        evas_object_resize(wd->bx, vw, h);
        _fix_items_visibility(wd, &iw, vw);
        evas_object_geometry_get(wd->VIEW(more_item), NULL, NULL, &more_w, NULL);
        if (iw - more_w <= vw)
          iw -= more_w;

        /* All items are removed from the box object, since removing individual
         * items won't trigger a resize. Items are be readded below. */
        evas_object_box_remove_all(wd->bx, EINA_FALSE);
        if (iw > vw)
          {
             Evas_Object *menu;

             _item_menu_destroy(wd->more_item);
             _item_menu_create(wd, wd->more_item);
             menu =
                elm_toolbar_item_menu_get((Elm_Object_Item *) wd->more_item);

             EINA_INLIST_FOREACH(wd->items, it)
               {
                  if (!it->prio.visible)
                    {
                       if (it->separator)
                         elm_menu_item_separator_add(menu, NULL);
                       else
                         {
                            Elm_Object_Item *menu_it;
                            menu_it = elm_menu_item_add(menu, NULL,
                                                it->icon_str, it->label,
                                                _elm_toolbar_item_menu_cb, it);
                            elm_object_item_disabled_set(menu_it, it->disabled);
                            if (it->o_menu)
                              elm_menu_clone(it->o_menu, menu, menu_it);
                         }
                       evas_object_hide(VIEW(it));
                    }
                  else
                    {
                       evas_object_box_append(wd->bx, VIEW(it));
                       evas_object_show(VIEW(it));
                    }
               }

             evas_object_box_append(wd->bx, wd->VIEW(more_item));
             evas_object_show(wd->VIEW(more_item));
          }
        else
          {
             /* All items are visible, show them all (except for the "More"
              * button, of course). */
             EINA_INLIST_FOREACH(wd->items, it)
               {
                  evas_object_show(VIEW(it));
                  evas_object_box_append(wd->bx, VIEW(it));
               }
             evas_object_hide(wd->VIEW(more_item));
          }
     }
   else if (wd->shrink_mode == ELM_TOOLBAR_SHRINK_HIDE)
     {
        Evas_Coord iw = 0;

        evas_object_resize(wd->bx, vw, h);
        _fix_items_visibility(wd, &iw, vw);
        evas_object_box_remove_all(wd->bx, EINA_FALSE);
        if (iw > vw)
          {
             EINA_INLIST_FOREACH(wd->items, it)
               {
                  if (!it->prio.visible)
                    evas_object_hide(VIEW(it));
                  else
                    {
                       evas_object_box_append(wd->bx, VIEW(it));
                       evas_object_show(VIEW(it));
                    }
               }
          }
        else
          {
             /* All items are visible, show them all */
             EINA_INLIST_FOREACH(wd->items, it)
               {
                  evas_object_show(VIEW(it));
                  evas_object_box_append(wd->bx, VIEW(it));
               }
          }
     }
   else
     {
        if ((vw >= mw) && (w != vw)) evas_object_resize(wd->bx, vw, h);
        EINA_INLIST_FOREACH(wd->items, it)
          {
             if (it->selected)
               {
                  _item_show(it);
                  break;
               }
          }
     }

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
}

static void
_resize_item(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
   _resize(data, NULL, NULL, NULL);
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd->resize_job)
     wd->resize_job = ecore_job_add(_resize_job, data);
}

static void
_select_filter(Elm_Toolbar_Item *it, Evas_Object *obj __UNUSED__, const char *emission, const char *source __UNUSED__)
{
   int button;
   char buf[sizeof("elm,action,click,") + 1];

   button = atoi(emission + sizeof("mouse,clicked,") - 1);
   if (button == 1) return; /* regular left click event */
   snprintf(buf, sizeof(buf), "elm,action,click,%d", button);
   edje_object_signal_emit(VIEW(it), buf, "elm");
}

static void
_select(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;

   if ((_elm_config->access_mode == ELM_ACCESS_MODE_OFF) ||
       (_elm_access_2nd_click_timeout(VIEW(it))))
     {
        if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
           _elm_access_say(E_("Selected"));
        _item_select(it);
     }
}

static Eina_Bool
_long_press(Elm_Toolbar_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   wd->long_timer = NULL;
   wd->long_press = EINA_TRUE;
   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, it);
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(Elm_Toolbar_Item *it, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Mouse_Down *ev)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   if (!wd) return;
   if (ev->button != 1) return;
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_DOUBLE, it);
   wd->long_press = EINA_FALSE;
   if (wd->long_timer) ecore_timer_interval_set(wd->long_timer, _elm_config->longpress_timeout);
   else wd->long_timer = ecore_timer_add(_elm_config->longpress_timeout, (Ecore_Task_Cb)_long_press, it);
}

static void
_mouse_up(Elm_Toolbar_Item *it, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Mouse_Up *ev)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   if (!wd) return;
   if (ev->button != 1) return;
   if (wd->long_timer)
     {
        ecore_timer_del(wd->long_timer);
        wd->long_timer = NULL;
     }
}

static void
_mouse_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   edje_object_signal_emit(VIEW(it), "elm,state,highlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,highlighted", "elm");
}

static void
_mouse_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   edje_object_signal_emit(VIEW(it), "elm,state,unhighlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,unhighlighted", "elm");
}

static void
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
   Evas_Object *obj = (Evas_Object *) data;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _els_box_layout(o, priv, !wd->vertical, wd->homogeneous, elm_widget_mirrored_get(obj));
}

static char *
_access_info_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, Elm_Widget_Item *item)
{
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)item;
   const char *txt = item->access_info;
   if (!txt) txt = it->label;
   if (txt) return strdup(txt);
   return NULL;
}

static char *
_access_state_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, Elm_Widget_Item *item __UNUSED__)
{
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)item;
   if (it->separator)
      return strdup(E_("Separator"));
   else if (it->disabled)
      return strdup(E_("State: Disabled"));
   else if (it->selected)
      return strdup(E_("State: Selected"));
   else if (it->menu)
      return strdup(E_("Has menu"));
   return NULL;
}

static Elm_Toolbar_Item *
_item_new(Evas_Object *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *icon_obj;
   Evas_Coord mw, mh;
   Elm_Toolbar_Item *it;

   icon_obj = elm_icon_add(obj);
   elm_icon_order_lookup_set(icon_obj, wd->lookup_order);
   if (!icon_obj) return NULL;
   it = elm_widget_item_new(obj, Elm_Toolbar_Item);
   if (!it)
     {
        evas_object_del(icon_obj);
        return NULL;
     }
   it->label = eina_stringshare_add(label);
   it->prio.visible = 1;
   it->prio.priority = 0;
   it->func = func;
   it->separator = EINA_FALSE;
   it->base.data = data;
   VIEW(it) = edje_object_add(evas_object_evas_get(obj));
   _elm_access_item_register(&it->base, VIEW(it));
   _elm_access_text_set(_elm_access_item_get(&it->base),
                        ELM_ACCESS_TYPE, E_("Tool Item"));
   _elm_access_callback_set(_elm_access_item_get(&it->base),
                            ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(_elm_access_item_get(&it->base),
                            ELM_ACCESS_STATE, _access_state_cb, it);

   if (_item_icon_set(icon_obj, "toolbar/", icon))
     {
        it->icon = icon_obj;
        it->icon_str = eina_stringshare_add(icon);
     }
   else
     {
        it->icon = NULL;
        it->icon_str = NULL;
        evas_object_del(icon_obj);
     }

   _elm_theme_object_set(obj, VIEW(it), "toolbar", "item",
                         elm_widget_style_get(obj));
   edje_object_signal_callback_add(VIEW(it), "elm,action,click", "elm",
                                   _select, it);
   edje_object_signal_callback_add(VIEW(it), "mouse,clicked,*", "*",
                                   (Edje_Signal_Cb)_select_filter, it);
   edje_object_signal_callback_add(VIEW(it), "elm,mouse,in", "elm",
                                   _mouse_in, it);
   edje_object_signal_callback_add(VIEW(it), "elm,mouse,out", "elm",
                                   _mouse_out, it);
   evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_DOWN,
                                  (Evas_Object_Event_Cb)_mouse_down, it);
   evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_UP,
                                  (Evas_Object_Event_Cb)_mouse_up, it);
   elm_widget_sub_object_add(obj, VIEW(it));
   if (it->icon)
     {
        int ms = 0;

        ms = ((double)wd->icon_size * _elm_config->scale);
        evas_object_size_hint_min_set(it->icon, ms, ms);
        evas_object_size_hint_max_set(it->icon, ms, ms);
        edje_object_part_swallow(VIEW(it), "elm.swallow.icon", it->icon);
        evas_object_show(it->icon);
        elm_widget_sub_object_add(obj, it->icon);
     }
   edje_object_part_text_set(VIEW(it), "elm.text", it->label);
   mw = mh = -1;
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(VIEW(it), &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   if (wd->vertical)
     {
        evas_object_size_hint_weight_set(VIEW(it), EVAS_HINT_EXPAND, -1.0);
        evas_object_size_hint_align_set(VIEW(it), EVAS_HINT_FILL, 0.5);
     }
   else
     {
        evas_object_size_hint_weight_set(VIEW(it), -1.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(VIEW(it), 0.5, EVAS_HINT_FILL);
     }
   evas_object_size_hint_min_set(VIEW(it), mw, mh);
   evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_RESIZE,
                                  _resize_item, obj);
   if ((!wd->items) && wd->always_select) _item_select(it);
   return it;
}

static void
_elm_toolbar_item_label_update(Elm_Toolbar_Item *item)
{
   Evas_Coord mw = -1, mh = -1;
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   edje_object_part_text_set(VIEW(item), "elm.text", item->label);

   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(VIEW(item), &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   if (wd->vertical)
     {
        evas_object_size_hint_weight_set(VIEW(item), EVAS_HINT_EXPAND, -1.0);
        evas_object_size_hint_align_set(VIEW(item), EVAS_HINT_FILL, 0.5);
     }
   else
     {
        evas_object_size_hint_weight_set(VIEW(item), -1.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(VIEW(item), 0.5, EVAS_HINT_FILL);
     }
   evas_object_size_hint_min_set(VIEW(item), mw, mh);
}

static void
_elm_toolbar_item_label_set_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Elm_Toolbar_Item *item = data;
   _elm_toolbar_item_label_update(item);
   edje_object_signal_callback_del(obj, emission, source,
                                   _elm_toolbar_item_label_set_cb);
   edje_object_signal_emit (VIEW(item), "elm,state,label,reset", "elm");
}

static void
_item_label_set(Elm_Toolbar_Item *item, const char *label, const char *signal)
{
   const char *s;

   if ((label) && (item->label) && (!strcmp(label, item->label))) return;

   eina_stringshare_replace(&item->label, label);
   s = edje_object_data_get(VIEW(item), "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        edje_object_part_text_set(VIEW(item), "elm.text_new", item->label);
        edje_object_signal_emit (VIEW(item), signal, "elm");
        edje_object_signal_callback_add(VIEW(item),
                                        "elm,state,label_set,done", "elm",
                                        _elm_toolbar_item_label_set_cb, item);
     }
   else
     _elm_toolbar_item_label_update(item);
   _resize(WIDGET(item), NULL, NULL, NULL);
}

static void
_elm_toolbar_item_icon_update(Elm_Toolbar_Item *item)
{
   Elm_Toolbar_Item_State *it_state;
   Eina_List *l;
   Evas_Coord mw = -1, mh = -1;
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   Evas_Object *old_icon = edje_object_part_swallow_get(VIEW(item),
                                                        "elm.swallow.icon");
   elm_widget_sub_object_del(VIEW(item), old_icon);
   evas_object_hide(old_icon);
   edje_object_part_swallow(VIEW(item), "elm.swallow.icon", item->icon);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(VIEW(item), &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   if (wd->vertical)
     {
        evas_object_size_hint_weight_set(VIEW(item), EVAS_HINT_EXPAND, -1.0);
        evas_object_size_hint_align_set(VIEW(item), EVAS_HINT_FILL, 0.5);
     }
   else
     {
        evas_object_size_hint_weight_set(VIEW(item), -1.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(VIEW(item), 0.5, EVAS_HINT_FILL);
     }
   evas_object_size_hint_min_set(VIEW(item), mw, mh);

   EINA_LIST_FOREACH(item->states, l, it_state)
      if (it_state->icon == old_icon)
        return;
   evas_object_del(old_icon);
}

static void
_elm_toolbar_item_icon_set_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Elm_Toolbar_Item *item = data;
   edje_object_part_unswallow(VIEW(item), item->icon);
   _elm_toolbar_item_icon_update(item);
   edje_object_signal_callback_del(obj, emission, source,
                                   _elm_toolbar_item_icon_set_cb);
   edje_object_signal_emit (VIEW(item), "elm,state,icon,reset", "elm");
}

static void
_elm_toolbar_item_icon_obj_set(Evas_Object *obj, Elm_Toolbar_Item *item, Evas_Object *icon_obj, const char *icon_str, double icon_size, const char *signal)
{
   Evas_Object *old_icon;
   int ms = 0;
   const char *s;

   if (icon_str)
     eina_stringshare_replace(&item->icon_str, icon_str);
   else
     {
        eina_stringshare_del(item->icon_str);
        item->icon_str = NULL;
     }
   item->icon = icon_obj;
   if (icon_obj)
     {
        ms = (icon_size * _elm_config->scale);
        evas_object_size_hint_min_set(item->icon, ms, ms);
        evas_object_size_hint_max_set(item->icon, ms, ms);
        evas_object_show(item->icon);
        elm_widget_sub_object_add(obj, item->icon);
     }
   s = edje_object_data_get(VIEW(item), "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        old_icon = edje_object_part_swallow_get(VIEW(item),
                                                "elm.swallow.icon_new");
        if (old_icon)
          {
             elm_widget_sub_object_del(VIEW(item), old_icon);
             evas_object_hide(old_icon);
          }
        edje_object_part_swallow(VIEW(item), "elm.swallow.icon_new",
                                 item->icon);
        edje_object_signal_emit (VIEW(item), signal, "elm");
        edje_object_signal_callback_add(VIEW(item),
                                        "elm,state,icon_set,done", "elm",
                                        _elm_toolbar_item_icon_set_cb, item);
     }
   else
     _elm_toolbar_item_icon_update(item);
   _resize(obj, NULL, NULL, NULL);
}

static void
_elm_toolbar_item_state_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Toolbar_Item *it = event_info;
   Elm_Toolbar_Item_State *it_state;

   it_state = eina_list_data_get(it->current_state);
   if (it_state->func)
     it_state->func((void *)it_state->data, obj, event_info);
}

static Elm_Toolbar_Item_State *
_item_state_new(const char *label, const char *icon_str, Evas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   Elm_Toolbar_Item_State *it_state;
   it_state = ELM_NEW(Elm_Toolbar_Item_State);
   it_state->label = eina_stringshare_add(label);
   it_state->icon_str = eina_stringshare_add(icon_str);
   it_state->icon = icon;
   it_state->func = func;
   it_state->data = data;
   return it_state;
}

EAPI Evas_Object *
elm_toolbar_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "toolbar");
   elm_widget_type_set(obj, "toolbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_translate_hook_set(obj, _translate_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   wd->more_item = NULL;
   wd->selected_item = NULL;
   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "toolbar", "base", "default");
   elm_smart_scroller_bounce_allow_set(wd->scr,
                                       _elm_config->thumbscroll_bounce_enable,
                                       EINA_FALSE);
   elm_widget_resize_object_set(obj, wd->scr);
   elm_smart_scroller_policy_set(wd->scr,
                                 ELM_SMART_SCROLLER_POLICY_AUTO,
                                 ELM_SMART_SCROLLER_POLICY_OFF);


   wd->icon_size = _elm_toolbar_icon_size_get(wd);


   wd->homogeneous = EINA_TRUE;
   wd->align = 0.5;

   wd->bx = evas_object_box_add(e);
   evas_object_size_hint_align_set(wd->bx, wd->align, 0.5);
   evas_object_box_layout_set(wd->bx, _layout, obj, NULL);
   elm_widget_sub_object_add(obj, wd->bx);
   elm_smart_scroller_child_set(wd->scr, wd->bx);
   evas_object_show(wd->bx);

   elm_toolbar_mode_shrink_set(obj, _elm_config->toolbar_shrink_mode);
   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_RESIZE, _resize, obj);
   evas_object_event_callback_add(wd->bx, EVAS_CALLBACK_RESIZE, _resize, obj);
   elm_toolbar_icon_order_lookup_set(obj, ELM_ICON_LOOKUP_THEME_FDO);

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   _sizing_eval(obj);
   return obj;
}

EAPI void
elm_toolbar_icon_size_set(Evas_Object *obj, int icon_size)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->icon_size == icon_size) return;
   wd->icon_size = icon_size;
   _theme_hook(obj);
}

EAPI int
elm_toolbar_icon_size_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->icon_size;
}

EAPI Elm_Object_Item *
elm_toolbar_item_append(Evas_Object *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   double scale = (elm_widget_scale_get(obj) * _elm_config->scale);

   wd->items = eina_inlist_append(wd->items, EINA_INLIST_GET(it));
   evas_object_box_append(wd->bx, VIEW(it));
   evas_object_show(VIEW(it));

   _theme_hook_item(obj, it, scale, wd->icon_size);
   _sizing_eval(obj);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_prepend(Evas_Object *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   double scale = (elm_widget_scale_get(obj) * _elm_config->scale);

   wd->items = eina_inlist_prepend(wd->items, EINA_INLIST_GET(it));
   evas_object_box_prepend(wd->bx, VIEW(it));
   evas_object_show(VIEW(it));
   _theme_hook_item(obj, it, scale, wd->icon_size);
   _sizing_eval(obj);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_insert_before(Evas_Object *obj, Elm_Object_Item *before, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_OBJ_ITEM_CHECK_OR_RETURN(before, NULL);
   Widget_Data *wd;
   Elm_Toolbar_Item *it, *_before;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   _before = (Elm_Toolbar_Item *) before;
   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   double scale = (elm_widget_scale_get(obj) * _elm_config->scale);

   wd->items = eina_inlist_prepend_relative(wd->items, EINA_INLIST_GET(it),
                                            EINA_INLIST_GET(_before));
   evas_object_box_insert_before(wd->bx, VIEW(it), VIEW(_before));
   evas_object_show(VIEW(it));
   _theme_hook_item(obj, it, scale, wd->icon_size);
   _sizing_eval(obj);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_insert_after(Evas_Object *obj, Elm_Object_Item *after, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_OBJ_ITEM_CHECK_OR_RETURN(after, NULL);
   Widget_Data *wd;
   Elm_Toolbar_Item *it, *_after;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   _after = (Elm_Toolbar_Item *) after;
   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   double scale = (elm_widget_scale_get(obj) * _elm_config->scale);

   wd->items = eina_inlist_append_relative(wd->items, EINA_INLIST_GET(it),
                                           EINA_INLIST_GET(_after));
   evas_object_box_insert_after(wd->bx, VIEW(it), VIEW(_after));
   evas_object_show(VIEW(it));
   _theme_hook_item(obj, it, scale, wd->icon_size);
   _sizing_eval(obj);

   return (Elm_Object_Item *) it;
}

EAPI Elm_Object_Item *
elm_toolbar_first_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return (Elm_Object_Item *) ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items);
}

EAPI Elm_Object_Item *
elm_toolbar_last_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return (Elm_Object_Item *) ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items->last);
}

EAPI Elm_Object_Item *
elm_toolbar_item_next_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return (Elm_Object_Item *) ELM_TOOLBAR_ITEM_FROM_INLIST(
      EINA_INLIST_GET(((Elm_Toolbar_Item *) it))->next);
}

EAPI Elm_Object_Item *
elm_toolbar_item_prev_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return (Elm_Object_Item *) ELM_TOOLBAR_ITEM_FROM_INLIST(
      EINA_INLIST_GET(((Elm_Toolbar_Item *) it))->prev);
}

EAPI Evas_Object *
elm_toolbar_item_toolbar_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_object_item_object_get(it);
}

EAPI void
elm_toolbar_item_priority_set(Elm_Object_Item *it, int priority)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   if (item->prio.priority == priority) return;
   item->prio.priority = priority;
   _resize(WIDGET(item), NULL, NULL, NULL);
}

EAPI int
elm_toolbar_item_priority_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, 0);
   return ((Elm_Toolbar_Item *) it)->prio.priority;
}

EAPI const char *
elm_toolbar_item_label_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Toolbar_Item *) it)->label;
}

EAPI void
elm_toolbar_item_label_set(Elm_Object_Item *it, const char *label)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   _item_label_set(((Elm_Toolbar_Item *) it), label, "elm,state,label_set");
}

EAPI void *
elm_toolbar_item_data_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_object_item_data_get(it);
}

EAPI void
elm_toolbar_item_data_set(Elm_Object_Item *it, const void *data)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_object_item_data_set(it, (void *) data);
}

EAPI Elm_Object_Item *
elm_toolbar_item_find_by_label(const Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Toolbar_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   EINA_INLIST_FOREACH(wd->items, it)
     {
        if (!strcmp(it->label, label))
          return (Elm_Object_Item *) it;
     }
   return NULL;
}

EAPI void
elm_toolbar_item_selected_set(Elm_Object_Item *it, Eina_Bool selected)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;

   if (item->selected == selected) return;
   if (selected) _item_select(item);
   else _item_unselect(item);
}

EAPI Eina_Bool
elm_toolbar_item_selected_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Toolbar_Item *) it)->selected;
}

EAPI Elm_Object_Item *
elm_toolbar_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return (Elm_Object_Item *) wd->selected_item;
}

EAPI void
elm_toolbar_item_icon_set(Elm_Object_Item *it, const char *icon)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);

   Evas_Object *icon_obj;
   Widget_Data *wd;
   Evas_Object *obj;
   Elm_Toolbar_Item * item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((icon) && (item->icon_str) && (!strcmp(icon, item->icon_str))) return;

   icon_obj = elm_icon_add(obj);
   if (!icon_obj) return;
   if (_item_icon_set(icon_obj, "toolbar/", icon))
     _elm_toolbar_item_icon_obj_set(obj, item, icon_obj, icon, wd->icon_size,
                                    "elm,state,icon_set");
   else
     {
        _elm_toolbar_item_icon_obj_set(obj, item, NULL, NULL, 0,
                                       "elm,state,icon_set");
        evas_object_del(icon_obj);
     }
}

EAPI const char *
elm_toolbar_item_icon_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Toolbar_Item *) it)->icon_str;
}

EAPI Evas_Object *
elm_toolbar_item_object_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);

   Widget_Data *wd;
   Evas_Object *obj;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return VIEW(item);
}

EAPI Evas_Object *
elm_toolbar_item_icon_object_get(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Toolbar_Item *) it)->icon;
}

EAPI Eina_Bool
elm_toolbar_item_icon_memfile_set(Elm_Object_Item *it, const void *img, size_t size, const char *format, const char *key)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   Evas_Object *icon_obj;
   Widget_Data *wd;
   Evas_Object *obj;
   Eina_Bool ret;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (img && size)
     {
        icon_obj = _els_smart_icon_add(evas_object_evas_get(obj));
        evas_object_repeat_events_set(icon_obj, EINA_TRUE);
        ret = _els_smart_icon_memfile_set(icon_obj, img, size, format, key);
        if (!ret)
          {
             evas_object_del(icon_obj);
             return EINA_FALSE;
          }
        _elm_toolbar_item_icon_obj_set(obj, item, icon_obj, NULL, wd->icon_size,
                                         "elm,state,icon_set");
     }
   else
     _elm_toolbar_item_icon_obj_set(obj, item, NULL, NULL, 0, "elm,state,icon_set");
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_toolbar_item_icon_file_set(Elm_Object_Item *it, const char *file, const char *key)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   Evas_Object *icon_obj;
   Widget_Data *wd;
   Evas_Object *obj;
   Eina_Bool ret;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (file)
     {
        icon_obj = _els_smart_icon_add(evas_object_evas_get(obj));
        evas_object_repeat_events_set(icon_obj, EINA_TRUE);
        ret = _els_smart_icon_file_key_set(icon_obj, file, key);
        if (!ret)
          {
             evas_object_del(icon_obj);
             return EINA_FALSE;
          }
        _elm_toolbar_item_icon_obj_set(obj, item, icon_obj, NULL, wd->icon_size,
                                         "elm,state,icon_set");
     }
   else
     _elm_toolbar_item_icon_obj_set(obj, item, NULL, NULL, 0, "elm,state,icon_set");
   return EINA_TRUE;
}

EAPI void
elm_toolbar_item_del(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);

   Widget_Data *wd;
   Evas_Object *obj2;
   Elm_Toolbar_Item *item, *next;
   item = (Elm_Toolbar_Item *) it;

   wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;
   obj2 = WIDGET(item);
   next = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(item)->next);
   wd->items = eina_inlist_remove(wd->items, EINA_INLIST_GET(item));
   if (!next) next = ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items);
   if (wd->always_select && item->selected && next) _item_select(next);
   _item_del(item);
   _theme_hook(obj2);
}

EAPI void
elm_toolbar_item_del_cb_set(Elm_Object_Item *it, Evas_Smart_Cb func)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_toolbar_item_del_cb_set(it, func);
}

EAPI Eina_Bool
elm_toolbar_item_disabled_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Widget_Item *) it)->disabled;
}

EAPI void
elm_toolbar_item_disabled_set(Elm_Object_Item *it, Eina_Bool disabled)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   _item_disable(item, disabled);
   _resize(WIDGET(item), NULL, NULL, NULL);
}

EAPI void
elm_toolbar_item_separator_set(Elm_Object_Item *it, Eina_Bool separator)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   Evas_Object *obj = WIDGET(item);
   Widget_Data *wd = elm_widget_data_get(obj);
   double scale;
   if (item->separator == separator) return;
   item->separator = separator;
   scale = (elm_widget_scale_get(obj) * _elm_config->scale);
   _theme_hook_item(obj, item, scale, wd->icon_size);
}

EAPI Eina_Bool
elm_toolbar_item_separator_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Toolbar_Item *) it)->separator;
}

EAPI void
elm_toolbar_mode_shrink_set(Evas_Object *obj, Elm_Toolbar_Shrink_Mode shrink_mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Bool bounce;

   if (!wd) return;
   wd->shrink_mode = shrink_mode;
   bounce = (_elm_config->thumbscroll_bounce_enable) &&
      (shrink_mode == ELM_TOOLBAR_SHRINK_SCROLL);
   elm_smart_scroller_bounce_allow_set(wd->scr, bounce, EINA_FALSE);

   if (wd->more_item)
     {
        _item_del(wd->more_item);
        wd->more_item = NULL;
     }

   if (shrink_mode == ELM_TOOLBAR_SHRINK_MENU)
     {
        elm_smart_scroller_policy_set(wd->scr, ELM_SMART_SCROLLER_POLICY_OFF, ELM_SMART_SCROLLER_POLICY_OFF);

        wd->more_item = _item_new(obj, "more_menu", "More",
                                  NULL, NULL);
     }
   else if (shrink_mode == ELM_TOOLBAR_SHRINK_HIDE)
     elm_smart_scroller_policy_set(wd->scr, ELM_SMART_SCROLLER_POLICY_OFF,
                                   ELM_SMART_SCROLLER_POLICY_OFF);
   else
     elm_smart_scroller_policy_set(wd->scr, ELM_SMART_SCROLLER_POLICY_AUTO,
                                   ELM_SMART_SCROLLER_POLICY_OFF);
   _sizing_eval(obj);
}

EAPI Elm_Toolbar_Shrink_Mode
elm_toolbar_mode_shrink_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_TOOLBAR_SHRINK_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ELM_TOOLBAR_SHRINK_NONE;
   return wd->shrink_mode;
}

EAPI void
elm_toolbar_homogeneous_set(Evas_Object *obj, Eina_Bool homogeneous)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->homogeneous = !!homogeneous;
   evas_object_smart_calculate(wd->bx);
}

EAPI Eina_Bool
elm_toolbar_homogeneous_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;
   return wd->homogeneous;
}

EAPI void
elm_toolbar_menu_parent_set(Evas_Object *obj, Evas_Object *parent)
{
   Elm_Toolbar_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(parent);
   wd->menu_parent = parent;
   EINA_INLIST_FOREACH(wd->items, it)
     {
        if (it->o_menu)
          elm_menu_parent_set(it->o_menu, wd->menu_parent);
     }
   if ((wd->more_item) && (wd->more_item->o_menu))
     elm_menu_parent_set(wd->more_item->o_menu, wd->menu_parent);
}

EAPI Evas_Object *
elm_toolbar_menu_parent_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->menu_parent;
}

EAPI void
elm_toolbar_align_set(Evas_Object *obj, double align)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->align != align)
     evas_object_size_hint_align_set(wd->bx, align, 0.5);
   wd->align = align;
}

EAPI double
elm_toolbar_align_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0.0;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0.0;
   return wd->align;
}

EAPI void
elm_toolbar_item_menu_set(Elm_Object_Item *it, Eina_Bool menu)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;

   if (item->menu == menu) return;
   if (menu) _item_menu_create(wd, item);
   else _item_menu_destroy(item);
}

EAPI Evas_Object *
elm_toolbar_item_menu_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   if (!item->menu) return NULL;
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return NULL;
   return item->o_menu;
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_add(Elm_Object_Item *it, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);

   Elm_Toolbar_Item_State *it_state;
   Evas_Object *icon_obj;
   Evas_Object *obj;
   Widget_Data *wd;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;
   obj = WIDGET(item);
   wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return NULL;

   if (!item->states)
     {
        it_state = _item_state_new(item->label, item->icon_str, item->icon,
                                   item->func, item->base.data);
        item->states = eina_list_append(item->states, it_state);
        item->current_state = item->states;
     }

   icon_obj = elm_icon_add(obj);
   elm_icon_order_lookup_set(icon_obj, wd->lookup_order);
   if (!icon_obj) goto error_state_add;

   if (!_item_icon_set(icon_obj, "toolbar/", icon))
     {
        evas_object_del(icon_obj);
        icon_obj = NULL;
        icon = NULL;
     }

   it_state = _item_state_new(label, icon, icon_obj, func, data);
   item->states = eina_list_append(item->states, it_state);
   item->func = _elm_toolbar_item_state_cb;
   item->base.data = NULL;

   return it_state;

error_state_add:
   if (item->states && !eina_list_next(item->states))
     {
        eina_stringshare_del(item->label);
        eina_stringshare_del(item->icon_str);
        free(eina_list_data_get(item->states));
        eina_list_free(item->states);
        item->states = NULL;
     }
   return NULL;
}

EAPI Eina_Bool
elm_toolbar_item_state_del(Elm_Object_Item *it, Elm_Toolbar_Item_State *state)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   Eina_List *del_state;
   Elm_Toolbar_Item_State *it_state;
   Elm_Toolbar_Item *item;

   if (!state) return EINA_FALSE;

   item = (Elm_Toolbar_Item *) it;
   if (!item->states) return EINA_FALSE;

   del_state = eina_list_data_find_list(item->states, state);
   if (del_state == item->states) return EINA_FALSE;
   if (del_state == item->current_state)
     elm_toolbar_item_state_unset(it);

   eina_stringshare_del(state->label);
   eina_stringshare_del(state->icon_str);
   if (state->icon) evas_object_del(state->icon);
   free(state);
   item->states = eina_list_remove_list(item->states, del_state);
   if (item->states && !eina_list_next(item->states))
     {
        it_state = eina_list_data_get(item->states);
        item->base.data = it_state->data;
        item->func = it_state->func;
        eina_stringshare_del(it_state->label);
        eina_stringshare_del(it_state->icon_str);
        free(eina_list_data_get(item->states));
        eina_list_free(item->states);
        item->states = NULL;
     }
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_toolbar_item_state_set(Elm_Object_Item *it, Elm_Toolbar_Item_State *state)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   Widget_Data *wd;
   Eina_List *next_state;
   Elm_Toolbar_Item_State *it_state;
   Evas_Object *obj;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (!item->states) return EINA_FALSE;

   if (state)
     {
        next_state = eina_list_data_find_list(item->states, state);
        if (!next_state) return EINA_FALSE;
     }
   else
     next_state = item->states;

   if (next_state == item->current_state) return EINA_TRUE;

   it_state = eina_list_data_get(next_state);
   if (eina_list_data_find(item->current_state, state))
     {
        _item_label_set(item, it_state->label, "elm,state,label_set,forward");
        _elm_toolbar_item_icon_obj_set(obj, item, it_state->icon, it_state->icon_str,
                                       wd->icon_size, "elm,state,icon_set,forward");
     }
   else
     {
        _item_label_set(item, it_state->label, "elm,state,label_set,backward");
        _elm_toolbar_item_icon_obj_set(obj,
                                       item,
                                       it_state->icon,
                                       it_state->icon_str,
                                       wd->icon_size,
                                       "elm,state,icon_set,backward");
     }
   if (item->disabled)
     elm_widget_signal_emit(item->icon, "elm,state,disabled", "elm");
   else
     elm_widget_signal_emit(item->icon, "elm,state,enabled", "elm");

   item->current_state = next_state;
   return EINA_TRUE;
}

EAPI void
elm_toolbar_item_state_unset(Elm_Object_Item *it)
{
   elm_toolbar_item_state_set(it, NULL);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   if ((!item->states) || (!item->current_state)) return NULL;
   if (item->current_state == item->states) return NULL;

   return eina_list_data_get(item->current_state);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_next(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);

   Widget_Data *wd;
   Evas_Object *obj;
   Eina_List *next_state;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!item->states) return NULL;

   next_state = eina_list_next(item->current_state);
   if (!next_state)
     next_state = eina_list_next(item->states);
   return eina_list_data_get(next_state);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_prev(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);

   Widget_Data *wd;
   Evas_Object *obj;
   Eina_List *prev_state;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *) it;

   obj = WIDGET(item);
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!item->states) return NULL;

   prev_state = eina_list_prev(item->current_state);
   if ((!prev_state) || (prev_state == item->states))
     prev_state = eina_list_last(item->states);
   return eina_list_data_get(prev_state);
}

EAPI void
elm_toolbar_item_tooltip_text_set(Elm_Object_Item *it, const char *text)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_tooltip_text_set(it, text);
}

EAPI void
elm_toolbar_item_tooltip_content_cb_set(Elm_Object_Item *it, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_tooltip_content_cb_set(it, func, data, del_cb);
}

EAPI void
elm_toolbar_item_tooltip_unset(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_tooltip_unset(it);
}

EAPI void
elm_toolbar_item_tooltip_style_set(Elm_Object_Item *it, const char *style)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_tooltip_style_set(it, style);
}

EAPI const char *
elm_toolbar_item_tooltip_style_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_widget_item_tooltip_style_get(it);
}

EAPI void
elm_toolbar_item_cursor_set(Elm_Object_Item *it, const char *cursor)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_cursor_set(it, cursor);
}

EAPI const char *
elm_toolbar_item_cursor_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_widget_item_cursor_get(it);
}

EAPI void
elm_toolbar_item_cursor_unset(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_cursor_unset(it);
}

EAPI void
elm_toolbar_item_cursor_style_set(Elm_Object_Item *it, const char *style)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_cursor_style_set(it, style);
}

EAPI const char *
elm_toolbar_item_cursor_style_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_widget_item_cursor_style_get(it);
}

EAPI void
elm_toolbar_item_cursor_engine_only_set(Elm_Object_Item *it, Eina_Bool engine_only)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_cursor_engine_only_set(it, engine_only);
}

EAPI Eina_Bool
elm_toolbar_item_cursor_engine_only_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return elm_widget_item_cursor_engine_only_get(it);
}

EAPI void
elm_toolbar_always_select_mode_set(Evas_Object *obj, Eina_Bool always_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (always_select && (!wd->always_select) && wd->items)
     _item_select(ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items));
   wd->always_select = always_select;
}

EAPI Eina_Bool
elm_toolbar_always_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->always_select;
}

EAPI void
elm_toolbar_no_select_mode_set(Evas_Object *obj, Eina_Bool no_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->no_select = no_select;
}

EAPI Eina_Bool
elm_toolbar_no_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->no_select;
}

EAPI void
elm_toolbar_icon_order_lookup_set(Evas_Object *obj, Elm_Icon_Lookup_Order order)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Elm_Toolbar_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->lookup_order = order;
   EINA_INLIST_FOREACH(wd->items, it)
      elm_icon_order_lookup_set(it->icon, order);
   if (wd->more_item)
     elm_icon_order_lookup_set(wd->more_item->icon, order);
}

EAPI Elm_Icon_Lookup_Order
elm_toolbar_icon_order_lookup_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_ICON_LOOKUP_THEME_FDO;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_ICON_LOOKUP_THEME_FDO;
   return wd->lookup_order;
}

EINA_DEPRECATED EAPI void
elm_toolbar_orientation_set(Evas_Object *obj, Eina_Bool vertical)
{
   elm_toolbar_horizontal_set(obj, !vertical);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_toolbar_orientation_get(const Evas_Object *obj)
{
   return !elm_toolbar_horizontal_get(obj);
}

EAPI void
elm_toolbar_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->vertical = !horizontal;
   _sizing_eval(obj);
}

EAPI Eina_Bool
elm_toolbar_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return !wd->vertical;
}
