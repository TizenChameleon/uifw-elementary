#include <Elementary.h>
#include "elm_priv.h"
#include <string.h>

/**
 * @defgroup Autocompleteview Autocompleteview
 * @ingroup Elementary
 *
 * This widget show's the completed strings in a dropdown list 
 * based on the initial few characters entered by the user.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *editfield;
   Evas_Object *hover;
   Evas_Object *layout;
   Evas_Object *list;
   Evas_Object *entry;
   Eina_List *data_list;
   Eina_Bool text_set : 1;
   elmautocompleteview_matchfunction func;
   void *data;
   int threshold;
};

static void _editfield_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_smart_callback_call(data, "clicked", NULL);
}

static void _entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   const char *text = NULL;
   int textlen = 0;
   char *real = NULL,*res = NULL;
   Eina_List *l;
   Eina_Bool textfound = EINA_FALSE;
   if(!wd) return;
   if (elm_widget_disabled_get(data)) return;
   if(wd->text_set)
     {
        evas_object_hide(wd->hover);
        wd->text_set = EINA_FALSE;
        return;
     }
   text = elm_entry_entry_get(obj);
   if(text == NULL)
      return;	
   textlen  = strlen(text);
   if(textlen <wd->threshold)
     {
        evas_object_hide(wd->hover);
        return;
     }		
   evas_object_hide(wd->hover);
   if(wd->func)
     {
        textfound = wd->func(data,text,wd->list,wd->data);
     }
   else if(wd->data_list) 
     {
        elm_list_clear(wd->list);
        EINA_LIST_FOREACH(wd->data_list, l, real) 
          {
             res  = strcasestr(real,text);
             if(res)
               {
                  elm_list_item_append(wd->list, real, NULL, NULL, 
                                       NULL, NULL);
                  textfound=EINA_TRUE;
               }
          }
     }
   else
      return;
   if(textfound)
     {
        elm_list_go(wd->list);		
        evas_object_show(wd->hover);
     }

}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(wd->hover)
      evas_object_del(wd->hover);
}


static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   free(wd);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(data);
}

static void
_hover_clicked(void *data, Evas_Object *obj, void *event_info)
{
   printf("\n\ncurrently nothin to be done\n");
}

static void _list_click( void *data, Evas_Object *obj, void *event_info )
{
   Elm_List_Item *it = (Elm_List_Item *) elm_list_selected_item_get(obj);
   Widget_Data *wd = elm_widget_data_get(data);
   if((it==NULL)||(wd==NULL))
      return;
   const char *text = elm_list_item_label_get(it);
   evas_object_smart_callback_call((Evas_Object *)data, "selected", (void *)text);
   if(wd->data_list)
     {
        if(text!=NULL)
          {
             elm_entry_entry_set(wd->entry, text);
             elm_entry_cursor_end_set(wd->entry);
             wd->text_set =  EINA_TRUE;
          }
     }
}

static int
_eina_cmp_str(const char *a, const char *b)
{
   return strcasecmp(a,b);
}


/**
 * Add a new Autocompleteview object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Autocompleteview
 */
