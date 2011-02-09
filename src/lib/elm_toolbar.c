#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Toolbar Toolbar
 *
 * A toolbar is a widget that displays a list of buttons inside
 * a box.  It is scrollable, and only one item can be selected at a time.
 */

typedef struct _Widget_Data Widget_Data;

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
   Ecore_Job *resize_job;
};

struct _Elm_Toolbar_Item
{
   Elm_Widget_Item base;
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
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   Evas_Coord x, y, w, h, bx, by;

   if (!wd) return;
   evas_object_geometry_get(wd->bx, &bx, &by, NULL, NULL);
   evas_object_geometry_get(it->base.view, &x, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scr, x - bx, y - by, w, h);
}

static void
_item_unselect(Elm_Toolbar_Item *item)
{
   Widget_Data *wd;
   if ((!item) || (!item->selected)) return;
   wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   item->selected = EINA_FALSE;
   wd->selected_item = NULL;
   edje_object_signal_emit(item->base.view, "elm,state,unselected", "elm");
   elm_widget_signal_emit(item->icon, "elm,state,unselected", "elm");
}

static void
_item_select(Elm_Toolbar_Item *it)
{
   Elm_Toolbar_Item *it2;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   Evas_Object *obj2;

   if (!wd) return;
   if ((it->selected) || (it->disabled) || (it->separator)) return;

   if (!wd->no_select)
     {
        it2 = elm_toolbar_selected_item_get(it->base.widget);
        _item_unselect(it2);

        it->selected = EINA_TRUE;
        wd->selected_item = it;
        edje_object_signal_emit(it->base.view, "elm,state,selected", "elm");
        elm_widget_signal_emit(it->icon, "elm,state,selected", "elm");
        _item_show(it);
     }
   obj2 = it->base.widget;
   if (it->menu)
     {
        evas_object_show(it->o_menu);
        evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_RESIZE,
                                       _menu_move_resize, it);
        evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOVE,
                                       _menu_move_resize, it);

        _menu_move_resize(it, NULL, NULL, NULL);
     }
   if (it->func) it->func((void *)(it->base.data), it->base.widget, it);
   evas_object_smart_callback_call(obj2, "clicked", it);
}

static void
_menu_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *selected;
   Elm_Toolbar_Item *it = data;
   selected = elm_toolbar_selected_item_get(it->base.widget);
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
    Widget_Data *wd = elm_widget_data_get(it->base.widget);

    if ((!wd) || (!wd->menu_parent)) return;
    evas_object_geometry_get(it->base.view, &x, &y, &w, &h);
    elm_menu_move(it->o_menu, x, y+h);
}

