#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Ratingbar Ratingbar
 *
 * This is a ratingbar.
 */


#define RATING_MAX 5			

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	Evas_Object *base;
	Evas_Coord x, y, w, h;
	int rating;
	bool pressed;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _update_ratingbar(Evas_Object *obj);


static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	_elm_theme_object_set(obj, wd->base, "ratingbar", "base", elm_widget_style_get(obj));
	_update_ratingbar(obj);
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
_changed(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	evas_object_smart_callback_call(obj, "changed", NULL);
}

static void
_update_ratingbar(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	char partname[20] = {0,};
	int i;
	if (!wd) return;	

	for(i=1; i<=RATING_MAX; i++)
	{
		sprintf(partname, "elm.star.%d", i);

		if(wd->pressed == FALSE){
			if(i <= wd->rating)	edje_object_signal_emit(wd->base, "elm,star,selected", partname);		
			else	edje_object_signal_emit(wd->base, "elm,star,unselected", partname);
		}else{
			if(i <= wd->rating)	edje_object_signal_emit(wd->base, "elm,star,pressed,on", partname);		
			else	edje_object_signal_emit(wd->base, "elm,star,pressed,off", partname);
		}
	}
		
	_sizing_eval(obj);
}

static int
_determine_rating(Evas_Coord x, Evas_Coord w, Evas_Coord ev_x)
{
	int rating = 0;
	
	if(ev_x < x){
		rating = 0;
	}else if(ev_x < x + 0.2*w){
		rating = 1;
	}else if(ev_x < x + 0.4*w){
		rating = 2;
	}else if(ev_x < x + 0.6*w){
		rating = 3;
	}else if(ev_x < x + 0.8*w){
		rating = 4;
	}else if(ev_x < x + w){
		rating = 5;
	}else{
		rating = 5;
	}

	return rating;
}

static void 
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);	
	if (!wd) return;

	evas_object_geometry_get(obj, NULL, NULL, &wd->w, &wd->h);
}

static void 
_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	evas_object_geometry_get(obj, &wd->x, &wd->y, NULL, NULL);
}

static void
_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Down *ev = event_info;
	if (!wd) return;	
	int rating_old = wd->rating;

	wd->pressed = TRUE;
	wd->rating = _determine_rating(wd->x, wd->w, ev->canvas.x);
	_update_ratingbar(data);	
	
	if(wd->rating != rating_old)	
		_changed(data);
}

static void
_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Move *ev = event_info;
	if (!wd || !ev->buttons) return;
	int rating_old = wd->rating;

	wd->pressed = TRUE;
	wd->rating = _determine_rating(wd->x, wd->w, ev->cur.canvas.x);
	_update_ratingbar(data);
	
	if(wd->rating != rating_old)	
		_changed(data);
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Up *ev = event_info;
	if (!wd) return;
	int rating_old = wd->rating;

	wd->pressed = FALSE;
	wd->rating  = _determine_rating(wd->x, wd->w, ev->canvas.x);
	_update_ratingbar(data);

	if(wd->rating != rating_old)	
		_changed(data);
}

static void
_event_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->base)	return;

	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOVE, _move_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up, obj);
}

/**
 * Add a new ratingbar to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Ratingbar
 */
EAPI Evas_Object *
elm_ratingbar_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "ratingbar");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "ratingbar", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);
	evas_object_show(wd->base);

	wd->rating = 0;
	wd->pressed = FALSE;
	
	_event_init(obj);
	_update_ratingbar(obj);

	return obj;
}

/**
 * Set the current rating
 *
 * @param obj The ratingbar object
 * @param rating The current rating 
 *
 * @ingroup Ratingbar
 */
EAPI void
elm_ratingbar_rating_set(Evas_Object *obj, int rating)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if(rating < 0 || rating > RATING_MAX){
		fprintf(stderr, "Invalid the value of rating\n");
		return;
	}

	if(wd->rating == rating) return;

	wd->rating = rating;
	_changed(obj);
	_update_ratingbar(obj);
}

/**
 * Get the current rating
 *
 * @param obj The ratingbar object 
 * @return The current rating
 *
 * @ingroup Ratingbar
 */
EAPI int
elm_ratingbar_rating_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return 0;

	return wd->rating;
}


