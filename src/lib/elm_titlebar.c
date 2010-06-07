#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Titlebar Titlebar
 *
 * This is a titlebar. It can contain simple label and icon objects.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *base;
	Evas_Object *label;
	Evas_Object *icon;
	Evas_Object *end;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if(wd->label) evas_object_del(wd->label);
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	char buf[1024];
	if (!wd) return;

	elm_layout_theme_set(wd->base, "titlebar", "base", elm_widget_style_get(obj));

	snprintf(buf, sizeof(buf), "titlebar/%s", elm_widget_style_get(obj));
	elm_object_style_set(wd->label, buf);

	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;

	edje_object_size_min_calc(elm_layout_edje_get(wd->base), &minw, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);
}

/**
 * Add a new titlebar object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "titlebar");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = elm_layout_add(obj);
	elm_layout_theme_set(wd->base, "titlebar", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);
	evas_object_size_hint_weight_set(wd->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->base, EVAS_HINT_FILL, EVAS_HINT_FILL);

	wd->label = elm_label_add(obj);
	elm_object_style_set(wd->label, "titlebar/default");

	elm_layout_content_set(wd->base, "elm.swallow.label", wd->label);

	_sizing_eval(obj);

	return obj;
}

/**
 * Set the label of titlebar object
 *
 * @param obj The titlebar object
 * @param label The label text
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_label_set(Evas_Object *obj, const char *label)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	elm_label_label_set(wd->label, label);

	_sizing_eval(obj);
}

/**
 * Get the label used on the titlebar object
 *
 * @param obj The titlebar object
 * @return label text
 *
 * @ingroup Titlebar
 */
EAPI const char*
elm_titlebar_label_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return elm_label_label_get(wd->label);
}

/**
 * Set the icon object of the titlebar object
 *
 * @param obj The titlebar object
 * @param icon The icon object
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_icon_set(Evas_Object *obj, Evas_Object *icon)
{
	Widget_Data *wd = elm_widget_data_get(obj);
   	if (!wd) return;
   	if ((wd->icon != icon) && (wd->icon))
     		elm_widget_sub_object_del(obj, wd->icon);
	if ((icon) && (wd->icon != icon))
     	{
		wd->icon = icon;
		elm_widget_sub_object_add(obj, icon);
		/*
		evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
		*/
		elm_layout_content_set(wd->base, "elm.swallow.icon", icon);
		edje_object_signal_emit(elm_layout_edje_get(wd->base), "elm,state,icon,visible", "elm");
		_sizing_eval(obj);
	}
   	else
    		wd->icon = icon;

}

/**
 * Get the icon object of the titlebar object
 *
 * @param obj The titlebar object
 * @return The icon object
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_icon_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
   	if (!wd) return NULL;

	return wd->icon;
}

/**
 * Set the end object of the titlebar object
 *
 * @param obj The titlebar object
 * @param end The end object
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_end_set(Evas_Object *obj, Evas_Object *end)
{
	Widget_Data *wd = elm_widget_data_get(obj);
   	if (!wd) return;
   	if ((wd->end != end) && (wd->end))
     		elm_widget_sub_object_del(obj, wd->end);
	if ((end) && (wd->end != end))
     	{
		wd->end = end;
		elm_widget_sub_object_add(obj, end);
		/*
		evas_object_event_callback_add(end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
		*/
		elm_layout_content_set(wd->base, "elm.swallow.end", end);
		edje_object_signal_emit(elm_layout_edje_get(wd->base), "elm,state,end,visible", "elm");
		_sizing_eval(obj);
	}
   	else
    		wd->end = end;
}

/**
 * Get the end object of the titlebar object
 *
 * @param obj The titlebar object
 * @return The icon object
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_end_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
   	if (!wd) return NULL;

	return wd->end;
}
