#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Datefield Datefield
 * @ingroup Elementary
 *
 * This is a date editfield. it is used to input date and time using keypad
 */

typedef struct _Widget_Data Widget_Data;

enum {
	DATE_YEAR,
	DATE_MON,
	DATE_DAY,
	DATE_MAX
};

enum {
	TIME_HOUR,
	TIME_MIN,
	TIME_MAX
};

#define YEAR_MAX_LENGTH	4
#define MONTH_MAX_LENGTH	3
#define DAY_MAX_LENGTH		2
#define TIME_MAX_LENGTH	2

#define YEAR_MAXIMUM		2099
#define YEAR_MINIMUM		1900
#define HOUR_24H_MAXIMUM	24
#define HOUR_12H_MAXIMUM	12
#define MIN_MAXIMUM		59

static char month_label[13][4] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

struct _Widget_Data
{
	Evas_Object *base;	
	Evas_Object *date[DATE_MAX];
	Evas_Object *time[TIME_MAX];
	Ecore_Event_Handler *handler;
	int layout;

	int year, month, day, hour, min;
	Eina_Bool pm:1;
	Eina_Bool time_mode:1;
	Eina_Bool editing:1;
};

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);

static void _signal_ampm_mouse_down(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_ampm_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _entry_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info);
static int _imf_event_commit_cb(void *data, int type, void *event);

static void _date_entry_add(Evas_Object *obj);
static void _time_entry_add(Evas_Object *obj);
static void _date_update(Evas_Object *obj);
static void _entry_focus_move(Evas_Object *obj, Evas_Object *focus_obj);
static Eina_Bool _check_input_done(Evas_Object *obj, Evas_Object *focus_obj, int strlen);
static int _maximum_day_get(int year, int month);

static void
_del_hook(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return ;	
	
	ecore_event_handler_del(wd->handler);
	free(wd);
}

static void
_on_focus_hook(void *data, Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->base) return ;	

	if (!elm_widget_focus_get(obj))
		edje_object_signal_emit(wd->base, "elm,state,focus,out", "elm");
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;
	if (!wd || !wd->base) return;
	
	_elm_theme_object_set(obj, wd->base, "datefield", "base", elm_widget_style_get(obj));

	for (i = 0; i < DATE_MAX; i++)
		_elm_theme_object_set(obj, wd->date[i], "entry", "base", "datefield");

	for (i = 0; i < TIME_MAX; i++)
		_elm_theme_object_set(obj, wd->time[i], "entry", "base", "datefield");

	edje_object_part_swallow(wd->base, "elm.swallow.date.year", wd->date[DATE_YEAR]);
	edje_object_part_swallow(wd->base, "elm.swallow.date.month", wd->date[DATE_MON]);
	edje_object_part_swallow(wd->base, "elm.swallow.date.day", wd->date[DATE_DAY]);
	edje_object_part_text_set(wd->base, "elm.text.date.comma", ",");	

	edje_object_part_swallow(wd->base, "elm.swallow.time.hour", wd->time[TIME_HOUR]);
	edje_object_part_swallow(wd->base, "elm.swallow.time.min", wd->time[TIME_MIN]);
	edje_object_part_text_set(wd->base, "elm.text.colon", ":");

	_date_update(obj);
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	edje_object_size_min_calc(wd->base, &minw, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_signal_ampm_mouse_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	ELM_CHECK_WIDTYPE(data, widtype);
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *focus_obj;
	
	if (!wd || !wd->base) return ;	

	focus_obj = elm_widget_focused_object_get(data);
	if (focus_obj) elm_object_unfocus(focus_obj);
	edje_object_signal_emit(wd->base, "elm,state,focus,out", "elm");
}

