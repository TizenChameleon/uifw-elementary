#include <Elementary.h>
#include "elm_priv.h"
#include <langinfo.h>
#include <string.h>

/**
 * @defgroup Datepicker Datepicker
 * @ingroup Elementary
 *
 * This is a date picker.
 */

enum {
	PICKER_YEAR,
	PICKER_MON,
	PICKER_DAY,
	PICKER_MAX
};

#define YEAR_MIN (1900)
#define YEAR_MAX (2099)
#define MONTH_MIN (1)
#define MONTH_MAX (12)
#define DAY_MIN (1)
#define DAY_MAX (31)
#define MONTH_MAX (12)
#define DAY_MAX (31)

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *pickers[PICKER_MAX];
	int y_max, m_max, d_max;
	int y_min, m_min, d_min;
	int day_of_month;
	int year, month, day;
	char fmt[8];
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _i18n(Evas_Object *obj);
static Evas_Object *_picker_add(Evas_Object *ly, const char *part, int min, int max, const char *fmt);
static void _pickers_del(Evas_Object *obj);
static void _picker_item_add(Evas_Object *eo, int min, int max, const char *fmt);
static void _picker_item_del(Evas_Object *eo);
static void _update_day_of_month(Evas_Object *obj, int month);
static void _update_picker(Evas_Object *picker, int nth);
static void _changed(Evas_Object *picker, Evas_Object *child, Evas_Object *obj);
static void _overflow_cb(void *data, Evas_Object *obj, void *event_info);
static void _underflow_cb(void *data, Evas_Object *obj, void *event_info);
static void _year_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _month_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _day_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _pickers_add(Evas_Object *obj);
static void _callback_init(Evas_Object *obj);

