#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Softkey Softkey
 * @ingroup Elementary
 *
 * This is a softkey
 */

/**
 * internal data structure of softkey object
 */
#define BTN_ANIMATOR_MAX	4
#define PANEL_ANIMATOR_MAX	1
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *lay;
   Evas_Object *button[2];
   Evas_Object *bg_rect;
   Evas_Object *panel;
   Evas_Object *glow_obj;

   Evas_Coord x, y, w, h;
   Evas_Coord glow_w, glow_h;
   Evas_Coord win_h;
   Evas_Coord panel_height;
   Ecore_Animator *animator;
   Eina_List *items;
   unsigned int panel_btn_idx;
   Eina_Bool button_show[2];
   Eina_Bool show_panel :1;
   Eina_Bool animating :1;
   Eina_Bool is_horizontal;
   double scale_factor;
   int max_button;
   Eina_Bool panel_suppported;
};

struct _Elm_Softkey_Item
{
   Evas_Object *obj;
   Evas_Object *base;
   Evas_Object *icon;
   const char *label;
   void
   (*func)(void *data, Evas_Object *obj, void *event_info);
   const void *data;
   Eina_Bool disabled :1;
};

static void
_item_disable(Elm_Softkey_Item *it, Eina_Bool disabled);
static void
_del_hook(Evas_Object *obj);
static void
_theme_hook(Evas_Object *obj);
static void
_sizing_eval(Evas_Object *obj);
static void
_sub_del(void *data, Evas_Object *obj, void *event_info);

/*
 * callback functions
 */
static void
_softkey_down_cb(void *data, Evas_Object *obj, const char *emission,
                 const char *source);
static void
_softkey_up_cb(void *data, Evas_Object *obj, const char *emission,
               const char *source);

static void
_panel_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void
_panel_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_more_btn_click_cb(void *data, Evas_Object *obj, const char *emission,
                   const char *source);
static void
_close_btn_click_cb(void *data, Evas_Object *obj, const char *emission,
                    const char *source);
static void
_bg_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Eina_Bool
_show_button_animator_cb(void *data);
static Eina_Bool
_hide_button_animator_cb(void *data);
static Eina_Bool
_panel_up_animator_cb(void *data);
static Eina_Bool
_panel_down_animator_cb(void *data);

/*
 * internal function
 */
static int
_show_button(Evas_Object *obj, Elm_Softkey_Type type, Eina_Bool show);
static int
_delete_button(Evas_Object *obj);

static void
_softkey_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void
_softkey_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void
_softkey_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void
_softkey_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void
_softkey_horizontal_set(Evas_Object *obj, Eina_Bool horizontal_mode);
static void
_icon_disable(Evas_Object *icon, Eina_Bool disabled)
{
   Evas_Object *edj;

   if (!icon) return;
   edj = elm_layout_edje_get(icon);

   if (disabled)
     {
        if (!edj)
           edje_object_signal_emit(icon, "elm,state,disabled", "elm");
        else
           edje_object_signal_emit(edj, "elm,state,disabled", "elm");
     }
   else
     {
        if (!edj)
           edje_object_signal_emit(icon, "elm,state,enabled", "elm");
        else
           edje_object_signal_emit(edj, "elm,state,enabled", "elm");
     }
}

static void
_item_disable(Elm_Softkey_Item *it, Eina_Bool disabled)
{
   Widget_Data *wd = elm_widget_data_get(it->obj);
   if (!wd) return;
   if (it->disabled == disabled) return;
   it->disabled = disabled;
   if (it->disabled)
      edje_object_signal_emit(it->base, "elm,state,disabled", "elm");
   else
      edje_object_signal_emit(it->base, "elm,state,enabled", "elm");

   _icon_disable(it->icon, disabled);
}

static void
_del_hook(Evas_Object *obj)
{
   int i;
   Evas_Object *btn;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_event_callback_del(obj, EVAS_CALLBACK_RESIZE,
                                  _softkey_object_resize);
   evas_object_event_callback_del(obj, EVAS_CALLBACK_MOVE, _softkey_object_move);
   evas_object_event_callback_del(obj, EVAS_CALLBACK_SHOW, _softkey_object_show);
   evas_object_event_callback_del(obj, EVAS_CALLBACK_HIDE, _softkey_object_hide);

   /* delete button */
   for (i = 0; i < 2; i++)
     {
        btn = wd->button[i];
        if (btn != NULL)
          {
             //_delete_button(btn);
             evas_object_del(wd->button[i]);
             wd->button[i] = NULL;
          }
     }

   //evas_object_smart_callback_del(obj, "sub-object-del", _sub_del);


   /* delete panel */
   if (wd->panel)
     {
        elm_softkey_panel_del(obj);
     }

   /* delete glow effect image */
   if (wd->glow_obj)
     {
        evas_object_del(wd->glow_obj);
        wd->glow_obj = NULL;
     }
   if (wd->lay)
     {
        evas_object_del(wd->lay);
        wd->lay = NULL;
     }

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Elm_Softkey_Item* item;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Softkey_Item *it;
   const Eina_List *l;

   if (!wd) return;

   _elm_theme_object_set(obj, wd->lay, "softkey", "bg",
                         elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->glow_obj, "softkey", "glow", "default");

   if (wd->button[ELM_SK_LEFT_BTN])
     {
        item = evas_object_data_get(wd->button[ELM_SK_LEFT_BTN], "item_data");
        _elm_theme_object_set(obj, wd->button[ELM_SK_LEFT_BTN], "softkey",
                              "button_left", elm_widget_style_get(obj));

        elm_softkey_item_label_set(item, item->label);

     }

   if (wd->button[ELM_SK_RIGHT_BTN])
     {

        item = evas_object_data_get(wd->button[ELM_SK_RIGHT_BTN], "item_data");

        _elm_theme_object_set(obj, wd->button[ELM_SK_RIGHT_BTN], "softkey",
                              "button_right", elm_widget_style_get(obj));

        elm_softkey_item_label_set(item, item->label);

     }

   if (wd->panel)
     {
        _elm_theme_object_set(obj, wd->panel, "softkey", "panel",
                              elm_widget_style_get(obj));
        if (wd->panel_btn_idx > 0)
          {
             //show more button
             edje_object_signal_emit(wd->lay, "more_btn_show", "");
             EINA_LIST_FOREACH (wd->items, l, it)
               {
                  _elm_theme_object_set(obj, it->base, "softkey", "panel_button",
                                        elm_widget_style_get(obj));
                  if (it->label)
                    {
                       edje_object_part_text_set(it->base, "elm.text", it->label); // set text
                    }
               }
          }
     }

   _sizing_eval(obj);
}