static void
_signal_ampm_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	ELM_CHECK_WIDTYPE(data, widtype);
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->base) return ;	

	wd->pm = !wd->pm;

	if (wd->pm)
	{
		edje_object_part_text_set(wd->base, "elm.text.ampm", "PM");
		wd->hour += HOUR_12H_MAXIMUM;
	}
	else
	{
		edje_object_part_text_set(wd->base, "elm.text.ampm", "AM");
		wd->hour -= HOUR_12H_MAXIMUM;
	}
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	ELM_CHECK_WIDTYPE(data, widtype);
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->base) return ;

	if (elm_widget_focus_get(data))
		edje_object_signal_emit(wd->base, "elm,state,focus,in", "elm");

	if (obj == wd->date[DATE_YEAR])
		edje_object_signal_emit(wd->base, "elm,state,year,focus,in", "elm");
	else if (obj == wd->date[DATE_MON])
		edje_object_signal_emit(wd->base, "elm,state,month,focus,in", "elm");
	else if (obj == wd->date[DATE_DAY])
		edje_object_signal_emit(wd->base, "elm,state,day,focus,in", "elm");
	else if (obj == wd->time[TIME_HOUR])
		edje_object_signal_emit(wd->base, "elm,state,hour,focus,in", "elm");
	else if (obj == wd->time[TIME_MIN])
		edje_object_signal_emit(wd->base, "elm,state,min,focus,in", "elm");
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	ELM_CHECK_WIDTYPE(data, widtype);
	Widget_Data *wd = elm_widget_data_get(data);
	char str[YEAR_MAX_LENGTH+1] = {0,};
	int num = 0;
	
	if (!wd || !wd->base) return ;

	wd->editing = FALSE;

	if (obj == wd->date[DATE_YEAR])
	{
		num = atoi(elm_entry_entry_get(wd->date[DATE_YEAR]));
		if (num > YEAR_MAXIMUM) sprintf(str, "%d", YEAR_MAXIMUM);
		else if (num < YEAR_MINIMUM) sprintf(str, "%d", YEAR_MINIMUM);
		else sprintf(str, "%d", num);

		elm_entry_entry_set(wd->date[DATE_YEAR], str);
		wd->year = atoi(elm_entry_entry_get(wd->date[DATE_YEAR]));
		edje_object_signal_emit(wd->base, "elm,state,year,focus,out", "elm");
	}
	else if (obj == wd->date[DATE_MON])
	{
		edje_object_signal_emit(wd->base, "elm,state,month,focus,out", "elm");
	}	
 	else if (obj == wd->date[DATE_DAY])
 	{
		char *entry_str = elm_entry_entry_get(wd->date[DATE_DAY]);
		int day_of_month = _maximum_day_get(wd->year, wd->month);
		
		num = atoi(entry_str);
		if (num > day_of_month) sprintf(str, "%d", day_of_month);
		else if (entry_str[0] == '0') 
		{
			str[0] = (entry_str[1] == '0' || entry_str[1] == '\0')? '1' : entry_str[1];
			str[1] = '\0';
		}
		else sprintf(str, "%d", num);

		elm_entry_entry_set(wd->date[DATE_DAY], str);
		wd->day = atoi(elm_entry_entry_get(wd->date[DATE_DAY]));
		edje_object_signal_emit(wd->base, "elm,state,day,focus,out", "elm");
 	}
	else if (obj == wd->time[TIME_HOUR])
	{
		char *entry_str = elm_entry_entry_get(wd->time[TIME_HOUR]);
		
		num = atoi(entry_str);
		if (!wd->time_mode)  //24h mode
		{
			if (num > HOUR_24H_MAXIMUM) sprintf(str, "%d", HOUR_24H_MAXIMUM);
			else if (entry_str[0] == '0') 
			{
				str[0] = (entry_str[1] == '\0')? '0' : entry_str[1];
				str[1] = '\0';
			}
			else sprintf(str, "%d", num);

			elm_entry_entry_set(wd->time[TIME_HOUR], str);
			wd->hour = atoi(elm_entry_entry_get(wd->time[TIME_HOUR]));
			edje_object_signal_emit(wd->base, "elm,state,hour,focus,out", "elm");
		}
		else  //12h mode
		{
			if (num > HOUR_12H_MAXIMUM) 
			{
				num -= HOUR_12H_MAXIMUM;
				wd->pm = EINA_TRUE;
			}
			if (num > HOUR_12H_MAXIMUM) sprintf(str, "%d", HOUR_12H_MAXIMUM);
			else if (entry_str[0] == '0') 
			{
				str[0] = (entry_str[1] == '\0')? '0' : entry_str[1];
				str[1] = '\0';
			}
			else sprintf(str, "%d", num);

			elm_entry_entry_set(wd->time[TIME_HOUR], str);
			if (wd->pm) edje_object_part_text_set(wd->base, "elm.text.ampm", "PM");
			else edje_object_part_text_set(wd->base, "elm.text.ampm", "AM");
			
			wd->hour = (wd->pm == EINA_TRUE)? atoi(elm_entry_entry_get(wd->time[TIME_HOUR])) + HOUR_12H_MAXIMUM : atoi(elm_entry_entry_get(wd->time[TIME_HOUR]));
			if((wd->hour % 12) == 0) wd->hour -= HOUR_12H_MAXIMUM;
			edje_object_signal_emit(wd->base, "elm,state,hour,focus,out", "elm");
		}	
	}
	else if (obj == wd->time[TIME_MIN])
	{
		num = atoi(elm_entry_entry_get(wd->time[TIME_MIN]));
		if (num > MIN_MAXIMUM) sprintf(str, "%d", MIN_MAXIMUM);
		else sprintf(str, "%d", num);

		elm_entry_entry_set(wd->time[TIME_MIN], str);
		wd->min = atoi(elm_entry_entry_get(wd->time[TIME_MIN]));
		edje_object_signal_emit(wd->base, "elm,state,min,focus,out", "elm");
		edje_object_signal_emit(wd->base, "elm,state,focus,out", "elm");
	}
}

