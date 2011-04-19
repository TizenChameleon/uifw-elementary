/**
 * @defgroup Controlbar Controlbar
 * @ingroup Elementary
 *
 * This is a Controlbar. It can contain label and icon objects.
 * In edit mode, you can change the location of items.
 */

#include <string.h>
#include <math.h>

#include <Elementary.h>
#include "elm_priv.h"

#ifndef EAPI
#define EAPI __attribute__ ((visibility("default")))
#endif

#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))
#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

#define TABBAR 0
#define TOOLBAR 1
#define OBJECT 2

typedef struct _Animation_Data Animation_Data;

struct _Animation_Data
{
   Evas_Object * obj;
   Evas_Coord fx;
   Evas_Coord fy;
   Evas_Coord fw;
   Evas_Coord fh;
   Evas_Coord tx;
   Evas_Coord ty;
   Evas_Coord tw;
   Evas_Coord th;
   unsigned int start_time;
   double time;
   void (*func) (void *data, Evas_Object * obj);
   void *data;
   Ecore_Animator * timer;
};

// internal data structure of controlbar object
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object * object;
   Evas_Object * parent;
   Evas_Object * view;
   Evas_Object * edje;
   Evas_Object * bg;
   Evas_Object * box;
   Evas_Object * event_box;
   Evas_Object * selected_box;
   Evas_Object * focused_box;
   Evas_Object * focused_box_left;
   Evas_Object * focused_box_right;

   Elm_Controlbar_Item * more_item;
   Elm_Controlbar_Item * pre_item;
   Elm_Controlbar_Item * cur_item;
   Evas_Coord x, y, w, h;
   Eina_Bool disabled;
   Eina_Bool vertical;
   Eina_Bool auto_align;
   int mode;
   int alpha;
   int num;
   int animating;
   Eina_List * items;
   Eina_List * visible_items;

   void (*ani_func) (void *data, Evas_Object * obj, void *event_info);
   void *ani_data;
   Ecore_Timer *effect_timer;
   Eina_Bool init_animation;
   Eina_Bool selected_animation;
   Animation_Data *ad;

   const char *pressed_signal;
   const char *selected_signal;
};

struct _Elm_Controlbar_Item
{
   Widget_Data * wd;
   Evas_Object * obj;
   Evas_Object * base;
   Evas_Object * base_item;
   Evas_Object * view;
   Evas_Object * label;
   Evas_Object * icon;
   const char *icon_path;
   const char *text;
   void (*func) (void *data, Evas_Object * obj, void *event_info);
   void *data;
   int order;
   int sel;
   int style;
   Eina_Bool selected;
   Eina_Bool disable;
};

static const char *widtype = NULL;
// prototype
static int check_bar_item_number(Widget_Data *wd);
static void selected_box(Elm_Controlbar_Item * it);
static void cancel_selected_box(Widget_Data *wd);
static Eina_Bool pressed_box(Elm_Controlbar_Item * it);
static void item_color_set(Elm_Controlbar_Item *item, const char *color_part);

///////////////////////////////////////////////////////////////////
//
//  Smart Object basic function
//
////////////////////////////////////////////////////////////////////

static void
_controlbar_move(void *data, Evas_Object * obj)
{
   Widget_Data * wd;
   Evas_Coord x, y, x_, y_, width;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_geometry_get(wd->edje, &x, &y, NULL, NULL);
   wd->x = x;
   wd->y = y;
   evas_object_move(wd->edje, x, y);
   evas_object_geometry_get(elm_layout_content_get(wd->edje, "bg_image"), NULL, NULL, &width, NULL);
   evas_object_geometry_get(wd->parent, &x_, &y_, NULL, NULL);
   switch(wd->mode)
     {
      case ELM_CONTROLBAR_MODE_LEFT:
         evas_object_move(wd->view, x + width, y);
         break;
      case ELM_CONTROLBAR_MODE_RIGHT:
      default:
         evas_object_move(wd->view, x, y);
         break;
     }
   evas_object_move(wd->event_box, x, y);
}

static void
_controlbar_resize(void *data, Evas_Object * obj)
{
   Widget_Data * wd;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Coord x, y, x_, y_, w, h, width, height;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_geometry_get(wd->edje, &x, &y, &w, &h);
   wd->w = w;
   wd->h = h;
   evas_object_resize(wd->edje, w, h);
   evas_object_geometry_get(elm_layout_content_get(wd->edje, "bg_image"), NULL, NULL, &width, &height);
   evas_object_geometry_get(wd->parent, &x_, &y_, NULL, NULL);
   switch(wd->mode)
     {
      case ELM_CONTROLBAR_MODE_LEFT:
      case ELM_CONTROLBAR_MODE_RIGHT:
         evas_object_resize(wd->view, w - width, h);
         break;
      default:
         evas_object_resize(wd->view, w, h - height + 1);
         break;
     }
   evas_object_resize(wd->event_box, w, h);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if(item->label)
          {
             if(!wd->vertical)
               {
                  elm_label_wrap_width_set(item->label, (int)(wd->w/check_bar_item_number(wd))-20);
               }
             else
               {
                  elm_label_wrap_width_set(item->label, (int)(wd->w-20));
               }
          }
     }
}

static void
_controlbar_object_move(void *data, Evas * e, Evas_Object * obj,
                        void *event_info)
{
   _controlbar_move(data, obj);
}

static void
_controlbar_object_resize(void *data, Evas * e, Evas_Object * obj,
                          void *event_info)
{
   _controlbar_resize(data, obj);
}

static void
_controlbar_object_show(void *data, Evas * e, Evas_Object * obj,
                        void *event_info)
{
   Widget_Data * wd;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_show(wd->view);
   evas_object_show(wd->edje);
   evas_object_show(wd->box);
   evas_object_show(wd->event_box);
}

static void
_controlbar_object_hide(void *data, Evas * e, Evas_Object * obj,
                        void *event_info)
{
   Widget_Data * wd;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_hide(wd->view);
   evas_object_hide(wd->edje);
   evas_object_hide(wd->box);
   evas_object_hide(wd->event_box);

   cancel_selected_box(wd);
}

static void
_item_del(Elm_Controlbar_Item *it)
{
   if(!it) return;
   Widget_Data *wd = elm_widget_data_get(it->obj);
   if(!wd) return;

   if(it == wd->more_item)
     if(it->view)
       evas_object_del(it->view);
   if (it->text)
     eina_stringshare_del(it->text);
   if (it->label)
     evas_object_del(it->label);
   if (it->icon_path)
     eina_stringshare_del(it->icon_path);
   if (it->icon)
     evas_object_del(it->icon);
   if (it->base)
     {
        if (it->style != OBJECT)
          evas_object_del(it->base);
        else
          evas_object_hide(it->base);
     }
   if (it->base_item)
     evas_object_del(it->base_item);
   if (it->view)
     {
        evas_object_hide(it->view);
     }
}

static void
_del_hook(Evas_Object * obj)
{
   Widget_Data * wd = elm_widget_data_get(obj);
   Elm_Controlbar_Item * item;
   if (!wd) return;

   EINA_LIST_FREE(wd->items, item)
     {
        _item_del(item);
        free(item);
        item = NULL;
     }
   if (wd->bg)
     {
        evas_object_del(wd->bg);
        wd->bg = NULL;
     }
   if (wd->box)
     {
        evas_object_del(wd->box);
        wd->box = NULL;
     }
   if (wd->event_box)
     {
        evas_object_del(wd->event_box);
        wd->event_box = NULL;
     }
   if (wd->focused_box)
     {
        evas_object_del(wd->focused_box);
        wd->focused_box = NULL;
     }
   if (wd->focused_box_left)
     {
        evas_object_del(wd->focused_box_left);
        wd->focused_box_left = NULL;
     }
   if (wd->focused_box_right)
     {
        evas_object_del(wd->focused_box_right);
        wd->focused_box_right = NULL;
     }
   if (wd->edje)
     {
        evas_object_del(wd->edje);
        wd->edje = NULL;
     }
   if (wd->effect_timer)
     {
        ecore_timer_del(wd->effect_timer);
        wd->effect_timer = NULL;
     }
   if (wd->ad)
     {
        if (wd->ad->timer) ecore_animator_del(wd->ad->timer);
        wd->ad->timer = NULL;
        free(wd->ad);
        wd->ad = NULL;
     }

   free(wd);
   wd = NULL;
}

