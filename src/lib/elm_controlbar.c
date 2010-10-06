/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/**
 * @defgroup Controlbar Controlbar
 * @ingroup Elementary
 *
 * This is a Controlbar. It can contain label and icon objects.
 * In edit mode, you can change the location of items.
 */  
   
#include <string.h>
#include <stdbool.h>
#include <math.h>
   
#include <Elementary.h>
#include "elm_priv.h"
   
#ifndef EAPI
#define EAPI __attribute__ ((visibility("default")))
#endif

#define MAX_ARGS	512
#define EDIT_ROW_NUM	4
   
#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))
#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)
   
#define TABBAR 0
#define TOOLBAR 1
#define OBJECT 2
   
// internal data structure of controlbar object
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data 
{
   Evas * evas;
   Evas_Object * object;
   Evas_Object * parent;
   Evas_Object * view;
   Evas_Object * view_content;
   Evas_Object * edit_box;
   Evas_Object * edit_table;
   Evas_Object * navigation;
   Evas_Object * edje;
   Evas_Object * bg;
   Evas_Object * box;
   Evas_Object * event_box;
   Evas_Object * selected_box;
   
   Evas_Object * moving_obj;
   Elm_Controlbar_Item * more_item;
   Elm_Controlbar_Item * moving_item;
   Elm_Controlbar_Item * pre_item;
   Elm_Controlbar_Item * cur_item;
   Evas_Coord x, y, w, h;
   Eina_Bool edit_mode;
   Eina_Bool auto_align;
   int mode;
   int alpha;
   int empty_num;
   int num;
   Eina_List * items;
   Eina_List * visible_items;
   int animating;
   Ecore_Event_Handler * move_event;
   Ecore_Event_Handler * up_event;
   Ecore_Event_Handler * bar_move_event;
   Ecore_Event_Handler * bar_up_event;
   
   void (*ani_func) (void *data, Evas_Object * obj, void *event_info);
   void *ani_data;
   Eina_Bool init_animation;

   Ecore_Timer *effect_timer;
   Eina_Bool selected_animation;

   Elm_Xml_Animator *xa;
};

struct _Elm_Controlbar_Item 
{
   Evas_Object * obj;
   Evas_Object * base;
   Evas_Object * base_item;
   Evas_Object * edit;
   Evas_Object * edit_item;
   Evas_Object * view;
   Evas_Object * label;
   Evas_Object * icon;
   Evas_Object * edit_label;
   Evas_Object * edit_icon;
   const char *icon_path;
   const char *text;
   void (*func) (void *data, Evas_Object * obj, void *event_info);
   void *data;
   int order;
   int sel;
   int style;
   int badge;
   Eina_Bool selected;
   Eina_Bool editable;
   Eina_Bool disable;
};

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


// prototype
static int check_bar_item_number(Widget_Data *wd);
static void selected_box(Elm_Controlbar_Item * it);
static int pressed_box(Elm_Controlbar_Item * it);
static void object_color_set(Evas_Object *ly, const char *color_part, const char *obj_part);
static void edit_item_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void edit_item_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void bar_item_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void bar_item_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

/////////////////////////
// temp function
////////////////////////

static void
print_all_items(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;

    EINA_LIST_FOREACH(wd->items, l, item)
   {
      printf("item label : %s  order : %d address : %p\n", item->text, item->order, item);
   }
}

///////////////////////////////////////////////////////////////////
//
//  Smart Object basic function
//
////////////////////////////////////////////////////////////////////
   
static void
_controlbar_move(void *data, Evas_Object * obj) 
{
   Widget_Data * wd;
   Evas_Coord x, y;
   if (!data)
      return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd)
      return;
   evas_object_geometry_get(wd->edje, &x, &y, NULL, NULL);
   wd->x = x;
   wd->y = y;
   evas_object_move(wd->edje, x, y);
   evas_object_move(wd->view, x, y);
   evas_object_geometry_get(wd->parent, &x, &y, NULL, NULL);
   evas_object_move(wd->edit_box, x, y);
   evas_object_move(wd->event_box, x, y);
}

static void
_controlbar_resize(void *data, Evas_Object * obj) 
{
   Widget_Data * wd;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   Evas_Coord y, y_, w, h, height;
   if (!data)
      return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd)
      return;
   evas_object_geometry_get(wd->edje, NULL, &y, &w, &h);
   wd->w = w;
   wd->h = h;
   evas_object_resize(wd->edje, w, h);
   evas_object_geometry_get(edje_object_part_object_get(wd->edje, "bg_image"),
			      NULL, NULL, NULL, &height);
   evas_object_resize(wd->view, w, h - height + 1);

   evas_object_geometry_get(wd->parent, NULL, &y_, NULL, NULL);
   //evas_object_resize(wd->edit_box, w, h);
   evas_object_resize(wd->edit_box, w, h + y - y_);
   //evas_object_resize(wd->event_box, w, h);
   evas_object_resize(wd->event_box, w, h + y - y_);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
	if(item->label)
	  elm_label_wrap_width_set(item->label, (int)(wd->w/check_bar_item_number(wd))-20);
	if(item->edit_label)
	  elm_label_wrap_width_set(item->edit_label, (int)(wd->w/check_bar_item_number(wd))-20);
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
   if (!data)
      return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd)
      return;
   evas_object_show(wd->view);
   evas_object_show(wd->edit_box);
   evas_object_show(wd->edje);
   evas_object_show(wd->box);
   evas_object_show(wd->event_box);
}

static void
_controlbar_object_hide(void *data, Evas * e, Evas_Object * obj,
			void *event_info) 
{
   Widget_Data * wd;
   if (!data)
      return;
   wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd)
      return;
   evas_object_hide(wd->view);
   evas_object_hide(wd->edit_box);
   evas_object_hide(wd->edje);
   evas_object_hide(wd->box);
   evas_object_hide(wd->event_box);
}

static void 
_del_hook(Evas_Object * obj) 
{
   Widget_Data * wd = elm_widget_data_get(obj);
   Elm_Controlbar_Item * item;
   Evas_Object * content;
   if (!wd)
      return;
   
   EINA_LIST_FREE(wd->items, item)
   {
      if(item == wd->more_item)
	   if(item->view) 
	     evas_object_del(item->view);
      eina_stringshare_del(item->text);
      if (item->icon)
	 evas_object_del(item->icon);
      if (item->base)
	 evas_object_del(item->base);
      if (item->base_item)
	 evas_object_del(item->base_item);
      if (item->edit)
	 evas_object_del(item->edit);
      if (item->edit_item)
	 evas_object_del(item->edit_item);
      free(item);
      item = NULL;
   }
   if (wd->view)
     {
	content = elm_layout_content_unset(wd->view, "elm.swallow.view");
	evas_object_hide(content);
	evas_object_del(wd->view);
	wd->view = NULL;
     }
   if (wd->edit_box)
     {
	evas_object_del(wd->edit_box);
	wd->edit_box = NULL;
     }
   if (wd->navigation)
     {
	evas_object_del(wd->navigation);
	wd->navigation = NULL;
     }
   if (wd->bg)
     {
	evas_object_del(wd->bg);
	wd->bg = NULL;
     }
   if (wd->edje)
     {
	evas_object_del(wd->edje);
	wd->edje = NULL;
     }
   if (wd->selected_box)
     {
	evas_object_del(wd->selected_box);
	wd->edje = NULL;
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
   if (wd->xa)
     {
	elm_xml_animator_del(wd->xa);
	wd->xa = NULL;
     }
   if (wd->effect_timer)
     {
	ecore_timer_del(wd->effect_timer);
	wd->effect_timer = NULL;
     }

   free(wd);
   wd = NULL;
}

static void 
_theme_hook(Evas_Object * obj) 
{
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   Evas_Object * color;
   int r, g, b, a;

   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd)
      return;
   _elm_theme_object_set(obj, wd->edje, "controlbar", "base",
			   elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->bg, "controlbar", "background",
			   elm_widget_style_get(obj));
   evas_object_color_get(wd->bg, &r, &g, &b, NULL);
   evas_object_color_set(wd->bg, r, g, b, (int)(255 * wd->alpha / 100));
   elm_layout_theme_set(wd->view, "controlbar", "view", elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->edit_box, "controlbar", "edit_box",
			  elm_widget_style_get(obj));
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->style != OBJECT)
	{
	   elm_layout_theme_set(item->base, "controlbar", "item_bg",
				 elm_widget_style_get(obj));
	   //object_color_set(item->base, "elm.item.default.color", "elm.swallow.icon");
	   elm_layout_theme_set(item->edit, "controlbar", "item_bg",
				  elm_widget_style_get(obj));
	   if (!item->editable)
	     {
		color =
		   (Evas_Object *)
		   edje_object_part_object_get(_EDJ(item->edit),
					       "elm.item.uneditable.color");
		if (color)
		   evas_object_color_get(color, &r, &g, &b, &a);
		evas_object_color_set(item->edit_item, r, g, b, a);
	     }
	   //if (item->label)
	     //{
		//edje_object_part_text_set(_EDJ(item->base), "elm.text",
		//			   item->label);
		//edje_object_part_text_set(_EDJ(item->edit_item), "elm.text",
		//			   item->label);
	     //}
	   if (item->label && item->icon)
	     {
		edje_object_signal_emit(_EDJ(item->base_item),
					 "elm,state,icon_text", "elm");
		edje_object_signal_emit(_EDJ(item->edit_item),
					 "elm,state,icon_text", "elm");
	     }
	   if (item->selected)
	     {
		selected_box(item);
	     }
	}
   }
}