static void 
_entry_focus_move(Evas_Object *obj, Evas_Object *focus_obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->base) return ;

	if (focus_obj == wd->date[DATE_YEAR])
		elm_object_focus(wd->time[TIME_HOUR]);
	else if (focus_obj == wd->date[DATE_MON])
		elm_object_focus(wd->date[DATE_DAY]);
	else if (focus_obj == wd->date[DATE_DAY])
		elm_object_focus(wd->date[DATE_YEAR]);
	else if (focus_obj == wd->time[TIME_HOUR])
		elm_object_focus(wd->time[TIME_MIN]);
	else if (focus_obj == wd->time[TIME_MIN])
		elm_object_unfocus(wd->time[TIME_MIN]);
}

static int
_maximum_day_get(int year, int month)
{
	int day_of_month = 0;
	if (year == 0 || month == 0) return 0;

	switch (month) {
		case 4: 
		case 6:	
		case 9:	
		case 11:
			day_of_month = 30;
			break;
		case 2:
			if ((!(year % 4) && (year % 100)) || !(year % 400))
				day_of_month = 29;
			else
				day_of_month = 28;
			break;
		default:
			day_of_month = 31;
			break;
	}

	return day_of_month;
}

static Eina_Bool 
_check_input_done(Evas_Object *obj, Evas_Object *focus_obj, int strlen)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->base) return EINA_FALSE;

	if (focus_obj == wd->date[DATE_YEAR] && strlen == YEAR_MAX_LENGTH)
		wd->editing = EINA_FALSE;
	else if (focus_obj == wd->date[DATE_MON] && strlen == MONTH_MAX_LENGTH)
 		wd->editing = EINA_FALSE;
 	else if (focus_obj == wd->date[DATE_DAY] && strlen == DAY_MAX_LENGTH)
		wd->editing = EINA_FALSE;
	else if (focus_obj == wd->time[TIME_HOUR])
	{	
		if (strlen == TIME_MAX_LENGTH || atoi(elm_entry_entry_get(focus_obj)) > 2)
			wd->editing = EINA_FALSE;
	}
	else if (focus_obj == wd->time[TIME_MIN] && strlen == TIME_MAX_LENGTH) 
		wd->editing = EINA_FALSE;

	return !wd->editing;
}

