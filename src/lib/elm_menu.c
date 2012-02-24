#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Menu_Item Elm_Menu_Item;

struct _Elm_Menu_Item
{
   ELM_WIDGET_ITEM;
   Elm_Menu_Item *parent;
   Evas_Object *content;
   const char *icon_str;
   const char *label;
   Evas_Smart_Cb func;
   unsigned int idx;

   struct {
      Evas_Object *hv, *bx, *location;
      Eina_List *items;
      Eina_Bool open : 1;
   } submenu;

   Eina_Bool separator : 1;
   Eina_Bool selected : 1;
   Eina_Bool object_item : 1;
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
static void _item_disable_hook(Elm_Object_Item *it);
static void _sizing_eval(Evas_Object *obj);
static void _submenu_sizing_eval(Elm_Menu_Item *parent);
static void _item_sizing_eval(Elm_Menu_Item *item);
static void _submenu_hide(Elm_Menu_Item *item);
static void _submenu_open(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _parent_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_hide(void *data, Evas_Object *obj, void *event_info);

static const char SIG_CLICKED[] = "clicked";

static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_CLICKED, ""},
   {NULL, NULL}
};


static void
_del_item(Elm_Menu_Item *item)
{
   Elm_Menu_Item *child;

   EINA_LIST_FREE(item->submenu.items, child)
     _del_item(child);

   if (item->label) eina_stringshare_del(item->label);
   if (item->submenu.hv) evas_object_del(item->submenu.hv);
   if (item->submenu.location) evas_object_del(item->submenu.location);
   if (item->icon_str) eina_stringshare_del(item->icon_str);
   elm_widget_item_free(item);
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
             edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(obj));
             ll = eina_list_append(ll, item->submenu.items);
             if (item->separator)
               _elm_theme_object_set(obj, VIEW(item), "menu", "separator",
                                     elm_widget_style_get(obj));
             else if (item->submenu.bx)
               {
                  _elm_theme_object_set
                     (obj, VIEW(item), "menu", "item_with_submenu",
                      elm_widget_style_get(obj));
                  elm_object_item_text_set((Elm_Object_Item *) item,
                                           item->label);
                  elm_menu_item_object_icon_name_set((Elm_Object_Item *) item,
                                                     item->icon_str);
               }
             else
               {
                  _elm_theme_object_set(obj, VIEW(item), "menu", "item",
                                        elm_widget_style_get(obj));
                  elm_object_item_text_set((Elm_Object_Item *) item,
                                           item->label);
                  elm_menu_item_object_icon_name_set((Elm_Object_Item *) item,
                                                     item->icon_str);
               }
             _item_disable_hook((Elm_Object_Item *) item);
             edje_object_scale_set(VIEW(item), elm_widget_scale_get(obj) *
                                   _elm_config->scale);
          }
     }
   _sizing_eval(obj);
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *label)
{
   Elm_Menu_Item *item;

   if (part && strcmp(part, "default")) return;

   item = (Elm_Menu_Item *) it;

   eina_stringshare_replace(&item->label, label);

   if (label)
     edje_object_signal_emit(VIEW(item), "elm,state,text,visible", "elm");
   else
     edje_object_signal_emit(VIEW(item), "elm,state,text,hidden", "elm");

   edje_object_message_signal_process(VIEW(item));
   edje_object_part_text_set(VIEW(item), "elm.text", label);
   _sizing_eval(WIDGET(item));
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;
   return ((Elm_Menu_Item *) it)->label;
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_Menu_Item *item;

   if (part && strcmp(part, "default")) return;

   item = (Elm_Menu_Item *) it;

   if (item->content)
     {
        elm_widget_sub_object_del(WIDGET(item), item->content);
        evas_object_del(item->content);
     }

   item->content = content;

   elm_widget_sub_object_add(WIDGET(item), item->content);
   edje_object_part_swallow(VIEW(item), "elm.swallow.content", item->content);
   _sizing_eval(WIDGET(item));
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;
   return ((Elm_Menu_Item *) it)->content;
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;

   if (elm_widget_item_disabled_get(item))
     {
        edje_object_signal_emit(VIEW(item), "elm,state,disabled", "elm");
        if (item->submenu.open) _submenu_hide(item);
     }
   else
     edje_object_signal_emit(VIEW(item), "elm,state,enabled", "elm");

   edje_object_message_signal_process(VIEW(item));
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
   Widget_Data *wd = elm_widget_data_get(WIDGET(parent));
   if (!wd) return;
   EINA_LIST_FOREACH(parent->submenu.items, l, item) _item_sizing_eval(item);
   evas_object_geometry_get(parent->submenu.location, &x_p, &y_p, &w_p, &h_p);
   evas_object_geometry_get(VIEW(parent), &x2, &y2, &w2, &h2);
   evas_object_geometry_get(parent->submenu.bx, &bx, &by, &bw, &bh);
   evas_object_geometry_get(wd->parent, &px, &py, &pw, &ph);

   x_p = x2+w2;
   y_p = y2;

   /* If it overflows on the right, adjust the x */
   if ((x_p + bw > px + pw) || elm_widget_mirrored_get(WIDGET(parent)))
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
   edje_object_size_min_restricted_calc(VIEW(item), &minw, &minh, minw, minh);
   if (!item->separator)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(VIEW(item), minw, minh);
   evas_object_size_hint_max_set(VIEW(item), maxw, maxh);
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
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
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
     _menu_hide(WIDGET(item), NULL, NULL);

   if (item->func) item->func((void *)(item->base.data), WIDGET(item), item);
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
             if (item2 != item)
               elm_menu_item_selected_set((Elm_Object_Item *) item2, 0);
          }
     }
   else
     {
        Widget_Data *wd = elm_widget_data_get(WIDGET(item));
        EINA_LIST_FOREACH(wd->items, l, item2)
          {
             if (item2 != item)
               elm_menu_item_selected_set((Elm_Object_Item *) item2, 0);
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
   _sizing_eval(WIDGET(item));
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
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;
   VIEW(item) = edje_object_add(evas_object_evas_get(wd->bx));
   edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(WIDGET(item)));
   evas_object_size_hint_weight_set(VIEW(item), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
   _elm_theme_object_set(WIDGET(item), VIEW(item), "menu", "item",  elm_widget_style_get(WIDGET(item)));
   edje_object_signal_callback_add(VIEW(item), "elm,action,click", "",
                                   _menu_item_select, item);
   edje_object_signal_callback_add(VIEW(item), "elm,action,activate", "",
                                   _menu_item_activate, item);
   edje_object_signal_callback_add(VIEW(item), "elm,action,inactivate", "",
                                   _menu_item_inactivate, item);
   evas_object_show(VIEW(item));
}

static void
_item_separator_obj_create(Elm_Menu_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;
   VIEW(item) = edje_object_add(evas_object_evas_get(wd->bx));
   evas_object_size_hint_weight_set(VIEW(item), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
   _elm_theme_object_set(WIDGET(item), VIEW(item), "menu", "separator",  elm_widget_style_get(WIDGET(item)));
   edje_object_signal_callback_add(VIEW(item), "elm,action,activate", "",
                                   _menu_item_activate, item);
   evas_object_show(VIEW(item));
}

static void
_item_submenu_obj_create(Elm_Menu_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(WIDGET(item));
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
   elm_object_part_content_set(item->submenu.hv, elm_hover_best_content_location_get(item->submenu.hv, ELM_HOVER_AXIS_VERTICAL), item->submenu.bx);

   edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(WIDGET(item)));
   _elm_theme_object_set(WIDGET(item), VIEW(item), "menu", "item_with_submenu",  elm_widget_style_get(WIDGET(item)));
   elm_object_item_text_set((Elm_Object_Item *) item,
                            item->label);
   if (item->icon_str)
     elm_menu_item_object_icon_name_set((Elm_Object_Item *) item,
                                        item->icon_str);
   edje_object_signal_callback_add(VIEW(item), "elm,action,open", "",
                                   _submenu_open, item);
   evas_object_event_callback_add(VIEW(item), EVAS_CALLBACK_MOVE, _item_move_resize, item);
   evas_object_event_callback_add(VIEW(item), EVAS_CALLBACK_RESIZE, _item_move_resize, item);

   evas_object_event_callback_add(item->submenu.bx, EVAS_CALLBACK_RESIZE, _menu_resize, WIDGET(item));
}

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
   elm_object_part_content_set(wd->hv, elm_hover_best_content_location_get(wd->hv, ELM_HOVER_AXIS_VERTICAL), wd->bx);

   evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_RESIZE, _parent_resize, wd->obj);
   evas_object_event_callback_add(wd->parent, EVAS_CALLBACK_DEL, _parent_del, wd);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, obj);

   evas_object_event_callback_add(wd->bx, EVAS_CALLBACK_RESIZE, _menu_resize, obj);

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   _sizing_eval(obj);
   return obj;
}

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