static void 
_sub_del(void *data, Evas_Object * obj, void *event_info) 
{
   Widget_Data * wd = elm_widget_data_get(obj);
   
      //Evas_Object *sub = event_info;
      if (!wd)
      return;
   
      /*
       * if (sub == wd->icon)
       * {
       * edje_object_signal_emit(wd->btn, "elm,state,icon,hidden", "elm");
       * evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
       * _changed_size_hints, obj);
       * wd->icon = NULL;
       * edje_object_message_signal_process(wd->btn);
       * _sizing_eval(obj);
       * }
       */ 
}

static void 
_sizing_eval(Evas_Object * obj) 
{
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd)
      return;
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
	
static void
set_evas_map(Evas_Object * obj, Evas_Coord x, Evas_Coord y, Evas_Coord w,
	     Evas_Coord h) 
{
   if (obj == NULL)
     {
	return;
     }
   Evas_Map * map = evas_map_new(4);
   if (map == NULL)
      return;
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, obj, 0);
   evas_object_map_enable_set(obj, EINA_TRUE);
   evas_map_util_3d_perspective(map, x + w / 2, y + h / 2, 0, w * 10);
   evas_map_util_points_populate_from_geometry(map, x, y, w, h, 0);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}

static Eina_Bool
move_evas_map(void *data) 
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
	ecore_animator_del(ad->timer);
	ad->timer = NULL;
	set_evas_map(ad->obj, px, py, pw, ph);
	if (ad->func != NULL)
	   ad->func(ad->data, ad->obj);
	return ECORE_CALLBACK_CANCEL;
     }
   else
     {
	set_evas_map(ad->obj, px, py, pw, ph);
     }
   return ECORE_CALLBACK_RENEW;
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
	ecore_animator_del(ad->timer);
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

static Eina_Bool 
move_fade_out_object(void *data) 
{
   double t;

   int dx, dy, dw, dh, da;

   int px, py, pw, ph, pa;

   int x, y, w, h, a;

   int r, g, b;
   
   Animation_Data * ad = (Animation_Data *) data;
   t = ELM_MAX(0.0, current_time_get() - ad->start_time) / 1000;
   dx = ad->tx - ad->fx;
   dy = ad->ty - ad->fy;
   dw = ad->tw - ad->fw;
   dh = ad->th - ad->fh;
   da = 255;
   if (t <= ad->time)
     {
	x = (1 * sin((t / ad->time) * (M_PI / 2)) * dx);
	y = (1 * sin((t / ad->time) * (M_PI / 2)) * dy);
	w = (1 * sin((t / ad->time) * (M_PI / 2)) * dw);
	h = (1 * sin((t / ad->time) * (M_PI / 2)) * dh);
	a = (1 * sin((t / ad->time) * (M_PI / 2)) * da);
     }
   else
     {
	x = dx;
	y = dy;
	w = dw;
	h = dh;
	a = da;
     }
   px = ad->fx + x;
   py = ad->fy + y;
   pw = ad->fw + w;
   ph = ad->fh + h;
   pa = 255 - a;
   if (x == dx && y == dy && w == dw && h == dh)
     {
	ecore_animator_del(ad->timer);
	ad->timer = NULL;
	evas_object_move(ad->obj, px, py);
	//evas_object_resize(ad->obj, 480, 600);
	evas_object_resize(ad->obj, pw, ph);
	evas_object_color_get(ad->obj, &r, &g, &b, &a);
	evas_object_color_set(ad->obj, r, g, b, pa);
	evas_object_show(ad->obj);
	if (ad->func != NULL)
	   ad->func(ad->data, ad->obj);
	return ECORE_CALLBACK_CANCEL;
     }
   else
     {
	evas_object_move(ad->obj, px, py);
	//evas_object_resize(ad->obj, 480, 600);
	evas_object_resize(ad->obj, pw, ph);
	evas_object_color_get(ad->obj, &r, &g, &b, &a);
	evas_object_color_set(ad->obj, r, g, b, pa);
	evas_object_show(ad->obj);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
move_fade_in_object(void *data) 
{
   double t;

   int dx, dy, dw, dh, da;

   int px, py, pw, ph, pa;

   int x, y, w, h, a;

   int r, g, b;

   Animation_Data * ad = (Animation_Data *) data;
   t = ELM_MAX(0.0, current_time_get() - ad->start_time) / 1000;
   dx = ad->tx - ad->fx;
   dy = ad->ty - ad->fy;
   dw = ad->tw - ad->fw;
   dh = ad->th - ad->fh;
   da = 255;
   if (t <= ad->time)
     {
	x = (1 * sin((t / ad->time) * (M_PI / 2)) * dx);
	y = (1 * sin((t / ad->time) * (M_PI / 2)) * dy);
	w = (1 * sin((t / ad->time) * (M_PI / 2)) * dw);
	h = (1 * sin((t / ad->time) * (M_PI / 2)) * dh);
	a = (1 * sin((t / ad->time) * (M_PI / 2)) * da);
     }
   else
     {
	x = dx;
	y = dy;
	w = dw;
	h = dh;
	a = da;
     }
   px = ad->fx + x;
   py = ad->fy + y;
   pw = ad->fw + w;
   ph = ad->fh + h;
   pa = a;
   if (x == dx && y == dy && w == dw && h == dh)
     {
	ecore_animator_del(ad->timer);
	ad->timer = NULL;
	evas_object_move(ad->obj, px, py);
	//evas_object_resize(ad->obj, 480, 600);
	evas_object_resize(ad->obj, pw, ph);
	evas_object_color_get(ad->obj, &r, &g, &b, &a);
	evas_object_color_set(ad->obj, r, g, b, pa);
	evas_object_show(ad->obj);
	if (ad->func != NULL)
	   ad->func(ad->data, ad->obj);
	return ECORE_CALLBACK_CANCEL;
     }
   else
     {
	evas_object_move(ad->obj, px, py);
	//evas_object_resize(ad->obj, 480, 600);
	evas_object_resize(ad->obj, pw, ph);
	evas_object_color_get(ad->obj, &r, &g, &b, &a);
	evas_object_color_set(ad->obj, r, g, b, pa);
	evas_object_show(ad->obj);
     }
   return ECORE_CALLBACK_RENEW;
}

static void
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
}

static void 
end_item_animation_effect(void *data, Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *)data;
   elm_xml_animator_object_del(wd->xa, obj);
}

