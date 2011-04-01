/**
 * @defgroup Diskselector
 *
 * A diskselector is a kind of list widget. It scrolls horizontally,
 * and can contain label and icon objects. Three items are displayed
 * with the selected on the middle.
 *
 * It can act like a circular list with round mode and labels can be
 * reduced for a defined lenght for side items.
 *
 * Signal emitted by this widget:
 * "selected" - when item is selected (scroller stops)
 */

#include <Elementary.h>
#include "elm_priv.h"

#ifndef MAX
# define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *self;
   Evas_Object *scroller;
   Evas_Object *main_box;
   Evas_Object *left_blank;
   Evas_Object *right_blank;
   Elm_Diskselector_Item *selected_item;
   Elm_Diskselector_Item *first;
   Elm_Diskselector_Item *second;
   Elm_Diskselector_Item *s_last;
   Elm_Diskselector_Item *last;
   Eina_List *items;
   Eina_List *r_items;
   int item_count, len_threshold, len_side;
   Ecore_Idler *idler;
   Ecore_Idler *check_idler;
   Eina_Bool init:1;
   Eina_Bool round:1;
};

struct _Elm_Diskselector_Item
{
   Elm_Widget_Item base;
   Eina_List *node;
   Evas_Object *icon;
   const char *label;
   Evas_Smart_Cb func;
};

static const char *widtype = NULL;

#define ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, ...)                    \
   ELM_WIDGET_ITEM_CHECK_OR_RETURN((Elm_Widget_Item *)it, __VA_ARGS__); \
   ELM_CHECK_WIDTYPE(it->base.widget, widtype) __VA_ARGS__;

static Eina_Bool _move_scroller(void *data);
static void _del_hook(Evas_Object * obj);
static void _del_pre_hook(Evas_Object * obj);
static void _sizing_eval(Evas_Object * obj);
static void _theme_hook(Evas_Object * obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src, Evas_Callback_Type type, void *event_info);
static void _sub_del(void *data, Evas_Object * obj, void *event_info);
static void _round_items_del(Widget_Data *wd);
static void _scroller_move_cb(void *data, Evas_Object *obj, void *event_info);

static const char SIG_SELECTED[] = "selected";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_SELECTED, ""},
       {NULL, NULL}
};

static void
_diskselector_object_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd;
   Evas_Coord w, h, minw = -1, minh = -1;

   wd = elm_widget_data_get(data);
   if (!wd) return;

   elm_coords_finger_size_adjust(6, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(elm_smart_scroller_edje_object_get(
         wd->scroller), &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);

   evas_object_geometry_get(wd->scroller, NULL, NULL, &w, &h);
   if (wd->round)
     evas_object_resize(wd->main_box, w / 3 * (wd->item_count + 4), h);
   else
     evas_object_resize(wd->main_box, w / 3 * (wd->item_count + 2), h);

   elm_smart_scroller_paging_set(wd->scroller, 0, 0,
                                 (int)(w / 3), 0);

   if (!wd->idler)
     wd->idler = ecore_idler_add(_move_scroller, data);
}

