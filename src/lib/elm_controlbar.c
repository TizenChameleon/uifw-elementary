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
   double start_time;
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
   Evas_Object * selected_box;

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
   Evas_Object * icon;
   const char *icon_path;
   const char *text;
   void (*func) (void *data, Evas_Object * obj, void *event_info);
   void *data;
   int order;
   int sel;
   int style;
   Eina_Bool selected;
   Eina_Bool disabled;
};

static const char *widtype = NULL;
// prototype
static int _check_bar_item_number(Widget_Data *wd);
static void _select_box(Elm_Controlbar_Item * it);
static void _cancel_selected_box(Widget_Data *wd);
static Eina_Bool _press_box(Elm_Controlbar_Item * it);

///////////////////////////////////////////////////////////////////
//
//  Smart Object basic function
//
////////////////////////////////////////////////////////////////////

static void
_controlbar_move(void *data, Evas_Object * obj __UNUSED__)
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
}

static void
_controlbar_resize(void *data, Evas_Object * obj __UNUSED__)
{
   Widget_Data * wd;
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
}

static void
_controlbar_object_move(void *data, Evas * e __UNUSED__, Evas_Object * obj,
                        void *event_info __UNUSED__)
{
   _controlbar_move(data, obj);
}

static void
_controlbar_object_resize(void *data, Evas * e __UNUSED__, Evas_Object * obj,
                          void *event_info __UNUSED__)
{
   _controlbar_resize(data, obj);
}

static void
_controlbar_object_show(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   Widget_Data * wd;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_show(wd->view);
   evas_object_show(wd->edje);
   evas_object_show(wd->box);
}

static void
_controlbar_object_hide(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   Widget_Data * wd;
   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   evas_object_hide(wd->view);
   evas_object_hide(wd->edje);
   evas_object_hide(wd->box);

   _cancel_selected_box(wd);
}