static Eina_Bool
item_animation_effect(void *data)
{
   Widget_Data *wd = (Widget_Data *)data;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int rand;
   Eina_Bool check;
  
   if(!wd->xa){
	wd->xa = elm_xml_animator_add(wd->object);
	elm_xml_animator_file_set(wd->xa, "/usr/share/xmls/elementary.xml");
   }

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
		   check = elm_xml_animator_object_add(wd->xa, item->base_item, "test", end_item_animation_effect, wd);
		   break;
		case 1:
		   elm_xml_animator_object_add(wd->xa, item->base_item, "test2", end_item_animation_effect, wd);
		   break;
		case 2:
		   elm_xml_animator_object_add(wd->xa, item->base_item, "test3", end_item_animation_effect, wd);
		   break;
		case 3:
		   elm_xml_animator_object_add(wd->xa, item->base_item, "test4", end_item_animation_effect, wd);
		   break;
		default:
		   break;
	       }
	     break;
	  }
     }

   elm_xml_animator_run(wd->xa);

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
   if (item1->order <= 0)
      return 1;
   if (item2->order <= 0)
      return -1;
   return item1->order > item2->order ? 1 : -1;
}

static int
sort_reverse_cb(const void *d1, const void *d2) 
{
   Elm_Controlbar_Item * item1, *item2;
   item1 = (Elm_Controlbar_Item *) d1;
   item2 = (Elm_Controlbar_Item *) d2;
   if (item1->order <= 0)
      return 1;
   if (item2->order <= 0)
      return -1;
   return item1->order > item2->order ? -1 : 1;
}

static void
done_button_cb(void *data, Evas_Object * obj, void *event_info) 
{
   Widget_Data * wd = (Widget_Data *) data;

   wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), sort_cb);

   evas_object_smart_callback_call(wd->object, "edit,end", wd->items);

   edje_object_signal_emit(wd->edit_box, "elm,state,hide,edit_box", "elm");
   wd->edit_mode = EINA_FALSE;
} 

///////////////////////////////////////////////////////////////////
//
//  basic utility function
//
////////////////////////////////////////////////////////////////////

static int
check_bar_item_number(Widget_Data *wd)
{
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   int num = 0;

   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if(item->order > 0) num++;
   }
   
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
		elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
		evas_object_show(item->base);
	     }
	}
     }
   it->order = order;
   elm_table_pack(wd->box, it->base, it->order - 1, 0, 1, 1);
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
		elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
	     }
	}
     }
}

static void
item_exchange_animation_cb(void *data, Evas_Object * obj) 
{
   Widget_Data * wd = (Widget_Data *) data;
   wd->animating--;
   if (wd->animating < 0)
     {
	printf("animation error\n");
	wd->animating = 0;
     }
   evas_object_map_enable_set(obj, EINA_FALSE);
}

static void
item_exchange_in_bar(Elm_Controlbar_Item * it1, Elm_Controlbar_Item * it2) 
{
   int order;

   Evas_Coord x, y, w, h;
   Evas_Coord x_, y_, w_, h_;
   Widget_Data * wd = elm_widget_data_get(it1->obj);
   evas_object_geometry_get(wd->moving_item->edit, &x, &y, &w, &h);
   set_evas_map(wd->moving_obj, (wd->x + wd->w / 2), (wd->y + wd->h / 2), 0,
		  0);
   evas_object_geometry_get(it1->base, &x, &y, &w, &h);
   evas_object_geometry_get(it2->base, &x_, &y_, &w_, &h_);
   wd->animating++;
   move_object_with_animation(it1->base, x, y, w, h, x_, y_, w_, h_, 0.25,
			       move_evas_map, item_exchange_animation_cb, wd);
   wd->animating++;
   move_object_with_animation(it2->base, x_, y_, w_, h_, x, y, w, h, 0.25,
			       move_evas_map, item_exchange_animation_cb, wd);
   elm_table_unpack(wd->box, it1->base);
   elm_table_unpack(wd->box, it2->base);
   order = it1->order;
   it1->order = it2->order;
   it2->order = order;
   elm_table_pack(wd->box, it1->base, it1->order - 1, 0, 1, 1);
   elm_table_pack(wd->box, it2->base, it2->order - 1, 0, 1, 1);
}

static void
item_change_animation_cb(void *data, Evas_Object * obj) 
{
   Evas_Coord x, y, w, h;
   Widget_Data * wd = (Widget_Data *) data;
   wd->animating--;
   if (wd->animating < 0)
     {
	printf("animation error\n");
	wd->animating = 0;
     }
   evas_object_map_enable_set(obj, EINA_FALSE);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   set_evas_map(obj, x, y, 0, 0);
   evas_object_move(obj, -1000, -1000);
   evas_object_geometry_get(wd->moving_item->base, &x, &y, &w, &h);
   evas_object_move(wd->moving_obj, -1000, -1000);
   evas_object_del(wd->moving_obj);
}

static void
item_change_in_bar(Elm_Controlbar_Item * it) 
{
   Evas_Coord x, y, w, h;
   Evas_Coord x_, y_, w_, h_;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (wd == NULL)
      return;
   if (wd->moving_item == NULL)
      return;
   evas_object_geometry_get(wd->moving_item->edit, &x, &y, &w, &h);
   set_evas_map(wd->moving_obj, x, y, w, h);
   elm_table_unpack(wd->box, it->base);
   wd->moving_item->order = it->order;
   it->order = 0;
   elm_table_pack(wd->box, wd->moving_item->base, wd->moving_item->order - 1,
		    0, 1, 1);
   evas_object_show(wd->moving_item->base);
   evas_render_updates_free(evas_render_updates
			      (evas_object_evas_get(wd->moving_item->base)));
   evas_object_geometry_get(it->base, &x, &y, &w, &h);
   evas_object_geometry_get(it->edit, &x_, &y_, &w_, &h_);
   wd->animating++;
   move_object_with_animation(it->base, x, y, w, h, x_, y_, w_, h_, 0.25,
			       move_evas_map, item_change_animation_cb, wd);
   evas_object_geometry_get(wd->moving_item->base, &x, &y, &w, &h);
   set_evas_map(wd->moving_item->base, x, y, w, h);
}

static Eina_Bool 
hide_selected_box(void *data)
{
   Evas_Object *selected_box = (Evas_Object *)data;

   evas_object_hide(selected_box);

   return ECORE_CALLBACK_CANCEL;
}

static void
object_color_set(Evas_Object *ly, const char *color_part, const char *obj_part)
{
	Evas_Object *color;
	int r, g, b, a;

	color =
		 (Evas_Object *) edje_object_part_object_get(_EDJ(ly), color_part);
	if (color)
	   evas_object_color_get(color, &r, &g, &b, &a);
	color =
	   edje_object_part_swallow_get(_EDJ(ly), obj_part);
	evas_object_color_set(color, r, g, b, a);
}

   static void
