#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup NoContents NoContents
 * @ingroup Elementary
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *noc;
   Evas_Object *custom;
   const char *label;
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
     _elm_theme_object_set(obj, wd->noc, "nocontents", "base", elm_widget_style_get(obj));
   edje_object_scale_set(wd->noc, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   edje_object_size_min_calc(wd->noc, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_size_hint_align_set(obj, maxw, maxh);
}

/**
 * Add a nocontents object to @p parent
 *
 * @param parent The parent object
 *
 * @return The nocontents object, or NULL upon failure
 *
 * @ingroup NoContents
 */
EAPI Evas_Object *
elm_nocontents_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "nocontents");
   elm_widget_type_set(obj, "nocontents");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->label = NULL;
   wd->custom = NULL;

   wd->noc = edje_object_add(e);
   _elm_theme_object_set(obj, wd->noc, "nocontents", "base", "default");
   elm_widget_resize_object_set(obj, wd->noc);
   _sizing_eval(obj);
   return obj;
}

/**
 * Set the label on the nocontents object
 *
 * @param obj The nocontents object
 * @param label The label will be used on the nocontents object
 *
 * @ingroup NoContents
 */
EAPI void
elm_nocontents_label_set(Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!label) label = "";
   eina_stringshare_replace(&wd->label, label);
   edje_object_part_text_set(wd->noc, "elm.text", label);
   _sizing_eval(obj);
}

/**
 * Get the label used on the nocontents object
 *
 * @param obj The nocontentsl object
 * @return The string inside the label
 * @ingroup NoContents
 */
EAPI const char *
elm_nocontents_label_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label;
}

/**
 * Set the custom object used on the nocontents object
 *
 * @param obj The nocontentsl object
 * @param custom The custom object will be used on the nocontents object
 * @ingroup NoContents
 */
EAPI void 
elm_nocontents_custom_set(const Evas_Object *obj, Evas_Object *custom)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!custom) return;
   edje_object_part_swallow(wd->noc, "custom", custom);
   wd->custom = custom;
}

/**
 * Get the custom object used on the nocontents object
 *
 * @param obj The nocontentsl object
 * @return The custom object inside the nocontents
 * @ingroup NoContents
 */
EAPI Evas_Object *
elm_nocontents_custom_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->custom;
}