EAPI Evas_Object *
elm_menu_parent_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->parent;
}

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

EAPI void
elm_menu_close(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   _menu_hide(obj, wd->hv, NULL);
}

EAPI Evas_Object *
elm_menu_item_object_get(const Elm_Object_Item *it)
{
   return VIEW(((Elm_Menu_Item *) it));
}

static void
_item_clone(Evas_Object *obj, Elm_Menu_Item *parent, Elm_Menu_Item *item)
{
   Elm_Object_Item *new_item;
   Elm_Menu_Item *subitem;
   Eina_List *iter;

   if (item->separator)
     new_item = elm_menu_item_separator_add(obj, (Elm_Object_Item *) parent);
   else
     new_item = elm_menu_item_add(obj,
                                  (Elm_Object_Item *) parent,
                                  item->icon_str,
                                  item->label,
                                  item->func,
                                  item->base.data);
   elm_object_item_disabled_set(new_item,
                                elm_widget_item_disabled_get(item));

   EINA_LIST_FOREACH(item->submenu.items, iter, subitem)
     _item_clone(obj, (Elm_Menu_Item *) new_item, subitem);
}

void
elm_menu_clone(Evas_Object *from_menu, Evas_Object *to_menu, Elm_Object_Item *parent)
{
   ELM_CHECK_WIDTYPE(from_menu, widtype);
   ELM_CHECK_WIDTYPE(to_menu, widtype);
   Widget_Data *from_wd = elm_widget_data_get(from_menu);
   Eina_List *iter;
   Elm_Menu_Item *item;

   if (!from_wd) return;
   EINA_LIST_FOREACH(from_wd->items, iter, item)
     _item_clone(to_menu, (Elm_Menu_Item *) parent, item);
}