_end_selected_box(void *data, Evas_Object *obj)
{
   Widget_Data * wd = (Widget_Data *)data;
   int rand;

   edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,state,selected", "elm");
   // item animation effect
   if(wd->selected_animation)
     {
	srand(time(NULL));
	rand = random()%3;

	switch(rand)
	  {
	   case 0: 
	      edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,animation,pop", "elm");
	      break;
	   case 1:
	      edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,animation,vibration", "elm");
	      break;
	   case 2:
	      edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,animation,jump", "elm");
	      break;
	   default:
	      break;
	  }
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

	from = (Evas_Object *)edje_object_part_object_get(_EDJ(fit->base), "bg_img");
	evas_object_geometry_get(from, &fx, &fy, &fw, &fh);

	to = (Evas_Object *)edje_object_part_object_get(_EDJ(tit->base), "bg_img");
	evas_object_geometry_get(to, &tx, &ty, &tw, &th);

	edje_object_signal_emit(_EDJ(wd->pre_item->base), "elm,state,unselected", "elm");
	edje_object_signal_emit(_EDJ(wd->cur_item->base), "elm,state,unselected", "elm");

	wd->animating++;
	move_object_with_animation(wd->selected_box, fx, fy, fw, fh, tx, ty, tw, th,
				    0.3, move_evas_object, _end_selected_box, wd);
}

static void
end_selected_box(void *data, Evas_Object *obj)
{
   Widget_Data * wd = (Widget_Data *)data;

   if(wd->pre_item) evas_object_hide(wd->pre_item->view);

   elm_layout_content_set(wd->view, "elm.swallow.view", obj);
   
   evas_object_show(obj);
}

static void
view_animation_push(Widget_Data *wd, Evas_Object *pre, Evas_Object *cur, void *data)
{
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(pre, &x, &y, &w, &h);
   move_object_with_animation(pre, x, y, w, h, x+20, y, w, h, 0.5, move_fade_out_object, NULL, NULL);
   move_object_with_animation(cur, x+120, y, w, h, x, y, w, h, 0.5, move_fade_in_object, end_selected_box, wd);
}

static void
view_animation_down(Widget_Data *wd, Evas_Object *pre, Evas_Object *cur, void *data)
{
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(pre, &x, &y, &w, &h);

   move_object_with_animation(cur, x, h, w, h, x, h, w, h, 0.5, move_evas_object, end_selected_box, wd);
   evas_object_raise(pre);
   move_object_with_animation(pre, x, y, w, h, x, h, w, 0, 0.5, move_evas_map, NULL, NULL);
}

static void 
selected_box(Elm_Controlbar_Item * it) 
{
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
	evas_object_smart_callback_call(it->obj, "view,change,before", it);
	//object_color_set(it->base, "elm.tabbar.selected.color", "elm.swallow.icon");
	   
	   if(fit != NULL && fit != it)
		   move_selected_box(wd, fit, it);
	   else 
		   edje_object_signal_emit(_EDJ(it->base), "elm,state,selected", "elm");

/*
	   if(fit != NULL && fit != it)
	     {
		//view_animation_down(wd, fit->view, it->view, NULL);
		view_animation_push(wd, fit->view, it->view, NULL);
		//evas_object_hide(content);

//		evas_object_geometry_get(fit->view, &x, &y, &w, &h);
	//	if(fit->order > it->order)
	//	  {
//		     move_object_with_animation(fit->view, x, y, w, h, x+20, y, w, h, 0.5, move_fade_out_object, NULL, NULL);
//		     move_object_with_animation(it->view, x+120, y, w, h, x, y, w, h, 0.5, move_fade_in_object, end_selected_box, wd);
	//	  }
	//	else
	//	  {
	//	     move_object_with_animation(fit->view, x, y, w, h, x-120, y, w, h, 1.5, move_fade_out_object, NULL, NULL);
	//	     move_object_with_animation(it->view, x-120, y, w, h, x, y, w, h, 1.5, move_fade_in_object, end_selected_box, wd);
	//	  }
	     }
	   else
	     {
		end_selected_box(wd, it->view);
	     }
*/	    
	   if(wd->pre_item) evas_object_hide(wd->pre_item->view);
	   elm_layout_content_set(wd->view, "elm.swallow.view", it->view);

//	   elm_layout_content_set(wd->view, "elm.swallow.view", it->view);
	   //edje_object_part_swallow(wd->view, "elm.swallow.view", it->view);
//	   evas_object_show(it->view);	   

   }else if(it->style == TOOLBAR){
	if (it->func)
	  it->func(it->data, it->obj, it);
	//object_color_set(it->base, "elm.item.default.color", "elm.swallow.icon");
	edje_object_signal_emit(_EDJ(it->base), "elm,state,text_unselected", "elm");
   }

   evas_object_smart_callback_call(it->obj, "clicked", it);
}

static void
unpressed_box_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;
   Evas_Event_Mouse_Up * ev = event_info;
   Evas_Coord x, y, w, h;
   Elm_Controlbar_Item * item;

   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_UP, unpressed_box_cb);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
	if (item->style == TABBAR)
	  {
	     if(item->selected)
	       {
		  //object_color_set(item->base, "elm.tabbar.selected.color", "elm.swallow.icon");
		  edje_object_signal_emit(_EDJ(item->base), "elm,state,selected", "elm");
	       }
	     else
	       {
		  //object_color_set(item->base, "elm.item.default.color", "elm.swallow.icon");
		  edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
	       }
	  }
	else if (item->style == TOOLBAR)
	  {
	     //object_color_set(item->base, "elm.item.default.color", "elm.swallow.icon");
	     edje_object_signal_emit(_EDJ(item->base), "elm,state,text_unselected", "elm");
	  }
     }

   evas_object_geometry_get(wd->pre_item->base, &x, &y, &w, &h);
   if(ev->output.x > x && ev->output.x < x+w && ev->output.y > y && ev->output.y < y+h)
     {
	selected_box(wd->pre_item);
     }

   return;
}

static int
pressed_box(Elm_Controlbar_Item * it) 
{
   Widget_Data * wd = elm_widget_data_get(it->obj);
   int check = 0;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   
   if(wd->animating) return EXIT_FAILURE;

   if(it->disable) return EXIT_FAILURE;

   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (it == item)
	{
	   if (it->style == TABBAR)
	     {

		//object_color_set(it->base, "elm.tabbar.selected.color", "elm.swallow.icon");
		edje_object_signal_emit(_EDJ(it->base), "elm,state,pressed",
					  "elm");
	     }
	   else if (it->style == TOOLBAR)
	     {
	
		//object_color_set(it->base, "elm.toolbar.pressed.color", "elm.swallow.icon");
		edje_object_signal_emit(_EDJ(it->base), "elm,state,text_selected",
					  "elm");
		}
	   evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_UP, unpressed_box_cb, (void *)wd);
	 
	   check = EINA_TRUE;
	}
   }
   if (!check)
      return EXIT_FAILURE;

   wd->pre_item = it;

   return EXIT_SUCCESS;
}

static Evas_Object *
create_item_label(Evas_Object *obj, Elm_Controlbar_Item * it)
{
   Evas_Object *label;
   label = elm_label_add(obj);
   elm_object_style_set(label, "controlbar");
   elm_label_label_set(label, it->text);
   elm_label_text_align_set(label, "center");
   elm_label_text_color_set(label, 255, 255, 255, 255);
   elm_label_line_wrap_set(label, EINA_TRUE);
   elm_label_ellipsis_set(label, EINA_TRUE);
   elm_label_wrap_mode_set(label, 1);

   elm_layout_content_set(obj, "elm.text", label);

   return label;
}

static Evas_Object *
create_item_icon(Evas_Object *obj, Elm_Controlbar_Item * it)
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
   edje_object_part_swallow(_EDJ(obj), "elm.swallow.icon", icon);
   
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
     {
	  *label = create_item_label(*item, it);
     }
   if (it->icon_path)
     {
	  *icon = create_item_icon(*item, it);
     }
   if (*label && *icon)
     {
	edje_object_signal_emit(_EDJ(*item), "elm,state,icon_text", "elm");
	elm_label_line_wrap_set(*label, EINA_FALSE);
	elm_label_wrap_mode_set(*label, 0);
     }
   
   return obj;
}

static void
edit_item_down_end_cb(void *data, Evas_Object * obj) 
{
   Widget_Data * wd = (Widget_Data *) data;
   wd->animating--;
   if (wd->animating < 0)
     {
	printf("animation error\n");
	wd->animating = 0;
     }
}

static void
edit_item_return_cb(void *data, Evas_Object * obj) 
{
   Evas_Coord x, y, w, h;
   Widget_Data * wd = (Widget_Data *) data;
   evas_object_geometry_get(wd->moving_item->edit, &x, &y, &w, &h);
   set_evas_map(obj, x, y, 0, 0);

   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_UP, edit_item_up_cb);
   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_MOVE, edit_item_move_cb);

   evas_object_data_set(wd->moving_obj, "returning", 0);
   wd->animating--;
   if (wd->animating < 0)
     {
	printf("animation error\n");
	wd->animating = 0;
     }
}

