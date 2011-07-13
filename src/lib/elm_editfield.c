#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Editfield Editfield
 * @ingroup Elementary
 *
 * This is a editfield. It can contain a simple label and icon objects.
 *
 * Smart callbacks that you can add are:
 *
 * clicked - This signal is emitted when an editfield is clicked.
 *
 * unfocused - This signal is emitted when an editfield is unfocused.
 *
 */

//#define ERASER_PADDING (10)

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *base;
   Evas_Object *entry;
   Evas_Object *ricon;
   Evas_Object *licon;
   const char *label;
   const char *guide_text;
   Eina_Bool needs_size_calc:1;
   Eina_Bool show_guide_text:1;
   Eina_Bool editing:1;
   Eina_Bool single_line:1;
   Eina_Bool eraser_show:1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _on_focus_hook(void *data, Evas_Object *obj);
static Eina_Bool _empty_entry(Evas_Object *entry);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->label) eina_stringshare_del(wd->label);
   free(wd);
}

static void
_on_focus_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->base)
      return;
   if (!elm_widget_focus_get(obj) && !(elm_widget_disabled_get(obj)) )
     {
        evas_object_smart_callback_call(obj, "unfocused", NULL);
        wd->editing = EINA_FALSE;
        edje_object_signal_emit(wd->base, "elm,state,over,show", "elm");
        edje_object_signal_emit(wd->base, "elm,state,eraser,hidden", "elm");
        if(_empty_entry(wd->entry))
          {
             if(wd->guide_text)
               {
                  edje_object_part_text_set(wd->base, "elm.guidetext", wd->guide_text);
                  edje_object_signal_emit(wd->base, "elm,state,guidetext,visible", "elm");
                  wd->show_guide_text = EINA_TRUE;
               }
          }
     }
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->base)
      return;
   _elm_theme_object_set(obj, wd->base, "editfield", "base", elm_widget_style_get(obj));
   edje_object_part_swallow(wd->base, "elm.swallow.content", wd->entry);
   if(!wd->editing)
      edje_object_signal_emit(wd->base, "elm,state,over,show", "elm");
   if(wd->show_guide_text)
     {
        if(_empty_entry(wd->entry))
          {
             if(wd->guide_text)
               {
                  edje_object_part_text_set(wd->base, "elm.guidetext", wd->guide_text);
                  edje_object_signal_emit(wd->base, "elm,state,guidetext,visible", "elm");
               }
          }
     }
   if(wd->ricon)
      edje_object_part_swallow(wd->base, "right_icon", wd->ricon);
   if(wd->licon)
      edje_object_part_swallow(wd->base, "left_icon", wd->licon);
   _sizing_eval(obj);
}

static void
_changed_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->needs_size_calc)
     {
        _sizing_eval(obj);
        wd->needs_size_calc = EINA_FALSE;
     }
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   edje_object_size_min_calc(wd->base, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_request_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd || !wd->base)
      return;
   if (wd->needs_size_calc)
      return;
   wd->needs_size_calc = EINA_TRUE;
   evas_object_smart_changed(obj);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   double weight_x;
   evas_object_size_hint_weight_get(data, &weight_x, NULL);
   if (weight_x == EVAS_HINT_EXPAND)
      _request_sizing_eval(data);
}

static Eina_Bool
_empty_entry(Evas_Object *entry)
{
   const char* text;
   char *strip_text;
   int len = 0;

   text = elm_entry_entry_get(entry);
   if(!text) return EINA_FALSE;
   strip_text = elm_entry_markup_to_utf8(text);
   if (strip_text) {
        len = strlen(strip_text);
        free(strip_text);
   }
   if(len == 0)
      return EINA_TRUE;
   else
      return EINA_FALSE;
}

static void
_entry_changed_cb(void *data, Evas_Object *obj, void* event_info)
{
   Evas_Object *ef_obj = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(ef_obj);

   if(!wd || !wd->base) return;
   if(wd->single_line)
     {
        if(elm_entry_password_get(wd->entry))
          {
             edje_object_signal_emit(wd->base, "elm,state,password,set", "elm");
             edje_object_part_text_set(wd->base, "elm.content.password", elm_entry_entry_get(wd->entry));
          }
        else
          {
             edje_object_signal_emit(wd->base, "elm,state,password,unset", "elm");
             edje_object_part_text_set(wd->base, "elm.content.text", elm_entry_entry_get(wd->entry));
          }
     }
   if(!_empty_entry(wd->entry))
     {
        if(wd->eraser_show && elm_object_focus_get(obj))
           edje_object_signal_emit(wd->base, "elm,state,eraser,show", "elm");
	if(wd->guide_text)
	  {
	     edje_object_signal_emit(wd->base, "elm,state,guidetext,hidden", "elm");
	     wd->show_guide_text = EINA_FALSE;
	  }
     }
   else
     {
        if(wd->eraser_show)
           edje_object_signal_emit(wd->base, "elm,state,eraser,hidden", "elm");
     }
}

