/*
 * SLP
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */


/**
 * @addtogroup Tab Tab
 *
 * This is a Tab. It can contain label and icon objects.
 * You can change the location of items.
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

#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))
#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

#define AQUA_THEME_FILE "/usr/share/elementary/themes/aqua.edj"
	
Evas_Object *btn;

// internal data structure of tab object
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas *evas;
	Evas_Object *object;
	Evas_Object *parent;
	Evas_Object *view;
	Evas_Object *edit_box;
	Evas_Object *edit_table;
	Evas_Object *navigation;
	Evas_Object *edje;
	Evas_Object *box;
	Evas_Object *rect;
	Evas_Object *moving_obj;
	Elm_Tab_Item *moving_item;

	Evas_Coord x, y, w, h;

	Eina_Bool edit_mode;

	int empty_num;
	int num;

	Eina_List *items;

	int animating;

	Ecore_Event_Handler *move_event;
	Ecore_Event_Handler *up_event;
	Ecore_Event_Handler *bar_move_event;
	Ecore_Event_Handler *bar_up_event;
};

struct _Elm_Tab_Item
  {
	Evas_Object *obj;
	Evas_Object *base;
	Evas_Object *edit_item;
	Evas_Object *view;
	Evas_Object *icon;
	const char *icon_path;
	const char *label;
	int order;
	int badge;
	Eina_Bool selected : 1;
	Eina_Bool editable : 1;
	Eina_Bool in_bar : 1;
  };

typedef struct _Animation_Data Animation_Data;

struct _Animation_Data
  {
	Evas_Object *obj;
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
	void (*func)(void *data, Evas_Object *obj);
	void *data;
	Ecore_Animator *timer;
  };

///////////////////////////////////////////////////////////////////
//
//  Smart Object basic function
//
////////////////////////////////////////////////////////////////////

static void _tab_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;
	Evas_Coord x, y;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_geometry_get(wd->edje, &x, &y, NULL, NULL);

	wd->x = x;
	wd->y = y;

	evas_object_move(wd->edje, x, y);
//	evas_object_move(wd->box, x, y);
	evas_object_move(wd->view, x, y);
	
	evas_object_geometry_get(wd->parent, &x, &y, NULL, NULL);
	evas_object_move(wd->edit_box, x, y);
}


static void _tab_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;
	Evas_Coord w, h, height;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_geometry_get(wd->edje, NULL, NULL, &w, &h);

	wd->w = w;
	wd->h = h;

	evas_object_resize(wd->edje, w, h);
//	evas_object_resize(wd->box, w, h);

	evas_object_geometry_get(edje_object_part_object_get(wd->edje, "bg_image"), NULL, NULL, NULL, &height);
	evas_object_resize(wd->view, w, h - height + 1);

	evas_object_geometry_get(wd->parent, NULL, NULL, &w, &h);
	evas_object_resize(wd->edit_box, w, h);
}


static void _tab_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_show(wd->view);
	evas_object_show(wd->edit_box);
	evas_object_show(wd->edje);
//	evas_object_show(wd->box);
}



static void _tab_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_hide(wd->view);
	evas_object_hide(wd->edit_box);
	evas_object_hide(wd->edje);
//	evas_object_hide(wd->box);
}

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Tab_Item *item;

	if (!wd) return;

	if (wd->view) {
		evas_object_smart_member_del(wd->view);
		evas_object_del(wd->view);
		wd->view = NULL;
	}

	if (wd->edit_box) {
		evas_object_smart_member_del(wd->edit_box);
		evas_object_del(wd->edit_box);
		wd->edit_box = NULL;
	}

	if (wd->navigation) {
		evas_object_smart_member_del(wd->navigation);
		evas_object_del(wd->navigation);
		wd->navigation = NULL;
	}

	if (wd->edje) {
		evas_object_smart_member_del(wd->edje);
		evas_object_del(wd->edje);
		wd->edje = NULL;
	}

	if (wd->box) {
		evas_object_smart_member_del(wd->box);
		evas_object_del(wd->box);
		wd->box = NULL;
	}

	EINA_LIST_FREE(wd->items, item){
		eina_stringshare_del(item->label);
		if (item->icon) evas_object_del(item->icon);
		if(item->base) evas_object_del(item->base);
		if(item->edit_item) evas_object_del(item->edit_item);
		free(item);
	}

	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	const Eina_List *l;
	Elm_Tab_Item *item;
	char buf[MAX_ARGS];
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_set(wd->edje, "tab", buf, elm_widget_style_get(obj));

	EINA_LIST_FOREACH(wd->items, l, item){
		_elm_theme_set(item->base, "tab", "item", elm_widget_style_get(item->obj));
		if (item->label){
			edje_object_part_text_set(item->base, "elm.text", item->label);
		}

		if (item->icon){
	//		int ms = 0;

	//		ms = ((double)wd->icon_size * _elm_config->scale);
			evas_object_size_hint_min_set(item->icon, 40, 40);
			evas_object_size_hint_max_set(item->icon, 100, 100);
			edje_object_part_swallow(item->base, "elm.swallow.icon", item->icon);
			evas_object_show(item->icon);
			elm_widget_sub_object_add(obj, item->icon);
		}
	}
}

static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	//Evas_Object *sub = event_info;
	if (!wd) return;
	/*
	   if (sub == wd->icon)
	   {
	   edje_object_signal_emit(wd->btn, "elm,state,icon,hidden", "elm");
	   evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
	   _changed_size_hints, obj);
	   wd->icon = NULL;
	   edje_object_message_signal_process(wd->btn);
	   _sizing_eval(obj);
	   }
	   */
}


