/*
 * SLP
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */

/**
 * @addtogroup Animatedicon Animatedicon 
 *
 * This is an animatedicon.
 */

#include <Elementary.h>
#include "elm_priv.h"

/**
 * internal data structure of animated icon object
 */
typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *parent;
	Elm_Transit *transit;
	Evas_Object *icon;
	Ecore_Timer *timer;

	char** images;
	int item_num;
	double duration;
	unsigned int repeat;
	double interval;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void _resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	if (wd->timer) {
		ecore_timer_del(wd->timer);
		wd->timer = NULL;
	}
	
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	// nothing to do with size
}

static void
_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static void
_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static int
_repeat_animation_cb(void *data)
{
	elm_animatedicon_animation_start((Evas_Object *)data);
}

/**
 * Add a new animatedicon to the parent.
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Animatedicon 
 */
EAPI Evas_Object *
elm_animatedicon_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "animatedicon");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "animatedicon", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->icon = elm_icon_add(parent);
	elm_widget_sub_object_add(obj, wd->icon);
	edje_object_part_swallow(wd->base, "elm.swallow.icon", wd->icon);

	wd->parent = parent;
	wd->repeat = 0;
	wd->interval = 0.0;

	_sizing_eval(obj);
	
	return obj;
}

/**
 * Set image files for animation. 
 *
 * @param obj animatedicon object. 
 * @param images the image files to be animated.
 * @param item_num the number of images.
 *
 * @ingroup Animatedicon 
 */
EAPI void 
elm_animatedicon_file_set(Evas_Object *obj, const char **images, const int item_num)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (images == NULL || images[0] == NULL) {
		return;
	}

	wd->images = (char **)images;
	wd->item_num = item_num;
	elm_icon_file_set(wd->icon, wd->images[0], NULL);
}

/**
 * Set animation duration time. 
 *
 * @param obj animatedicon object. 
 * @param duration duration time. 
 *
 * @ingroup Animatedicon 
 */
EAPI void
elm_animatedicon_duration_set(Evas_Object *obj, const double duration)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	wd->duration = duration;
}

/**
 * Start animatedicon animation. 
 *
 * @param obj animatedicon object. 
 *
 * @ingroup Animatedicon 
 */
EAPI void
elm_animatedicon_animation_start(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	wd->transit = elm_transit_add(wd->parent);
	elm_transit_fx_insert(wd->transit, elm_fx_imageanimation_add(wd->icon, wd->images, wd->item_num));
	elm_transit_event_block_disabled_set(wd->transit, EINA_FALSE);
	if (wd->repeat >= 0) {
		elm_transit_repeat_set(wd->transit, wd->repeat);
	}
	elm_transit_run(wd->transit, wd->duration);
	elm_transit_del(wd->transit);

	/*
	wd->repeat--;

	if (wd->repeat >= 0) {
		wd->timer = ecore_timer_add(wd->interval, _repeat_animation_cb, obj);
	} else {
		if (wd->timer) {
			ecore_timer_del(wd->timer);
			wd->timer = NULL;
		}
	}
	*/
}

/**
 * Set the repeat count and interval. 
 *
 * @param obj animatedicon object.
 * @param repeat repeat count.
 * @param interval repeat interval. 
 *
 * @ingroup Animatedicon 
 */
EAPI void
elm_animatedicon_repeat_set(Evas_Object *obj, const unsigned int repeat, const double interval)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	wd->repeat = repeat;
	wd->interval = interval;
}
