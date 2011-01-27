#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

/**
 * @defgroup DialogueGroup DialogueGroup 
 * @ingroup Elementary
 *
 * Using dialoguegroup, you can make a dialogue group.
 */

struct _Dialogue_Item
{
   Evas_Object *obj;
   Evas_Object *bg_layout;
   Evas_Object *content;
   Elm_Dialoguegroup_Item_Style style;
   const char *location;
   Eina_Bool press;
   Eina_Bool disabled;
   //   Eina_Bool line_show;
};


typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
   Evas_Object *box;
   Evas_Object *title_layout;
   const char *title;
   unsigned int num;
   Eina_List *items;
};

static const char*widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void _remove_all(Evas_Object *obj);
static void _set_item_theme(Dialogue_Item *item, const char *location);
static void _change_item_bg(Dialogue_Item *item, const char *location);
static Dialogue_Item* _create_item(Evas_Object *obj, Evas_Object *subobj, Elm_Dialoguegroup_Item_Style style, const char *location);


static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return;
   if (wd->title) eina_stringshare_del(wd->title);
   
   _remove_all(obj);
   
   if (wd->box){
      evas_object_del(wd->box);
      wd->box = NULL;
   }
   
   free(wd);
}


static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Dialogue_Item *item; 
   
   if (!wd) return;     
   if (wd->title) {
      elm_layout_theme_set(wd->title_layout, "dialoguegroup", "title", elm_widget_style_get(obj));
      edje_object_part_text_set(elm_layout_edje_get(wd->title_layout), "text", wd->title);
   }
   EINA_LIST_FOREACH(wd->items, l, item) 
     _change_item_bg( item, item->location );   
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw, minh, maxw, maxh;
   
   if (!wd) return;
   evas_object_size_hint_min_get(wd->box, &minw, &minh);
   evas_object_size_hint_max_get(wd->box, &maxw, &maxh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void 
_remove_all(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *item; 
   
   if (!wd) return;
   
   wd->num = 0;
   
   if (wd->items) {
      EINA_LIST_FREE(wd->items, item) {
         if (item->content){
            evas_object_del(item->content);
            item->content = NULL;
         }
         if (item->bg_layout){
            evas_object_del(item->bg_layout);
            item->bg_layout = NULL;
         }
         if (item->location)
           eina_stringshare_del(item->location);
         free(item);
      }
   }
}

static void 
_set_item_theme(Dialogue_Item *item, const char *location)
{
   if (!item) return;
   char buf[30];
   
   if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_DEFAULT) 
     snprintf(buf, sizeof(buf), "bg_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDITFIELD) 
     snprintf(buf, sizeof(buf), "editfield_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDITFIELD_WITH_TITLE) 
     snprintf(buf, sizeof(buf), "editfield_with_title_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDIT_TITLE) 
     snprintf(buf, sizeof(buf), "edit_title_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_HIDDEN) 
     snprintf(buf, sizeof(buf), "hidden_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_DATAVIEW) 
     snprintf(buf, sizeof(buf), "dataview_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_NO_BG) 
     snprintf(buf, sizeof(buf), "no_bg_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB) 
     snprintf(buf, sizeof(buf), "sub_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDIT)
     snprintf(buf, sizeof(buf), "bg_edit_%s", location);
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDIT_MERGE)
     snprintf(buf, sizeof(buf), "bg_edit_merge_%s", location);
   elm_layout_theme_set(item->bg_layout, "dialoguegroup", buf, elm_widget_style_get(item->obj));
}

/*
   static void _set_line_show(Dialogue_Item *item, Dialogue_Item *after)
   {
   if(!item || !after) return;

   if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_DEFAULT) {
   if (after->style == ELM_DIALOGUEGROUP_ITEM_STYLE_DEFAULT) {
   item->line_show = EINA_TRUE;
   edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,line,show", "elm"); 
   }
   else if (after->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDITFIELD) {
   item->line_show = EINA_FALSE;
   edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,line,hide", "elm"); 
   }    
   }
   else if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_EDITFIELD) 
   item->line_show = EINA_TRUE; 
   }
*/

static void 
_change_item_bg(Dialogue_Item *item, const char *location)
{
   if (!item) return;
   
   eina_stringshare_replace(&item->location, location);
   _set_item_theme(item, location);
   elm_layout_content_set(item->bg_layout, "swallow", item->content);
   if ((item->press == EINA_TRUE) && (item->disabled == EINA_FALSE))
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,on", "elm");
   else
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,off", "elm");   
   if (item->disabled == EINA_TRUE)
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,enabled", "elm");
   
   if (item->style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB)
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "flip_item", "");

   /*   if(item->line_show == EINA_FALSE)
    edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,line,hide", "elm");*/
   
}

static Dialogue_Item* 
_create_item(Evas_Object *obj, Evas_Object *subobj, Elm_Dialoguegroup_Item_Style style, const char *location)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *item;
   
   if (!wd) return NULL;
   
   item = ELM_NEW(Dialogue_Item);
   item->obj = obj;
   item->content = subobj;
   item->press = EINA_TRUE;
   item->disabled = EINA_FALSE;
   item->style = style;
   //   item->line_show = EINA_TRUE;
   eina_stringshare_replace(&item->location, location);
   
   item->bg_layout = elm_layout_add(wd->box);
   _set_item_theme(item, location);
   evas_object_size_hint_weight_set(item->bg_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(item->bg_layout, EVAS_HINT_FILL, 0.0);
   evas_object_show(item->bg_layout);   
   
   elm_layout_content_set(item->bg_layout, "swallow", item->content);
   
   return item;
}

static void 
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(data);
}

/**
 * Add a new dialoguegroup to the parent.
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup DialogueGroup
 */
EAPI Evas_Object *
elm_dialoguegroup_add(Evas_Object *parent)
{
   Evas_Object *obj = NULL;
   Widget_Data *wd = NULL;
   Evas *e = NULL;
   
   e = evas_object_evas_get(parent);
   if (e == NULL) return NULL;
   wd = ELM_NEW(Widget_Data);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "dialoguegroup");
   elm_widget_type_set(obj, "dialoguegroup");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);
   
   wd->num = 0;
   
   wd->box = elm_box_add(obj);
   evas_object_event_callback_add(wd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
   evas_object_show(wd->box);
   elm_widget_resize_object_set(obj, wd->box);
   
   _sizing_eval(obj);
   return obj;
}

/**
 * Append an item to the dialogue group.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param style sytle of the item
 * @return Dialogue_Item pointer, just made by this function
 * 
 * @ingroup DialogueGroup
 */
EAPI Dialogue_Item *
elm_dialoguegroup_append(Evas_Object *obj, Evas_Object *subobj, Elm_Dialoguegroup_Item_Style style)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *item = NULL, *new_item = NULL;        
   
   if (!wd || !subobj) return NULL;
   
   if (!wd->items) 
     new_item = _create_item(obj, subobj, style, "default");
   else {
      if (wd->num == 1) {       
         item = eina_list_data_get(wd->items);
         _change_item_bg(item, "top");          
      }         
      else {
         item = eina_list_data_get( eina_list_last(wd->items) );
         _change_item_bg(item, "middle");               
      }
      new_item = _create_item(obj, subobj, style, "bottom");
      //                _set_line_show(item, new_item);
   }
   elm_box_pack_end(wd->box, new_item->bg_layout);
   if (style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB)
     edje_object_signal_emit(elm_layout_edje_get(new_item->bg_layout), "flip_item", "");
   wd->items = eina_list_append(wd->items, new_item);                   
   wd->num++;
   _sizing_eval(obj);
   return new_item;
}


/**
 * Prepend an item to the dialogue group.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param style sytle of the item
 * @return Dialogue_Item pointer, just made by this function
 *
 * @ingroup DialogueGroup
 */
EAPI Dialogue_Item *
elm_dialoguegroup_prepend(Evas_Object *obj, Evas_Object *subobj, Elm_Dialoguegroup_Item_Style style)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *item = NULL, *new_item = NULL;
   
   if (!wd || !subobj) return NULL;
   
   if (!wd->items)
     new_item = _create_item(obj, subobj, style, "default");    
   else {
      if (wd->num == 1) {       
         item = eina_list_data_get(wd->items);
         _change_item_bg(item, "bottom");
      }         
      else {
         item = eina_list_data_get(wd->items);
         _change_item_bg(item, "middle");               
      }
      new_item = _create_item(obj, subobj, style, "top");
      //                _set_line_show(new_item, item);
   }
   if (wd->title_layout)
     elm_box_pack_after(wd->box, new_item->bg_layout, wd->title_layout);        
   else                 
     elm_box_pack_start(wd->box, new_item->bg_layout);          
   if (style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB) 
     edje_object_signal_emit(elm_layout_edje_get(new_item->bg_layout), "flip_item", "");
   wd->items = eina_list_prepend(wd->items, new_item);          
   wd->num++;
   _sizing_eval(obj);
   return new_item;
}

/**
 * Insert an item to the dialogue group just after the specified item.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param after specified item existing in the dialogue group
 * @param style sytle of the item
 * @return Dialogue_Item pointer, just made by this function
 *
 * @ingroup DialogueGroup
 */
EAPI Dialogue_Item * 
elm_dialoguegroup_insert_after(Evas_Object *obj, Evas_Object *subobj, Dialogue_Item *after, Elm_Dialoguegroup_Item_Style style)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *after_item = NULL, *item = NULL;
   Eina_List *l;
   
   if (!wd || !subobj || !after || !wd->items) return NULL;
   
   EINA_LIST_FOREACH(wd->items, l, after_item) {
      if (after == after_item) {
         if ( !strcmp(after_item->location, "default") ) {
            _change_item_bg(after_item, "top");
            item = _create_item(obj, subobj, style, "bottom");
         }                      
         else if ( !strcmp(after_item->location, "top") || !strcmp(after_item->location, "middle") )    
           item = _create_item(obj, subobj, style, "middle");           
         else if ( !strcmp(after_item->location, "bottom") ) {
            _change_item_bg(after_item, "middle");
            item = _create_item(obj, subobj, style, "bottom");          
         }
         if (!item)
            return NULL;
         elm_box_pack_after(wd->box, item->bg_layout, after_item->bg_layout);
         if (style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB) 
           edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "flip_item", "");
         wd->items = eina_list_append_relative(wd->items, item, after_item);
         //     _set_line_show(after, item);
      }         
   }
   
   wd->num++;
   _sizing_eval(obj);
   return item;
}