static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   const Eina_List *l;
   Elm_Softkey_Item *it;
   if (!wd) return;

   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (sub == it->icon)
          {
             it->icon = NULL;
          }
        break;
     }
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   _softkey_object_move(obj, NULL, obj, NULL);
   _softkey_object_resize(obj, NULL, obj, NULL);
}

static Eina_Bool
_panel_up_animator_cb(void *data)
{
   int max = PANEL_ANIMATOR_MAX;
   static int progress = 0;
   Widget_Data *wd;
   Evas_Coord ypos;

   wd = elm_widget_data_get(data);
   if (!wd) return 0;

   progress++;
   wd->animating = EINA_TRUE;

   if (progress > max)
     {
        if (!wd->animator) return 0;
        ecore_animator_del(wd->animator);
        wd->animator = NULL;
        wd->animating = EINA_FALSE;
        wd->show_panel = EINA_TRUE;
        progress = 0;
        return 0;
     }

   /* move up panel */
   if (wd->panel)
     {
        ypos = wd->win_h - (wd->panel_height * progress / max);
        evas_object_move(wd->panel, wd->x, ypos);
     }

   return 1;
}

static Eina_Bool
_panel_down_animator_cb(void *data)
{
   int max = PANEL_ANIMATOR_MAX;
   static int progress = 0;
   Widget_Data *wd;
   Evas_Coord ypos;

   wd = elm_widget_data_get(data);
   if (!wd) return 0;

   progress++;
   wd->animating = EINA_TRUE;

   if (progress > max)
     {
        if (!wd->animator) return 0;
        ecore_animator_del(wd->animator);
        wd->animator = NULL;
        wd->animating = EINA_FALSE;
        //wd->animator = ecore_animator_add(_show_button_animator_cb, data);
        wd->show_panel = EINA_FALSE;
        progress = 0;

        evas_object_smart_callback_call(data, "panel,hide", NULL);

        return 0;
     }

   /* move down panel */
   if (wd->panel)
     {
        ypos = wd->win_h - wd->panel_height + (wd->panel_height * progress / max);
        evas_object_move(wd->panel, wd->x, ypos);
     }

   return 1;
}

static Eina_Bool
_hide_button_animator_cb(void *data)
{
   int max = BTN_ANIMATOR_MAX;
   static int progress = 0;
   Widget_Data *wd;
   Evas_Coord btn_w, xpos;

   wd = elm_widget_data_get(data);
   if (!wd) return 0;

   progress++;
   wd->animating = EINA_TRUE;

   if (progress > max)
     {
        if (!wd->animator) return 0;
        ecore_animator_del(wd->animator);
        wd->animating = EINA_FALSE;
        wd->animator = ecore_animator_add(_panel_up_animator_cb, data);
        progress = 0;
        return 0;
     }

   /* move left button */
   if (wd->button[ELM_SK_LEFT_BTN])
     {
        edje_object_part_geometry_get(wd->button[ELM_SK_LEFT_BTN], "button_rect",
                                      NULL, NULL, &btn_w, NULL);
        //evas_object_geometry_get(wd->button[ELM_SK_LEFT_BTN], NULL, NULL, &btn_w, NULL);
        xpos = wd->x + -1 * btn_w * progress / max;
        evas_object_move(wd->button[ELM_SK_LEFT_BTN], xpos, wd->y);
     }

   /* move right button */
   if (wd->button[ELM_SK_RIGHT_BTN])
     {
        edje_object_part_geometry_get(wd->button[ELM_SK_RIGHT_BTN],
                                      "button_rect", NULL, NULL, &btn_w, NULL);
        //evas_object_geometry_get(wd->button[ELM_SK_RIGHT_BTN], NULL, NULL, &btn_w, NULL);
        xpos = (wd->x + wd->w - btn_w) + (btn_w * progress / max);
        evas_object_move(wd->button[ELM_SK_RIGHT_BTN], xpos, wd->y);
     }

   return 1;
}