static void
_item_disable(Elm_Toolbar_Item *it, Eina_Bool disabled)
{
   Widget_Data *wd = elm_widget_data_get(it->base.widget);

   if (!wd) return;
   if (it->disabled == disabled) return;
   it->disabled = disabled;
   if (it->disabled)
     {
        edje_object_signal_emit(it->base.view, "elm,state,disabled", "elm");
        elm_widget_signal_emit(it->icon, "elm,state,disabled", "elm");
     }
   else
     {
        edje_object_signal_emit(it->base.view, "elm,state,enabled", "elm");
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
   edje_object_mirrored_set(it->base.view, mirrored);
   elm_widget_mirrored_set(it->o_menu, mirrored);
}

static void
_theme_hook_item(Evas_Object *obj, Elm_Toolbar_Item *it, double scale, int icon_size)
{
   Evas_Object *view = it->base.view;
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
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, minw_bx;
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
   if (w > minw) minw = w;
   evas_object_resize(wd->bx, minw, minh);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   switch (wd->shrink_mode)
     {
       case ELM_TOOLBAR_SHRINK_MENU: /* fallthrough */
       case ELM_TOOLBAR_SHRINK_HIDE: /* fallthrough */
       case ELM_TOOLBAR_SHRINK_SCROLL: minw = w - vw; break;
       case ELM_TOOLBAR_SHRINK_NONE: minw = minw_bx + (w - vw); break;
     }
   minh = minh + (h - vh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_item_menu_create(Widget_Data *wd, Elm_Toolbar_Item *item)
{
   item->o_menu = elm_menu_add(item->base.view);
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
        evas_object_geometry_get(wd->more_item->base.view, NULL, NULL, &ciw, NULL);
        *iw += ciw;
     }
   EINA_LIST_FREE(sorted, it)
     {
        evas_object_geometry_get(it->base.view, NULL, NULL, &ciw, NULL);
        *iw += ciw;
        it->prio.visible = (*iw <= vw);
     }
}

static void
_elm_toolbar_item_menu_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   if (it->func) it->func((void *)(it->base.data), it->base.widget, it);
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
        evas_object_geometry_get(wd->more_item->base.view, NULL, NULL, &more_w, NULL);
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
             menu = elm_toolbar_item_menu_get(wd->more_item);

             EINA_INLIST_FOREACH(wd->items, it)
               {
                 if (!it->prio.visible)
                    {
                       if (it->separator)
                         elm_menu_item_separator_add(menu, NULL);
                       else
                         {
                            Elm_Menu_Item *item;
                            item = elm_menu_item_add(menu, NULL, it->icon_str, it->label,
                                                     _elm_toolbar_item_menu_cb, it);
                            elm_menu_item_disabled_set(item, it->disabled);
                            if (it->o_menu) elm_menu_clone(it->o_menu, menu, item);
                         }
                       evas_object_hide(it->base.view);
                    }
                 else
                    {
                       evas_object_box_append(wd->bx, it->base.view);
                       evas_object_show(it->base.view);
                    }
               }

             evas_object_box_append(wd->bx, wd->more_item->base.view);
             evas_object_show(wd->more_item->base.view);
          }
        else
          {
             /* All items are visible, show them all (except for the "More"
              * button, of course). */
             EINA_INLIST_FOREACH(wd->items, it)
               {
                  evas_object_show(it->base.view);
                  evas_object_box_append(wd->bx, it->base.view);
               }
             evas_object_hide(wd->more_item->base.view);
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
                   evas_object_hide(it->base.view);
                 else
                   {
                      evas_object_box_append(wd->bx, it->base.view);
                      evas_object_show(it->base.view);
                   }
               }
          }
        else
          {
             /* All items are visible, show them all */
             EINA_INLIST_FOREACH(wd->items, it)
               {
                  evas_object_show(it->base.view);
                  evas_object_box_append(wd->bx, it->base.view);
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
_select(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _item_select(data);
}

static void
_mouse_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   edje_object_signal_emit(it->base.view, "elm,state,highlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,highlighted", "elm");
}

static void
_mouse_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   edje_object_signal_emit(it->base.view, "elm,state,unhighlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,unhighlighted", "elm");
}

static void
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
   Evas_Object *obj = (Evas_Object *) data;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _els_box_layout(o, priv, 1, wd->homogeneous, elm_widget_mirrored_get(obj));
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
   it->base.view = edje_object_add(evas_object_evas_get(obj));
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

   _elm_theme_object_set(obj, it->base.view, "toolbar", "item",
                         elm_widget_style_get(obj));
   edje_object_signal_callback_add(it->base.view, "elm,action,click", "elm",
                                   _select, it);
   edje_object_signal_callback_add(it->base.view, "elm,mouse,in", "elm",
				   _mouse_in, it);
   edje_object_signal_callback_add(it->base.view, "elm,mouse,out", "elm",
				   _mouse_out, it);
   elm_widget_sub_object_add(obj, it->base.view);
   if (it->icon)
     {
        int ms = 0;

        ms = ((double)wd->icon_size * _elm_config->scale);
        evas_object_size_hint_min_set(it->icon, ms, ms);
        evas_object_size_hint_max_set(it->icon, ms, ms);
        edje_object_part_swallow(it->base.view, "elm.swallow.icon", it->icon);
        evas_object_show(it->icon);
        elm_widget_sub_object_add(obj, it->icon);
     }
   edje_object_part_text_set(it->base.view, "elm.text", it->label);
   mw = mh = -1;
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(it->base.view, &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   evas_object_size_hint_weight_set(it->base.view, -1.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(it->base.view, 0.5, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(it->base.view, mw, mh);
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_RESIZE,
                                  _resize_item, obj);
   return it;
}

/**
 * Add a toolbar object to @p parent.
 *
 * @param parent The parent object
 *
 * @return The created object, or NULL on failure
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *
elm_toolbar_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "toolbar");
   elm_widget_type_set(obj, "toolbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
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

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the icon size (in pixels) for the toolbar.
 *
 * @param obj The toolbar object
 * @param icon_size The icon size in pixels
 *
 * @ingroup Toolbar
 */
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

/**
 * Get the icon size (in pixels) for the toolbar.
 *
 * @param obj The toolbar object
 * @return The icon size in pixels
 *
 * @ingroup Toolbar
 */
EAPI int
elm_toolbar_icon_size_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->icon_size;
}

