#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Picker Picker
 * @ingroup Elementary
 *
 * This is a picker.
 */


#define PICKER_DRAG_BOUND (10)

struct _Picker_Item {
	Evas_Object *picker;
	const char *label;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	void *data;
};

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data {
	Evas_Object *base;
	Eina_List *items;
	Eina_List *current;
	int item_num;
	unsigned char ani_lock;
};

Evas_Event_Mouse_Down g_down_ev;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _update_picker(Evas_Object *obj);
static void _up_pressed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _down_pressed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _item_up(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _item_down(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _changed(void *data);
static Eina_Bool _animator(void *data);
static void _flick(Evas_Object *obj, float speed);
static void _flick_init(Evas_Object *obj);
static void _mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->items) {
		Elm_Picker_Item *item;
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
	_elm_theme_object_set(obj, wd->base, "picker", "base", elm_widget_style_get(obj));
	_flick_init(obj);
	_update_picker(obj);
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
_update_picker(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char *label = NULL;
	Elm_Picker_Item *item;

	if (!wd) return;
	if (wd->current) {
		item = eina_list_data_get(wd->current);
		if (item && item->label)
			label = item->label;
	}

	if (label) {
		edje_object_signal_emit(wd->base, "elm,state,text,visible", "elm");
		edje_object_part_text_set(wd->base, "elm.text.top", label);
		edje_object_part_text_set(wd->base, "elm.text.bottom", label);
	} else {
		edje_object_signal_emit(wd->base, "elm,state,text,hidden", "elm");
	}

	edje_object_message_signal_process(wd->base);
	_sizing_eval(obj);
}

static void
_up_pressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd->items) return;
	evas_object_data_set(wd->base, "ani_cnt", (void *)1);
	if (!wd->ani_lock)
		_animator(data);
}

static void
_down_pressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd->items) return;
	evas_object_data_set(wd->base, "ani_cnt", (void *)(-1));
	if (!wd->ani_lock)
		_animator(data);
}

static void
_item_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->current || !wd->items) return;
	if (eina_list_count(wd->items) <= 1) return;
	_animator(data);
}

static void
_item_half_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->current || !wd->items) return;
	if (eina_list_count(wd->items) <= 1) return;
	wd->current = eina_list_next(wd->current);
	if (!wd->current) {
		wd->current = wd->items;
		evas_object_smart_callback_call(data, "overflowed", eina_list_data_get(wd->current));
	}
	_update_picker(data);
}

static void
_item_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->current || !wd->items) return;
	if (eina_list_count(wd->items) <= 1) return;
	_animator(data);
}

static void
_item_half_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->current || !wd->items) return;
	if (eina_list_count(wd->items) <= 1) return;

	if (wd->current == wd->items) {
		wd->current = eina_list_last(wd->items);
		evas_object_smart_callback_call(data, "underflowed", eina_list_data_get(wd->current));
	} else {
		wd->current = eina_list_prev(wd->current);
	}
	_update_picker(data);
}



static void
_changed(void *data)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Elm_Picker_Item *item;

	evas_object_smart_callback_call(data, "changed", eina_list_data_get(wd->current));

	item = eina_list_data_get(wd->current);
	if (item->func) {
		item->func(item->data, data, item);
	}
}

static Eina_Bool
_animator(void *data)
{
	Evas_Object *obj = data;
	Widget_Data *wd = elm_widget_data_get(obj);
	int cnt;
	float time;
	signed char direction;
	Edje_Message_Float msg;

	if (eina_list_count(wd->items) <= 1) {
		evas_object_data_set(wd->base, "ani_cnt", (void *)0);
		wd->ani_lock = 0;
		_changed(data);
		return 0;
	}

	cnt = (int)evas_object_data_get(wd->base, "ani_cnt");

	if (cnt > 0) {
		direction = 1;
		time = 0.45  / sqrt(cnt);
	} else if (cnt == 0) {
		wd->ani_lock = 0;
		_changed(data);
		return 0;
	} else {
		direction = -1;
		time = 0.45  / sqrt(-cnt);
	}


	if (time < 0.20)
		time = 0.20;

	msg.val = time * direction;
	edje_object_message_send(wd->base, EDJE_MESSAGE_FLOAT, 1, &msg);

	cnt -= direction;
	evas_object_data_set(wd->base, "ani_cnt", (void *)cnt);
	return 0;
}

static void
_flick(Evas_Object *obj, float speed)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int cnt, orig_cnt = 0, item_cnt;


	item_cnt = eina_list_count(wd->items);
	if (eina_list_count(wd->items) <= 1) return;

	if (item_cnt < 3) {
		cnt = speed < 0 ? -1 : 1;
	} else {
		cnt = (int)(speed * 3);
		orig_cnt = (int)evas_object_data_get(wd->base, "ani_cnt");
	}

	if (wd->ani_lock) {
		cnt = cnt + orig_cnt;
	} else {
		wd->ani_lock = 1;
		ecore_idler_add(_animator, obj);
	}

	evas_object_data_set(wd->base, "ani_cnt", (void *)cnt);
}