static void
_theme_hook(Evas_Object * obj)
{
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   int r, g, b;

   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_layout_theme_set(wd->edje, "controlbar", "base",
                         elm_widget_style_get(obj));
   elm_layout_theme_set(wd->bg, "controlbar", "background",
                         elm_widget_style_get(obj));
   evas_object_color_get(wd->bg, &r, &g, &b, NULL);
   evas_object_color_set(wd->bg, r, g, b, (int)(255 * wd->alpha / 100));
   elm_layout_theme_set(wd->view, "controlbar", "view", elm_widget_style_get(obj));
   elm_layout_theme_set(wd->focused_box, "controlbar", "item_bg_move", elm_widget_style_get(obj));
   elm_layout_theme_set(wd->focused_box_left, "controlbar", "item_bg_move", elm_widget_style_get(obj));
   elm_layout_theme_set(wd->focused_box_right, "controlbar", "item_bg_move", elm_widget_style_get(obj));
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->style != OBJECT)
          {
             elm_layout_theme_set(item->base, "controlbar", "item_bg", elm_widget_style_get(obj));
             if (item->label && item->icon)
               edje_object_signal_emit(_EDJ(item->base_item), "elm,state,icon_text", "elm");
             if (item->selected)
               selected_box(item);
          }
     }
}

static void
_disable_hook(Evas_Object * obj)
{
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd) return;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Eina_Bool disabled;

   wd->disabled = elm_widget_disabled_get(obj);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (wd->disabled)
          disabled = wd->disabled;
        else
          disabled = item->disable;

        if (item->style == OBJECT)
          {
             if (item->base) elm_widget_disabled_set(item->base, disabled);
          }
        else
          {
             if (disabled)
               item_color_set(item, "elm.item.disable.color");
             else
               item_color_set(item, "elm.item.default.color");
          }
     }
}

static void
_sub_del(void *data, Evas_Object * obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   Evas_Object *content;
   if (!wd) return;

   if (sub == wd->view)
     {
        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        evas_object_hide(content);
     }
}

static void
_sizing_eval(Evas_Object * obj)
{
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd) return;
   _controlbar_move(obj, obj);
   _controlbar_resize(obj, obj);
}

/////////////////////////////////////////////////////////////
//
// animation function
//
/////////////////////////////////////////////////////////////

static unsigned int
current_time_get()
{
   struct timeval timev;

   gettimeofday(&timev, NULL);
   return ((timev.tv_sec * 1000) + ((timev.tv_usec) / 1000));
}

static Eina_Bool
move_evas_object(void *data)
{
   double t;

   int dx, dy, dw, dh;

   int px, py, pw, ph;

   int x, y, w, h;

   Animation_Data * ad = (Animation_Data *) data;
   t = ELM_MAX(0.0, current_time_get() - ad->start_time) / 1000;
   dx = ad->tx - ad->fx;
   dy = ad->ty - ad->fy;
   dw = ad->tw - ad->fw;
   dh = ad->th - ad->fh;
   if (t <= ad->time)
     {
        x = (1 * sin((t / ad->time) * (M_PI / 2)) * dx);
        y = (1 * sin((t / ad->time) * (M_PI / 2)) * dy);
        w = (1 * sin((t / ad->time) * (M_PI / 2)) * dw);
        h = (1 * sin((t / ad->time) * (M_PI / 2)) * dh);
     }
   else
     {
        x = dx;
        y = dy;
        w = dw;
        h = dh;
     }
   px = ad->fx + x;
   py = ad->fy + y;
   pw = ad->fw + w;
   ph = ad->fh + h;
   if (x == dx && y == dy && w == dw && h == dh)
     {
        if(ad->timer) ecore_animator_del(ad->timer);
        ad->timer = NULL;
        evas_object_move(ad->obj, px, py);
        evas_object_resize(ad->obj, pw, ph);
        evas_object_show(ad->obj);
        if (ad->func != NULL)
          ad->func(ad->data, ad->obj);
        return ECORE_CALLBACK_CANCEL;
     }
   else
     {
        evas_object_move(ad->obj, px, py);
        evas_object_resize(ad->obj, pw, ph);
        evas_object_show(ad->obj);
     }
   return ECORE_CALLBACK_RENEW;
}

static Animation_Data*
move_object_with_animation(Evas_Object * obj, Evas_Coord x, Evas_Coord y,
                           Evas_Coord w, Evas_Coord h, Evas_Coord x_,
                           Evas_Coord y_, Evas_Coord w_, Evas_Coord h_,
                           double time, Eina_Bool (*mv_func) (void *data),
                           void (*func) (void *data,
                                         Evas_Object * obj), void *data)
{
   Animation_Data * ad = (Animation_Data *) malloc(sizeof(Animation_Data));
   ad->obj = obj;
   ad->fx = x;
   ad->fy = y;
   ad->fw = w;
   ad->fh = h;
   ad->tx = x_;
   ad->ty = y_;
   ad->tw = w_;
   ad->th = h_;
   ad->start_time = current_time_get();
   ad->time = time;
   ad->func = func;
   ad->data = data;
   ad->timer = ecore_animator_add(mv_func, ad);

   return ad;
}

static Eina_Bool
item_animation_effect(void *data)
{
   Widget_Data *wd = (Widget_Data *)data;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int rand;

   srand(time(NULL));

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        rand = random()%100;
        if(rand > 65 && item->order > 0)
          {
             rand = random()%4;
             switch(rand)
               {
                case 0:
                   break;
                case 1:
                   break;
                case 2:
                   break;
                case 3:
                   break;
                default:
                   break;
               }
             break;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

/////////////////////////////////////////////////////////////
//
// callback function
//
/////////////////////////////////////////////////////////////

static int
sort_cb(const void *d1, const void *d2)
{
   Elm_Controlbar_Item * item1, *item2;
   item1 = (Elm_Controlbar_Item *) d1;
   item2 = (Elm_Controlbar_Item *) d2;
   if (item1->order <= 0) return 1;
   if (item2->order <= 0) return -1;
   return item1->order > item2->order ? 1 : -1;
}

///////////////////////////////////////////////////////////////////
//
//  basic utility function
//
////////////////////////////////////////////////////////////////////

static Eina_Bool
item_exist_check(Widget_Data *wd, Elm_Controlbar_Item *item)
{
   const Eina_List *l;
   Elm_Controlbar_Item *it;

   if(!wd) return EINA_FALSE;
   if(!wd->items) return EINA_FALSE;

   EINA_LIST_FOREACH(wd->items, l, it)
      if(it == item) return EINA_TRUE;

   return EINA_FALSE;
}

static void
check_background(Widget_Data *wd)
{
   if(!wd) return;
   Eina_List *l;
   Elm_Controlbar_Item *it;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if(it->style == TABBAR)
          {
             if(wd->mode == ELM_CONTROLBAR_MODE_LEFT)
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar_left", "elm");
             else if(wd->mode == ELM_CONTROLBAR_MODE_RIGHT)
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar_right", "elm");
             else
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar", "elm");
             return;
          }
     }
   edje_object_signal_emit(_EDJ(wd->bg), "elm,state,toolbar", "elm");
}

static void
check_toolbar_line(Widget_Data *wd)
{
   if(!wd) return;
   Eina_List *l;
   Elm_Controlbar_Item *it, *it2;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        it2 = elm_controlbar_item_prev(it);
        if(!it2) continue;
        if(it->style != TOOLBAR || it2->style != TOOLBAR) continue;

        if(wd->vertical)
          {
             edje_object_signal_emit(_EDJ(it2->base), "elm,state,right_line_hide", "elm");
             edje_object_signal_emit(_EDJ(it->base), "elm,state,left_line_hide", "elm");

             if((it->icon || it->label) && (it2->icon || it2->label))
               {
                  edje_object_signal_emit(_EDJ(it2->base), "elm,state,bottom_line_show", "elm");
                  edje_object_signal_emit(_EDJ(it->base), "elm,state,top_line_show", "elm");
               }
             else
               {
                  edje_object_signal_emit(_EDJ(it2->base), "elm,state,bottom_line_hide", "elm");
                  edje_object_signal_emit(_EDJ(it->base), "elm,state,top_line_hide", "elm");
               }
          }
        else
          {
             edje_object_signal_emit(_EDJ(it2->base), "elm,state,bottom_line_hide", "elm");
             edje_object_signal_emit(_EDJ(it->base), "elm,state,top_line_hide", "elm");

             if((it->icon || it->label) && (it2->icon || it2->label))
               {
                  edje_object_signal_emit(_EDJ(it2->base), "elm,state,right_line_show", "elm");
                  edje_object_signal_emit(_EDJ(it->base), "elm,state,left_line_show", "elm");
               }
             else
               {
                  edje_object_signal_emit(_EDJ(it2->base), "elm,state,right_line_hide", "elm");
                  edje_object_signal_emit(_EDJ(it->base), "elm,state,left_line_hide", "elm");
               }
          }
     }
}