static void
_elm_menu_item_add_helper(Evas_Object *obj, Elm_Menu_Item *parent, Elm_Menu_Item *subitem, Widget_Data *wd)
{
   if (parent)
     {
        if (!parent->submenu.bx) _item_submenu_obj_create(parent);
        elm_box_pack_end(parent->submenu.bx, VIEW(subitem));
        parent->submenu.items = eina_list_append(parent->submenu.items, subitem);
        subitem->idx = eina_list_count(parent->submenu.items) - 1;
     }
   else
     {
        elm_box_pack_end(wd->bx, VIEW(subitem));
        wd->items = eina_list_append(wd->items, subitem);
        subitem->idx = eina_list_count(wd->items) - 1;
     }

   _sizing_eval(obj);
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;
   Elm_Object_Item *_item;

   EINA_LIST_FREE(item->submenu.items, _item) elm_object_item_del(_item);
   if (item->label) eina_stringshare_del(item->label);
   if (item->content) evas_object_del(item->content);
   if (item->submenu.hv) evas_object_del(item->submenu.hv);
   if (item->submenu.location) evas_object_del(item->submenu.location);

   if (item->parent)
     item->parent->submenu.items = eina_list_remove(item->parent->submenu.items, item);
   else
     {
        Widget_Data *wd = elm_widget_data_get(WIDGET(item));
        wd->items = eina_list_remove(wd->items, item);
     }

   return EINA_TRUE;
}