static void 
edit_item_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move * ev = event_info;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Evas_Coord x, y, w, h;
   Widget_Data * wd = (Widget_Data *) data;
   if (wd->animating)
      return;
   evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
   w *= 2.0;
   h *= 2.0;
   x = ev->cur.output.x - w / 2;
   y = ev->cur.output.y - h;
   set_evas_map(wd->moving_obj, x, y, w, h);

   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (wd->moving_item->edit == item->edit || item->style == OBJECT)
	 continue;
      evas_object_geometry_get(item->base, &x, &y, &w, &h);
      if (ev->cur.output.x > x && ev->cur.output.x < x + w && ev->cur.output.y > y && ev->cur.output.y < y + h
	   && item->editable)
	{
	   edje_object_signal_emit(_EDJ(item->base), "elm,state,show,glow",
				    "elm");
	}
      else
	{
	   edje_object_signal_emit(_EDJ(item->base), "elm,state,hide,glow",
				    "elm");
	}
   }
   return;
}

static void 
edit_item_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up * ev = event_info;
   Evas_Coord x, y, w, h;
   Evas_Coord x_, y_, w_, h_;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   Widget_Data * wd = (Widget_Data *) data;
   if (wd->moving_obj)
      if ((int)evas_object_data_get(wd->moving_obj, "returning") == 1)
	 return;
   evas_object_color_set(wd->moving_item->edit_item, 255, 255, 255, 255);
   
      // check which change or not
      EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (wd->moving_item->edit == item->edit)
	 continue;
      if (item->order <= 0)
	 continue;
      evas_object_geometry_get(item->base, &x, &y, &w, &h);
      if (ev->output.x > x && ev->output.x < x + w && ev->output.y > y && ev->output.y < y + h
	   && item->style != OBJECT && item->editable)
	{
	   edje_object_signal_emit(_EDJ(item->base), "elm,state,hide,glow",
				    "elm");
	   break;
	}
   }
   if (item != NULL)
     {
	if (wd->moving_item->order > 0)
	  {
	     item_exchange_in_bar(wd->moving_item, item);
	  }
	else
	  {
	     item_change_in_bar(item);
	  }
		evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_UP, edit_item_up_cb);
		evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_MOVE, edit_item_move_cb);
     }
   else
     {
	
	   // return moving object to original location
	   evas_object_geometry_get(wd->moving_item->edit, &x_, &y_, &w_,
				    &h_);
	evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
	w *= 2.0;
	h *= 2.0;
	x = ev->output.x - w / 2;
	y = ev->output.y - h;
	evas_object_data_set(wd->moving_obj, "returning", (void *)1);
	wd->animating++;
	move_object_with_animation(wd->moving_obj, x, y, w, h, x_, y_, w_, h_,
				    0.25, move_evas_map, edit_item_return_cb, wd);
     } 
   return;
}

static void
edit_item_down_cb(void *data, Evas * evas, Evas_Object * obj,
		  void *event_info) 
{
   Evas_Event_Mouse_Down * ev = event_info;
   Evas_Coord x, y, w, h;
   Evas_Coord x_, y_, w_, h_;
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   Evas_Object * color;
   int r, g, b, a;

   if (wd->animating)
      return;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->edit == obj)
	 break;
   }
   if (item == NULL)
      return;
   if (!item->editable)
      return;
   
   evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_UP, edit_item_up_cb, (void *)wd);
   evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_MOVE, edit_item_move_cb, (void *)wd);

   wd->moving_item = item;
   color =
      (Evas_Object *)
      edje_object_part_object_get(_EDJ(wd->moving_item->edit),
				  "elm.item.uneditable.color");
   if (color)
      evas_object_color_get(color, &r, &g, &b, &a);
   evas_object_color_set(item->edit_item, r, g, b, a);
   if (wd->moving_obj)
      evas_object_del(wd->moving_obj);
   wd->moving_obj = NULL;
   wd->moving_obj = create_item_layout(obj, item, &(item->base_item), &(item->label), &(item->icon));
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(wd->moving_obj, -1000, -1000);
   evas_object_resize(wd->moving_obj, w, h);
   evas_object_show(wd->moving_obj);
   w_ = w * 2.0;
   h_ = h * 2.0;
   x_ = ev->output.x - w_ / 2;
   y_ = ev->output.y - h_;
   wd->animating++;
   move_object_with_animation(wd->moving_obj, x, y, w, h, x_, y_, w_, h_, 0.1,
			       move_evas_map, edit_item_down_end_cb, wd);
}

static void
bar_item_move_end_cb(void *data, Evas_Object * obj) 
{
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->base == obj)
	 break;
   }
   wd->animating--;
   if (wd->animating < 0)
     {
	printf("animation error\n");
	wd->animating = 0;
     }
   evas_object_data_set(obj, "animating", 0);
   evas_object_map_enable_set(obj, EINA_FALSE);
}
	
static Eina_Bool
bar_item_animation_end_check(void *data) 
{
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   Widget_Data * wd = (Widget_Data *) data;
   if (wd->animating)
      return EXIT_FAILURE;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->base == wd->moving_obj)
	 break;
   }
   if (item == NULL)
     {
	printf("item is NULL\n");
     }
   else
     {
	item->order = wd->empty_num;
	wd->empty_num = 0;
	wd->moving_obj = NULL;
     }
   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_UP, bar_item_up_cb);
   evas_object_event_callback_del(wd->event_box, EVAS_CALLBACK_MOUSE_MOVE, bar_item_move_cb);
   return ECORE_CALLBACK_CANCEL;
}

static void 
bar_item_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move * ev = event_info;
   const Eina_List *l;
   Elm_Controlbar_Item * item, *it;
   Widget_Data * wd = (Widget_Data *) data;
   Evas_Coord x, y, w, h, x_, y_, w_, h_;
   int tmp;

   if (wd->moving_obj == NULL)
     {
	printf("%s : moving_obj is NULL\n", __func__);
	return;
     }
   evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
   x = ev->cur.output.x - w / 2;
   set_evas_map(wd->moving_obj, x, y, w, h);
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->base == wd->moving_obj)
	{
	   it = item;
	   continue;
	}
      if ((int)evas_object_data_get(item->base, "animating") == 1)
	 continue;
      evas_object_geometry_get(item->base, &x, &y, &w, &h);
      if (ev->cur.output.x > x && ev->cur.output.x < x + w && item->editable)
	{
	   break;
	}
   }
   if (item)
     {
	evas_object_geometry_get(wd->moving_obj, &x_, &y_, &w_, &h_);
	evas_object_move(wd->moving_obj, x, y);
	tmp = wd->empty_num;
	wd->empty_num = item->order;
	item->order = tmp;
	elm_table_unpack(wd->box, item->base);
	elm_table_unpack(wd->box, wd->moving_obj);
	elm_table_pack(wd->box, item->base, item->order - 1, 0, 1, 1);
	elm_table_pack(wd->box, wd->moving_obj, wd->empty_num - 1, 0, 1, 1);
	wd->animating++;
	evas_object_data_set(item->base, "animating", (void *)1);
	move_object_with_animation(item->base, x, y, w, h, x_, y_, w_, h_,
				    0.25, move_evas_map, bar_item_move_end_cb, wd);
     }
   return;
}

static void 
bar_item_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up * ev = event_info;
   Evas_Coord tx, x, y, w, h;
   Widget_Data * wd = (Widget_Data *) data;
   evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
   tx = ev->output.x - w / 2;
   wd->animating++;
   evas_object_data_set(wd->moving_obj, "animating", (void *)1);
   move_object_with_animation(wd->moving_obj, tx, y, w, h, x, y, w, h, 0.25,
			       move_evas_map, bar_item_move_end_cb, wd);
   ecore_timer_add(0.1, bar_item_animation_end_check, wd);
   return;
}