static int 
_imf_event_commit_cb(void *data, int type, void *event)
{
	ELM_CHECK_WIDTYPE(data, widtype);
	Widget_Data *wd = elm_widget_data_get(data);
	Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;
	Evas_Object *focus_obj;
	char str[YEAR_MAX_LENGTH+1] = {0,};

	if (!wd || !wd->base) return ECORE_CALLBACK_RENEW;
	if(!elm_widget_focus_get(data)) return ECORE_CALLBACK_RENEW;
	
	focus_obj = elm_widget_focused_object_get(data);
	if (!wd->editing) 
	{
		elm_entry_entry_set(focus_obj, "");
		wd->editing = EINA_TRUE;
	}
	
	if (focus_obj == wd->date[DATE_MON])
	{
		wd->month = atoi(ev->str);
		strcpy(str, month_label[wd->month]);
	}
	else
	{
		strcpy(str, elm_entry_entry_get(focus_obj));
		str[strlen(str)] = ev->str[0];
	}
	elm_entry_entry_set(focus_obj, str);

	if (_check_input_done(data, focus_obj, strlen(str)))
		_entry_focus_move(data, focus_obj);

	return ECORE_CALLBACK_CANCEL;
}

static void
_date_update(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	char str[YEAR_MAX_LENGTH+1] = {0,};

	if (!wd || !wd->base) return;

	sprintf(str, "%d", wd->year);
	elm_entry_entry_set(wd->date[DATE_YEAR], str);

	memset(str, 0, YEAR_MAX_LENGTH+1);
	sprintf(str, "%s", month_label[wd->month]);
	elm_entry_entry_set(wd->date[DATE_MON], str);

	memset(str, 0, YEAR_MAX_LENGTH+1);
	sprintf(str, "%d", wd->day);
	elm_entry_entry_set(wd->date[DATE_DAY], str);

	if (wd->hour >= HOUR_12H_MAXIMUM)
	{
		wd->pm = EINA_TRUE;
		edje_object_part_text_set(wd->base, "elm.text.ampm", "PM");
	}
	else
	{
		wd->pm = EINA_FALSE;
		edje_object_part_text_set(wd->base, "elm.text.ampm", "AM");		
	}

	memset(str, 0, YEAR_MAX_LENGTH+1);
	if (wd->time_mode && (wd->hour > HOUR_12H_MAXIMUM))
		sprintf(str, "%d", wd->hour - HOUR_12H_MAXIMUM);
	else if (wd->time_mode && (wd->hour == 0))
		sprintf(str, "%d", HOUR_12H_MAXIMUM);
	else
		sprintf(str, "%d", wd->hour);
	elm_entry_entry_set(wd->time[TIME_HOUR], str);

	memset(str, 0, YEAR_MAX_LENGTH+1);
	sprintf(str, "%d", wd->min);
	if (wd->min == 0) str[1] = '0';
	elm_entry_entry_set(wd->time[TIME_MIN], str);
}

