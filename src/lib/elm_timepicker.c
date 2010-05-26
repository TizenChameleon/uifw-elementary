#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Timepicker Timepicker
 *
 * This is a time picker.
 */


#define PICKER_DRAG_BOUND (10)

#define MAX_HOUR (12)
#define MAX_HOUR2 (23)
#define MAX_DIGIT (9)
#define MAX_DIGIT2 (59)

enum {
	PICKER_HRS,
	PICKER_MIN,
	PICKER_SUB,
	PICKER_AMPM,
	PICKER_MAX
};

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *pickers[PICKER_MAX];

	int hrs, min, sec;
	Eina_Bool is_pm;

	Eina_Bool ampm;
	Eina_Bool seconds;
};

static inline Eina_Bool _is_time_valid(int hrs, int min, int sec);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _update_ampm(Evas_Object *obj);
static void _update_seconds(Evas_Object *obj);
static void _update_picker(Evas_Object *picker, int nth);
static void _update_all(Evas_Object *obj);
static void _hrs_reload(Evas_Object *picker, Eina_Bool ampm);
static void _digit_reload(Evas_Object *picker, Eina_Bool seconds, int max);
static Evas_Object *_hrs_add(Evas_Object *ly, const char *part, Eina_Bool ampm);
static Evas_Object *_digit_add(Evas_Object *ly, const char *part, unsigned char max);
static Evas_Object *_ampm_add(Evas_Object *ly, const char *part);
static void _callback_init(Evas_Object *obj);
static void _pickers_add(Evas_Object *obj);
static void _pickers_del(Evas_Object *obj);
static void _changed(Evas_Object *picker, Evas_Object *child, Evas_Object *obj);
static void _overflow_cb(void *data, Evas_Object *obj, void *event_info);
static void _underflow_cb(void *data, Evas_Object *obj, void *event_info);
static void _hrs_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _min_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _sub_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _ampm_changed_cb(void *data, Evas_Object *obj, void *event_info);