/**
 * Insert an item to the dialogue group just before the specified item.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param before specified item existing in the dialogue group
 * @param style sytle of the item
 * @return Dialogue_Item pointer, just made by this function
 *
 * @ingroup DialogueGroup
 */
EAPI Dialogue_Item *
elm_dialoguegroup_insert_before(Evas_Object *obj, Evas_Object *subobj, Dialogue_Item *before, Elm_Dialoguegroup_Item_Style style)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Dialogue_Item *before_item = NULL, *item = NULL;
   Eina_List *l;
   Eina_List *prev;
   
   if (!wd || !subobj || !before || !wd->items) return NULL;
   
   EINA_LIST_FOREACH(wd->items, l, before_item) {
      if (before == before_item) {
         if ( !strcmp(before_item->location, "default") ) {
            _change_item_bg(before_item, "bottom");
            item = _create_item(obj, subobj, style, "top");
         }
         
         else if ( !strcmp(before_item->location, "top") ) {
            _change_item_bg(before_item, "middle");
            item = _create_item(obj, subobj, style, "top");                     
         }
         
         else if ( !strcmp(before_item->location, "middle") || !strcmp(before_item->location, "bottom") ) {
            item = _create_item(obj, subobj, style, "middle");
            prev = eina_list_prev(l);
            //  _set_line_show(prev->data, item);
         }
         if (!item)
            return NULL;
         elm_box_pack_before(wd->box, item->bg_layout, before_item->bg_layout);
         if (style == ELM_DIALOGUEGROUP_ITEM_STYLE_SUB) 
           edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "flip_item", "");
         wd->items = eina_list_prepend_relative(wd->items, item, before_item);
      }         
   }
   
   wd->num++;
   _sizing_eval(obj);   
   return item;
}