static void
_item_del(Elm_Controlbar_Item *it)
{
   if (!it) return;
   Widget_Data *wd = elm_widget_data_get(it->obj);
   if (!wd) return;

   if (it->text)
     eina_stringshare_del(it->text);
   if (it->icon_path)
     eina_stringshare_del(it->icon_path);
   if (it->icon)
     evas_object_del(it->icon);
   if (it->base)
     evas_object_del(it->base);
   if (it->base_item)
     evas_object_del(it->base_item);
   if (it->view)
     evas_object_del(it->view);
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
   if (wd->selected_box)
     {
        evas_object_del(wd->selected_box);
        wd->selected_box = NULL;
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
   if (wd->view)
     {
        evas_object_del(wd->view);
        wd->view = NULL;
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
   elm_layout_theme_set(wd->selected_box, "controlbar", "item_bg_move", elm_widget_style_get(obj));
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        elm_layout_theme_set(item->base, "controlbar", "item_bg", elm_widget_style_get(obj));
        if (item->selected)
          _select_box(item);
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
          disabled = item->disabled;

        if (item->base_item) elm_widget_disabled_set(item->base_item, disabled);
     }
}

static void
_sub_del(void *data __UNUSED__, Evas_Object * obj, void *event_info)
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

static Eina_Bool
_move_evas_object(void *data)
{
   Evas_Object *bg_image;
   double t, vx, vy, vw, vh;
   int dx, dy, dw, dh;
   int px, py, pw, ph;
   int ox, oy, ow, oh;
   int x, y, w, h;

   Animation_Data * ad = (Animation_Data *) data;
   bg_image = edje_object_part_object_get(_EDJ(ad->obj), "bg_image");
   if (bg_image) evas_object_geometry_get(bg_image, &ox, &oy, &ow, &oh);
   t = ELM_MAX(0.0, ecore_loop_time_get() - ad->start_time);
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

   if ((ow - pw) == 0)
     vx = 0;
   else
     vx = (double)(px - ox) / (double)(ow - pw);
   if ((oh - ph) == 0)
     vy = 0;
   else
     vy = (double)(py - oy) / (double)(oh - ph);
   vw = (double)pw / (double)ow;
   vh = (double)ph / (double)oh;

   if (x == dx && y == dy && w == dw && h == dh)
     {
        if (ad->timer) ecore_animator_del(ad->timer);
        ad->timer = NULL;
        edje_object_part_drag_size_set(_EDJ(ad->obj), "elm.dragable.box", vw, vh);
        edje_object_part_drag_value_set(_EDJ(ad->obj), "elm.dragable.box", vx, vy);
        if (ad->func != NULL)
          ad->func(ad->data, ad->obj);
        return ECORE_CALLBACK_CANCEL;
     }
   else
     {
        edje_object_part_drag_size_set(_EDJ(ad->obj), "elm.dragable.box", vw, vh);
        edje_object_part_drag_value_set(_EDJ(ad->obj), "elm.dragable.box", vx, vy);
     }
   return ECORE_CALLBACK_RENEW;
}

static Animation_Data*
_move_object_with_animation(Evas_Object * obj, Evas_Coord x, Evas_Coord y,
                           Evas_Coord w, Evas_Coord h, Evas_Coord x_,
                           Evas_Coord y_, Evas_Coord w_, Evas_Coord h_,
                           double time, Eina_Bool (*mv_func) (void *data),
                           void (*func) (void *data,
                                         Evas_Object * obj), void *data)
{
   Animation_Data * ad = (Animation_Data *) malloc(sizeof(Animation_Data));
   if (!ad) return NULL;
   ad->obj = obj;
   ad->fx = x;
   ad->fy = y;
   ad->fw = w;
   ad->fh = h;
   ad->tx = x_;
   ad->ty = y_;
   ad->tw = w_;
   ad->th = h_;
   ad->start_time = ecore_loop_time_get();
   ad->time = time;
   ad->func = func;
   ad->data = data;
   ad->timer = ecore_animator_add(mv_func, ad);

   return ad;
}

/////////////////////////////////////////////////////////////
//
// callback function
//
/////////////////////////////////////////////////////////////

static int
_sort_cb(const void *d1, const void *d2)
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
_check_item(Widget_Data *wd, Elm_Controlbar_Item *item)
{
   const Eina_List *l;
   Elm_Controlbar_Item *it;

   if (!wd) return EINA_FALSE;
   if (!wd->items) return EINA_FALSE;

   EINA_LIST_FOREACH(wd->items, l, it)
      if (it == item) return EINA_TRUE;

   return EINA_FALSE;
}

static void
_check_background(Widget_Data *wd)
{
   if (!wd) return;
   Eina_List *l;
   Elm_Controlbar_Item *it;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->style == TABBAR)
          {
             if (wd->mode == ELM_CONTROLBAR_MODE_LEFT)
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar_left", "elm");
             else if (wd->mode == ELM_CONTROLBAR_MODE_RIGHT)
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar_right", "elm");
             else
               edje_object_signal_emit(_EDJ(wd->bg), "elm,state,tabbar", "elm");
             return;
          }
     }
   edje_object_signal_emit(_EDJ(wd->bg), "elm,state,toolbar", "elm");
}

static void
_check_toolbar_line(Widget_Data *wd)
{
   if (!wd) return;
   Eina_List *l;
   Elm_Controlbar_Item *it, *it2;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        it2 = elm_controlbar_item_prev(it);
        if (!it2) continue;
        if (it->style != TOOLBAR || it2->style != TOOLBAR) continue;

        if (wd->vertical)
          {
             edje_object_signal_emit(_EDJ(it2->base), "elm,state,right_line_hide", "elm");
             edje_object_signal_emit(_EDJ(it->base), "elm,state,left_line_hide", "elm");

             if ((it->icon || it->text) && (it2->icon || it2->text))
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

             if ((it->icon || it->text) && (it2->icon || it2->text))
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
_check_bar_item_number(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int num = 0;

   EINA_LIST_FOREACH(wd->items, l, item)
      if (item->order > 0) num++;

   return num;
}

static void
_insert_item_in_bar(Elm_Controlbar_Item * it, int order)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return NULL;
   int check = 0;

   if (order == 0) return;

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
                  if (!wd->vertical)
                    elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
                  else
                    elm_table_pack(wd->box, item->base, 0, item->order - 1, 1, 1);
                  evas_object_show(item->base);
               }
          }
     }
   it->order = order;
   if (!wd->vertical)
     elm_table_pack(wd->box, it->base, it->order - 1, 0, 1, 1);
   else
     elm_table_pack(wd->box, it->base, 0, it->order - 1, 1, 1);
   evas_object_show(it->base);
}