static inline Eina_Bool
_is_time_valid(int hrs, int min, int sec)
{
	if (hrs < 0 || hrs > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
		return EINA_FALSE;
	return EINA_TRUE;
}

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_pickers_del(obj);
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_pickers_del(obj);
	_elm_theme_object_set(obj, wd->base, "timepicker", "base", elm_widget_style_get(obj));
	_pickers_add(obj);

	_update_ampm(obj);
	_update_seconds(obj);
	_sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{

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
_update_ampm(Evas_Object *obj)
{
	const char *sig;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->ampm)
		sig = "elm,state,ampm,visible";

	else
		sig = "elm,state,ampm,hidden";

	_hrs_reload(wd->pickers[PICKER_HRS], wd->ampm);
	edje_object_signal_emit(wd->base, sig, "elm");
	_update_all(obj);
}

static void
_update_seconds(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char *sig;
	if (!wd) return;

	if (wd->seconds)
		sig = "elm,state,sec,visible";
	else
		sig = "elm,state,sec,hidden";

	_digit_reload(wd->pickers[PICKER_MIN], wd->seconds, 5);
	_digit_reload(wd->pickers[PICKER_SUB], wd->seconds, MAX_DIGIT);

	edje_object_signal_emit(wd->base, sig, "elm");
	_update_all(obj);
}

static void
_update_picker(Evas_Object *picker, int nth)
{
	const Eina_List *l;
	Elm_Picker_Item *item;
	int i;
	l = elm_picker_items_get(picker);
	for (i = 0; i < nth; i++) {
		l = l->next;
		if (!l) break;
	}

	item = eina_list_data_get(l);

	elm_picker_item_selected_set(item);
}

static void
_update_all(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int hrs;
	if (!wd) return;

	hrs = wd->hrs;
	if (wd->ampm)
		hrs = wd->hrs % 12;

	_update_picker(wd->pickers[PICKER_HRS],  hrs);
	if (wd->hrs >= 12)
		wd->is_pm = 1;
	else
		wd->is_pm = 0;
	_update_picker(wd->pickers[PICKER_AMPM], wd->is_pm);

	if (wd->seconds) {
		_update_picker(wd->pickers[PICKER_MIN],  wd->min);
		_update_picker(wd->pickers[PICKER_SUB],  wd->sec);
	} else {
		_update_picker(wd->pickers[PICKER_MIN],  wd->min / 10);
		_update_picker(wd->pickers[PICKER_SUB],  wd->min % 10);
	}
	_sizing_eval(obj);
}

static void
_hrs_reload(Evas_Object *picker, Eina_Bool ampm)
{
	int i, max;
	char buf[8];
	Elm_Picker_Item *item;

	item = elm_picker_first_item_get(picker);

	while (item) {
		elm_picker_item_del(item);
		item = elm_picker_first_item_get(picker);
	}

	if (ampm) {
		i = 1;
		snprintf(buf, 8, "%02d", MAX_HOUR);
		elm_picker_item_append(picker, buf, NULL, NULL);
		max = MAX_HOUR - 1;
	} else {
		i = 0;
		max = MAX_HOUR2;
	}

	for (; i <= max; i++) {
		snprintf(buf, 8, "%02d", i);
		elm_picker_item_append(picker, buf, NULL, NULL);
	}
}

static void
_digit_reload(Evas_Object *picker, Eina_Bool seconds, int max)
{
	int i;
	char buf[8];
	Elm_Picker_Item *item;
	const char *exp;

	item = elm_picker_first_item_get(picker);

	while (item) {
		elm_picker_item_del(item);
		item = elm_picker_first_item_get(picker);
	}

	if (seconds) max = MAX_DIGIT2;
	if (max >= 10) exp = "%02d";
	else exp = "%d";

	for (i = 0; i <= max; i++) {
		snprintf(buf, 8, exp, i);
		elm_picker_item_append(picker, buf, NULL, NULL);
	}
}

static Evas_Object *
_hrs_add(Evas_Object *ly, const char *part, Eina_Bool ampm)
{
	int i, max;
	char buf[8];
	Evas_Object *eo;
	eo = elm_picker_add(ly);

	if (ampm) {
		i = 1;
		snprintf(buf, 8, "%02d", MAX_HOUR);
		elm_picker_item_append(eo, buf, NULL, NULL);
		max = MAX_HOUR - 1;
	} else {
		i = 0;
		max = MAX_HOUR2;
	}

	for (; i <= max; i++) {
		snprintf(buf, 8, "%02d", i);
		elm_picker_item_append(eo, buf, NULL, NULL);
	}
	if (part)
		edje_object_part_swallow(ly, part, eo);
	return eo;
}

static Evas_Object *
_digit_add(Evas_Object *ly, const char *part, unsigned char max)
{
	int i;
	char buf[8];
	Evas_Object *eo;
	eo = elm_picker_add(ly);
	for (i = 0; i <= max; i++) {
		snprintf(buf, 8, "%d", i);
		elm_picker_item_append(eo, buf, NULL, NULL);
	}
	if (part)
		edje_object_part_swallow(ly, part, eo);
	return eo;
}

static Evas_Object *
_ampm_add(Evas_Object *ly, const char *part)
{
	Evas_Object *eo;
	eo = elm_picker_add(ly);
	elm_object_style_set(eo, "timepicker/ampm");
	elm_picker_item_append(eo, "AM", NULL, NULL);
	elm_picker_item_append(eo, "PM", NULL, NULL);
	if (part)
		edje_object_part_swallow(ly, part, eo);
	return eo;
}

static void
_callback_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	evas_object_data_set(wd->pickers[PICKER_HRS], "child", wd->pickers[PICKER_MIN]);
	evas_object_data_set(wd->pickers[PICKER_MIN], "child", wd->pickers[PICKER_SUB]);
	evas_object_data_set(wd->pickers[PICKER_SUB], "child", NULL);
	evas_object_data_set(wd->pickers[PICKER_AMPM], "child", wd->pickers[PICKER_HRS]);
	evas_object_data_set(wd->pickers[PICKER_HRS], "parent", wd->pickers[PICKER_AMPM]);
	evas_object_data_set(wd->pickers[PICKER_MIN], "parent", wd->pickers[PICKER_HRS]);
	evas_object_data_set(wd->pickers[PICKER_SUB], "parent", wd->pickers[PICKER_MIN]);

	evas_object_smart_callback_add(wd->pickers[PICKER_HRS], "overflowed", _overflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_HRS], "underflowed", _underflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_MIN], "overflowed", _overflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_MIN], "underflowed", _underflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_SUB], "overflowed", _overflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_SUB], "underflowed", _underflow_cb, obj);

	evas_object_smart_callback_add(wd->pickers[PICKER_HRS], "changed", _hrs_changed_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_MIN], "changed", _min_changed_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_SUB], "changed", _sub_changed_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_AMPM], "changed", _ampm_changed_cb, obj);
}

static void
_pickers_add(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	wd->pickers[PICKER_HRS] = _hrs_add(wd->base, "elm.swallow.hour", EINA_TRUE);
	wd->pickers[PICKER_MIN] = _digit_add(wd->base, "elm.swallow.min", 5);
	wd->pickers[PICKER_SUB] = _digit_add(wd->base, "elm.swallow.sub", MAX_DIGIT);
	wd->pickers[PICKER_AMPM] = _ampm_add(wd->base, "elm.swallow.ampm");
	edje_object_signal_emit(wd->base, "elm,state,ampm,visible", "elm");
	wd->ampm = EINA_TRUE;

	_callback_init(obj);
}