static Eina_Bool
_show_button_animator_cb(void *data)
{
   int max = BTN_ANIMATOR_MAX;
   static int progress = 0;
   Widget_Data *wd;
   Evas_Coord btn_w, xpos;

   wd = elm_widget_data_get(data);
   if (!wd) return 0;

   progress++;

   wd->animating = EINA_TRUE;

   if (progress > max)
     {
        if (!wd->animator) return 0;
        ecore_animator_del(wd->animator);
        wd->animating = EINA_FALSE;
        progress = 0;
        return 0;
     }

   /* move left button */
   if (wd->button[ELM_SK_LEFT_BTN])
     {
        edje_object_part_geometry_get(wd->button[ELM_SK_LEFT_BTN], "button_rect",
                                      NULL, NULL, &btn_w, NULL);
        //evas_object_geometry_get(wd->button[ELM_SK_LEFT_BTN], NULL, NULL, &btn_w, NULL);
        xpos = wd->x + (-1 * btn_w) + (btn_w * progress / max);
        evas_object_move(wd->button[ELM_SK_LEFT_BTN], xpos, wd->y);
     }

   /* move right button */
   if (wd->button[ELM_SK_RIGHT_BTN])
     {
        edje_object_part_geometry_get(wd->button[ELM_SK_RIGHT_BTN],
                                      "button_rect", NULL, NULL, &btn_w, NULL);
        //evas_object_geometry_get(wd->button[ELM_SK_RIGHT_BTN], NULL, NULL, &btn_w, NULL);
        xpos = wd->x + wd->w - (btn_w * progress / max);
        evas_object_move(wd->button[ELM_SK_RIGHT_BTN], xpos, wd->y);
     }

   return 1;
}

static void
_more_btn_click_cb(void *data, Evas_Object *obj, const char *emission,
                   const char *source)
{
   Widget_Data *wd;
   wd = elm_widget_data_get(data);
   if (!wd) return;

   evas_object_smart_callback_call(data, "panel,show", NULL);

   if (!wd->panel) return;
   evas_object_show(wd->panel);

   if (wd->bg_rect)
     {
        evas_object_show(wd->bg_rect);
     }

   /*if (wd->animating == EINA_FALSE) {
    wd->animator = ecore_animator_add(_hide_button_animator_cb, data);
    }*/
   if (wd->animating == EINA_FALSE)
     {
        wd->animator = ecore_animator_add(_panel_up_animator_cb, data);
     }
}

static void
_close_panel(Evas_Object *obj)
{
   Widget_Data *wd;
   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->bg_rect)
     {
        evas_object_hide(wd->bg_rect);
     }

   if (!wd->panel) return;

   if (wd->animating == EINA_FALSE)
     {
        wd->animator = ecore_animator_add(_panel_down_animator_cb, obj);
     }
}

static void
_bg_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _close_panel(data);
}

static void
_close_btn_click_cb(void *data, Evas_Object *obj, const char *emission,
                    const char *source)
{
   _close_panel(data);
}

static int
_show_button(Evas_Object *obj, Elm_Softkey_Type type, Eina_Bool show)
{
   if (!obj) return -1;

   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *btn = wd->button[type];
   if (!btn) return -1;

   /* Make visible button */
   if (show)
     {
        wd->button_show[type] = EINA_TRUE;
        evas_object_show(btn);
     }
   else
     {
        wd->button_show[type] = EINA_FALSE;
        evas_object_hide(btn);
     }

   return 0;
}

static int
_arrange_button(Evas_Object *obj, Elm_Softkey_Type type)
{
   Widget_Data *wd;
   Evas_Coord btn_w = 0;
   Evas_Object *btn;

   if (!obj) return -1;
   wd = elm_widget_data_get(obj);
   if (!wd) return -1;

   btn = wd->button[type];
   if (!btn) return -1;

   switch (type)
   {
   case ELM_SK_LEFT_BTN:
      evas_object_move(btn, wd->x, wd->y);
      break;
   case ELM_SK_RIGHT_BTN:
      edje_object_part_geometry_get(btn, "button_rect", NULL, NULL, &btn_w,
                                    NULL);
      //evas_object_geometry_get(btn, NULL, NULL, &btn_w, NULL);
      evas_object_move(btn, wd->x + wd->w - btn_w, wd->y);
      break;
   default:
      break;
   }

   return 0;
}

static void
_softkey_up_cb(void *data, Evas_Object *obj, const char *emission,
               const char *source)
{
   Evas_Object *edj = NULL;

   Elm_Softkey_Item *it = (Elm_Softkey_Item *) data;
   elm_softkey_panel_close(it->obj);
   if (it->icon)
     {
        if(!strcmp(evas_object_type_get(it->icon), "edje"))
          {
             edj = it->icon;
          }
        else if(!strcmp(evas_object_type_get(it->icon), "elm_widget") && !strcmp(elm_widget_type_get(it->icon), "layout"))
          {
             edj = elm_layout_edje_get(it->icon);
          }
        if (edj) 
           edje_object_signal_emit(edj, "elm,state,unselected", "elm");
     }

   if (it->func)
      it->func((void *) (it->data), it->obj, it);
   else
      evas_object_smart_callback_call(it->obj, "clicked", it);
}

