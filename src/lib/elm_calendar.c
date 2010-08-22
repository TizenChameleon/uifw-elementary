#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Ctxpopup
 *
 * Contextual popup.
 */

#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

typedef void (*CalendarReturnCB)(void* cb_data, int year, int month, int day);

enum
{
	SUN = 0x0,
	MON,
	TUE,
	WED,
	THU,
	FRI,
	SAT,
	WDAY_MAX
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
	Evas_Object* parent;
	Evas_Object* layout;
	Evas_Object* bg;
	Evas_Object* ly_title;
	Evas_Object* ly_content;

	struct tm startTime;
	Evas_Coord x, y;

	CalendarReturnCB return_cb;
	void* cb_data;
};

static void _del_hook(Evas_Object* obj);
static void _del_pre_hook(Evas_Object* obj);
static void _theme_hook(Evas_Object* obj);
static void _sizing_eval(Evas_Object *obj);
static void _bg_clicked_cb(void* data, Evas_Object* obj, void* event_info);
static void _parent_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _calendar_show(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _calendar_hide(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _changed_size_hints(void* data, Evas* evas, Evas_Object* obj, void* event_info);

static void 
_changed_size_hints(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	_sizing_eval( data );
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
	Evas_Coord w, h;

	if (!wd) return;
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->bg, &minw, &minh, minw, minh);
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);

	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minh) minh = h;

	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void 
_del_pre_hook(Evas_Object* obj)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return;
	evas_object_event_callback_del_full(wd->parent,	EVAS_CALLBACK_RESIZE, _parent_resize, obj);
}

static void 
_del_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return;
	elm_ctxpopup_clear(obj);
	free( wd );
}

static void 
_theme_hook(Evas_Object* obj)
{
	Eina_List* elist;
	Eina_List* elist_child;
	Eina_List* elist_temp;

	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd)	return;

	_sizing_eval(obj);
}

static void 
_bg_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	Widget_Data* wd = (Widget_Data*) data;
	if (!wd) return;

	edje_object_signal_emit( _EDJ(wd->layout), "calendar.hide", "hide" );
}

static void 
_parent_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	_sizing_eval(data);
}

static void 
_calendar_show(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Widget_Data *wd = (Widget_Data *) data;
	if (!wd) return;

	evas_object_show(wd->layout);
	edje_object_signal_emit( _EDJ(wd->layout), "calendar.show", "show" );
}

static void 
_calendar_hide(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Widget_Data* wd = (Widget_Data*) data;
	if (!wd) return;

	edje_object_signal_emit( _EDJ(wd->layout), "calendar.hide", "hide" );
}

static void 
_mouse_move_cb(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Evas_Event_Mouse_Move *move = (Evas_Event_Mouse_Move *)event_info;

	Evas_Coord x, y, w, h;
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;
	evas_object_geometry_get( obj, &x, &y, &w, &h );
	wd->x = x;
	wd->y = y;
	_sizing_eval(obj);
}

static void 
_mouse_down_cb(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Evas_Event_Mouse_Down *down = (Evas_Event_Mouse_Down *)event_info;

	Evas_Coord x, y, w, h;
	const char *text;
	int day;
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;

	evas_object_geometry_get( obj, &x, &y, &w, &h );

	text = edje_object_part_text_get( _EDJ(obj), "month_mday_text" );
	if( text == NULL )
		return;

	day = atoi(text);
	if( day > 0 && day < 32 )
	{
		edje_object_signal_emit( _EDJ(obj), "mday.focus", "month" );
		wd->return_cb(wd->cb_data, wd->startTime.tm_year+1900, wd->startTime.tm_mon+1, day);
		edje_object_signal_emit( _EDJ(wd->layout), "calendar.hide", "hide" );
	}
}

static void 
_mouse_up_cb(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Evas_Event_Mouse_Up *up = (Evas_Event_Mouse_Up *)event_info;
	
	Evas_Coord x, y, w, h;
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;

	evas_object_geometry_get( obj, &x, &y, &w, &h );
}

static int
_calendar_get_first_wday(int year, int month)
{
	int y;
	static int t[12] = { SUN, WED, TUE, FRI, SUN, WED, FRI, MON, THU, SAT, TUE, THU };

	y = year - (month < 3);

	return (y + y/4 - y/100 + y/400 + t[month-1] + 1) % 7;
}

static int
_calendar_get_last_day(int year, int month)
{
	switch (month)
	{
		case 2: return ((year%400 == 0) || (year%4 == 0 && year%100 != 0)) ? 29 : 28;
		case 4:
		case 6:
		case 9:
		case 11: return 30;
		default: return 31;
	}
}

static int
_calendar_get_yday(int year, int month, int day)
{
	int ret = 0;
	
	while( --month )
	{
		ret += _calendar_get_last_day(year, month);
	}

	return (ret+day);
}

