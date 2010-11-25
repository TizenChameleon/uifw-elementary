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
 * @ingroup Elementary
 *
 * This is a Tab. It can contain label and icon objects.
 * You can change the location of items.
 */


#include <string.h>
#include <math.h>

#include <Elementary.h>
#include "elm_priv.h"

#ifndef EAPI
#define EAPI __attribute__ ((visibility("default")))
#endif

#define MAX_ITEM 8
#define BASIC_SLOT_NUMBER 3
#define KEYDOWN_INTERVAL	0.6

#define PORTRAIT		0
#define LANDSCAPE		1
#define LANDSCAPE_GAP   10

#define MAX_ARGS	512

#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)
#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))


// internal data structure of tab object
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas *evas;
	Evas_Object *object;
	Evas_Object *parent;
	Evas_Object *edje;
	Evas_Object *moving_obj;
	Evas_Object *box[MAX_ITEM];
	Evas_Object *ebx;

	Evas_Coord x, y, w, h;

	int cur_first_slot;
	int mode;
	int view_slot_num;
	int tab_down_x;
	int tab_down_y;
	int num;
	int edit_from;
	int edit_to;
	double time;

	Eina_List *items;

	Ecore_Timer *timer;
	Eina_Bool flag;
	Eina_Bool edit_disable;

	Ecore_Event_Handler *move_event;
	Ecore_Event_Handler *up_event;
};

struct _Elm_Tab_Item
  {
	Evas_Object *obj;
	Evas_Object *base;
	Evas_Object *icon;
	const char *label;
	int slot;
	int badge;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	const void *data;
	Eina_Bool selected : 1;
  };

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static void _tab_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _tab_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _tab_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _tab_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void light_check(Widget_Data *wd);
static void select_check(Widget_Data *wd);
static void edit_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static int _selected_box(Elm_Tab_Item *it);
static int _move_obj_to_left(Widget_Data *wd);
static int _move_obj_to_right(Widget_Data *wd);
static void item_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void press_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void press_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void press_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
void tab_item_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Tab_Item *item;

	if (!wd) return;

	if (wd->edje) {
		evas_object_smart_member_del(wd->edje);
		evas_object_del(wd->edje);
		wd->edje = NULL;
	}

	EINA_LIST_FREE(wd->items, item){
		eina_stringshare_del(item->label);
		if (item->icon) evas_object_del(item->icon);
		if(item->base) evas_object_del(item->base);
		free(item);
	}

	if (wd->ebx) {
		evas_object_smart_member_del(wd->ebx);
		evas_object_del(wd->ebx);
		wd->ebx = NULL;
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

	if(wd->mode == PORTRAIT){
		snprintf(buf, sizeof(buf), "bg_portrait_%d", wd->view_slot_num);
	}else if(wd->mode == LANDSCAPE){
		snprintf(buf, sizeof(buf), "bg_landscape_%d", wd->view_slot_num);
	}
	_elm_theme_object_set(obj, wd->edje, "tab", buf, elm_widget_style_get(obj));

	EINA_LIST_FOREACH(wd->items, l, item){
		_elm_theme_object_set(obj, item->base, "tab", "item", elm_widget_style_get(item->obj));
		if (item->label){
			edje_object_part_text_set(item->base, "elm.text", item->label);
		}

		if (item->icon){
	//		int ms = 0;

	//		ms = ((double)wd->icon_size * _elm_config->scale);
			evas_object_size_hint_min_set(item->icon, 24, 24);
			evas_object_size_hint_max_set(item->icon, 40, 40);
			edje_object_part_swallow(item->base, "elm.swallow.icon", item->icon);
			evas_object_show(item->icon);
			elm_widget_sub_object_add(obj, item->icon);
		}

		if (item->label && item->icon){
			edje_object_signal_emit(item->base, "elm,state,icon_text", "elm");
		}
	}
	select_check(wd);
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

/**
 * Add a new tab object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Tab
 */
EAPI Evas_Object *elm_tab_add(Evas_Object *parent) {
	Evas_Object *obj = NULL;
	Widget_Data *wd = NULL;
	Evas_Coord x, y, w, h;
	char buf[MAX_ARGS];

	wd = ELM_NEW(Widget_Data);
	wd->evas = evas_object_evas_get(parent);
	if (wd->evas == NULL) return NULL;
	obj = elm_widget_add(wd->evas);
	if(obj == NULL) return NULL;
	elm_widget_type_set(obj, "tab");
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
	wd->edit_disable = EINA_FALSE;
	wd->cur_first_slot = 1;
	//wd->view_slot_num = BASIC_SLOT_NUMBER;
	wd->view_slot_num = 3;

	// initialization
	// edit box
	wd->ebx = edje_object_add(wd->evas);
	if(wd->ebx == NULL)
		return NULL;
	_elm_theme_object_set(obj, wd->ebx, "tab", "dim", "default");
	evas_object_resize(wd->ebx, wd->w, wd->h);
	evas_object_move(wd->ebx, wd->x, wd->y);
	evas_object_hide(wd->ebx);
	evas_object_event_callback_add(wd->ebx, EVAS_CALLBACK_MOUSE_UP, edit_up_cb, wd);

	// initialization
	/* load background edj */
	wd->edje = edje_object_add(wd->evas);
	snprintf(buf, sizeof(buf), "bg_portrait_%d", wd->view_slot_num);
	_elm_theme_object_set(obj, wd->edje, "tab", buf, "default");
	if(wd->edje == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}
	// initialization
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb, wd);
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_UP,  press_up_cb, wd);
	evas_object_show(wd->edje);

	// initialization
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE, _tab_object_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _tab_object_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _tab_object_show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _tab_object_hide, obj);

	// initialization
	//FIXME
	//	evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

	evas_object_smart_member_add(wd->ebx, obj);
	elm_widget_resize_object_set(obj, wd->edje);

	// initialization
	_sizing_eval(obj);

	return obj;
}