static void
_delete_item_in_bar(Elm_Controlbar_Item * it)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return;
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
                  if (!wd->vertical)
                    elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
                  else
                    elm_table_pack(wd->box, item->base, 0, item->order - 1, 1, 1);
               }
          }
     }
}

static void
_set_item_visible(Elm_Controlbar_Item *it, Eina_Bool visible)
{
   Elm_Controlbar_Item *item;
   Eina_Bool check = EINA_TRUE;

   if (!it) return;
   if (it->obj == NULL) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items) return;
   if (it->order <= 0) check = EINA_FALSE;
   if (check == visible) return;
   if (visible)
     {
        item = elm_controlbar_last_item_get(it->obj);
        if (!item) return;
        while (!elm_controlbar_item_visible_get(item)){
             item = elm_controlbar_item_prev(item);
        }
        _insert_item_in_bar(it, item->order + 1);
     }
   else
     {
        _delete_item_in_bar(it);
     }
   wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), _sort_cb);
   _sizing_eval(it->obj);
}

static Eina_Bool
_hide_selected_box(void *data)
{
   Evas_Object *base = (Evas_Object *)data;

   edje_object_part_drag_size_set(_EDJ(base), "elm.dragable.box", 0.0, 0.0);

   return ECORE_CALLBACK_CANCEL;
}

static void
_end_selected_box(void *data, Evas_Object *obj __UNUSED__)
{
   Widget_Data * wd = (Widget_Data *)data;

   if (_check_item(wd, wd->cur_item))
     {
        edje_object_signal_emit(_EDJ(wd->cur_item->base), wd->selected_signal, "elm");
     }

   wd->animating--;
   if (wd->animating < 0)
     {
        printf("animation error\n");
        wd->animating = 0;
     }

   ecore_idler_add(_hide_selected_box, wd->edje);
}

static void
_move_selected_box(Widget_Data *wd, Elm_Controlbar_Item * fit, Elm_Controlbar_Item * tit)
{
   Evas_Coord fx, fy, fw, fh, tx, ty, tw, th;
   Evas_Object *from, *to;

   if (fit->order <= 0 && wd->auto_align)
     fit = wd->more_item;

   from = (Evas_Object *) edje_object_part_object_get(_EDJ(fit->base), "bg_img");
   evas_object_geometry_get(from, &fx, &fy, &fw, &fh);

   to = (Evas_Object *) edje_object_part_object_get(_EDJ(tit->base), "bg_img");
   evas_object_geometry_get(to, &tx, &ty, &tw, &th);

   if (_check_item(wd, wd->pre_item))
     {
        edje_object_signal_emit(_EDJ(wd->pre_item->base), "elm,state,unselected", "elm");
     }
   if (_check_item(wd, wd->cur_item))
     edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,state,unselected", "elm");

   wd->animating++;
   if (wd->ad)
     {
        if (wd->ad->timer) ecore_animator_del(wd->ad->timer);
        wd->ad->timer = NULL;
        free(wd->ad);
        wd->ad = NULL;
     }
   wd->ad = _move_object_with_animation(wd->edje, fx, fy, fw, fh, tx, ty, tw, th,
                                       0.3, _move_evas_object, _end_selected_box, wd);
}

static void
_select_box(Elm_Controlbar_Item * it)
{
   if (!it) return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return;
   const Eina_List *l;
   Elm_Controlbar_Item * item, *fit = NULL;
   Evas_Object * content;

   if (wd->animating) return;

   wd->cur_item = it;

   if (it->style == TABBAR)
     {
        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        if (content) evas_object_hide(content);

        EINA_LIST_FOREACH(wd->items, l, item){
             if (item->selected) {
                  fit = item;
                  wd->pre_item = fit;
             }
             item->selected = EINA_FALSE;
        }
        it->selected = EINA_TRUE;

        if (fit != NULL && fit != it)
          {
             _move_selected_box(wd, fit, it);
          }
        else
          {
             edje_object_signal_emit(_EDJ(it->base), wd->selected_signal, "elm");
          }

        if (fit != it)
          {
             if (wd->more_item != it)
               evas_object_smart_callback_call(it->obj, "view,change,before", it);
          }

        elm_layout_content_set(wd->view, "elm.swallow.view", it->view);
     }
   else if (it->style == TOOLBAR)
     {
        edje_object_signal_emit(_EDJ(it->base), "elm,state,text_unselected", "elm");
        if (it->func)
          it->func(it->data, it->obj, it);
     }
}

