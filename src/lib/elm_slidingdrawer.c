/*
 * @defgroup SlidingDrawer SlidingDrawer
 * @ingroup Elementary
 *
 * This is a slidingdrawer.
 */
#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *parent;
	Evas_Object *base;
	Evas_Object *handler;
	Evas_Object *dragable_rect;
	Elm_SlidingDrawer_Pos pos;
	Evas_Coord max_drag_w;
	Evas_Coord max_drag_h;
	Elm_SlidingDrawer_Drag_Value value;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _drag_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _up_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _down_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	elm_slidingdrawer_pos_set(obj, wd->pos);
}

static void
_parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static void 
_drag_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	edje_object_part_drag_value_get(wd->base, "elm.dragable.handler", &wd->value.x, &wd->value.y);
	evas_object_smart_callback_call(data, "mouse,move", (void*) &wd->value);
}

static void
_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	edje_object_part_drag_value_get(wd->base, "elm.dragable.handler", &wd->value.x, &wd->value.y);
	evas_object_smart_callback_call(data, "mouse,up", (void*) &wd->value);
}

static void
_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	edje_object_part_drag_value_get(wd->base, "elm.dragable.handler", &wd->value.x, &wd->value.y);
	evas_object_smart_callback_call(data, "mouse,down", (void*) &wd->value);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd;
	Evas_Coord x, y, w, h;
	const Evas_Object  *part;

	wd = elm_widget_data_get(obj);

	evas_object_geometry_get(wd->parent, &x, &y, &w, &h);
	evas_object_move(obj, x, y);
	evas_object_resize(obj, w, h);

	part = edje_object_part_object_get(wd->base, "elm.dragable.handler");
	
	if((wd->pos == ELM_SLIDINGDRAWER_TOP) || (wd->pos == ELM_SLIDINGDRAWER_BOTTOM)) {
		edje_object_size_min_get(part, NULL, &h);
	}else {
		edje_object_size_min_get(part, &w, NULL);
	}

	evas_object_size_hint_min_set(wd->handler, w, h);
}

EAPI Evas_Object *
elm_slidingdrawer_content_unset(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd;
	Evas_Object *content;

	wd = elm_widget_data_get(obj);

	content = edje_object_part_swallow_get(wd->base, "elm.swallow.content");
	if(!content) return NULL;
	edje_object_part_unswallow(wd->base, content);
	elm_widget_sub_object_del(obj, content);
	return content;
}

EAPI void
elm_slidingdrawer_content_set (Evas_Object *obj, Evas_Object *content)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

	Widget_Data *wd = elm_widget_data_get(obj);
	if (!content) return;

	elm_widget_sub_object_add(obj, content);
	edje_object_part_swallow (wd->base, "elm.swallow.content", content);
}

EAPI void
elm_slidingdrawer_pos_set(Evas_Object *obj, Elm_SlidingDrawer_Pos pos)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

	Widget_Data *wd = elm_widget_data_get(obj);

	switch(pos)
	{
	case ELM_SLIDINGDRAWER_BOTTOM:
		_elm_theme_object_set(obj, wd->base, "slidingdrawer", "bottom", elm_widget_style_get(obj));
		break;
	case ELM_SLIDINGDRAWER_LEFT:
		_elm_theme_object_set(obj, wd->base, "slidingdrawer", "left", elm_widget_style_get(obj));
		break;
	case ELM_SLIDINGDRAWER_RIGHT:
		_elm_theme_object_set(obj, wd->base, "slidingdrawer", "right", elm_widget_style_get(obj));
		break;
	case ELM_SLIDINGDRAWER_TOP:
		_elm_theme_object_set(obj, wd->base, "slidingdrawer", "top", elm_widget_style_get(obj));
		break;
	}

	edje_object_part_drag_value_set(wd->base, "elm.dragable.handler", 0, 0);
	wd->pos = pos;
	_sizing_eval(obj);
}

EAPI void
elm_slidingdrawer_drag_value_set(Evas_Object *obj, double dx, double dy)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd;
	wd = elm_widget_data_get(obj);
	edje_object_part_drag_value_set(wd->base, "elm.dragable.handler", dx, dy);
}

EAPI void
elm_slidingdrawer_max_drag_value_set(Evas_Object *obj, double dw,  double dh)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd;
	Evas_Coord w, h;

	wd = elm_widget_data_get(obj);
	wd->max_drag_w = dw;
	wd->max_drag_h = dh;
	_sizing_eval(obj);

	evas_object_geometry_get(wd->parent, NULL, NULL, &w, &h);
	evas_object_size_hint_max_set(wd->dragable_rect, ((double) w) * dw, ((double) h) * dh);
}


EAPI Evas_Object *
elm_slidingdrawer_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	if(!wd) return NULL;

	ELM_SET_WIDTYPE(widtype, "slidingdrawer");

	wd->max_drag_w = 1;
	wd->max_drag_h = 1;

	wd->parent = parent;
	e = evas_object_evas_get(parent);

	//widget
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "slidingdrawer");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_can_focus_set(obj, EINA_FALSE);

	//base
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "slidingdrawer", "bottom", "default");
	edje_object_signal_callback_add(wd->base, "drag", "*", _drag_cb, obj);
	edje_object_signal_callback_add(wd->base, "mouse,up,*", "*", _up_cb, obj);
	edje_object_signal_callback_add(wd->base, "mouse,down,*", "*", _down_cb, obj);
	elm_widget_sub_object_add(obj, wd->base);
	elm_widget_resize_object_set(obj, wd->base);
	
	//dragable_rect
	wd->dragable_rect = evas_object_rectangle_add(e);
	edje_object_part_swallow(wd->base, "elm.swallow.dragable_rect", wd->dragable_rect);

	//handler
	wd->handler = evas_object_rectangle_add(e);
	evas_object_color_set(wd->handler, 0, 0, 0, 0);
	edje_object_part_swallow(wd->base, "elm.dragable.handler", wd->handler);
	
	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);
		
	_sizing_eval(obj);

	return obj;
}