/**
 * Remove an item from the dialogue group.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_remove(Dialogue_Item *item)
{
   if (!item) return;
   ELM_CHECK_WIDTYPE(item->obj, widtype) ;
   Dialogue_Item *current_item;
   Widget_Data *wd = elm_widget_data_get(item->obj);
   Eina_List *l;
   
   if (!wd || !wd->items || !item) return ;
   
   EINA_LIST_FOREACH(wd->items, l, current_item) {
      if (current_item == item) {
         if (current_item->content){
            evas_object_del(current_item->content);
            current_item->content = NULL;
         }
         if (current_item->bg_layout){
            evas_object_del(current_item->bg_layout);
            current_item->bg_layout = NULL;
         }
         elm_box_unpack(wd->box, current_item->bg_layout);                      
         wd->items = eina_list_remove(wd->items, current_item);
      }
   }
   
   wd->num--;
   
   if (wd->num == 0) return;
   
   if (wd->num == 1) {
      current_item = eina_list_data_get(wd->items);
      _change_item_bg(current_item, "default");
   }
   
   else {               
      current_item = eina_list_data_get(wd->items);
      _change_item_bg(current_item, "top");
      current_item = eina_list_data_get( eina_list_last(wd->items) );
      _change_item_bg(current_item, "bottom");          
   }
   
   _sizing_eval(item->obj);  
}

/**
 * Remove all items from the dialogue group.
 *
 * @param obj dialoguegroup object 
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_remove_all(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   _remove_all(obj);    
   _sizing_eval(obj);   
}


/**
 * Set the title text of the  dialogue group.
 *
 * @param obj dialoguegroup object 
 * @param title title text, if NULL title space will be disappeared 
 * 
 * @ingroup DialogueGroup
 */