static void
_cancel_selected_box(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->style == TABBAR)
          {
             if (item->selected)
               {
                  edje_object_signal_emit(_EDJ(item->base), wd->selected_signal, "elm");
               }
             else
               {
                  edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
               }
          }
        else if (item->style == TOOLBAR)
          {
             edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
          }
     }
}

static void
_unpress_box_cb(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data * wd = (Widget_Data *) data;
   Evas_Event_Mouse_Up * ev = event_info;
   Evas_Coord x, y, w, h;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP, _unpress_box_cb);

   _cancel_selected_box(wd);

   if (_check_item(wd, wd->pre_item))
     {
        evas_object_geometry_get(wd->pre_item->base, &x, &y, &w, &h);
        if (ev->output.x > x && ev->output.x < x+w && ev->output.y > y && ev->output.y < y+h)
          {
             _select_box(wd->pre_item);
          }
     }
   return;
}

static Eina_Bool
_press_box(Elm_Controlbar_Item * it)
{
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return EINA_FALSE;
   int check = 0;
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   if (wd->animating) return EINA_FALSE;

   if (wd->disabled || it->disabled) return EINA_FALSE;

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
             evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_UP, _unpress_box_cb, (void *)wd);

             check = EINA_TRUE;
          }
     }
   if (!check)
     return EINA_FALSE;

   wd->pre_item = it;

   return EINA_TRUE;
}

static Evas_Object *
_create_item_icon(Evas_Object *obj, Elm_Controlbar_Item * it, char *part)
{

   Evas_Object *icon;
   icon = elm_icon_add(obj);
   if (!elm_icon_standard_set(icon, it->icon_path))
     {
        elm_icon_file_set(icon, it->icon_path, NULL);
     }

   evas_object_size_hint_min_set(icon, 40, 40);
   evas_object_size_hint_max_set(icon, 100, 100);
   evas_object_show(icon);
   if (obj && part)
     elm_button_icon_set(obj, icon);

   return icon;
}

static Evas_Object *
_create_item_layout(Evas_Object * parent, Elm_Controlbar_Item * it, Evas_Object **item, Evas_Object **icon)
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

   *item = elm_button_add(parent);
   if (*item == NULL) return NULL;
   elm_object_style_set(*item, "controlbar/vertical");
   elm_layout_content_set(obj, "item", *item);

   if (it->text)
     elm_button_label_set(*item, it->text);
   if (it->icon_path)
     *icon = _create_item_icon(*item, it, "elm.swallow.icon");

   return obj;
}

static void
_bar_item_down_cb(void *data, Evas * evas __UNUSED__, Evas_Object * obj, void *event_info __UNUSED__)
{
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   if (wd->animating) return;

   EINA_LIST_FOREACH(wd->items, l, item)
      if (item->base == obj) break;

   if (item == NULL) return;

   _press_box(item);
}

static Elm_Controlbar_Item *
_create_tab_item(Evas_Object * obj, const char *icon_path, const char *label,
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
   if (!wd)
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
   it->base = _create_item_layout(wd->edje, it, &(it->base_item), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  _bar_item_down_cb, wd);
   evas_object_show(it->base);

   return it;
}

static Elm_Controlbar_Item *
_create_tool_item(Evas_Object * obj, const char *icon_path, const char *label,
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
   if (!wd)
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
   it->base = _create_item_layout(wd->edje, it, &(it->base_item), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  _bar_item_down_cb, wd);
   evas_object_show(it->base);

   return it;
}

