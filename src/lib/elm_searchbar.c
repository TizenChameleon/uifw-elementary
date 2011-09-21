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
   Evas_Object *base, *ef, *cancel_btn;
   Eina_Bool cancel_btn_ani_flag;
   Eina_Bool cancel_btn_show_mode;
   Eina_Bool boundary_mode;
   Ecore_Idler *idler;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _clicked(void *data, Evas_Object *obj, void *event_info);
static void _changed(void *data, Evas_Object *obj, void *event_info);
static void _cancel_clicked(void *data, Evas_Object *obj, void *event_info);

static void _del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->idler) ecore_idler_del(wd->idler);

   free(wd);
}

static void _theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[4096];

   if (!wd) return;

   _elm_theme_object_set(obj, wd->base, "searchbar", "base", elm_widget_style_get(obj));

   if (wd->ef)
     edje_object_part_swallow(wd->base, "search_textfield", wd->ef);
   if (wd->cancel_btn)
     edje_object_part_swallow(wd->base, "button_cancel", wd->cancel_btn);

   snprintf(buf, sizeof(buf), "searchbar/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->ef, buf);

   snprintf(buf, sizeof(buf), "searchbar/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->cancel_btn, buf);

   edje_object_scale_set(wd->cancel_btn, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->base)
      return;
   if (elm_widget_focus_get(obj))
     {
        /*FIXME: Sending clicked signal to focus editfield once open source is merged
         * remove the below call
         */
        elm_object_signal_emit(wd->ef, "clicked", "elm");
        if (wd->cancel_btn_show_mode)
          {
             if (wd->cancel_btn_ani_flag) edje_object_signal_emit(wd->base, "CANCELIN", "PROG");
             else edje_object_signal_emit(wd->base, "CANCELSHOW", "PROG");
          }
     }
   else
     {
        if (wd->cancel_btn_ani_flag) edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
        else edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");
     }
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

static void _clicked(void *data, Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   evas_object_smart_callback_call(data, "clicked", NULL);
}

static Eina_Bool _delay_changed(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_CANCEL;

   evas_object_smart_callback_call(data, "delay-changed", NULL);
   wd->idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void _changed(void *data, Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   if (!wd->idler)
      wd->idler = ecore_idler_add(_delay_changed, data);
}

static void _cancel_clicked(void *data, Evas_Object *obj __UNUSED__,
                            void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   const char* text;
   text = elm_entry_entry_get(elm_editfield_entry_get(wd->ef));
   if (text != NULL && strlen(text) > 0)
     elm_entry_entry_set(elm_editfield_entry_get(wd->ef), NULL);

   evas_object_smart_callback_call(data, "cancel,clicked", NULL);
   elm_object_unfocus(data);
}

static void
_basebg_clicked(void *data, Evas_Object *obj, const char *emission __UNUSED__,
                const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;

   if (!strcmp(source, "base_bg"))
      _clicked(data, obj, NULL);
}

static void
_searchsymbol_clicked(void *data, Evas_Object *obj __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   evas_object_smart_callback_call(data, "searchsymbol,clicked", NULL);
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
   char buf[4096];

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "searchbar");
   elm_widget_type_set(obj, "searchbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
   elm_widget_can_focus_set(obj, 1 );

   wd->base = edje_object_add(e);
   if (wd->base == NULL) return NULL;

   _elm_theme_object_set(obj, wd->base, "searchbar", "base", "default");

   // Add Entry
   wd->ef = elm_editfield_add(parent);
   elm_object_style_set(wd->ef, "searchbar/default");

   edje_object_part_swallow(wd->base, "search_textfield", wd->ef);
   elm_editfield_guide_text_set(wd->ef, E_("Search"));
   elm_editfield_entry_single_line_set(wd->ef, EINA_TRUE);
   elm_editfield_eraser_set(wd->ef, EINA_TRUE);
   evas_object_smart_callback_add(wd->ef, "clicked", _clicked, obj);
   evas_object_smart_callback_add(elm_editfield_entry_get(wd->ef), "changed", _changed, obj);
   edje_object_signal_callback_add(wd->base, "mouse,up,1", "*", _basebg_clicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,click", "", _searchsymbol_clicked, obj);

   elm_widget_sub_object_add(obj, wd->ef);

   // Add Button
   wd->cancel_btn = elm_button_add(parent);
   edje_object_part_swallow(wd->base, "button_cancel", wd->cancel_btn);

   snprintf(buf, sizeof(buf), "searchbar/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->cancel_btn, buf);

   elm_object_text_set(wd->cancel_btn, E_("Cancel"));
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
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   elm_entry_entry_set(elm_editfield_entry_get(wd->ef), entry);
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
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return elm_entry_entry_get(elm_editfield_entry_get(wd->ef));
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
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return elm_editfield_entry_get(wd->ef);
}

/**
 * get the pointer of editfield
 *
 * @param obj The searchbar object
 * @return the editfield object
 *
 * @ingroup Searchbar
 */
EAPI Evas_Object *elm_searchbar_editfield_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->ef) return NULL;

   return wd->ef;
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
   ELM_CHECK_WIDTYPE(obj, widtype);
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
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->cancel_btn_show_mode == visible) return;
   else wd->cancel_btn_show_mode = visible;

   if (!visible)
     {
        if (wd->cancel_btn_ani_flag)
           edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
        else
           edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");
     }
   _sizing_eval(obj);
}

/**
 * clear searchbar status
 *
 * @param obj The searchbar object
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->cancel_btn_show_mode)
     {
        if (wd->cancel_btn_ani_flag)
           edje_object_signal_emit(wd->base, "CANCELOUT", "PROG");
        else
           edje_object_signal_emit(wd->base, "CANCELHIDE", "PROG");
     }
//   elm_entry_entry_set(elm_editfield_entry_get(wd->ef), NULL);
}

/**
 * set the searchbar boundary rect mode(with bg rect) set
 *
 * @param obj The searchbar object
 * @param boundary The present flag of boundary rect or not
 * @return void
 *
 * @ingroup Searchbar
 */
EAPI void elm_searchbar_boundary_rect_set(Evas_Object *obj, Eina_Bool boundary)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->boundary_mode == boundary) return;
   else wd->boundary_mode = boundary;

   if (wd->boundary_mode)
     {
        edje_object_signal_emit(wd->base, "BDSHOW", "PROG");
     }
   else
     {
        edje_object_signal_emit(wd->base, "BDHIDE", "PROG");
     }
   _sizing_eval(obj);
}