static int
_calendar_get_current_wcount(int year, int month, int day, int start)
{
	int wday, yday;

	wday = _calendar_get_first_wday( year, 1 );
	wday = ( (7+wday) - start ) % 7;
	yday = wday + _calendar_get_yday( year, month, day );
	return (yday % 7 == 0) ? (int)(yday / 7) : ((int)(yday / 7) + 1);
}

static void 
_calendar_set_text_mday(Evas_Object *tb, int year, int month, int start)
{
	int i, col, row;
	int first_wday, last_mday;
	char buf[32] = {0};
	Evas_Object* eo;

	first_wday = _calendar_get_first_wday( year, month );
	last_mday = _calendar_get_last_day( year, month );	
	
	col = 0;
	row = (first_wday + start) % 7;

	for( i=1; i<last_mday+1; i++ )
	{
		snprintf( buf, sizeof(buf), "%d%d", col, row );
		eo = evas_object_data_get( tb, buf );
		if( !eo )
			printf("\n col=%d  row=%d  eo NULL!!!!!!\n", col, row);

		snprintf( buf, sizeof(buf), "%d", i );
		edje_object_part_text_set( _EDJ(eo), "month_mday_text", buf ); 
		evas_object_show( eo );

		if( (7-start) % 7 == row )
			edje_object_color_class_set( _EDJ(eo), "cc_month_mday", 226, 20, 20, 255, 255, 255, 255, 255, 0, 0, 0, 0 );
		else if( (6-start) == row )
			edje_object_color_class_set( _EDJ(eo), "cc_month_mday", 47, 120, 220, 255, 255, 255, 255, 255, 0, 0, 0, 0 );
		else
			edje_object_color_class_set( _EDJ(eo), "cc_month_mday", 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0 );

		if( (row+1) % 7 == 0 )
		{
			col++;
			row = 0;
		}
		else
		{
			row++;
		}
	}
}

static int
_calendar_resize_table(Evas_Object *tb, void *data)
{
	int i, j;
	char buf[32] = {0};
	Evas_Coord mw, mh, x, y, w, h;
	Evas_Object *rect, *ly;

	evas_object_geometry_get( tb, &x, &y, &w, &h );
	elm_table_clear( tb, EINA_TRUE );

	mw = (int)(w / 7);
	mh = (int)(h / 6);
	
	for( i=0; i<6; i++ )
	{
		for( j=0; j<7; j++ )
		{
			rect = evas_object_rectangle_add( evas_object_evas_get(tb) );
			evas_object_size_hint_min_set( rect, mw, mh );
			evas_object_size_hint_weight_set( rect, 0.0, 0.0 );
			evas_object_size_hint_align_set( rect, EVAS_HINT_FILL, EVAS_HINT_FILL );
			evas_object_color_set( rect, 0, 0, 0, 255 );
			evas_object_show( rect );
			elm_table_pack( tb, rect, j, i, 1, 1 );

			ly = elm_layout_add( tb );
			elm_layout_theme_set( ly, "calendar", "month", "mday" );
			evas_object_size_hint_weight_set( ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
			evas_object_size_hint_align_set( ly, EVAS_HINT_FILL, EVAS_HINT_FILL );
			evas_object_show( ly );
			elm_table_pack( tb, ly, j, i, 1, 1 );
			
			snprintf( buf, sizeof(buf), "%d%d", i, j );
			evas_object_data_set( tb, buf, ly );
			evas_object_data_set( ly, "table", tb );

			evas_object_event_callback_add( ly, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, data );
			evas_object_event_callback_add( ly, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, data );
			evas_object_event_callback_add( ly, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, data );
		}
	}
}


static void
_calendar_update(void* data, struct tm *st)
{
	Evas_Object *tb, *eo;
	int i, wday, last_mday, wcount_f, wcount_l;
	int today_row, today_col;
	char tmp[32] = {0};
	char buf[32] = {0};
	struct tm stday;
	time_t tt;

	Widget_Data *wd = (Widget_Data *) data;
	if (!wd) return;
	
	eo = edje_object_part_swallow_get( _EDJ(wd->layout), "elm.swallow.title" );
	strftime(buf, sizeof(buf), "%B %Y", st);
	edje_object_part_text_set( _EDJ(eo), "text.title", buf );

	tb = edje_object_part_swallow_get( _EDJ(wd->ly_content), "month/swallow/days/c" );
		
	stday.tm_wday = 0;

	for( i=0; i<7; i++ )
	{
		snprintf( tmp, sizeof(tmp), "month_text_wday_%d", i );
		strftime( buf, sizeof(buf), "%a", &stday );
		stday.tm_wday++;

		edje_object_part_text_set( _EDJ(wd->ly_content), tmp, buf );
	}

	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_0", 226, 20, 20, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_1", 102, 102, 102, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_2", 102, 102, 102, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_3", 102, 102, 102, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_4", 102, 102, 102, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_5", 102, 102, 102, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	edje_object_color_class_set( _EDJ(wd->ly_content), "cc_month_wday_6", 47, 120, 220, 255, 255, 255, 255, 255, 0, 0, 0, 0  );
	
	wcount_f = _calendar_get_current_wcount( st->tm_year + 1900, st->tm_mon + 1, 1, 0 );
	last_mday = _calendar_get_last_day( st->tm_year + 1900, st->tm_mon + 1 );
	wcount_l = _calendar_get_current_wcount( st->tm_year + 1900, st->tm_mon + 1, last_mday, 0 );
	
	snprintf( tmp, sizeof(tmp), "month.list.wcount.%d", wcount_l - wcount_f + 1 );
	edje_object_signal_emit( _EDJ(wd->ly_content), tmp, "month" );

	_calendar_resize_table( tb, data );
	_calendar_set_text_mday( tb, st->tm_year + 1900, st->tm_mon + 1, 0 );

	// Mark today.
	tt = time(NULL);
	localtime_r(&tt, &stday);

	if( (stday.tm_year == st->tm_year) && (stday.tm_mon == st->tm_mon) )
	{
		today_col = _calendar_get_current_wcount( st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, 0 ) - wcount_f;
		wday = _calendar_get_first_wday( st->tm_year + 1900, st->tm_mon + 1 );
		today_row = ( wday + (st->tm_mday-1) % 7 ) % 7;
		snprintf( tmp, sizeof(tmp), "%d%d", today_col, today_row );
		eo = evas_object_data_get( tb, tmp );
		edje_object_signal_emit( _EDJ(eo), "mday.today", "month" );	
	}
}

static void
_calendar_resize_table_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *) data;

	_calendar_update( wd, &wd->startTime );
}