EAPI Elm_Object_Item *
elm_menu_item_add(Evas_Object *obj, Elm_Object_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
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

   elm_widget_item_del_pre_hook_set(subitem, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(subitem, _item_disable_hook);
   elm_widget_item_text_set_hook_set(subitem, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(subitem, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(subitem, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(subitem, _item_content_get_hook);

   subitem->base.data = data;
   subitem->func = func;
   subitem->parent = (Elm_Menu_Item *) parent;
   subitem->content = icon_obj;

   _item_obj_create(subitem);
   elm_object_item_text_set((Elm_Object_Item *) subitem, label);

   elm_widget_sub_object_add(WIDGET(subitem), subitem->content);
   edje_object_part_swallow(VIEW(subitem), "elm.swallow.content", subitem->content);
   if (icon)
     elm_menu_item_object_icon_name_set((Elm_Object_Item *) subitem, icon);

   _elm_menu_item_add_helper(obj, (Elm_Menu_Item *) parent, subitem, wd);

   return (Elm_Object_Item *) subitem;
}

EAPI Elm_Object_Item *
elm_menu_item_add_object(Evas_Object *obj, Elm_Object_Item *parent, Evas_Object *subobj, Evas_Smart_Cb func, const void *data)
{
   Elm_Menu_Item *subitem;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   subitem = elm_widget_item_new(obj, Elm_Menu_Item);
   if (!subitem) return NULL;

   elm_widget_item_del_pre_hook_set(subitem, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(subitem, _item_disable_hook);
   elm_widget_item_text_set_hook_set(subitem, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(subitem, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(subitem, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(subitem, _item_content_get_hook);

   subitem->base.data = data;
   subitem->func = func;
   subitem->parent = (Elm_Menu_Item *) parent;
   subitem->object_item = EINA_TRUE;
   subitem->content = subobj;

   _item_obj_create(subitem);

   elm_widget_sub_object_add(WIDGET(subitem), subitem->content);
   edje_object_part_swallow(VIEW(subitem), "elm.swallow.content", subobj);
   _sizing_eval(WIDGET(subitem));

   _elm_menu_item_add_helper(obj, (Elm_Menu_Item *) parent, subitem, wd);

   return (Elm_Object_Item *) subitem;
}

EAPI unsigned int
elm_menu_item_index_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, 0);
   return ((Elm_Menu_Item *) it)->idx;
}

EAPI void
elm_menu_item_label_set(Elm_Object_Item *it, const char *label)
{
   _item_text_set_hook(it, NULL, label);
}

EAPI const char *
elm_menu_item_label_get(const Elm_Object_Item *it)
{
   return _item_text_get_hook(it, NULL);
}

EAPI void
elm_menu_item_object_icon_name_set(Elm_Object_Item *it, const char *icon)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   EINA_SAFETY_ON_NULL_RETURN(icon);
   char icon_tmp[512];
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;

   if (!*icon) return;
   if ((item->icon_str) && (!strcmp(item->icon_str, icon))) return;
   if ((snprintf(icon_tmp, sizeof(icon_tmp), "menu/%s", icon) > 0) &&
       (elm_icon_standard_set(item->content, icon_tmp) ||
        elm_icon_standard_set(item->content, icon)))
     {
        eina_stringshare_replace(&item->icon_str, icon);
        edje_object_signal_emit(VIEW(item), "elm,state,icon,visible", "elm");
     }
   else
     edje_object_signal_emit(VIEW(item), "elm,state,icon,hidden", "elm");
   edje_object_message_signal_process(VIEW(item));
   _sizing_eval(WIDGET(item));
}

EAPI void
elm_menu_item_disabled_set(Elm_Object_Item *it, Eina_Bool disabled)
{
   elm_object_item_disabled_set(it, disabled);
}

EAPI Eina_Bool
elm_menu_item_disabled_get(const Elm_Object_Item *it)
{
   return elm_object_item_disabled_get(it);
}

EAPI Elm_Object_Item *
elm_menu_item_separator_add(Evas_Object *obj, Elm_Object_Item *parent)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Menu_Item *subitem;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Menu_Item *p_item = (Elm_Menu_Item *) parent;
   if (!wd) return NULL;
   /* don't add a separator as the first item */
   if (!wd->items) return NULL;
   /* don't allow adding more than one separator in a row */
   if (p_item) subitem = eina_list_last(p_item->submenu.items)->data;
   else subitem = eina_list_last(wd->items)->data;
   if (subitem->separator) return NULL;

   subitem = elm_widget_item_new(obj, Elm_Menu_Item);
   if (!subitem) return NULL;

   elm_widget_item_del_pre_hook_set(subitem, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(subitem, _item_disable_hook);
   elm_widget_item_text_set_hook_set(subitem, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(subitem, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(subitem, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(subitem, _item_content_get_hook);

   subitem->separator = EINA_TRUE;
   _item_separator_obj_create(subitem);
   if (!p_item)
     {
        elm_box_pack_end(wd->bx, VIEW(subitem));
        wd->items = eina_list_append(wd->items, subitem);
     }
   else
     {
        if (!p_item->submenu.bx) _item_submenu_obj_create(p_item);
        elm_box_pack_end(p_item->submenu.bx, VIEW(subitem));
        p_item->submenu.items = eina_list_append(p_item->submenu.items,
                                                 subitem);
     }
   _sizing_eval(obj);
   return (Elm_Object_Item *) subitem;
}

EAPI Eina_Bool
elm_menu_item_object_content_set(Elm_Object_Item *it, Evas_Object *obj)
{
   _item_content_set_hook(it, NULL, obj);
   return EINA_TRUE;
}

EAPI Evas_Object *
elm_menu_item_object_content_get(const Elm_Object_Item *it)
{
   return _item_content_get_hook(it, NULL);
}

EAPI const char *
elm_menu_item_object_icon_name_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Menu_Item *) it)->icon_str;
}

EAPI Eina_Bool
elm_menu_item_is_separator(Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Menu_Item *) it)->separator;
}

EAPI void
elm_menu_item_del(Elm_Object_Item *it)
{
   elm_object_item_del(it);
}

EAPI void
elm_menu_item_del_cb_set(Elm_Object_Item *it, Evas_Smart_Cb func)
{
   elm_object_item_del_cb_set(it, func);
}

EAPI void *
elm_menu_item_data_get(const Elm_Object_Item *it)
{
   return elm_object_item_data_get(it);
}

EAPI void
elm_menu_item_data_set(Elm_Object_Item *it, const void *data)
{
   elm_object_item_data_set(it, (void *) data);
}

EAPI const Eina_List *
elm_menu_item_subitems_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   return ((Elm_Menu_Item *) it)->submenu.items;
}

EAPI const Eina_List *
elm_menu_items_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   return wd->items;
}

EAPI void
elm_menu_item_selected_set(Elm_Object_Item *it, Eina_Bool selected)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;

   if (selected == item->selected) return;
   item->selected = selected;
   if (selected)
     {
        edje_object_signal_emit(VIEW(item), "elm,state,selected", "elm");
        _menu_item_activate(item, NULL, NULL, NULL);
     }
   else
     {
        edje_object_signal_emit(VIEW(item), "elm,state,unselected", "elm");
        _menu_item_inactivate(item, NULL, NULL, NULL);
     }
   edje_object_message_signal_process(VIEW(item));
}