static int
check_bar_item_number(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int num = 0;

   EINA_LIST_FOREACH(wd->items, l, item)
      if(item->order > 0) num++;

   return num;
}

static void
item_insert_in_bar(Elm_Controlbar_Item * it, int order)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   int check = 0;

   if(order == 0) return;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->order == order && item != it)
          check = 1;
     }
   if (check)
     {
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item->order > 0)
               elm_table_unpack(wd->box, item->base);
          }
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item->order > 0)
               {
                  if (item->order >= order)
                    item->order += 1;
                  if(!wd->vertical)
                    elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
                  else
                    elm_table_pack(wd->box, item->base, 0, item->order - 1, 1, 1);
                  evas_object_show(item->base);
               }
          }
     }
   it->order = order;
   if(!wd->vertical)
     elm_table_pack(wd->box, it->base, it->order - 1, 0, 1, 1);
   else
     elm_table_pack(wd->box, it->base, 0, it->order - 1, 1, 1);
   evas_object_show(it->base);
}

static void
item_delete_in_bar(Elm_Controlbar_Item * it)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   int i = 0;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item == it)
          {
             i = it->order;
             it->order = 0;
             elm_table_unpack(wd->box, it->base);
             evas_object_hide(it->base);
          }
     }
   if (i)
     {
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item->order > i)
               {
                  item->order--;
                  elm_table_unpack(wd->box, item->base);
                  if(!wd->vertical)
                    elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
                  else
                    elm_table_pack(wd->box, item->base, 0, item->order - 1, 1, 1);
               }
          }
     }
}

static void
item_visible_set(Elm_Controlbar_Item *it, Eina_Bool visible)
{
   Elm_Controlbar_Item *item;
   Eina_Bool check = EINA_TRUE;

   if(!it) return;
   if (it->obj == NULL) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items) return;
   if (it->order <= 0) check = EINA_FALSE;
   if (check == visible) return;
   if (visible)
     {
        item = elm_controlbar_last_item_get(it->obj);
        while(!elm_controlbar_item_visible_get(item)){
             item = elm_controlbar_item_prev(item);
        }
        item_insert_in_bar(it, item->order + 1);
     }
   else
     {
        item_delete_in_bar(it);
     }
   wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), sort_cb);
   _sizing_eval(it->obj);
}

static Eina_Bool
hide_selected_box(void *data)
{
   Evas_Object *selected_box = (Evas_Object *)data;

   evas_object_move(selected_box, -999, -999);
   evas_object_hide(selected_box);

   return ECORE_CALLBACK_CANCEL;
}

static void
item_color_set(Elm_Controlbar_Item *item, const char *color_part)
{
   Evas_Object *color;
   int r, g, b, a;

   color = edje_object_part_object_get(_EDJ(item->base), color_part);
   if (color)
     evas_object_color_get(color, &r, &g, &b, &a);
   evas_object_color_set(item->label, r, g, b, a);
   evas_object_color_set(item->icon, r, g, b, a);
}

static void
_end_selected_box(void *data, Evas_Object *obj)
{
   Widget_Data * wd = (Widget_Data *)data;

   if(item_exist_check(wd, wd->cur_item))
     {
        edje_object_signal_emit(_EDJ(wd->cur_item->base), wd->selected_signal, "elm");
        edje_object_signal_emit(_EDJ(wd->cur_item->base_item), "elm,state,shadow_show", "elm");
     }

   wd->animating--;
   if (wd->animating < 0)
     {
        printf("animation error\n");
        wd->animating = 0;
     }

   ecore_idler_add(hide_selected_box, wd->selected_box);
}

static void
move_selected_box(Widget_Data *wd, Elm_Controlbar_Item * fit, Elm_Controlbar_Item * tit)
{
   Evas_Coord fx, fy, fw, fh, tx, ty, tw, th;
   Evas_Object *from, *to;

   if(fit->order <= 0 && wd->auto_align)
     fit = wd->more_item;

   from = edje_object_part_object_get(_EDJ(fit->base), "bg_img");
   evas_object_geometry_get(from, &fx, &fy, &fw, &fh);

   to = edje_object_part_object_get(_EDJ(tit->base), "bg_img");
   evas_object_geometry_get(to, &tx, &ty, &tw, &th);

   if(item_exist_check(wd, wd->pre_item))
     {
        edje_object_signal_emit(_EDJ(wd->pre_item->base), "elm,state,unselected", "elm");
        edje_object_signal_emit(_EDJ(wd->pre_item->base_item), "elm,state,shadow_hide", "elm");
     }
   if(item_exist_check(wd, wd->cur_item))
     edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,state,unselected", "elm");

   wd->animating++;
   if (wd->ad)
     {
        if (wd->ad->timer) ecore_animator_del(wd->ad->timer);
        wd->ad->timer = NULL;
        free(wd->ad);
        wd->ad = NULL;
     }
   wd->ad = move_object_with_animation(wd->selected_box, fx, fy, fw, fh, tx, ty, tw, th,
                                       0.3, move_evas_object, _end_selected_box, wd);
}

static void
selected_box(Elm_Controlbar_Item * it)
{
   if(!it) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   const Eina_List *l;
   Elm_Controlbar_Item * item, *fit = NULL;
   Evas_Object * content;

   if(wd->animating) return;

   wd->cur_item = it;

   if(it->style == TABBAR){

        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        EINA_LIST_FOREACH(wd->items, l, item){
             if(item->selected) {
                  fit = item;
                  wd->pre_item = fit;
             }
             item->selected = EINA_FALSE;
        }
        it->selected = EINA_TRUE;

        if(fit != NULL && fit != it)
          {
             move_selected_box(wd, fit, it);
          }
        else
          {
             edje_object_signal_emit(_EDJ(it->base), wd->selected_signal, "elm");
             edje_object_signal_emit(_EDJ(wd->cur_item->base_item), "elm,state,shadow_show", "elm");
          }

        if(fit != it)
          {
             if(wd->more_item != it)
               evas_object_smart_callback_call(it->obj, "view,change,before", it);
          }

        elm_layout_content_set(wd->view, "elm.swallow.view", it->view);

        if(content) evas_object_hide(content);

   }else if(it->style == TOOLBAR){
        edje_object_signal_emit(_EDJ(it->base), "elm,state,text_unselected", "elm");
        if (it->func)
          it->func(it->data, it->obj, it);
   }
}