/**
 * Set the mode of tab object
 *
 * @param obj The tab object
 * @param mode The mode of tab
 *
 * @ingroup Tab
 */
EAPI void elm_tab_set(Evas_Object *obj, int mode)
{
	int i = 1;
	char buf[MAX_ARGS];
	Widget_Data *wd;
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		printf("Cannot get smart data\n");
		return;
	}

	EINA_LIST_FOREACH(wd->items, l, item){
		edje_object_part_unswallow(wd->edje, item->base);
	}

	evas_object_del(wd->edje);

	evas_object_smart_member_del(wd->ebx);
//	elm_widget_resize_object_set(obj, wd->edje);

	// load edj
	if(mode >= ELM_TAB_PORTRAIT_2 && mode <= ELM_TAB_PORTRAIT_4){
		snprintf(buf, sizeof(buf), "bg_portrait_%d", mode);
	}else if(mode >= ELM_TAB_LANDSCAPE_2 && mode <= ELM_TAB_LANDSCAPE_5){
		snprintf(buf, sizeof(buf), "bg_landscape_%d", mode - LANDSCAPE_GAP);
	}
	wd->edje = edje_object_add(wd->evas);
	if(wd->edje == NULL) {
		printf("Cannot load bg edj\n");
		return;
	}

	_elm_theme_object_set(obj, wd->edje, "tab", buf, elm_widget_style_get(obj));

	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb, wd);
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_UP,  press_up_cb, wd);
	evas_object_show(wd->edje);

	evas_object_smart_member_add(wd->ebx, obj);
	elm_widget_resize_object_set(obj, wd->edje);

	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_RESIZE, _tab_object_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _tab_object_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _tab_object_show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _tab_object_hide, obj);

	EINA_LIST_FOREACH(wd->items, l, item){
		snprintf(buf, sizeof(buf), "slot_%d", item->slot);
		edje_object_part_swallow(wd->edje, buf, item->base);
		i++;
	}

	// set current mode
	if(mode < 10) wd->mode = PORTRAIT;
	else wd->mode = LANDSCAPE;

	wd->view_slot_num = mode % LANDSCAPE_GAP;

	light_check(wd);

	evas_object_resize(wd->edje, wd->w, wd->h);
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
EAPI Elm_Tab_Item *elm_tab_item_add(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), const void *data)
{
	char buf[MAX_ARGS];
	Elm_Tab_Item *it;
	Widget_Data *wd;

	if(obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return NULL;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		printf("Cannot get smart data\n");
		return NULL;
	}

	it = ELM_NEW(Elm_Tab_Item);
	if(it == NULL) {
		printf("Cannot add item");
		return NULL;
	}
	wd->items = eina_list_append(wd->items, it);
	it->obj = obj;
	it->label = eina_stringshare_add(label);
	it->icon = icon;
	it->badge = 0;
	it->func = func;
	it->data = data;
	it->base = edje_object_add(evas_object_evas_get(obj));
	if(it->base == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}
	_elm_theme_object_set(obj, it->base, "tab", "item", elm_widget_style_get(obj));
	elm_widget_sub_object_add(obj, it->base);

	edje_object_signal_callback_add(it->base, "elm,action,click", "elm", tab_item_cb, (void *)it);

	if (it->label){
		edje_object_part_text_set(it->base, "elm.text", it->label);
	}

	if (it->icon){
		//		int ms = 0;

		//		ms = ((double)wd->icon_size * _elm_config->scale);
		evas_object_size_hint_min_set(it->icon, 24, 24);
		evas_object_size_hint_max_set(it->icon, 40, 40);
		edje_object_part_swallow(it->base, "elm.swallow.icon", it->icon);
		evas_object_show(it->icon);
		elm_widget_sub_object_add(obj, it->icon);
	}

	if (it->label && it->icon){
		edje_object_signal_emit(it->base, "elm,state,icon_text", "elm");
	}

	wd->num += 1;
	it->slot = wd->num;
	snprintf(buf, sizeof(buf), "slot_%d", wd->num);
	edje_object_part_swallow(wd->edje, buf, it->base);

	light_check(wd);

	return it;
}