static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	DBG("[%s]\n", __func__);

	_tab_object_move(obj, NULL, obj, NULL);
	_tab_object_resize(obj, NULL, obj, NULL);
}

/////////////////////////////////////////////////////////////
//
// animation function
//
/////////////////////////////////////////////////////////////

static unsigned int _current_time_get()
{
	struct timeval timev;

	gettimeofday(&timev, NULL);
	return ((timev.tv_sec * 1000) + ((timev.tv_usec) / 1000));
}

static void set_evas_map(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
	if(obj == NULL) {
		return;
	}

	Evas_Map *map = evas_map_new(4);
	if(map == NULL) return;

	evas_map_smooth_set( map, EINA_TRUE );
	evas_map_util_points_populate_from_object_full( map, obj, 0 );
	evas_object_map_enable_set( obj, EINA_TRUE );

	evas_map_util_3d_perspective( map, x + w/2, y + h/2, 0, w * 10 );
	evas_map_util_points_populate_from_geometry( map, x, y, w, h, 0 );

	evas_object_map_set( obj, map );
	evas_map_free( map );
}

static int move_evas_map(void *data)
{
	double t;
	int dx, dy, dw, dh;
	int px, py, pw, ph;
	int x, y, w, h;
	Animation_Data *ad = (Animation_Data *)data;
	
	t = ELM_MAX(0.0, _current_time_get() - ad->start_time) / 1000;
	dx = ad->tx - ad->fx;
	dy = ad->ty - ad->fy;
	dw = ad->tw - ad->fw;
	dh = ad->th - ad->fh;

	if (t <= ad->time){
		x = (1 * sin((t / ad->time) * (M_PI / 2)) * dx);
		y = (1 * sin((t / ad->time) * (M_PI / 2)) * dy);
		w = (1 * sin((t / ad->time) * (M_PI / 2)) * dw);
		h = (1 * sin((t / ad->time) * (M_PI / 2)) * dh);
	} else {
		x = dx;
		y = dy;
		w = dw;
		h = dh;
	}

	px = ad->fx + x;
	py = ad->fy + y;
	pw = ad->fw + w;
	ph = ad->fh + h;

	if( x == dx && y == dy && w == dw && h == dh ){
		ecore_animator_del(ad->timer);
		ad->timer = NULL;

		set_evas_map(ad->obj, px, py, pw, ph);

		if(ad->func != NULL) ad->func(ad->data, ad->obj);
	}else{	
		set_evas_map(ad->obj, px, py, pw, ph);
	}

	return EXIT_FAILURE;
}

static void move_object_with_animation(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, Evas_Coord x_, Evas_Coord y_, Evas_Coord w_, Evas_Coord h_, double time, void (*func) (void *data, Evas_Object *obj), void *data)
{
	Animation_Data *ad = (Animation_Data *)malloc(sizeof(Animation_Data));

	ad->obj = obj;
	ad->fx = x;
	ad->fy = y;
	ad->fw = w;
	ad->fh = h;
	ad->tx = x_;
	ad->ty = y_;
	ad->tw = w_;
	ad->th = h_;
	ad->start_time = _current_time_get();
	ad->time = time;
	ad->func = func;
	ad->data = data;
	ad->timer = ecore_animator_add( move_evas_map , ad);
}


/////////////////////////////////////////////////////////////
//
// callback function
//
/////////////////////////////////////////////////////////////

static void print_all_item(Widget_Data *wd)
{
	const Eina_List *l;
	Elm_Tab_Item *item;
	Evas_Coord x, y, w, h;

	printf("=========\n");
	EINA_LIST_FOREACH(wd->items, l, item){
		evas_object_geometry_get(item->base, &x, &y, &w, &h);
		printf("label: %s order: %d %d %d %d %d\n", item->label, item->order, x, y, w, h);
		evas_object_show(item->base);
	}
	printf("=========\n");
}


static void done_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *)data;

//	print_all_item(wd);
	edje_object_signal_emit(wd->edit_box, "elm,state,hide,edit_box", "elm");

	wd->edit_mode = EINA_FALSE;
}



///////////////////////////////////////////////////////////////////
//
//  basic utility function
//
////////////////////////////////////////////////////////////////////

		
static void item_insert_in_bar(Elm_Tab_Item *it, int order)
{	
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = elm_widget_data_get(it->obj);
	int check = 0;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->order == order && item != it) check = 1;
	}

	if(check){
		EINA_LIST_FOREACH(wd->items, l, item){
			if(item->order > 0 ) elm_table_unpack(wd->box, item->base);
		}

		EINA_LIST_FOREACH(wd->items, l, item){
			if(item->order > 0){
				if(item->order >= order) item->order += 1;
				elm_table_pack(wd->box, item->base, item->order-1, 0, 1, 1);
				evas_object_show(item->base);
			}
		}
	}
	it->order = order;
	elm_table_pack(wd->box, it->base, order-1, 0, 1, 1);
	evas_object_show(it->base);
}