static void
_softkey_down_cb(void *data, Evas_Object *obj, const char *emission,
                 const char *source)
{
   Evas_Object *edj = NULL;

   Elm_Softkey_Item *it = (Elm_Softkey_Item *) data;
   evas_object_smart_callback_call(it->obj, "press", it);

   if (!it->icon) return;
   
   if(!strcmp(evas_object_type_get(it->icon), "edje"))
     {
        edj = it->icon;
     }
   else if(!strcmp(evas_object_type_get(it->icon), "elm_widget") && !strcmp(elm_widget_type_get(it->icon), "layout"))
     {
        edj = elm_layout_edje_get(it->icon);
     }

   if (edj) 
      edje_object_signal_emit(edj, "elm,state,selected", "elm");
}

static void
_panel_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Elm_Softkey_Item *it = (Elm_Softkey_Item *) data;

   Widget_Data *wd;
   wd = elm_widget_data_get(it->obj);
   if (wd == NULL) return;

   /* hide glow effect */
   if (wd->glow_obj)
     {
        evas_object_hide(wd->glow_obj);
     }

   elm_softkey_panel_close(it->obj);

   if (it->func)
      it->func((void *) (it->data), it->obj, it);
   else
      evas_object_smart_callback_call(it->obj, "clicked", it);
}

static void
_panel_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord glow_x, glow_y;
   Widget_Data *wd;

   Elm_Softkey_Item *it = (Elm_Softkey_Item *) data;
   wd = elm_widget_data_get(it->obj);
   if (wd == NULL) return;

   /* show glow effect */
   if (wd->glow_obj)
     {
        Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *) event_info;
        glow_x = ev->canvas.x - (wd->glow_w / 2);
        glow_y = ev->canvas.y - (wd->glow_h / 2);

        evas_object_move(wd->glow_obj, glow_x, glow_y);
        evas_object_show(wd->glow_obj);
     }

   evas_object_smart_callback_call(it->obj, "press", it);
}

static int
_delete_button(Evas_Object *obj)
{
   if (!obj) return -1;

   if (obj)
     {
        evas_object_del(obj);
        obj = NULL;
     }

   return 0;
}

static void
_calc_win_height(Widget_Data *wd)
{
   wd->win_h = wd->y + wd->h;

   if (wd->bg_rect)
     {
        evas_object_resize(wd->bg_rect, wd->w, wd->win_h);
        evas_object_move(wd->bg_rect, wd->x, 0);
     }

   if (wd->panel)
     {
        if (wd->show_panel)
          {
             evas_object_move(wd->panel, wd->x, (wd->win_h - wd->panel_height));
          }
        else
          {
             evas_object_move(wd->panel, wd->x, wd->win_h);
          }
     }
}

static void
_softkey_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd;
   int i;
   Evas_Coord x, y;

   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;

   evas_object_geometry_get(wd->lay, &x, &y, NULL, NULL);

   wd->x = x;
   wd->y = y;

   evas_object_move(wd->lay, x, y);

   for (i = 0; i < 2; i++)
     {
        _arrange_button((Evas_Object *) data, i);
     }

   _calc_win_height(wd);
}

static void
_softkey_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd;
   Evas_Object *btn;
   int i;
   Evas_Coord btn_w;
   Evas_Coord w, h;

   if (!data) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;

   evas_object_geometry_get(wd->lay, NULL, NULL, &w, &h);

   wd->w = w;
   wd->h = h;

   if (!wd->lay) return;
   evas_object_resize(wd->lay, w, h);

   /* resize button */
   for (i = 0; i < 2; i++)
     {
        btn = wd->button[i];
        if (btn != NULL)
          {
             edje_object_part_geometry_get(btn, "button_rect", NULL, NULL, &btn_w,
                                           NULL);
             evas_object_resize(btn, btn_w, h);
             _arrange_button((Evas_Object *) data, i);
          }
     }

   if (wd->w >= wd->win_h)
     {
        _softkey_horizontal_set(data, EINA_TRUE);
     }
   else
     {
        _softkey_horizontal_set(data, EINA_FALSE);
     }
   _calc_win_height(wd);
}

static void
_softkey_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = NULL;
   Evas_Object *btn;
   int i;
   if (data == NULL) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (wd == NULL) return;

   if (wd->lay)
     {
        evas_object_show(wd->lay);
     }

   /* show button */
   for (i = 0; i < 2; i++)
     {
        btn = wd->button[i];
        if (btn != NULL && wd->button_show[i] == EINA_TRUE)
          {
             evas_object_show(btn);
             //evas_object_clip_set(btn, evas_object_clip_get((Evas_Object *)data));
          }
     }
   if (wd->panel_btn_idx > 0)
     {
        /* show more button */
        edje_object_signal_emit(wd->lay, "more_btn_show", "");
     }
}

static void
_softkey_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = NULL;
   Evas_Object *btn;
   int i;

   if (data == NULL) return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (wd == NULL) return;

   if (wd->lay)
     {
        evas_object_hide(wd->lay);
     }

   /* hide button */
   for (i = 0; i < 2; i++)
     {
        btn = wd->button[i];
        if (btn != NULL)
          {
             evas_object_hide(btn);
          }
     }

   if (wd->panel_btn_idx > 0)
     {
        /* hide more button */
        edje_object_signal_emit(wd->lay, "more_btn_hide", "");
     }
}