/**
 * Delete item from tab by index
 *
 * @param	it The item of tab

 * @ingroup Tab
 */
EAPI void elm_tab_item_del(Elm_Tab_Item *it)
{
	int check = 0;
	int i = 1;
	char buf[MAX_ARGS];
	Widget_Data *wd;
	const Eina_List *l;
	Elm_Tab_Item *item;

	if(it->obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(it->obj);
	if(wd == NULL) {
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
	wd->items = eina_list_remove(wd->items, it);
	wd->num = wd->num - 1;
}

/**
 * Select item in tab
 *
 * @param	it The item of tab

 * @ingroup Tab
 */
EAPI void elm_tab_item_select(Elm_Tab_Item *it)
{
	_selected_box(it);
}

/**
 * Get the icon of item
 *
 * @param	it The item of tab
 * @return The icon object
 *
 * @ingroup Tab
 */
EAPI Evas_Object *elm_tab_item_icon_get(Elm_Tab_Item *it)
{
	return it->icon;
}

/**
 * Get the label of item
 *
 * @param	it The item of tab
 * @return the label of item
 *
 * @ingroup Tab
 */
EAPI const char *elm_tab_item_label_get(Elm_Tab_Item *it)
{
	return it->label;
}

/**
 * Set the label of item
 *
 * @param	it The item of tab
 * @param	label The label of item
 *
 * @ingroup Tab
 */
EAPI void elm_tab_item_label_set(Elm_Tab_Item *it, const char *label)
{
	if(!it->base) return;

	edje_object_part_text_set(it->base, "elm.text", label);
}

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

	if(obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		printf("Cannot get smart data\n");
		return;
	}

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

	if(obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return;
	}
	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		printf("Cannot get smart data\n");
		return;
	}

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

	if(obj == NULL) {
		printf("Invalid argument: tab object is NULL\n");
		return -1;
	}

	wd = elm_widget_data_get(obj);
	if(wd == NULL) {
		printf("Cannot get smart data\n");
		return -1;
	}

	return wd->edit_disable;
}



///////////////////////////////////////////////////////////////////
//
//  basic utility function
//
////////////////////////////////////////////////////////////////////

static void light_check(Widget_Data *wd)
{
	edje_object_signal_emit(wd->edje, "off_light", "light");

	if(wd->view_slot_num > wd->num) return;

	if(wd->cur_first_slot > 1){
		edje_object_signal_emit(wd->edje, "left", "light");
	}
	if(wd->cur_first_slot + wd->view_slot_num - 1 != wd->num){
		edje_object_signal_emit(wd->edje, "right", "light");
	}
}

static void select_check(Widget_Data *wd)
{
	int i = 1;
//	int selected = -1;
	const Eina_List *l;
	Elm_Tab_Item *item;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(wd->cur_first_slot <= i && wd->cur_first_slot + wd->view_slot_num > i){
			if(item->selected){
				edje_object_signal_emit(item->base, "selected", "elm");
			
			}
//				selected = i;
//			}else{
//				snprintf(buf, sizeof(buf), "show_partition_%d_%d", i, i+1);
//				edje_object_signal_emit(wd->edje, buf, "elm");
//			}
		}else{
			edje_object_signal_emit(item->base, "unselected", "elm");
		}
		i++;
	}