static void
_signal_mouse_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if(!wd || !wd->base) return;

   if(!strcmp(source, "eraser"))
     {
        elm_entry_entry_set(wd->entry, "");
        edje_object_signal_emit(wd->base, "elm,state,eraser,hidden", "elm");
     }
   else if(strcmp(source, "left_icon") && strcmp(source, "right_icon") && strcmp(source, "eraser"))
     {
        edje_object_signal_emit(wd->base, "elm,state,over,hide", "elm");

        elm_object_focus(wd->entry);

        if(wd->editing == EINA_FALSE)
           elm_entry_cursor_end_set(wd->entry);

        if(!(_empty_entry(wd->entry)) && (wd->eraser_show))
           edje_object_signal_emit(wd->base, "elm,state,eraser,show", "elm");

        if(wd->guide_text)
          {
             edje_object_signal_emit(wd->base, "elm,state,guidetext,hidden", "elm");
             wd->show_guide_text = EINA_FALSE;
          }
        evas_object_smart_callback_call(data, "clicked", NULL);
        wd->editing = EINA_TRUE;
     }
}

static void
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord h;
   if (!wd || !wd->base) return;
   evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
}

static void
_signal_emit_hook(Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_emit(wd->base, emission, source);
}


/**
 * Add a new editfield object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   e = evas_object_evas_get(parent);
   if (e == NULL)
      return NULL;
   wd = ELM_NEW(Widget_Data);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "editfield");
   elm_widget_type_set(obj, "editfield");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_changed_hook_set(obj, _changed_hook);
   elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "editfield", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);
   edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "*",
                                   _signal_mouse_clicked, obj);
   edje_object_signal_callback_add(wd->base, "clicked", "*",
                                   _signal_mouse_clicked, obj);

   evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);

   wd->editing = EINA_FALSE;
   wd->single_line = EINA_FALSE;
   wd->eraser_show = EINA_TRUE;

   wd->entry = elm_entry_add(obj);
   elm_object_style_set(wd->entry, "editfield");
   evas_object_size_hint_weight_set(wd->entry, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->entry, 0, EVAS_HINT_FILL);
   evas_object_event_callback_add(wd->entry,
                                  EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);
   edje_object_part_swallow(wd->base, "elm.swallow.content", wd->entry);
   evas_object_smart_callback_add(wd->entry, "changed", _entry_changed_cb, obj);
   elm_widget_sub_object_add(obj, wd->entry);
   evas_object_show(wd->entry);
   _sizing_eval(obj);

   return obj;
}

/**
 * Set the label of editfield
 *
 * @param obj The editfield object
 * @param label The label text
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_label_set(Evas_Object *obj, const char *label)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!wd || !wd->base)
      return;
   if (wd->label)
      eina_stringshare_del(wd->label);
   if (label)
     {
        wd->label = eina_stringshare_add(label);
        edje_object_signal_emit(wd->base, "elm,state,text,visible", "elm");
        edje_object_signal_emit(wd->base, "elm,state,left,icon,hide", "elm");
     }
   else
     {
        wd->label = NULL;
        edje_object_signal_emit(wd->base, "elm,state,text,hidden", "elm");
        edje_object_signal_emit(wd->base, "elm,state,left,icon,show", "elm");
     }
   edje_object_message_signal_process(wd->base);
   edje_object_part_text_set(wd->base, "elm.text", label);
   _sizing_eval(obj);
}

/**
 * Get the label used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI const char*
elm_editfield_label_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!wd || !wd->base)
      return NULL;
   return wd->label;
}

/**
 * Set the guidance text used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_guide_text_set(Evas_Object *obj, const char *text)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!wd || !wd->base)
      return;
   if (wd->guide_text)
      eina_stringshare_del(wd->guide_text);
   if (text)
     {
        wd->guide_text = eina_stringshare_add(text);
        edje_object_part_text_set(wd->base, "elm.guidetext", wd->guide_text);
        wd->show_guide_text = EINA_TRUE;
     }
   else
      wd->guide_text = NULL;
}

/**
 * Get the guidance text used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI const char*
elm_editfield_guide_text_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!wd || !wd->base)
      return NULL;
   return wd->guide_text;
}

/**
 * Get the entry of the editfield object
 *
 * @param obj The editfield object
 * @return entry object
 *
 * @ingroup Editfield
 */