static void item_delete_in_bar(Elm_Tab_Item *it)
{
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = elm_widget_data_get(it->obj);
	int i = 1;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item == it){
			it->order = 0;
			elm_table_unpack(wd->box, it->base);
			evas_object_hide(it->base);
		}else{
			if(item->order > 0){
				item->order = i;
				i++;
			}
		}
	}
}

static void item_exchange_animation_cb(void *data, Evas_Object *obj)
{
	Widget_Data *wd = (Widget_Data *)data;

	wd->animating--;
	if(wd->animating < 0){
		printf("animation error\n");
		wd->animating = 0;
	}
//	printf("animation end : %d\n", wd->animating);
}

static void item_exchange_in_bar(Elm_Tab_Item *it1, Elm_Tab_Item *it2)
{
	int order;
	Evas_Coord x, y, w, h;
	Evas_Coord x_, y_, w_, h_;
	Widget_Data *wd = elm_widget_data_get(it1->obj);

	evas_object_geometry_get(wd->moving_item->edit_item, &x, &y, &w, &h);

//	evas_render_updates_free(evas_render_updates(evas_object_evas_get(wd->moving_obj)));

	set_evas_map(wd->moving_obj, (wd->x+wd->w/2), (wd->y+wd->h/2), 0, 0);
//	evas_object_del(wd->moving_obj);
//	wd->moving_obj = NULL;
	
	evas_object_geometry_get(it1->base, &x, &y, &w, &h);
	evas_object_geometry_get(it2->base, &x_, &y_, &w_, &h_);

	wd->animating++;
	move_object_with_animation(it1->base, x, y, w, h, x_, y_, w_, h_, 0.25, item_exchange_animation_cb, wd);
	wd->animating++;
	move_object_with_animation(it2->base, x_, y_, w_, h_, x, y, w, h, 0.25, item_exchange_animation_cb, wd);

	elm_table_unpack(wd->box, it1->base);
	elm_table_unpack(wd->box, it2->base);

	order = it1->order;
	it1->order = it2->order;
	it2->order = order;

	elm_table_pack(wd->box, it1->base, it1->order-1, 0, 1, 1);
	elm_table_pack(wd->box, it2->base, it2->order-1, 0, 1, 1);
}

static void item_change_animation_cb(void *data, Evas_Object *obj)
{
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;

	wd->animating--;
	if(wd->animating < 0){
		printf("animation error\n");
		wd->animating = 0;
	}
//printf("animation end : %d\n", wd->animating);

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	set_evas_map(obj, x, y, 0, 0);
	evas_object_move(obj, -1000, -1000);

	evas_object_geometry_get(wd->moving_item->base, &x, &y, &w, &h);

	evas_object_move(wd->moving_obj, -1000, -1000);
	evas_object_del(wd->moving_obj);
}

static void item_change_in_bar(Elm_Tab_Item *it)
{
	Evas_Coord x, y, w, h;
	Evas_Coord x_, y_, w_, h_;
	Widget_Data *wd = elm_widget_data_get(it->obj);
	if(wd == NULL) return;
	if(wd->moving_item == NULL) return;

	evas_object_geometry_get(wd->moving_item->edit_item, &x, &y, &w, &h);
//	evas_object_geometry_get(it->base, &x, &y, &w, &h);
	set_evas_map(wd->moving_obj, x, y, w, h);
//	evas_object_del(wd->moving_obj);

	elm_table_unpack(wd->box, it->base);

	wd->moving_item->order = it->order;
	it->order = 0;

	elm_table_pack(wd->box, wd->moving_item->base, wd->moving_item->order-1, 0, 1, 1);
	
	evas_object_show(wd->moving_item->base);
		
	evas_render_updates_free(evas_render_updates(evas_object_evas_get(wd->moving_item->base)));

	evas_object_geometry_get(it->base, &x, &y, &w, &h);
	evas_object_geometry_get(it->edit_item, &x_, &y_, &w_, &h_);

	wd->animating++;
	move_object_with_animation(it->base, x, y, w, h, x_, y_, w_, h_, 0.25, item_change_animation_cb, wd);

	evas_object_geometry_get(wd->moving_item->base, &x, &y, &w, &h);
	set_evas_map(wd->moving_item->base, x, y, w, h);
}