static void
_flick_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *flick;
	flick = (Evas_Object *)edje_object_part_object_get(wd->base, "elm.rect.flick");
	evas_object_event_callback_add(flick, EVAS_CALLBACK_MOUSE_UP, _mouse_up, obj);
	evas_object_event_callback_add(flick, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, obj);
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Up *ev;
	unsigned int dur;
	Evas_Coord distance;

	if (!wd) return;
	ev = event_info;

	dur = ev->timestamp - g_down_ev.timestamp;
	distance = g_down_ev.canvas.y - ev->canvas.y;

	if ((dur && dur > 1000) || (distance < 10 && distance > -10)) {
		if (wd->ani_lock)
        {
			if ((int)evas_object_data_get(wd->base, "ani_cnt") < 0)
				evas_object_data_set(wd->base, "ani_cnt", (void *)-1);
			else
				evas_object_data_set(wd->base, "ani_cnt", (void *)1);
        }
		return;
	}
	_flick(data, (float)distance / dur);
}

static void
_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;
	memcpy(&g_down_ev, event_info, sizeof(Evas_Event_Mouse_Down));
}

/**
 * Add a new picker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Picker
 */
EAPI Evas_Object *
elm_picker_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "picker");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "picker", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "elm.rect.button.up", _up_pressed, obj);
	edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "elm.rect.button.down", _down_pressed, obj);
	edje_object_signal_callback_add(wd->base, "pressed", "up", _item_up, obj);
	edje_object_signal_callback_add(wd->base, "pressed", "down", _item_down, obj);
	edje_object_signal_callback_add(wd->base, "half", "up", _item_half_up, obj);
	edje_object_signal_callback_add(wd->base, "half", "down", _item_half_down, obj);

	_flick_init(obj);
	_sizing_eval(obj);
	return obj;
}

/**
 * Select next item of picker
 *
 * @param obj The picker object
 *
 * @ingroup Picker
 */
EAPI void
elm_picker_next(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_up_pressed(obj, wd->base, "mouse,clicked,1", "elm.rect.button.up");
}

/**
 * Select previous item of picker
 *
 * @param obj The picker object
 *
 * @ingroup Picker
 */
EAPI void
elm_picker_prev(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_down_pressed(obj, wd->base, "mouse,clicked,1", "elm.rect.button.down");
}

/**
 * Append item to the end of picker
 *
 * @param obj The picker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_item_append(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Picker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Picker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->picker = obj;
		wd->items = eina_list_append(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_picker(obj);
		}
		if (eina_list_count(wd->items) == 2)
			edje_object_signal_emit(wd->base, "elm,state,button,visible", "elm");
	}
	return item;
}

/**
 * Prepend item at start of picker
 *
 * @param obj The picker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_item_prepend(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Picker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Picker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->picker = obj;
		wd->items = eina_list_prepend(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_picker(obj);
		}
		if (eina_list_count(wd->items) == 2)
			edje_object_signal_emit(wd->base, "elm,state,button,visible", "elm");
	}
	return item;
}

/**
 * Get a list of items in the picker
 *
 * @param obj The picker object
 * @return The list of items, or NULL if none
 *
 * @ingroup Picker
 */
EAPI const Eina_List *
elm_picker_items_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->items;
}

/**
 * Get the first item in the picker
 *
 * @param obj The picker object
 * @return The first item, or NULL if none
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_first_item_get(Evas_Object *obj)
{
	Widget_Data *wd;

	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the picker
 *
 * @param obj The picker object
 * @return The last item, or NULL if none
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_last_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the selected item in the picker
 *
 * @param obj The picker object
 * @return The selected item, or NULL if none
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_selected_item_get(Evas_Object *obj)
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
 * @ingroup Picker
 */
EAPI void
elm_picker_item_selected_set(Elm_Picker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item) return;
	wd = elm_widget_data_get(item->picker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			wd->current = l;
			_update_picker(item->picker);
		}
	}
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Picker
 */
EAPI void
elm_picker_item_del(Elm_Picker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item) return;
	wd = elm_widget_data_get(item->picker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			if (wd->current == l)
				wd->current = eina_list_prev(wd->current);
			wd->items = eina_list_remove(wd->items, _item);
			if (!wd->current)
				wd->current = wd->items;
			_update_picker(item->picker);

			if (eina_list_count(wd->items) <= 1)
				edje_object_signal_emit(wd->base, "elm,state,button,hidden", "elm");
			break;
		}
	}
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Picker
 */
EAPI const char *
elm_picker_item_label_get(Elm_Picker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->picker);
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
 * @ingroup Picker
 */
EAPI void
elm_picker_item_label_set(Elm_Picker_Item *item, const char *label)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item || !label) return;
	wd = elm_widget_data_get(item->picker);
	if (!wd || !wd->items) return;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			eina_stringshare_del(item->label);
			item->label = eina_stringshare_add(label);
			_update_picker(item->picker);
		}
}

/**
 * Get the previous item in the picker
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_item_prev(Elm_Picker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->picker);
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
 * Get the next item in the picker
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Picker
 */
EAPI Elm_Picker_Item *
elm_picker_item_next(Elm_Picker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Picker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->picker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			l = eina_list_next(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}