static void
cancel_selected_box(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->style == TABBAR)
          {
             if(item->selected)
               {
                  edje_object_signal_emit(_EDJ(item->base), wd->selected_signal, "elm");
               }
             else
               {
                  edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
                  edje_object_signal_emit(_EDJ(item->base_item), "elm,state,shadow_hide", "elm");
               }
          }
        else if (item->style == TOOLBAR)
          {
             edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
          }
     }
}

static void
unpressed_box_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Widget_Data * wd = (Widget_Data *) data;
   Evas_Event_Mouse_Up * ev = event_info;
   Evas_Coord x, y, w, h;

   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_UP, unpressed_box_cb);

   cancel_selected_box(wd);

   if(item_exist_check(wd, wd->pre_item))
     {
        evas_object_geometry_get(wd->pre_item->base, &x, &y, &w, &h);
        if(ev->output.x > x && ev->output.x < x+w && ev->output.y > y && ev->output.y < y+h)
          {
             selected_box(wd->pre_item);
          }
     }
   return;
}

static Eina_Bool
pressed_box(Elm_Controlbar_Item * it)
{
   Widget_Data * wd = elm_widget_data_get(it->obj);
   int check = 0;
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   if(wd->animating) return EINA_FALSE;

   if(wd->disabled || it->disable) return EINA_FALSE;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (it == item)
          {
             if (it->style == TABBAR)
               {
                  edje_object_signal_emit(_EDJ(it->base), wd->pressed_signal, "elm");
               }
             else if (it->style == TOOLBAR)
               {
                  edje_object_signal_emit(_EDJ(it->base), "elm,state,toolbar_pressed", "elm");
               }
             evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_UP, unpressed_box_cb, (void *)wd);

             check = EINA_TRUE;
          }
     }
   if (!check)
     return EINA_FALSE;

   wd->pre_item = it;

   return EINA_TRUE;
}

static Evas_Object *
create_item_label(Evas_Object *obj, Elm_Controlbar_Item * it, char *part)
{

   Evas_Object *label;
   label = elm_label_add(obj);
   elm_object_style_set(label, "controlbar");
   elm_label_label_set(label, it->text);
   evas_object_color_set(label, 255, 255, 255, 255);
   elm_label_line_wrap_set(label, EINA_TRUE);
   elm_label_ellipsis_set(label, EINA_TRUE);
   elm_label_wrap_mode_set(label, 1);

   elm_layout_content_set(obj, part, label);

   return label;
}

static Evas_Object *
create_item_icon(Evas_Object *obj, Elm_Controlbar_Item * it, char *part)
{

   Evas_Object *icon;
   icon = elm_icon_add(obj);
   if(!elm_icon_standard_set(icon, it->icon_path))
     {
        elm_icon_file_set(icon, it->icon_path, NULL);
     }

   evas_object_size_hint_min_set(icon, 40, 40);
   evas_object_size_hint_max_set(icon, 100, 100);
   evas_object_show(icon);
   if(obj && part)
     elm_layout_content_set(obj, part, icon);

   return icon;
}

static Evas_Object *
create_item_layout(Evas_Object * parent, Elm_Controlbar_Item * it, Evas_Object **item, Evas_Object **label, Evas_Object **icon)
{

   Evas_Object * obj;
   obj = elm_layout_add(parent);
   if (obj == NULL)
     {
        fprintf(stderr, "Cannot load bg edj\n");
        return NULL;
     }
   elm_layout_theme_set(obj, "controlbar", "item_bg",
                        elm_widget_style_get(it->obj));
   evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   *item = elm_layout_add(parent);
   if (*item == NULL)
     {
        fprintf(stderr, "Cannot load bg edj\n");
        return NULL;
     }
   elm_layout_theme_set(*item, "controlbar", "item",
                        elm_widget_style_get(it->obj));
   elm_layout_content_set(obj, "item", *item);

   if (it->text)
     *label = create_item_label(*item, it, "elm.swallow.text");
   if (it->icon_path)
     *icon = create_item_icon(*item, it, "elm.swallow.icon");
   if (*label && *icon)
     {
        edje_object_signal_emit(_EDJ(*item), "elm,state,icon_text", "elm");
        elm_label_line_wrap_set(*label, EINA_FALSE);
        elm_label_wrap_mode_set(*label, 0);
     }

   return obj;
}

static void
bar_item_down_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   if (wd->animating) return;

   EINA_LIST_FOREACH(wd->items, l, item)
      if (item->base == obj) break;

   if (item == NULL) return;

   pressed_box(item);
}

static Elm_Controlbar_Item *
create_tab_item(Evas_Object * obj, const char *icon_path, const char *label,
                Evas_Object * view)
{
   Elm_Controlbar_Item * it;
   Widget_Data * wd;
   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return NULL;
     }
   wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return NULL;
     }
   it = ELM_NEW(Elm_Controlbar_Item);
   if (!it) return NULL;
   it->wd = wd;
   it->obj = obj;
   it->text = eina_stringshare_add(label);
   it->icon_path = eina_stringshare_add(icon_path);
   it->selected = EINA_FALSE;
   it->sel = 1;
   it->view = view;
   it->style = TABBAR;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  bar_item_down_cb, wd);
   evas_object_show(it->base);

   return it;
}

static Elm_Controlbar_Item *
create_tool_item(Evas_Object * obj, const char *icon_path, const char *label,
                 void (*func) (void *data, Evas_Object * obj,
                               void *event_info), void *data)
{

   Elm_Controlbar_Item * it;
   Widget_Data * wd;
   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return NULL;
     }
   wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return NULL;
     }
   it = ELM_NEW(Elm_Controlbar_Item);
   if (!it)
     return NULL;
   it->wd = wd;
   it->obj = obj;
   it->text = eina_stringshare_add(label);
   it->icon_path = eina_stringshare_add(icon_path);
   it->selected = EINA_FALSE;
   it->sel = 1;
   it->func = func;
   it->data = data;
   it->style = TOOLBAR;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  bar_item_down_cb, wd);
   evas_object_show(it->base);

   return it;
}

static Elm_Controlbar_Item *
create_object_item(Evas_Object * obj, Evas_Object * obj_item, const int sel)
{
   Elm_Controlbar_Item * it;
   Widget_Data * wd;
   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return NULL;
     }
   wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return NULL;
     }
   it = ELM_NEW(Elm_Controlbar_Item);
   if (!it)
     return NULL;
   it->wd = wd;
   it->obj = obj;
   it->sel = sel;
   it->style = OBJECT;
   it->base = obj_item;
   return it;
}

static void
repack_items(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if(item->order > 0)
          {
             elm_table_unpack(wd->box, item->base);
             if(!wd->vertical)
               elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
             else
               elm_table_pack(wd->box, item->base, 0, item->order - 1, item->sel, 1);
          }
     }
}

static void
set_items_position(Evas_Object * obj, Elm_Controlbar_Item * it,
                   Elm_Controlbar_Item * mit, Eina_Bool bar)
{
   Widget_Data * wd;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int i = 1;
   int check = EINA_FALSE;
   int order = 1;

   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return;
     }
   wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item == mit && item->order > 0)
          {
             check = EINA_TRUE;
             it->order = mit->order;
          }
        if (check)
          {
             if(item->order > 0){
                  elm_table_unpack(wd->box, item->base);
                  item->order += it->sel;
                  if(!wd->vertical)
                    {
                       elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
                    }
                  else
                    {
                       elm_table_pack(wd->box, item->base, 0, item->order - 1, item->sel, 1);
                    }
             }
          }
        if (item->style != OBJECT)
          i++;
        if(item->order > 0) order += item->sel;
     }
   if (!check)
     {
        if(bar)
          it->order = order;
        else
          it->order = 0;
     }
   wd->num++;

   if(bar)
     {
        if(!wd->vertical)
          {
             elm_table_pack(wd->box, it->base, it->order - 1, 0, it->sel, 1);
          }
        else
          {
             elm_table_pack(wd->box, it->base, 0, it->order - 1, it->sel, 1);
          }
     }
   else
     evas_object_hide(it->base);
}

