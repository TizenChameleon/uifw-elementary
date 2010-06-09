#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Buttonpicker Buttonpicker
 *
 * This is a buttonpicker.
 */


#define LONGPRESS_START_INTERVAL (0.7)
#define UPDATE_INTERVAL (0.1)



struct _Buttonpicker_Item {
	Evas_Object *buttonpicker;
	const char *label;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	void *data;
	int walking;
  int delete_me : 1;
};

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data {
	Evas_Object *base;
	Eina_List *items;
	Eina_List *current;
	int item_num;
	unsigned char ani_lock;

	Ecore_Timer *top_timer;
	bool top_pressed;
	bool top_count;
	
	Ecore_Timer *bottom_timer;
	bool bottom_pressed;
	bool bottom_count;
};



static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _update_buttonpicker(Evas_Object *obj);
static void	_changed(Evas_Object *obj);
static void	_item_del(Elm_Buttonpicker_Item *item);



static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->items) {
		Elm_Buttonpicker_Item *item;
		EINA_LIST_FREE(wd->items, item) {
			if (item->label)
				eina_stringshare_del(item->label);
			free(item);
		}
	}
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_elm_theme_object_set(obj, wd->base, "buttonpicker", "base", elm_widget_style_get(obj));
	_update_buttonpicker(obj);
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

	if (!wd) return;
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_update_buttonpicker(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char *label = NULL;
	Elm_Buttonpicker_Item *item;

	if (!wd) return;
	if (wd->current) {
		item = eina_list_data_get(wd->current);
		if (item && item->label)
			label = item->label;
	}

	if (label)
		edje_object_part_text_set(wd->base, "elm.text", label);

	_sizing_eval(obj);
}

static void
_set_next(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);	
	if (!wd || !wd->current || !wd->items) return;	
	if (eina_list_count(wd->items) <= 1) return;
	wd->current = eina_list_next(wd->current);
	if (!wd->current) {
		wd->current = wd->items;
		evas_object_smart_callback_call(obj, "overflowed", eina_list_data_get(wd->current));
	}
	_update_buttonpicker(obj);
	_changed(obj);
}

static void
_top_timer_del(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if(wd->top_timer){
		ecore_timer_del(wd->top_timer);
		wd->top_timer = NULL;	
	}
	wd->top_count = FALSE;		
}

static int 
_top_timer_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return 0;
	
	if (wd->top_pressed == FALSE) {
		_top_timer_del(obj);	
		return 0;
	}

	if(wd->top_count == FALSE){	// set the timer interval again
		wd->top_count = TRUE;
		ecore_timer_interval_set(wd->top_timer, UPDATE_INTERVAL);
	}else{ 
		_set_next(obj);
	}
	
	return 1;
}

static void
_top_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	wd->top_pressed = TRUE;
	edje_object_signal_emit(obj, "elm,bg,top,pressed", "elm.bg.top");
	_top_timer_del(data);		
 	wd->top_timer = ecore_timer_add(LONGPRESS_START_INTERVAL, (void *)_top_timer_cb, data);
}

static void
_top_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);	
	if (!wd ) return;
	
	wd->top_pressed = FALSE;
	edje_object_signal_emit(obj, "elm,bg,top,released", "elm.bg.top");
	_top_timer_del(data);	
	_set_next(data);
}


static void
_set_prev(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);		
	if (!wd || !wd->current || !wd->items) return;
	if (eina_list_count(wd->items) <= 1) return;
	if (wd->current == wd->items) {
		wd->current = eina_list_last(wd->items);
		evas_object_smart_callback_call(obj, "underflowed", eina_list_data_get(wd->current));
	} else {
		wd->current = eina_list_prev(wd->current);
	}
	_update_buttonpicker(obj);
	_changed(obj);
}

static void
_bottom_timer_del(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if(wd->bottom_timer){
		ecore_timer_del(wd->bottom_timer);
		wd->bottom_timer = NULL;	
	}
	wd->bottom_count = FALSE;		
}

static int 
_bottom_timer_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return 0;
	
	if (wd->bottom_pressed == FALSE) {
		_bottom_timer_del(obj);	
		return 0;
	}

	if(wd->bottom_count == FALSE){	// set the timer interval again
		wd->bottom_count = TRUE;
		ecore_timer_interval_set(wd->bottom_timer, UPDATE_INTERVAL);
	}else{ 
		_set_prev(obj);
	}
	
	return 1;
}

static void
_bottom_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;
	
	wd->bottom_pressed = TRUE;
	edje_object_signal_emit(obj, "elm,bg,bottom,pressed", "elm.bg.bottom");
	_bottom_timer_del(data);		
 	wd->bottom_timer = ecore_timer_add(LONGPRESS_START_INTERVAL, (void *)_bottom_timer_cb, data);
}

static void
_bottom_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	wd->bottom_pressed = FALSE;
	edje_object_signal_emit(obj, "elm,bg,bottom,released", "elm.bg.bottom");
	_bottom_timer_del(data);	
	_set_prev(data);
}