EAPI Evas_Object *
elm_autocompleteview_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "autocompleteview");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);

   wd->editfield = elm_editfield_add(parent);
   elm_widget_resize_object_set(obj, wd->editfield);
   elm_editfield_entry_single_line_set(wd->editfield, EINA_TRUE);
   evas_object_size_hint_weight_set(wd->editfield, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->editfield, 0, EVAS_HINT_FILL);
   evas_object_event_callback_add(wd->editfield,
                                  EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);
   wd->threshold = 1;
   wd->entry = elm_editfield_entry_get(wd->editfield);

   evas_object_smart_callback_add(wd->editfield, "clicked", _editfield_clicked_cb, obj);
   evas_object_smart_callback_add(wd->entry, "changed", _entry_changed_cb, obj);

   wd->hover = elm_hover_add(obj);
   elm_hover_parent_set(wd->hover, parent);
   elm_hover_target_set(wd->hover, wd->editfield);

   wd->layout = elm_layout_add(wd->hover);
   elm_layout_theme_set(wd->layout,"autocompleteview","base","default");
   wd->list = elm_list_add(wd->layout);
   evas_object_size_hint_weight_set(wd->list, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(wd->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_list_horizontal_mode_set(wd->list, ELM_LIST_COMPRESS);
   elm_object_style_set(wd->list,"autocompleteview");
   elm_list_go(wd->list);
   evas_object_smart_callback_add(wd->list, "selected", _list_click, obj);
   elm_layout_content_set( wd->layout, "elm.swallow.content", wd->list );
   elm_hover_content_set(wd->hover,
                         elm_hover_best_content_location_get(wd->hover,
                                                             ELM_HOVER_AXIS_VERTICAL),
                         wd->layout);
   evas_object_smart_callback_add(wd->hover, "clicked", _hover_clicked, obj);

   _sizing_eval(obj);
   return obj;
}


/**
 * This Get's the entry object of Autocompleteview object.
 *
 * @param obj The Autocompleteview object.
 * @return the entry object.
 *
 * @ingroup Autocompleteview
 */
EAPI Evas_Object *
elm_autocompleteview_entry_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   return wd->entry;
}

/**
 * This Get's the editfield object of the Autocompleteview object.
 *
 * @param obj The Autocompleteview object.
 * @return The Editfield object.
 *
 * @ingroup Autocompleteview
 */
EAPI Evas_Object *
elm_autocompleteview_editfield_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   return wd->editfield;
}


/**
 * This Specifies the minimum number of characters the user has to type in the editfield 
 * before the drop down list is shown.When threshold is less than or equals 0, a threshold of 1 is applied by default.
 *
 * @param obj The Autocompleteview object
 * @param threshold the number of characters to type before the drop down is shown
 *
 * @ingroup Autocompleteview
 */
EAPI void
elm_autocompleteview_threshold_set(Evas_Object *obj, int threshold)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   if(threshold <=0)
     {
        wd->threshold = 1;
     }
   else
     {
        wd->threshold = threshold;
     }
}


/**
 * This Returns the number of characters the user must type before the drop down list is shown.
 *
 * @param obj The Autocompleteview object.
 * @return The threshold value.
 *
 * @ingroup Autocompleteview
 */
EAPI int
elm_autocompleteview_threshold_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return -1;

   return wd->threshold;
}


/**
 * This Specifies the list of strings which has to be searched to get the list of completion strings.
 *
 * @param obj The Autocompleteview object
 * @param data_list the list of static strings, which has to be searched to get the completion strings.
 *
 * @ingroup Autocompleteview
 */
EAPI void
elm_autocompleteview_data_set(Evas_Object *obj, Eina_List *data_list)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   wd->data_list = data_list;

   /*storing sorted list*/
   wd->data_list = eina_list_sort(wd->data_list, eina_list_count(wd->data_list), EINA_COMPARE_CB(_eina_cmp_str));
}


/**
 * This Registers the callback function that would be called whenever text is entered in to the entry.
 *
 * @param obj The Autocompleteview object
 * @param elmautocompleteview_matchfunction completion function which list's the completion strings.
 * @param data userdata that would be passed whenever the callback function is called.
 *
 * @ingroup Autocompleteview
 */
EAPI void
elm_autocompleteview_match_func_set(Evas_Object *obj, elmautocompleteview_matchfunction func,void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   wd->func = func;
   wd->data = data;
}

/**
 * This updates the text in to autocomplete view.
 *
 * @param obj The Autocompleteview object
 * @param text the text to be updated in to the entry of autocompleteview.
 *
 * @ingroup Autocompleteview
 */
EAPI void
elm_autocompleteview_text_update(Evas_Object *obj, char *text)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   if(text!=NULL)
     {
        elm_entry_entry_set(wd->entry, text);
        elm_entry_cursor_end_set(wd->entry);
        wd->text_set =  EINA_TRUE;
     }
}
