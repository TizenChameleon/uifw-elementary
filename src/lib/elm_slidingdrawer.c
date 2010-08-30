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
	Evas_Object *win;
	Evas_Object *parent;
	Evas_Object *base;
	Evas_Coord_Rectangle win_rect;
	Evas_Coord_Rectangle base_rect;
	Evas_Coord handler_h;
	Evas_Coord drag;
	Evas_Coord_Point down_pos;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _slidingdrawer_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _slidingdrawer_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
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
	_elm_theme_object_set(obj, wd->base, "slidingdrawer", "base", elm_widget_style_get(obj));
}

static void
_parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static void
_slidingdrawer_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data*) data;
	evas_object_show(wd->win);
}

static void
_slidingdrawer_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data*) data;
	evas_object_hide(wd->win);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Evas_Coord x, y, w, h;
	Widget_Data *wd;

	wd = elm_widget_data_get(obj);

	evas_object_geometry_get(wd->parent, &x, &y, &w, &h);
	evas_object_move(obj, x, y);
	evas_object_resize(obj, w, h);

	/*
	ecore_x_window_geometry_get(ecore_x_window_root_get(ecore_x_window_focus_get()), &wd->win_rect.x, &wd->win_rect.y, &wd->win_rect.w, &wd->win_rect.h);
	edje_object_part_geometry_get(wd->base, "handler", NULL, NULL, NULL, &wd->handler_h);
	evas_object_resize(wd->win, wd->win_rect.w, wd->handler_h);
	evas_object_move(wd->win, 0, wd->win_rect.h - wd->handler_h);
	evas_object_resize(wd->base, wd->win_rect.w, wd->handler_h);
	wd->base_rect.y = 0;
	wd->base_rect.h = wd->handler_h;
	*/
}

void
_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev;
	Widget_Data *wd = (Widget_Data*) data;

	ev = (Evas_Event_Mouse_Down *) event_info;
	wd = (Widget_Data*) data;

	evas_object_move(wd->win, 0, 0 );
	evas_object_resize(wd->win, wd->win_rect.w, wd->win_rect.h);
	evas_object_move(wd->base, 0, wd->win_rect.h - wd->base_rect.h);
	wd->base_rect.y = wd->win_rect.h - wd->base_rect.h;
	wd->down_pos.x = ev->output.x;
	wd->down_pos.y = wd->win_rect.h - wd->base_rect.h + ev->output.y;
	fprintf( stderr, "DOWN! window - %d %d %d %d\n", 0, 0, wd->win_rect.w, wd->win_rect.h);
	fprintf( stderr, "DOWN! base - %d %d\n", 0, wd->win_rect.h - wd->base_rect.h );

}

void
_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ev;
	Widget_Data *wd;

	ev = (Evas_Event_Mouse_Move *) event_info;

	if(!ev->buttons)
			return ;

	wd  = (Widget_Data*) data;

	wd->drag = ev->cur.output.y - wd->down_pos.y;
	evas_object_move(wd->base, 0, wd->base_rect.y + wd->drag );
	evas_object_resize(wd->base, wd->win_rect.w, wd->handler_h - wd->drag );
}

void
_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev;
	Widget_Data *wd;
	ev = (Evas_Event_Mouse_Up *) event_info;

	wd = (Widget_Data *) data;

	wd->base_rect.y = wd->base_rect.y + wd->drag;
	wd->base_rect.h = wd->handler_h - wd->drag;
	evas_object_move(wd->win, 0, wd->base_rect.y);
	evas_object_resize(wd->win, wd->win_rect.w, wd->base_rect.h);
	evas_object_move(wd->base, 0, 0);
	wd->drag = 0;
//	fprintf( stderr, "UP! window - %d %d %d %d\n", 0, wd->base_rect.y, wd->win_rect.w, wd->base_rect.h);
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
	_elm_theme_object_set(obj, wd->base, "slidingdrawer", "base", "default");
	elm_widget_sub_object_add(obj, wd->base);
	elm_widget_resize_object_set(obj, wd->base);

	//handler
	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);
	//evas_object_event_callback_add(handler, EVAS_CALLBACK_MOUSE_DOWN, _down, wd);
	//evas_object_event_callback_add(handler, EVAS_CALLBACK_MOUSE_MOVE, _move, wd);
	//evas_object_event_callback_add(handler, EVAS_CALLBACK_MOUSE_UP, _up, wd);
	//evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _slidingdrawer_show, wd);
	//evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _slidingdrawer_hide, wd);

	_sizing_eval(obj);

	return obj;
}

