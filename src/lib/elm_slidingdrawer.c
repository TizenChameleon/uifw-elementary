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
	Elm_SlidingDrawer_Pos pos;
	double 	max_drag_dw;
	double  max_drag_dh;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);

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
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd;
	Evas_Coord x, y, w, h;
	Evas_Object  *part;

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
elm_slidingdrawer_content_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;

	Evas_Object *swallow;
	Widget_Data *wd;

	wd = elm_widget_data_get(obj);
	swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.content");
	edje_object_part_unswallow(wd->base, swallow);
	return swallow;
}

EAPI void
elm_slidingdrawer_content_set (Evas_Object *obj, Evas_Object *content)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

	Widget_Data *wd = elm_widget_data_get(obj);
	if (!content) return;

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
elm_slidingdrawer_drag_max_set(Evas_Object *obj, double dw,  double dh)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

	Widget_Data *wd = elm_widget_data_get(obj);
	wd->max_drag_dw = dw;
	wd->max_drag_dh = dh;
}


EAPI Evas_Object *
elm_slidingdrawer_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas_Object *handler;
	Evas *e;
	Widget_Data *wd;

	if(!parent)
		return ;

	wd = ELM_NEW(Widget_Data);
	if(!wd) return NULL;

	ELM_SET_WIDTYPE(widtype, "slidingdrawer");

	wd->parent = parent;

	e = evas_object_evas_get(parent);

	//window
	/*
	wd->win = elm_win_add(parent, "slidingdrawer", ELM_WIN_BASIC);
	elm_win_autodel_set(wd->win, EINA_TRUE);
	elm_win_borderless_set(wd->win, EINA_TRUE);
	elm_win_alpha_set(wd->win, EINA_TRUE);
	e = evas_object_evas_get(wd->win);

	//widget
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "slidingdrawer");
	elm_widget_sub_object_add(wd->win, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_win_resize_object_add(wd->win, obj);

	//base
	wd->base = edje_object_add(e);
	_elm_theme_object_set(wd->win, wd->base, "slidingdrawer", "base", "default");
	elm_widget_sub_object_add(obj, wd->base);
	evas_object_show(wd->base);
	elm_widget_resize_object_set(obj, wd->base);
	*/

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
	elm_widget_sub_object_add(obj, wd->base);
	elm_widget_resize_object_set(obj, wd->base);

	//handler
	wd->handler = evas_object_rectangle_add(e);
	evas_object_color_set(wd->handler, 0, 0, 0, 0);
	edje_object_part_swallow(wd->base, "elm.dragable.handler", wd->handler);

	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);

	_sizing_eval(obj);

	return obj;
}

