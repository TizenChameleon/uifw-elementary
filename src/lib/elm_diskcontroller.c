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
 * @defgroup Diskcontroller Diskcontroller
 * @ingroup Elementary
 *
 * This is a Diskcontroller. It can contain label and icon objects.
 */

#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <Elementary.h>
#include "elm_priv.h"

#ifndef EAPI
#define EAPI __attribute__ ((visibility("default")))
#endif

#define MAX_ARGS				512
#define SCROLLED_DISTANCE		215
#define STANDARD_W				480
#define SIDE_STRING_LENGTH		3
#define SIDE_POINT				50

#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

// internal data structure of diskcontroller object
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *object;
	Evas_Object *parent;
	Evas_Object *edje;
	Evas_Object *scroller;
	Evas_Object *main_box;
	Evas_Object *left_blank;
	Evas_Object *right_blank;

	Elm_Diskcontroller_Item *selected_item;
	Elm_Diskcontroller_Item *first;
	Elm_Diskcontroller_Item *second;
	Elm_Diskcontroller_Item *s_last;
	Elm_Diskcontroller_Item *last;

	Eina_List *items;
	Eina_List *r_items;

	Evas_Coord x, y, w, h;

	Ecore_Idler *i;
	int item_num;
	int round;
	int stop;
	double time;


	Ecore_Idler *idler;
	Ecore_Idler *check_idler;
	Eina_Bool flag;
	Eina_Bool init;

	Ecore_Event_Handler *move_event;
	Ecore_Event_Handler *up_event;
};

struct _Elm_Diskcontroller_Item
{
	Evas_Object *obj;
	Evas_Object *base;
	Evas_Object *icon;
	const char *label;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	const void *data;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static void _diskcontroller_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _diskcontroller_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _diskcontroller_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _diskcontroller_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Diskcontroller_Item *item;

	if (!wd) return;

	if (wd->main_box) {
		evas_object_del(wd->main_box);
		wd->main_box = NULL;
	}

	if (wd->scroller) {
		evas_object_del(wd->scroller);
		wd->scroller = NULL;
	}

	if (wd->edje) {
		evas_object_del(wd->edje);
		wd->edje = NULL;
	}

	EINA_LIST_FREE(wd->items, item){
		eina_stringshare_del(item->label);
		if(item->icon) evas_object_del(item->icon);
		if(item->base) evas_object_del(item->base);
		free(item);
		item = NULL;
	}

	free(wd);
	wd = NULL;
}