static void
list_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Controlbar_Item *item = (Elm_Controlbar_Item *)data;
   Elm_Controlbar_Item *it;
   const Eina_List *l;
   Widget_Data *wd;
   Evas_Object *content;
   Elm_List_Item *lit = (Elm_List_Item *) elm_list_selected_item_get(obj);
   if(!lit) return;

   elm_list_item_selected_set(lit, 0);

   if(!item) return;

   wd = elm_widget_data_get(item->obj);

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        it->selected = EINA_FALSE;
     }

   if(item->style == TABBAR)
     {
        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        evas_object_hide(content);
        item->selected = EINA_TRUE;
        evas_object_smart_callback_call(item->obj, "view,change,before", item);
        elm_layout_content_set(wd->view, "elm.swallow.view", item->view);
     }

   if(item->style == TOOLBAR && item->func)
     item->func(item->data, item->obj, item);
}

static Evas_Object *
create_more_view(Widget_Data *wd)
{
   Evas_Object *list;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Object *icon;

   list = elm_list_add( wd->object );
   elm_list_mode_set( list, ELM_LIST_COMPRESS );

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if(item->order <= 0)
          {
             icon = NULL;
             if(item->icon_path)
               {
                  icon = create_item_icon(list, item, NULL);
                  evas_object_color_set(icon, 0, 0, 0, 255);
               }
             elm_list_item_append(list, item->text, icon, NULL, list_clicked, item);
          }
     }

   elm_list_go( list );

   return list;
}

static void _ctxpopup_cb(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Controlbar_Item *it;
   const Eina_List *l;
   Evas_Object *ctxpopup = obj;
   Widget_Data *wd = (Widget_Data *)data;

   EINA_LIST_FOREACH(wd->items, l, it)
      if(!strcmp(it->text, elm_ctxpopup_item_label_get((Elm_Ctxpopup_Item *) event_info))) break;

   if(it->func)
     it->func(it->data, it->obj, it);

   if(item_exist_check(wd, it)) evas_object_smart_callback_call(it->obj, "clicked", it);

   evas_object_del(ctxpopup);
   ctxpopup = NULL;
}

static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *ctxpopup = obj;

   evas_object_del(ctxpopup);
   ctxpopup = NULL;
}

static void
create_more_func(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *ctxpopup;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Object *icon;
   Evas_Coord x, y, w, h;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   ctxpopup = elm_ctxpopup_add(wd->parent);
   evas_object_smart_callback_add( ctxpopup, "dismissed", _ctxpopup_dismissed_cb, wd);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if(item->order <= 0)
          {
             icon = NULL;
             if(item->icon_path)
               {
                  icon = create_item_icon(ctxpopup, item, NULL);
                  evas_object_color_set(icon, 0, 0, 0, 255);
               }
             elm_ctxpopup_item_append(ctxpopup, item->text, icon, _ctxpopup_cb, wd);
          }
     }

   evas_object_geometry_get(wd->more_item->base, &x, &y, &w, &h);
   evas_object_move(ctxpopup, x + w/2, y + h/2);

   evas_object_show(ctxpopup);
}

static Elm_Controlbar_Item *
create_more_item(Widget_Data *wd, int style)
{
   Elm_Controlbar_Item * it;

   it = ELM_NEW(Elm_Controlbar_Item);
   if (!it) return NULL;
   it->obj = wd->object;
   it->text = eina_stringshare_add("more");
   it->icon_path = eina_stringshare_add(CONTROLBAR_SYSTEM_ICON_MORE);
   it->selected = EINA_FALSE;
   it->sel = 1;
   it->view = create_more_view(wd);
   it->func = create_more_func;
   it->style = style;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  bar_item_down_cb, wd);
   evas_object_show(it->base);

   set_items_position(it->obj, it, NULL, EINA_TRUE);
   wd->items = eina_list_append(wd->items, it);
   wd->more_item = it;
   wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), sort_cb);

   return it;
}

///////////////////////////////////////////////////////////////////
//
//  API function
//
////////////////////////////////////////////////////////////////////

/**
 * Add a new controlbar object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object * elm_controlbar_add(Evas_Object * parent)
{
   if (parent == NULL) return NULL;
   Evas_Object * obj = NULL;
   Evas_Object * bg = NULL;
   Widget_Data * wd = NULL;
   Evas_Coord x, y, w, h;
   wd = ELM_NEW(Widget_Data);
   Evas *evas = evas_object_evas_get(parent);
   if (evas == NULL) return NULL;
   obj = elm_widget_add(evas);
   if (obj == NULL) return NULL;
   ELM_SET_WIDTYPE(widtype, "controlbar");
   elm_widget_type_set(obj, "controlbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);

   // initialization
   wd->parent = parent;
   evas_object_geometry_get(parent, &x, &y, &w, &h);
   wd->object = obj;
   wd->x = x;
   wd->y = y;
   wd->w = w;
   wd->h = h;
   wd->mode = ELM_CONTROLBAR_MODE_DEFAULT;
   wd->alpha = 100;
   wd->num = 0;
   wd->animating = 0;
   wd->vertical = EINA_FALSE;
   wd->auto_align = EINA_FALSE;
   wd->init_animation = EINA_FALSE;
   wd->selected_animation = EINA_FALSE;
   wd->pressed_signal = eina_stringshare_add("elm,state,pressed");
   wd->selected_signal = eina_stringshare_add("elm,state,selected");
   wd->view = elm_layout_add(wd->parent);
   elm_layout_theme_set(wd->view, "controlbar", "view", "default");
   if (wd->view == NULL)
     {
        printf("Cannot load bg edj\n");
        return NULL;
     }
   evas_object_show(wd->view);

   /* load background edj */
   wd->edje = elm_layout_add(obj);
   elm_layout_theme_set(wd->edje, "controlbar", "base", "default");
   if (wd->edje == NULL)
     {
        printf("Cannot load base edj\n");
        return NULL;
     }
   evas_object_show(wd->edje);

   wd->bg = elm_layout_add(wd->edje);
   elm_layout_theme_set(wd->bg, "controlbar", "background", "default");
   if (wd->bg == NULL)
     {
        printf("Cannot load bg edj\n");
        return NULL;
     }
   elm_layout_content_set(wd->edje, "bg_image", wd->bg);

   // initialization
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE,
                                  _controlbar_object_resize, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOVE,
                                  _controlbar_object_move, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_SHOW,
                                  _controlbar_object_show, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_HIDE,
                                  _controlbar_object_hide, obj);
   bg = elm_layout_content_get(wd->edje, "bg_image");
   evas_object_event_callback_add(bg, EVAS_CALLBACK_MOVE, _controlbar_object_move, obj);
   evas_object_event_callback_add(bg, EVAS_CALLBACK_RESIZE, _controlbar_object_resize, obj);

   wd->selected_box = wd->focused_box = elm_layout_add(wd->bg);
   elm_layout_theme_set(wd->focused_box, "controlbar", "item_bg_move", "default");
   evas_object_hide(wd->focused_box);

   wd->focused_box_left = elm_layout_add(wd->bg);
   elm_layout_theme_set(wd->focused_box_left, "controlbar", "item_bg_move_left", "default");
   evas_object_hide(wd->focused_box_left);

   wd->focused_box_right = elm_layout_add(wd->bg);
   elm_layout_theme_set(wd->focused_box_right, "controlbar", "item_bg_move_right", "default");
   evas_object_hide(wd->focused_box_right);

   // items container
   wd->box = elm_table_add(wd->edje);
   elm_table_homogenous_set(wd->box, EINA_TRUE);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_content_set(wd->edje, "elm.swallow.items", wd->box);
   evas_object_show(wd->box);

   wd->event_box = evas_object_rectangle_add(evas);
   evas_object_color_set(wd->event_box, 255, 255, 255, 0);
   evas_object_repeat_events_set(wd->event_box, EINA_TRUE);
   evas_object_show(wd->event_box);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   evas_object_smart_member_add(wd->view, obj);
   elm_widget_resize_object_set(obj, wd->edje);
   evas_object_smart_member_add(wd->focused_box, obj);
   evas_object_smart_member_add(wd->focused_box_left, obj);
   evas_object_smart_member_add(wd->focused_box_right, obj);
   evas_object_smart_member_add(wd->box, obj);
   evas_object_smart_member_add(wd->event_box, obj);

   _sizing_eval(obj);

   return obj;
}