static int selected_box(Elm_Tab_Item *it)
{
	int i = 1;
	int check = 0;
	Evas_Object *icon;
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = elm_widget_data_get(it->obj);

	EINA_LIST_FOREACH(wd->items, l, item){
		edje_object_signal_emit(_EDJ(item->base), "elm,state,unselected", "elm");
		item->selected = EINA_FALSE;
		edje_object_part_unswallow(wd->view, item->view);
		evas_object_hide(item->view);

		icon = edje_object_part_swallow_get(_EDJ(item->base), "elm.swallow.icon");
		if(icon){
//			if(strcmp(evas_object_type_get(icon), "edje") == 0){
//				edje_object_signal_emit(icon, "elm,state,unselected", "elm");
//			}else{
//				if(_EDJ(icon))
//					edje_object_signal_emit(_EDJ(icon), "elm,state,unselected", "elm");
				evas_object_color_set(icon, 255, 255, 255, 255);
//			}
		}

		if(it == item){
			edje_object_signal_emit(_EDJ(it->base), "elm,state,selected", "elm");
			edje_object_part_swallow(wd->view, "elm.swallow.view", it->view);
			evas_object_show(it->view);

			if(icon){
//				if(strcmp(evas_object_type_get(icon), "edje") == 0){
//					edje_object_signal_emit(icon, "elm,state,selected", "elm");
//				}else{
//					if(_EDJ(icon))
//						edje_object_signal_emit(_EDJ(icon), "elm,state,selected", "elm");
					evas_object_color_set(icon, 0, 151, 222, 15);
//				}
			}
			item->selected = EINA_TRUE;

			check = 1;
		}
		i++;
	}

	if(!check) return EXIT_FAILURE;

	evas_object_smart_callback_call(it->obj, "clicked", it);

	return EXIT_SUCCESS;
}

static int sort_cb(const void *d1, const void *d2)
{
	Elm_Tab_Item *item1, *item2;

	item1 = (Elm_Tab_Item *)d1;
	item2 = (Elm_Tab_Item *)d2;

	return item1->order > item2->order ? 1 : -1;
}

static Evas_Object *create_item_layout(Evas_Object *parent, Elm_Tab_Item *it)
{
	Evas_Object *obj;
	Evas_Object *icon;

	obj = elm_layout_add(parent);
	if(obj == NULL){
		fprintf(stderr, "Cannot load bg edj\n");
		return NULL;
	}
	elm_layout_theme_set(obj, "tabbar", "item", elm_widget_style_get(it->obj));
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_widget_sub_object_add(it->obj, obj);

	if (it->label){
		edje_object_part_text_set(_EDJ(obj), "elm.text", it->label);
	}

	if (it->icon_path){
		icon = elm_icon_add(obj);
		elm_icon_file_set(icon, AQUA_THEME_FILE, it->icon_path);

		evas_object_size_hint_min_set(icon, 40, 40);
		evas_object_size_hint_max_set(icon, 100, 100);
		evas_object_show(icon);
		edje_object_part_swallow(_EDJ(obj), "elm.swallow.icon", icon);
		it->icon = icon;
	}

	return obj;
}

static void edit_item_down_end_cb (void *data, Evas_Object *obj)
{
	Widget_Data *wd = (Widget_Data *)data;

//	evas_render_updates(evas_object_evas_get(wd->moving_obj));

	wd->animating--;
	if(wd->animating < 0){
		printf("animation error\n");
		wd->animating = 0;
	}
//printf("animation end : %d\n", wd->animating);
}

static void edit_item_return_cb (void *data, Evas_Object *obj)
{
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;

//	evas_render_updates(evas_object_evas_get(obj));
	evas_object_geometry_get(wd->moving_item->edit_item, &x, &y, &w, &h);
	set_evas_map(obj, x, y, 0, 0);

	if(wd->move_event != NULL) {
		ecore_event_handler_del(wd->move_event);
		wd->move_event = NULL;
	}
	if(wd->up_event != NULL) {
		ecore_event_handler_del(wd->up_event);
		wd->up_event = NULL;
	}

	evas_object_data_set(wd->moving_obj, "returning", 0);

	wd->animating--;
	if(wd->animating < 0){
		printf("animation error\n");
		wd->animating = 0;
	}
//printf("animation end : %d\n", wd->animating);
}

static int edit_item_move_cb(void *data, int type, void *event_info)
{
	const Eina_List *l;
	Elm_Tab_Item *item;
	Ecore_Event_Mouse_Move *ev = event_info;
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;

	if(wd->animating) return EXIT_FAILURE;

	evas_object_geometry_get( wd->moving_obj, &x, &y, &w, &h );

	w *= 2.0;
	h *= 2.0;

	x = ev->x - w/2;
	y = ev->y - h;

	set_evas_map(wd->moving_obj, x, y, w, h);

	EINA_LIST_FOREACH(wd->items, l, item){
		if(wd->moving_item->edit_item == item->edit_item) continue;
		evas_object_geometry_get(item->base, &x, &y, &w, &h);
		if(ev->x > x && ev->x < x+w && ev->y > y && ev->y < y+h){
			edje_object_signal_emit(_EDJ(item->base), "elm,state,show,glow", "elm");
		}else{
			edje_object_signal_emit(_EDJ(item->base), "elm,state,hide,glow", "elm");
		}
	}

	return EXIT_SUCCESS;
}

