#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Menu Menu
 *
 * A menu is a list of items displayed above the window. Each item can
 * have a sub-menu. The menu object can be used to display a menu on right
 * click, in a toolbar, anywhere.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Elm_Menu_Item
{
   Elm_Widget_Item base;
   Elm_Menu_Item *parent;
   Evas_Object *icon;
   const char *icon_str;
   const char *label;
   Evas_Smart_Cb func;

   struct {
      Evas_Object *hv, *bx, *location;
      Eina_List *items;
      Eina_Bool open : 1;
   } submenu;

   Eina_Bool separator : 1;
   Eina_Bool disabled : 1;
   Eina_Bool selected: 1;
};

struct _Widget_Data
{
   Evas_Object *hv, *bx, *location, *parent, *obj;
   Eina_List *items;
   Evas_Coord xloc, yloc;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _submenu_sizing_eval(Elm_Menu_Item *parent);
static void _item_sizing_eval(Elm_Menu_Item *item);
static void _submenu_hide(Elm_Menu_Item *item);
static void _submenu_open(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _parent_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_hide(void *data, Evas_Object *obj, void *event_info);

static void
_del_item(Elm_Menu_Item *item)
{
   Elm_Menu_Item *child;

   elm_widget_item_pre_notify_del(item);

   EINA_LIST_FREE(item->submenu.items, child)
     _del_item(child);

   if (item->label) eina_stringshare_del(item->label);
   if (item->submenu.hv) evas_object_del(item->submenu.hv);
   if (item->submenu.location) evas_object_del(item->submenu.location);
   if (item->icon_str) eina_stringshare_del(item->icon_str);
   elm_widget_item_del(item);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Elm_Menu_Item *item;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);
   evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL, _parent_del, wd);

   EINA_LIST_FREE(wd->items, item)
     _del_item(item);

   if (wd->hv) evas_object_del(wd->hv);
   if (wd->location) evas_object_del(wd->location);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Eina_List *l, *_l, *_ll, *ll = NULL;
   Elm_Menu_Item *item;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   ll = eina_list_append(ll, wd->items);
   EINA_LIST_FOREACH(ll, _ll, l)
     {
	EINA_LIST_FOREACH(l, _l, item)
	  {
             edje_object_mirrored_set(item->base.view, elm_widget_mirrored_get(obj));
	     ll = eina_list_append(ll, item->submenu.items);
	     if (item->separator)
	       _elm_theme_object_set(obj, item->base.view, "menu", "separator",
                                     elm_widget_style_get(obj));
	     else if (item->submenu.bx)
	       {
		  _elm_theme_object_set
                    (obj, item->base.view, "menu", "item_with_submenu",
                     elm_widget_style_get(obj));
		  elm_menu_item_label_set(item, item->label);
		  elm_menu_item_icon_set(item, item->icon_str);
	       }
	     else
	       {
		  _elm_theme_object_set(obj, item->base.view, "menu", "item",
                                        elm_widget_style_get(obj));
		  elm_menu_item_label_set(item, item->label);
		  elm_menu_item_icon_set(item, item->icon_str);
	       }
	     if (item->disabled)
	       edje_object_signal_emit
                 (item->base.view, "elm,state,disabled", "elm");
	     else
	       edje_object_signal_emit
                 (item->base.view, "elm,state,enabled", "elm");
	     edje_object_message_signal_process(item->base.view);
	     edje_object_scale_set(item->base.view, elm_widget_scale_get(obj) *
                                   _elm_config->scale);
	  }
     }
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Menu_Item *item;
   Evas_Coord x_p, y_p, w_p, h_p, x2, y2, w2, h2, bx, by, bw, bh;
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->parent)) return;
   EINA_LIST_FOREACH(wd->items,l,item) _item_sizing_eval(item);
   evas_object_geometry_get(wd->location, &x_p, &y_p, &w_p, &h_p);
   evas_object_geometry_get(wd->parent, &x2, &y2, &w2, &h2);
   evas_object_geometry_get(wd->bx, &bx, &by, &bw, &bh);

   x_p = wd->xloc;
   y_p = wd->yloc;

   if (elm_widget_mirrored_get(obj))
     x_p -= w_p;

   if (x_p+bw > x2+w2) x_p -= x_p+bw - (x2+w2);
   if (x_p < x2) x_p += x2 - x_p;

   if (y_p+h_p+bh > y2+h2) y_p -= y_p+h_p+bh - (y2+h2);
   if (y_p < y2) y_p += y2 - y_p;


   evas_object_move(wd->location, x_p, y_p);
   evas_object_resize(wd->location, bw, h_p);
   evas_object_size_hint_min_set(wd->location, bw, h_p);
   evas_object_size_hint_max_set(wd->location, bw, h_p);
   elm_hover_target_set(wd->hv, wd->location);

   EINA_LIST_FOREACH(wd->items,l,item)
     {
	if (item->submenu.open) _submenu_sizing_eval(item);
     }
}