static Elm_Diskselector_Item *
_item_new(Evas_Object *obj, Evas_Object *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   Elm_Diskselector_Item *it;
   const char *style = elm_widget_style_get(obj);

   it = elm_widget_item_new(obj, Elm_Diskselector_Item);
   if (!it) return NULL;

   it->label = eina_stringshare_add(label);
   it->icon = icon;
   it->func = func;
   it->base.data = data;
   it->base.view = edje_object_add(evas_object_evas_get(obj));
   _elm_theme_object_set(obj, it->base.view, "diskselector", "item", style);
   evas_object_size_hint_weight_set(it->base.view, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(it->base.view, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(it->base.view);

   if (it->label)
        edje_object_part_text_set(it->base.view, "elm.text", it->label);
   if (it->icon)
     {
        evas_object_size_hint_min_set(it->icon, 24, 24);
        evas_object_size_hint_max_set(it->icon, 40, 40);
        edje_object_part_swallow(it->base.view, "elm.swallow.icon", it->icon);
        evas_object_show(it->icon);
        elm_widget_sub_object_add(obj, it->icon);
     }
   return it;
}

static void
_item_del(Elm_Diskselector_Item *item)
{
   if (!item) return;
   eina_stringshare_del(item->label);
   if (item->icon)
     evas_object_del(item->icon);
   elm_widget_item_del(item);
}

static void
_theme_data_get(Widget_Data *wd)
{
   const char* str;
   str = edje_object_data_get(wd->right_blank, "len_threshold");
   if (str) wd->len_threshold = atoi(str);
   else wd->len_threshold = 0;
}

static void
_del_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void
_del_pre_hook(Evas_Object * obj)
{
   Elm_Diskselector_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->left_blank)
     evas_object_del(wd->left_blank);
   if (wd->right_blank)
     evas_object_del(wd->right_blank);
   if (wd->last)
     {
        eina_stringshare_del(wd->last->label);
        evas_object_del(wd->last->base.view);
        free(wd->last);
     }
   if (wd->s_last)
     {
        eina_stringshare_del(wd->s_last->label);
        evas_object_del(wd->s_last->base.view);
        free(wd->s_last);
     }
   if (wd->second)
     {
        eina_stringshare_del(wd->second->label);
        evas_object_del(wd->second->base.view);
        free(wd->second);
     }
   if (wd->first)
     {
        eina_stringshare_del(wd->first->label);
        evas_object_del(wd->first->base.view);
        free(wd->first);
     }

   EINA_LIST_FREE(wd->items, it) _item_del(it);
   eina_list_free(wd->r_items);
}

static void
_sizing_eval(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _diskselector_object_resize(obj, NULL, obj, NULL);
}

static void
_theme_hook(Evas_Object * obj)
{
   Eina_List *l;
   Elm_Diskselector_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->scroller)
     elm_smart_scroller_object_theme_set(obj, wd->scroller, "diskselector",
                                         "base", elm_widget_style_get(obj));
   if (wd->round)
     {
        EINA_LIST_FOREACH(wd->r_items, l, it)
          {
             _elm_theme_object_set(obj, it->base.view, "diskselector", "item",
                                   elm_widget_style_get(obj));
          }
     }
   else
     {
        EINA_LIST_FOREACH(wd->items, l, it)
          {
             _elm_theme_object_set(obj, it->base.view, "diskselector", "item",
                                   elm_widget_style_get(obj));
          }
     }
   _theme_data_get(wd);
   _sizing_eval(obj);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object * obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   Elm_Diskselector_Item *it;
   const Eina_List *l;

   if (!wd) return;
   if (!sub) abort();
   if (sub == wd->scroller)
     wd->scroller = NULL;
   else
     {
        EINA_LIST_FOREACH(wd->items, l, it)
          {
             if (sub == it->icon)
               {
                  it->icon = NULL;
                  _sizing_eval(obj);
                  break;
               }
          }
     }
}

static void
_select_item(Elm_Diskselector_Item *it)
{
   if (!it) return;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   wd->selected_item = it;
   if (it->func) it->func((void *)it->base.data, it->base.widget, it);
   evas_object_smart_callback_call(it->base.widget, SIG_SELECTED, it);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)
     return;

   if (elm_widget_focus_get(obj))
     {
	edje_object_signal_emit(wd->self, "elm,action,focus", "elm");
	evas_object_focus_set(wd->self, EINA_TRUE);
     }
   else
     {
	edje_object_signal_emit(wd->self, "elm,action,unfocus", "elm");
	evas_object_focus_set(wd->self, EINA_FALSE);
     }
}

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   Elm_Diskselector_Item *it = NULL;
   Eina_List *l;

   if (!wd->selected_item) {
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
   }

   if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")) ||
       (!strcmp(ev->keyname, "Up"))  || (!strcmp(ev->keyname, "KP_Up")))
     {
        l = wd->selected_item->node->prev;
        if ((!l) && (wd->round))
          l = eina_list_last(wd->items);
     }
   else if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")) ||
            (!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
        l = wd->selected_item->node->next;
        if ((!l) && (wd->round))
          l = wd->items;
     }
   else if ((!strcmp(ev->keyname, "Home")) || (!strcmp(ev->keyname, "KP_Home")))
     l = wd->items;
   else if ((!strcmp(ev->keyname, "End")) || (!strcmp(ev->keyname, "KP_End")))
     l = eina_list_last(wd->items);
   else return EINA_FALSE;

   if (l)
     it = eina_list_data_get(l);

   if (it)
     {
        wd->selected_item = it;
        if (!wd->idler)
          wd->idler = ecore_idler_add(_move_scroller, obj);
     }

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static int
_check_letter(const char *str, int length)
{
   int code = str[length];

   if (code == '\0')
     return length;		// null string
   else if (((code >= 65) && (code <= 90)) || ((code >= 97) && (code <= 122)))
     return length;		// alphabet
   else if ((48 <= code) && (code < 58))
     return length;		// number
   else if (((33 <= code) && (code < 47)) || ((58 <= code) && (code < 64))
            || ((91 <= code) && (code < 96)) || ((123 <= code) && (code < 126)))
     return length;		// special letter
   return length - 1;
}