static int edit_item_up_cb(void *data, int type, void *event_info)
{
	Ecore_Event_Mouse_Button *ev = event_info;
	Evas_Coord x, y, w, h;
	Evas_Coord x_, y_, w_, h_;
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = (Widget_Data *)data;

	if(wd->moving_obj)
		if(evas_object_data_get(wd->moving_obj, "returning") == 1) return EXIT_FAILURE; 

	evas_object_color_set(wd->moving_item->edit_item, 255, 255, 255, 255);

	// check which change or not
	EINA_LIST_FOREACH(wd->items, l, item){
		if(wd->moving_item->edit_item == item->edit_item) continue;
		if(item->order <= 0) continue;
		evas_object_geometry_get(item->base, &x, &y, &w, &h);
		if(ev->x > x && ev->x < x+w && ev->y > y && ev->y < y+h){
			edje_object_signal_emit(_EDJ(item->base), "elm,state,hide,glow", "elm");
			break;	
		}
	}

	if(item != NULL) {     
		if(wd->moving_item->order > 0){
			item_exchange_in_bar(wd->moving_item, item);
		}else{
			item_change_in_bar(item);
		}

		if(wd->move_event != NULL) {
			ecore_event_handler_del(wd->move_event);
			wd->move_event = NULL;
		}
		if(wd->up_event != NULL) {
			ecore_event_handler_del(wd->up_event);
			wd->up_event = NULL;
		}
	}else{
		// return moving object to original location
		evas_object_geometry_get(wd->moving_item->edit_item, &x_, &y_, &w_, &h_);
		evas_object_geometry_get( wd->moving_obj, &x, &y, &w, &h );

		w *= 2.0;
		h *= 2.0;
		x = ev->x - w/2;
		y = ev->y - h;

		evas_object_data_set(wd->moving_obj, "returning", 1);
		wd->animating++;
		move_object_with_animation(wd->moving_obj, x, y, w, h, x_, y_, w_, h_, 0.25, edit_item_return_cb, wd);
	}

	return EXIT_SUCCESS;
}

static void edit_item_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = event_info;
	Evas_Coord x, y, w, h;
	Evas_Coord x_, y_, w_, h_;
	Widget_Data *wd = (Widget_Data *)data;
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(wd->animating) return;

	if(wd->up_event == NULL)
		wd->up_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, edit_item_up_cb, (void *)wd);
	if(wd->move_event == NULL)
		wd->move_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, edit_item_move_cb, (void *)wd);


	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->edit_item == obj) break;
	}
	if(item == NULL) return;

	wd->moving_item = item;

	evas_object_color_set(item->edit_item, 100, 100, 100, 255);
	
	if(wd->moving_obj) evas_object_del(wd->moving_obj);
	wd->moving_obj = NULL;

	wd->moving_obj = create_item_layout(obj, item); 
	evas_object_geometry_get(obj, &x, &y, &w, &h);
	evas_object_move(wd->moving_obj, -1000 , -1000);
	evas_object_resize(wd->moving_obj, w, h);
	evas_object_show(wd->moving_obj);

	w_ = w * 2.0;
	h_ = h * 2.0;
	x_ = ev->output.x - w_/2;
	y_ = ev->output.y - h_;

	wd->animating++;
	move_object_with_animation(wd->moving_obj, x, y, w, h, x_, y_, w_, h_, 0.1, edit_item_down_end_cb, wd);
}

static void bar_item_move_end_cb (void *data, Evas_Object *obj)
{
	Widget_Data *wd = (Widget_Data *)data;
	const Eina_List *l;
	Elm_Tab_Item *item;

//	evas_render_updates(evas_object_evas_get(obj));

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->base == obj) break;
	}

	wd->animating--;
	if(wd->animating < 0){
		printf("animation error\n");
		wd->animating = 0;
	}
	evas_object_data_set(obj, "animating", 0);
//printf("animation end : %d\n", wd->animating);
}


static int bar_item_animation_end_check(void *data)
{
	const Eina_List *l;
	Elm_Tab_Item *item;
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;


	if(wd->animating) return EXIT_FAILURE;

//	evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
//	set_evas_map(wd->moving_obj, x, y, w, h);

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->base == wd->moving_obj) break;
	}
	if(item == NULL) {
		printf("item is NULL\n");
	}else{
		item->order = wd->empty_num;
		wd->empty_num = 0;
		wd->moving_obj = NULL;
	}
	if(wd->bar_move_event != NULL) {
		ecore_event_handler_del(wd->bar_move_event);
		wd->bar_move_event = NULL;
	}
	if(wd->bar_up_event != NULL) {
		ecore_event_handler_del(wd->bar_up_event);
		wd->bar_up_event = NULL;
	}

	return EXIT_SUCCESS;
}

