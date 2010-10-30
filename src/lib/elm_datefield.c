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

enum {
	DATE_FORMAT_YYMMDD,
	DATE_FORMAT_YYDDMM,
	DATE_FORMAT_MMYYDD,
	DATE_FORMAT_MMDDYY,
	DATE_FORMAT_DDYYMM,
	DATE_FORMAT_DDMMYY,
	DATE_FORMAT_MAX
};

#define YEAR_MAX_LENGTH	4
#define MONTH_MAX_LENGTH	3
#define DAY_MAX_LENGTH		2
#define TIME_MAX_LENGTH	2

#define YEAR_MAXIMUM		2099
#define YEAR_MINIMUM		1900
#define MONTH_MAXIMUM		12
#define HOUR_24H_MAXIMUM	23
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
	int date_format;
	Eina_Bool pm:1;
	Eina_Bool time_mode:1;
	Eina_Bool editing:1;

	void (*func)(void *data, Evas_Object *obj, int value);
	void *func_data;
};

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);

static void _signal_rect_mouse_down(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_ampm_mouse_down(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_ampm_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _entry_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info);
static Eina_Bool _imf_event_commit_cb(void *data, int type, void *event);
static void _input_panel_event_callback(void *data, Ecore_IMF_Context *ctx, int value);

static void _date_entry_add(Evas_Object *obj);
static void _time_entry_add(Evas_Object *obj);
static void _date_update(Evas_Object *obj);
static void _entry_focus_move(Evas_Object *obj, Evas_Object *focus_obj);
static Eina_Bool _check_input_done(Evas_Object *obj, Evas_Object *focus_obj, int strlen);
static int _maximum_day_get(int year, int month);

static void
_del_hook(Evas_Object *obj)
{
 	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return ;	

	ecore_event_handler_del(wd->handler);
		
	free(wd);
}

static void
_on_focus_hook(void *data, Evas_Object *obj)
{
 	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->base) return ;
	
	if (elm_widget_focus_get(obj))
	{
		if (wd->layout == ELM_DATEFIELD_LAYOUT_TIME)
			elm_object_focus(wd->time[TIME_HOUR]);
		
		else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME || wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
		{
			switch (wd->date_format)
			{
				case DATE_FORMAT_YYDDMM:
				case DATE_FORMAT_YYMMDD:
					elm_object_focus(wd->date[DATE_YEAR]);
					break;
				case DATE_FORMAT_MMDDYY:
				case DATE_FORMAT_MMYYDD:
					elm_object_focus(wd->date[DATE_MON]);
					break;
				case DATE_FORMAT_DDMMYY:
				case DATE_FORMAT_DDYYMM:
					elm_object_focus(wd->date[DATE_DAY]);
			}
		}
	}
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;
	if (!wd || !wd->base) return;

	if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
	{
		_elm_theme_object_set(obj, wd->base, "datefield", "dateandtime", elm_widget_style_get(obj));

		for (i = 0; i < DATE_MAX; i++)
			elm_object_style_set(wd->date[i], "datefield/hybrid");
		for (i = 0; i < TIME_MAX; i++)
			elm_object_style_set(wd->time[i], "datefield/hybrid");
	}
	else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
	{
		_elm_theme_object_set(obj, wd->base, "datefield", "date", elm_widget_style_get(obj));

		for (i = 0; i < DATE_MAX; i++)
			elm_object_style_set(wd->date[i], "datefield");

		for (i = 0; i < TIME_MAX; i++)
			evas_object_hide(wd->time[i]);
	}
	else if (wd->layout == ELM_DATEFIELD_LAYOUT_TIME)
	{
		_elm_theme_object_set(obj, wd->base, "datefield", "time", elm_widget_style_get(obj));

		for (i = 0; i < TIME_MAX; i++)
			elm_object_style_set(wd->time[i], "datefield");

		for (i = 0; i < DATE_MAX; i++)
			evas_object_hide(wd->date[i]);
	}

	if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME || wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
	{
		edje_object_part_swallow(wd->base, "elm.swallow.date.year", wd->date[DATE_YEAR]);
		edje_object_part_swallow(wd->base, "elm.swallow.date.month", wd->date[DATE_MON]);
		edje_object_part_swallow(wd->base, "elm.swallow.date.day", wd->date[DATE_DAY]);
		edje_object_part_text_set(wd->base, "elm.text.date.comma", ",");	
	}
	
	if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME || wd->layout == ELM_DATEFIELD_LAYOUT_TIME)
	{
		edje_object_part_swallow(wd->base, "elm.swallow.time.hour", wd->time[TIME_HOUR]);
		edje_object_part_swallow(wd->base, "elm.swallow.time.min", wd->time[TIME_MIN]);
		edje_object_part_text_set(wd->base, "elm.text.colon", ":");
	}

	edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);

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
 	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Object *focus_obj;
	
	if (!wd || !wd->base) return ;	

	focus_obj = elm_widget_focused_object_get(data);
	if (focus_obj) elm_object_unfocus(focus_obj);
}

