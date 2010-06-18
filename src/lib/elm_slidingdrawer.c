/*
 * @addgroup SlidingDrawer
 *
 * This is a slidingdrawer.
 */
#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *sd;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	free(wd);
}

/*
 * FIXME:
 */
static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_object_set(obj,wd->sd, "slidingdrawer", "base", elm_widget_style_get(obj));

	_sizing_eval(obj);

	evas_object_show (wd->sd);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
}

/**
 * Set content to the slidingdrawer
 *
 * @param[in] obj Slidingdrawer object
 * @param[in] content Custom object
 * @ingroup SlidingDrawer
 */
EAPI void
elm_slidingdrawer_content_set (Evas_Object *obj, Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (!content) return;

	edje_object_part_swallow (wd->sd, "content.swallow", content);
}

/**
 * Add a new slidingdrawer to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 * @ingroup SlidingDrawer
 */
EAPI Evas_Object *
elm_slidingdrawer_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "slidingdrawer");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_can_focus_set(obj, 0);

	wd->sd = edje_object_add(e);

	_elm_theme_object_set(obj,wd->sd, "slidingdrawer", "base", "default");

	elm_widget_resize_object_set(obj, wd->sd);

	_sizing_eval(obj);

	return obj;
}