static int bar_item_move_cb(void *data, int type, void *event_info)
{
	const Eina_List *l;
	Elm_Tab_Item *item, *it;
	Ecore_Event_Mouse_Move *ev = event_info;
	Widget_Data *wd = (Widget_Data *)data;
	Evas_Coord x, y, w, h, x_, y_, w_, h_;
	int tmp;

	if(wd->moving_obj == NULL) {
		printf("%s : moving_obj is NULL\n", __func__);
		return EXIT_FAILURE;
	}
	evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
	x = ev->x - w/2;

	set_evas_map(wd->moving_obj, x, y, w, h);
//	evas_render_updates(evas_object_evas_get(wd->moving_obj));
	
	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->base == wd->moving_obj) {
			it = item;
			continue;
		}
		if(evas_object_data_get(item->base, "animating") == 1) continue;
		evas_object_geometry_get(item->base, &x, &y, &w, &h);
		if(ev->x > x && ev->x < x+w){
			evas_object_geometry_get(wd->moving_obj, &x_, &y_, &w_, &h_);
			evas_object_move(wd->moving_obj, x, y);

			tmp = wd->empty_num;
			wd->empty_num = item->order;
			item->order = tmp;

			elm_table_unpack(wd->box, item->base);
			elm_table_unpack(wd->box, wd->moving_obj);
			elm_table_pack(wd->box, item->base, item->order-1, 0, 1, 1);
			elm_table_pack(wd->box, wd->moving_obj, wd->empty_num-1, 0, 1, 1);

			wd->animating++;
			evas_object_data_set(item->base, "animating", 1);
			move_object_with_animation(item->base, x, y, w, h, x_, y_, w_, h_, 0.25, bar_item_move_end_cb, wd);
		}
	}
	
	return EXIT_SUCCESS;
}

static int bar_item_up_cb(void *data, int type, void *event_info)
{
	Evas_Coord tx, x, y, w, h;
	Ecore_Event_Mouse_Button *ev = event_info;
	Widget_Data *wd = (Widget_Data *)data;

	evas_object_geometry_get(wd->moving_obj, &x, &y, &w, &h);
	tx = ev->x - w/2;
	move_object_with_animation(wd->moving_obj, tx, y, w, h, x, y, w, h, 0.25, NULL, NULL);
//	set_evas_map(wd->moving_obj, x, y, w, h);
	
	ecore_timer_add(0.1, bar_item_animation_end_check, wd);

	return EXIT_SUCCESS;
}

static void bar_item_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *)data;
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(wd->animating) return;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->base == obj) break;
	}

	if(wd->edit_mode){
		if(wd->bar_up_event != NULL || wd->bar_move_event != NULL) return;
		
		wd->moving_obj = obj;

		wd->empty_num = item->order;

		if(wd->bar_up_event == NULL)
			wd->bar_up_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, bar_item_up_cb, (void *)wd);
		if(wd->bar_move_event == NULL)
			wd->bar_move_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, bar_item_move_cb, (void *)wd);
	}else{
		selected_box(item);
	}
}


///////////////////////////////////////////////////////////////////
//
//  API function
//
////////////////////////////////////////////////////////////////////

/**
 * Add a new tab object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Tab
 */
EAPI Evas_Object *elm_tabbar_add(Evas_Object *parent) {
	Evas_Object *obj = NULL;
	Widget_Data *wd = NULL;
	Evas_Coord x, y, w, h;
	Evas_Object *navi_item, *r_button;

	wd = ELM_NEW(Widget_Data);
	wd->evas = evas_object_evas_get(parent);
	if (wd->evas == NULL) return NULL;
	obj = elm_widget_add(wd->evas);
	if(obj == NULL) return NULL;
	elm_widget_type_set(obj, "tabbar");
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
	wd->num = 0;
	wd->animating = 0;
	wd->edit_mode = EINA_FALSE;

	wd->view = edje_object_add(wd->evas);
	_elm_theme_set(wd->view, "tabbar", "view", "default");
	if(wd->view == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}
	evas_object_show(wd->view);

	// edit box
	wd->edit_box = edje_object_add(wd->evas);
	_elm_theme_set(wd->edit_box, "tabbar", "edit_box", "default");
	if(wd->edit_box == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}
	evas_object_show(wd->edit_box);

	wd->navigation = elm_navigationbar_add(wd->edit_box);
	r_button = elm_button_add(wd->navigation);
	elm_button_label_set(r_button, "Done");
	evas_object_smart_callback_add(r_button, "clicked", done_button_cb, wd);
	elm_navigationbar_push(wd->navigation, "Configure", NULL, r_button, NULL, EINA_FALSE);
	edje_object_part_swallow(wd->edit_box, "elm.swallow.navigation", wd->navigation);
	
	wd->edit_table = elm_table_add(wd->edit_box);
	edje_object_part_swallow(wd->edit_box, "elm.swallow.table", wd->edit_table);

	/* load background edj */
	wd->edje = edje_object_add(wd->evas);
//	snprintf(buf, sizeof(buf), "bg_portrait_%d", wd->view_slot_num);
	_elm_theme_set(wd->edje, "tabbar", "base", "default");
	if(wd->edje == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}
	evas_object_show(wd->edje);

	// initialization
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE, _tab_object_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _tab_object_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _tab_object_show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _tab_object_hide, obj);

	// items container
	wd->box = elm_table_add(wd->edje);
	evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(wd->edje, "elm.swallow.items", wd->box);	
	elm_widget_sub_object_add(obj, wd->box);

	//FIXME
	//	evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

	evas_object_smart_member_add(wd->view, obj);
	evas_object_smart_member_add(wd->edit_box, obj);
	elm_widget_resize_object_set(obj, wd->edje);
	evas_object_smart_member_add(wd->box, obj);

	// initialization
	_sizing_eval(obj);

	return obj;
}