static void
_submenu_sizing_eval(Elm_Menu_Item *parent)
{
   Eina_List *l;
   Elm_Menu_Item *item;
   Evas_Coord x_p, y_p, w_p, h_p, x2, y2, w2, h2, bx, by, bw, bh, px, py, pw, ph;
   Widget_Data *wd = elm_widget_data_get(parent->base.widget);
   if (!wd) return;
   EINA_LIST_FOREACH(parent->submenu.items, l, item) _item_sizing_eval(item);
   evas_object_geometry_get(parent->submenu.location, &x_p, &y_p, &w_p, &h_p);
   evas_object_geometry_get(parent->base.view, &x2, &y2, &w2, &h2);
   evas_object_geometry_get(parent->submenu.bx, &bx, &by, &bw, &bh);
   evas_object_geometry_get(wd->parent, &px, &py, &pw, &ph);

   x_p = x2+w2;
   y_p = y2;

   /* If it overflows on the right, adjust the x */
   if ((x_p + bw > px + pw) || elm_widget_mirrored_get(parent->base.widget))
     x_p = x2-bw;

   /* If it overflows on the left, adjust the x - usually only happens
    * with an RTL interface */
   if (x_p < px)
     x_p = x2 + w2;

   /* If after all the adjustments it still overflows, fix it */
   if (x_p + bw > px + pw)
     x_p = x2-bw;

   if (y_p+bh > py+ph)
     y_p -= y_p+bh - (py+ph);
   if (y_p < py)
     y_p += y_p - y_p;

   evas_object_move(parent->submenu.location, x_p, y_p);
   evas_object_resize(parent->submenu.location, bw, h_p);
   evas_object_size_hint_min_set(parent->submenu.location, bw, h_p);
   evas_object_size_hint_max_set(parent->submenu.location, bw, h_p);
   elm_hover_target_set(parent->submenu.hv, parent->submenu.location);

   EINA_LIST_FOREACH(parent->submenu.items, l, item)
     {
	if (item->submenu.open)
	  _submenu_sizing_eval(item);
     }
}

static void
_item_sizing_eval(Elm_Menu_Item *item)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!item->separator)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(item->base.view, &minw, &minh, minw, minh);
   if (!item->separator)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(item->base.view, minw, minh);
   evas_object_size_hint_max_set(item->base.view, maxw, maxh);
}

static void
_menu_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_parent_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_parent_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = data;
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, _parent_resize, wd->obj);
   wd->parent = NULL;
}

static void
_item_move_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Menu_Item *item = data;
   if (item->submenu.open) _submenu_sizing_eval(item);
}

static void
_hover_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   _menu_hide(data, obj, event_info);
   evas_object_smart_callback_call(data, "clicked", NULL);
}

static void
_menu_hide(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_List *l;
   Elm_Menu_Item *item2;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_hide(wd->hv);
   evas_object_hide(data);

   EINA_LIST_FOREACH(wd->items, l, item2)
     {
	if (item2->submenu.open) _submenu_hide(item2);
     }
}

static void
_submenu_hide(Elm_Menu_Item *item)
{
   Eina_List *l;
   Elm_Menu_Item *item2;
   evas_object_hide(item->submenu.hv);
   item->submenu.open = EINA_FALSE;
   EINA_LIST_FOREACH(item->submenu.items, l, item2)
     {
	if (item2->submenu.open) _submenu_hide(item2);
     }
}

