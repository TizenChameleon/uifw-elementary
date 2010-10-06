

/**
 *
 * @defgroup Analogclock Analogclock
 * @ingroup Elementary
 *
 * This is an analogclock.
 */


/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#define dbg(fmt,args...) printf("[%s:%d] "fmt"\n", __FILE__, __LINE__, ##args); 

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *dial,
				*hand_hour,
				*hand_min;

	Ecore_Timer *ticker;

	int hour,
		min;
};


static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static int  _ticker(void *data);
static void _time_update(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->ticker) ecore_timer_del(wd->ticker);
	free(wd);
}

static int
_ticker(void *data)
{
	Widget_Data *wd = elm_widget_data_get(data);
	double t;
	struct timeval timev;

	gettimeofday(&timev, NULL);
	t = ((double)(1000000 - timev.tv_usec)) / 1000000.0;
	wd->ticker = ecore_timer_add(t, _ticker, data);
	_time_update(data);
	return 0;
}

static Eina_Bool
_idle_draw (void *data)
{
	_ticker(data);
	return ECORE_CALLBACK_CANCEL;
}

static void
_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _time_update(data);
}

static void
_object_rotate(Evas_Object *obj, double degree)
{
	const Evas_Map *m;
	Evas_Map *m2;
	Evas_Coord x, y, w, h;

	evas_object_geometry_get (obj, &x, &y, &w, &h);
	evas_object_hide (obj);

	m = evas_object_map_get (obj);
	if (m != NULL)
		m2 = evas_map_dup(m);
	else {
		m2 = evas_map_new(4);
	}

	evas_map_util_points_populate_from_object (m2, obj);
	evas_map_util_rotate (m2, degree, x+w/2, y+h/2);
	evas_object_map_set (obj, m2);
	evas_object_map_enable_set(obj, 1);
	evas_map_free (m2);
	evas_object_show (obj);
}

static void
_time_update(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	double degree;
	Evas_Coord x, y, w, h;
	struct timeval timev;
	struct tm *tm;
	time_t tt;

	evas_object_geometry_get (wd->dial, &x, &y, &w, &h);
	if (w <= 0 || h <= 0) {
		return;
	}

	gettimeofday(&timev, NULL);
	tt = (time_t)(timev.tv_sec);
	tzset();
	tm = localtime(&tt);
	if (tm)
	{
		wd->hour = tm->tm_hour;
		wd->min = tm->tm_min;
		if (wd->hour > 12) wd->hour -= 12;
	}

	degree = 6 * wd->min;
	_object_rotate (wd->hand_min, degree);

	degree = 30 * wd->hour + (wd->min / 2);
	_object_rotate (wd->hand_hour, degree);
}

/*
 * FIXME:
 */
static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_object_set(obj,wd->dial, "analogclock", "base", elm_widget_style_get(obj));

	_sizing_eval(obj);

	evas_object_show (wd->dial);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
	Evas_Coord w, h;

	if (!wd) return;

	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->dial, &minw, &minh, minw, minh);   
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);

	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minw) minh = h;

	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);
}

/**
 * Add a new analog clock to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 */
EAPI Evas_Object *
elm_analogclock_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "analogclock");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_can_focus_set(obj, 0);

	wd->dial = edje_object_add(e);

	_elm_theme_object_set(obj,wd->dial, "analogclock", "base", "default");

	wd->hand_hour = edje_object_part_object_get (wd->dial, "hour");
	wd->hand_min = edje_object_part_object_get (wd->dial, "minute");

	evas_object_event_callback_add(wd->dial, EVAS_CALLBACK_MOVE, _move, obj);

	elm_widget_resize_object_set(obj, wd->dial);
	wd->hour = 0;
	wd->min = 0;

	_sizing_eval(obj);

	ecore_idler_add (_idle_draw, obj);

	return obj;
}