static void
_signal_ampm_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
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
_signal_rect_mouse_down(void *data, Evas_Object *obj, const char *emission, const char *source)
{
 	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return ;

	if (!strcmp(source, "elm.rect.date.year.over"))
		elm_object_focus(wd->date[DATE_YEAR]);
	else if (!strcmp(source, "elm.rect.date.month.over"))
		elm_object_focus(wd->date[DATE_MON]);
	else if (!strcmp(source, "elm.rect.date.day.over"))
		elm_object_focus(wd->date[DATE_DAY]);
	else if (!strcmp(source, "elm.rect.time.hour.over"))
		elm_object_focus(wd->time[TIME_HOUR]);
	else if (!strcmp(source, "elm.rect.time.min.over"))
		elm_object_focus(wd->time[TIME_MIN]);
	else if (!strcmp(source, "elm.rect.date.left.pad"))
	{
		switch (wd->date_format)
		{
			case DATE_FORMAT_YYDDMM:
			case DATE_FORMAT_YYMMDD:
				elm_object_focus(wd->date[DATE_YEAR]);
				break;
			case DATE_FORMAT_MMDDYY:
			case DATE_FORMAT_MMYYDD:
				elm_object_focus(wd->date[DATE_MON]);
				break;
			case DATE_FORMAT_DDMMYY:
			case DATE_FORMAT_DDYYMM:
				elm_object_focus(wd->date[DATE_DAY]);
		}
	}
	else if (!strcmp(source, "elm.rect.date.right.pad"))
	{
		switch (wd->date_format)
		{
			case DATE_FORMAT_MMDDYY:
			case DATE_FORMAT_DDMMYY:
				elm_object_focus(wd->date[DATE_YEAR]);
				break;
			case DATE_FORMAT_DDYYMM:
			case DATE_FORMAT_YYDDMM:
				elm_object_focus(wd->date[DATE_MON]);
				break;
			case DATE_FORMAT_YYMMDD:
			case DATE_FORMAT_MMYYDD:
				elm_object_focus(wd->date[DATE_DAY]);
		}
	}
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
 	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->base) return;
 
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
 	Widget_Data *wd = elm_widget_data_get(data);
	char str[YEAR_MAX_LENGTH+1] = {0,};
	int num = 0;

	if (!wd || !wd->base) return;
	
 	wd->editing = FALSE;

	if (obj == wd->date[DATE_YEAR])
	{
		if (!strlen(elm_entry_entry_get(wd->date[DATE_YEAR]))) num = wd->year;
		else num = atoi(elm_entry_entry_get(wd->date[DATE_YEAR]));
		
		if (num > YEAR_MAXIMUM) num = YEAR_MAXIMUM;
		else if (num < YEAR_MINIMUM) num = YEAR_MINIMUM;
		wd->year = num;

		sprintf(str, "%d", num);
		elm_entry_entry_set(wd->date[DATE_YEAR], str);
		edje_object_signal_emit(wd->base, "elm,state,year,focus,out", "elm");
	}
	else if (obj == wd->date[DATE_MON])
	{
 		int day_of_month = _maximum_day_get(wd->year, wd->month);

		if (wd->day > day_of_month) 
		{
			wd->day = day_of_month;
			sprintf(str, "%d", wd->day);
			elm_entry_entry_set(wd->date[DATE_DAY], str);
		}
		edje_object_signal_emit(wd->base, "elm,state,month,focus,out", "elm");
	}	
 	else if (obj == wd->date[DATE_DAY])
 	{
 		int day_of_month = _maximum_day_get(wd->year, wd->month);

		if (!strlen(elm_entry_entry_get(wd->date[DATE_DAY]))) num = wd->day;
		else num = atoi(elm_entry_entry_get(wd->date[DATE_DAY]));
		
		if (num > day_of_month) num = day_of_month;
		else if (num == 0) num = 1;
		wd->day = num;
		
		sprintf(str, "%d", num);		
		elm_entry_entry_set(wd->date[DATE_DAY], str);
		edje_object_signal_emit(wd->base, "elm,state,day,focus,out", "elm");
 	}
	else if (obj == wd->time[TIME_HOUR])
	{
		if (!strlen(elm_entry_entry_get(wd->time[TIME_HOUR]))) num = (wd->hour == 0)? 24 : wd->hour;
		else num = atoi(elm_entry_entry_get(wd->time[TIME_HOUR]));

		if (!wd->time_mode) // 24 mode
		{
			if (num > HOUR_24H_MAXIMUM) num = HOUR_24H_MAXIMUM;
			wd->hour = num;
		}
		else // 12 mode
		{
			if (num > HOUR_24H_MAXIMUM)
			{
				num = HOUR_12H_MAXIMUM;
				wd->pm = EINA_FALSE;
			}
			else if (num > HOUR_12H_MAXIMUM)
			{
				num -= HOUR_12H_MAXIMUM;
				wd->pm = EINA_TRUE;
			}
			else if (num == 0) num = 1;
			
			wd->hour = (wd->pm == EINA_TRUE)? num + HOUR_12H_MAXIMUM : num;
			if((wd->hour % 12) == 0) wd->hour -= HOUR_12H_MAXIMUM;

			if (wd->pm) edje_object_part_text_set(wd->base, "elm.text.ampm", "PM");
			else edje_object_part_text_set(wd->base, "elm.text.ampm", "AM");
		}
		sprintf(str, "%02d", num);		
		elm_entry_entry_set(wd->time[TIME_HOUR], str);
		edje_object_signal_emit(wd->base, "elm,state,hour,focus,out", "elm");			
	}
	else if (obj == wd->time[TIME_MIN])
	{
		if (!strlen(elm_entry_entry_get(wd->time[TIME_MIN]))) num = wd->min;
		else num = atoi(elm_entry_entry_get(wd->time[TIME_MIN]));
		
		if (num > MIN_MAXIMUM) num = MIN_MAXIMUM;
		wd->min = num;

		sprintf(str, "%02d", num);
		elm_entry_entry_set(wd->time[TIME_MIN], str);
		edje_object_signal_emit(wd->base, "elm,state,min,focus,out", "elm");
	}
	
	evas_object_smart_callback_call(data, "changed", NULL);	
}

