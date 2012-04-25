#include <Elementary.h>
#include "elm_priv.h"
#include "els_scroller.h"

#ifndef MAX
# define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef CEIL
#define CEIL(a) (((a) % 2 != 0) ? ((a) / 2 + 1) : ((a) / 2))
#endif

#define DISPLAY_ITEM_NUM_MIN 3

typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Diskselector_Item Elm_Diskselector_Item;

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
   Eina_List *over_items;
   Eina_List *under_items;
   int item_count, len_threshold, len_side, display_item_num;
   Ecore_Idle_Enterer *idler;
   Ecore_Idle_Enterer *check_idler;
   Evas_Coord minw, minh;
   Eina_Bool init:1;
   Eina_Bool round:1;
   Eina_Bool display_item_num_by_api:1;
};

struct _Elm_Diskselector_Item
{
   ELM_WIDGET_ITEM;
   Eina_List *node;
   Evas_Object *icon;
   const char *label;
   Evas_Smart_Cb func;
};

static const char *widtype = NULL;

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
static void _item_click_cb(void *data, Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__);
static void _selected_item_indicate(Elm_Diskselector_Item *it);
static void _item_text_set_hook(Elm_Object_Item *it,
                                const char *part,
                                const char *label);
static const char * _item_text_get_hook(const Elm_Object_Item *it,
                                        const char *part);
static void _item_content_set_hook(Elm_Object_Item *it,
                                   const char *part,
                                   Evas_Object *content);
static Evas_Object * _item_content_get_hook(const Elm_Object_Item *it,
                                            const char *part);

static const char SIG_SELECTED[] = "selected";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";

static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_SELECTED, ""},
       {SIG_SCROLL_ANIM_START, ""},
       {SIG_SCROLL_ANIM_STOP, ""},
       {SIG_SCROLL_DRAG_START, ""},
       {SIG_SCROLL_DRAG_STOP, ""},
       {NULL, NULL}
};

static void
_diskselector_object_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd;
   Evas_Coord w, h;

   wd = elm_widget_data_get(data);
   if (!wd) return;

   if ((wd->minw == -1) && (wd->minh == -1))
      elm_coords_finger_size_adjust(6, &wd->minw, 1, &wd->minh);
   edje_object_size_min_restricted_calc
      (elm_smart_scroller_edje_object_get
          (wd->scroller), &wd->minw, &wd->minh, wd->minw, wd->minh);
   evas_object_size_hint_min_set(obj, wd->minw, wd->minh);
   evas_object_size_hint_max_set(obj, -1, -1);

   evas_object_geometry_get(wd->scroller, NULL, NULL, &w, &h);
   if (wd->round)
     evas_object_resize(wd->main_box, (w / wd->display_item_num) * (wd->item_count + (CEIL(wd->display_item_num) * 2)), h);
   else
     evas_object_resize(wd->main_box, (w / wd->display_item_num) * (wd->item_count + CEIL(wd->display_item_num)), h);

   elm_smart_scroller_paging_set(wd->scroller, 0, 0,
                                 (int)(w / wd->display_item_num), 0);

   if (!wd->idler)
     wd->idler = ecore_idle_enterer_before_add(_move_scroller, data);
}

static void
_item_del(Elm_Diskselector_Item *item)
{
   if (!item) return;
   eina_stringshare_del(item->label);
   if (item->icon)
     evas_object_del(item->icon);
}

static int
_count_letter(const char *str)
{
   int pos = 0;
   int code = 0, chnum;

   for (chnum = 0; ; chnum++)
     {
        pos = evas_string_char_next_get(str, pos, &code);
        if (code == 0) break;
     }
   return chnum;
}