static Eina_Bool
_is_valid_date(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int cur, min, max;
	if (!wd) return EINA_FALSE;

	cur = wd->year * 10000 + wd->month * 100 + wd->day;
	max = wd->y_max * 10000 + wd->m_max * 100 + wd->d_max;
	min = wd->y_min * 10000 + wd->m_min * 100 + wd->d_min;

	if (cur > max || cur < min) return EINA_FALSE;
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
	_elm_theme_object_set(obj, wd->base, "datepicker", "base", elm_widget_style_get(obj));
	_pickers_add(obj);
	_i18n(obj);

	_update_picker(wd->pickers[PICKER_YEAR], wd->year - wd->y_min);
	_update_picker(wd->pickers[PICKER_MON], wd->month - 1);
	_update_picker(wd->pickers[PICKER_DAY], wd->day - 1);

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
_i18n(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char *fmt;
	char sig[32] = { "elm,state," };
	int i = 0, j, k;

	if (!wd) return;

	j = strlen(sig);
	k = j;
	fmt = nl_langinfo(D_FMT);

	while (fmt[i] && j < 32) {
		if (fmt[i] == '%' && fmt[i+1]) {
			i++;
			switch (fmt[i]) {
			case 'Y': case 'M': case 'D': case 'y': case 'm': case 'd':
				sig[j++] = tolower(fmt[i]);
				sig[j++] = tolower(fmt[i]);
				break;
			default:
				break;
			}
		}
		i++;
	}
	sig[j] = '\0';
	edje_object_signal_emit(wd->base, sig, "elm");
	snprintf(wd->fmt, 8, sig + k);
}

static void
_picker_item_add(Evas_Object *eo, int min, int max, const char *fmt)
{
	char buf[8];
	for (; min <= max; min++) {
		snprintf(buf, 8, fmt, min);
		elm_picker_item_append(eo, buf, NULL, NULL);
	}
}

static void
_picker_item_del(Evas_Object *eo)
{
	Elm_Picker_Item *item;
	item = elm_picker_first_item_get(eo);
	while (item) {
		elm_picker_item_del(item);
		item = elm_picker_first_item_get(eo);
	}
}

static Evas_Object *
_picker_add(Evas_Object *ly, const char *part, int min, int max, const char *fmt)
{
	Evas_Object *eo;
	eo = elm_picker_add(ly);
	_picker_item_add(eo, min, max, fmt);
	if (part)
		edje_object_part_swallow(ly, part, eo);
	return eo;
}

static void
_update_day_of_month(Evas_Object *obj, int month)
{
	int tmp;
	Elm_Picker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	tmp = wd->day_of_month;
	switch (month) {
	case 4: case 6:	case 9:	case 11:
		wd->day_of_month = 30;
		break;
	case 2:
		if ((!(wd->year % 4) && (wd->year % 100)) || !(wd->year % 400))
			wd->day_of_month = 29;
		else
			wd->day_of_month = 28;
		break;
	default:
		wd->day_of_month = 31;
		break;
	}

	if (wd->day > wd->day_of_month) {
		wd->day = wd->day_of_month;
	}

	if (tmp == wd->day_of_month) return;
	if (tmp > wd->day_of_month) {
		for (; tmp > wd->day_of_month; tmp--) {
			item = elm_picker_last_item_get(wd->pickers[PICKER_DAY]);
			elm_picker_item_del(item);
		}
	} else {
		char buf[8];
		tmp++;
		for (; tmp <= wd->day_of_month; tmp++) {
			snprintf(buf, 8, "%2d", tmp);
			elm_picker_item_append(wd->pickers[PICKER_DAY], buf, NULL, NULL);
		}
	}
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
	Widget_Data *wd = elm_widget_data_get(data);

	if (eo) {
		elm_picker_prev(eo);
		evas_object_data_set(obj, "carryon", (void *)-1);
	}

	if (obj == wd->pickers[PICKER_DAY]) {
		Elm_Picker_Item *item;
		_update_day_of_month(data, wd->month - 1);
		wd->day = wd->day_of_month;
		item = elm_picker_last_item_get(obj);
		elm_picker_item_selected_set(item);
	}
}

static void
_year_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *year;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	year = elm_picker_item_label_get(event_info);
	wd->year = atoi(year);

	if (wd->month == 2)
		_update_day_of_month(data, wd->month);
	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_month_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *month;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	month = elm_picker_item_label_get(event_info);
	wd->month = atoi(month);

	_update_day_of_month(data, wd->month);
	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_day_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	const char *day;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *child;
	if (!wd) return;

	day = elm_picker_item_label_get(event_info);
	wd->day = atoi(day);

	child = evas_object_data_get(obj, "child");
	_changed(obj, child, data);
}

static void
_pickers_add(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	wd->pickers[PICKER_YEAR] = _picker_add(wd->base, "elm.swallow.year", wd->y_min, wd->y_max, "%04d");
	wd->pickers[PICKER_MON] = _picker_add(wd->base, "elm.swallow.mon", 1, 12, "%d");
	wd->pickers[PICKER_DAY] = _picker_add(wd->base, "elm.swallow.day", 1, wd->day_of_month, "%02d");
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
_callback_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	evas_object_data_set(wd->pickers[PICKER_YEAR], "child", wd->pickers[PICKER_MON]);
	evas_object_data_set(wd->pickers[PICKER_MON], "child", wd->pickers[PICKER_DAY]);
	evas_object_data_set(wd->pickers[PICKER_MON], "parent", wd->pickers[PICKER_YEAR]);
	evas_object_data_set(wd->pickers[PICKER_DAY], "parent", wd->pickers[PICKER_MON]);

	evas_object_smart_callback_add(wd->pickers[PICKER_MON], "overflowed", _overflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_MON], "underflowed", _underflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_DAY], "overflowed", _overflow_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_DAY], "underflowed", _underflow_cb, obj);

	evas_object_smart_callback_add(wd->pickers[PICKER_YEAR], "changed", _year_changed_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_MON], "changed", _month_changed_cb, obj);
	evas_object_smart_callback_add(wd->pickers[PICKER_DAY], "changed", _day_changed_cb, obj);
}