static void
_menu_item_select(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Menu_Item *item = data;
   if (item->submenu.items)
     {
	if (!item->submenu.open) _submenu_open(item, NULL, NULL, NULL);
	else _submenu_hide(item);
     }
   else
     _menu_hide(item->base.widget, NULL, NULL);

   if (item->func) item->func((void *)(item->base.data), item->base.widget, item);
}

static void
_menu_item_activate(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Eina_List *l;
   Elm_Menu_Item *item2;
   Elm_Menu_Item *item = data;
   item->selected = 1;
   if (item->parent)
     {
	EINA_LIST_FOREACH(item->parent->submenu.items, l, item2)
	  {
            if (item2 != item) elm_menu_item_selected_set(item2, 0);
	  }
     }
   else
     {
	Widget_Data *wd = elm_widget_data_get(item->base.widget);
	EINA_LIST_FOREACH(wd->items, l, item2)
	  {
            if (item2 != item) elm_menu_item_selected_set(item2, 0);
	  }
     }
}

static void
_menu_item_inactivate(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Menu_Item *item = data;
   item->selected = 0;
   if (item->submenu.open) _submenu_hide(item);
}

static void
_submenu_open(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Menu_Item *item = data;
   item->submenu.open = EINA_TRUE;
   evas_object_show(item->submenu.hv);
   _sizing_eval(item->base.widget);
}

static void
_show(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_show(wd->hv);
}

static void
_item_obj_create(Elm_Menu_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   item->base.view = edje_object_add(evas_object_evas_get(wd->bx));
   edje_object_mirrored_set(item->base.view, elm_widget_mirrored_get(item->base.widget));
   evas_object_size_hint_weight_set(item->base.view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(item->base.view, EVAS_HINT_FILL, EVAS_HINT_FILL);
   _elm_theme_object_set(item->base.widget, item->base.view, "menu", "item",  elm_widget_style_get(item->base.widget));
   edje_object_signal_callback_add(item->base.view, "elm,action,click", "",
                                   _menu_item_select, item);
   edje_object_signal_callback_add(item->base.view, "elm,action,activate", "",
                                   _menu_item_activate, item);
   edje_object_signal_callback_add(item->base.view, "elm,action,inactivate", "",
                                   _menu_item_inactivate, item);
   evas_object_show(item->base.view);
}

static void
_item_separator_obj_create(Elm_Menu_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   item->base.view = edje_object_add(evas_object_evas_get(wd->bx));
   evas_object_size_hint_weight_set(item->base.view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(item->base.view, EVAS_HINT_FILL, EVAS_HINT_FILL);
   _elm_theme_object_set(item->base.widget, item->base.view, "menu", "separator",  elm_widget_style_get(item->base.widget));
   edje_object_signal_callback_add(item->base.view, "elm,action,activate", "",
                                   _menu_item_activate, item);
   evas_object_show(item->base.view);
}

static void
_item_submenu_obj_create(Elm_Menu_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   item->submenu.location = elm_icon_add(wd->bx);
   item->submenu.hv = elm_hover_add(wd->bx);
   elm_widget_mirrored_set(item->submenu.hv, EINA_FALSE);
   elm_hover_target_set(item->submenu.hv, item->submenu.location);
   elm_hover_parent_set(item->submenu.hv, wd->parent);
   elm_object_style_set(item->submenu.hv, "submenu");

   item->submenu.bx = elm_box_add(wd->bx);
   elm_widget_mirrored_set(item->submenu.bx, EINA_FALSE);
   evas_object_size_hint_weight_set(item->submenu.bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(item->submenu.bx);
   elm_hover_content_set(item->submenu.hv, elm_hover_best_content_location_get(item->submenu.hv, ELM_HOVER_AXIS_VERTICAL), item->submenu.bx);

   edje_object_mirrored_set(item->base.view, elm_widget_mirrored_get(item->base.widget));
   _elm_theme_object_set(item->base.widget, item->base.view, "menu", "item_with_submenu",  elm_widget_style_get(item->base.widget));
   elm_menu_item_label_set(item, item->label);
   elm_menu_item_icon_set(item, item->icon_str);

   edje_object_signal_callback_add(item->base.view, "elm,action,open", "",
                                   _submenu_open, item);
   evas_object_event_callback_add(item->base.view, EVAS_CALLBACK_MOVE, _item_move_resize, item);
   evas_object_event_callback_add(item->base.view, EVAS_CALLBACK_RESIZE, _item_move_resize, item);

   evas_object_event_callback_add(item->submenu.bx, EVAS_CALLBACK_RESIZE, _menu_resize, item->base.widget);
}

/**
 * Add a new menu to the parent
 *
 * @param parent The parent object.
 * @return The new object or NULL if it cannot be created.
 *
 * @ingroup Menu
 */
EAPI Evas_Object *
elm_menu_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
   
   ELM_SET_WIDTYPE(widtype, "menu");
   elm_widget_type_set(obj, "menu");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   wd->location = elm_icon_add(obj);
   wd->parent = parent;
   wd->obj = obj;

   wd->hv = elm_hover_add(obj);
   elm_widget_mirrored_set(wd->hv, EINA_FALSE);
   elm_hover_parent_set(wd->hv, parent);
   elm_hover_target_set(wd->hv, wd->location);
   elm_object_style_set(wd->hv, "menu");
   evas_object_smart_callback_add(wd->hv, "clicked", _hover_clicked_cb, obj);

   wd->bx = elm_box_add(obj);
   elm_widget_mirrored_set(wd->bx, EINA_FALSE);
   evas_object_size_hint_weight_set(wd->bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(wd->bx);
   elm_hover_content_set(wd->hv, elm_hover_best_content_location_get(wd->hv, ELM_HOVER_AXIS_VERTICAL), wd->bx);

   evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_RESIZE, _parent_resize, wd->obj);
   evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_DEL, _parent_del, wd);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, obj);

   evas_object_event_callback_add(wd->bx, EVAS_CALLBACK_RESIZE, _menu_resize, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the parent
 *
 * @param obj The menu object.
 * @param parent The new parent.
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_parent_set(Evas_Object *obj, Evas_Object *parent)
{
   Eina_List *l, *_l, *_ll, *ll = NULL;
   Elm_Menu_Item *item;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->parent == parent) return;
   if (wd->parent)
     {
       evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE, _parent_resize, wd->obj);
       evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL, _parent_del, wd);
     }
   wd->parent = parent;
   if (wd->parent)
     {
       evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_RESIZE, _parent_resize, wd->obj);
       evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_DEL, _parent_del, wd);
     }
   elm_hover_parent_set(wd->hv, parent);

   ll = eina_list_append(ll, wd->items);
   EINA_LIST_FOREACH(ll, _ll, l)
     {
	EINA_LIST_FOREACH(l, _l, item)
	  {
	     if (item->submenu.hv)
	       {
		  elm_hover_parent_set(item->submenu.hv, parent);
		  ll = eina_list_append(ll, item->submenu.items);
	       }
	  }
     }
   _sizing_eval(obj);
}