static int
_check_letter(const char *str, int length)
{
   int pos = 0;
   int code = 0, chnum;

   for (chnum = 0; ; chnum++)
     {
        if (chnum == length) break;
        pos = evas_string_char_next_get(str, pos, &code);
        if (code == 0) break;
     }
   return pos;
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
        evas_object_geometry_get(VIEW(it), &x, NULL, &w, NULL);
        /* item not visible */
        if ((x + w <= ox) || (x >= ox + ow))
          continue;

        len = _count_letter(it->label);
//        // FIXME: len should be # of ut8f letters. ie count using utf8 string walk, not stringshare len
//        len = eina_stringshare_strlen(it->label);

        if (x <= ox + 5)
          edje_object_signal_emit(VIEW(it), "elm,state,left_side",
                                  "elm");
        else if (x + w >= ox + ow - 5)
          edje_object_signal_emit(VIEW(it), "elm,state,right_side",
                                  "elm");
        else
          {
             if ((wd->len_threshold) && (len > wd->len_threshold))
               edje_object_signal_emit(VIEW(it), "elm,state,center_small",
                                       "elm");
             else
               edje_object_signal_emit(VIEW(it), "elm,state,center",
                                       "elm");
          }

        // if len is les that the limit len, skip anyway
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
        // limit string len to "length" ut8f chars
        length = _check_letter(it->label, length);
        // cut it off at byte mark returned form _check_letter
        strncpy(buf, it->label, length);
        buf[length] = '\0';
        edje_object_part_text_escaped_set(VIEW(it), "elm.text", buf);
     }

   if (wd->check_idler)
     ecore_idle_enterer_del(wd->check_idler);
   wd->check_idler = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Diskselector_Item *item, *item2, *dit;
   Eina_List *l;
   int i = 0;
   Widget_Data *wd;
   item = (Elm_Diskselector_Item *)it;
   wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return EINA_FALSE;

   elm_box_unpack(wd->main_box, VIEW(item));

   if (wd->round)
     wd->r_items = eina_list_remove(wd->r_items, item);

   wd->items = eina_list_remove(wd->items, item);

   if (wd->selected_item == item)
     {
        dit = (Elm_Diskselector_Item *) eina_list_nth(wd->items, 0);
        if (dit != item)
          wd->selected_item = dit;
        else
          wd->selected_item = eina_list_nth(wd->items, 1);

        _selected_item_indicate(wd->selected_item);
     }

   _item_del(item);
   wd->item_count -= 1;

   if (wd->round)
     {
        if (!wd->item_count)
          {
             evas_object_hide(wd->VIEW(first));
             evas_object_hide(wd->VIEW(second));
             evas_object_hide(wd->VIEW(last));
             evas_object_hide(wd->VIEW(s_last));

             EINA_LIST_FOREACH(wd->under_items, l, item2)
               evas_object_hide(VIEW(item2));

             EINA_LIST_FOREACH(wd->over_items, l, item2)
               evas_object_hide(VIEW(item2));
          }
        else
          {
             dit = eina_list_nth(wd->items, 0);
             if (dit)
               {
                  eina_stringshare_replace(&wd->first->label, dit->label);
                  edje_object_part_text_escaped_set(wd->VIEW(first), "elm.text",
                                            wd->first->label);
               }
             dit = eina_list_nth(wd->items, 1);
             if (dit)
               {
                  eina_stringshare_replace(&wd->second->label, dit->label);
                  edje_object_part_text_escaped_set(wd->VIEW(second), "elm.text",
                                            wd->second->label);
               }
             // if more than 3 itmes should be displayed
             for (i = 2; i < CEIL(wd->display_item_num); i++)
               {
                  dit = eina_list_nth(wd->items, i);
                  item2 = eina_list_nth(wd->over_items, i - 2);
                  eina_stringshare_replace(&item2->label, dit->label);
                  edje_object_part_text_escaped_set(VIEW(item2), "elm.text", item2->label);
               }

             dit = eina_list_nth(wd->items, eina_list_count(wd->items) - 1);
             if (dit)
               {
                  eina_stringshare_replace(&wd->last->label, dit->label);
                  edje_object_part_text_escaped_set(wd->VIEW(last), "elm.text",
                                            wd->last->label);
               }
             dit = eina_list_nth(wd->items, eina_list_count(wd->items) - 2);
             if (dit)
               {
                  eina_stringshare_replace(&wd->s_last->label, dit->label);
                  edje_object_part_text_escaped_set(wd->VIEW(s_last), "elm.text",
                                            wd->s_last->label);
               }
             // if more than 3 itmes should be displayed
             for (i = 3; i <= CEIL(wd->display_item_num); i++)
               {
                  dit = eina_list_nth(wd->items, wd->item_count - i);
                  item2 = eina_list_nth(wd->under_items, i - 3);
                  eina_stringshare_replace(&item2->label, dit->label);
                  edje_object_part_text_escaped_set(VIEW(item2), "elm.text",
                                            item2->label);
               }
          }
     }
   wd->check_idler = ecore_idle_enterer_before_add(_check_string, wd);
   _sizing_eval(wd->self);

   return EINA_TRUE;
}