static Eina_Bool
_check_string(void *data)
{
   int mid, steps, length, diff;
   Elm_Diskselector_Item *it;
   Eina_List *list, *l;
   Evas_Coord ox, ow;
   char buf[1024];
   Widget_Data *wd = data;

   evas_object_geometry_get(wd->scroller, &ox, NULL, &ow, NULL);
   if (ow <= 0)
     return EINA_FALSE;
   if (!wd->init)
     return EINA_FALSE;
   if (!wd->round)
     list = wd->items;
   else
     list = wd->r_items;

   EINA_LIST_FOREACH(list, l, it)
     {
        Evas_Coord x, w;
        int len;
        evas_object_geometry_get(it->base.view, &x, NULL, &w, NULL);
        /* item not visible */
        if ((x + w <= ox) || (x >= ox + ow))
          continue;

        len = eina_stringshare_strlen(it->label);

        if (x <= ox + 5)
             edje_object_signal_emit(it->base.view, "elm,state,left_side",
                                     "elm");
        else if (x + w >= ox + ow - 5)
             edje_object_signal_emit(it->base.view, "elm,state,right_side",
                                     "elm");
        else
          {
             if ((wd->len_threshold) && (len > wd->len_threshold))
               edje_object_signal_emit(it->base.view, "elm,state,center_small",
                                       "elm");
             else
               edje_object_signal_emit(it->base.view, "elm,state,center",
                                       "elm");
          }

        if (len <= wd->len_side)
          continue;

        steps = len - wd->len_side + 1;
        mid = x + w / 2;
        if (mid <= ox + ow / 2)
          diff = (ox + ow / 2) - mid;
        else
          diff = mid - (ox + ow / 2);

        length = len - (int)(diff * steps / (ow / 3));
        length = MAX(length, wd->len_side);
        length = _check_letter(it->label, length);
        strncpy(buf, it->label, length);
        buf[length] = '\0';
        edje_object_part_text_set(it->base.view, "elm.text", buf);
     }

   if (wd->check_idler)
     ecore_idler_del(wd->check_idler);
   wd->check_idler = NULL;
   return EINA_FALSE;
}

static void
_scroller_move_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Coord x, y, w, h, bw;
   Widget_Data *wd = data;

   _check_string(wd);
   elm_smart_scroller_child_pos_get(obj, &x, &y);
   elm_smart_scroller_child_viewport_size_get(obj, &w, &h);
   if (wd->round)
     {
        evas_object_geometry_get(wd->main_box, NULL, NULL, &bw, NULL);
        if (x > w / 3 * (wd->item_count + 1))
          elm_smart_scroller_child_region_show(wd->scroller,
                                               x - w / 3 * wd->item_count,
                                               y, w, h);
        else if (x < 0)
          elm_smart_scroller_child_region_show(wd->scroller,
                                               x + w / 3 * wd->item_count,
                                               y, w, h);
     }
}

static void
_scroller_stop_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Diskselector_Item *it;
   Widget_Data *wd = data;
   Evas_Coord x, w, ow;
   Eina_List *l, *list;

   if (wd->idler)
     return;

   if (!wd->round)
     list = wd->items;
   else
     list = wd->r_items;

   evas_object_geometry_get(wd->scroller, NULL, NULL, &ow, NULL);
   EINA_LIST_FOREACH(list, l, it)
     {
        evas_object_geometry_get(it->base.view, &x, NULL, &w, NULL);
        if (abs((int)(ow / 2 - (int)(x + w / 2))) < 10)
          break;
     }

   if (!it)
     return;

   _select_item(it);
}

static Eina_Bool
_move_scroller(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd;
   Eina_List *l;
   Elm_Diskselector_Item *dit;
   Evas_Coord y, w, h;
   int i;

   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (wd->round)
     i = 1;
   else
     i = 0;

   EINA_LIST_FOREACH(wd->items, l, dit)
     {
        if (wd->selected_item == dit)
          break;
        i++;
     }
   if (!dit)
     {
        wd->selected_item =
           (Elm_Diskselector_Item *) eina_list_nth(wd->items, 0);
        return EINA_FALSE;
     }

   evas_object_geometry_get(wd->scroller, NULL, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scroller, w / 3 * i, y, w, h);
   _select_item(dit);
   if (wd->idler)
     {
        ecore_idler_del(wd->idler);
        wd->idler = NULL;
     }
   wd->init = EINA_TRUE;
   _check_string(wd);

   return EINA_TRUE;
}