/**
 * Append item to the toolbar
 *
 * @param obj The toolbar object
 * @param icon A string with icon name or the absolute path of an image file.
 * @param label The label of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item
 * @return The toolbar item, or NULL upon failure
 *
 * @see elm_toolbar_item_icon_set
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_append(Evas_Object *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;

   wd->items = eina_inlist_append(wd->items, EINA_INLIST_GET(it));
   evas_object_box_append(wd->bx, it->base.view);
   evas_object_show(it->base.view);
   _sizing_eval(obj);

   return it;
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

/**
 * Sets the next @p item state as the current state.
 *
 * @param item The item.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_next(Elm_Toolbar_Item *item)
{
   Widget_Data *wd;
   Evas_Object *obj;
   Eina_List *next_state;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);

   obj = item->base.widget;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!item->states) return NULL;

   next_state = eina_list_next(item->current_state);
   if (!next_state)
      next_state = eina_list_next(item->states);
   return eina_list_data_get(next_state);
}

/**
 * Sets the previous @p item state as the current state.
 *
 * @param item The item.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_prev(Elm_Toolbar_Item *item)
{
   Widget_Data *wd;
   Evas_Object *obj;
   Eina_List *prev_state;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);

   obj = item->base.widget;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!item->states) return NULL;

   prev_state = eina_list_prev(item->current_state);
   if ((!prev_state) || (prev_state == item->states))
      prev_state = eina_list_last(item->states);
   return eina_list_data_get(prev_state);
}

/**
 * Unset the state of @p it
 * The default icon and label from this item will be displayed.
 *
 * @param it The item.
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_state_unset(Elm_Toolbar_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it);
   elm_toolbar_item_state_set(it, NULL);
}

/**
 * Sets @p state as the current state of @p it.
 * If @p state is NULL, it won't select any state and the default icon and
 * label will be used.
 *
 * @param it The item.
 * @param state The state to use.
 *
 * @return True if the state was correctly set.
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_item_state_set(Elm_Toolbar_Item *it, Elm_Toolbar_Item_State *state)
{
   Widget_Data *wd;
   Eina_List *next_state;
   Elm_Toolbar_Item_State *it_state;
   Evas_Object *obj;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, EINA_FALSE);

   obj = it->base.widget;
   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (!it->states) return EINA_FALSE;

   if (state)
     {
        next_state = eina_list_data_find_list(it->states, state);
        if (!next_state) return EINA_FALSE;
     }
   else
      next_state = it->states;

   if (next_state == it->current_state) return EINA_TRUE;

   it_state = eina_list_data_get(next_state);
   if (eina_list_data_find(it->current_state, state))
     {
        _item_label_set(it, it_state->label, "elm,state,label_set,forward");
        _elm_toolbar_item_icon_obj_set(obj, it, it_state->icon, it_state->icon_str,
                                       wd->icon_size, "elm,state,icon_set,forward");
     }
   else
     {
        _item_label_set(it, it_state->label, "elm,state,label_set,backward");
        _elm_toolbar_item_icon_obj_set(obj, it, it_state->icon, it_state->icon_str,
                                       wd->icon_size, "elm,state,icon_set,backward");
     }
   if (it->disabled)
        elm_widget_signal_emit(it->icon, "elm,state,disabled", "elm");
   else
        elm_widget_signal_emit(it->icon, "elm,state,enabled", "elm");

   it->current_state = next_state;
   return EINA_TRUE;
}

/**
 * Get the current state of @p item.
 * If no state is selected, returns NULL.
 *
 * @param item The item.
 *
 * @return The state.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_get(const Elm_Toolbar_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, NULL);
   if ((!it->states) || (!it->current_state)) return NULL;
   if (it->current_state == it->states) return NULL;

   return eina_list_data_get(it->current_state);
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

/**
 * Add a new state to @p item
 *
 * @param item The item.
 * @param icon The icon string
 * @param label The label of the new state
 * @param func The function to call when the item is clicked when this state is
 * selected.
 * @param data The data to associate with the state
 * @return The toolbar item state, or NULL upon failure
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_add(Elm_Toolbar_Item *item, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   Elm_Toolbar_Item_State *it_state;
   Evas_Object *icon_obj;
   Evas_Object *obj;
   Widget_Data *wd;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   obj = item->base.widget;
   wd = elm_widget_data_get(item->base.widget);
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
elm_toolbar_item_state_del(Elm_Toolbar_Item *item, Elm_Toolbar_Item_State *state)
{
   Eina_List *del_state;
   Elm_Toolbar_Item_State *it_state;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);

   if (!state) return EINA_FALSE;
   if (!item->states) return EINA_FALSE;

   del_state = eina_list_data_find_list(item->states, state);
   if (del_state == item->states) return EINA_FALSE;
   if (del_state == item->current_state)
      elm_toolbar_item_state_unset(item);

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


/**
 * Prepend item to the toolbar
 *
 * @param obj The toolbar object
 * @param icon A string with icon name or the absolute path of an image file.
 * @param label The label of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item
 * @return The toolbar item, or NULL upon failure
 *
 * @see elm_toolbar_item_icon_set
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_prepend(Evas_Object *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;

   wd->items = eina_inlist_prepend(wd->items, EINA_INLIST_GET(it));
   evas_object_box_prepend(wd->bx, it->base.view);
   evas_object_show(it->base.view);
   _sizing_eval(obj);

   return it;
}

/**
 * Insert item before another in the toolbar
 *
 * @param obj The toolbar object
 * @param before The item to insert before
 * @param icon A string with icon name or the absolute path of an image file.
 * @param label The label of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item
 * @return The toolbar item, or NULL upon failure
 *
 * @see elm_toolbar_item_icon_set
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_insert_before(Evas_Object *obj, Elm_Toolbar_Item *before, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(before, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;

   wd->items = eina_inlist_prepend_relative(wd->items, EINA_INLIST_GET(it),
                                            EINA_INLIST_GET(before));
   evas_object_box_insert_before(wd->bx, it->base.view, before->base.view);
   evas_object_show(it->base.view);
   _sizing_eval(obj);

   return it;
}

/**
 * Insert item after another in the toolbar
 *
 * @param obj The toolbar object
 * @param after The item to insert after
 * @param icon A string with icon name or the absolute path of an image file.
 * @param label The label of the item
 * @param func The function to call when the item is clicked
 * @param data The data to associate with the item
 * @return The toolbar item, or NULL upon failure
 *
 * @see elm_toolbar_item_icon_set
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_insert_after(Evas_Object *obj, Elm_Toolbar_Item *after, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(after, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Toolbar_Item *it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;

   wd->items = eina_inlist_append_relative(wd->items, EINA_INLIST_GET(it),
                                            EINA_INLIST_GET(after));
   evas_object_box_insert_after(wd->bx, it->base.view, after->base.view);
   evas_object_show(it->base.view);
   _sizing_eval(obj);

   return it;
}

/**
 * Get the first item in the toolbar
 *
 * @param obj The toolbar object
 * @return The first item, or NULL if none
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_first_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   Elm_Toolbar_Item *it = ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items);
   return it;
}

/**
 * Get the last item in the toolbar
 *
 * @return The last item, or NULL if none
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_last_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   Elm_Toolbar_Item *it = ELM_TOOLBAR_ITEM_FROM_INLIST(wd->items->last);
   return it;
}

/**
 * Get the next item in the toolbar
 *
 * This returns the item after the item @p it.
 *
 * @param item The item
 * @return The item after @p it, or NULL if none
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_next_get(const Elm_Toolbar_Item *item)
{
   Elm_Toolbar_Item *next;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   next = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(item)->next);
   return next;
}

/**
 * Get the previous item in the toolbar
 *
 * This returns the item before the item @p it.
 *
 * @param item The item
 * @return The item before @p it, or NULL if none
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_prev_get(const Elm_Toolbar_Item *item)
{
   Elm_Toolbar_Item *prev;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   prev = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(item)->prev);
   return prev;
}

/**
 * Get the toolbar object from an item
 *
 * This returns the toolbar object itself that an item belongs to.
 *
 * @param item The item
 * @return The toolbar object
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *
elm_toolbar_item_toolbar_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->base.widget;
}

/**
 * Sets the priority of a toolbar item. This is used only when the toolbar
 * shrink mode is set to ELM_TOOLBAR_SHRINK_MENU or ELM_TOOLBAR_SHRINK_HIDE:
 * when space is at a premium, items with low priority will be removed from
 * the toolbar and added to a dynamically-created menu, while items with
 * higher priority will remain on the toolbar, with the same order they were
 * added.
 *
 * @param item The toolbar item.
 * @param priority The item priority. The default is zero.
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_priority_set(Elm_Toolbar_Item *item, int priority)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
    if (item->prio.priority == priority) return;
    item->prio.priority = priority;
    _resize(item->base.widget, NULL, NULL, NULL);
}

/**
 * Gets the priority of a toolbar item.
 *
 * @param item The toolbar item.
 * @return The item priority, or 0 if an error occurred.
 *
 * @ingroup Toolbar
 */