/**
 * Add new item
 *
 * @param obj The tab object
 * @param	icon The icon of item
 * @param	label The label of item
 * @param	func Callback function of item
 * @param	data The data of callback function
 * @return The item of tab
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tabbar_item_add(Evas_Object *obj, const char *icon_path, const char *label, Evas_Object *view)
{
	Elm_Tab_Item *it;
	Widget_Data *wd;

	if(obj == NULL) {
		fprintf(stderr, "Invalid argument: tab object is NULL\n");
		return NULL;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		fprintf(stderr, "Cannot get smart data\n");
		return NULL;
	}

	it = ELM_NEW(Elm_Tab_Item);
    if(it == NULL) return NULL;
    wd->items = eina_list_append(wd->items, it);
    it->obj = obj;
    it->label = eina_stringshare_add(label);
	it->icon_path = icon_path;
    it->badge = 0;
	it->view = view;

	it->base = create_item_layout(wd->edje, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN, bar_item_down_cb, wd);	
	evas_object_show(it->base);

	it->edit_item = create_item_layout(wd->edje, it);
	evas_object_event_callback_add(it->edit_item, EVAS_CALLBACK_MOUSE_DOWN, edit_item_down_cb, wd);
	evas_object_show(it->edit_item);

	wd->num += 1;
	it->order = wd->num;
	
	elm_table_pack(wd->box, it->base, wd->num-1, 0, 1, 1);

	elm_table_pack(wd->edit_table, it->edit_item, (wd->num-1)%4, (wd->num-1)/4, 1, 1);
		
	if(wd->num == 1) selected_box(it);

	return it;
}

#if 0

/**
 * Delete item from tab by index
 *
 * @param	it The item of tab

 * @ingroup Tab
 */