static void
_round_item_del(Widget_Data *wd, Elm_Diskselector_Item *it)
{
   if (!it) return;
   elm_box_unpack(wd->main_box, it->base.view);
   wd->r_items = eina_list_remove(wd->r_items, it);
   eina_stringshare_del(it->label);
   evas_object_del(it->base.view);
   free(it);
}

static void
_round_items_del(Widget_Data *wd)
{
   _round_item_del(wd, wd->last);
   wd->last = NULL;
   _round_item_del(wd, wd->s_last);
   wd->s_last = NULL;
   _round_item_del(wd, wd->second);
   wd->second = NULL;
   _round_item_del(wd, wd->first);
   wd->first = NULL;
}

static void
_round_items_add(Widget_Data *wd)
{
   Elm_Diskselector_Item *dit;
   Elm_Diskselector_Item *it;

   dit = it = eina_list_nth(wd->items, 0);
   if (!dit) return;

   if (!wd->first)
     {
        wd->first = _item_new(it->base.widget, it->icon, it->label, it->func,
                              it->base.data);
        wd->first->node = it->node;
        wd->r_items = eina_list_append(wd->r_items, wd->first);
     }

   it = eina_list_nth(wd->items, 1);
   if (!it)
     it = dit;
   if (!wd->second)
     {
        wd->second = _item_new(it->base.widget, it->icon, it->label, it->func,
                               it->base.data);
        wd->second->node = it->node;
        wd->r_items = eina_list_append(wd->r_items, wd->second);
     }

   it = eina_list_nth(wd->items, wd->item_count - 1);
   if (!it)
     it = dit;
   if (!wd->last)
     {
        wd->last = _item_new(it->base.widget, it->icon, it->label, it->func,
                             it->base.data);
        wd->last->node = it->node;
        wd->r_items = eina_list_prepend(wd->r_items, wd->last);
     }

   it = eina_list_nth(wd->items, wd->item_count - 2);
   if (!it)
     it = dit;
   if (!wd->s_last)
     {
        wd->s_last = _item_new(it->base.widget, it->icon, it->label, it->func,
                               it->base.data);
        wd->s_last->node = it->node;
        wd->r_items = eina_list_prepend(wd->r_items, wd->s_last);
     }
}

/**
 * Add a new diskselector object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Diskselector
 */