static void
bar_item_down_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info) 
{
   Widget_Data * wd = (Widget_Data *) data;
   const Eina_List *l;
   Elm_Controlbar_Item * item;
   if (wd->animating)
      return;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->base == obj)
	 break;
   }
   if (item == NULL)
      return;
   if (wd->edit_mode)
     {
	if (!item->editable)
	   return;

	wd->moving_obj = obj;
	wd->empty_num = item->order;
	evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_UP, bar_item_up_cb, (void *)wd);
    evas_object_event_callback_add(wd->event_box, EVAS_CALLBACK_MOUSE_MOVE, bar_item_move_cb, (void *)wd);
     }
   else
     {
	pressed_box(item);
     }
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
   if (it == NULL)
      return NULL;
   it->obj = obj;
   it->text = eina_stringshare_add(label);
   it->icon_path = icon_path;
   it->selected = EINA_FALSE;
   it->editable = EINA_TRUE;
   it->badge = 0;
   it->sel = 1;
   it->view = view;
   it->style = TABBAR;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
				   bar_item_down_cb, wd);
   //object_color_set(it->base, "elm.item.default.color", "elm.swallow.icon");
   evas_object_show(it->base);
   it->edit = create_item_layout(wd->edje, it, &(it->edit_item), &(it->edit_label), &(it->edit_icon));
   evas_object_event_callback_add(it->edit, EVAS_CALLBACK_MOUSE_DOWN,
				   edit_item_down_cb, wd);
   evas_object_show(it->edit);

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
   if (it == NULL)
      return NULL;
   it->obj = obj;
   it->text = eina_stringshare_add(label);
   it->icon_path = icon_path;
   it->selected = EINA_FALSE;
   it->editable = EINA_TRUE;
   it->badge = 0;
   it->sel = 1;
   it->func = func;
   it->data = data;
   it->style = TOOLBAR;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
				   bar_item_down_cb, wd);
   //edje_object_signal_callback_add(_EDJ(it->base), "elm,action,click", "elm",
	//			    clicked_box_cb, wd);
   //object_color_set(it->base, "elm.item.default.color", "elm.swallow.icon");
   evas_object_show(it->base);
   it->edit = create_item_layout(wd->edje, it, &(it->edit_item), &(it->edit_label), &(it->edit_icon));
   evas_object_event_callback_add(it->edit, EVAS_CALLBACK_MOUSE_DOWN,
				   edit_item_down_cb, wd);
   evas_object_show(it->edit);
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
   if (it == NULL)
      return NULL;
   it->obj = obj;
   it->badge = 0;
   it->sel = sel;
   it->style = OBJECT;
   it->base = obj_item;
   return it;
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

   int edit = 1;

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
	   edit = i;
	   it->order = mit->order;
	}
      if (check)
	{
	   if(item->order > 0){
	      elm_table_unpack(wd->box, item->base);
	      item->order += it->sel;
	      elm_table_pack(wd->box, item->base, item->order - 1, 0, item->sel, 1);
	   }
	   if (item->style != OBJECT && it->style != OBJECT)
	     {
		elm_table_unpack(wd->edit_table, item->edit);
		elm_table_pack(wd->edit_table, item->edit,
				i % EDIT_ROW_NUM, i / EDIT_ROW_NUM, 1, 1);
	     }
	}
      if (item->style != OBJECT)
	 i++;
      if(item->order > 0) order += item->sel;
   }
   if (!check)
     {
	edit = i;
	if(bar)
	  it->order = order;
	else
	  it->order = 0;
     }
   wd->num++;
   if(bar) elm_table_pack(wd->box, it->base, it->order - 1, 0, it->sel, 1);
   else evas_object_hide(it->base);
   if (it->style != OBJECT)
      elm_table_pack(wd->edit_table, it->edit, (edit - 1) % EDIT_ROW_NUM,
		     (edit - 1) / EDIT_ROW_NUM, 1, 1);
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
   if(lit == NULL) return;
   
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

   list = elm_list_add( wd->object );
   elm_list_horizontal_mode_set( list, ELM_LIST_COMPRESS );

   EINA_LIST_FOREACH(wd->items, l, item)
     {
	if(item->order <= 0)
	  {
	     evas_object_color_set(item->icon, 0, 0, 0, 255);
	     elm_list_item_append(list, item->text, item->icon, NULL, list_clicked, item); 
	  }
     }

   elm_list_go( list );

   return list;
}