static Elm_Diskselector_Item *
_item_new(Evas_Object *obj, Evas_Object *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   Elm_Diskselector_Item *it;
   const char *style = elm_widget_style_get(obj);

   it = elm_widget_item_new(obj, Elm_Diskselector_Item);
   if (!it) return NULL;

   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_widget_item_text_set_hook_set(it, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(it, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(it, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(it, _item_content_get_hook);

   it->label = eina_stringshare_add(label);
   it->icon = icon;
   it->func = func;
   it->base.data = data;
   VIEW(it) = edje_object_add(evas_object_evas_get(obj));
   _elm_theme_object_set(obj, VIEW(it), "diskselector", "item", style);
   evas_object_size_hint_weight_set(VIEW(it), EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(VIEW(it), EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(VIEW(it));

   if (it->label)
     {
        edje_object_part_text_escaped_set(VIEW(it), "elm.text", it->label);
        edje_object_signal_callback_add(VIEW(it), "elm,action,click", "", _item_click_cb, it);
     }
   if (it->icon)
     {
        evas_object_size_hint_min_set(it->icon, 24, 24);
        evas_object_size_hint_max_set(it->icon, 40, 40);
        edje_object_part_swallow(VIEW(it), "elm.swallow.icon", it->icon);
        evas_object_show(it->icon);
        elm_widget_sub_object_add(obj, it->icon);
     }
   return it;
}

static void
_theme_data_get(Widget_Data *wd)
{
   const char* str;
   str = edje_object_data_get(wd->right_blank, "len_threshold");
   if (str) wd->len_threshold = MAX(0, atoi(str));
   else wd->len_threshold = 0;

   if (!wd->display_item_num_by_api)
     {
        str = edje_object_data_get(wd->right_blank, "display_item_num");
        if (str) wd->display_item_num = MAX(DISPLAY_ITEM_NUM_MIN, atoi(str));
        else wd->display_item_num = DISPLAY_ITEM_NUM_MIN;
     }

   str = edje_object_data_get(wd->right_blank, "min_width");
   if (str) wd->minw = MAX(-1, atoi(str));
   else wd->minw = -1;

   str = edje_object_data_get(wd->right_blank, "min_height");
   if (str) wd->minh = MAX(-1, atoi(str));
   else wd->minh = -1;
}

static void
_del_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->idler)
     {
        ecore_idle_enterer_del(wd->idler);
        wd->idler = NULL;
     }
   free(wd);
}

static void
_del_pre_hook(Evas_Object * obj)
{
   Elm_Diskselector_Item *it;
   Eina_List *l;

   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->left_blank)
     evas_object_del(wd->left_blank);
   if (wd->right_blank)
     evas_object_del(wd->right_blank);
   if (wd->last)
     {
        eina_stringshare_del(wd->last->label);
        evas_object_del(wd->VIEW(last));
        free(wd->last);
     }
   if (wd->s_last)
     {
        eina_stringshare_del(wd->s_last->label);
        evas_object_del(wd->VIEW(s_last));
        free(wd->s_last);
     }
   if (wd->second)
     {
        eina_stringshare_del(wd->second->label);
        evas_object_del(wd->VIEW(second));
        free(wd->second);
     }
   if (wd->first)
     {
        eina_stringshare_del(wd->first->label);
        evas_object_del(wd->VIEW(first));
        free(wd->first);
     }

   EINA_LIST_FOREACH(wd->under_items, l, it)
     {
        if (it)
          {
             eina_stringshare_del(it->label);
             evas_object_del(VIEW(it));
             free(it);
          }
     }

   EINA_LIST_FOREACH(wd->over_items, l, it)
   {
     if (it)
        {
           eina_stringshare_del(it->label);
           evas_object_del(VIEW(it));
           free(it);
        }
   }

   EINA_LIST_FREE(wd->items, it)
     {
        _item_del(it);
        elm_widget_item_free(it);
     }
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
             _elm_theme_object_set(obj, VIEW(it), "diskselector", "item",
                                   elm_widget_style_get(obj));
             edje_object_part_text_escaped_set(VIEW(it), "elm.text", it->label);
          }
     }
   else
     {
        EINA_LIST_FOREACH(wd->items, l, it)
          {
             _elm_theme_object_set(obj, VIEW(it), "diskselector", "item",
                                   elm_widget_style_get(obj));
             edje_object_part_text_escaped_set(VIEW(it), "elm.text", it->label);
          }
     }
   _elm_theme_object_set(obj, wd->right_blank, "diskselector", "item",
                                   elm_widget_style_get(obj));
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
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   wd->selected_item = it;
   _selected_item_indicate(wd->selected_item);
   if (it->func) it->func((void *)it->base.data, WIDGET(it), it);
   evas_object_smart_callback_call(WIDGET(it), SIG_SELECTED, it);
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

   if ((!strcmp(ev->keyname, "Left")) ||
       ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)) ||
       (!strcmp(ev->keyname, "Up"))  ||
       ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
     {
        l = wd->selected_item->node->prev;
        if ((!l) && (wd->round))
          l = eina_list_last(wd->items);
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)) ||
            (!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
     {
        l = wd->selected_item->node->next;
        if ((!l) && (wd->round))
          l = wd->items;
     }
   else if ((!strcmp(ev->keyname, "Home")) ||
            ((!strcmp(ev->keyname, "KP_Home")) && (!ev->string)))
     l = wd->items;
   else if ((!strcmp(ev->keyname, "End")) ||
            ((!strcmp(ev->keyname, "KP_End")) && (!ev->string)))
     l = eina_list_last(wd->items);
   else return EINA_FALSE;

   if (l)
     it = eina_list_data_get(l);

   if (it)
     {
        wd->selected_item = it;
        if (!wd->idler)
          wd->idler = ecore_idle_enterer_before_add(_move_scroller, obj);
     }

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static void
_selected_item_indicate(Elm_Diskselector_Item *it)
{
   Elm_Diskselector_Item *item;
   Eina_List *l;
   Widget_Data *wd;
   wd = elm_widget_data_get(WIDGET(it));

   if (!wd) return;
   if (!it->label) return;

   EINA_LIST_FOREACH(wd->r_items, l, item)
     {
        if (item->label && !strcmp(item->label, it->label))
           edje_object_signal_emit(VIEW(item), "elm,state,selected", "elm");
        else
           edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
     }
}

static void
_item_click_cb(void *data, Evas_Object *obj __UNUSED__,
               const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Diskselector_Item *it = data;

   if (!it) return;

   Widget_Data *wd;
   wd = elm_widget_data_get(WIDGET(it));

   if (!wd) return;

   if (wd->selected_item != it)
     {
        wd->selected_item = it;
        _selected_item_indicate(wd->selected_item);
     }

   if (it->func) it->func((void *)it->base.data, WIDGET(it), it);
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
        if (x > ((w / wd->display_item_num) * (wd->item_count + (wd->display_item_num % 2))))
           elm_smart_scroller_child_region_show(wd->scroller,
                                               x - ((w / wd->display_item_num) * wd->item_count),
                                               y, w, h);
        else if (x < 0)
           elm_smart_scroller_child_region_show(wd->scroller,
                                               x + ((w / wd->display_item_num) * wd->item_count),
                                               y, w, h);
     }
}