/**
 * Add a new datepicker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Datepicker
 */
EAPI Evas_Object *
elm_datepicker_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "datepicker");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_disable_hook_set(obj, _disable_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "datepicker", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->y_max = YEAR_MAX;
	wd->y_min = YEAR_MIN;
	wd->m_max = MONTH_MAX;
	wd->m_min = MONTH_MIN;
	wd->d_min = DAY_MIN;
	wd->d_max = DAY_MAX;
	wd->day_of_month = DAY_MAX;

	wd->year = YEAR_MIN;
	wd->month = 1;
	wd->day = 1;

	_pickers_add(obj);

	_i18n(obj);

	_sizing_eval(obj);
	return obj;
}

/**
 * Set selected date of the datepicker
 *
 * @param obj The datepicker object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_set(Evas_Object *obj, int year, int month, int day)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (year < wd->y_min || year > wd->y_max
			|| month < 1 || month > 12
			|| day < 1 || day > 31)
		return;
	wd->year = year;
	wd->month = month;
	wd->day = day;
	_update_day_of_month(obj, month);
	_update_picker(wd->pickers[PICKER_YEAR], wd->year - wd->y_min);
	_update_picker(wd->pickers[PICKER_MON], wd->month - 1);
	_update_picker(wd->pickers[PICKER_DAY], wd->day - 1);
}

/**
 * Get selected date of the datepicker
 *
 * @param obj The datepicker object
 * @param year The pointer to the variable get the selected year
 * @param month The pointer to the variable get the selected month
 * @param day The pointer to the variable get the selected day
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_get(Evas_Object *obj, int *year, int *month, int *day)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (year)
		*year = wd->year;
	if (month)
		*month = wd->month;
	if (day)
		*day = wd->day;
}

/**
 * Set upper bound of the datepicker
 *
 * @param obj The datepicker object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_min_set(Evas_Object *obj, int year, int month, int day)
{
	// TODO
}

/**
 * Get lower bound of the datepicker
 *
 * @param obj The datepicker object
 * @param year The pointer to the variable get the minimum year
 * @param month The pointer to the variable get the minimum month
 * @param day The pointer to the variable get the minimum day
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_min_get(Evas_Object *obj, int *year, int *month, int *day)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (year)
		*year = wd->y_min;
	if (month)
		*month = wd->m_min;
	if (day)
		*day = wd->d_min;
}

/**
 * Set lower bound of the datepicker
 *
 * @param obj The datepicker object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_max_set(Evas_Object *obj, int year, int month, int day)
{
	// TODO
}

/**
 * Get upper bound of the datepicker
 *
 * @param obj The datepicker object
 * @param year The pointer to the variable get the maximum year
 * @param month The pointer to the variable get the maximum month
 * @param day The pointer to the variable get the maximum day
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_max_get(Evas_Object *obj, int *year, int *month, int *day)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (year)
		*year = wd->y_max;
	if (month)
		*month = wd->m_max;
	if (day)
		*day = wd->d_max;
}

/**
 * Set date format of datepicker
 *
 * @param obj The datepicker object
 * @param fmt The date format, ex) yymmdd
 *
 * @ingroup Datepicker
 */
EAPI void
elm_datepicker_date_format_set(Evas_Object *obj, const char *fmt)
{
	char sig[32] = "elm,state,";
	int i = 0, j;
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd || !fmt) return;

	j = strlen(sig);
	while (j < 32) {
		sig[j++] = tolower(fmt[i++]);
	}

	edje_object_signal_emit(wd->base, sig, "elm");
}

/**
 * Get date format of datepicker
 *
 * @param obj The datepicker object
 * @return The date format of given datepicker
 *
 * @ingroup Datepicker
 */
EAPI const char *
elm_datepicker_date_format_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return  wd->fmt;
}
