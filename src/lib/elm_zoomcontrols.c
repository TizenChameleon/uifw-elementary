/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#define ZC_DEFAULT_RANGE 5
/**
 * @defgroup ZoomControls ZoomControls
 * @ingroup Elementary
 *
 * This is a zoomcontrols. Press it and run some function.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *base;
   int count;
   int range;
   int downdefaultstate;
   int updefaultstate;
   int downdisabledstate;
   int updisabledstate;
   int downdisabledfocusedstate;
   int updisabledfocusedstate;
};


static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _signal_down_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_up_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_object_set(obj, wd->base, "zoomcontrols", "base", elm_widget_style_get(obj));
	edje_object_message_signal_process(wd->base);
	edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);
	_sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(wd->downdisabledstate)
	{
		edje_object_signal_emit(obj, "elm,state,down,disabled", "elm");
	}
	else if(wd->downdefaultstate)
	{
		edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
	}
	
	if(wd->updisabledstate)
	{
		edje_object_signal_emit(obj, "elm,state,up,disabled", "elm");
	}
	else if(wd->updefaultstate)
	{
		edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
	}
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord w, h;
   
   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);   
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   evas_object_size_hint_min_get(obj, &w, &h);
   if (w > minw) minw = w;
   if (h > minw) minh = h;

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_signal_down_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	if((wd->downdefaultstate == 0) && (wd->downdisabledstate == 1))
	{
		return;
	}
	else
	{
		edje_object_signal_emit(wd->base, "elm,state,down,selected", "elm");
	}

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count > -(wd->range))
	{
		if(wd->count == wd->range)
		{
			wd->updisabledstate = 0;
			wd->updefaultstate = 1;
			edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
		 
		}
		wd->count--;	
	        printf("smart callback call (down)\n");
		evas_object_smart_callback_call(data, "down_clicked", NULL);
	}

	printf("count %d\n", wd->count);

	if(wd->count == -(wd->range))
	{ 
		printf("down disabled \n");
		wd->downdisabledstate = 1;
		wd->downdefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,down,disabled", "elm");
	}
}
static void
_signal_up_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	if((wd->updefaultstate == 0) && (wd->updisabledstate == 1))
	{
		return;
	}
	else
	{
		edje_object_signal_emit(wd->base, "elm,state,up,selected", "elm");
	}

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count < wd->range)
	{
		if(wd->count == -(wd->range))
		{
			wd->downdisabledstate = 0;
			wd->downdefaultstate = 1;
			edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
		}
		wd->count++;
		printf("smart callback call (up)\n");
		evas_object_smart_callback_call(data, "up_clicked", NULL);

	}

	printf("count %d\n", wd->count);
}
static void
_signal_down_unclicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count == -(wd->range))
	{ 
		printf("down enabled \n");
		wd->downdisabledstate = 1;
		wd->downdefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,down,disabled", "elm");
	}
	else if(wd->count > -(wd->range))
	{
		edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
	}     
}
static void
_signal_up_unclicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count == wd->range)
	{
		printf("up enabled\n");
		wd->updisabledstate = 1;
		wd->updefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,up,disabled", "elm");
	}
	else if(wd->count < wd->range)
	{
		edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
	}     
}

/**
 * Add a new zoomcontrols to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup ZoomControls
 */
EAPI Evas_Object *
elm_zoomcontrols_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "zoomcontrols");
   elm_widget_sub_object_add(parent, obj);
   
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   
   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "zoomcontrols", "base", "default");

   wd->count = 0;
   wd->range = 0;
   wd->downdefaultstate = 1;
   wd->updefaultstate = 1;
   wd->downdisabledstate = 0;
   wd->updisabledstate = 0;
   wd->downdisabledfocusedstate = 0;
   wd->updisabledfocusedstate = 0;

   edje_object_signal_callback_add(wd->base, "elm,action,down,click", "", _signal_down_clicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,up,click", "", _signal_up_clicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,down,unclick", "", _signal_down_unclicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,up,unclick", "", _signal_up_unclicked, obj);
   
   elm_widget_resize_object_set(obj, wd->base);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the range used in the zoomcontrols
 * 
 * @param obj The zoomcontrols object
 * @param range The value will limit count that each button of zoomcontrols is clicked 
 *
 * @ingroup ZoomControls
 */
EAPI void
elm_zoomcontrols_range_set(Evas_Object *obj, int range)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd)
		return;
	
	if(range <= 0)
		return;

	wd->range = range;
}