/**
 * Get the parent
 *
 * @param obj The menu object.
 * @return The parent.
 *
 * @ingroup Menu
 */
EAPI Evas_Object *
elm_menu_parent_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->parent;
}

/**
 * Move the menu to a new position
 *
 * @param obj The menu object.
 * @param x The new position.
 * @param y The new position.
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->xloc = x;
   wd->yloc = y;
   _sizing_eval(obj);
}

/**
 * Close a opened menu
 *
 * @param obj the menu object
 * @return void
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_close(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   _menu_hide(obj, wd->hv, NULL);
}

/**
 * Get the Evas_Object of an Elm_Menu_Item
 *
 * @param item The menu item object.
 *
 * @ingroup Menu
 */
EAPI Evas_Object *
elm_menu_item_object_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->base.view;
}

static void
_item_clone(Evas_Object *obj, Elm_Menu_Item *parent, Elm_Menu_Item *item)
{
   Elm_Menu_Item *new_item, *subitem;
   Eina_List *iter;

   if (item->separator)
      new_item = elm_menu_item_separator_add(obj, parent);
   else
      new_item = elm_menu_item_add(obj, parent, item->icon_str, item->label, item->func, item->base.data);
   elm_menu_item_disabled_set(new_item, item->disabled);

   EINA_LIST_FOREACH(item->submenu.items, iter, subitem)
      _item_clone(obj, new_item, subitem);
}