/**
 * Add a new softkey to the parent

 * @param[in] parent the parent of the smart object
 * @return		Evas_Object* pointer of softkey(evas object) or NULL
 * @ingroup		Softkey 
 */
EAPI Evas_Object *
elm_softkey_add(Evas_Object *parent)
{
   Evas_Object *obj = NULL;
   Widget_Data *wd = NULL;
   Evas *e;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (e == NULL) return NULL;
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "softkey");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   /* load background edj */
   wd->lay = edje_object_add(e);
   _elm_theme_object_set(obj, wd->lay, "softkey", "bg", "default");
   if (wd->lay == NULL)
     {
        printf("Cannot load bg edj\n");
        return NULL;
     }
   elm_widget_resize_object_set(obj, wd->lay);
   edje_object_signal_callback_add(wd->lay, "clicked", "more_btn",
                                   _more_btn_click_cb, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                  _softkey_object_resize, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                  _softkey_object_move, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW,
                                  _softkey_object_show, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE,
                                  _softkey_object_hide, obj);
   wd->is_horizontal = EINA_FALSE;
   wd->panel_suppported = EINA_TRUE;
   wd->scale_factor = elm_scale_get();
   if (wd->scale_factor == 0.0)
     {
        wd->scale_factor = 1.0;
     }

   /* load glow effect */
   wd->glow_obj = edje_object_add(e);
   _elm_theme_object_set(obj, wd->glow_obj, "softkey", "glow", "default");
   evas_object_geometry_get(wd->glow_obj, NULL, NULL, &wd->glow_w, &wd->glow_h);
   evas_object_resize(wd->glow_obj, wd->glow_w, wd->glow_h);

   // FIXME
   //evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   wd->button[ELM_SK_LEFT_BTN] = wd->button[ELM_SK_RIGHT_BTN] = NULL;
   wd->panel = NULL;
   //_sizing_eval(obj);
   wd->show_panel = EINA_FALSE;
   wd->bg_rect = NULL;

   return obj;
}

static void
_softkey_horizontal_set(Evas_Object *obj, Eina_Bool horizontal_mode)
{
   Widget_Data *wd;
   char buff[32];
   if (!obj) return;
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->is_horizontal = horizontal_mode;
   if (wd->panel)
   {
      if ((edje_object_data_get(wd->panel, "max_item_count") == NULL)
               || (edje_object_data_get(wd->panel, "panel_height") == NULL)
               || (edje_object_data_get(wd->panel, "panel_height_horizontal")
                        == NULL))
        {
           wd->panel_suppported = EINA_FALSE;
        }
      else
         wd->panel_suppported = EINA_TRUE;
      if (wd->panel_suppported == EINA_TRUE)
        {
           if (wd->is_horizontal == EINA_TRUE)
             {
                snprintf(buff, sizeof(buff), "button_%d", (wd->panel_btn_idx
                                                           + wd->max_button));
                edje_object_signal_emit(wd->panel, buff, "panel_rect");
                wd->panel_height
                   = (int) (atoi(edje_object_data_get(wd->panel, buff))
                            * wd->scale_factor);
                evas_object_resize(
                   wd->panel,
                   wd->w,
                   ((int) (atoi(
                            edje_object_data_get(wd->panel,
                                                 "panel_height_horizontal"))
                         * wd->scale_factor)));
             }
           else
             {
                snprintf(buff, sizeof(buff), "button_%d", (wd->panel_btn_idx));
                edje_object_signal_emit(wd->panel, buff, "panel_rect");
                wd->panel_height
                   = (int) (atoi(edje_object_data_get(wd->panel, buff))
                            * wd->scale_factor);
                evas_object_resize(
                   wd->panel,
                   wd->w,
                   ((int) (atoi(
                            edje_object_data_get(wd->panel,
                                                 "panel_height"))
                         * wd->scale_factor)));
             }
        }
      _calc_win_height(wd);
   }
}

/**
 * add side button of softkey
 * @param[in]	obj	softkey object
 * @param[in]	type softkey button type
 * @param[in]	icon The icon object to use for the item
 * @param[in]	label The text label to use for the item
 * @param[in]	func Convenience function to call when this item is selected
 * @param[in]	data Data to pass to convenience function
 * @return	A handle to the item added
 * 
 * @ingroup	Softkey  
 */
