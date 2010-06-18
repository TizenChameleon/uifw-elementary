#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Titlebar Titlebar
 *
 * This is a titlebar widget. It can contain simple label,icon 
 * and other objects.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *tbar;
   Evas_Object *icon;
   Evas_Object *end;
   const char *label;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object * obj);
static void _theme_hook(Evas_Object * obj);
static void _sizing_eval(Evas_Object * obj);
static void _changed_size_hints(void *data, Evas * e, Evas_Object * obj, void *event_info);
static void _sub_del(void *data, Evas_Object * obj, void *event_info);

static void
_del_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->label) eina_stringshare_del(wd->label);
   free(wd);
}

static void
_theme_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_theme_object_set(obj, wd->tbar, "titlebar", "base", elm_widget_style_get(obj));
   edje_object_part_text_set(wd->tbar, "elm.text", wd->label);
   edje_object_scale_set(wd->tbar, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   edje_object_size_min_calc(wd->tbar, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object * obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
   if (sub == wd->icon)
     {
	edje_object_signal_emit(wd->tbar, "elm,state,icon,hidden", "elm");
	wd->icon = NULL;
	edje_object_message_signal_process(wd->tbar);
     }
   else if (sub == wd->end) wd->end = NULL;
   _sizing_eval(obj);
}

/**
 * Add a new titlebar object.
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_add(Evas_Object * parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "titlebar");
   elm_widget_type_set(obj, "titlebar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->tbar = edje_object_add(e);
   _elm_theme_object_set(obj, wd->tbar, "titlebar", "base", "default");
   elm_widget_resize_object_set(obj, wd->tbar);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * This set's the label in titlebar object.
 *
 * @param obj The titlebar object
 * @param label The label text
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_label_set(Evas_Object * obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->label, label);
   edje_object_part_text_set(wd->tbar, "elm.text", label);
   _sizing_eval(obj);
}

/**
 * This get's the label packed in titlebar object.
 *
 * @param obj The titlebar object
 * @return label text
 *
 * @ingroup Titlebar
 */
EAPI const char *
elm_titlebar_label_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label;
}

/**
 * This set's the icon object in titlebar object.
 *
 * @param obj The titlebar object
 * @param icon The icon object
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_icon_set(Evas_Object * obj, Evas_Object * icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->icon == icon) return;
   if (wd->icon) evas_object_del(wd->icon);
   wd->icon = icon;
   if (icon)
     {
	elm_widget_sub_object_add(obj, icon);
	edje_object_part_swallow(wd->tbar, "elm.swallow.icon", icon);
	evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	edje_object_signal_emit(wd->tbar, "elm,state,icon,visible", "elm");
	edje_object_message_signal_process(wd->tbar);
     }
   _sizing_eval(obj);
}

/**
 * This get's the icon object packed in titlebar object.
 *
 * @param obj The titlebar object
 * @return The icon object
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_icon_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon;
}

/**
 * This set's the end object in titlebar object.
 *
 * @param obj The titlebar object
 * @param end The end object
 *
 * @ingroup Titlebar
 */
EAPI void
elm_titlebar_end_set(Evas_Object * obj, Evas_Object * end)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->end == end) return;
   if (wd->end) evas_object_del(wd->end);
   wd->end = end;
   if (end)
     {
	elm_widget_sub_object_add(obj, end);
	evas_object_event_callback_add(end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	edje_object_part_swallow(wd->tbar, "elm.swallow.end", end);
     }
   _sizing_eval(obj);
}

/**
 * This get's the end object packed in titlebar object.
 *
 * @param obj The titlebar object
 * @return The icon object
 *
 * @ingroup Titlebar
 */
EAPI Evas_Object *
elm_titlebar_end_get(Evas_Object * obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->end;
}