void
elm_menu_clone(Evas_Object *from_menu, Evas_Object *to_menu, Elm_Menu_Item *parent)
{
   ELM_CHECK_WIDTYPE(from_menu, widtype);
   ELM_CHECK_WIDTYPE(to_menu, widtype);
   Widget_Data *from_wd = elm_widget_data_get(from_menu);
   Eina_List *iter;
   Elm_Menu_Item *item;

   if (!from_wd) return;
   EINA_LIST_FOREACH(from_wd->items, iter, item)
      _item_clone(to_menu, parent, item);
}

/**
 * Add an item at the end
 *
 * @param obj The menu object.
 * @param icon A icon display on the item. The icon will be destryed by the menu.
 * @param label The label of the item.
 * @param func Function called when the user select the item.
 * @param data Data sent by the callback.
 * @return Returns the new item.
 *
 * @ingroup Menu
 */
EAPI Elm_Menu_Item *
elm_menu_item_add(Evas_Object *obj, Elm_Menu_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   Elm_Menu_Item *subitem;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *icon_obj;

   if (!wd) return NULL;
   icon_obj = elm_icon_add(obj);
   if (!icon_obj) return NULL;
   subitem = elm_widget_item_new(obj, Elm_Menu_Item);
   if (!subitem)
     {
        evas_object_del(icon_obj);
        return NULL;
     }
   subitem->base.data = data;
   subitem->func = func;
   subitem->parent = parent;
   subitem->icon = icon_obj;

   _item_obj_create(subitem);
   elm_menu_item_label_set(subitem, label);

   elm_widget_sub_object_add(subitem->base.widget, subitem->icon);
   edje_object_part_swallow(subitem->base.view, "elm.swallow.content", subitem->icon);
   if (icon) elm_menu_item_icon_set(subitem, icon);

   if (parent)
     {
	if (!parent->submenu.bx) _item_submenu_obj_create(parent);
	elm_box_pack_end(parent->submenu.bx, subitem->base.view);
	parent->submenu.items = eina_list_append(parent->submenu.items, subitem);
     }
   else
     {
	elm_box_pack_end(wd->bx, subitem->base.view);
	wd->items = eina_list_append(wd->items, subitem);
     }

   _sizing_eval(obj);
   return subitem;
}

/**
 * Set the label of a menu item
 *
 * @param item The menu item object.
 * @param label The label to set for @p item
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_label_set(Elm_Menu_Item *item, const char *label)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   eina_stringshare_replace(&item->label, label);

   if (label)
     edje_object_signal_emit(item->base.view, "elm,state,text,visible", "elm");
   else
     edje_object_signal_emit(item->base.view, "elm,state,text,hidden", "elm");

   edje_object_message_signal_process(item->base.view);
   edje_object_part_text_set(item->base.view, "elm.text", label);
   _sizing_eval(item->base.widget);
}

/**
 * Get the label of a menu item
 *
 * @param item The menu item object.
 * @return The label of @p item
 *
 * @ingroup Menu
 */
EAPI const char *
elm_menu_item_label_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->label;
}

/**
 * Set the icon of a menu item
 *
 * Once the icon object is set, a previously set one will be deleted.
 *
 * @param item The menu item object.
 * @param icon The icon object to set for @p item
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_icon_set(Elm_Menu_Item *item, const char *icon)
{
   char icon_tmp[512];
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(icon);
   if (!*icon) return;
   if ((item->icon_str) && (!strcmp(item->icon_str, icon))) return;
   if ((snprintf(icon_tmp, sizeof(icon_tmp), "menu/%s", icon) > 0) &&
       (elm_icon_standard_set(item->icon, icon_tmp)))
     {
        eina_stringshare_replace(&item->icon_str, icon);
	edje_object_signal_emit(item->base.view, "elm,state,icon,visible", "elm");
     }
   else
     edje_object_signal_emit(item->base.view, "elm,state,icon,hidden", "elm");
   edje_object_message_signal_process(item->base.view);
   _sizing_eval(item->base.widget);
}

/**
 * Set the disabled state of @p item.
 *
 * @param item The menu item object.
 * @param disabled The enabled/disabled state of the item
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_disabled_set(Elm_Menu_Item *item, Eina_Bool disabled)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   if (disabled == item->disabled) return;
   item->disabled = disabled;
   if (disabled)
     {
        edje_object_signal_emit(item->base.view, "elm,state,disabled", "elm");
        if (item->submenu.open) _submenu_hide(item);
     }
   else
     edje_object_signal_emit(item->base.view, "elm,state,enabled", "elm");
   edje_object_message_signal_process(item->base.view);
}

/**
 * Get the disabled state of @p item.
 *
 * @param item The menu item object.
 * @return The enabled/disabled state of the item
 *
 * @ingroup Menu
 */