//	snprintf(buf, sizeof(buf), "hide_partition_%d_%d", selected, selected+1);
//	edje_object_signal_emit(wd->edje, buf, "elm");
//	snprintf(buf, sizeof(buf), "hide_partition_%d_%d", selected-1, selected);
//	edje_object_signal_emit(wd->edje, buf, "elm");
}

static int _selected_box(Elm_Tab_Item *it)
{
	int i = 1;
	int check = 0;
	Evas_Object *icon;
	char buf[MAX_ARGS];
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = elm_widget_data_get(it->obj);

	EINA_LIST_FOREACH(wd->items, l, item){
		edje_object_signal_emit(item->base, "unselected", "elm");
		item->selected = EINA_FALSE;

		icon = edje_object_part_swallow_get(item->base, "elm.swallow.icon");
		if(icon){
			if(strcmp(evas_object_type_get(icon), "edje") == 0){
				edje_object_signal_emit(icon, "elm,state,unselected", "elm");
			}else{
				if(_EDJ(icon))
					edje_object_signal_emit(_EDJ(icon), "elm,state,unselected", "elm");
			}
		}

		if(it == item){
			edje_object_signal_emit(it->base, "selected", "elm");
			snprintf(buf, sizeof(buf), "selected_%d", i);
			edje_object_signal_emit(wd->edje, buf, "elm");

			if(icon){
				if(strcmp(evas_object_type_get(icon), "edje") == 0){
					edje_object_signal_emit(icon, "elm,state,selected", "elm");
				}else{
					if(_EDJ(icon))
						edje_object_signal_emit(_EDJ(icon), "elm,state,selected", "elm");
				}
			}
			item->selected = EINA_TRUE;

			check = 1;
		}
		i++;
	}

	if(!check) return EXIT_FAILURE;

	if (it->func) it->func((void *)(it->data), it->obj, it);
	evas_object_smart_callback_call(it->obj, "clicked", it);

//	select_check(wd);

	return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////
//
//  general function
//
////////////////////////////////////////////////////////////////////

static int _move_obj_to_left(Widget_Data *wd)
{
	char buf[MAX_ARGS];
	int first_slot;

	first_slot = wd->cur_first_slot + wd->view_slot_num;
	if(first_slot + wd->view_slot_num > wd->num){
		first_slot = wd->num - wd->view_slot_num +1;
	}

	snprintf(buf, sizeof(buf), "step_%d", first_slot);
	edje_object_signal_emit(wd->edje, buf, "elm");
	wd->cur_first_slot = first_slot;

	select_check(wd);

	light_check(wd);

	return EXIT_SUCCESS;
}

static int _move_obj_to_right(Widget_Data *wd)
{
	char buf[MAX_ARGS];
	int first_slot;

	first_slot = wd->cur_first_slot - wd->view_slot_num;
	if(first_slot < 1){
		first_slot = 1;
	}

	snprintf(buf, sizeof(buf), "step_%d", first_slot);
	edje_object_signal_emit(wd->edje, buf, "elm");
	wd->cur_first_slot = first_slot;

	select_check(wd);

	light_check(wd);

	return EXIT_SUCCESS;
}

static void edit_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	char buf[MAX_ARGS];
	Evas_Object *icon;
	Widget_Data *wd = (Widget_Data *)data;
	const Eina_List *l;
	Elm_Tab_Item *item;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->selected){
			edje_object_signal_emit(item->base, "selected", "elm");
			icon = edje_object_part_swallow_get(item->base, "elm.swallow.icon");
			if(icon){
				if(strcmp(evas_object_type_get(icon), "edje") == 0){
					edje_object_signal_emit(icon, "elm,state,selected", "elm");
				}else{
					if(_EDJ(icon))
						edje_object_signal_emit(_EDJ(icon), "elm,state,selected", "elm");
				}
			}
		}
		edje_object_signal_callback_add(item->base, "elm,action,click", "elm", tab_item_cb, item);
		evas_object_event_callback_del(item->base, EVAS_CALLBACK_MOUSE_DOWN,  item_down_cb);
	}

	evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb);
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb, wd);
	evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_UP,  press_up_cb);
	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_UP,  press_up_cb, wd);

	snprintf(buf, sizeof(buf), "step_%d", wd->cur_first_slot);
	edje_object_signal_emit(wd->edje, buf, "elm");

	light_check(wd);
	evas_object_hide(wd->ebx);
}