/**
 * Append new tab item
 *
 * @param	obj The controlbar object
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	view The view of item
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_tab_item_append(Evas_Object * obj,
                                                          const char
                                                          *icon_path,
                                                          const char *label,
                                                          Evas_Object *
                                                          view)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Widget_Data * wd;
   it = create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TABBAR);
        }
        set_items_position(obj, it, NULL, EINA_FALSE);
   }
   else{
        set_items_position(obj, it, NULL, EINA_TRUE);
   }
   if(wd->init_animation) evas_object_hide(it->base);
   wd->items = eina_list_append(wd->items, it);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));

   check_background(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Prepend new tab item
 *
 * @param	obj The controlbar object
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	view The view of item
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_tab_item_prepend(Evas_Object *
                                                           obj,
                                                           const char
                                                           *icon_path,
                                                           const char
                                                           *label,
                                                           Evas_Object *
                                                           view)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   it = create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TABBAR);
        }
        lit = elm_controlbar_item_prev(wd->more_item);
        item_visible_set(lit, EINA_FALSE);
        set_items_position(obj, it, item, EINA_TRUE);
   }
   else{
        set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_prepend(wd->items, it);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));

   check_background(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new tab item before given item
 *
 * @param	obj The controlbar object
 * @param	before The given item
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	view The view of item
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_tab_item_insert_before(Evas_Object * obj,
                                      Elm_Controlbar_Item * before,
                                      const char *icon_path,
                                      const char *label, Evas_Object * view)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   if (!before) return NULL;
   it = create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TABBAR);
          }
        before = wd->more_item;
        if(before->order > 0)
          {
             lit = elm_controlbar_item_prev(wd->more_item);
             item_visible_set(lit, EINA_FALSE);
             set_items_position(obj, it, before, EINA_TRUE);
          }
        else
          {
             set_items_position(obj, it, before, EINA_FALSE);
          }
   }
   else{
        set_items_position(obj, it, before, EINA_TRUE);
   }
   wd->items = eina_list_prepend_relative(wd->items, it, before);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));

   check_background(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new tab item after given item
 *
 * @param	obj The controlbar object
 * @param	after The given item
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	view The view of item
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_tab_item_insert_after(Evas_Object * obj,
                                     Elm_Controlbar_Item * after,
                                     const char *icon_path, const char *label,
                                     Evas_Object * view)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   if (!after) return NULL;
   it = create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TABBAR);
          }
        lit = elm_controlbar_item_prev(wd->more_item);
        if(lit != after && item->order > 0)
          {
             item_visible_set(lit, EINA_FALSE);
             set_items_position(obj, it, item, EINA_TRUE);
          }
        else
          {
             set_items_position(obj, it, NULL, EINA_FALSE);
          }
   }
   else{
        set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_append_relative(wd->items, it, after);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));

   check_background(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Append new tool item
 *
 * @param	obj The controlbar object
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	func Callback function of item
 * @param	data The data of callback function
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_tool_item_append(Evas_Object *
                                                           obj,
                                                           const char
                                                           *icon_path,
                                                           const char
                                                           *label,
                                                           void (*func)
                                                           (void *data,
                                                            Evas_Object *
                                                            obj,
                                                            void
                                                            *event_info),
                                                           void *data)

{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Widget_Data * wd;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TOOLBAR);
        }
        set_items_position(obj, it, NULL, EINA_FALSE);
   }
   else{
        set_items_position(obj, it, NULL, EINA_TRUE);
   }
   wd->items = eina_list_append(wd->items, it);
   check_toolbar_line(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Prepend new tool item
 *
 * @param	obj The controlbar object
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	func Callback function of item
 * @param	data The data of callback function
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_tool_item_prepend(Evas_Object *
                                                            obj,
                                                            const char
                                                            *icon_path,
                                                            const char
                                                            *label,
                                                            void (*func)
                                                            (void
                                                             *data,
                                                             Evas_Object *
                                                             obj,
                                                             void
                                                             *event_info),
                                                            void
                                                            *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TOOLBAR);
        }
        lit = elm_controlbar_item_prev(wd->more_item);
        item_visible_set(lit, EINA_FALSE);
        set_items_position(obj, it, item, EINA_TRUE);
   }
   else{
        set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_prepend(wd->items, it);
   check_toolbar_line(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new tool item before given item
 *
 * @param	obj The controlbar object
 * @param	before The given item	
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	func Callback function of item
 * @param	data The data of callback function
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_tool_item_insert_before(Evas_Object * obj,
                                       Elm_Controlbar_Item * before,
                                       const char *icon_path,
                                       const char *label,
                                       void (*func) (void *data,
                                                     Evas_Object * obj,
                                                     void *event_info),
                                       void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   if (!before) return NULL;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TOOLBAR);
          }
        before = wd->more_item;
        if(before->order > 0)
          {
             lit = elm_controlbar_item_prev(wd->more_item);
             item_visible_set(lit, EINA_FALSE);
             set_items_position(obj, it, before, EINA_TRUE);
          }
        else
          {
             set_items_position(obj, it, before, EINA_FALSE);
          }
   }
   else{
        set_items_position(obj, it, before, EINA_TRUE);
   }
   wd->items = eina_list_prepend_relative(wd->items, it, before);
   check_toolbar_line(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new tool item after given item
 *
 * @param	obj The controlbar object
 * @param	after The given item	
 * @param	icon_path The icon path of item
 * @param	label The label of item
 * @param	func Callback function of item
 * @param	data The data of callback function
 * @return	The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_tool_item_insert_after(Evas_Object * obj,
                                      Elm_Controlbar_Item * after,
                                      const char *icon_path,
                                      const char *label,
                                      void (*func) (void *data,
                                                    Evas_Object * obj,
                                                    void *event_info),
                                      void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   if (!after) return NULL;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
        if(!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             item_visible_set(lit, EINA_FALSE);
             create_more_item(wd, TOOLBAR);
          }
        lit = elm_controlbar_item_prev(wd->more_item);
        if(lit != after && item->order > 0)
          {
             item_visible_set(lit, EINA_FALSE);
             set_items_position(obj, it, item, EINA_TRUE);
          }
        else
          {
             set_items_position(obj, it, NULL, EINA_FALSE);
          }
   }
   else{
        set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_append_relative(wd->items, it, after);
   check_toolbar_line(wd);
   _sizing_eval(obj);
   return it;
}

/**
 * Append new object item
 *
 * @param	obj The controlbar object
 * @param	obj_item The object of item
 * @param	sel The number of sel occupied
 * @return  The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_object_item_append(Evas_Object *
                                                             obj,
                                                             Evas_Object *
                                                             obj_item,
                                                             const int sel)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   it = create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   set_items_position(obj, it, NULL, EINA_TRUE);
   wd->items = eina_list_append(wd->items, it);
   _sizing_eval(obj);
   return it;
}

/**
 * Prepend new object item
 *
 * @param	obj The controlbar object
 * @param	obj_item The object of item
 * @param	sel The number of sel occupied 
 * @return  The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_object_item_prepend(Evas_Object *
                                                              obj,
                                                              Evas_Object *
                                                              obj_item,
                                                              const int sel)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   it = create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   set_items_position(obj, it, item, EINA_TRUE);
   wd->items = eina_list_prepend(wd->items, it);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new object item before given item
 *
 * @param	obj The controlbar object
 * @param	before The given item	
 * @param	obj_item The object of item
 * @param	sel The number of sel occupied 
 * @return  The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_object_item_insert_before(Evas_Object * obj,
                                         Elm_Controlbar_Item * before,
                                         Evas_Object * obj_item, const int sel)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   if (!before) return NULL;
   it = create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   set_items_position(obj, it, before, EINA_TRUE);
   wd->items = eina_list_prepend_relative(wd->items, it, before);
   _sizing_eval(obj);
   return it;
}

/**
 * Insert new object item after given item
 *
 * @param	obj The controlbar object
 * @param	after The given item	
 * @param	obj_item The object of item
 * @param	sel The number of sel occupied 
 * @return  The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item *
elm_controlbar_object_item_insert_after(Evas_Object * obj,
                                        Elm_Controlbar_Item * after,
                                        Evas_Object * obj_item, const int sel)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   if (!after) return NULL;
   it = create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   set_items_position(obj, it, item, EINA_TRUE);
   wd->items = eina_list_append_relative(wd->items, it, after);
   _sizing_eval(obj);
   return it;
}

/**
 * Get the object of the object item
 *
 * @param       it The item of controlbar
 * @return      The object of the object item
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object *
elm_controlbar_object_item_object_get(Elm_Controlbar_Item * it)
{
   if (!it) return NULL;
   if (it->style != OBJECT) return NULL;
   if (!it->base) return NULL;
   return it->base;
}

/**
 * Delete item from controlbar
 *
 * @param	it The item of controlbar

 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_del(Elm_Controlbar_Item * it)
{
   if (!it) return;
   Evas_Object * obj;
   Widget_Data * wd;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   int check = 0;

   //   int i = 1;

   int sel = 1;

   if (!it) return;

   obj = it->obj;
   if (it->obj == NULL)
     {
        printf("Invalid argument: controlbar object is NULL\n");
        return;
     }
   wd = elm_widget_data_get(it->obj);
   if (wd == NULL)
     {
        printf("Cannot get smart data\n");
        return;
     }

   // unpack base item
   if (it->order > 0)
     {
        if (it->base)
          elm_table_unpack(wd->box, it->base);
        sel = it->sel;
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (it != item)
               {
                  if (item->order > it->order)
                    {
                       if (item->base)
                         elm_table_unpack(wd->box, item->base);
                       item->order -= sel;
                       if(!wd->vertical)
                         elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
                       else
                         elm_table_pack(wd->box, item->base, 0, item->order - 1, item->sel, 1);
                    }
               }
             if (it == item)
               check = 1;
          }
     }

   // delete item in list
   _item_del(it);
   wd->items = eina_list_remove(wd->items, it);
   free(it);
   it = NULL;
   wd->num = wd->num - 1;
   _sizing_eval(obj);
}

/**
 * Select item in controlbar
 *
 * @param	it The item of controlbar

 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_select(Elm_Controlbar_Item * it)
{
   if (!it) return;
   if (it->obj == NULL) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (wd == NULL) return;

   selected_box(it);
}

/**
 * Set the icon of item
 *
 * @param	it The item of controlbar
 * @param	icon_path The icon path of the item
 * @return	The icon object
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_icon_set(Elm_Controlbar_Item * it, const char *icon_path)
{
   if (!it) return;
   if(it->icon_path)
     {
        eina_stringshare_del(it->icon_path);
        it->icon_path = NULL;
     }
   if(it->icon)
     {
        evas_object_del(it->icon);
        it->icon = NULL;
     }

   if(icon_path != NULL)
     {
        it->icon_path = eina_stringshare_add(icon_path);
        it->icon = create_item_icon(it->base_item, it, "elm.swallow.icon");
     }

   if(it->label && it->icon)
     {
        edje_object_signal_emit(_EDJ(it->base_item), "elm,state,icon_text", "elm");
        elm_label_line_wrap_set(it->label, EINA_FALSE);
        elm_label_wrap_mode_set(it->label, 0);
     }
   else if(!it->icon)
     edje_object_signal_emit(_EDJ(it->base_item), "elm,state,default", "elm");

   if(it->wd->disabled || it->disable)
     item_color_set(it, "elm.item.disable.color");
   else
     item_color_set(it, "elm.item.default.color");
}

/**
 * Get the icon of item
 *
 * @param	it The item of controlbar
 * @return	The icon object
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object *
elm_controlbar_item_icon_get(Elm_Controlbar_Item * it)
{
   if (!it) return NULL;
   return it->icon;
}

/**
 * Set the label of item
 *
 * @param	it The item of controlbar
 * @param	label The label of item
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_label_set(Elm_Controlbar_Item * it, const char *label)
{
   if (!it) return;
   if(it->text)
     {
        eina_stringshare_del(it->text);
        it->text = NULL;
     }
   if(it->label)
     {
        evas_object_del(it->label);
        it->label = NULL;
     }
   if(label != NULL)
     {
        it->text = eina_stringshare_add(label);
        it->label = create_item_label(it->base_item, it, "elm.swallow.text");
     }
   if(it->wd->disabled || it->disable) item_color_set(it, "elm.item.disable.color");

   if(it->label && it->icon)
     {
        edje_object_signal_emit(_EDJ(it->base_item), "elm,state,icon_text", "elm");
        elm_label_line_wrap_set(it->label, EINA_FALSE);
        elm_label_wrap_mode_set(it->label, 0);
     }
   else if(!it->label)
     edje_object_signal_emit(_EDJ(it->base_item), "elm,state,default", "elm");
}

/**
 * Get the label of item
 *
 * @param	it The item of controlbar
 * @return The label of item
 *
 * @ingroup Controlbar
 */