static void
_scroller_stop_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Diskselector_Item *it;
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, w, ow;
   Eina_List *l, *list;

   if (wd->idler) return;

   if (!wd->round)
     list = wd->items;
   else
     list = wd->r_items;

   evas_object_geometry_get(wd->scroller, NULL, NULL, &ow, NULL);
   EINA_LIST_FOREACH(list, l, it)
     {
        evas_object_geometry_get(VIEW(it), &x, NULL, &w, NULL);
        if (abs((int)(ow / 2 - (int)(x + w / 2))) < 10) break;
     }

   if (!it) return;
   _select_item(it);
   evas_object_smart_callback_call(data, SIG_SCROLL_ANIM_STOP, it);
}

static void
_scroller_start_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_ANIM_START,
                                   elm_diskselector_selected_item_get(data));
}

static void
_drag_start_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_START,
                                   elm_diskselector_selected_item_get(data));
}

static void
_drag_stop_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_STOP,
                                   elm_diskselector_selected_item_get(data));
}

static Eina_Bool
_move_scroller(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd;
   Eina_List *list, *l;
   Elm_Diskselector_Item *dit;
   Evas_Coord y, w, h;
   int i;

   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (!wd->round)
     {
        i = 0;
        list = wd->items;
     }
   else
     {
        i = 1;
        list = wd->r_items;
     }

   EINA_LIST_FOREACH(list, l, dit)
     {
        if (wd->selected_item == dit)
           break;
        i++;
     }

   if (wd->round) i -= CEIL(wd->display_item_num);

   if (!dit)
     {
        wd->selected_item =
           (Elm_Diskselector_Item *) eina_list_nth(wd->items, 0);
        return EINA_FALSE;
     }

   evas_object_geometry_get(wd->scroller, NULL, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scroller, w / wd->display_item_num * i, y, w, h);
   _select_item(dit);
   if (wd->idler)
     {
        ecore_idle_enterer_del(wd->idler);
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
   elm_box_unpack(wd->main_box, VIEW(it));
   wd->r_items = eina_list_remove(wd->r_items, it);
   eina_stringshare_del(it->label);
   elm_widget_item_free(it);
}

static void
_round_items_del(Widget_Data *wd)
{
   Eina_List *l;
   Elm_Diskselector_Item * it;

   _round_item_del(wd, wd->last);
   wd->last = NULL;
   _round_item_del(wd, wd->s_last);
   wd->s_last = NULL;
   _round_item_del(wd, wd->second);
   wd->second = NULL;
   _round_item_del(wd, wd->first);
   wd->first = NULL;

   EINA_LIST_FOREACH(wd->under_items, l, it)
     {
        _round_item_del(wd, it);
     }
   wd->under_items = eina_list_free(wd->under_items);

   EINA_LIST_FOREACH(wd->over_items, l, it)
     {
        _round_item_del(wd, it);
     }
   wd->over_items = eina_list_free(wd->over_items);
}

static void
_round_items_add(Widget_Data *wd)
{
   Elm_Diskselector_Item *dit;
   Elm_Diskselector_Item *it;
   Elm_Diskselector_Item *temp_it;
   int i = 0;
   dit = it = eina_list_nth(wd->items, 0);
   if (!dit) return;

   if (!wd->first)
     {
        wd->first = _item_new(WIDGET(it), it->icon, it->label, it->func,
                              it->base.data);
        wd->first->node = it->node;
        wd->r_items = eina_list_append(wd->r_items, wd->first);
     }

   it = eina_list_nth(wd->items, 1);
   if (!it)
     it = dit;
   if (!wd->second)
     {
        wd->second = _item_new(WIDGET(it), it->icon, it->label, it->func,
                               it->base.data);
        wd->second->node = it->node;
        wd->r_items = eina_list_append(wd->r_items, wd->second);
     }

   // if more than 3 itmes should be displayed
   for (i = 2; i < CEIL(wd->display_item_num); i++)
     {
        it = eina_list_nth(wd->items, i);
        if (!it) it = dit;
        temp_it = _item_new(WIDGET(it), it->icon, it->label, it->func, it->base.data);
        wd->over_items = eina_list_append(wd->over_items, temp_it);
        wd->r_items = eina_list_append(wd->r_items, temp_it);
     }

   it = eina_list_nth(wd->items, wd->item_count - 1);
   if (!it)
     it = dit;
   if (!wd->last)
     {
        wd->last = _item_new(WIDGET(it), it->icon, it->label, it->func,
                             it->base.data);
        wd->last->node = it->node;
        wd->r_items = eina_list_prepend(wd->r_items, wd->last);
     }

   it = eina_list_nth(wd->items, wd->item_count - 2);
   if (!it)
     it = dit;
   if (!wd->s_last)
     {
        wd->s_last = _item_new(WIDGET(it), it->icon, it->label, it->func,
                               it->base.data);
        wd->s_last->node = it->node;
        wd->r_items = eina_list_prepend(wd->r_items, wd->s_last);
     }

   // if more than 3 itmes should be displayed
   for (i = 3; i <= CEIL(wd->display_item_num); i++)
     {
        it = eina_list_nth(wd->items, wd->item_count - i);
        if (!it) it = dit;
        temp_it = _item_new(WIDGET(it), it->icon, it->label, it->func, it->base.data);
        wd->under_items = eina_list_append(wd->under_items, temp_it);
        wd->r_items = eina_list_prepend(wd->r_items, temp_it);
     }
}

static void
_item_icon_set(Elm_Diskselector_Item *it, Evas_Object *icon)
{
   if (it->icon == icon) return;
   if (it->icon) evas_object_del(it->icon);
   it->icon = icon;
   if (VIEW(it))
     {
        evas_object_size_hint_min_set(it->icon, 24, 24);
        evas_object_size_hint_max_set(it->icon, 40, 40);
        edje_object_part_swallow(VIEW(it), "elm.swallow.icon", it->icon);
        evas_object_show(it->icon);
        elm_widget_sub_object_add(WIDGET(it), it->icon);
     }
}

static void
_check_identical_item(Elm_Diskselector_Item *it, Evas_Object *icon)
{
   Widget_Data *wd;
   Elm_Diskselector_Item *dit;
   Eina_List *l;
   int idx = 0;
   int ic = 0;
   int ac = 0;

   wd = elm_widget_data_get(WIDGET(it));
   if (!wd) return;

   if (wd->round)
     {
        // Get index from indentical item from round items
        EINA_LIST_FOREACH(wd->r_items, l, dit)
          {
             if (it == dit) break;
             idx++;
          }

        // No item to match
        ic = eina_list_count(wd->r_items);
        if (idx >= ic) return;
        dit = NULL;

        // Number of added items: CEIL(wd->display_item_num)
        ac = CEIL(wd->display_item_num);

        if (((idx >= 0) && (idx < ac)) ||
            ((idx >= ac) && (idx < (2 * ac))))
          {
              // Selected item: under, low region
             dit = eina_list_nth(wd->r_items,
                                 idx + ic - (2 * ac));
          }
        else if (((idx >= (ic - ac)) && (idx < ic)) ||
                 ((idx >= (ic - (2 * ac))) && (idx < ic - ac)))
          {
              // Selected item: over, high region
              dit = eina_list_nth(wd->r_items,
                                  idx - ic + (2 * ac));
          }

        if (dit) _item_icon_set(dit, icon);
        _sizing_eval(wd->self);
     }
}

static void
_item_text_set_hook(Elm_Object_Item *it, const char *part, const char *label)
{
   Elm_Diskselector_Item *item;
   if (part && strcmp(part, "default")) return;
   item = (Elm_Diskselector_Item *)it;
   eina_stringshare_replace(&item->label, label);
   edje_object_part_text_escaped_set(VIEW(item), "elm.text", item->label);
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;
   return ((Elm_Diskselector_Item *)it)->label;
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   if (part && strcmp(part, "icon")) return;
   _item_icon_set((Elm_Diskselector_Item *)it, content);
   _check_identical_item((Elm_Diskselector_Item *)it, content);
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it, const char *part)
{
   if (part && strcmp(part, "icon")) return NULL;
   return ((Elm_Diskselector_Item *)it)->icon;
}


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
   wd->display_item_num_by_api = EINA_FALSE;

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
                                  _scroller_stop_cb, obj);
   evas_object_smart_callback_add(wd->scroller, "animate,start",
                                  _scroller_start_cb, obj);
   evas_object_smart_callback_add(wd->scroller, "drag,stop",
                                  _drag_stop_cb, obj);
   evas_object_smart_callback_add(wd->scroller, "drag,start",
                                  _drag_start_cb, obj);

   _elm_theme_object_set(obj, wd->scroller, "diskselector", "base",
                         "default");
   evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE,
                                  _diskselector_object_resize, obj);

   wd->main_box = elm_box_add(parent);
   elm_box_horizontal_set(wd->main_box, EINA_TRUE);
   elm_box_homogeneous_set(wd->main_box, EINA_TRUE);
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