EAPI void 
elm_dialoguegroup_title_set(Evas_Object *obj, const char *title)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return ;
   eina_stringshare_replace(&wd->title, title);
   if (!title) {
      wd->title = NULL;         
      elm_box_unpack(wd->box, wd->title_layout);                
   }    
   if (!wd->title_layout) {
      wd->title_layout = elm_layout_add(wd->box);
      elm_layout_theme_set(wd->title_layout, "dialoguegroup", "title", elm_widget_style_get(obj));
      evas_object_size_hint_weight_set(wd->title_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      evas_object_size_hint_align_set(wd->title_layout, EVAS_HINT_FILL, 0.0);
      evas_object_show(wd->title_layout);       
      edje_object_part_text_set(elm_layout_edje_get(wd->title_layout), "text", title);
      elm_box_pack_start(wd->box, wd->title_layout);
   }
   edje_object_part_text_set(elm_layout_edje_get(wd->title_layout), "text", title);
}

/**
 * Get the title text of the dialogue group
 *
 * @param obj The dialoguegroup object
 * @return The text title string in UTF-8
 *
 * @ingroup DialogueGroup
 */
EAPI const char *
elm_dialoguegroup_title_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->title;
}

/**
 * Set whether the press effect will be shown or not
 *
 * @param obj The dialoguegroup object
 * @param item Dialogue_Item pointer
 * @param press If set as 1, press effect will be shown 
 *
 * @ingroup DialogueGroup 
 */