EAPI const char *
elm_controlbar_item_label_get(Elm_Controlbar_Item * it)
{
   if (!it) return NULL;
   return it->text;
}

/**
 * Get the selected item
 *
 * @param	obj The controlbar object
 * @return		The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_selected_item_get(Evas_Object *
                                                            obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (obj == NULL) return NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->selected) return item;
     }
   return NULL;
}

/**
 * Get the first item
 *
 * @param	obj The controlbar object
 * @return		The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_first_item_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(wd->items);
}

/**
 * Get the last item
 *
 * @param	obj The controlbar object
 * @return		The item of controlbar
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_last_item_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the items
 *
 * @param	obj The controlbar object
 * @return	The list of the items
 *
 * @ingroup Controlbar
 */
EAPI Eina_List * elm_controlbar_items_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return wd->items;
}

/**
 * Get the previous item
 *
 * @param	it The item of controlbar
 * @return	The previous item of the parameter item
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_item_prev(Elm_Controlbar_Item *
                                                    it)
{
   if (!it) return NULL;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (it->obj == NULL) return NULL;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items) return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (it == item)
          {
             l = eina_list_prev(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

/**
 * Get the next item
 *
 * @param	obj The controlbar object
 * @return	The next item of the parameter item
 *
 * @ingroup Controlbar
 */