static void 
_entry_focus_move(Evas_Object *obj, Evas_Object *focus_obj)
{
 	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;
 
	if (focus_obj == wd->date[DATE_YEAR])
	{
		switch (wd->date_format)
		{
			case DATE_FORMAT_DDMMYY:
			case DATE_FORMAT_MMDDYY:
				if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
					elm_object_focus(wd->time[TIME_HOUR]);
				else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
					elm_object_unfocus(wd->date[DATE_YEAR]);
				break;
			case DATE_FORMAT_DDYYMM:
			case DATE_FORMAT_YYMMDD:
				elm_object_focus(wd->date[DATE_MON]);
				break;
			case DATE_FORMAT_MMYYDD:
			case DATE_FORMAT_YYDDMM:
				elm_object_focus(wd->date[DATE_DAY]);
		}
	}
	else if (focus_obj == wd->date[DATE_MON])
	{
		switch (wd->date_format)
		{
			case DATE_FORMAT_DDYYMM:
			case DATE_FORMAT_YYDDMM:
				if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
					elm_object_focus(wd->time[TIME_HOUR]);
				else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
					elm_object_unfocus(wd->date[DATE_MON]);
				break;
			case DATE_FORMAT_DDMMYY:
			case DATE_FORMAT_MMYYDD:
				elm_object_focus(wd->date[DATE_YEAR]);
				break;
			case DATE_FORMAT_MMDDYY:
			case DATE_FORMAT_YYMMDD:
				elm_object_focus(wd->date[DATE_DAY]);
		}
	}
	else if (focus_obj == wd->date[DATE_DAY])
	{
		switch (wd->date_format)
		{
			case DATE_FORMAT_YYMMDD:
			case DATE_FORMAT_MMYYDD:
				if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
					elm_object_focus(wd->time[TIME_HOUR]);
				else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
					elm_object_unfocus(wd->date[DATE_DAY]);
				break;
			case DATE_FORMAT_DDYYMM:
			case DATE_FORMAT_MMDDYY:
				elm_object_focus(wd->date[DATE_YEAR]);
				break;
			case DATE_FORMAT_DDMMYY:
			case DATE_FORMAT_YYDDMM:
				elm_object_focus(wd->date[DATE_MON]);
		}
	}
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
 	Widget_Data *wd = elm_widget_data_get(obj);

 	if (!wd) return EINA_FALSE;

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

static void
_entry_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
  	Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

 	if (!strcmp(ev->keyname, "BackSpace"))
		elm_entry_entry_set(obj, "");
}