EAPI Evas_Object *
elm_diskselector_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
   
   ELM_SET_WIDTYPE(widtype, "diskselector");
   elm_widget_type_set(obj, "diskselector");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_event_hook_set(obj, _event_hook);

   wd->self = obj;
   wd->item_count = 0;
   wd->round = EINA_FALSE;
   wd->init = EINA_FALSE;
   wd->len_side = 3;

   wd->scroller = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scroller, obj);
   _theme_hook(obj);
   elm_widget_resize_object_set(obj, wd->scroller);
   elm_smart_scroller_policy_set(wd->scroller, ELM_SMART_SCROLLER_POLICY_OFF,
                           ELM_SMART_SCROLLER_POLICY_OFF);
   elm_smart_scroller_bounce_allow_set(wd->scroller, EINA_TRUE, EINA_FALSE);
   evas_object_smart_callback_add(wd->scroller, "scroll", _scroller_move_cb,
                                  wd);
   evas_object_smart_callback_add(wd->scroller, "animate,stop",
                                  _scroller_stop_cb, wd);
   _elm_theme_object_set(obj, wd->scroller, "diskselector", "base",
                         "default");
   evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE,
                                  _diskselector_object_resize, obj);

   wd->main_box = elm_box_add(parent);
   elm_box_horizontal_set(wd->main_box, EINA_TRUE);
   elm_box_homogenous_set(wd->main_box, EINA_TRUE);
   evas_object_size_hint_weight_set(wd->main_box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->main_box, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   _elm_theme_object_set(obj, wd->main_box, "diskselector", "base",
                         "default");
   elm_widget_sub_object_add(obj, wd->main_box);

   elm_smart_scroller_child_set(wd->scroller, wd->main_box);

   wd->left_blank = edje_object_add(evas_object_evas_get(obj));
   _elm_theme_object_set(obj, wd->left_blank, "diskselector", "item",
                         "default");
   evas_object_size_hint_weight_set(wd->left_blank, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->left_blank, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_box_pack_end(wd->main_box, wd->left_blank);
   evas_object_show(wd->left_blank);

   wd->right_blank = edje_object_add(evas_object_evas_get(obj));
   _elm_theme_object_set(obj, wd->right_blank, "diskselector", "item",
                         "default");
   evas_object_size_hint_weight_set(wd->right_blank, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->right_blank, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_box_pack_end(wd->main_box, wd->right_blank);
   evas_object_show(wd->right_blank);

   _theme_data_get(wd);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_smart_callbacks_descriptions_set(obj, _signals);
   _sizing_eval(obj);
   return obj;
}

/**
 * Get round mode
 *
 * If round mode is activated the items list will work like a circle list,
 * so when the user reaches the last item, the first one will popup.
 *
 * @param obj The diskselector object
 * @return if or not set round mode or false if not a valid diskselector
 *
 * @ingroup Diskselector
 */
EAPI Eina_Bool
elm_diskselector_round_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->round;
}

/**
 * Set round mode
 *
 * If round mode is activated the items list will work like a circle list,
 * so when the user reaches the last item, the first one will popup.
 *
 * @param it The item of diskselector
 * @param if or not set round mode
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_round_set(Evas_Object * obj, Eina_Bool round)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->round == round)
     return;

   wd->round = round;
   if (round)
     {
        wd->r_items = eina_list_clone(wd->items);
        elm_box_unpack(wd->main_box, wd->left_blank);
        evas_object_hide(wd->left_blank);
        elm_box_unpack(wd->main_box, wd->right_blank);
        evas_object_hide(wd->right_blank);
        if (!wd->items)
          return;

        _round_items_add(wd);

        if (wd->last)
          elm_box_pack_start(wd->main_box, wd->last->base.view);
        if (wd->s_last)
          elm_box_pack_start(wd->main_box, wd->s_last->base.view);
        if (wd->first)
          elm_box_pack_end(wd->main_box, wd->first->base.view);
        if (wd->second)
          elm_box_pack_end(wd->main_box, wd->second->base.view);
     }
   else
     {
        _round_items_del(wd);
        elm_box_pack_start(wd->main_box, wd->left_blank);
        elm_box_pack_end(wd->main_box, wd->right_blank);
        eina_list_free(wd->r_items);
        wd->r_items = NULL;
     }
   _sizing_eval(obj);
}

/**
 * Get the side labels max lenght
 *
 * @param obj The diskselector object
 * @return The max lenght defined for side labels, or 0 if not a valid
 * diskselector
 *
 * @ingroup Diskselector
 */
EAPI int
elm_diskselector_side_label_lenght_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->len_side;
}

/**
 * Set the side labels max lenght
 *
 * @param obj The diskselector object
 * @param len The max lenght defined for side labels
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_side_label_lenght_set(Evas_Object *obj, int len)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->len_side = len;
}

/**
 * Set bounce mode
 *
 * This will enable or disable the scroller bounce mode for the diskselector.
 * See elm_scroller_bounce_set() for details. Horizontal bounce is enabled by
 * default.
 *
 * @param obj The diskselector object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scroller)
     elm_smart_scroller_bounce_allow_set(wd->scroller, h_bounce, v_bounce);
}

/**
 * Get the bounce mode
 *
 * @param obj The Diskselector object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_get(wd->scroller, h_bounce, v_bounce);
}

/**
 * Get the scrollbar policy
 *
 * This sets the scrollbar visibility policy for the given scroller.
 * ELM_SMART_SCROLLER_POLICY_AUTO means the scrollber is made visible if it
 * is needed, and otherwise kept hidden. ELM_SMART_SCROLLER_POLICY_ON turns
 * it on all the time, and ELM_SMART_SCROLLER_POLICY_OFF always keeps it off.
 * This applies respectively for the horizontal and vertical scrollbars.
 * The both are disabled by default.
 *
 * @param obj The diskselector object
 * @param policy_h Horizontal scrollbar policy
 * @param policy_v Vertical scrollbar policy
 *
 * @ingroup Diskselector
 */

EAPI void
elm_diskselector_scroller_policy_get(const Evas_Object *obj, Elm_Scroller_Policy *policy_h, Elm_Scroller_Policy *policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Elm_Smart_Scroller_Policy s_policy_h, s_policy_v;
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->scroller)) return;
   elm_smart_scroller_policy_get(wd->scroller, &s_policy_h, &s_policy_v);
   *policy_h = (Elm_Scroller_Policy) s_policy_h;
   *policy_v = (Elm_Scroller_Policy) s_policy_v;
}