EAPI void elm_tabbar_item_del(Elm_Tab_Item *it)
{
	int check = 0;
	int i = 1;
	char buf[MAX_ARGS];
	Widget_Data *wd;
	const Eina_List *l;
	Elm_Tab_Item *item;


	if(it->obj == NULL){
		printf("Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(it->obj);
	if(wd == NULL){
		printf("Cannot get smart data\n");
		return;
	}

	edje_object_part_unswallow(wd->edje, it->base);

	EINA_LIST_FOREACH(wd->items, l, item){
		if(check){
			edje_object_part_unswallow(wd->edje, item->base);
			snprintf(buf, sizeof(buf), "slot_%d", i-1);
			edje_object_part_swallow(wd->edje, buf, item->base);
		}
		if(it == item){
			check = 1;
		}
		i++;
	}

	eina_stringshare_del(it->label);
	if (it->icon) evas_object_del(it->icon);
	evas_object_del(it->base);
	evas_object_del(it->edit_item);
	wd->items = eina_list_remove(wd->items, it);
	wd->num = wd->num - 1;
}

#endif

/**
 * Select item in tabbar
 *
 * @param	it The item of tabbar

 * @ingroup Tab
 */
EAPI void elm_tabbar_item_select(Elm_Tab_Item *it)
{
	if(it->obj == NULL) return;
	Widget_Data *wd = elm_widget_data_get(it->obj);
	if(wd == NULL) return;

	if(!wd->edit_mode){
		selected_box(it);
	}
}

/**
 * Get the icon of item
 *
 * @param	it The item of tabbar
 * @return The icon object
 *
 * @ingroup Tab
 */
EAPI Evas_Object *elm_tabbar_item_icon_get(Elm_Tab_Item *it)
{
	return it->icon;
}

/**
 * Get the label of item
 *
 * @param	it The item of tabbar
 * @return the label of item
 *
 * @ingroup Tab
 */
EAPI const char *elm_tabbar_item_label_get(Elm_Tab_Item *it)
{
	return it->label;
}

/**
 * Set the label of item
 *
 * @param	it The item of tabbar
 * @param	label The label of item
 *
 * @ingroup Tab
 */
EAPI void elm_tabbar_item_label_set(Elm_Tab_Item *it, const char *label)
{
	if(!it->base) return;

	edje_object_part_text_set(it->base, "elm.text", label);
}

#if 0

/**
 * Set the badge of item.
 *
 * @param	it The item of tab
 * @param	badge The number in item badge. If -1, badge is not visible. Otherwise, it is just number.
 *
 * @ingroup Tab
 */
EAPI void elm_tab_item_badge_set(Elm_Tab_Item *it, const int badge)
{
	char buf[MAX_ARGS];

	if(!it->base) return;

	if(it->badge == badge) return;

	if(badge < -1) {
		printf("elm_tab_item_badge_set : second parameter range not availble. (-1 <= badge < 1000)\n");
		return;
	}

	it->badge = badge;

	if(badge == -1){
		edje_object_signal_emit(it->base, "elm,badge,unvisible", "elm");
	}else{
		if(it->badge > 1000) {
			printf("Badge Number is too large. ( badge < 1000)\n");
			return;
		}

		snprintf(buf, sizeof(buf), "%d", it->badge);
		edje_object_part_text_set(it->base, "elm.text.badge", buf);

		if(it->badge > 100){
			edje_object_signal_emit(it->base, "elm,badge,text_normal", "elm");
		}else{
			edje_object_signal_emit(it->base, "elm,badge,text_small", "elm");
		}
		edje_object_signal_emit(it->base, "elm,badge,visible", "elm");
	}
}

/**
 * Get the selected item.
 *
 * @param	obj The tab object
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tab_selected_item_get(Evas_Object *obj)
{
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->selected) return item;
	}

	return NULL;
}

/**
 * Get the first item.
 *
 * @param	obj The tab object
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tab_first_item_get(Evas_Object *obj)
{
	if(obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->items) return NULL;

	return eina_list_data_get(wd->items);
}

/**
 * Get the last item.
 *
 * @param	obj The tab object
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tab_last_item_get(Evas_Object *obj)
{
	if(obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->items) return NULL;

	return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the items.
 *
 * @param	obj The tab object
 *
 * @ingroup Tab
 */
EAPI Eina_List *elm_tab_items_get(Evas_Object *obj)
{
	if(obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->items) return NULL;

	return wd->items;
}

/**
 * Get the previous item.
 *
 * @param	it The item of tab
 * @return	The previous item of the parameter item
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tab_item_prev(Elm_Tab_Item *it)
{
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(it->obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(it->obj);
	if(!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(it == item){
			l = eina_list_prev(l);
			if(!l) return NULL;
			return eina_list_data_get(l);
		}
	}

	return NULL;
}

/**
 * Get the next item.
 *
 * @param	obj The tab object
 * @return	The next item of the parameter item
 *
 * @ingroup Tab
 */
EAPI Elm_Tab_Item *elm_tab_item_next(Elm_Tab_Item *it)
{
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(it->obj == NULL) return NULL;

	Widget_Data *wd = elm_widget_data_get(it->obj);
	if(!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(it == item){
			l = eina_list_next(l);
			if(!l) return NULL;
			return eina_list_data_get(l);
		}
	}

	return NULL;
}

/**
 * Move the tab object
 *
 * @param	obj The tab object
 * @param	direction the direction of movement
 *
 * @ingroup Tab
 */
EAPI void elm_tab_move(Evas_Object *obj, int direction)
{
	Widget_Data *wd;

	retm_if(obj == NULL, "Invalid argument: tab object is NULL\n");
	wd = elm_widget_data_get(obj);
	retm_if(wd == NULL, "Cannot get smart data\n");

	if(direction == ELM_TAB_MOVE_LEFT){
		_move_obj_to_left(wd);
	}else if(direction == ELM_TAB_MOVE_RIGHT){
		_move_obj_to_right(wd);
	}
}

/**
 * Set the edit mode of the tab disable
 *
 * @param	obj The tab object
 * @param	disable if 1, edit mode is disable.
 *
 * @ingroup Tab
 */
EAPI void elm_tab_edit_disable_set(Evas_Object *obj, Eina_Bool disable)
{
	Widget_Data *wd;

	retm_if(obj == NULL, "Invalid argument: tab object is NULL\n");
	wd = elm_widget_data_get(obj);
	retm_if(wd == NULL, "Cannot get smart data\n");

	wd->edit_disable = disable;
}

/**
 * Get the availability for the edit mode of the tab
 *
 * @param	obj The tab object
 * @return  disable or not
 *
 * @ingroup Tab
 */
EAPI Eina_Bool elm_tab_edit_disable_get(Evas_Object *obj)
{
	Widget_Data *wd;

	retvm_if(obj == NULL, -1, "Invalid argument: tab object is NULL\n");
	wd = elm_widget_data_get(obj);
	retvm_if(wd == NULL, -1, "Cannot get smart data\n");

	return wd->edit_disable;
}

#endif

/**
 * Start edit mode
 *
 * @param	The tab object

 * @ingroup Tab
 */
EAPI void elm_tabbar_edit_start(Evas_Object *obj)
{
	Widget_Data *wd;

	if(obj == NULL){
		fprintf(stderr, "Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL){
		fprintf(stderr, "Cannot get smart data\n");
		return;
	}

	edje_object_signal_emit(wd->edit_box, "elm,state,show,edit_box", "elm");

	wd->edit_mode = EINA_TRUE;
}

/**
 * Set item in bar
 *
 * @param	it The item of tab
 * @param	bar true or false
 *
 * @ingroup Tab
 */
EAPI void elm_tabbar_item_visible_set(Elm_Tab_Item *it, Eina_Bool bar)
{
	Eina_Bool check = EINA_TRUE;
	
	if(it->order <= 0) check = EINA_FALSE;
	
	if(check == bar) return;

	if(bar){
		item_insert_in_bar(it, 0);
	}else{
		item_delete_in_bar(it);	
	}
}