static Eina_Bool item_move_cb(void *data, int type, void *event_info)
{
	Evas_Object *obj;
	Ecore_Event_Mouse_Move *ev = event_info;
	Evas_Coord x, y, w, h, mx, my, mw, mh, ex, ey;
	Widget_Data *wd = (Widget_Data *)data;

	obj = wd->moving_obj;

	evas_object_geometry_get(obj, NULL, NULL ,&w, &h);
	evas_object_geometry_get(wd->edje, &ex, &ey , NULL, NULL);
	edje_object_part_geometry_get(wd->edje, "bg_b", &mx, &my ,&mw, &mh);

	if(ev->x < ex + mx + w/2) x = ex + mx;
	else if(ev->x > ex + mx + mw - w/2) x = ex + mx + mw - w;
	else x = ev->x - w/2;

	if(ev->y < ey + my + h) y = ey + my;
	else if(ev->y > ey + my + mh) y = ey + my + mh - h;
	else y = ev->y - h;

	evas_object_move(obj, x, y);

	return 1;
}

static int sort_cb(const void *d1, const void *d2)
{
	Elm_Tab_Item *item1, *item2;

	item1 = (Elm_Tab_Item *)d1;
	item2 = (Elm_Tab_Item *)d2;

	return item1->slot > item2->slot ? 1 : -1;
}

static Eina_Bool item_up_cb(void *data, int type, void *event_info)
{
	Evas_Object *obj;
	Ecore_Event_Mouse_Button *ev = event_info;
	char buf[MAX_ARGS];
	Evas_Coord x, y, w, h;
	int i = 0;
	const Eina_List *l;
	Elm_Tab_Item *item;
	Elm_Tab_Item *edit_to_item, *edit_from_item;
	Widget_Data *wd = (Widget_Data *)data;

	obj = wd->moving_obj;

	wd->edit_to = 0;
	EINA_LIST_FOREACH(wd->items, l, item){
		i++;
		if(item->base == obj) continue;
		evas_object_geometry_get(item->base, &x, &y, &w, &h);
		if(x < ev->x && ev->x < x+w && y < ev->y && ev->y < y+h){
			wd->edit_to = i;
		}
	}

	if(wd->edit_to > 0 && wd->edit_to <= wd->num){

		edit_to_item = eina_list_nth(wd->items, wd->edit_to-1);
		edit_from_item = eina_list_nth(wd->items, wd->edit_from-1);

		edje_object_part_unswallow(wd->edje, edit_to_item->base);
		snprintf(buf, sizeof(buf), "slot_%d", wd->edit_from);
		edje_object_part_swallow(wd->edje, buf, edit_to_item->base);
		edit_to_item->slot = wd->edit_from;

		snprintf(buf, sizeof(buf), "slot_%d", wd->edit_to);
		edje_object_part_swallow(wd->edje, buf, edit_from_item->base);
		edit_from_item->slot = wd->edit_to;

		wd->items = eina_list_sort(wd->items, eina_list_count(wd->items), sort_cb);

	}else{
		wd->edit_to = wd->edit_from;
		snprintf(buf, sizeof(buf), "slot_%d", wd->edit_from);
		edje_object_part_swallow(wd->edje, buf, obj);
	}

	ecore_event_handler_del(wd->move_event);
	wd->move_event = NULL;
	ecore_event_handler_del(wd->up_event);
	wd->up_event = NULL;

	return EXIT_SUCCESS;
}

static void item_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = event_info;
	Evas_Coord x, y, w, h;
	Widget_Data *wd = (Widget_Data *)data;
	const Eina_List *l;
	Elm_Tab_Item *item;

	wd->moving_obj = obj;

	EINA_LIST_FOREACH(wd->items, l, item){
		if(item->base == obj) wd->edit_from = item->slot;
	}

	edje_object_part_unswallow(wd->edje, obj);
	evas_object_geometry_get(obj, &x, &y, &w, &h);
	evas_object_move(obj, ev->output.x - w/2 , ev->output.y - h);
	edje_object_signal_emit(obj, "edit", "elm");

	wd->up_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, item_up_cb, (void *)wd);
	wd->move_event = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, item_move_cb, (void *)wd);
}