/**
 * Set the scrollbar policy
 *
 * This sets the scrollbar visibility policy for the given scroller.
 * ELM_SMART_SCROLLER_POLICY_AUTO means the scrollber is made visible if it
 * is needed, and otherwise kept hidden. ELM_SMART_SCROLLER_POLICY_ON turns
 * it on all the time, and ELM_SMART_SCROLLER_POLICY_OFF always keeps it off.
 * This applies respectively for the horizontal and vertical scrollbars.
 * The both are disabled by default.
 *
 * @param obj The diskselector object
 * @param policy_h Horizontal scrollbar policy
 * @param policy_v Vertical scrollbar policy
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_scroller_policy_set(Evas_Object *obj, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((policy_h >= 3) || (policy_v >= 3)) return;
   if (wd->scroller)
     elm_smart_scroller_policy_set(wd->scroller, policy_h, policy_v);
}

/**
 * Clears a diskselector of all items.
 *
 * @param obj The diskselector object
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Diskselector_Item *it;

   if (!wd) return;
   if (!wd->items) return;

   wd->selected_item = NULL;
   EINA_LIST_FREE(wd->items, it) _item_del(it);
   _round_items_del(wd);
   _sizing_eval(obj);
}

/**
 * Returns a list of all the diskselector items.
 *
 * @param obj The diskselector object
 * @return An Eina_List* of the diskselector items, or NULL on failure
 *
 * @ingroup Diskselector
 */
EAPI const Eina_List *
elm_diskselector_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}

/**
 * Appends an item to the diskselector object.
 *
 * @param obj The diskselector object
 * @param label The label of the diskselector item
 * @param icon The icon object to use for the left side of the item
 * @param func The function to call when the item is selected
 * @param data The data to associate with the item for related callbacks
 *
 * @return The created item or NULL upon failure
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_item_append(Evas_Object *obj, const char *label, Evas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   Elm_Diskselector_Item *it;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(obj, icon, label, func, data);
   wd->items = eina_list_append(wd->items, it);
   it->node = eina_list_last(wd->items);
   wd->item_count++;
   if (wd->round)
     {
        _round_items_del(wd);
        wd->r_items = eina_list_append(wd->r_items, it);
        _round_items_add(wd);
        if (wd->last)
          elm_box_pack_start(wd->main_box, wd->last->base.view);
        if (wd->s_last)
          elm_box_pack_start(wd->main_box, wd->s_last->base.view);
        elm_box_pack_end(wd->main_box, it->base.view);
        if (wd->first)
          elm_box_pack_end(wd->main_box, wd->first->base.view);
        if (wd->second)
          elm_box_pack_end(wd->main_box, wd->second->base.view);
     }
   else
     {
        elm_box_unpack(wd->main_box, wd->right_blank);
        elm_box_pack_end(wd->main_box, it->base.view);
        elm_box_pack_end(wd->main_box, wd->right_blank);
     }
   if (!wd->selected_item)
     wd->selected_item = it;
   if (!wd->idler)
     wd->idler = ecore_idler_add(_move_scroller, obj);
   _sizing_eval(obj);
   return it;
}

/**
 * Delete the item
 *
 * @param it The item of diskselector
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_del(Elm_Diskselector_Item * it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it);
   Elm_Diskselector_Item *dit;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   if (!wd) return;

   elm_box_unpack(wd->main_box, it->base.view);

   if (wd->round)
     wd->r_items = eina_list_remove(wd->r_items, it);

   wd->items = eina_list_remove(wd->items, it);

   if (wd->selected_item == it)
     {
        dit = (Elm_Diskselector_Item *) eina_list_nth(wd->items, 0);
        if (dit != it)
          wd->selected_item = dit;
        else
          wd->selected_item = eina_list_nth(wd->items, 1);
     }

   _item_del(it);
   wd->item_count -= 1;

   if (wd->round)
     {
        if (!wd->item_count)
          {
             evas_object_hide(wd->first->base.view);
             evas_object_hide(wd->second->base.view);
             evas_object_hide(wd->last->base.view);
             evas_object_hide(wd->s_last->base.view);
          }
        else
          {
             dit = eina_list_nth(wd->items, 0);
             if (dit)
               {
                  eina_stringshare_replace(&wd->first->label, dit->label);
                  edje_object_part_text_set(wd->first->base.view, "elm.text",
                                            wd->first->label);
               }
             dit = eina_list_nth(wd->items, 1);
             if (dit)
               {
                  eina_stringshare_replace(&wd->second->label, dit->label);
                  edje_object_part_text_set(wd->second->base.view, "elm.text",
                                            wd->second->label);
               }
             dit = eina_list_nth(wd->items, eina_list_count(wd->items) - 1);
             if (dit)
               {
                  eina_stringshare_replace(&wd->last->label, dit->label);
                  edje_object_part_text_set(wd->last->base.view, "elm.text",
                                            wd->last->label);
               }
             dit = eina_list_nth(wd->items, eina_list_count(wd->items) - 2);
             if (dit)
               {
                  eina_stringshare_replace(&wd->s_last->label, dit->label);
                  edje_object_part_text_set(wd->s_last->base.view, "elm.text",
                                            wd->s_last->label);
               }
          }
     }
   wd->check_idler = ecore_idler_add(_check_string, wd);
   _sizing_eval(wd->self);
}

/**
 * Get the label of item
 *
 * @param it The item of diskselector
 * @return The label of item
 *
 * @ingroup Diskselector
 */