EAPI Eina_Bool
elm_diskselector_round_enabled_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->round;
}

EAPI void
elm_diskselector_round_enabled_set(Evas_Object * obj, Eina_Bool enabled)
{
   Eina_List *elist;
   Elm_Diskselector_Item *it;

   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->round == enabled)
     return;

   wd->round = !!enabled;
   if (enabled)
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
          elm_box_pack_start(wd->main_box, wd->VIEW(last));
        if (wd->s_last)
          elm_box_pack_start(wd->main_box, wd->VIEW(s_last));

        // if more than 3 items should be displayed
        EINA_LIST_FOREACH(wd->under_items, elist, it)
           elm_box_pack_start(wd->main_box, VIEW(it));

        if (wd->first)
          elm_box_pack_end(wd->main_box, wd->VIEW(first));
        if (wd->second)
          elm_box_pack_end(wd->main_box, wd->VIEW(second));

        // if more than 3 items should be displayed
        EINA_LIST_FOREACH(wd->over_items, elist, it)
           elm_box_pack_end(wd->main_box, VIEW(it));
     }
   else
     {
        _round_items_del(wd);
        elm_box_pack_start(wd->main_box, wd->left_blank);
        elm_box_pack_end(wd->main_box, wd->right_blank);
        eina_list_free(wd->r_items);
        wd->r_items = NULL;
     }

   _selected_item_indicate(wd->selected_item);
   _sizing_eval(obj);
}