EAPI Eina_Bool
elm_menu_item_disabled_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->disabled;
}

/**
 * Add a separator item to menu @p obj under @p parent.
 *
 * @param obj The menu object
 * @param parent The item to add the separator under
 *
 * @return The created item or NULL on failure
 *
 * @ingroup Menu
 */
EAPI Elm_Menu_Item *
elm_menu_item_separator_add(Evas_Object *obj, Elm_Menu_Item *parent)
{
   Elm_Menu_Item *subitem;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   /* don't add a separator as the first item */
   if (!wd->items) return NULL;
   /* don't allow adding more than one separator in a row */
   if (parent) subitem = eina_list_last(parent->submenu.items)->data;
   else subitem = eina_list_last(wd->items)->data;
   if (subitem->separator) return NULL;

   subitem = elm_widget_item_new(obj, Elm_Menu_Item);
   if (!subitem) return NULL;
   subitem->base.widget = obj;
   subitem->separator = 1;
   _item_separator_obj_create(subitem);
   if (!parent)
     {
	elm_box_pack_end(wd->bx, subitem->base.view);
	wd->items = eina_list_append(wd->items, subitem);
     }
   else
     {
	if (!parent->submenu.bx) _item_submenu_obj_create(parent);
	elm_box_pack_end(parent->submenu.bx, subitem->base.view);
	parent->submenu.items = eina_list_append(parent->submenu.items, subitem);
     }
   _sizing_eval(obj);
   return subitem;
}

/**
 * Get the icon object from a menu item
 *
 * @param item The menu item object
 * @return The icon object or NULL if there's no icon
 *
 * @ingroup Menu
 */
EAPI const Evas_Object *
elm_menu_item_object_icon_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return (const Evas_Object *)item->icon;
}

/**
 * Get the string representation from the icon of a menu item
 *
 * @param item The menu item object.
 * @return The string representation of @p item's icon or NULL
 *
 * @ingroup Menu
 */
EAPI const char *
elm_menu_item_icon_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->icon_str;
}

/**
 * Returns whether @p item is a separator.
 *
 * @param item The item to check
 * @return If true, @p item is a separator
 *
 * @ingroup Menu
 */
EAPI Eina_Bool
elm_menu_item_is_separator(Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->separator;
}

/**
 * Deletes an item from the menu.
 *
 * @param item The item to delete.
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_del(Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   Elm_Menu_Item *_item;

   elm_widget_item_pre_notify_del(item);

   EINA_LIST_FREE(item->submenu.items, _item) elm_menu_item_del(_item);
   if (item->label) eina_stringshare_del(item->label);
   if (item->icon) evas_object_del(item->icon);
   if (item->submenu.hv) evas_object_del(item->submenu.hv);
   if (item->submenu.location) evas_object_del(item->submenu.location);

   if (item->parent)
      item->parent->submenu.items = eina_list_remove(item->parent->submenu.items, item);
   else
     {
	Widget_Data *wd = elm_widget_data_get(item->base.widget);
	wd->items = eina_list_remove(wd->items, item);
     }

   elm_widget_item_del(item);
}

/**
 * Set the function called when a menu item is freed.
 *
 * @param item The item to set the callback on
 * @param func The function called
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_del_cb_set(Elm_Menu_Item *item, Evas_Smart_Cb func)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_del_cb_set(item, func);
}

/**
 * Returns the data associated with menu item @p item.
 *
 * @param item The item
 * @return The data associated with @p item
 *
 * @ingroup Menu
 */
EAPI void *
elm_menu_item_data_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_data_get(item);
}