EAPI const char *
elm_diskselector_item_label_get(const Elm_Diskselector_Item * it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);
   return it->label;
}

/**
 * Set the label of item
 *
 * @param it The item of diskselector
 * @param label The label of item
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_label_set(Elm_Diskselector_Item * it, const char *label)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it);
   eina_stringshare_replace(&it->label, label);
   edje_object_part_text_set(it->base.view, "elm.text", it->label);
}

/**
 * Get the selected item
 *
 * @param obj The diskselector object
 * @return The selected diskselector item
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->selected_item;
}

/**
 * Set the selected state of an item
 *
 * This sets the selected state (EINA_TRUE selected, EINA_FALSE not selected)
 * of the given item @p it.
 * If a new item is selected the previosly selected will be unselected.
 * If the item @p it is unselected, the first item will be selected.
 *
 * @param it The diskselector item
 * @param selected The selected state
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_selected_set(Elm_Diskselector_Item *it, Eina_Bool selected)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it);
   Widget_Data *wd;
   wd = elm_widget_data_get(it->base.widget);
   if (!wd) return;

   if ((wd->selected_item == it) && (selected))
     return;

   if ((wd->selected_item == it) && (!selected))
     wd->selected_item = eina_list_data_get(wd->items);
   else
     wd->selected_item = it;

   if (!wd->idler)
     ecore_idler_add(_move_scroller, it->base.widget);
}

/*
 * Get the selected state of @p item.
 *
 * @param it The diskselector item
 * @return If true, the item is selected
 *
 * @ingroup Diskselector
 */
EAPI Eina_Bool
elm_diskselector_item_selected_get(const Elm_Diskselector_Item *it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   Widget_Data *wd;

   wd = elm_widget_data_get(it->base.widget);
   if (!wd) return EINA_FALSE;
   return (wd->selected_item == it);
}

/**
 * Set the function called when a diskselector item is freed.
 *
 * @param it The item to set the callback on
 * @param func The function called
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_del_cb_set(Elm_Diskselector_Item *it, Evas_Smart_Cb func)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it);
   elm_widget_item_del_cb_set(it, func);
}

/**
 * Returns the data associated with the item.
 *
 * @param it The diskselector item
 * @return The data associated with @p it
 *
 * @ingroup Diskselector
 */
EAPI void *
elm_diskselector_item_data_get(const Elm_Diskselector_Item *it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);
   return elm_widget_item_data_get(it);
}

/**
 * Returns the icon associated with the item.
 *
 * @param it The diskselector item
 * @return The icon associated with @p it
 *
 * @ingroup Diskselector
 */
EAPI Evas_Object *
elm_diskselector_item_icon_get(const Elm_Diskselector_Item *it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);
   return it->icon;
}

/**
 * Sets the icon associated with the item.
 *
 * Once the icon object is set, a previously set one will be deleted.
 * You probably don't want, then, to have the <b>same</b> icon object set
 * for more than one item of the diskselector.
 *
 * @param it The diskselector item
 * @param icon The icon object to associate with @p it
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_icon_set(Elm_Diskselector_Item *it, Evas_Object *icon)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it);
   if (it->icon == icon) return;
   if (it->icon)
     evas_object_del(it->icon);
   it->icon = icon;
   if (it->base.view)
     edje_object_part_swallow(it->base.view, "elm.swallow.icon", icon);
}

/**
 * Gets the item before @p it in the list.
 *
 * @param it The diskselector item
 * @return The item before @p it, or NULL on failure
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_item_prev_get(const Elm_Diskselector_Item *it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);
   if (it->node->prev) return it->node->prev->data;
   else return NULL;
}

/**
 * Gets the item after @p it in the list.
 *
 * @param it The diskselector item
 * @return The item after @p it, or NULL on failure
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_item_next_get(const Elm_Diskselector_Item *it)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);
   if (it->node->next) return it->node->next->data;
   else return NULL;
}

/**
 * Get the first item in the diskselector
 *
 * @param obj The diskselector object
 * @return The first item, or NULL if none
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_first_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
     return NULL;

   return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the diskselector
 *
 * @param obj The diskselector object
 * @return The last item, or NULL if none
 *
 * @ingroup Diskselector
 */