EAPI int
elm_toolbar_item_priority_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, 0);
   return item->prio.priority;
}

/**
 * Get the string used to set the icon of @p item.
 *
 * @param item The toolbar item
 * @return The string associated with the icon object.
 *
 * @see elm_toolbar_item_icon_set()
 *
 * @ingroup Toolbar
 */
EAPI const char *
elm_toolbar_item_icon_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->icon_str;
}

/**
 * Get the label associated with @p item.
 *
 * @param item The toolbar item
 * @return The label
 *
 * @ingroup Toolbar
 */
EAPI const char *
elm_toolbar_item_label_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->label;
}

static void
_elm_toolbar_item_label_update(Elm_Toolbar_Item *item)
{
   Evas_Coord mw = -1, mh = -1;
   edje_object_part_text_set(item->base.view, "elm.text", item->label);

   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(item->base.view, &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   evas_object_size_hint_weight_set(item->base.view, -1.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(item->base.view, 0.5, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(item->base.view, mw, mh);
}

static void
_elm_toolbar_item_label_set_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Elm_Toolbar_Item *item = data;
   _elm_toolbar_item_label_update(item);
   edje_object_signal_callback_del(obj, emission, source,
                                   _elm_toolbar_item_label_set_cb);
   edje_object_signal_emit (item->base.view, "elm,state,label,reset", "elm");
}

static void
_item_label_set(Elm_Toolbar_Item *item, const char *label, const char *signal)
{
   const char *s;

   if ((label) && (item->label) && (!strcmp(label, item->label))) return;

   eina_stringshare_replace(&item->label, label);
   s = edje_object_data_get(item->base.view, "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        edje_object_part_text_set(item->base.view, "elm.text_new", item->label);
        edje_object_signal_emit (item->base.view, signal, "elm");
        edje_object_signal_callback_add(item->base.view,
                                        "elm,state,label_set,done", "elm",
                                        _elm_toolbar_item_label_set_cb, item);
     }
   else
      _elm_toolbar_item_label_update(item);
   _resize(item->base.widget, NULL, NULL, NULL);
}

/**
 * Set the label associated with @p item.
 *
 * @param item The toolbar item
 * @param label The label of @p item
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_label_set(Elm_Toolbar_Item *item, const char *label)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   _item_label_set(item, label, "elm,state,label_set");
}

static void
_elm_toolbar_item_icon_update(Elm_Toolbar_Item *item)
{
   Elm_Toolbar_Item_State *it_state;
   Eina_List *l;
   Evas_Coord mw = -1, mh = -1;
   Evas_Object *old_icon = edje_object_part_swallow_get(item->base.view,
                                                        "elm.swallow.icon");
   elm_widget_sub_object_del(item->base.view, old_icon);
   evas_object_hide(old_icon);
   edje_object_part_swallow(item->base.view, "elm.swallow.icon", item->icon);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   edje_object_size_min_restricted_calc(item->base.view, &mw, &mh, mw, mh);
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   evas_object_size_hint_weight_set(item->base.view, -1.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(item->base.view, 0.5, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(item->base.view, mw, mh);

   EINA_LIST_FOREACH(item->states, l, it_state)
      if (it_state->icon == old_icon)
         return;
   evas_object_del(old_icon);
}

/**
 * Get the selected state of @p item.
 *
 * @param item The toolbar item
 * @return If true, the item is selected
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_item_selected_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->selected;
}

/**
 * Set the selected state of an item
 *
 * This sets the selected state (1 selected, 0 not selected) of the given
 * item @p it. If a new item is selected the previosly selected will be
 * unselected.
 *
 * @param item The item
 * @param selected The selected state
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_selected_set(Elm_Toolbar_Item *item, Eina_Bool selected)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;

   if (item->selected == selected) return;

   if (selected)
     _item_select(item);
   else
     _item_unselect(item);
}

/**
 * Get the selectd item in the toolbar
 *
 * If no item is selected, NULL is returned.
 *
 * @param obj The toolbar object
 * @return The selected item, or NULL if none.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->selected_item;
}

static void
_elm_toolbar_item_icon_set_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Elm_Toolbar_Item *item = data;
   edje_object_part_unswallow(item->base.view, item->icon);
   _elm_toolbar_item_icon_update(item);
   edje_object_signal_callback_del(obj, emission, source,
                                   _elm_toolbar_item_icon_set_cb);
   edje_object_signal_emit (item->base.view, "elm,state,icon,reset", "elm");
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
   s = edje_object_data_get(item->base.view, "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        old_icon = edje_object_part_swallow_get(item->base.view,
                                                "elm.swallow.icon_new");
        if (old_icon)
          {
             elm_widget_sub_object_del(item->base.view, old_icon);
             evas_object_hide(old_icon);
          }
        edje_object_part_swallow(item->base.view, "elm.swallow.icon_new",
                                 item->icon);
        edje_object_signal_emit (item->base.view, signal, "elm");
        edje_object_signal_callback_add(item->base.view,
                                        "elm,state,icon_set,done", "elm",
                                        _elm_toolbar_item_icon_set_cb, item);
     }
   else
      _elm_toolbar_item_icon_update(item);
   _resize(obj, NULL, NULL, NULL);
}

/**
 * Set the icon associated with @p item.
 *
 * Toolbar will load icon image from fdo or current theme.
 * This behavior can be set by elm_toolbar_icon_order_lookup_set() function.
 * If an absolute path is provided it will load it direct from a file.
 *
 * @param obj The parent of this item
 * @param item The toolbar item
 * @param icon A string with icon name or the absolute path of an image file.
 *
 * @see elm_toolbar_icon_order_lookup_set(), elm_toolbar_icon_order_lookup_get()
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_icon_set(Elm_Toolbar_Item *item, const char *icon)
{
   Evas_Object *icon_obj;
   Widget_Data *wd;
   Evas_Object *obj = item->base.widget;

   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
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

/**
 * Delete a toolbar item.
 *
 * @param item The toolbar item
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_del(Elm_Toolbar_Item *item)
{
   Widget_Data *wd;
   Evas_Object *obj2;

   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   obj2 = item->base.widget;
   wd->items = eina_inlist_remove(wd->items, EINA_INLIST_GET(item));
   _item_del(item);
   _theme_hook(obj2);
}

/**
 * Set the function called when a toolbar item is freed.
 *
 * @param item The item to set the callback on
 * @param func The function called
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_del_cb_set(Elm_Toolbar_Item *item, Evas_Smart_Cb func)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_del_cb_set(item, func);
}

/**
 * Get the disabled state of @p item.
 *
 * @param item The toolbar item
 * @return If true, the item is disabled
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_item_disabled_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->disabled;
}

/**
 * Set the disabled state of @p item.
 *
 * @param item The toolbar item
 * @param disabled If true, the item is disabled
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_disabled_set(Elm_Toolbar_Item *item, Eina_Bool disabled)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   _item_disable(item, disabled);
   _resize(item->base.widget, NULL, NULL, NULL);
}

/**
 * Get the separator state of @p item.
 *
 * @param item The toolbar item
 * @param separator If true, the item is a separator
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_separator_set(Elm_Toolbar_Item *item, Eina_Bool separator)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   if (item->separator == separator) return;
   item->separator = separator;
   _theme_hook(item->base.view);
}

/**
 * Set the separator state of @p item.
 *
 * @param item The toolbar item
 * @return If true, the item is a separator
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_item_separator_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->separator;
}

/**
 * Set the shrink state of toolbar @p obj.
 *
 * @param obj The toolbar object
 * @param shrink_mode The toolbar won't scroll if ELM_TOOLBAR_SHRINK_NONE,
 * but will enforce a minimun size so all the items will fit, won't scroll
 * and won't show the items that don't fit if ELM_TOOLBAR_SHRINK_HIDE,
 * will scroll if ELM_TOOLBAR_SHRINK_SCROLL, and will create a button to
 * pop up excess elements with ELM_TOOLBAR_SHRINK_MENU.
 *
 * @ingroup Toolbar
 */
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

/**
 * Get the shrink mode of toolbar @p obj.
 *
 * @param obj The toolbar object
 * @return See elm_toolbar_mode_shrink_set.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Shrink_Mode
elm_toolbar_mode_shrink_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_TOOLBAR_SHRINK_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ELM_TOOLBAR_SHRINK_NONE;
   return wd->shrink_mode;
}

/**
 * Set the homogenous mode of toolbar @p obj.
 *
 * @param obj The toolbar object
 * @param homogenous If true, the toolbar items will be uniform in size
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_homogenous_set(Evas_Object *obj, Eina_Bool homogenous)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->homogeneous = !!homogenous;
   evas_object_smart_calculate(wd->bx);
}

/**
 * Get the homogenous mode of toolbar @p obj.
 *
 * @param obj The toolbar object
 * @return If true, the toolbar items are uniform in size
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_homogenous_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;
   return wd->homogeneous;
}

/**
 * Set the parent object of the toolbar menu
 *
 * @param obj The toolbar object
 * @param parent The parent of the menu object
 *
 * @ingroup Toolbar
 */
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

/**
 * Get the parent object of the toolbar menu
 *
 * @param obj The toolbar object
 * @return The parent of the menu object
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *
elm_toolbar_menu_parent_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->menu_parent;
}

/**
 * Set the alignment of the items.
 *
 * @param obj The toolbar object
 * @param align The new alignment. (left) 0.0 ... 1.0 (right)
 *
 * @ingroup Toolbar
 */
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

/**
 * Get the alignment of the items.
 *
 * @param obj The toolbar object
 * @return The alignment. (left) 0.0 ... 1.0 (right)
 *
 * @ingroup Toolbar
 */
EAPI double
elm_toolbar_align_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0.0;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0.0;
   return wd->align;
}

/**
 * Set whether the toolbar item opens a menu.
 *
 * @param item The toolbar item
 * @param menu If true, @p item will open a menu when selected
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_menu_set(Elm_Toolbar_Item *item, Eina_Bool menu)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;

   if (item->menu == menu) return;
   item->menu = menu;
   if (menu) _item_menu_create(wd, item);
   else _item_menu_destroy(item);
}

/**
 * Set the text to be shown in the toolbar item.
 *
 * @param item Target item
 * @param text The text to set in the content
 *
 * Setup the text as tooltip to object. The item can have only one tooltip,
 * so any previous tooltip data is removed.
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_tooltip_text_set(Elm_Toolbar_Item *item, const char *text)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
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
 * @param item the toolbar item being attached a tooltip.
 * @param func the function used to create the tooltip contents.
 * @param data what to provide to @a func as callback data/context.
 * @param del_cb called when data is not needed anymore, either when
 *        another callback replaces @func, the tooltip is unset with
 *        elm_toolbar_item_tooltip_unset() or the owner @a item
 *        dies. This callback receives as the first parameter the
 *        given @a data, and @c event_info is the item.
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_tooltip_content_cb_set(Elm_Toolbar_Item *item, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_content_cb_set(item, func, data, del_cb);
}

/**
 * Unset tooltip from item
 *
 * @param item toolbar item to remove previously set tooltip.
 *
 * Remove tooltip from item. The callback provided as del_cb to
 * elm_toolbar_item_tooltip_content_cb_set() will be called to notify
 * it is not used anymore.
 *
 * @see elm_toolbar_item_tooltip_content_cb_set()
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_tooltip_unset(Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_unset(item);
}

/**
 * Sets a different style for this item tooltip.
 *
 * @note before you set a style you should define a tooltip with
 *       elm_toolbar_item_tooltip_content_cb_set() or
 *       elm_toolbar_item_tooltip_text_set()
 *
 * @param item toolbar item with tooltip already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_tooltip_style_set(Elm_Toolbar_Item *item, const char *style)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_style_set(item, style);
}

/**
 * Get the style for this item tooltip.
 *
 * @param item toolbar item with tooltip already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a tooltip set, then NULL is returned.
 *
 * @ingroup Toolbar
 */
EAPI const char *
elm_toolbar_item_tooltip_style_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_tooltip_style_get(item);
}