EAPI Eina_Bool
elm_menu_item_selected_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   return ((Elm_Menu_Item *) it)->selected;
}

EAPI Elm_Object_Item *
elm_menu_item_prev_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;

   if (item->parent)
     {
        Eina_List *l = eina_list_data_find_list(item->parent->submenu.items,
                                                item);
        l = eina_list_prev(l);
        if (!l) return NULL;
        return l->data;
     }
   else
     {
        Widget_Data *wd = elm_widget_data_get(WIDGET(item));
        if (!wd || !wd->items) return NULL;
        Eina_List *l = eina_list_data_find_list(wd->items, item);
        l = eina_list_prev(l);
        if (!l) return NULL;
        return l->data;
     }
   return NULL;
}

EAPI Elm_Object_Item *
elm_menu_item_next_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Menu_Item *item = (Elm_Menu_Item *) it;

   if (item->parent)
     {
        Eina_List *l = eina_list_data_find_list(item->parent->submenu.items,
                                                item);
        l = eina_list_next(l);
        if (!l) return NULL;
        return l->data;
     }
   else
     {
        Widget_Data *wd = elm_widget_data_get(WIDGET(item));
        if (!wd || !wd->items) return NULL;
        Eina_List *l = eina_list_data_find_list(wd->items, item);
        l = eina_list_next(l);
        if (!l) return NULL;
        return l->data;
     }
   return NULL;
}

EINA_DEPRECATED EAPI Evas_Object *
elm_menu_item_menu_get(const Elm_Object_Item *it)
{
   return elm_object_item_widget_get(it);
}

EAPI Elm_Object_Item *
elm_menu_first_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->items) return wd->items->data;
   return NULL;
}

EAPI Elm_Object_Item *
elm_menu_last_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Eina_List *l = eina_list_last(wd->items);
   if (l) return l->data;
   return NULL;
}

EAPI Elm_Object_Item *
elm_menu_selected_item_get(const Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Eina_List *l;
   Elm_Menu_Item *item;
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->selected) return ((Elm_Object_Item *) item);
     }
   return NULL;
}