static Elm_Controlbar_Item *
create_more_item(Widget_Data *wd, int style)
{
   Elm_Controlbar_Item * it;

   it = ELM_NEW(Elm_Controlbar_Item);
   if (it == NULL)
      return NULL;
   it->obj = wd->object;
   it->text = eina_stringshare_add("more");
   it->icon_path = CONTROLBAR_SYSTEM_ICON_MORE;
   it->selected = EINA_FALSE;
   it->badge = 0;
   it->sel = 1;
   it->view = create_more_view(wd);
   it->style = style;
   it->base = create_item_layout(wd->edje, it, &(it->base_item), &(it->label), &(it->icon));
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
				   bar_item_down_cb, wd);
   //object_color_set(it->base, "elm.item.default.color", "elm.swallow.icon");
   evas_object_show(it->base);
   it->edit = create_item_layout(wd->edje, it, &(it->edit_item), &(it->edit_label), &(it->edit_icon));
   evas_object_event_callback_add(it->edit, EVAS_CALLBACK_MOUSE_DOWN,
				   edit_item_down_cb, wd);
   evas_object_show(it->edit);

   set_items_position(it->obj, it, NULL, EINA_TRUE);

   wd->items = eina_list_append(wd->items, it);

   wd->more_item = it;

   elm_controlbar_item_editable_set(it, EINA_FALSE);
   
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
   Evas_Object * obj = NULL;
   Evas_Object * bg = NULL;
   Widget_Data * wd = NULL;
   Evas_Coord x, y, w, h;
   Evas_Object * r_button;
   wd = ELM_NEW(Widget_Data);
   wd->evas = evas_object_evas_get(parent);
   if (wd->evas == NULL)
      return NULL;
   obj = elm_widget_add(wd->evas);
   if (obj == NULL)
      return NULL;
   elm_widget_type_set(obj, "controlbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   
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
   wd->edit_mode = EINA_FALSE;
   wd->auto_align = EINA_FALSE;
   wd->init_animation = EINA_FALSE;
   wd->selected_animation = EINA_FALSE;
   wd->view = elm_layout_add(wd->parent); //edje_object_add(wd->evas);
   elm_layout_theme_set(wd->view, "controlbar", "view", "default");
   //_elm_theme_object_set(obj, wd->view, "controlbar", "view", "default");
   if (wd->view == NULL)
     {
	printf("Cannot load bg edj\n");
	return NULL;
     }
   evas_object_show(wd->view);

   // edit box
   wd->edit_box = edje_object_add(wd->evas);
   _elm_theme_object_set(obj, wd->edit_box, "controlbar", "edit_box",
	 "default");
   if (wd->edit_box == NULL)
     {
	printf("Cannot load bg edj\n");
	return NULL;
     }
   evas_object_show(wd->edit_box);

   //instead of navigationbar
   /*    r_button = elm_button_add(wd->edit_box);
	 elm_button_label_set(r_button, "Done");
	 evas_object_smart_callback_add(r_button, "clicked", done_button_cb, wd);
	 edje_object_part_swallow(wd->edit_box, "elm.swallow.navigation", r_button);
    */
   // navigationbar will contribution. but not yet
   wd->navigation = elm_navigationbar_add(wd->edit_box);
   r_button = elm_button_add(wd->navigation);
   elm_button_label_set(r_button, "Done");
   evas_object_smart_callback_add(r_button, "clicked", done_button_cb, wd);
   elm_navigationbar_push(wd->navigation, "Configure", NULL, r_button, NULL, NULL);
   edje_object_part_swallow(wd->edit_box, "elm.swallow.navigation", wd->navigation);

   wd->edit_table = elm_table_add(wd->edit_box);
   elm_table_homogenous_set(wd->edit_table, EINA_TRUE);
   edje_object_part_swallow(wd->edit_box, "elm.swallow.table", wd->edit_table);

   /* load background edj */ 
   wd->edje = edje_object_add(wd->evas);
   _elm_theme_object_set(obj, wd->edje, "controlbar", "base", "default");
   if (wd->edje == NULL)
     {
	printf("Cannot load base edj\n");
	return NULL;
     }
   evas_object_show(wd->edje);
   
   wd->bg = edje_object_add(wd->evas);
   _elm_theme_object_set(obj, wd->bg, "controlbar", "background", "default");
   if (wd->bg == NULL)
     {
	printf("Cannot load bg edj\n");
	return NULL;
     }
   edje_object_part_swallow(wd->edje, "bg_image", wd->bg);

   // initialization
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE,
				     _controlbar_object_resize, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOVE,
				   _controlbar_object_move, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_SHOW,
				   _controlbar_object_show, obj);
   evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_HIDE,
				   _controlbar_object_hide, obj);
   bg = (Evas_Object *)edje_object_part_object_get(wd->edje, "bg_image");
   evas_object_event_callback_add(bg, EVAS_CALLBACK_MOVE, _controlbar_object_move, obj);
   evas_object_event_callback_add(bg, EVAS_CALLBACK_RESIZE, _controlbar_object_resize, obj);

   wd->selected_box = edje_object_add(wd->evas);
   _elm_theme_object_set(obj, wd->selected_box, "controlbar", "item_bg_move", "default");
   //edje_object_part_swallow(wd->edje, "elm.swallow.panel", wd->selected_box);
   evas_object_hide(wd->selected_box);

      // items container
   wd->box = elm_table_add(wd->edje);
   elm_table_homogenous_set(wd->box, EINA_TRUE);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND,
				     EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   edje_object_part_swallow(wd->edje, "elm.swallow.items", wd->box);
   evas_object_show(wd->box);
   
   wd->event_box = evas_object_rectangle_add(wd->evas);
   evas_object_color_set(wd->event_box, 255, 255, 255, 0);
   evas_object_repeat_events_set(wd->event_box, EINA_TRUE);
   evas_object_show(wd->event_box);

      //FIXME
      //      evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_smart_member_add(wd->view, obj);
   evas_object_smart_member_add(wd->edit_box, obj);
   elm_widget_resize_object_set(obj, wd->edje);
   evas_object_smart_member_add(wd->selected_box, obj);
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
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Widget_Data * wd;
   it = create_tab_item(obj, icon_path, label, view);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
	if(!wd->more_item) {
	     lit = elm_controlbar_last_item_get(obj);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
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
   if (wd->num == 1)
      selected_box(it);

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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   it = create_tab_item(obj, icon_path, label, view);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
	if(!wd->more_item) {
	     lit = elm_controlbar_last_item_get(obj);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
	     create_more_item(wd, TABBAR);
	}
	lit = elm_controlbar_item_prev(wd->more_item);
	elm_controlbar_item_visible_set(lit, EINA_FALSE);
	set_items_position(obj, it, item, EINA_TRUE);
   }
   else{
	set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_prepend(wd->items, it);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));
   if (wd->num == 1)
      selected_box(it);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   if (!before)
      return NULL;
   it = create_tab_item(obj, icon_path, label, view);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
	if(!wd->more_item) 
	  {
	     lit = elm_controlbar_last_item_get(obj);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
	     create_more_item(wd, TABBAR);
	  }
	if(before->order > 0)
	  {
	     lit = elm_controlbar_item_prev(wd->more_item);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
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
   if (wd->num == 1)
      selected_box(it);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * lit;
   Elm_Controlbar_Item * item;
   if (!after)
      return NULL;
   it = create_tab_item(obj, icon_path, label, view);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   if(check_bar_item_number(wd) >= 5 && wd->auto_align){
	if(!wd->more_item) 
	  {
	     lit = elm_controlbar_last_item_get(obj);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
	     create_more_item(wd, TABBAR);
	  }
	if(item->order > 0)
	  {
	     lit = elm_controlbar_item_prev(wd->more_item);
	     elm_controlbar_item_visible_set(lit, EINA_FALSE);
	     set_items_position(obj, it, item, EINA_TRUE);
	  }
	else
	  {
	     set_items_position(obj, it, item, EINA_FALSE);
	  }
   }
   else{
	set_items_position(obj, it, item, EINA_TRUE);
   }
   wd->items = eina_list_append_relative(wd->items, it, after);
   if(wd->more_item)
     elm_controlbar_item_view_set(wd->more_item, create_more_view(wd));
   if (wd->num == 1)
      selected_box(it);
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
   Elm_Controlbar_Item * it;
   Widget_Data * wd;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   set_items_position(obj, it, NULL, EINA_TRUE);
   wd->items = eina_list_append(wd->items, it);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   item = eina_list_data_get(wd->items);
   set_items_position(obj, it, item, EINA_TRUE);
   wd->items = eina_list_prepend(wd->items, it);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   if (!before)
      return NULL;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   set_items_position(obj, it, before, EINA_TRUE);
   wd->items = eina_list_prepend_relative(wd->items, it, before);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   if (!after)
      return NULL;
   it = create_tool_item(obj, icon_path, label, func, data);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   set_items_position(obj, it, item, EINA_TRUE);
   wd->items = eina_list_append_relative(wd->items, it, after);
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   it = create_object_item(obj, obj_item, sel);
   if (it == NULL)
      return NULL;
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   it = create_object_item(obj, obj_item, sel);
   if (it == NULL)
      return NULL;
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   if (!before)
      return NULL;
   it = create_object_item(obj, obj_item, sel);
   if (it == NULL)
      return NULL;
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
   Widget_Data * wd;
   Elm_Controlbar_Item * it;
   Elm_Controlbar_Item * item;
   if (!after)
      return NULL;
   it = create_object_item(obj, obj_item, sel);
   if (it == NULL)
      return NULL;
   wd = elm_widget_data_get(obj);
   item = elm_controlbar_item_next(after);
   set_items_position(obj, it, item, EINA_TRUE);
   wd->items = eina_list_append_relative(wd->items, it, after);
   _sizing_eval(obj);
   return it;
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
   Evas_Object * obj;
   Widget_Data * wd;
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   int check = 0;

   int i = 1;

   int sel = 1;

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
   
      // delete base item
      if (it->order > 0)
     {
	elm_table_unpack(wd->box, it->base);
	sel = it->sel;
	EINA_LIST_FOREACH(wd->items, l, item)
	{
	   if (it != item)
	     {
		if (item->order > it->order)
		  {
		     elm_table_unpack(wd->box, item->base);
		     item->order -= sel;
		     elm_table_pack(wd->box, item->base, item->order - 1, 0,
				     item->sel, 1);
		  }
	     }
	   if (it == item)
	     {
		check = 1;
	     }
	}
     }
   
      // delete edit item
      check = 0;
   if (it->edit_item != NULL)
     {
	elm_table_unpack(wd->edit_table, it->edit_item);
	EINA_LIST_FOREACH(wd->items, l, item)
	{
	   if (check)
	     {
		if (item->edit_item != NULL)
		  {
		     elm_table_unpack(wd->edit_table, item->edit_item);
		     elm_table_pack(wd->edit_table, item->edit_item,
				     (i - 1) % 4, (i - 1) / 4, 1, 1);
		  }
	     }
	   if (it == item && item->style != OBJECT)
	     {
		check = 1;
		i--;
	     }
	   if (item->style != OBJECT)
	      i++;
	}
     }
   
      // delete item in list
   if (it->text)
      eina_stringshare_del(it->text);
   if (it->label)
      evas_object_del(it->label);
   if (it->icon)
      evas_object_del(it->icon);
   if (it->base)
     {
	if (it->style != OBJECT)
	   evas_object_del(it->base);
	
	else
	   evas_object_hide(it->base);
     }
   if (it->view)
     {
	//edje_object_part_unswallow(wd->view, it->view);
	if(it->selected) elm_layout_content_unset(wd->view, "elm.swallow.view");
	evas_object_hide(it->view);
     }
   if (it->edit_item)
      evas_object_del(it->edit_item);
   wd->items = eina_list_remove(wd->items, it);
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
   if (it->obj == NULL)
      return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (wd == NULL)
      return;
   if (!wd->edit_mode)
     {
	selected_box(it);
     }
}