static Elm_Controlbar_Item *
_create_object_item(Evas_Object * obj, Evas_Object * obj_item, const int sel)
{
   Elm_Controlbar_Item * it;
   Widget_Data * wd;
   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return NULL;
     }
   wd = elm_widget_data_get(obj);
   if (!wd)
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
   it->base = elm_layout_add(wd->edje);
   elm_layout_theme_set(it->base, "controlbar", "item_bg",
                        elm_widget_style_get(it->obj));
   evas_object_size_hint_weight_set(it->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(it->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
   it->base_item = obj_item;
   elm_layout_content_set(it->base, "item", it->base_item);
   evas_object_show(it->base);
   return it;
}

static void
_repack_items(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->order > 0)
          {
             elm_table_unpack(wd->box, item->base);
             if (!wd->vertical)
               elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
             else
               elm_table_pack(wd->box, item->base, 0, item->order - 1, item->sel, 1);
          }
     }
}

static void
_set_items_position(Evas_Object * obj, Elm_Controlbar_Item * it,
                   Elm_Controlbar_Item * mit, Eina_Bool bar)
{
   Widget_Data * wd;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int check = EINA_FALSE;
   int order = 1;

   if (obj == NULL)
     {
        fprintf(stderr, "Invalid argument: controlbar object is NULL\n");
        return;
     }
   wd = elm_widget_data_get(obj);
   if (!wd)
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
             if (item->order > 0)
               {
                  elm_table_unpack(wd->box, item->base);
                  item->order += it->sel;
                  if (!wd->vertical)
                    elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
                  else
                    elm_table_pack(wd->box, item->base, 0, item->order - 1, item->sel, 1);
               }
          }
        if (item->order > 0) order += item->sel;
     }
   if (!check)
     {
        if (bar)
          it->order = order;
        else
          it->order = 0;
     }
   wd->num++;

   if (bar)
     {
        if (!wd->vertical)
          elm_table_pack(wd->box, it->base, it->order - 1, 0, it->sel, 1);
        else
          elm_table_pack(wd->box, it->base, 0, it->order - 1, it->sel, 1);
     }
   else
     evas_object_hide(it->base);
}

static void
_list_clicked(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Controlbar_Item *item = (Elm_Controlbar_Item *)data;
   Elm_Controlbar_Item *it;
   const Eina_List *l;
   Widget_Data *wd;
   Evas_Object *content;
   Elm_List_Item *lit = (Elm_List_Item *) elm_list_selected_item_get(obj);
   if (!lit) return;

   elm_list_item_selected_set(lit, 0);

   if (!item) return;

   wd = elm_widget_data_get(item->obj);
   if (!wd) return;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        it->selected = EINA_FALSE;
     }

   if (item->style == TABBAR)
     {
        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        evas_object_hide(content);
        item->selected = EINA_TRUE;
        evas_object_smart_callback_call(item->obj, "view,change,before", item);
        elm_layout_content_set(wd->view, "elm.swallow.view", item->view);
     }

   if (item->style == TOOLBAR && item->func)
     item->func(item->data, item->obj, item);
}

static Evas_Object *
_create_more_view(Widget_Data *wd)
{
   Evas_Object *list;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Object *icon;

   list = elm_list_add( wd->object );
   elm_list_mode_set( list, ELM_LIST_COMPRESS );

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->order <= 0)
          {
             icon = NULL;
             if (item->icon_path)
               {
                  icon = _create_item_icon(list, item, NULL);
                  evas_object_color_set(icon, 0, 0, 0, 255);
               }
             elm_list_item_append(list, item->text, icon, NULL, _list_clicked, item);
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
      if (!strcmp(it->text, elm_ctxpopup_item_label_get((Elm_Ctxpopup_Item *) event_info))) break;

   if (it->func)
     it->func(it->data, it->obj, it);

   if (_check_item(wd, it)) evas_object_smart_callback_call(it->obj, "clicked", it);

   evas_object_del(ctxpopup);
   ctxpopup = NULL;
}

static void _ctxpopup_dismissed_cb(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *ctxpopup = obj;

   evas_object_del(ctxpopup);
   ctxpopup = NULL;
}