EAPI int
elm_diskselector_side_text_max_length_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->len_side;
}

EAPI void
elm_diskselector_side_text_max_length_set(Evas_Object *obj, int len)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->len_side = len;
}

EAPI void
elm_diskselector_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->scroller)
     elm_smart_scroller_bounce_allow_set(wd->scroller, h_bounce, v_bounce);
}

EAPI void
elm_diskselector_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_get(wd->scroller, h_bounce, v_bounce);
}

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

EAPI void
elm_diskselector_scroller_policy_set(Evas_Object *obj, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->scroller)) return;
   if ((policy_h >= ELM_SCROLLER_POLICY_LAST) ||
       (policy_v >= ELM_SCROLLER_POLICY_LAST))
     return;
   elm_smart_scroller_policy_set(wd->scroller, policy_h, policy_v);
}

EAPI void
elm_diskselector_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Diskselector_Item *it;

   if (!wd) return;
   if (!wd->items) return;

   wd->selected_item = NULL;
   EINA_LIST_FREE(wd->items, it)
     {
        _item_del(it);
        elm_widget_item_free(it);
     }
   _round_items_del(wd);
   _sizing_eval(obj);
}

EAPI const Eina_List *
elm_diskselector_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}

EAPI Elm_Object_Item *
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
          elm_box_pack_start(wd->main_box, wd->VIEW(last));
        if (wd->s_last)
          elm_box_pack_start(wd->main_box, wd->VIEW(s_last));
        elm_box_pack_end(wd->main_box, VIEW(it));
        if (wd->first)
          elm_box_pack_end(wd->main_box, wd->VIEW(first));
        if (wd->second)
          elm_box_pack_end(wd->main_box, wd->VIEW(second));
     }
   else
     {
        elm_box_unpack(wd->main_box, wd->right_blank);
        elm_box_pack_end(wd->main_box, VIEW(it));
        elm_box_pack_end(wd->main_box, wd->right_blank);
     }
   if (!wd->selected_item)
     wd->selected_item = it;
   if (!wd->idler)
     wd->idler = ecore_idle_enterer_before_add(_move_scroller, obj);
   _sizing_eval(obj);
   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_diskselector_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return (Elm_Object_Item *) wd->selected_item;
}