static Eina_Bool 
_imf_event_commit_cb(void *data, int type, void *event)
{
 	Widget_Data *wd = elm_widget_data_get(data);
	Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;
	Evas_Object *focus_obj;
	char str[YEAR_MAX_LENGTH+1] = {0,};
	
	if (!wd) return ECORE_CALLBACK_PASS_ON;
 	if (!elm_widget_focus_get(data)) return ECORE_CALLBACK_PASS_ON;
	
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

	return ECORE_CALLBACK_DONE;
}

static void 
_input_panel_event_callback(void *data, Ecore_IMF_Context *ctx, int value)
{
	Widget_Data *wd = elm_widget_data_get(data);

	if (!wd) return;

	if (wd->func)
		wd->func(wd->func_data, data, value);
}

static void
_date_update(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	char str[YEAR_MAX_LENGTH+1];

	if (!wd || !wd->base) return;
	
 	sprintf(str, "%d", wd->year);
	elm_entry_entry_set(wd->date[DATE_YEAR], str);

	sprintf(str, "%s", month_label[wd->month]);
	elm_entry_entry_set(wd->date[DATE_MON], str);

	sprintf(str, "%d", wd->day);
	elm_entry_entry_set(wd->date[DATE_DAY], str);

	if (!wd->time_mode) //24 mode
		sprintf(str, "%d", wd->hour);
	else
	{
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

		if (wd->hour > HOUR_12H_MAXIMUM)
			sprintf(str, "%02d", wd->hour - HOUR_12H_MAXIMUM);
		else if (wd->hour == 0)
			sprintf(str, "%02d", HOUR_12H_MAXIMUM);
		else
			sprintf(str, "%02d", wd->hour);
	}
	elm_entry_entry_set(wd->time[TIME_HOUR], str);

	sprintf(str, "%02d", wd->min);
	elm_entry_entry_set(wd->time[TIME_MIN], str);
}