/**
 * Set the cursor to be shown when mouse is over the toolbar item
 *
 * @param item Target item
 * @param cursor the cursor name to be used.
 *
 * @see elm_object_cursor_set()
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_cursor_set(Elm_Toolbar_Item *item, const char *cursor)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_set(item, cursor);
}

/**
 * Get the cursor to be shown when mouse is over the toolbar item
 *
 * @param item toolbar item with cursor already set.
 * @return the cursor name.
 *
 * @ingroup Toolbar
 */
EAPI const char *
elm_toolbar_item_cursor_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_cursor_get(item);
}

/**
 * Unset the cursor to be shown when mouse is over the toolbar item
 *
 * @param item Target item
 *
 * @see elm_object_cursor_unset()
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_cursor_unset(Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_unset(item);
}

/**
 * Sets a different style for this item cursor.
 *
 * @note before you set a style you should define a cursor with
 *       elm_toolbar_item_cursor_set()
 *
 * @param item toolbar item with cursor already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_cursor_style_set(Elm_Toolbar_Item *item, const char *style)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_style_set(item, style);
}

/**
 * Get the style for this item cursor.
 *
 * @param item toolbar item with cursor already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a cursor set, then NULL is returned.
 *
 * @ingroup Toolbar
 */
EAPI const char *
elm_toolbar_item_cursor_style_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
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
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_cursor_engine_only_set(Elm_Toolbar_Item *item, Eina_Bool engine_only)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
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
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_item_cursor_engine_only_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_widget_item_cursor_engine_only_get(item);
}

