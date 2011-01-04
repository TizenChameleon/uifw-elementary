/*
 * @defgroup WiperControl WiperControl
 * @ingroup Elementary
 *
 * This is a wipercontrol.
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

	_elm_theme_object_set(obj,wd->sd, "wipercontrol", "base", elm_widget_style_get(obj));

	_sizing_eval(obj);

	evas_object_show (wd->sd);
}

static void
_sizing_eval(Evas_Object *obj)
{
//	Widget_Data *wd = elm_widget_data_get(obj);
}

/**
 * Set below content to the wipercontrol
 *
 * @param[in] obj Wipercontrol object
 * @param[in] content Custom object
 * @ingroup WiperControl
 */
EAPI void
elm_wipercontrol_below_content_set (Evas_Object *obj, Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (!content) return;

	edje_object_part_swallow (wd->sd, "content.below.swallow", content);
}

/**
 * Set above content to the wipercontrol
 *
 * @param[in] obj Wipercontrol object
 * @param[in] content Custom object
 * @ingroup WiperControl
 */
EAPI void
elm_wipercontrol_above_content_set (Evas_Object *obj, Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (!content) return;

	edje_object_part_swallow (wd->sd, "content.above.swallow", content);
}

/**
 * Add a new wipercontrol to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 * @ingroup WiperControl
 */
EAPI Evas_Object *
elm_wipercontrol_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "wipercontrol");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_can_focus_set(obj, 0);

	wd->sd = edje_object_add(e);

	_elm_theme_object_set(obj,wd->sd, "wipercontrol", "base", "default");

	elm_widget_resize_object_set(obj, wd->sd);

	_sizing_eval(obj);

	return obj;
}