EAPI void 
elm_dialoguegroup_press_effect_set(Dialogue_Item *item, Eina_Bool press)
{
   if (!item) return;
   ELM_CHECK_WIDTYPE(item->obj, widtype) ;
   
   item->press = press;
   if ((press == EINA_TRUE) && (item->disabled == EINA_FALSE))
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,on", "elm");
   else
     edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,off", "elm");       
}

/**
 * Get the press effect state
 *
 * @param obj The dialoguegroup object
 * @param item Dialogue_Item pointer
 * @return 1 if press effect on, 0 if press effect off 
 *
 * @ingroup DialogueGroup 
 */
EAPI Eina_Bool
elm_dialoguegroup_press_effect_get(Dialogue_Item *item)
{
   if (!item) return EINA_FALSE;
   ELM_CHECK_WIDTYPE(item->obj, widtype) EINA_FALSE;
   
   return item->press;
}

/**
 * Get the conetent object from the specified dialogue item
 *
 * @param obj The dialoguegroup object
 * @param item Dialogue_Item pointer
 * @return content object 
 *
 * @ingroup DialogueGroup 
 */
EAPI Evas_Object *
elm_dialoguegroup_item_content_get(Dialogue_Item *item)
{
   if (!item) return NULL;
   ELM_CHECK_WIDTYPE(item->obj, widtype) EINA_FALSE;
   
   return item->content;
}

/**
 * Set the style of the item.
 *
 * @param item dialoguegroup item
 * @param style sytle of the item
 * 
 * @ingroup DialogueGroup
 */
EAPI void 
elm_dialoguegroup_item_style_set(Dialogue_Item *item, Elm_Dialoguegroup_Item_Style style)
{
   if (!item) return;
   ELM_CHECK_WIDTYPE(item->obj, widtype);
   Widget_Data *wd = elm_widget_data_get(item->obj);
   
   item->style = style;
   _change_item_bg(item, item->location);
   
   if (!wd) return ;
}

/**
 * Get the style of the item.
 *
 * @param item dialoguegroup item
 * @return dialoguegroup item style
 * 
 * @ingroup DialogueGroup
 */

EAPI Elm_Dialoguegroup_Item_Style
elm_dialoguegroup_item_style_get(Dialogue_Item *item)
{
   if (!item) return ELM_DIALOGUEGROUP_ITEM_STYLE_LAST;
   ELM_CHECK_WIDTYPE(item->obj, widtype) ELM_DIALOGUEGROUP_ITEM_STYLE_LAST;
   Widget_Data *wd = elm_widget_data_get(item->obj);
   
   if (!wd) return ELM_DIALOGUEGROUP_ITEM_STYLE_LAST;
   
   return item->style;
}

/**
 * Set item state as disable or not.
 *
 * @param item dialoguegroup item.
 * @param disabled if EINA_TRUE disabled, else abled. 
 * 
 * @ingroup DialogueGroup
 */
EAPI void 
elm_dialoguegroup_item_disabled_set(Dialogue_Item *item, Eina_Bool disabled)
{
   if (!item) return;
   ELM_CHECK_WIDTYPE(item->obj, widtype);
   
   item->disabled = disabled;
   
   if (disabled == EINA_TRUE)
     {
        edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,disabled", "elm");
        edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,off", "elm");    
     }
   else
     {
        edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,enabled", "elm");
        if (item->press == EINA_TRUE)
          edje_object_signal_emit(elm_layout_edje_get(item->bg_layout), "elm,state,press,on", "elm");
     }
}

/**
 * Get item state whether disabled or not.
 *
 * @param item dialoguegroup item.
 * @return if EINA_TRUE, then disabled else abled.
 * 
 * @ingroup DialogueGroup
 */

EAPI Eina_Bool
elm_dialoguegroup_item_disabled_get(Dialogue_Item *item)
{
   if (!item) return EINA_FALSE;
   ELM_CHECK_WIDTYPE(item->obj, widtype) EINA_FALSE;
   
   return item->disabled;
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/