static void
_theme_hook(Evas_Object *obj)
{
	Eina_List *l;
	Elm_Diskcontroller_Item *it;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_object_set(obj, wd->edje, "diskcontroller", "bg", elm_widget_style_get(obj));

	if(wd->round){
		EINA_LIST_FOREACH(wd->r_items, l, it) {
			elm_layout_theme_set(it->base, "diskcontroller", "item", "default");
		}
	}else{
		EINA_LIST_FOREACH(wd->items, l, it) {
			elm_layout_theme_set(it->base, "diskcontroller", "item", "default");
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

	_diskcontroller_object_move(obj, NULL, obj, NULL);
	_diskcontroller_object_resize(obj, NULL, obj, NULL);
}

static void select_item(Elm_Diskcontroller_Item *it)
{
	if(it == NULL) return;

	Widget_Data *wd = elm_widget_data_get(it->obj);
	wd->selected_item = it;

	if(it->func) it->func((void *)(it->data), it->obj, it);
	evas_object_smart_callback_call(it->obj, "stop", it);
}

static void string_signal_emit(Elm_Diskcontroller_Item *it)
{
	char buf[MAX_ARGS];

	if(strlen(it->label) > 14){
		if(strlen(it->label) < 18){
			edje_object_signal_emit(_EDJ(it->base), "elm,state,center_small", "elm");
		}else{
			strncpy(buf, it->label, 40);
			buf[40] = '\0';
			edje_object_part_text_set(_EDJ(it->base), "elm.text", buf);
			edje_object_signal_emit(_EDJ(it->base), "elm,state,center_small", "elm");
		}
	}else{
		edje_object_signal_emit(_EDJ(it->base), "elm,state,center", "elm");
	}
}

static int check_letter(const char *str, int length)
{
	int code = str[length];

	if(code == '\0') return length; // null string
	else if((code >= 65 && code <= 90) || (code >= 97 && code <= 122)) return length; // alphabet
    else if (48<=code && code<58) return length; // number
    else if ((33<=code && code<47) || (58<=code && code<64) || (91<=code && code<96) || (123<=code && code<126)) return length; // special letter

	return length-1;
}

static Eina_Bool check_string(void *data)
{
	Eina_List *l;
	Elm_Diskcontroller_Item *it;
	Evas_Coord x, y, w, h;
	int mid, step, length, diff, side;
	char buf[MAX_ARGS];
	Widget_Data *wd = (Widget_Data *)data;
	Eina_List *list;

	if(wd->w <= 0) return EXIT_FAILURE;
	if(!wd->init) return EXIT_FAILURE;

	if(!wd->round) list = wd->items;
	else list = wd->r_items;
	
	EINA_LIST_FOREACH(list, l, it) {
		evas_object_geometry_get(it->base, &x, &y, &w, &h);
		mid = (int)(x + w/2);
		if(mid <= -SCROLLED_DISTANCE || mid >= wd->w+SCROLLED_DISTANCE) continue;

		side = MIN(SIDE_STRING_LENGTH, strlen(it->label));

		step = strlen(it->label)-side+1;
		if(mid <= wd->x + wd->w/2){
			diff = (wd->x + wd->w/2) - mid;
		}else{
			diff = mid - (wd->x + wd->w/2);
		}

		length = strlen(it->label) - (int)(diff/(int)(wd->w*SCROLLED_DISTANCE/STANDARD_W/step));
		if(mid - wd->x <= wd->w*SIDE_POINT/STANDARD_W || mid - wd->x >= wd->w-wd->w*SIDE_POINT/STANDARD_W){
			edje_object_signal_emit(_EDJ(it->base), "elm,state,side", "elm");
		}else{
			string_signal_emit(it);
		}
		length = MAX(length, side);
		length = check_letter(it->label, length);
		strncpy(buf, it->label, length);
		buf[length] = '\0';
		edje_object_part_text_set(_EDJ(it->base), "elm.text", buf);
	}

	if(wd->check_idler) ecore_idler_del(wd->check_idler);
	wd->check_idler = NULL;

	return EXIT_SUCCESS;
}

static void scroller_move_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;
	
	check_string(wd);

	if(wd->round) {
		elm_scroller_region_get(wd->scroller, &x, &y, &w, &h);

		if(x > 100000 || x < -100000){
			elm_scroller_region_show(wd->scroller, 0, y, w, h);
			return;
		}
		if(x > wd->w*SCROLLED_DISTANCE/STANDARD_W*(wd->item_num+1)){
			elm_scroller_region_show(wd->scroller, x-wd->w*SCROLLED_DISTANCE/STANDARD_W*wd->item_num, y, w, h);
		}else if(x < wd->w*SCROLLED_DISTANCE/STANDARD_W*(1)){
			elm_scroller_region_show(wd->scroller, x+wd->w*SCROLLED_DISTANCE/STANDARD_W*wd->item_num, y, w, h);
		}
	}

	wd->stop = 0;
}

static void scroller_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *)data;
	Eina_List *l;
	Elm_Diskcontroller_Item *it;
	Evas_Coord x,y,w,h;
	Eina_List *list;

	if(!wd->round) list = wd->items;
	else list = wd->r_items;

	if(wd->idler != NULL) return;

	EINA_LIST_FOREACH(list, l, it) {
		evas_object_geometry_get(it->base, &x, &y, &w, &h);
		if(abs((int)(wd->w/2 - (int)(x + w/2))) < 10) break;
	}

	if(it == NULL) return;

	select_item(it);

	wd->stop = 1;
}