EAPI Elm_Softkey_Item *
elm_softkey_button_add(Evas_Object *obj, Elm_Softkey_Type type,
                       Evas_Object *icon, const char *label, void
                       (*func)(void *data, Evas_Object *obj, void *event_info),
                       const void *data)
{
   Widget_Data *wd;
   Evas* evas;
   char button_type[64];
   Elm_Softkey_Item *it;

   if (!obj) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   if (wd->button[type])
     {
        printf("already created.\n");
        return NULL;
     }

   /* get evas */
   evas = evas_object_evas_get(obj);
   if (!evas) return NULL;

   /* set item data */
   it = ELM_NEW(Elm_Softkey_Item);
   it->obj = obj;
   it->func = func;
   it->data = data;
   it->label = NULL;
   it->icon = NULL;
   /* load button edj */
   if (wd->button[type] == NULL)
     {
        if (type == ELM_SK_LEFT_BTN)
          {
             strcpy(button_type, "button_left");
          }
        else
          {
             strcpy(button_type, "button_right");
          }

        it->base = wd->button[type] = edje_object_add(evas);
        if (!wd->button[type])
          {
             free(it);
             return NULL;
          }
        _elm_theme_object_set(obj, wd->button[type], "softkey", button_type,
                              elm_widget_style_get(obj));

        wd->button_show[type] = EINA_TRUE;
        if (evas_object_visible_get(obj))
          {
             evas_object_show(wd->button[type]);
          }
        evas_object_smart_member_add(wd->button[type], obj);

        edje_object_signal_callback_add(wd->button[type], "elm,action,down", "",
                                        _softkey_down_cb, it);
        edje_object_signal_callback_add(wd->button[type], "elm,action,click", "",
                                        _softkey_up_cb, it);

        evas_object_clip_set(wd->button[type], evas_object_clip_get(obj));
        if (wd->panel) evas_object_raise(wd->panel);
     }

   _sizing_eval(obj);

   elm_softkey_item_label_set(it, label);
   elm_softkey_item_icon_set(it, icon);

   if (wd->button[type])
      evas_object_data_set(wd->button[type], "item_data", it);
   else
     {
        if (it->label) eina_stringshare_del(it->label);
        it->label = NULL;
        free(it);
        return NULL;
     }

   return it;
}

/**
 * delete side button of softkey
 * @param[in]	obj	softkey object
 * @param[in]	type softkey button type
 * 
 * @ingroup	Softkey  
 */
EAPI void
elm_softkey_button_del(Evas_Object *obj, Elm_Softkey_Type type)
{
   Widget_Data *wd;
   Elm_Softkey_Item *it;
   Evas_Object *btn;

   if (!obj)
     {
        printf("Invalid argument: softkey object is NULL\n");
        return;
     }

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   btn = wd->button[type];
   if (!btn) return;

   it = evas_object_data_get(btn, "item_data");
   //_delete_button(btn);
   edje_object_signal_callback_del(wd->button[type], "elm,action,down", "",
                                   _softkey_down_cb);
   edje_object_signal_callback_del(wd->button[type], "elm,action,click", "",
                                   _softkey_up_cb);
   evas_object_del(wd->button[type]);
   if (it->label) eina_stringshare_del(it->label);
   if (it->icon) evas_object_del(it->icon);
   free(it);
   wd->button[type] = NULL;
}

/**
 * show button of softkey
 * @param[in]	obj	softkey object
 * @param[in]	type	softkey button type
 *
 * @ingroup	Softkey  
 */
EAPI void
elm_softkey_button_show(Evas_Object *obj, Elm_Softkey_Type type)
{
   _show_button(obj, type, EINA_TRUE);
}

/**
 * hide button of softkey
 * @param[in]	obj	softkey object
 * @param[in]	type	softkey button type
 *
 * @ingroup	Softkey  
 */
EAPI void
elm_softkey_button_hide(Evas_Object *obj, Elm_Softkey_Type type)
{
   _show_button(obj, type, EINA_FALSE);
}

/**
 * add item in panel
 * @param[in]	obj	softkey object
 * @param[in]	icon The icon object
 * @param[in]	label The text label to use for the item
 * @param[in]	func Convenience function to call when this item is selected
 * @param[in]	data Data to pass to convenience function
 * @return	A handle to the item added
 * 
 * @ingroup	Softkey  
 */