static int
_calendar_goto_prev_month(void *data)
{
	Evas_Object *tb;
	int new_mday;
	time_t tt;
	Widget_Data *wd = (Widget_Data *) data;
	
	tb = edje_object_part_swallow_get( _EDJ(wd->ly_content), "month/swallow/days/r" );
	if( tb )
	{
		edje_object_part_unswallow( _EDJ(wd->ly_content), tb );
		evas_object_del( tb );
	}

	tb = edje_object_part_swallow_get( _EDJ(wd->ly_content), "month/swallow/days/c" );
	edje_object_part_unswallow( _EDJ(wd->ly_content), tb );
	
	elm_layout_content_set( wd->ly_content, "month/swallow/days/r", tb );
	edje_object_signal_emit( _EDJ(wd->ly_content), "move.days.r.middle", "month" );

	tb = elm_table_add( wd->ly_content );
	elm_table_homogenous_set( tb, EINA_FALSE );
	elm_table_padding_set( tb, 1, 1 );
	evas_object_size_hint_weight_set( tb, 0.0, 0.0 );
	evas_object_size_hint_align_set( tb, EVAS_HINT_FILL, EVAS_HINT_FILL );
	
	wd->startTime.tm_mon--;
	new_mday = _calendar_get_last_day( wd->startTime.tm_year + 1900, wd->startTime.tm_mon + 1 );
	if( wd->startTime.tm_mday > new_mday )
		wd->startTime.tm_mday = new_mday;

	tt = mktime( &wd->startTime );
	localtime_r( &tt, &wd->startTime );

	elm_layout_content_set( wd->ly_content, "month/swallow/days/c", tb );
	edje_object_signal_emit( _EDJ(wd->ly_content), "move.days.c.left", "month" );

	//evas_object_event_callback_add( wd->table, EVAS_CALLBACK_RESIZE, _calendar_resize_table_cb, wd );
	_calendar_update( wd, &wd->startTime );
}

static int
_calendar_goto_next_month(void *data)
{
	Evas_Object *tb;
	int new_mday;
	time_t tt;
	Widget_Data *wd = (Widget_Data *) data;
	
	tb = edje_object_part_swallow_get( _EDJ(wd->ly_content), "month/swallow/days/l" );
	if( tb )
	{
		edje_object_part_unswallow( _EDJ(wd->ly_content), tb );
		evas_object_del( tb );
	}

	tb = edje_object_part_swallow_get( _EDJ(wd->ly_content), "month/swallow/days/c" );
	edje_object_part_unswallow( _EDJ(wd->ly_content), tb );
	
	elm_layout_content_set( wd->ly_content, "month/swallow/days/l", tb );
	edje_object_signal_emit( _EDJ(wd->ly_content), "move.days.l.middle", "month" );

	tb = elm_table_add( wd->ly_content );
	elm_table_homogenous_set( tb, EINA_FALSE );
	elm_table_padding_set( tb, 1, 1 );
	evas_object_size_hint_weight_set( tb, 0.0, 0.0 );
	evas_object_size_hint_align_set( tb, EVAS_HINT_FILL, EVAS_HINT_FILL );
	
	wd->startTime.tm_mon++;
	new_mday = _calendar_get_last_day( wd->startTime.tm_year + 1900, wd->startTime.tm_mon + 1 );
	if( wd->startTime.tm_mday > new_mday )
		wd->startTime.tm_mday = new_mday;

	tt = mktime( &wd->startTime );
	localtime_r( &tt, &wd->startTime );

	elm_layout_content_set( wd->ly_content, "month/swallow/days/c", tb );
	edje_object_signal_emit( _EDJ(wd->ly_content), "move.days.c.right", "month" );

	//evas_object_event_callback_add( wd->table, EVAS_CALLBACK_RESIZE, _calendar_resize_table_cb, wd );
	_calendar_update( wd, &wd->startTime );
}