static void 
_date_entry_add(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;

	if (!wd) return;	
	
	for (i = 0; i < DATE_MAX; i++)
	{
		wd->date[i] = elm_entry_add(obj);
		elm_entry_single_line_set(wd->date[i], EINA_TRUE);
		elm_entry_context_menu_disabled_set(wd->date[i], EINA_TRUE);
	        if (i == DATE_MON) elm_entry_input_panel_layout_set(wd->date[i], ELM_INPUT_PANEL_LAYOUT_MONTH);
	        else elm_entry_input_panel_layout_set(wd->date[i], ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
		evas_object_size_hint_weight_set(wd->date[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(wd->date[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
    		evas_object_smart_callback_add(wd->date[i], "focused", _entry_focused_cb, obj);
		evas_object_smart_callback_add(wd->date[i], "unfocused", _entry_unfocused_cb, obj);
		evas_object_event_callback_add(wd->date[i], EVAS_CALLBACK_KEY_UP, _entry_key_up_cb, obj);
		elm_widget_sub_object_add(obj, wd->date[i]);
	}
	elm_entry_maximum_bytes_set(wd->date[DATE_YEAR], YEAR_MAX_LENGTH);
	elm_entry_maximum_bytes_set(wd->date[DATE_MON], MONTH_MAX_LENGTH);
	elm_entry_maximum_bytes_set(wd->date[DATE_DAY], DAY_MAX_LENGTH);
}

static void 
_time_entry_add(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;

	if (!wd) return;	
	
	for (i = 0; i < TIME_MAX; i++)
	{
		wd->time[i] = elm_entry_add(obj);
		elm_entry_single_line_set(wd->time[i], EINA_TRUE);
		elm_entry_context_menu_disabled_set(wd->time[i], EINA_TRUE);
		elm_entry_input_panel_layout_set(wd->time[i], ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);		
		elm_entry_maximum_bytes_set(wd->time[i], TIME_MAX_LENGTH);
		evas_object_size_hint_weight_set(wd->time[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(wd->time[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_smart_callback_add(wd->time[i], "focused", _entry_focused_cb, obj);
		evas_object_smart_callback_add(wd->time[i], "unfocused", _entry_unfocused_cb, obj);
		evas_object_event_callback_add(wd->time[i], EVAS_CALLBACK_KEY_UP, _entry_key_up_cb, obj);
		elm_widget_sub_object_add(obj, wd->time[i]);
	}
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
	elm_widget_resize_object_set(obj, wd->base);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.date.left.pad", _signal_rect_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.date.year.over", _signal_rect_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.date.month.over", _signal_rect_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.date.day.over", _signal_rect_mouse_down, obj);	
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.date.right.pad", _signal_rect_mouse_down, obj);

	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.time.hour.over", _signal_rect_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.time.min.over", _signal_rect_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,1", "elm.rect.time.ampm.over", _signal_ampm_mouse_down, obj);
	edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "elm.rect.time.ampm.over", _signal_ampm_clicked, obj);

	wd->handler =  ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _imf_event_commit_cb, obj);
	_date_entry_add(obj);
	_time_entry_add(obj);	

	wd->year = YEAR_MINIMUM;
	wd->month = 1;
	wd->day = 1;
	wd->hour = 0;
	wd->min = 0;
	wd->layout = ELM_DATEFIELD_LAYOUT_DATEANDTIME;
	wd->editing = EINA_FALSE;
	wd->time_mode = EINA_TRUE;
	wd->date_format = DATE_FORMAT_MMDDYY;
	
	_theme_hook(obj);
	
   	return obj;
}

/**
 * set layout for the datefield
 *
 * @param obj The datefield object
 * @param layout set layout for date/time/dateandtime (default: ELM_DATEFIELD_LAYOUT_DATEANDTIME)
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_layout_set(Evas_Object *obj, Elm_Datefield_Layout layout)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return;

	if (wd->layout != layout)
	{
		wd->layout = layout;
		_theme_hook(obj);
	}
}

/**
 * get layout of the datefield
 *
 * @param obj The datefield object
 * @return layout of the datefield
 *
 * @ingroup Datefield
 */
EAPI Elm_Datefield_Layout
elm_datefield_layout_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return 0;

	return wd->layout;
}

/**
 * Set selected date of the datefield
 *
 * @param obj The datefield object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 * @param hour The hours to set (24hour mode - 0~23)
 * @param min The minutes to set (0~59)
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_set(Evas_Object *obj, int year, int month, int day, int hour, int min)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	int day_of_month;
	
	if (!wd) return;

	if (year > YEAR_MAXIMUM) wd->year = YEAR_MAXIMUM;
	else if (year < YEAR_MINIMUM) wd->year = YEAR_MINIMUM;
	else wd->year = year;

	if (month > MONTH_MAXIMUM) wd->month = MONTH_MAXIMUM;
	else if (month <= 0) wd->month = 1;
	else wd->month = month;

 	day_of_month = _maximum_day_get(wd->year, wd->month);	
	if (day > day_of_month) wd->day = day_of_month;
	else if (day <= 0) wd->day = 1;
	else wd->day = day;
	
	if (hour > HOUR_24H_MAXIMUM) wd->hour = HOUR_24H_MAXIMUM;
	else if (hour < 0) wd->hour = 0;
	else wd->hour = hour;

	if (min > MIN_MAXIMUM) wd->min = MIN_MAXIMUM;
	else if (min < 0) wd->min = 0;
	else wd->min = min;

	_date_update(obj);
}

/**
 * Get selected date of the datefield
 *
 * @param obj The datefield object
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
	ELM_CHECK_WIDTYPE(obj, widtype);
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
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return;

	if (wd->time_mode != mode) 
	{
		char str[YEAR_MAX_LENGTH+1];
		
		wd->time_mode = mode;
		if (!wd->time_mode) edje_object_signal_emit(wd->base, "elm,state,mode,24h", "elm");
		else edje_object_signal_emit(wd->base, "elm,state,mode,12h", "elm");

		if (!wd->time_mode) //24 mode
			sprintf(str, "%d", wd->hour);
		else
		{
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

			if (wd->hour > HOUR_12H_MAXIMUM)
				sprintf(str, "%d", wd->hour - HOUR_12H_MAXIMUM);
			else if (wd->hour == 0)
				sprintf(str, "%d", HOUR_12H_MAXIMUM);
			else
				sprintf(str, "%d", wd->hour);
		}
		elm_entry_entry_set(wd->time[TIME_HOUR], str);
	}
}

/**
 * get time mode of the datefield
 *
 * @param obj The datefield object
 * @return time mode (EINA_TRUE: 12hour mode / EINA_FALSE: 24hour mode) 
 *
 * @ingroup Datefield
 */
EAPI Eina_Bool
elm_datefield_time_mode_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if (!wd) return EINA_FALSE;

	return wd->time_mode;
}

/**
 * Set date format of datefield
 *
 * @param obj The datefield object
 * @param fmt The date format, ex) yymmdd. Default value is mmddyy.
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_format_set(Evas_Object *obj, const char *fmt)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	char sig[32] = "elm,state,format,";
	int i = 0, j;

	if (!wd || !fmt) return;

	j = strlen(sig);
	while (j < 32) {
		sig[j++] = tolower(fmt[i++]);
	}

	edje_object_signal_emit(wd->base, sig, "elm");

	if (strstr(sig, "yymmdd")) wd->date_format = DATE_FORMAT_YYMMDD;
	else if (strstr(sig, "yyddmm")) wd->date_format = DATE_FORMAT_YYDDMM;
	else if (strstr(sig, "mmyydd")) wd->date_format = DATE_FORMAT_MMYYDD;
	else if (strstr(sig, "mmddyy")) wd->date_format = DATE_FORMAT_MMDDYY;
	else if (strstr(sig, "ddyymm")) wd->date_format = DATE_FORMAT_DDYYMM;
	else if (strstr(sig, "ddmmyy")) wd->date_format = DATE_FORMAT_DDMMYY;
}

/**
 * get date format of the datefield
 *
 * @param obj The datefield object
 * @return date format string. ex) yymmdd
 *
 * @ingroup Datefield
 */
EAPI const char *
elm_datefield_date_format_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);

	switch (wd->date_format)
	{
		case DATE_FORMAT_YYMMDD: return "yymmdd";
		case DATE_FORMAT_YYDDMM: return "yyddmm";
		case DATE_FORMAT_MMYYDD: return "mmyydd";
		case DATE_FORMAT_MMDDYY: return "mmddyy";
		case DATE_FORMAT_DDYYMM: return "ddyymm";
		case DATE_FORMAT_DDMMYY: return "ddmmyy";
	}
}

/**
 * Add a callback function for input panel state
 *
 * @param obj The datefield object
 * @param func The function to be called when the event is triggered (value will be the Ecore_IMF_Input_Panel_State)
 * @param data The data pointer to be passed to @p func 
 *
 * @ingroup Datefield
 */
EAPI void 
elm_datefield_input_panel_state_callback_add(Evas_Object *obj, void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value), void *data)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;

	if (!wd) return;

	if (wd->func && (wd->func != pEventCallbackFunc))
		elm_datefield_input_panel_state_callback_del(obj, wd->func);	

	if (wd->func != pEventCallbackFunc)
	{
		wd->func = pEventCallbackFunc;
		wd->func_data = data;

		for (i = 0; i < DATE_MAX; i++)
			ecore_imf_context_input_panel_event_callback_add(
				elm_entry_imf_context_get(wd->date[i]), ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, obj);

		for (i = 0; i < TIME_MAX; i++)
			ecore_imf_context_input_panel_event_callback_add(
				elm_entry_imf_context_get(wd->time[i]), ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, obj);
	}		
}

/**
 * Delete a callback function for input panel state
 *
 * @param obj The datefield object
 * @param func The function to be called when the event is triggered
 *
 * @ingroup Datefield
 */
EAPI void 
elm_datefield_input_panel_state_callback_del(Evas_Object *obj, void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value))
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;

	if (!wd) return;

	if (wd->func && wd->func == pEventCallbackFunc) 
	{
		for (i = 0; i < DATE_MAX; i++)
			ecore_imf_context_input_panel_event_callback_del(
				elm_entry_imf_context_get(wd->date[i]), ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback);

		for (i = 0; i < TIME_MAX; i++)
			ecore_imf_context_input_panel_event_callback_del(
				elm_entry_imf_context_get(wd->time[i]), ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback);

		wd->func = NULL;
		wd->func_data = NULL;
	}
}