/**
 * Get whether the toolbar item opens a menu.
 *
 * @param item The toolbar item
 * @return If true, @p item opens a menu when selected
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *
elm_toolbar_item_menu_get(Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return NULL;
   /* FIXME: It's not ok. This function needs to be reviewed. And should
    * receive a const item */
   elm_toolbar_item_menu_set(item, 1);
   return item->o_menu;
}

/**
 * Returns a pointer to a toolbar item by its label
 *
 * @param obj The toolbar object
 * @param label The label of the item to find
 *
 * @return The pointer to the toolbar item matching @p label
 * Returns NULL on failure.
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar_Item *
elm_toolbar_item_find_by_label(const Evas_Object *obj, const char *label)
{
   Elm_Toolbar_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   EINA_INLIST_FOREACH(wd->items, it)
     {
        if (!strcmp(it->label, label)) return it;
     }

   return NULL;
}

/**
 * Set the data item from the toolbar item
 *
 * This set the data value passed on the elm_toolbar_item_append() and
 * related item addition calls.
 *
 * @param item The item
 * @param data The new data pointer to set
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_item_data_set(Elm_Toolbar_Item *item, const void *data)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_data_set(item, data);
}

/**
 * Get the data item from the toolbar item
 *
 * This returns the data value passed on the elm_toolbar_item_append() and
 * related item addition calls.
 *
 * @param item The item
 * @return The data pointer provided when created
 *
 * @ingroup Toolbar
 */