static void
_pickers_del(Evas_Object *obj)
{
	int i = 0;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	for (i = 0; i < PICKER_MAX; i++) {
		if (wd->pickers[i]) {
			evas_object_del(wd->pickers[i]);
			wd->pickers[i] = NULL;
		}
	}
}

static void
_changed(Evas_Object *picker, Evas_Object *child, Evas_Object *obj)
{
	int c1, c2 = 0;
	c1 = (int)(evas_object_data_get(picker, "carryon"));
	evas_object_data_set(picker, "carryon", (void *)0);

	if (child) {
		c2 = (int)(evas_object_data_get(child, "carryon"));
		evas_object_data_set(child, "carryon", (void *)0);
	}

	if (!c1 && !c2)
		evas_object_smart_callback_call(obj, "changed", NULL);
}

static void
_overflow_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *eo;
	eo = evas_object_data_get(obj, "parent");
	if (eo) {
		elm_picker_next(eo);
		evas_object_data_set(obj, "carryon", (void *)1);
	}
}

static void
_underflow_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *eo;
	eo = evas_object_data_get(obj, "parent");
	if (eo) {
		elm_picker_prev(eo);
		evas_object_data_set(obj, "carryon", (void *)1);
	}
}

static void
_hrs_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *hrs;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	hrs = elm_picker_item_label_get(event_info);
	wd->hrs = atoi(hrs);

	if (wd->ampm) {
		wd->hrs	%= 12;
		if (wd->is_pm)
			wd->hrs += 12;
	}

	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_min_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *min;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	min = elm_picker_item_label_get(event_info);

	if (wd->seconds) {
		wd->min = atoi(min);
	} else {
		wd->min = wd->min % 10;
		wd->min += atoi(min) * 10;
	}

	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_sub_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *sub;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	sub = elm_picker_item_label_get(event_info);

	if (wd->seconds) {
		wd->sec = atoi(sub);
	} else {
		wd->min = wd->min - wd->min % 10;
		wd->min += atoi(sub);
	}

	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_ampm_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *ampm;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	ampm = elm_picker_item_label_get(event_info);

	wd->hrs = (wd->hrs) % 12;
	if (!strcmp(ampm, "AM")) {
		wd->is_pm = 0;
	} else {
		wd->is_pm = 1;
		wd->hrs += 12;
	}

	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

/**
 * Add a new timepicker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Timepicker
 */
EAPI Evas_Object *
elm_timepicker_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "timepicker");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_disable_hook_set(obj, _disable_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "timepicker", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);
	_pickers_add(obj);
	_sizing_eval(obj);
	return obj;
}

/**
 * Set selected time of the timepicker
 *
 * @param obj The timepicker object
 * @param hrs The hours to set
 * @param min The minutes to set
 * @param sec The seconds to set
 *
 * @ingroup Timepicker
 */
EAPI void
elm_timepicker_time_set(Evas_Object *obj, int hrs, int min, int sec)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (!_is_time_valid(hrs, min, sec)) return;

	wd->hrs = hrs;
	wd->min = min;
	wd->sec = sec;

	_update_all(obj);
}

/**
 * Get selected time of the timepicker
 *
 * @param obj The timepicker object
 * @param hrs The Pointer to the variable to get the selected hours
 * @param min The Pointer to the variable to get the selected minute
 * @param sec The Pointer to the variable to get the selected seconds
 *
 * @ingroup Timepicker
 */
EAPI void
elm_timepicker_time_get(Evas_Object *obj, int *hrs, int *min, int *sec)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (hrs)
		*hrs = wd->hrs;
	if (min)
		*min = wd->min;
	if (sec)
		*sec = wd->sec;
}

/**
 * Set if the timepicker show hours in military or am/pm mode
 *
 * @param obj The timepicker object
 * @param am_pm Bool option for the hours mode
 *
 * @ingroup Timepicker
 */
EAPI void
elm_timepicker_show_am_pm_set(Evas_Object *obj, Eina_Bool am_pm)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->ampm != am_pm) {
		wd->ampm = am_pm;
		_update_ampm(obj);
	}
}

/**
 * Set if the timepicker show seconds picker or not
 *
 * @param obj The timepicker object
 * @param am_pm Bool option for the show seconds picker
 *
 * @ingroup Timepicker
 */
EAPI void
elm_timepicker_show_seconds_set(Evas_Object *obj, Eina_Bool seconds)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->seconds != seconds) {
		wd->seconds = seconds;
		_update_seconds(obj);
	}
}