static void 
_date_entry_add(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;
	
	if (!wd || !wd->base) return ;	

	for (i = 0; i < DATE_MAX; i++)
	{
		wd->date[i] = elm_entry_add(obj);
		elm_object_style_set(wd->date[i], "datefield");
		elm_entry_context_menu_disabled_set(wd->date[i], EINA_TRUE);
		elm_entry_input_panel_layout_set(wd->date[i], ELM_INPUT_PANEL_LAYOUT_NUMBER);
		evas_object_size_hint_weight_set(wd->date[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(wd->date[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
    		evas_object_smart_callback_add(wd->date[i], "focused", _entry_focused_cb, obj);
		evas_object_smart_callback_add(wd->date[i], "unfocused", _entry_unfocused_cb, obj);
		elm_widget_sub_object_add(obj, wd->date[i]);
	}

	elm_entry_maximum_bytes_set(wd->date[DATE_YEAR], YEAR_MAX_LENGTH);
	elm_entry_maximum_bytes_set(wd->date[DATE_MON], MONTH_MAX_LENGTH);
	elm_entry_maximum_bytes_set(wd->date[DATE_DAY], DAY_MAX_LENGTH);
	
	edje_object_part_swallow(wd->base, "elm.swallow.date.year", wd->date[DATE_YEAR]);
	edje_object_part_swallow(wd->base, "elm.swallow.date.month", wd->date[DATE_MON]);
	edje_object_part_swallow(wd->base, "elm.swallow.date.day", wd->date[DATE_DAY]);
	edje_object_part_text_set(wd->base, "elm.text.date.comma", ",");	
}

static void 
_time_entry_add(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;
	
	if (!wd || !wd->base) return ;	

	for (i = 0; i < TIME_MAX; i++)
	{
		wd->time[i] = elm_entry_add(obj);
		elm_object_style_set(wd->time[i], "datefield");	
		elm_entry_context_menu_disabled_set(wd->time[i], EINA_TRUE);
		elm_entry_input_panel_layout_set(wd->time[i], ELM_INPUT_PANEL_LAYOUT_NUMBER);		
		elm_entry_maximum_bytes_set(wd->time[i], TIME_MAX_LENGTH);
		evas_object_size_hint_weight_set(wd->time[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(wd->time[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_smart_callback_add(wd->time[i], "focused", _entry_focused_cb, obj);
		evas_object_smart_callback_add(wd->time[i], "unfocused", _entry_unfocused_cb, obj);
		elm_widget_sub_object_add(obj, wd->time[i]);
	}

	edje_object_part_swallow(wd->base, "elm.swallow.time.hour", wd->time[TIME_HOUR]);
	edje_object_part_swallow(wd->base, "elm.swallow.time.min", wd->time[TIME_MIN]);
	edje_object_part_text_set(wd->base, "elm.text.colon", ":");
	edje_object_part_text_set(wd->base, "elm.text.ampm", "AM");
	edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "elm.rect.ampm.over", _signal_ampm_clicked, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.ampm.over", _signal_ampm_mouse_down, obj);
}

/**
 * Add a new datefield object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Datefield
 */
EAPI Evas_Object *
elm_datefield_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	e = evas_object_evas_get(parent);
	if (!e) return NULL; 
	wd = ELM_NEW(Widget_Data);
	obj = elm_widget_add(e); 
	ELM_SET_WIDTYPE(widtype, "datefield");
	elm_widget_type_set(obj, "datefield");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
	elm_widget_can_focus_set(obj, EINA_TRUE);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "datefield", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->handler =  ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _imf_event_commit_cb, obj);
	_date_entry_add(obj);
	_time_entry_add(obj);	
		
	wd->editing = EINA_FALSE;
	wd->time_mode = EINA_TRUE;
	elm_datefield_date_set(obj, 1900, 1, 1, 0, 0);

	_sizing_eval(obj);

   	return obj;
}

/**
 * Add a new datefield object
 *
 * @param parent The parent object
 * @param layout set layout for date/time/dateandtime
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_layout_set(Evas_Object *obj, Elm_Datefield_Layout layout)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return ;	

	wd->layout = layout; 	
}

/**
 * Set selected date of the datefield
 *
 * @param obj The datefield object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 * @param hour The hours to set (24hour mode)
 * @param min The minutes to set
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_set(Evas_Object *obj, int year, int month, int day, int hour, int min)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return ;	

	wd->year = year;
	wd->month = month;
	wd->day = day;
	wd->hour = hour;
	wd->min = min;

	_date_update(obj);
}

/**
 * Get selected date of the datefield
 *
 * @param obj The datepicker object
 * @param year The pointer to the variable get the selected year
 * @param month The pointer to the variable get the selected month
 * @param day The pointer to the variable get the selected day
 * @param hour The pointer to the variable get the selected hour (24hour mode)
 * @param hour The pointer to the variable get the selected min
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_get(Evas_Object *obj, int *year, int *month, int *day, int *hour, int *min)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	if (year)
		*year = wd->year;
	if (month)
		*month = wd->month;
	if (day)
		*day = wd->day;
	if (hour)
		*hour = wd->hour;
	if (min)
		*min = wd->min;
}

/**
 * Set if the datefield show hours in military or am/pm mode
 *
 * @param obj The datefield object
 * @param mode option for the hours mode. If true, it is shown as 12h mode, if false, it is shown as 24h mode. Default value is true 
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_time_mode_set(Evas_Object *obj, Eina_Bool mode)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->time_mode != mode) {
		wd->time_mode = mode;
		//_update_ampm(obj);
	}
}