/**
 * Get the icon of item
 *
 * @param	it The item of controlbar
 * @return The icon object
 *
 * @ingroup Controlbar
 */ 
   EAPI Evas_Object * elm_controlbar_item_icon_get(Elm_Controlbar_Item * it) 
{
   return it->icon;
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
   return it->text;
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
   if (!it->base)
      return;
   it->text = eina_stringshare_add(label);
   create_item_label(it->base, it);
   create_item_label(it->edit_item, it);
   if(it->icon){
	edje_object_signal_emit(_EDJ(it->base),
					 "elm,state,icon_text", "elm");
	elm_label_line_wrap_set(it->label, EINA_FALSE);
	elm_label_wrap_mode_set(it->label, 0);
   }
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
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
      return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (item->selected)
	 return item;
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
   if (obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
      return NULL;
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
   if (obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
      return NULL;
   return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the items
 *
 * @param	obj The controlbar object
 * @return  The list of the items
 *
 * @ingroup Controlbar
 */ 
   EAPI Eina_List * elm_controlbar_items_get(Evas_Object * obj) 
{
   if (obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(obj);
   if (!wd || !wd->items)
      return NULL;
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
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (it->obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items)
      return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (it == item)
	{
	   l = eina_list_prev(l);
	   if (!l)
	      return NULL;
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
   const Eina_List *l;

   Elm_Controlbar_Item * item;
   if (it->obj == NULL)
      return NULL;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items)
      return NULL;
   EINA_LIST_FOREACH(wd->items, l, item)
   {
      if (it == item)
	{
	   l = eina_list_next(l);
	   if (!l)
	      return NULL;
	   return eina_list_data_get(l);
	}
   }
   return NULL;
}

   EAPI void
elm_controlbar_edit_start(Evas_Object * obj) 
{
   printf("\n==================================\n");
   printf("%s\n", __func__);
   printf("==================================\n");
   printf("This API is just for test.\n");
   printf("Please don't use it!!\n");
   printf("Thank you.\n");
   printf("==================================\n");

   Widget_Data * wd;
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
   edje_object_signal_emit(wd->edit_box, "elm,state,show,edit_box", "elm");
   wd->edit_mode = EINA_TRUE;
}

/**
 * Set the visible status of item in bar
 *
 * @param	it The item of controlbar
 * @param	bar true or false
 *
 * @ingroup Controlbar
 */ 
EAPI void
elm_controlbar_item_visible_set(Elm_Controlbar_Item * it, Eina_Bool visible) 
{
   Elm_Controlbar_Item *item;
   Eina_Bool check = EINA_TRUE;

   if(!it) return;
   if (it->obj == NULL)
      return;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd || !wd->items)
      return;
   if (it->order <= 0)
      check = EINA_FALSE;
   if (check == visible)
      return;
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


/**
 * Get the result which or not item is visible in bar
 *
 * @param	it The item of controlbar
 * @return	true or false
 *
 * @ingroup Controlbar
 */ 
EAPI Eina_Bool
elm_controlbar_item_visible_get(Elm_Controlbar_Item * it) 
{
   if(!it) return EINA_FALSE;

   if (it->obj == NULL)
      return EINA_FALSE;
   Widget_Data * wd = elm_widget_data_get(it->obj);
   if (!wd)
      return EINA_FALSE;

   if(it->order <= 0) 
     return EINA_FALSE;

   return EINA_TRUE;
}

   EAPI void
elm_controlbar_item_editable_set(Elm_Controlbar_Item * it, Eina_Bool editable) 
{
   Evas_Object * color;
   int r, g, b, a;

   if(!it) return;

   if(it->editable == editable) return;

   it->editable = editable;
   if(it->editable){

   }else{
      color =
	 (Evas_Object *) edje_object_part_object_get(_EDJ(it->edit_item),
						     "elm.item.uneditable.color");
      if (color)
	 evas_object_color_get(color, &r, &g, &b, &a);
      evas_object_color_set(it->edit_item, r, g, b, a);
   }
}

/**
 * Set item disable
 *
 * @param	it The item of controlbar
 * @param	bar true or false
 *
 * @ingroup Controlbar
 */ 
   EAPI void
elm_controlbar_item_disable_set(Elm_Controlbar_Item * it, Eina_Bool disable) 
{
   Evas_Object * color;
   int r, g, b, a;

   if(!it) return;

   if(it->disable == disable) return;

   it->disable = disable;

   if(it->disable){
	color =
	   (Evas_Object *) edje_object_part_object_get(_EDJ(it->base),
		 "elm.item.disable.color");
	if (color)
	  evas_object_color_get(color, &r, &g, &b, &a);
	evas_object_color_set(it->base_item, r, g, b, a);
   }else{
	color =
	   (Evas_Object *) edje_object_part_object_get(_EDJ(it->base),
		 "elm.item.default.color");
	if (color)
	  evas_object_color_get(color, &r, &g, &b, &a);
	evas_object_color_set(it->base_item, r, g, b, a);
   }
}

   EAPI void
elm_controlbar_view_set(Evas_Object * obj, Evas_Object * view) 
{
   printf("\n==================================\n");
   printf("%s\n", __func__);
   printf("==================================\n");
   printf("This API is just for test.\n");
   printf("Please don't use it!!\n");
   printf("Thank you.\n");
   printf("==================================\n");

   Widget_Data * wd;
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
   wd->view_content = view;
   //edje_object_part_swallow(wd->view, "elm.swallow.view", wd->view_content);
   elm_layout_content_set(wd->view, "elm.swallow.view", wd->view_content);
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
   Widget_Data * wd;
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

   if(wd->mode == mode) return;

   switch(mode)
     {
      case ELM_CONTROLBAR_MODE_DEFAULT: 
	 edje_object_signal_emit(wd->edje, "elm,state,default", "elm");
	 break;
      case ELM_CONTROLBAR_MODE_TRANSLUCENCE: 
	 elm_controlbar_alpha_set(obj, 85);
	 //edje_object_signal_emit(wd->edje, "elm,state,translucence", "elm");
	 break;
      case ELM_CONTROLBAR_MODE_TRANSPARENCY:
	 elm_controlbar_alpha_set(obj, 0);
	 //edje_object_signal_emit(wd->edje, "elm,state,transparency", "elm");
	 break;
      case ELM_CONTROLBAR_MODE_LARGE: 
	 edje_object_signal_emit(wd->edje, "elm,state,large", "elm");
	 break;
      case ELM_CONTROLBAR_MODE_SMALL: 
	 edje_object_signal_emit(wd->edje, "elm,state,small", "elm");
	 break;
      default:
	 break;
     }
   _sizing_eval(obj);
}

/**
 * Set the alpha of the controlbar
 *
 * @param	obj The object of the controlbar
 * @param	alpha The mode of the controlbar (0-100)
 *
 * @ingroup Controlbar
 */ 
   EAPI void
elm_controlbar_alpha_set(Evas_Object *obj, int alpha)
{
   int r, g, b;
   Widget_Data * wd;
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

   if(alpha < 0)
     wd->alpha = 0;
   else if(alpha > 100)
     wd->alpha = 100;
   else
     wd->alpha = alpha;

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
   Widget_Data * wd;
   Elm_Controlbar_Item *item;
   const Eina_List *l;
   int i;
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
	     if (wd->more_item->edit)
	       evas_object_del(wd->more_item->edit);
	     if (wd->more_item->edit_item)
	       evas_object_del(wd->more_item->edit_item);
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

   Widget_Data * wd;
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

   Widget_Data * wd;
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