static void
_arrow_left_btn_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = (Widget_Data *) data;
	
	_calendar_goto_prev_month(wd);
	edje_object_signal_emit( _EDJ(wd->ly_content), "flick.days.prev", "month" );
}

static void
_arrow_right_btn_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = (Widget_Data *) data;
	
	_calendar_goto_next_month(wd);
	edje_object_signal_emit( _EDJ(wd->ly_content), "flick.days.next", "month" );
}

/**
 * Add a new calendar object to the parent.
 *
 * @param parent 	Parent object
 * @return 		New object or NULL if it cannot be created
 *
 * @ingroup Calendar
 */
EAPI Evas_Object* 
elm_calendar_add(Evas_Object* parent)
{
	Evas_Object* obj, *title, *table;
	Evas* e;
	Widget_Data* wd;
	Evas_Coord x, y, w, h;
	time_t tt;
	struct tm st;
	char buf[32] = {0};

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "calendar");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	wd->parent = parent;

	wd->layout = elm_layout_add( parent );
	elm_widget_sub_object_add(obj, wd->layout);
	elm_layout_theme_set(wd->layout, "calendar", "bg", "default");
	evas_object_size_hint_weight_set(wd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	edje_object_signal_callback_add(_EDJ(wd->layout), "mouse,clicked,1", "base_temp", _bg_clicked_cb, wd);
	
	wd->ly_title = elm_layout_add( parent );
	elm_layout_theme_set(wd->ly_title, "calendar", "title", "default");
	evas_object_size_hint_weight_set(wd->ly_title, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->ly_title, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_signal_callback_add(_EDJ(wd->ly_title), "mouse,clicked,1", "rect.arrow.left", _arrow_left_btn_cb, wd);
	edje_object_signal_callback_add(_EDJ(wd->ly_title), "mouse,clicked,1", "rect.arrow.right", _arrow_right_btn_cb, wd);
	elm_layout_content_set(wd->layout, "elm.swallow.title", wd->ly_title);
	
	wd->ly_content = elm_layout_add( parent );
	elm_layout_theme_set(wd->ly_content, "calendar", "content", "default");
	evas_object_size_hint_weight_set(wd->ly_content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->ly_content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_layout_content_set(wd->layout, "elm.swallow.content", wd->ly_content);

	evas_object_geometry_get(parent, &x, &y, &w, &h);
	evas_object_move(wd->layout, x, y);
	evas_object_resize(wd->layout, w, h);

	// Set the current date.
	tt = time(NULL);
	localtime_r(&tt, &st);
	title = edje_object_part_swallow_get(_EDJ(wd->layout), "elm.swallow.title");
	strftime(buf, sizeof(buf), "%B %Y", &st);	
	edje_object_part_text_set(_EDJ(title), "text.title", buf);
	wd->startTime = st;

	table = elm_table_add( wd->ly_content );
	elm_table_homogenous_set( table, EINA_FALSE );
	elm_table_padding_set( table, 1, 1 );
	evas_object_size_hint_weight_set( table, 0.0, 0.0 );
	evas_object_size_hint_align_set( table, EVAS_HINT_FILL, EVAS_HINT_FILL );
	elm_layout_content_set( wd->ly_content, "month/swallow/days/c", table );

	_calendar_update( wd, &st );
	//evas_object_event_callback_add( wd->table, EVAS_CALLBACK_RESIZE, _calendar_resize_table_cb, wd );

	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _calendar_show, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _calendar_hide, wd);

	_sizing_eval(obj);
	return obj;
}

EAPI void 
elm_calendar_show(Evas_Object* obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	evas_object_show(wd->layout);
}

EAPI void 
elm_calendar_hide(Evas_Object* obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	evas_object_hide(wd->layout);
}

EAPI void 
elm_calendar_return_callback_set(Evas_Object* obj, void* return_cb, void* cb_data)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	wd->return_cb = return_cb;
	wd->cb_data = cb_data;
}