EAPI Elm_Diskselector_Item *
elm_diskselector_last_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;

   Widget_Data *wd;
   wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
     return NULL;

   return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Set the text to be shown in the diskselector item.
 *
 * @param item Target item
 * @param text The text to set in the content
 *
 * Setup the text as tooltip to object. The item can have only one tooltip,
 * so any previous tooltip data is removed.
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_tooltip_text_set(Elm_Diskselector_Item *item, const char *text)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
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
 * @param item the diskselector item being attached a tooltip.
 * @param func the function used to create the tooltip contents.
 * @param data what to provide to @a func as callback data/context.
 * @param del_cb called when data is not needed anymore, either when
 *        another callback replaces @func, the tooltip is unset with
 *        elm_diskselector_item_tooltip_unset() or the owner @a item
 *        dies. This callback receives as the first parameter the
 *        given @a data, and @c event_info is the item.
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_tooltip_content_cb_set(Elm_Diskselector_Item *item, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_content_cb_set(item, func, data, del_cb);
}

/**
 * Unset tooltip from item
 *
 * @param item diskselector item to remove previously set tooltip.
 *
 * Remove tooltip from item. The callback provided as del_cb to
 * elm_diskselector_item_tooltip_content_cb_set() will be called to notify
 * it is not used anymore.
 *
 * @see elm_diskselector_item_tooltip_content_cb_set()
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_tooltip_unset(Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_unset(item);
}

/**
 * Sets a different style for this item tooltip.
 *
 * @note before you set a style you should define a tooltip with
 *       elm_diskselector_item_tooltip_content_cb_set() or
 *       elm_diskselector_item_tooltip_text_set()
 *
 * @param item diskselector item with tooltip already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_tooltip_style_set(Elm_Diskselector_Item *item, const char *style)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_tooltip_style_set(item, style);
}

/**
 * Get the style for this item tooltip.
 *
 * @param item diskselector item with tooltip already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a tooltip set, then NULL is returned.
 *
 * @ingroup Diskselector
 */
EAPI const char *
elm_diskselector_item_tooltip_style_get(const Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_tooltip_style_get(item);
}

/**
 * Set the cursor to be shown when mouse is over the diskselector item
 *
 * @param item Target item
 * @param cursor the cursor name to be used.
 *
 * @see elm_object_cursor_set()
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_cursor_set(Elm_Diskselector_Item *item, const char *cursor)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_set(item, cursor);
}

/**
 * Get the cursor to be shown when mouse is over the diskselector item
 *
 * @param item diskselector item with cursor already set.
 * @return the cursor name.
 *
 * @ingroup Diskselector
 */
EAPI const char *
elm_diskselector_item_cursor_get(const Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_cursor_get(item);
}

/**
 * Unset the cursor to be shown when mouse is over the diskselector item
 *
 * @param item Target item
 *
 * @see elm_object_cursor_unset()
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_cursor_unset(Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_unset(item);
}

/**
 * Sets a different style for this item cursor.
 *
 * @note before you set a style you should define a cursor with
 *       elm_diskselector_item_cursor_set()
 *
 * @param item diskselector item with cursor already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_cursor_style_set(Elm_Diskselector_Item *item, const char *style)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
   elm_widget_item_cursor_style_set(item, style);
}

/**
 * Get the style for this item cursor.
 *
 * @param item diskselector item with cursor already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a cursor set, then NULL is returned.
 *
 * @ingroup Diskselector
 */
EAPI const char *
elm_diskselector_item_cursor_style_get(const Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item, NULL);
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
 * @ingroup Diskselector
 */
EAPI void
elm_diskselector_item_cursor_engine_only_set(Elm_Diskselector_Item *item, Eina_Bool engine_only)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item);
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
 * @ingroup Diskselector
 */
EAPI Eina_Bool
elm_diskselector_item_cursor_engine_only_get(const Elm_Diskselector_Item *item)
{
   ELM_DISKSELECTOR_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_widget_item_cursor_engine_only_get(item);
}