EAPI Elm_Controlbar_Item * elm_controlbar_item_next(Elm_Controlbar_Item *
                                                    it)
{
   if (!it) return NULL;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (it->obj == NULL) return NULL;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items) return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (it == item)
          {
             l = eina_list_next(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

/**
 * Set the visible status of item in bar
 *
 * @param	it The item of controlbar
 * @param	bar EINA_TRUE or EINA_FALSE
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_visible_set(Elm_Controlbar_Item * it, Eina_Bool visible)
{
   if (!it) return;
   if (it->obj == NULL) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return;

   if(!wd->auto_align)
     item_visible_set(it, visible);
}

/**
 * Get the result which or not item is visible in bar
 *
 * @param	it The item of controlbar
 * @return	EINA_TRUE or EINA_FALSE
 *
 * @ingroup Controlbar
 */
EAPI Eina_Bool
elm_controlbar_item_visible_get(Elm_Controlbar_Item * it)
{
   if (!it) return EINA_FALSE;

   if (it->obj == NULL) return EINA_FALSE;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return EINA_FALSE;
   if(it->order <= 0) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * Set item disable
 *
 * @param	it The item of controlbar
 * @param	bar EINA_TRUE or EINA_FALSE
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_disable_set(Elm_Controlbar_Item * it, Eina_Bool disable)
{
   if (!it) return;

   if (it->disable == disable) return;

   it->disable = disable;

   if (it->wd && it->wd->disabled) return;

   if (it->style == OBJECT)
     {
        if (it->base) elm_widget_disabled_set(it->base, it->disable);
     }
   else
     {
        if (it->disable)
          item_color_set(it, "elm.item.disable.color");
        else
          item_color_set(it, "elm.item.default.color");
     }
}

/**
 * Get item disable
 *
 * @param	it The item of controlbar
 * @return 	EINA_TRUE or EINA_FALSE
 *
 * @ingroup Controlbar
 */
EAPI Eina_Bool
elm_controlbar_item_disable_get(Elm_Controlbar_Item * it)
{
   if (!it) return EINA_FALSE;

   return it->disable;
}

/**
 * Set the view of the item
 *
 * @param	it The item of controlbar
 * @param	view The view for the item
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_view_set(Elm_Controlbar_Item *it, Evas_Object * view)
{
   if(!it) return;

   it->view = view;
}

/**
 * Get the view of the item
 *
 * @param	it The item of controlbar
 * @return	The view for the item
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object *
elm_controlbar_item_view_get(Elm_Controlbar_Item *it)
{
   if(!it) return NULL;

   return it->view;
}

/**
 * Set the mode of the controlbar
 *
 * @param	obj The object of the controlbar
 * @param	mode The mode of the controlbar
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_mode_set(Evas_Object *obj, int mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if(wd->mode == mode) return;

   wd->mode = mode;

   switch(wd->mode)
     {
      case ELM_CONTROLBAR_MODE_DEFAULT:
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,default", "elm");
         break;
      case ELM_CONTROLBAR_MODE_TRANSLUCENCE:
         elm_controlbar_alpha_set(obj, 85);
         break;
      case ELM_CONTROLBAR_MODE_TRANSPARENCY:
         elm_controlbar_alpha_set(obj, 0);
         break;
      case ELM_CONTROLBAR_MODE_LARGE:
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,large", "elm");
         break;
      case ELM_CONTROLBAR_MODE_SMALL:
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,small", "elm");
         break;
      case ELM_CONTROLBAR_MODE_LEFT:
         wd->selected_box = wd->focused_box_left;
         wd->selected_signal = eina_stringshare_add("elm,state,selected_left");
         wd->pressed_signal = eina_stringshare_add("elm,state,pressed_left");
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,left", "elm");
         check_background(wd);
         _sizing_eval(obj);
         return;
      case ELM_CONTROLBAR_MODE_RIGHT:
         wd->selected_box = wd->focused_box_right;
         wd->selected_signal = eina_stringshare_add("elm,state,selected_right");
         wd->pressed_signal = eina_stringshare_add("elm,state,pressed_right");
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,right", "elm");
         check_background(wd);
         _sizing_eval(obj);
         return;
      default:
         break;
     }

   wd->selected_box = wd->focused_box;
   wd->selected_signal = eina_stringshare_add("elm,state,selected");
   wd->pressed_signal = eina_stringshare_add("elm,state,pressed");
   check_background(wd);
   _sizing_eval(obj);
}

/**
 * Set the alpha of the controlbar
 *
 * @param	obj The object of the controlbar
 * @param	alpha The alpha value of the controlbar (0-100)
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_alpha_set(Evas_Object *obj, int alpha)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   int r, g, b;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if(alpha < 0) wd->alpha = 0;
   else if(alpha > 100) wd->alpha = 100;
   else wd->alpha = alpha;

   evas_object_color_get(wd->bg, &r, &g, &b, NULL);
   evas_object_color_set(wd->bg, r, g, b, (int)(255 * wd->alpha / 100));
}


/**
 * Set auto-align mode of the controlbar(It's not prepared yet)
 * If you set the auto-align and add items more than 5, 
 * the "more" item will be made and the items more than 5 will be unvisible.
 *
 * @param	obj The object of the controlbar
 * @param	auto_align The dicision that the controlbar use the auto-align
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_item_auto_align_set(Evas_Object *obj, Eina_Bool auto_align)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   int i;
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if(wd->auto_align == auto_align) return;

   if(auto_align)
     {
        if(check_bar_item_number(wd) >= 5 && !wd->more_item)
          {
             i = 0;
             EINA_LIST_FOREACH(wd->items, l, item)
               {
                  if(elm_controlbar_item_visible_get(item))
                    i++;
                  if(i >= 5){
                       item_delete_in_bar(item);
                  }
               }
             item = elm_controlbar_last_item_get(obj);
             while(!elm_controlbar_item_visible_get(item)){
                  item = elm_controlbar_item_prev(item);
             }
             create_more_item(wd, item->style);
          }
     }
   else
     {
        if(wd->more_item)
          {
             // delete more item
             if(wd->more_item->view)
               evas_object_del(wd->more_item->view);
             wd->items = eina_list_remove(wd->items, wd->more_item);
             eina_stringshare_del(wd->more_item->text);
             if (wd->more_item->icon)
               evas_object_del(wd->more_item->icon);
             if (wd->more_item->base)
               evas_object_del(wd->more_item->base);
             if (wd->more_item->base_item)
               evas_object_del(wd->more_item->base_item);
             free(wd->more_item);
             wd->more_item = NULL;

             // make all item is visible
             i = 1;
             EINA_LIST_FOREACH(wd->items, l, item)
               {
                  if(!elm_controlbar_item_visible_get(item))
                    item_insert_in_bar(item, i);
                  i++;
               }
          }
     }
   wd->auto_align = auto_align;
   _sizing_eval(obj);
}

/**
 * Set the vertical mode of the controlbar
 *
 * @param	obj The object of the controlbar
 * @param	vertical The vertical mode of the controlbar (TRUE = vertical, FALSE = horizontal)
 *
 * @ingroup Controlbar
 */
EAPI void
elm_controlbar_vertical_set(Evas_Object *obj, Eina_Bool vertical)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if(wd->vertical == vertical) return;
   wd->vertical = vertical;

   if(check_bar_item_number(wd) > 1)
     {
        repack_items(wd);
     }
   check_toolbar_line(wd);
}

static Eina_Bool
init_animation(void *data)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = (Widget_Data *)data;

   wd->visible_items = eina_list_free(wd->visible_items);
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if(item->order > 0)
          {
             wd->visible_items = eina_list_append(wd->visible_items, item->base);
          }
     }

   if(wd->ani_func)
     wd->ani_func(wd->ani_data, wd->object, wd->visible_items);

   return ECORE_CALLBACK_CANCEL;
}

EAPI void
elm_controlbar_animation_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, void *event_info), void *data)
{
   printf("\n==================================\n");
   printf("%s\n", __func__);
   printf("==================================\n");
   printf("This API is just for test.\n");
   printf("Please don't use it!!\n");
   printf("Thank you.\n");
   printf("==================================\n");

   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   //   if(!func)
   //   {
   wd->init_animation = EINA_TRUE;

   wd->ani_func = func;
   wd->ani_data = data;

   ecore_idler_add(init_animation, wd);
   // }
}

EAPI void
elm_controlbar_item_animation_set(Evas_Object *obj, Eina_Bool auto_animation, Eina_Bool selected_animation)
{
   printf("\n==================================\n");
   printf("%s\n", __func__);
   printf("==================================\n");
   printf("This API is just for test.\n");
   printf("Please don't use it!!\n");
   printf("Thank you.\n");
   printf("==================================\n");

   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd == NULL)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if(auto_animation && !wd->effect_timer)
     {
        wd->effect_timer = ecore_timer_add(1.5, item_animation_effect, wd);
     }
   else
     {
        if(wd->effect_timer) ecore_timer_del(wd->effect_timer);
        wd->effect_timer = NULL;
     }

   wd->selected_animation = selected_animation;
}