EAPI void
elm_diskselector_item_selected_set(Elm_Object_Item *it, Eina_Bool selected)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it);
   Widget_Data *wd;
   Elm_Diskselector_Item *item = (Elm_Diskselector_Item *)it;
   wd = elm_widget_data_get(WIDGET(item));
   if (!wd) return;

   if ((wd->selected_item == item) && (selected))
     return;

   if ((wd->selected_item == item) && (!selected))
     wd->selected_item = eina_list_data_get(wd->items);
   else
     {
        wd->selected_item = item;
        _selected_item_indicate(wd->selected_item);
     }

   if (!wd->idler)
     wd->idler = ecore_idle_enterer_before_add(_move_scroller, WIDGET(item));
}

EAPI Eina_Bool
elm_diskselector_item_selected_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   Widget_Data *wd = elm_widget_data_get(WIDGET(it));
   if (!wd) return EINA_FALSE;
   return (wd->selected_item == ((Elm_Diskselector_Item *)it));
}

EAPI Elm_Object_Item *
elm_diskselector_item_prev_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Diskselector_Item *item = (Elm_Diskselector_Item *)it;
   if (item->node->prev) return item->node->prev->data;
   else return NULL;
}

EAPI Elm_Object_Item *
elm_diskselector_item_next_get(const Elm_Object_Item *it)
{
   ELM_OBJ_ITEM_CHECK_OR_RETURN(it, NULL);
   Elm_Diskselector_Item *item = (Elm_Diskselector_Item *)it;
   if (item->node->next) return item->node->next->data;
   else return NULL;
}

EAPI Elm_Object_Item *
elm_diskselector_first_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(wd->items);
}

EAPI Elm_Object_Item *
elm_diskselector_last_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(eina_list_last(wd->items));
}

EAPI void
elm_diskselector_display_item_num_set(Evas_Object *obj, int num)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (num < DISPLAY_ITEM_NUM_MIN) num = DISPLAY_ITEM_NUM_MIN;
   wd->display_item_num = num;
   wd->display_item_num_by_api = EINA_TRUE;
}

EAPI int
elm_diskselector_display_item_num_get(const Evas_Object *item)
{
   ELM_CHECK_WIDTYPE(item, widtype) (-1);
   Widget_Data *wd = elm_widget_data_get(item);
   if (!wd) return -1;
   return wd->display_item_num;
}