static Eina_Bool move_scroller(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	Eina_List *l;
	Elm_Diskcontroller_Item *dit;
	Evas_Coord x, y, w, h;	
	int i;

	if(wd->round) i = 1;
	else i = 0;

	EINA_LIST_FOREACH(wd->items, l, dit){
		if(wd->selected_item == dit) break;
		i++;
	}

	if(dit == NULL) {
		wd->selected_item = (Elm_Diskcontroller_Item *)eina_list_nth(wd->items, 0);
		return EXIT_FAILURE;
	}

	elm_scroller_region_get(wd->scroller, &x, &y, &w, &h);
	elm_scroller_region_show(wd->scroller, wd->w*SCROLLED_DISTANCE/STANDARD_W*i+1, y, w, h);
	elm_scroller_region_get(wd->scroller, &x, &y, &w, &h);

	select_item(dit);

	if(wd->idler){
		ecore_idler_del(wd->idler);
		wd->idler = NULL;
	}

	wd->init = EINA_TRUE;

	check_string(wd);

	return EXIT_SUCCESS;
}

static Elm_Diskcontroller_Item *item_new(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), const void *data)
{
	Elm_Diskcontroller_Item *it;
	Widget_Data *wd;

	if(obj == NULL) return NULL;
	wd = elm_widget_data_get(obj);
	if(wd == NULL) return NULL;

	it = ELM_NEW(Elm_Diskcontroller_Item);
    if(it == NULL) return NULL;

    it->obj = obj;
    it->label = eina_stringshare_add(label);
    it->icon = icon;
    it->func = func;
    it->data = data;
	it->base = elm_layout_add(wd->edje);
	if(it->base == NULL) return NULL;
	elm_layout_theme_set(it->base, "diskcontroller", "item", "default");
	evas_object_size_hint_weight_set(it->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(it->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(it->base);
	elm_widget_sub_object_add(obj, it->base);

	if (it->label){
		//string_signal_emit(it);
		edje_object_part_text_set(_EDJ(it->base), "elm.text", it->label);
	}

	if (it->icon){
//		int ms = 0;

//		ms = ((double)wd->icon_size * _elm_config->scale);
		evas_object_size_hint_min_set(it->icon, 24, 24);
		evas_object_size_hint_max_set(it->icon, 40, 40);
		edje_object_part_swallow(_EDJ(it->base), "elm.swallow.icon", it->icon);
		evas_object_show(it->icon);
		elm_widget_sub_object_add(obj, it->icon);
	}

	if (it->label && it->icon){
		edje_object_signal_emit(_EDJ(it->base), "elm,state,icon_text", "elm");
	}

	return it;
}

static void dummy_item_del(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;

	if(wd->last){
		elm_box_unpack(wd->main_box, wd->last->base);
		wd->r_items = eina_list_remove(wd->r_items, wd->last);
		evas_object_del(wd->last->base);
		free(wd->last);
		wd->last = NULL;
	}
	if(wd->s_last){
		elm_box_unpack(wd->main_box, wd->s_last->base);
		wd->r_items = eina_list_remove(wd->r_items, wd->s_last);
		evas_object_del(wd->s_last->base);
		free(wd->s_last);
		wd->s_last = NULL;
	}
	if(wd->second){
		elm_box_unpack(wd->main_box, wd->second->base);
		wd->r_items = eina_list_remove(wd->r_items, wd->second);
		evas_object_del(wd->second->base);
		free(wd->second);
		wd->second = NULL;
	}
	if(wd->first){
		elm_box_unpack(wd->main_box, wd->first->base);
		wd->r_items = eina_list_remove(wd->r_items, wd->first);
		evas_object_del(wd->first->base);
		free(wd->first);
		wd->first = NULL;
	}
}

static void dummy_item_add(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	Elm_Diskcontroller_Item *dit;
	Elm_Diskcontroller_Item *it;

	dit = it = eina_list_nth(wd->items, 0);
	if(it != NULL){
		wd->first = item_new(it->obj, it->icon, it->label, it->func, it->data);
		wd->r_items = eina_list_append(wd->r_items, wd->first);
	}
	it = eina_list_nth(wd->items, 1);
	if(it != NULL){
		wd->second = item_new(it->obj, it->icon, it->label, it->func, it->data);
		wd->r_items = eina_list_append(wd->r_items, wd->second);
	}else{
		wd->second = item_new(dit->obj, dit->icon, dit->label, dit->func, dit->data);
		wd->r_items = eina_list_append(wd->r_items, wd->second);
	}
	it = eina_list_nth(wd->items, wd->item_num-1);
	if(it != NULL && wd->last == NULL){
		wd->last = item_new(it->obj, it->icon, it->label, it->func, it->data);
		wd->r_items = eina_list_prepend(wd->r_items, wd->last);
	}else{
		wd->last = item_new(dit->obj, dit->icon, dit->label, dit->func, dit->data);
		wd->r_items = eina_list_prepend(wd->r_items, wd->last);
	}
	it = eina_list_nth(wd->items, wd->item_num-2);
	if(it != NULL && wd->s_last == NULL){
		wd->s_last = item_new(it->obj, it->icon, it->label, it->func, it->data);
		wd->r_items = eina_list_prepend(wd->r_items, wd->s_last);
	}else{
		wd->s_last = item_new(dit->obj, dit->icon, dit->label, dit->func, dit->data);
		wd->r_items = eina_list_prepend(wd->r_items, wd->s_last);
	}
}

/**
 * Add a new diskcontroller object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Diskcontroller
 */
EAPI Evas_Object *elm_diskcontroller_add(Evas_Object *parent)
{
	Evas_Object *obj = NULL;
	Widget_Data *wd = NULL;
	Evas_Coord x, y, w, h;
	Evas *e;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	if (e == NULL) return NULL;
	obj = elm_widget_add(e);
	if(obj == NULL) return NULL;
	elm_widget_type_set(obj, "diskcontroller");
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
	wd->item_num = 0;
	wd->round = 0;
	wd->stop = 0;
	wd->init = EINA_FALSE;

	/* load background edj */
	wd->edje = edje_object_add(e);
	_elm_theme_object_set(obj, wd->edje, "diskcontroller", "bg", "default");
	if(wd->edje == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}

	evas_object_show(wd->edje);

	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE, _diskcontroller_object_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _diskcontroller_object_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _diskcontroller_object_show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _diskcontroller_object_hide, obj);

	wd->scroller = elm_scroller_add(wd->edje);
	elm_scroller_policy_set(wd->scroller, ELM_SMART_SCROLLER_POLICY_OFF, ELM_SMART_SCROLLER_POLICY_OFF);
	elm_scroller_bounce_set(wd->scroller, 1, 0);
	elm_scroller_page_size_set(wd->scroller, (int)(wd->w*SCROLLED_DISTANCE/STANDARD_W), 0);
	edje_object_part_swallow(wd->edje, "elm.scroller", wd->scroller);

	evas_object_smart_callback_add(wd->scroller, "scroll", scroller_move_cb, wd);
	evas_object_smart_callback_add(wd->scroller, "scroll,anim,stop", scroller_stop_cb, wd);

	wd->main_box = elm_box_add(wd->edje);
	elm_box_horizontal_set(wd->main_box, 1);
	elm_box_homogenous_set(wd->main_box, 2);
	evas_object_show(wd->main_box);
	evas_object_size_hint_weight_set(wd->main_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->main_box, EVAS_HINT_FILL, EVAS_HINT_FILL);

	wd->left_blank = elm_layout_add(wd->edje);
	if(wd->left_blank == NULL) return NULL;
	elm_layout_theme_set(wd->left_blank, "diskcontroller", "item", "default");
	evas_object_size_hint_weight_set(wd->left_blank, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->left_blank, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(wd->left_blank);
	elm_box_pack_end(wd->main_box, wd->left_blank);
	elm_widget_sub_object_add(obj, wd->left_blank);

	wd->right_blank = elm_layout_add(wd->edje);
	if(wd->right_blank == NULL) return NULL;
	elm_layout_theme_set(wd->right_blank, "diskcontroller", "item", "default");
	evas_object_size_hint_weight_set(wd->right_blank, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->right_blank, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(wd->right_blank);
	elm_box_pack_end(wd->main_box, wd->right_blank);
	elm_widget_sub_object_add(obj, wd->right_blank);

	elm_scroller_content_set(wd->scroller, wd->main_box);

	//FIXME
	//	evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);


	elm_widget_resize_object_set(obj, wd->edje);
	evas_object_smart_member_add(wd->scroller, obj);
	evas_object_smart_member_add(wd->main_box, obj);

	_sizing_eval(obj);
	return obj;
}

/**
 * Set round mode
 *
 * @param it The item of diskcontroller
 * @param if or not set round mode
 *
 * @ingroup Diskcontroller
 */
EAPI void elm_diskcontroller_round_set(Evas_Object *obj, Eina_Bool round)
{
	Widget_Data *wd;

	if(obj == NULL) return;
	wd = elm_widget_data_get(obj);
	if(wd == NULL) return;

	if(wd->round == round) return;
	wd->round = round;


	if(wd->r_items) eina_list_free(wd->r_items);
	wd->r_items = eina_list_clone(wd->items);

	if(wd->round) {
		elm_box_unpack(wd->main_box, wd->left_blank);
		evas_object_hide(wd->left_blank);
		elm_box_unpack(wd->main_box, wd->right_blank);
		evas_object_hide(wd->right_blank);

		if(wd->items == NULL) return;

		dummy_item_add(wd);
		dummy_item_del(wd);

		//composite another list
		if(wd->first->base) elm_box_pack_end(wd->main_box, wd->first->base);
		if(wd->second->base) elm_box_pack_end(wd->main_box, wd->second->base);
		if(wd->last->base) elm_box_pack_start(wd->main_box, wd->last->base);
		if(wd->s_last->base) elm_box_pack_start(wd->main_box, wd->s_last->base);

	} else {
		elm_box_unpack(wd->main_box, wd->first->base);
		evas_object_hide(wd->first->base);
		elm_box_unpack(wd->main_box, wd->second->base);
		evas_object_hide(wd->second->base);
		elm_box_unpack(wd->main_box, wd->last->base);
		evas_object_hide(wd->last->base);
		elm_box_unpack(wd->main_box, wd->s_last->base);
		evas_object_hide(wd->s_last->base);


		elm_box_pack_start(wd->main_box, wd->left_blank);
		elm_box_pack_end(wd->main_box, wd->right_blank);
	}

	_sizing_eval(obj);
}


/**
 * @fn			int elm_diskcontroller_item_add(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, Evas_Object *obj, void event_info), const void *data)
 * @ingroup		TW3_diskcontroller_group
 * @brief		Add text item
 * @param[in]	obj The diskcontroller object
 * @param[in]	icon The icon of item
 * @param[in]	label The label of item
 * @param[in]	func Callback function of item
 * @param[in]	data The data of callback function
 * @return		0 (SUCCESS) or -1 (FAIL)
 */
EAPI Elm_Diskcontroller_Item *elm_diskcontroller_item_add(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), const void *data)
{
	Elm_Diskcontroller_Item *it;
	Widget_Data *wd;

	if(obj == NULL) return NULL;
	wd = elm_widget_data_get(obj);
	if(wd == NULL) return NULL;

	it = item_new(obj, icon, label, func, data);

	wd->items = eina_list_append(wd->items, it);

	wd->item_num++;

	if(wd->round){
		dummy_item_del(wd);
		wd->r_items = eina_list_append(wd->r_items, it);
		dummy_item_add(wd);

		if(wd->last) elm_box_pack_start(wd->main_box, wd->last->base);
		if(wd->s_last) elm_box_pack_start(wd->main_box, wd->s_last->base);
		elm_box_pack_end(wd->main_box, it->base);
		if(wd->first) elm_box_pack_end(wd->main_box, wd->first->base);
		if(wd->second) elm_box_pack_end(wd->main_box, wd->second->base);
	}else{
		elm_box_unpack(wd->main_box, wd->right_blank);
		elm_box_pack_end(wd->main_box, it->base);
		elm_box_pack_end(wd->main_box, wd->right_blank);
	}

	wd->selected_item = it;
	if(wd->idler == NULL) wd->idler = ecore_idler_add(move_scroller, wd);

	_sizing_eval(obj);

	return it;
}

/**
 * Delete the item
 *
 * @param it The item of diskcontroller
 *
 * @ingroup Diskcontroller
 */
EAPI void elm_diskcontroller_item_del(Elm_Diskcontroller_Item *it)
{
	Widget_Data *wd;
	Elm_Diskcontroller_Item *dit;
	Evas_Object *obj;
	
	if(it == NULL) return;
	if(it->obj == NULL) return;
	obj = it->obj;
	wd = elm_widget_data_get(it->obj);
	if(wd == NULL) return;

	elm_box_unpack(wd->main_box, it->base);

	if(wd->round){
		wd->r_items = eina_list_remove(wd->r_items, it);
	}
	wd->items = eina_list_remove(wd->items, it);

	if(wd->selected_item == it){
		dit = (Elm_Diskcontroller_Item *)eina_list_nth(wd->items, 0);
		if(dit != it){
			wd->selected_item = dit;
		}else{
			wd->selected_item = (Elm_Diskcontroller_Item *)eina_list_nth(wd->items, 1);
		}
	}

	if(it->base) evas_object_del(it->base);
	if(it->icon) evas_object_del(it->icon);
	if(it) free(it);
	it = NULL;

	wd->item_num -= 1;

	if(wd->round){
		if(wd->item_num == 0){
			evas_object_hide(wd->first->base);
			evas_object_hide(wd->second->base);
			evas_object_hide(wd->last->base);
			evas_object_hide(wd->s_last->base);
		}else{
			dit = eina_list_nth(wd->items, 0);
			if(dit){
				wd->first->label = eina_stringshare_add(dit->label);
				edje_object_part_text_set(_EDJ(wd->first->base), "elm.text", wd->first->label);
			}
			dit = eina_list_nth(wd->items, 1);
			if(dit){
				wd->second->label = eina_stringshare_add(dit->label);
				edje_object_part_text_set(_EDJ(wd->second->base), "elm.text", wd->second->label);
			}
			dit = eina_list_nth(wd->items, eina_list_count(wd->items)-1);
			if(dit){
				wd->last->label = eina_stringshare_add(dit->label);
				edje_object_part_text_set(_EDJ(wd->last->base), "elm.text", wd->last->label);
			}
			dit = eina_list_nth(wd->items, eina_list_count(wd->items)-2);
			if(dit){
				wd->s_last->label = eina_stringshare_add(dit->label);
				edje_object_part_text_set(_EDJ(wd->s_last->base), "elm.text", wd->s_last->label);
			}
		}
	}

	wd->check_idler = ecore_idler_add(check_string, wd);

	_sizing_eval(obj);
}

/**
 * Get the label of item
 *
 * @param it The item of diskcontroller
 * @return The label of item
 *
 * @ingroup Diskcontroller
 */
EAPI const char *elm_diskcontroller_item_label_get(Elm_Diskcontroller_Item *it)
{
	return it->label;
}


/**
 * Set the label of item
 *
 * @param it The item of diskcontroller
 * @return The label of item
 *
 * @ingroup Diskcontroller
 */
EAPI void elm_diskcontroller_item_label_set(Elm_Diskcontroller_Item *it, const char *label)
{
	it->label = eina_stringshare_add(label);
	edje_object_part_text_set(_EDJ(it->base), "elm.text", it->label);
}

/**
 * Set the item visible
 *
 * @param it The item of diskcontroller
 *
 * @ingroup Diskcontroller
 */
EAPI void elm_diskcontroller_item_focus_set(Elm_Diskcontroller_Item *it)
{
	Widget_Data *wd;

	if(it->obj == NULL) return;
	wd = elm_widget_data_get(it->obj);
	if(wd == NULL) return;

	wd->selected_item = it;

	if(wd->idler == NULL) ecore_idler_add(move_scroller, wd);
}


///////////////////////////////////////////////////////////////////
//
//  Smart Object basic function
//
////////////////////////////////////////////////////////////////////

static void _diskcontroller_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
}


static void _diskcontroller_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd;
	Evas_Coord w, h;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_geometry_get(wd->edje, NULL, NULL, &w, &h);

	wd->w = w;
	wd->h = h;

	evas_object_resize(wd->edje, w, h);
	if(wd->round){
		evas_object_size_hint_min_set(wd->main_box, w*SCROLLED_DISTANCE*(wd->item_num+4)/STANDARD_W, h-1);
	}else{
		evas_object_size_hint_min_set(wd->main_box, w*SCROLLED_DISTANCE*(wd->item_num+2)/STANDARD_W, h-1);
	}
	elm_scroller_page_size_set(wd->scroller, (int)(wd->w*SCROLLED_DISTANCE/STANDARD_W), 0);

	if(wd->idler == NULL) {
		wd->idler = ecore_idler_add(move_scroller, wd);
	}
}


static void _diskcontroller_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_show(wd->edje);
}



static void _diskcontroller_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_hide(wd->edje);
}

