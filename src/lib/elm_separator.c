#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Separator Separator
 *
 * A separator is a widget that adds a very thin object to separate other objects.
 * A separator can be vertical or horizontal.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *sep;
   Eina_Bool horizontal;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

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
   if (wd->horizontal)
     _elm_theme_object_set(obj, wd->sep, "separator", "horizontal", elm_widget_style_get(obj));
   else
     _elm_theme_object_set(obj, wd->sep, "separator", "vertical", elm_widget_style_get(obj));
   edje_object_scale_set(wd->sep, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   edje_object_size_min_calc(wd->sep, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_size_hint_align_set(obj, maxw, maxh);
}

/**
 * Add a separator object to @p parent
 *
 * @param parent The parent object
 *
 * @return The separator object, or NULL upon failure
 *
 * @ingroup Separator
 */
EAPI Evas_Object *
elm_separator_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "separator");
   wd->horizontal = EINA_FALSE;
   elm_widget_type_set(obj, "separator");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->sep = edje_object_add(e);
   _elm_theme_object_set(obj, wd->sep, "separator", "vertical", "default");
   elm_widget_resize_object_set(obj, wd->sep);
   _sizing_eval(obj);
   return obj;
}

/**
 * Set the horizontal mode of a separator object
 *
 * @param obj The separator object
 * @param horizontal If true, the separator is horizontal
 *
 * @ingroup Separator
 */
EAPI void
elm_separator_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   horizontal = !!horizontal;
   if (wd->horizontal == horizontal) return;
   wd->horizontal = horizontal;
   _theme_hook(obj);
}

/**
 * Get the horizontal mode of a separator object
 *
 * @param obj The separator object
 * @return If true, the separator is horizontal
 *
 * @ingroup Separator
 */
EAPI Eina_Bool
elm_separator_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->horizontal;
}
