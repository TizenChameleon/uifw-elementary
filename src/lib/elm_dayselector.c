#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Dayselector Dayselector
 * @ingroup Elementary
 *
 * DaySelector. 
 *
 * Signals that you can add callbacks for are:
 *
 * dayselector,changed - This is called whenever the user changes the state of one of the check object. 
 * 
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *parent;
	Evas_Object *base;
	Evas_Object *check[7];
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
//static void _sizing_eval(Evas_Object* obj);
//static void _dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
//static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}
/*
static void
_dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(obj);
}
*/
static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord w, h;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	edje_object_size_min_get(wd->base, &w, &h);
	evas_object_size_hint_min_set(obj, w * elm_scale_get(), h * elm_scale_get());
}

static void 
_check_clicked(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(data);
	int idx;
	for(idx = 0; idx< 7; ++idx) {
		if(obj==wd->check[idx]) {
			evas_object_smart_callback_call(data, "dayselector,changed", (void *)idx);
			return ;
		}
	}
}


static void
_theme_hook(Evas_Object *obj)
{
	int idx;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return;

  _elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));

  for(idx=0; idx<7; ++idx) 
		elm_object_style_set(wd->check[idx], "dayselector");
}



/**
 * Get the state of given check object.
 *
 * @param obj  	 	Dayselector
 * @param day        day user want to know. 
 *
 * @ingroup Dayselector
 */
EAPI Eina_Bool 
elm_dayselector_check_state_get(Evas_Object *obj, Elm_DaySelector_Day day)
{
	ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return EINA_FALSE;

	return elm_check_state_get(wd->check[day]);
}
/*
static void
_sizing_eval(Evas_Object* obj)
{
	Evas_Coord w, h;
	Evas_Coord min_w, min_h, max_h;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	edje_object_size_min_get(wd->base, &min_w, &min_h);
	edje_object_size_max_get(wd->base, NULL, &max_h);

	fprintf( stderr, "%d %d\n", w, h );
	if(w < min_w) w = min_w;
	if(h < min_h) h = min_h;
	else if(h > max_h) h = max_h;

	evas_object_resize(wd->base, w, h);  
}
*/
/**
 * Add the dayselector.
 *
 * @param item 	 	Parent object.
 *
 * @ingroup Dayselector
 */
EAPI Evas_Object *
elm_dayselector_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
	int idx;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "dayselector");
   elm_widget_type_set(obj, "dayselector");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;

	//Base
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));
	elm_object_style_set(wd->base, "dayselector");
	elm_widget_resize_object_set(obj, wd->base); 

	//Buttons

	//Left-side Button
	wd->check[0]=elm_check_add(wd->base);
	elm_widget_sub_object_add(obj, wd->check[0]);
	evas_object_smart_callback_add(wd->check[0], "changed", _check_clicked, obj);

 	for(idx=1; idx<6; ++idx)
	{
		wd->check[idx]=elm_check_add(wd->base);
		elm_widget_sub_object_add(obj, wd->check[idx]);
		evas_object_smart_callback_add(wd->check[idx], "changed", _check_clicked, obj);
	}

 	//Right-side Button
 	wd->check[6]=elm_check_add(wd->base);
 	elm_widget_sub_object_add(obj, wd->check[6]);
 	evas_object_smart_callback_add(wd->check[6], "changed", _check_clicked, obj);

	elm_check_label_set(wd->check[ELM_DAYSELECTOR_SUN], "S");
	edje_object_part_swallow(wd->base, "sun", wd->check[0]);
	elm_object_style_set(wd->check[0], "dayselector_sun");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_MON], "M");
	edje_object_part_swallow(wd->base, "mon", wd->check[1]);
	elm_object_style_set(wd->check[1], "dayselector_mon");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_TUE], "T");
	edje_object_part_swallow(wd->base, "tue", wd->check[2]);
	elm_object_style_set(wd->check[2], "dayselector_tue");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_WED], "W");
	edje_object_part_swallow(wd->base, "wed", wd->check[3]);
	elm_object_style_set(wd->check[3], "dayselector_wed");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_THU], "T");
	edje_object_part_swallow(wd->base, "thu", wd->check[4]);
	elm_object_style_set(wd->check[4], "dayselector_thu");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_FRI], "F");
	edje_object_part_swallow(wd->base, "fri", wd->check[5]);
	elm_object_style_set(wd->check[5], "dayselector_fri");
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_SAT], "S");
	edje_object_part_swallow(wd->base, "sat", wd->check[6]);
	elm_object_style_set(wd->check[6], "dayselector_sat");

//	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _dayselector_resize, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);

//	_sizing_eval(obj);

   return obj;
}