static void
_changed(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Buttonpicker_Item *item;
	if (!wd || !wd->current) return;

	item = eina_list_data_get(wd->current);
	if (!item) return;

	item->walking++;

	if (item->func)
	  item->func(item->data, obj, item);
	if (!item->delete_me) 
	  evas_object_smart_callback_call(obj, "changed", eina_list_data_get(wd->current));

	item->walking--;

	if (!item->walking && item->delete_me)
	  _item_del(item);
}

static void
_item_del(Elm_Buttonpicker_Item *item)
{
	Eina_List *l;
	Elm_Discpicker_Item *_item;
	Widget_Data *wd;

	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			if (wd->current == l)
				wd->current = eina_list_prev(wd->current);
			wd->items = eina_list_remove(wd->items, _item);
			if (!wd->current)
				wd->current = wd->items;
			_update_buttonpicker(item->buttonpicker);
			free(_item);
			break;
		}
	}
}

/**
 * Add a new buttonpicker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Buttonpicker
 */
EAPI Evas_Object *
elm_buttonpicker_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "buttonpicker");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "buttonpicker", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->top_pressed = FALSE;
	wd->top_count = FALSE;
	wd->bottom_pressed = FALSE;
	wd->bottom_count = FALSE;

	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.bg.top", _top_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,up,1", "elm.bg.top", _top_up, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.bg.bottom", _bottom_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,up,1", "elm.bg.bottom", _bottom_up, obj);

	_sizing_eval(obj);
	return obj;
}

/**
 * Select next item of buttonpicker
 *
 * @param obj The buttonpicker object
 *
 * @ingroup Buttonpicker
 */
EAPI void
elm_buttonpicker_next(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_top_up(obj, wd->base, "mouse,up,1", "elm.bg.top");
}

/**
 * Select previous item of buttonpicker
 *
 * @param obj The buttonpicker object
 *
 * @ingroup Buttonpicker
 */
EAPI void
elm_buttonpicker_prev(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_bottom_up(obj, wd->base, "mouse,up,1", "elm.bg.bottom");
}

/**
 * Append item to the end of buttonpicker
 *
 * @param obj The buttonpicker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_item_append(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Buttonpicker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Buttonpicker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->buttonpicker = obj;
		wd->items = eina_list_append(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_buttonpicker(obj);
		}
	}
	return item;
}

/**
 * Prepend item at start of buttonpicker
 *
 * @param obj The buttonpicker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_item_prepend(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Buttonpicker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Buttonpicker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->buttonpicker = obj;
		wd->items = eina_list_prepend(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_buttonpicker(obj);
		}
	}
	return item;
}

/**
 * Get a list of items in the buttonpicker
 *
 * @param obj The buttonpicker object
 * @return The list of items, or NULL if none
 *
 * @ingroup Buttonpicker
 */
EAPI const Eina_List *
elm_buttonpicker_items_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->items;
}

/**
 * Get the first item in the buttonpicker
 *
 * @param obj The buttonpicker object
 * @return The first item, or NULL if none
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_first_item_get(Evas_Object *obj)
{
	Widget_Data *wd;

	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the buttonpicker
 *
 * @param obj The buttonpicker object
 * @return The last item, or NULL if none
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_last_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the selected item in the buttonpicker
 *
 * @param obj The buttonpicker object
 * @return The selected item, or NULL if none
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_selected_item_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->current) return NULL;
	return eina_list_data_get(wd->current);
}

/**
 * Set the selected state of an item
 *
 * @param item The item
 *
 * @ingroup Buttonpicker
 */
EAPI void
elm_buttonpicker_item_selected_set(Elm_Buttonpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Buttonpicker_Item *_item;

	if (!item) return;
	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			wd->current = l;
			_update_buttonpicker(item->buttonpicker);
		}
	}
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Buttonpicker
 */
EAPI void
elm_buttonpicker_item_del(Elm_Buttonpicker_Item *item)
{
	if (!item) return;
	if (item->walking > 0)
	  item->delete_me = EINA_TRUE;
	else
	  _item_del(item);
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Buttonpicker
 */
EAPI const char *
elm_buttonpicker_item_label_get(Elm_Buttonpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Buttonpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item)
			return item->label;

	return NULL;
}

/**
 * Set the label of a given item
 *
 * @param item The item
 * @param label The text label string in UTF-8
 *
 * @ingroup Buttonpicker
 */
EAPI void
elm_buttonpicker_item_label_set(Elm_Buttonpicker_Item *item, const char *label)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Buttonpicker_Item *_item;

	if (!item || !label) return;
	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd || !wd->items) return;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			eina_stringshare_del(item->label);
			item->label = eina_stringshare_add(label);
			_update_buttonpicker(item->buttonpicker);
		}
}

/**
 * Get the previous item in the buttonpicker
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_item_prev(Elm_Buttonpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Buttonpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			l = eina_list_prev(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}

/**
 * Get the next item in the buttonpicker
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Buttonpicker
 */
EAPI Elm_Buttonpicker_Item *
elm_buttonpicker_item_next(Elm_Buttonpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Buttonpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->buttonpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			l = eina_list_next(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}
