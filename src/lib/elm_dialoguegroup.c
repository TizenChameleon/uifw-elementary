#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

/**
 * @defgroup DialogueGroup DialogueGroup 
 * @ingroup Elementary
 *
 * Using dialoguegroup, you can make a dialogue group.
 */

typedef struct _Dialogue_Item Dialogue_Item;
struct _Dialogue_Item
{
	Evas_Object *parent;
	Evas_Object *bg_layout;
	Evas_Object *content_layout;
};


typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
	Evas_Object *parent;
	Evas *e;
	Evas_Object *box;
	unsigned int num;

	Eina_List *items;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);

static void _remove_all(Evas_Object *obj);
static void _change_item_bg(Dialogue_Item *item, const char *style);
static Dialogue_Item* _create_item(Evas_Object *obj, Evas_Object *subobj, const char *style);

	
static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;	

	if (!wd) return;

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
	
	if (!wd)
		return;	
	EINA_LIST_FOREACH(wd->items, l, item) 
		_change_item_bg( item, elm_widget_style_get(item->bg_layout) );	

	_sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{

}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Dialogue_Item *item;
	Evas_Coord minw, minh, maxw, maxh;
	
	if (!wd) return;
	evas_object_size_hint_min_get(wd->box, &minw, &minh);
	evas_object_size_hint_max_get(wd->box, &maxw, &maxh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void _remove_all(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;	

	if (!wd) return;

	wd->num = 0;
	
	if (wd->items) {
		EINA_LIST_FREE(wd->items, item) {
			if (item->content_layout){
				evas_object_del(item->content_layout);
				item->content_layout = NULL;
			}
			if (item->bg_layout){
				evas_object_del(item->bg_layout);
				item->bg_layout = NULL;
			}
			free(item);
		}
	}
}

static void _change_item_bg(Dialogue_Item *item, const char *style)
{
	if ( ! item ) return;
	
	elm_layout_theme_set(item->bg_layout, "dialoguegroup", "bg", style);
	elm_object_style_set(item->bg_layout, style);
	elm_layout_content_set(item->bg_layout, "swallow", item->content_layout);
}

static Dialogue_Item* _create_item(Evas_Object *obj, Evas_Object *subobj, const char *style)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;

	if ( ! wd ) return NULL;
	
	item = ELM_NEW(Dialogue_Item);
	item->parent = obj;
	item->content_layout = subobj; 
	
	item->bg_layout = elm_layout_add(wd->parent);
	elm_layout_theme_set(item->bg_layout, "dialoguegroup", "bg", style );
	elm_object_style_set(item->bg_layout, style);
	evas_object_size_hint_weight_set(item->bg_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(item->bg_layout, EVAS_HINT_FILL, 0.0);
	evas_object_show(item->bg_layout);	

	elm_layout_content_set(item->bg_layout, "swallow", item->content_layout);

	return item;
}

static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
EAPI Evas_Object *elm_dialoguegroup_add(Evas_Object *parent)
{
	Evas_Object *obj = NULL;
	Widget_Data *wd = NULL;

	wd = ELM_NEW(Widget_Data);
	wd->e = evas_object_evas_get(parent);
	if (wd->e == NULL) return NULL;
	obj = elm_widget_add(wd->e);
	elm_widget_type_set(obj, "dialoguegroup");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;
	wd->num = 0;
	
	wd->box = elm_box_add(parent);
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
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_append(Evas_Object *obj, Evas_Object *subobj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;
	
	if ( ! wd || ! subobj ) return;
	
	if ( !wd->items ) {
		item = _create_item(obj, subobj, "default");	
		elm_box_pack_end(wd->box, item->bg_layout);
		wd->items = eina_list_append(wd->items, item);	
	}

	else {
	       	if ( wd->num == 1 ) {	
			item = eina_list_data_get(wd->items);
			_change_item_bg(item, "top");		
		}		
		else {
			item = eina_list_data_get( eina_list_last(wd->items) );
			_change_item_bg(item, "middle");		
		}
		item = _create_item(obj, subobj, "bottom");
		elm_box_pack_end(wd->box, item->bg_layout);
		wd->items = eina_list_append(wd->items, item);		
	}

	wd->num++;
	_sizing_eval(obj);
}


/**
 * Prepend an item to the dialogue group.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_prepend(Evas_Object *obj, Evas_Object *subobj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;
	
	if ( ! wd || ! subobj ) return;
	
	if ( !wd->items ) {
		item = _create_item(obj, subobj, "default");	
		elm_box_pack_start(wd->box, item->bg_layout);
		wd->items = eina_list_prepend(wd->items, item);	
	}

	else {
	       	if ( wd->num == 1 ) {	
			item = eina_list_data_get(wd->items);
			_change_item_bg(item, "bottom");
		}		
		else {
			item = eina_list_data_get( wd->items );
			_change_item_bg(item, "middle");		
		}
		item = _create_item(obj, subobj, "top");
		elm_box_pack_start(wd->box, item->bg_layout);
		wd->items = eina_list_prepend(wd->items, item);		
	}

	wd->num++;
	_sizing_eval(obj);
}

/**
 * Insert an item to the dialogue group just after the specified item.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param after specified item existing in the dialogue group
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_insert_after(Evas_Object *obj, Evas_Object *subobj, Evas_Object *after)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *after_item, *item;
	Eina_List *l;
	const char *style;

	if ( !wd || !subobj || !after || !wd->items)
		return ;
	
	EINA_LIST_FOREACH(wd->items, l, after_item) {
		if( after == after_item->content_layout ) {
			style = elm_object_style_get(after_item->bg_layout);
			
			if( !strcmp(style, "default") ) {
				_change_item_bg(after_item, "top");
				item = _create_item(obj, subobj, "bottom");
			}
			
			else if( !strcmp(style, "top") || !strcmp(style, "middle") )		
				item = _create_item(obj, subobj, "middle");
			
			else if( !strcmp(style, "bottom") ) {
				_change_item_bg(after_item, "middle");
				item = _create_item(obj, subobj, "bottom");			
			}

			elm_box_pack_after(wd->box, item->bg_layout, after_item->bg_layout);
			wd->items = eina_list_append_relative(wd->items, item, after_item);
		}		
	}
		
	wd->num++;

	_sizing_eval(obj);
}

/**
 * Insert an item to the dialogue group just before the specified item.
 *
 * @param obj dialoguegroup object 
 * @param subobj item
 * @param before specified item existing in the dialogue group
 *
 * @ingroup DialogueGroup
 */
EAPI void
elm_dialoguegroup_insert_before(Evas_Object *obj, Evas_Object *subobj, Evas_Object *before)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *before_item, *item;
	Eina_List *l;
	const char *style;
	
	if ( !wd || !subobj || !before || !wd->items)
		return ;

	EINA_LIST_FOREACH(wd->items, l, before_item) {
		if( before == before_item->content_layout ) {
			style = elm_object_style_get(before_item->bg_layout);
			
			if( !strcmp(style, "default") ) {
				_change_item_bg(before_item, "bottom");
				item = _create_item(obj, subobj, "top");
			}
				
			else if( !strcmp(style, "top") ) {
				_change_item_bg(before_item, "middle");
				item = _create_item(obj, subobj, "top");			
			}

			else if( !strcmp(style, "middle") || !strcmp(style, "bottom") )		
				item = _create_item(obj, subobj, "middle");

			elm_box_pack_before(wd->box, item->bg_layout, before_item->bg_layout);
			wd->items = eina_list_prepend_relative(wd->items, item, before_item);
		}		
	}
		
	wd->num++;

	_sizing_eval(obj);	
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
elm_dialoguegroup_remove(Evas_Object *obj, Evas_Object *subobj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Dialogue_Item *item;
	Eina_List *l;
	
	if ( !wd || !subobj || !wd->items)
		return ;
	
	EINA_LIST_FOREACH(wd->items, l, item) {
		if (subobj == item->content_layout) {
			if (item->content_layout){
				evas_object_del(item->content_layout);
				item->content_layout = NULL;
			}
			if (item->bg_layout){
				evas_object_del(item->bg_layout);
				item->bg_layout = NULL;
			}
			elm_box_unpack(wd->box, item->bg_layout);			
			wd->items = eina_list_remove(wd->items, item);
		}
	}
		
	wd->num--;
	
	if ( wd->num == 0 ) 
		return;
	
	if ( wd->num == 1 ) {
		item = eina_list_data_get(wd->items);
		_change_item_bg(item, "default");
	}

	else {		
		item = eina_list_data_get(wd->items);
		_change_item_bg(item, "top");
		item = eina_list_data_get( eina_list_last(wd->items) );
		_change_item_bg(item, "bottom");		
	}

	_sizing_eval(obj);	
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
	_remove_all(obj);	
	_sizing_eval(obj);	
}
