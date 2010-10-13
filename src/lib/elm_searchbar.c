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
   Eina_Bool cancel_btn_show_mode;
   Eina_Bool boundary_mode;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _changed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cancel_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);

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

   if (wd->cancel_btn_show_mode)
   {
	   if (wd->cancel_btn_ani_flag)
		   edje_object_signal_emit(wd->base, "CANCELIN", "PROG");
	   else
		   edje_object_signal_emit(wd->base, "CANCELSHOW", "PROG");
   }

   evas_object_smart_callback_call(data, "clicked", NULL);
}

static void _changed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;

   // TODO : inform to use entry changed callback 
//   evas_object_smart_callback_call(data, "changed", NULL);
}

static void _cancel_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->cancel_btn_show_mode)
   {
	   if (wd->cancel_btn_ani_flag)
		   edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
	   else
		   edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");
   }

   const char* text;
   text = elm_entry_entry_get(elm_editfield_entry_get(wd->eb));
   if (text != NULL && strlen(text) > 0)
     elm_entry_entry_set(elm_editfield_entry_get(wd->eb), NULL);

   evas_object_smart_callback_call(data, "cancel,clicked", NULL);
}

static void
_searchicon_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (!strcmp(source, "search_icon"))
     evas_object_smart_callback_call(data, "searchsymbol,clicked", NULL);
   else if (!strcmp(source, "base_bg"))
     _clicked(data, obj, emission, source);
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

   _elm_theme_object_set(obj, wd->base, "searchbar", "base", "default_with_bd");

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
   evas_object_smart_callback_add(elm_editfield_entry_get(wd->eb), "changed", _changed, obj);
   edje_object_signal_callback_add(wd->base, "mouse,up,1", "*", _searchicon_clicked, obj);

   elm_widget_sub_object_add(obj, wd->eb);

   // Add Button
   wd->cancel_btn = elm_button_add(parent);
   edje_object_part_swallow(wd->base, "button_cancel", wd->cancel_btn);
   elm_object_style_set(wd->cancel_btn, "custom/darkblue");
   elm_button_label_set(wd->cancel_btn, "Cancel");
   evas_object_smart_callback_add(wd->cancel_btn, "clicked", _cancel_clicked, obj);
   elm_widget_sub_object_add(obj, wd->cancel_btn);

   wd->cancel_btn_ani_flag = EINA_FALSE;
   wd->cancel_btn_show_mode = EINA_TRUE;
   wd->boundary_mode = EINA_TRUE;

   elm_widget_resize_object_set(obj, wd->base);

   _sizing_eval(obj);

   return obj;
}

/**
 * set the text of entry
 *
 * @param obj The searchbar object
 * @return void
 *
 * @ingroup Searchbar
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
 * @param obj The searchbar object
 * @return string pointer of entry
 *
 * @ingroup Searchbar
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
 * @param obj The searchbar object
 * @return the entry object
 *
 * @ingroup Searchbar
 */
EAPI Evas_Object *elm_searchbar_entry_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return elm_editfield_entry_get(wd->eb);
}

/**
 * set the cancel button animation flag
 *
 * @param obj The searchbar object
 * @param cancel_btn_ani_flag The flag of animating cancen button or not
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_cancel_button_animation_set(Evas_Object *obj, Eina_Bool cancel_btn_ani_flag)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->cancel_btn_ani_flag == cancel_btn_ani_flag) return;
   else wd->cancel_btn_ani_flag = cancel_btn_ani_flag;
}

/**
 * set the cancel button show mode
 *
 * @param obj The searchbar object
 * @param visible The flag of cancen button show or not
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_cancel_button_set(Evas_Object *obj, Eina_Bool visible)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->cancel_btn_show_mode == visible) return;
   else wd->cancel_btn_show_mode = visible;

   if (!visible)
   {
	   if (wd->cancel_btn_show_mode && wd->cancel_btn_ani_flag)
		   edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
	   else
		   edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");

	   _sizing_eval(obj);
   }
}

/**
 * clear searchbar content
 *
 * @param obj The searchbar object
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_clear(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->cancel_btn_show_mode)
   {
	   if (wd->cancel_btn_ani_flag)
		   edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
	   else
		   edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");
   }

   elm_entry_entry_set(elm_editfield_entry_get(wd->eb), NULL);
}

/**
 * set the searchbar background mode(with bg rect) set
 *
 * @param obj The searchbar object
 * @param boundary The flag of background mode or not
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_boundary_rect_set(Evas_Object *obj, Eina_Bool boundary)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->boundary_mode == boundary) return;
   else wd->boundary_mode = boundary;

   if (wd->boundary_mode)
   {
      _elm_theme_object_set(obj, wd->base, "searchbar", "base", "default_with_bd");

   }
   else
   {
      _elm_theme_object_set(obj, wd->base, "searchbar", "base", "default");
   }

   _sizing_eval(obj);
}