EAPI Evas_Object *
elm_editfield_entry_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!wd)
      return NULL;
   return wd->entry;
}

/**
 * Set the left side icon.
 *
 * @param obj The editfield object
 * @param icon The icon object
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_left_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) ;
   if (!wd || !wd->base || !icon)
      return;
   if ((wd->licon != icon) && (wd->licon))
      elm_widget_sub_object_del(obj, wd->licon);
   if (icon)
     {
        if (!(edje_object_part_swallow(wd->base, "left_icon", icon)))
           return;
        wd->licon = icon;
        elm_widget_sub_object_add(obj, icon);
        evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_signal_emit(wd->base, "elm,state,left,icon,show", "elm");
        edje_object_signal_emit(wd->base, "elm,state,text,hidden", "elm");
        _sizing_eval(obj);
     }
   return;
}

/**
 * Get the left side icon
 *
 * @param obj The editfield object
 * @return icon object
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_left_icon_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!wd || !wd->base || !wd->licon)
      return NULL;
   return wd->licon;
}

/**
 * Set the right side icon.
 *
 * @param obj The editfield object
 * @param icon The icon object
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_right_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) ;
   if (!wd || !wd->base || !icon)
      return;
   if ((wd->ricon != icon) && (wd->ricon))
      elm_widget_sub_object_del(obj, wd->ricon);
   if (icon)
     {
        if ( !(edje_object_part_swallow(wd->base, "right_icon", icon)) )
           return;
        wd->ricon = icon;
        elm_widget_sub_object_add(obj, icon);
        evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_signal_emit(wd->base, "elm,state,right,icon,show", "elm");
        _sizing_eval(obj);
     }
   return;
}

/**
 * Get the right side icon
 *
 * @param obj The editfield object
 * @return icon object
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_right_icon_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!wd || !wd->base || !wd->ricon)
      return NULL;
   return wd->ricon;
}

/**
 * Set entry object style as single-line or multi-line.
 *
 * @param obj The editfield object
 * @param single_line 1 if single-line , 0 if multi-line
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *entry;
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!wd || !wd->base || wd->single_line == single_line)
      return;
   wd->single_line = !!single_line;
   elm_entry_single_line_set(wd->entry, single_line);
   if(single_line)
     {
        elm_entry_scrollable_set(wd->entry, EINA_TRUE);
        elm_entry_single_line_set(wd->entry,EINA_TRUE);
        edje_object_signal_emit(wd->base, "elm,state,text,singleline", "elm");
     }
   else
     {
        elm_entry_scrollable_set(wd->entry, EINA_FALSE);
        elm_entry_single_line_set(wd->entry,EINA_FALSE);
        edje_object_signal_emit(wd->base, "elm,state,text,multiline", "elm");
     }
}

/**
 * Get the current entry object style(single-line or multi-line)
 *
 * @param obj The editfield object
 * @return 1 if single-line , 0 if multi-line
 *
 * @ingroup Editfield
 */
EAPI Eina_Bool
elm_editfield_entry_single_line_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   if (!wd || !wd->base)
      return EINA_FALSE;
   return wd->single_line;
}

/**
 * Set enable user to clean all of text.
 *
 * @param obj The editfield object
 * @param visible If true, the eraser is visible and user can clean all of text by using eraser.
 * If false, the eraser is invisible.
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_eraser_set(Evas_Object *obj, Eina_Bool visible)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!wd || !wd->base)
      return;

   wd->eraser_show = visible;

   if (!visible)
      edje_object_signal_emit(wd->base, "elm,state,eraser,hidden", "elm");

   return;
}

/**
 * Get the current state of erase (visible/invisible)
 *
 * @param obj The editfield object
 * @return 1 if visible, 0 if invisible
 *
 * @ingroup Editfield
 */
EAPI Eina_Bool
elm_editfield_eraser_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   if (!wd || !wd->base)
      return EINA_FALSE;
   return wd->eraser_show;
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