EAPI void *
elm_toolbar_item_data_get(const Elm_Toolbar_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_data_get(item);
}

/**
 * Set no select mode.
 *
 * This will turn off the ability to select items entirely and they will
 * neither appear selected nor emit selected signals. The clicked
 * callback function will still be called.
 *
 * @param obj The Toolbar object
 * @param no_select The no select mode (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Toolbar
 */
EAPI void
elm_toolbar_no_select_mode_set(Evas_Object *obj, Eina_Bool no_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->no_select = no_select;
}

/**
 * Gets no select mode.
 *
 * @param obj The Toolbar object
 * @return The no select mode (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool
elm_toolbar_no_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->no_select;
}

/**
 * Sets icon lookup order, for icons used in this toolbar.
 * Icons added before calling this function will not be affected.
 * The default lookup order is ELM_ICON_LOOKUP_THEME_FDO.
 *
 * @param obj The toolbar object
 * @param order The icon lookup order
 *
 * @ingroup Toolbar
 */
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

/**
 * Gets the icon lookup order.
 *
 * @param obj The Toolbar object
 * @return The icon lookup order
 *
 * @ingroup Toolbar
 */
EAPI Elm_Icon_Lookup_Order
elm_toolbar_icon_order_lookup_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_ICON_LOOKUP_THEME_FDO;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_ICON_LOOKUP_THEME_FDO;
   return wd->lookup_order;
}