EAPI Elm_Softkey_Item *
elm_softkey_panel_item_add(Evas_Object *obj, Evas_Object *icon,
                           const char *label, void
                           (*func)(void *data, Evas_Object *obj,
                                   void *event_info), const void *data)
{
   Widget_Data *wd;
   Evas *evas;
   char button_name[32];
   Evas_Object *btn;
   Elm_Softkey_Item *it;
   char buff[PATH_MAX];

   wd = elm_widget_data_get(obj);
   if (!wd)
     {
        printf("Cannot get smart data\n");
        return NULL;
     }

   /* get evas */
   evas = evas_object_evas_get(obj);
   if (!evas) return NULL;

   if (wd->panel == NULL)
     {
        /* create panel */
        wd->bg_rect = evas_object_rectangle_add(evas);
        if (!wd->bg_rect) return NULL;
        evas_object_color_set(wd->bg_rect, 10, 10, 10, 150);
        evas_object_pass_events_set(wd->bg_rect, 0);

        if (wd->bg_rect)
          {
             evas_object_resize(wd->bg_rect, wd->w, wd->win_h);
             evas_object_event_callback_add(wd->bg_rect, EVAS_CALLBACK_MOUSE_UP,
                                            _bg_click_cb, obj);
             evas_object_smart_member_add(wd->bg_rect, obj);
          }

        wd->panel = edje_object_add(evas);
        if (!wd->panel) return NULL;
        _elm_theme_object_set(obj, wd->panel, "softkey", "panel",
                              elm_widget_style_get(obj));

        evas_object_move(wd->panel, 0, wd->win_h);
        edje_object_signal_callback_add(wd->panel, "clicked", "close_btn",
                                        _close_btn_click_cb, obj);
        evas_object_smart_member_add(wd->panel, obj);
        if (evas_object_visible_get(obj))
          {
             evas_object_show(wd->panel);
          }
        wd->panel_height = 0;
        if ((edje_object_data_get(wd->panel, "max_item_count") == NULL)
            || (edje_object_data_get(wd->panel, "panel_height") == NULL)
            || (edje_object_data_get(wd->panel, "panel_height_horizontal")
                == NULL))
          {
             //If this key is not found in data section, then it means the panel won't come.
             wd->max_button = 0;
             // delete panel
             if (wd->panel)
               {
                  elm_softkey_panel_del(obj);
               }
             wd->panel_suppported = EINA_FALSE;
             return NULL;
          }
        else
          {
             wd->max_button = (int) (atoi(edje_object_data_get(wd->panel,
                                                               "max_item_count")));
             if (wd->is_horizontal)
               {
                  evas_object_resize(
                     wd->panel,
                     wd->w,
                     ((int) (atoi(
                              edje_object_data_get(wd->panel,
                                                   "panel_height_horizontal"))
                           * wd->scale_factor)));
               }
             else
               {
                  evas_object_resize(
                     wd->panel,
                     wd->w,
                     ((int) (atoi(
                              edje_object_data_get(wd->panel,
                                                   "panel_height"))
                           * wd->scale_factor)));
               }
          }

        evas_object_clip_set(wd->panel, evas_object_clip_get(obj));
     }

   wd->panel_btn_idx++;
   if (wd->panel_btn_idx >= wd->max_button) return NULL;

   /* set item data */
   it = ELM_NEW(Elm_Softkey_Item);
   it->obj = obj;
   if (label) it->label = eina_stringshare_add(label);
   it->icon = icon;
   it->func = func;
   it->data = data;
   /* load panel button */
   it->base = btn = edje_object_add(evas);
   if (!btn)
     {
        if (it->label) eina_stringshare_del(it->label);
        free(it);
        wd->panel_btn_idx--;
        return NULL;
     }
   _elm_theme_object_set(obj, btn, "softkey", "panel_button",
                         elm_widget_style_get(obj));

   edje_object_part_text_set(btn, "elm.text", label); /* set text */
   //edje_object_message_signal_process(btn);

   if (wd->panel)
     {
        /* swallow button */
        snprintf(button_name, sizeof(button_name), "panel_button_area_%d",
                 wd->panel_btn_idx);
        edje_object_part_swallow(wd->panel, button_name, btn);

        if (wd->is_horizontal)
          {
             snprintf(buff, sizeof(buff), "button_%d", wd->max_button
                      + wd->panel_btn_idx);
             edje_object_signal_emit(wd->panel, buff, "panel_rect");
             const char* val = edje_object_data_get(wd->panel, buff);
             if (val)
                wd->panel_height = (int) (atoi(val) * wd->scale_factor);
             else
               {
                  if (it->label) eina_stringshare_del(it->label);
                  evas_object_del(it->base);
                  free(it);
                  wd->panel_btn_idx--;
                  return NULL;
               }
          }
        else
          {
             snprintf(buff, sizeof(buff), "button_%d", wd->panel_btn_idx);
             edje_object_signal_emit(wd->panel, buff, "panel_rect");
             const char* val = edje_object_data_get(wd->panel, buff);
             if (val)
                wd->panel_height = (int) (atoi(val) * wd->scale_factor);
             else
               {
                  if (it->label) eina_stringshare_del(it->label);
                  evas_object_del(it->base);
                  free(it);
                  wd->panel_btn_idx--;
                  return NULL;
               }
          }
     }
   else
     {
        wd->panel_btn_idx--;
        return NULL;
     }
   evas_object_event_callback_add(btn, EVAS_CALLBACK_MOUSE_DOWN,
                                  _panel_down_cb, it);
   evas_object_event_callback_add(btn, EVAS_CALLBACK_MOUSE_UP, _panel_up_cb, it);
   wd->items = eina_list_append(wd->items, it);
   if (evas_object_visible_get(obj))
     {
        /* show more button */
        edje_object_signal_emit(wd->lay, "more_btn_show", "");
     }
   return it;
}

/**
 * delete panel

 * @param[in] obj softkey object
 * @return int 0 (SUCCESS) or -1 (FAIL)
 *
 * @ingroup Softkey
 */
EAPI int
elm_softkey_panel_del(Evas_Object *obj)
{
   Widget_Data *wd;
   char button_name[32];
   Evas_Object *btn;
   int i;
   Elm_Softkey_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   if (wd->panel == NULL) return -1;
   if (wd->animator)
     {
        ecore_animator_del(wd->animator);
        wd->animator = NULL;
        wd->animating = EINA_FALSE;
     }
   /* delete background */
   if (wd->bg_rect)
     {
        //evas_object_event_callback_del(wd->bg_rect, EVAS_CALLBACK_MOUSE_UP, _bg_click_cb);
        evas_object_del(wd->bg_rect);
        wd->bg_rect = NULL;
     }

   for (i = 1; i <= wd->panel_btn_idx; i++)
     {
        snprintf(button_name, sizeof(button_name), "panel_button_area_%d", i);
        btn = edje_object_part_swallow_get(wd->panel, button_name);
        //_delete_button(btn);
        if (btn)
          {
             //edje_object_part_unswallow(wd->panel, btn);
             //evas_object_event_callback_del(btn, EVAS_CALLBACK_MOUSE_DOWN, _panel_down_cb);
             //evas_object_event_callback_del(btn, EVAS_CALLBACK_MOUSE_UP, _panel_up_cb);
             evas_object_del(btn);
          }
     }

   EINA_LIST_FREE(wd->items, it)
     {
        if (it->label) eina_stringshare_del(it->label);
        it->base = NULL;
        if (it->icon) it->icon = NULL;
        free(it);
     }

   wd->panel_btn_idx = 0;
   wd->panel_height = 0;

   //hide more button
   edje_object_signal_emit(wd->lay, "more_btn_hide", "");

   if (wd->panel)
     {
        //evas_object_move(wd->panel, 0, wd->win_h);
        evas_object_clip_unset(wd->panel);
        evas_object_del(wd->panel);
        wd->show_panel = EINA_FALSE;
        wd->panel = NULL;
     }

   return 0;
}