/**
 * Sets the data to be associated with menu item @p item.
 *
 * @param item The item
 * @param data The data to be associated with @p item
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_data_set(Elm_Menu_Item *item, const void *data)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   elm_widget_item_data_set(item, data);
}

/**
 * Returns a list of @p item's subitems.
 *
 * @param item The item
 * @return An Eina_List* of @p item's subitems
 *
 * @ingroup Menu
 */
EAPI const Eina_List *
elm_menu_item_subitems_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->submenu.items;
}

/**
 * Returns a list of @p item's items.
 *
 * @param obj The menu object
 * @return An Eina_List* of @p item's items
 *
 * @ingroup Menu
 */
EAPI const Eina_List *
elm_menu_items_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   return wd->items;
}

/**
 * Set the selected state of @p item.
 *
 * @param item The menu item object.
 * @param selected The selected/unselected state of the item
 *
 * @ingroup Menu
 */
EAPI void
elm_menu_item_selected_set(Elm_Menu_Item *item, Eina_Bool selected)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   if (selected == item->selected) return;
   item->selected = selected;
   if (selected)
     {
        edje_object_signal_emit(item->base.view, "elm,state,selected", "elm");
        _menu_item_activate(item, NULL, NULL, NULL);
     }
   else
     {
        edje_object_signal_emit(item->base.view, "elm,state,unselected", "elm");
        _menu_item_inactivate(item, NULL, NULL, NULL);
     }
   edje_object_message_signal_process(item->base.view);
}

/**
 * Get the selected state of @p item.
 *
 * @param item The menu item object.
 * @return The selected/unselected state of the item
 *
 * @ingroup Menu
 */
EAPI Eina_Bool
elm_menu_item_selected_get(const Elm_Menu_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->selected;
}

/**
 * Get the previous item in the menu.
 *
 * @param item The menu item object.
 * @return The item before it, or NULL if none
 *
 * @ingroup Menu
 */
EAPI const Elm_Menu_Item *
elm_menu_item_prev_get(const Elm_Menu_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, NULL);
   if (it->parent)
     {
        Eina_List *l = eina_list_data_find_list(it->parent->submenu.items, it);
        l = eina_list_prev(l);
        if (!l) return NULL;
        return l->data;
     }
   else
     {
        Widget_Data *wd = elm_widget_data_get(it->base.widget);
        if (!wd | !wd->items) return NULL;
        Eina_List *l = eina_list_data_find_list(wd->items, it);
        l = eina_list_prev(l);
        if (!l) return NULL;
        return l->data;
     }
   return NULL;
}

/**
 * Get the next item in the menu.
 *
 * @param item The menu item object.
 * @return The item after it, or NULL if none
 *
 * @ingroup Menu
 */
EAPI const Elm_Menu_Item *
elm_menu_item_next_get(const Elm_Menu_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, NULL);
   if (it->parent)
     {
        Eina_List *l = eina_list_data_find_list(it->parent->submenu.items, it);
        l = eina_list_next(l);
        if (!l) return NULL;
        return l->data;
     }
   else
     {
        Widget_Data *wd = elm_widget_data_get(it->base.widget);
        if (!wd | !wd->items) return NULL;
        Eina_List *l = eina_list_data_find_list(wd->items, it);
        l = eina_list_next(l);
        if (!l) return NULL;
        return l->data;
     }
   return NULL;
}

/**
 * Get the first item in the menu
 *
 * @param obj The menu object
 * @return The first item, or NULL if none
 *
 * @ingroup Menu
 */
EAPI const Elm_Menu_Item *
elm_menu_first_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->items) return wd->items->data;
   return NULL;
}

/**
 * Get the last item in the menu
 *
 * @param obj The menu object
 * @return The last item, or NULL if none
 *
 * @ingroup Menu
 */
EAPI const Elm_Menu_Item *
elm_menu_last_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Eina_List *l = eina_list_last(wd->items);
   if (l) return l->data;
   return NULL;
}

/**
 * Get the selected item in the menu
 *
 * @param obj The menu object
 * @return The selected item, or NULL if none
 *
 * @ingroup Menu
 */
EAPI const Elm_Menu_Item *
elm_menu_selected_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Eina_List *l;
   Elm_Menu_Item *item;
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->selected) return item;
     }
   return NULL;
}

