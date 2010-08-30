/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Searchbar Searchbar
 * @ingroup Elementary
 *
 * This is Searchbar. 
 * It can contain a simple entry and button object.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *base, *eb, *cancel_btn;
   Eina_Bool cancel_btn_ani_flag;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _changed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cancel_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_reset_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);

static void _del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   free(wd);
}

static void _theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   _elm_theme_object_set(obj, wd->base, "searchbar", "base", elm_widget_style_get(obj));

   if (wd->eb)
     edje_object_part_swallow(wd->base, "search_textfield", wd->eb);
   if (wd->cancel_btn)
     edje_object_part_swallow(wd->base, "button_cancel", wd->cancel_btn);

   edje_object_signal_callback_add(wd->base, "elm,action,click", "", _signal_reset_clicked, obj);

   edje_object_scale_set(wd->cancel_btn, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void _sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void _clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   elm_entry_cursor_end_set(elm_editfield_entry_get(wd->eb));
   if (wd->cancel_btn_ani_flag == EINA_TRUE)
     edje_object_signal_emit(wd->base, "CANCELIN", "PROG");
   else
     edje_object_signal_emit(wd->base, "CANCELSHOW", "PROG");

   evas_object_smart_callback_call(data, "clicked", NULL);
}

static void _changed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;

   evas_object_smart_callback_call(data, "changed", NULL);
}

static void _cancel_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->cancel_btn_ani_flag == EINA_TRUE)
     edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
   else
     edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");


   const char* text;
   text = elm_entry_entry_get(elm_editfield_entry_get(wd->eb));
   if (text != NULL && strlen(text) > 0)
     elm_entry_entry_set(elm_editfield_entry_get(wd->eb), NULL);

   evas_object_smart_callback_call(data, "cancel,clicked", NULL);
}

static void _signal_reset_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   elm_entry_entry_set(elm_editfield_entry_get(wd->eb), NULL);
}

/**
 * Add a new searchbar to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Searchbar
 */
EAPI Evas_Object *elm_searchbar_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (e == NULL) return NULL;
   obj = elm_widget_add(e);
   if (obj == NULL) return NULL;
   elm_widget_type_set(obj, "searchbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 1 );

   wd->base = edje_object_add(e);
   if (wd->base == NULL) return NULL;

   _elm_theme_object_set(obj, wd->base, "searchbar", "base", "default");

   //	evas_object_size_hint_weight_set(wd->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   //	evas_object_size_hint_align_set(wd->base, EVAS_HINT_FILL, EVAS_HINT_FILL);

   // Add Entry
   wd->eb = elm_editfield_add(parent);
   elm_object_style_set(wd->eb, "searchbar");
   edje_object_part_swallow(wd->base, "search_textfield", wd->eb);
//   elm_editfield_guide_text_set(wd->eb, "Search");
   elm_editfield_entry_single_line_set(wd->eb, EINA_TRUE);
   elm_editfield_eraser_set(wd->eb, EINA_TRUE);
   evas_object_smart_callback_add(wd->eb, "clicked", _clicked, obj);
   evas_object_smart_callback_add(wd->eb, "changed", _changed, obj);
   elm_widget_sub_object_add(obj, wd->eb);

   // Add Button
   wd->cancel_btn = elm_button_add(parent);
   edje_object_part_swallow(wd->base, "button_cancel", wd->cancel_btn);
   elm_object_style_set(wd->cancel_btn, "custom/darkblue");
   elm_button_label_set(wd->cancel_btn, "Cancel");
   evas_object_smart_callback_add(wd->cancel_btn, "clicked", _cancel_clicked, obj);
   elm_widget_sub_object_add(obj, wd->cancel_btn);

   wd->cancel_btn_ani_flag = EINA_FALSE;

   edje_object_signal_callback_add(wd->base, "elm,action,click", "", _signal_reset_clicked, obj);

   elm_widget_resize_object_set(obj, wd->base);

   _sizing_eval(obj);

   return obj;
}

/**
 * get the text of entry
 *
 * @param obj The entry object
 * @return The title of entry
 *
 * @ingroup entry
 */
EAPI void elm_searchbar_text_set(Evas_Object *obj, const char *entry)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   elm_entry_entry_set(elm_editfield_entry_get(wd->eb), entry);
}

/**
 * get the text of entry
 *
 * @param obj The entry object
 * @return The title of entry
 *
 * @ingroup entry
 */
EAPI const char* elm_searchbar_text_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return elm_entry_entry_get(elm_editfield_entry_get(wd->eb));
}

/**
 * get the pointer of entry
 *
 * @param obj The entry object
 * @return The title of entry
 *
 * @ingroup entry
 */
EAPI Evas_Object *elm_searchbar_entry_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return elm_editfield_entry_get(wd->eb);
}

/**
 * get the pointer of entry
 *
 * @param obj The entry object
 * @return The title of entry
 *
 * @ingroup entry
 */
EAPI void elm_searchbar_cancel_button_animation_set(Evas_Object *obj, Eina_Bool cancel_btn_ani_flag)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (cancel_btn_ani_flag == EINA_TRUE)
     wd->cancel_btn_ani_flag = EINA_TRUE;
   else
     wd->cancel_btn_ani_flag = EINA_FALSE;
}