static void edit_mode(void *data)
{
	Evas_Object *icon;
	const Eina_List *l;
	Elm_Tab_Item *item;
	Widget_Data *wd = (Widget_Data *)data;

	edje_object_signal_emit(wd->edje, "edit", "elm");

	if((int)(wd->num/wd->view_slot_num) < 2){
		edje_object_signal_emit(wd->edje, "edit_2", "elm");
	}else{
		edje_object_signal_emit(wd->edje, "edit_3", "elm");
	}

	// delete normal mode callback & add normal mode callback
	EINA_LIST_FOREACH(wd->items, l, item){
		edje_object_signal_emit(item->base, "unselected", "elm");
		icon = edje_object_part_swallow_get(item->base, "elm.swallow.icon");
		if(icon){
			if(strcmp(evas_object_type_get(icon), "edje") == 0){
				edje_object_signal_emit(icon, "elm,state,unselected", "elm");
			}else{
				if(_EDJ(icon))
					edje_object_signal_emit(_EDJ(icon), "elm,state,unselected", "elm");
			}
		}
		edje_object_signal_callback_del(item->base, "elm,action,click", "elm", tab_item_cb);
		evas_object_event_callback_add(item->base, EVAS_CALLBACK_MOUSE_DOWN, item_down_cb, wd);
	}
	evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb);
	evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_UP,  press_up_cb);
	evas_object_show(wd->ebx);

	edje_object_signal_emit(wd->edje, "off_light", "light");
}


static Eina_Bool tab_timer_cb(void* data)
{
	Widget_Data *wd = (Widget_Data *)data;

	if(wd->time > KEYDOWN_INTERVAL){
		evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_MOVE,  press_move_cb);

		if(!wd->edit_disable) edit_mode(wd);

		if(wd->timer){
			ecore_timer_del(wd->timer);
			wd->timer = NULL;
		}

	} else {
		wd->time += 0.1;
	}

	return EXIT_FAILURE;
}

static void press_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = event_info;
	Widget_Data *wd = (Widget_Data *)data;

	if(abs(wd->tab_down_x-ev->output.x) > 10 || abs(wd->tab_down_y-ev->output.y) > 10){
		if(wd->timer){
			ecore_timer_del(wd->timer);
			wd->timer = NULL;
		}
	}
}

static void press_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = event_info;
	Widget_Data *wd = (Widget_Data *)data;

	wd->flag = EINA_TRUE;
	wd->tab_down_x = ev->output.x;
	wd->tab_down_y = ev->output.y;

	wd->time = 0;
	wd->timer = ecore_timer_add(0.1, tab_timer_cb, wd);

	evas_object_event_callback_add(wd->edje, EVAS_CALLBACK_MOUSE_MOVE,  press_move_cb, wd);
}

static void press_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	Widget_Data *wd = (Widget_Data *)data;
	Evas_Coord x, y, w, h;

	evas_object_event_callback_del(wd->edje, EVAS_CALLBACK_MOUSE_MOVE,  press_move_cb);

	if(wd->time < KEYDOWN_INTERVAL){
		if(wd->timer){
			ecore_timer_del(wd->timer);
			wd->timer = NULL;
		}
	} else {
		return;
	}

	evas_object_geometry_get(wd->edje, &x, &y, &w, &h);

	if(ev->output.y > y+(h*1.5) && abs(ev->output.x - wd->tab_down_x) < wd->w/10){
		if(!wd->edit_disable) edit_mode(wd);
	}else{
		// return if dont need to move
		if(wd->num <= wd->view_slot_num) return;

		if(wd->flag == EINA_TRUE){
			if(abs(wd->tab_down_y - ev->output.y) < wd->h){
				if((wd->tab_down_x - ev->output.x ) > wd->w / 4){
					_move_obj_to_left(wd);
				}else if((ev->output.x - wd->tab_down_x) > wd->w / 4){
					_move_obj_to_right(wd);
				}
			}
		}
	}
	wd->flag = EINA_FALSE;
}

void tab_item_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Elm_Tab_Item *it = (Elm_Tab_Item *)data;
	if(it->obj == NULL)
		return;
	Widget_Data *wd = elm_widget_data_get(it->obj);
	if(wd == NULL)
		return;

	if(wd->time > KEYDOWN_INTERVAL) return;
	_selected_box(it);
}

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
}


static void _tab_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;
	Evas_Coord w, h;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_geometry_get(wd->edje, NULL, NULL, &w, &h);

	wd->w = w;
	wd->h = h;

	evas_object_resize(wd->edje, w, h);

	evas_object_geometry_get(wd->parent, NULL, NULL, &w, &h);
	evas_object_resize(wd->ebx, w, h);
}


static void _tab_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	edit_up_cb(wd, NULL, NULL, NULL);

	evas_object_show(wd->edje);
}



static void _tab_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    DBG("%s", __func__);

	Widget_Data *wd;

	if(!data) return;
	wd = elm_widget_data_get((Evas_Object *)data);
	if(!wd) return;

	evas_object_hide(wd->edje);
	evas_object_hide(wd->ebx);
}