/**
 * sliding up panel if it is closed

 * @param[in] obj softkey object
 * @return int 0 (SUCCESS) or -1 (FAIL)
 *
 * @ingroup Softkey
 */
EAPI int
elm_softkey_panel_open(Evas_Object *obj)
{
   Widget_Data *wd;
   wd = elm_widget_data_get(obj);

   if (!wd) return -1;
   if (!wd->panel) return -1;

   if (wd->show_panel == EINA_FALSE)
     {
        _more_btn_click_cb(obj, NULL, NULL, NULL);
     }

   return 0;
}

/**
 * sliding down panel if it is opened

 * @param[in] obj softkey object
 * @return int 0 (SUCCESS) or -1 (FAIL)
 *
 * @ingroup Softkey
 */
EAPI int
elm_softkey_panel_close(Evas_Object *obj)
{
   Widget_Data *wd;
   wd = elm_widget_data_get(obj);
   if (!wd) return -1;

   if (wd->panel == NULL) return -1;

   if (wd->show_panel == EINA_TRUE)
     {
        _close_btn_click_cb(obj, obj, NULL, NULL);
     }

   return 0;
}

/**
 * Set the icon of an softkey item
 *
 * @param[in] it The item to set the icon
 * @param[in] icon The icon object
 *
 * @ingroup Softkey
 */
EAPI void
elm_softkey_item_icon_set(Elm_Softkey_Item *it, Evas_Object *icon)
{
   if (!it) return;

   if ((it->icon != icon) && (it->icon))
     {
        //elm_widget_sub_object_del(it->obj, it->icon);
        evas_object_del(it->icon);
        it->icon = NULL;
     }

   if ((icon) && (it->icon != icon))
     {
        it->icon = icon;
        //elm_widget_sub_object_add(it->obj, icon);
        edje_object_part_swallow(it->base, "elm.swallow.icon", icon);
        edje_object_signal_emit(it->base, "elm,state,icon,visible", "elm");
        //edje_object_message_signal_process(it->base);
        _sizing_eval(it->obj);
     }
   else
      it->icon = icon;
}

/**
 * Get the icon of an softkey item
 *
 * @param[in] it The item to get the icon
 * @return The icon object
 *
 * @ingroup Softkey
 */
EAPI Evas_Object *
elm_softkey_item_icon_get(Elm_Softkey_Item *it)
{
   if (!it) return NULL;
   return it->icon;
}

/**
 * Get the text label of an softkey item
 *
 * @param[in] it The item to set the label
 * @param[in] label label
 *
 * @ingroup Softkey
 */
EAPI void
elm_softkey_item_label_set(Elm_Softkey_Item *it, const char *label)
{
   if (!it) return;
   if (it->label) eina_stringshare_del(it->label);

   if (label)
     {
        it->label = eina_stringshare_add(label);
        edje_object_signal_emit(it->base, "elm,state,text,visible", "elm");

        /* set text */
        edje_object_part_text_set(it->base, "elm.text", label == NULL ? ""
                                  : label);
     }
   else
     {
        it->label = NULL;
        edje_object_signal_emit(it->base, "elm,state,text,hidden", "elm");
     }
   //edje_object_message_signal_process(it->base);
}

/**
 * Set the item callback function 
 *
 * @param[in] it Item to set callback function.
 * @param[in] func callback function pointer.
 * @param[in] data callback function argument data.
 *
 * @ingroup Softkey
 */
EAPI void
elm_softkey_item_callback_set(Elm_Softkey_Item* item, void
(*func)(void *data, Evas_Object *obj, void *event_info), const void *data)
{
   if (!item) return;

   item->func = func;
   item->data = data;
}

/**
 * Get the text label of an softkey item
 *
 * @param[in] it The item to get the label
 * @return The text label of the softkey item
 *
 * @ingroup Softkey
 */
EAPI const char *
elm_softkey_item_label_get(Elm_Softkey_Item *it)
{
   if (!it) return NULL;
   return it->label;
}

EAPI Eina_Bool
elm_softkey_item_disabled_get(Elm_Softkey_Item *it)
{
   if (!it) return EINA_FALSE;
   return it->disabled;
}

EAPI void
elm_softkey_item_disabled_set(Elm_Softkey_Item *it, Eina_Bool disabled)
{
   if (!it) return;
   _item_disable(it, disabled);
}