static void
_create_more_func(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *ctxpopup;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Object *icon;
   Evas_Coord x, y, w, h;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   ctxpopup = elm_ctxpopup_add(wd->parent);
   evas_object_smart_callback_add( ctxpopup, "dismissed", _ctxpopup_dismissed_cb, wd);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->order <= 0)
          {
             icon = NULL;
             if (item->icon_path)
               {
                  icon = _create_item_icon(ctxpopup, item, NULL);
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
_create_more_item(Widget_Data *wd, int style)
{
   Elm_Controlbar_Item * it;

   it = ELM_NEW(Elm_Controlbar_Item);
   if (!it) return NULL;
   it->obj = wd->object;
   it->text = eina_stringshare_add("more");
   it->icon_path = eina_stringshare_add(CONTROLBAR_SYSTEM_ICON_MORE);
   it->selected = EINA_FALSE;
   it->sel = 1;
   it->view = _create_more_view(wd);
   it->func = _create_more_func;
   it->style = style;
   it->base = _create_item_layout(wd->edje, it, &(it->base_item), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
                                  _bar_item_down_cb, wd);
   evas_object_show(it->base);

   _set_items_position(it->obj, it, NULL, EINA_TRUE);
   wd->items = eina_list_append(wd->items, it);
   wd->more_item = it;
   wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), _sort_cb);

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
   elm_widget_can_focus_set(obj, EINA_FALSE);
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

   wd->selected_box = elm_layout_add(wd->bg);
   elm_layout_theme_set(wd->selected_box, "controlbar", "item_bg_move", "default");
   evas_object_hide(wd->selected_box);

   // items container
   wd->box = elm_table_add(wd->edje);
   elm_table_homogenous_set(wd->box, EINA_TRUE);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_content_set(wd->edje, "elm.swallow.items", wd->box);
   evas_object_show(wd->box);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   elm_widget_resize_object_set(obj, wd->edje);

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
   it = _create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TABBAR);
        }
        _set_items_position(obj, it, NULL, EINA_FALSE);
   }
   else{
        _set_items_position(obj, it, NULL, EINA_TRUE);
   }
   wd->items = eina_list_append(wd->items, it);
   if (wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, _create_more_view(wd));

   _check_background(wd);
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
   it = _create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TABBAR);
        }
        lit = elm_controlbar_item_prev(wd->more_item);
        _set_item_visible(lit, EINA_FALSE);
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   else{
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_prepend(wd->items, it);
   if (wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, _create_more_view(wd));

   _check_background(wd);
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
   it = _create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TABBAR);
          }
        before = wd->more_item;
        if (before->order > 0)
          {
             lit = elm_controlbar_item_prev(wd->more_item);
             _set_item_visible(lit, EINA_FALSE);
             _set_items_position(obj, it, before, EINA_TRUE);
          }
        else
          {
             _set_items_position(obj, it, before, EINA_FALSE);
          }
   }
   else{
        _set_items_position(obj, it, before, EINA_TRUE);
   }
   wd->items = eina_list_prepend_relative(wd->items, it, before);
   if (wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, _create_more_view(wd));

   _check_background(wd);
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
   it = _create_tab_item(obj, icon_path, label, view);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TABBAR);
          }
        lit = elm_controlbar_item_prev(wd->more_item);
        if (lit != after && item->order > 0)
          {
             _set_item_visible(lit, EINA_FALSE);
             _set_items_position(obj, it, item, EINA_TRUE);
          }
        else
          {
             _set_items_position(obj, it, NULL, EINA_FALSE);
          }
   }
   else{
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_append_relative(wd->items, it, after);
   if (wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, _create_more_view(wd));

   _check_background(wd);
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
   it = _create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TOOLBAR);
        }
        _set_items_position(obj, it, NULL, EINA_FALSE);
   }
   else{
        _set_items_position(obj, it, NULL, EINA_TRUE);
   }
   wd->items = eina_list_append(wd->items, it);
   _check_toolbar_line(wd);
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
   it = _create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item) {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TOOLBAR);
        }
        lit = elm_controlbar_item_prev(wd->more_item);
        _set_item_visible(lit, EINA_FALSE);
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   else{
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_prepend(wd->items, it);
   _check_toolbar_line(wd);
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
   it = _create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TOOLBAR);
          }
        before = wd->more_item;
        if (before->order > 0)
          {
             lit = elm_controlbar_item_prev(wd->more_item);
             _set_item_visible(lit, EINA_FALSE);
             _set_items_position(obj, it, before, EINA_TRUE);
          }
        else
          {
             _set_items_position(obj, it, before, EINA_FALSE);
          }
   }
   else{
        _set_items_position(obj, it, before, EINA_TRUE);
   }
   wd->items = eina_list_prepend_relative(wd->items, it, before);
   _check_toolbar_line(wd);
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
   it = _create_tool_item(obj, icon_path, label, func, data);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   if (_check_bar_item_number(wd) >= 5 && wd->auto_align){
        if (!wd->more_item)
          {
             lit = elm_controlbar_last_item_get(obj);
             _set_item_visible(lit, EINA_FALSE);
             _create_more_item(wd, TOOLBAR);
          }
        lit = elm_controlbar_item_prev(wd->more_item);
        if (lit != after && item->order > 0)
          {
             _set_item_visible(lit, EINA_FALSE);
             _set_items_position(obj, it, item, EINA_TRUE);
          }
        else
          {
             _set_items_position(obj, it, NULL, EINA_FALSE);
          }
   }
   else{
        _set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_append_relative(wd->items, it, after);
   _check_toolbar_line(wd);
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
   it = _create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   _set_items_position(obj, it, NULL, EINA_TRUE);
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
   it = _create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   item = eina_list_data_get(wd->items);
   _set_items_position(obj, it, item, EINA_TRUE);
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
   it = _create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   _set_items_position(obj, it, before, EINA_TRUE);
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
   it = _create_object_item(obj, obj_item, sel);
   if (!it) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   item = elm_controlbar_item_next(after);
   _set_items_position(obj, it, item, EINA_TRUE);
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
elm_controlbar_object_item_object_get(const Elm_Controlbar_Item * it)
{
   if (!it) return NULL;
   if (it->style != OBJECT) return NULL;
   if (!it->base_item) return NULL;
   return it->base_item;
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
   if (!wd)
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
                       if (!wd->vertical)
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
   if (!wd) return;

   _select_box(it);
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
   if (it->style == OBJECT) return;
   if (it->icon_path)
     {
        eina_stringshare_del(it->icon_path);
        it->icon_path = NULL;
     }
   if (it->icon)
     {
        evas_object_del(it->icon);
        it->icon = NULL;
     }
   if (icon_path != NULL)
     {
        it->icon_path = eina_stringshare_add(icon_path);
        it->icon = _create_item_icon(it->base_item, it, "elm.swallow.icon");
     }
   if (it->wd->disabled || it->disabled)
     elm_widget_disabled_set(it->base_item, EINA_TRUE);
   else
     elm_widget_disabled_set(it->base_item, EINA_FALSE);
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
elm_controlbar_item_icon_get(const Elm_Controlbar_Item * it)
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
   if (it->style == OBJECT) return;
   if (it->text)
     {
        eina_stringshare_del(it->text);
        it->text = NULL;
     }
   if (label != NULL)
     {
        it->text = eina_stringshare_add(label);
        elm_button_label_set(it->base_item, it->text);
     }
   if (it->wd->disabled || it->disabled)
     elm_widget_disabled_set(it->base_item, EINA_TRUE);
   else
     elm_widget_disabled_set(it->base_item, EINA_FALSE);
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
elm_controlbar_item_label_get(const Elm_Controlbar_Item * it)
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
EAPI Elm_Controlbar_Item * elm_controlbar_selected_item_get(const Evas_Object *
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
EAPI Elm_Controlbar_Item * elm_controlbar_first_item_get(const Evas_Object * obj)
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
EAPI Elm_Controlbar_Item * elm_controlbar_last_item_get(const Evas_Object * obj)
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
EAPI const Eina_List * elm_controlbar_items_get(const Evas_Object * obj)
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

   if (!wd->auto_align)
     _set_item_visible(it, visible);
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
elm_controlbar_item_visible_get(const Elm_Controlbar_Item * it)
{
   if (!it) return EINA_FALSE;
   if (it->obj == NULL) return EINA_FALSE;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return EINA_FALSE;
   if (it->order <= 0) return EINA_FALSE;

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
elm_controlbar_item_disabled_set(Elm_Controlbar_Item * it, Eina_Bool disabled)
{
   if (!it) return;

   if (it->disabled == disabled) return;

   it->disabled = disabled;

   if (it->wd && it->wd->disabled) return;

   if (it->base_item) elm_widget_disabled_set(it->base_item, disabled);
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
elm_controlbar_item_disabled_get(const Elm_Controlbar_Item * it)
{
   if (!it) return EINA_FALSE;

   return it->disabled;
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
   if (!it) return;
   if (it->view == view) return;

   evas_object_del(it->view);
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
elm_controlbar_item_view_get(const Elm_Controlbar_Item *it)
{
   if (!it) return NULL;

   return it->view;
}

/**
 * Unset the view of the item
 *
 * @param	it The item of controlbar
 * @return	The view for the item
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object *
elm_controlbar_item_view_unset(Elm_Controlbar_Item *it)
{
   if (!it) return NULL;
   if (it->obj == NULL) return NULL;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd) return NULL;
   Evas_Object *content;

   if (it->view == elm_layout_content_get(wd->view, "elm.swallow.view"))
     {
        content = elm_layout_content_unset(wd->view, "elm.swallow.view");
        if (content) evas_object_hide(content);
     }
   else
     content = it->view;

   it->view = NULL;

   return content;
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
   if (!wd)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if (wd->mode == mode) return;

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
         edje_object_signal_emit(_EDJ(wd->selected_box), "elm,state,left", "elm");
         wd->selected_signal = eina_stringshare_add("elm,state,selected_left");
         wd->pressed_signal = eina_stringshare_add("elm,state,pressed_left");
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,left", "elm");
         _check_background(wd);
         _sizing_eval(obj);
         return;
      case ELM_CONTROLBAR_MODE_RIGHT:
         edje_object_signal_emit(_EDJ(wd->selected_box), "elm,state,right", "elm");
         wd->selected_signal = eina_stringshare_add("elm,state,selected_right");
         wd->pressed_signal = eina_stringshare_add("elm,state,pressed_right");
         edje_object_signal_emit(_EDJ(wd->edje), "elm,state,right", "elm");
         _check_background(wd);
         _sizing_eval(obj);
         return;
      default:
         break;
     }

   edje_object_signal_emit(_EDJ(wd->selected_box), "elm,state,default", "elm");
   wd->selected_signal = eina_stringshare_add("elm,state,selected");
   wd->pressed_signal = eina_stringshare_add("elm,state,pressed");
   _check_background(wd);
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
   if (!wd)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if (alpha < 0) wd->alpha = 0;
   else if (alpha > 100) wd->alpha = 100;
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
   if (!wd)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if (wd->auto_align == auto_align) return;

   if (auto_align)
     {
        if (_check_bar_item_number(wd) >= 5 && !wd->more_item)
          {
             i = 0;
             EINA_LIST_FOREACH(wd->items, l, item)
               {
                  if (elm_controlbar_item_visible_get(item))
                    i++;
                  if (i >= 5){
                       _delete_item_in_bar(item);
                  }
               }
             item = elm_controlbar_last_item_get(obj);
             while (!elm_controlbar_item_visible_get(item)){
                  item = elm_controlbar_item_prev(item);
             }
             _create_more_item(wd, item->style);
          }
     }
   else
     {
        if (wd->more_item)
          {
             // delete more item
             if (wd->more_item->view)
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
                  if (!elm_controlbar_item_visible_get(item))
                    _insert_item_in_bar(item, i);
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
   if (!wd)
     {
        fprintf(stderr, "Cannot get smart data\n");
        return;
     }

   if (wd->vertical == vertical) return;
   wd->vertical = vertical;

   if (_check_bar_item_number(wd) > 1)
     {
        _repack_items(wd);
     }
   _check_toolbar_line(wd);
}

/**
 * Get the button object of the item
 *
 * @param	it The item of controlbar
 * @return  button object of the item	
 *
 * @ingroup Controlbar
 */
EAPI Evas_Object *
elm_controlbar_item_button_get(const Elm_Controlbar_Item *it)
{
   if (!it) return NULL;
   if (it->style == OBJECT) return NULL;

   if (it->base_item) return it->base_item;

   return NULL;
}
