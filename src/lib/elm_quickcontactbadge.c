/*
 * SLP
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */

/**
 *
 * @addtogroup QuickContactBadge
 *
 * This is an quickcontactbadge.
 */
#include <Elementary.h>
#include <Ecore.h>
#include <Edje.h>
#include "elm_priv.h"

#define QUICKCONTACTBADGE_EXPAND_COUNT      4
#define QUICKCONTACTBADGE_ICON_LEFT_MARGIN  10
#define QUICKCONTACTBADGE_BG_TOP_MARGIN     18
#define QUICKCONTATBADGE_ICON_TOP_MARGIN    31
#define WORLD_PARENT_W (parent_x + parent_w)

typedef struct _Widget_Data Widget_Data;
typedef struct _Geometry_Rect Geometry_Rect;
typedef enum { Bottom_Arrow, Right_Arrow, Left_Arrow, Top_Arrow } Arrow_Direction;

struct _Geometry_Rect
{
	Evas_Coord x, y, w, h;
};

struct _Quickcontactbadge_Item
{
	Evas_Object* obj;
	Evas_Object* quickcontactbadge;
	Evas_Object* content;
	Eina_Bool disabled;
	Eina_Bool separator;
	void (*func) (void* data, Evas_Object* obj, void* event_info);
	const void* data;
};

struct _Widget_Data
{
	Evas_Object* parent;
	Evas_Object* location;
	Evas_Object* hover;
	Evas_Object* box;
	Evas_Object* arrow;
	Evas_Object* layout;	
	Eina_List* items;
	Evas_Coord x, y;
	Eina_Bool horizontal;
};

static void _sizing_eval( Evas_Object *obj );
static void _del_pre_hook( Evas_Object* obj );
static void _del_hook( Evas_Object* obj );
static void _theme_hook( Evas_Object* obj );
static void _hover_clicked_cb( void* data, Evas_Object* obj, void* event_info );
static void _parent_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _quickcontactbadge_show( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _quickcontactbadge_hide( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _quickcontactbadge_move( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _item_obj_create( Elm_Quickcontactbadge_Item* item );
static void _item_sizing_eval( Elm_Quickcontactbadge_Item* item );
static void _quickcontactbadge_item_select(
		void* data,
		Evas_Object* obj,
		const char* emission,
		const char* source );
static void _separator_obj_add( Evas_Object* quickcontactbadge );
static void _separator_obj_del( Widget_Data* wd, Elm_Quickcontactbadge_Item* remove_item );
static void _changed_size_hints( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _arrow_obj_add( Evas_Object* quickcontactbadge );
static void _scale_shrinked_set( Elm_Quickcontactbadge_Item* item );
static void _item_scale_shrinked_set( Widget_Data* wd, Elm_Quickcontactbadge_Item* add_item );
static void _item_scale_normal_set( Widget_Data* wd );
static void _get_obj_size( Evas_Object* quickcontactbadge, double scale );

static Evas_Coord _quickcontactbadge_arrow_size = 1;
static Evas_Coord _quickcontactbadge_arrow_max_size = 1;
static Evas_Coord _quickcontactbadge_width_max = 1;
static Evas_Coord _quickcontactbadge_height_max = 1;
static Evas_Coord _quickcontactbadge_touch_range = 1;
static Evas_Coord _quickcontactbadge_indicator_height = 1;
static float _size_rate = 1;
static int _item_cnt = 0;

/**
 * Get the icon object for the given item.
 *
 * @param item 	Quickcontactbadge item
 * @return 		Icon object or NULL if the item does not have icon
 *
 * @ingroup Quickcontactbadge
 */
EAPI Evas_Object* elm_quickcontactbadge_item_icon_get( Elm_Quickcontactbadge_Item* item )
{
	if( item == NULL ) {
		return NULL;
	}

	return item->content;
}

/**
 * Add a new Quickcontactbadge object to the parent.
 *
 * @param parent    Parent object
 * @return 		New object or NULL if it cannot be created
 * 
 * @ingroup Quickcontactbadge 
 */
EAPI Evas_Object* elm_quickcontactbadge_add( Evas_Object* parent )
{
	Evas_Object* obj;
	Widget_Data* wd;
	Evas* evas;

	evas = evas_object_evas_get( parent );


	if( evas == NULL ) {
		return NULL;
	}

	obj = (Evas_Object*) elm_widget_add( evas );

	if( obj == NULL ) {
		return NULL;
	}

	elm_widget_type_set( obj, "ctx-popup" );
	elm_widget_sub_object_add( parent, obj );

	wd = calloc( 1, sizeof( Widget_Data ) );

	if( wd == NULL ) {
		return NULL;
	}

	elm_widget_data_set( obj, wd );
	elm_widget_del_pre_hook_set( obj, _del_pre_hook );
	elm_widget_del_hook_set( obj, _del_hook );
	elm_widget_theme_hook_set( obj, _theme_hook );

	wd->location = elm_icon_add( obj );
	wd->parent = parent;

    wd->layout = elm_layout_add( obj );	
    elm_layout_theme_set(wd->layout, "quickcontactbadge", "background", "default");

	//Box
	wd->box = elm_box_add( obj );
	evas_object_size_hint_weight_set( wd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_align_set( wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL );

	evas_object_show( wd->box );

	//Hoversel
	wd->hover = elm_hover_add( obj );
	elm_hover_parent_set( wd->hover, parent );
	elm_hover_target_set( wd->hover, wd->location );
	elm_object_style_set( wd->hover, "quickcontactbadge");
	evas_object_size_hint_weight_set( wd->hover, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_align_set( wd->hover, EVAS_HINT_FILL, EVAS_HINT_FILL );

	evas_object_smart_callback_add( wd->hover, "clicked", _hover_clicked_cb, obj );

	elm_hover_content_set(
		wd->hover,
		elm_hover_best_content_location_get( wd->hover, ELM_HOVER_AXIS_VERTICAL ),
		wd->box );

	evas_object_event_callback_add( parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_SHOW, _quickcontactbadge_show, wd );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_HIDE, _quickcontactbadge_hide, wd );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_MOVE, _quickcontactbadge_move, wd );

	_arrow_obj_add( obj );
	_get_obj_size( obj, edje_scale_get() );
	_sizing_eval( obj );

	return obj;
}

/**
 * Clear all items in given Quickcontactbadge object.
 *
 * @param obj 		Quickcontactbadge object
 *
 * @ingroup Quickcontactbadge
 */
EAPI void elm_quickcontactbadge_clear( Evas_Object* obj )
{
	Eina_List* elist;
	Elm_Quickcontactbadge_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	EINA_LIST_FOREACH( wd->items, elist, item ) {

		if( item->content ) {
			evas_object_del( item->content );
		}

		evas_object_del( item->obj );

		wd->items = eina_list_remove( wd->items, item );

		free( item );

	}
	evas_object_hide( wd->arrow );
	evas_object_hide( wd->hover );
	evas_object_hide( wd->layout );	
}

/**
 * Change the mode to horizontal or vertical.
 *
 * @param obj   	Quickcontactbadge object
 * @param horizontal 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode
 *
 * @ingroup Quickcontactbadge
 */
EAPI void elm_quickcontactbadge_horizontal_set( Evas_Object* obj, Eina_Bool horizontal )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	if( wd->horizontal == horizontal ) {
		return ;
	}

	wd->horizontal = horizontal;

	if( horizontal == EINA_FALSE ) {
		elm_box_horizontal_set( wd->box, EINA_FALSE );

	}else {
		elm_box_horizontal_set( wd->box, EINA_TRUE );
	}

}


/**
 * reset the icon on the given item. This function is only for icon item.
 *
 * @param obj 	 	Quickcontactbadge item
 * @param icon		Icon object to be set
 *
 * @ingroup Quickcontactbadge
 */
EAPI void elm_quickcontactbadge_item_icon_set( Elm_Quickcontactbadge_Item* item, Evas_Object* icon )
{
	Widget_Data* wd;

	if( item == NULL || icon == NULL ) {
		return ;
	}

	wd = (Widget_Data*) elm_widget_data_get( item->quickcontactbadge );

	if( wd == NULL ) {
		return;
	}

	if( item->content == icon ) {
		return ;
	}

	if( item->content ) {
		elm_widget_sub_object_del( item->quickcontactbadge, item->content );
	}


	item->content = icon;
	elm_icon_scale_set( icon, EINA_TRUE, EINA_TRUE );
	elm_widget_sub_object_add( item->quickcontactbadge, icon );
	edje_object_part_swallow( item->obj, "elm.swallow.content", item->content );
	edje_object_signal_emit( item->obj, "elm,state,enable_icon", "elm" );

	evas_object_event_callback_add( icon,
			                EVAS_CALLBACK_CHANGED_SIZE_HINTS,
					_changed_size_hints,
					item->quickcontactbadge );

	edje_object_message_signal_process( item->obj );

	_sizing_eval( item->quickcontactbadge );

}

/**
 * Add a new item as an icon in given quickcontactbadge object.
 *
 * @param obj 	 	Quickcontactbadge object
 * @param icon		Icon to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data          User data for callback function
 * @return 		Added quickcontactbadge item
 * 
 * @ingroup Quickcontactbadge
 */
EAPI Elm_Quickcontactbadge_Item* elm_quickcontactbadge_icon_add(
		Evas_Object* obj,
		Evas_Object* icon,
		void (*func ) (void* data, Evas_Object* obj, void* event_info ),
		const void* data
		)
{
	Elm_Quickcontactbadge_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
    _item_cnt++;	

	if( wd == NULL ) {
		return NULL;
	}

	_separator_obj_add( obj );

	item = calloc( 1, sizeof( Elm_Quickcontactbadge_Item ) );

	if( item == NULL ) {
		return NULL;
	}

	item->func = func;
	item->data = data;
	item->quickcontactbadge = obj;
	item->separator = EINA_FALSE;

	_item_obj_create( item );

	wd->items = eina_list_append( wd->items, item );

	elm_quickcontactbadge_item_icon_set( item, icon );
	_item_scale_shrinked_set( wd, item );

	elm_box_pack_end( wd->box, item->obj );

	_sizing_eval( obj );

	return item;

}



/**
 * Delete the given item in quickcontactbadge object.
 *
 * @param item 	 	Quickcontactbadge item to be deleted
 *
 * @ingroup Quickcontactbade
 */
EAPI void elm_quickcontactbadge_item_del( Elm_Quickcontactbadge_Item* item )
{
	Widget_Data* wd;

	if( item == NULL ) {
		return ;
	}

	if( item->content ) {
		evas_object_del( item->content );
	}

	if( item->obj ) {
		evas_object_del( item->obj );
	}

	wd = (Widget_Data*) elm_widget_data_get( item->quickcontactbadge );

	if( wd ) {
		_separator_obj_del( wd, item );
		wd->items = eina_list_remove( wd->items, item );
	}

	free( item );

	_item_scale_normal_set( wd );

	if( eina_list_count( wd->items ) < 1 ) {
		evas_object_hide( wd->arrow );
		evas_object_hide( wd->hover );
		evas_object_hide( wd->layout );		
	}
}


static void _changed_size_hints( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	_sizing_eval( data );
}

static void _separator_obj_del( Widget_Data* wd, Elm_Quickcontactbadge_Item* remove_item )
{
	Eina_List* elist;
	Elm_Quickcontactbadge_Item* separator;

	if( remove_item == NULL || wd == NULL ) {
		return ;
	}

	elist = wd->items;

	Eina_List* cur_list = eina_list_data_find_list( elist, remove_item );

	if( cur_list == NULL ) {
		return ;
	}

	Eina_List* prev_list = eina_list_prev( cur_list );

	if( prev_list ) {

		separator = (Elm_Quickcontactbadge_Item*) eina_list_data_get( prev_list );

		if( separator == NULL ) {
			return ;
		}

		wd->items = eina_list_remove( wd->items, separator );
		evas_object_del( separator->obj );
		free( separator );
	}

}


static void _separator_obj_add( Evas_Object* quickcontactbadge )
{
	Elm_Quickcontactbadge_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( quickcontactbadge );

	if( wd == NULL ) {
		return ;
	}

	if( eina_list_count( wd->items ) == 0 ) {
		return ;
	}

	item = calloc( 1, sizeof( Elm_Quickcontactbadge_Item ) );

	if( item == NULL ) {
		return ;
	}

	item->obj = edje_object_add( evas_object_evas_get( wd->location ) );

	if( item->obj == NULL ) {
		free( item );
		return ;
	}

	evas_object_size_hint_weight_set( item->obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND ) ;
	evas_object_size_hint_fill_set( item->obj, EVAS_HINT_FILL, EVAS_HINT_FILL );
	_elm_theme_object_set(quickcontactbadge, item->obj, "quickcontactbadge", "separator", elm_widget_style_get( quickcontactbadge ) ) ;


	if( wd->horizontal == EINA_TRUE ) {
		edje_object_signal_emit( item->obj, "elm,state,horizontal", "elm" );
	}else {
		edje_object_signal_emit( item->obj, "elm,state,vertical", "elm" );
	}


	evas_object_show( item->obj );

	item->separator = EINA_TRUE;

	elm_box_pack_end( wd->box, item->obj );
	wd->items = eina_list_append( wd->items, item );

}


static void _scale_shrinked_set( Elm_Quickcontactbadge_Item* item )
{
	if( item == NULL ) {
		return ;
	}

	if( item->disabled == EINA_FALSE ) {
		edje_object_signal_emit( item->obj, "elm,state,shrinked", "elm" );
	}else {
		edje_object_signal_emit( item->obj, "elm,state,shrinked_disabled", "elm" );
	}
}


static void _item_scale_normal_set( Widget_Data* wd )
{
	int item_count;
	Eina_List* elist;
	Elm_Quickcontactbadge_Item* item;

	if( wd == NULL ) {
		return ;
	}

	item_count =  eina_list_count( wd->items );
	item_count -= item_count >> 1;

	if( item_count == QUICKCONTACTBADGE_EXPAND_COUNT ) {

		EINA_LIST_FOREACH( wd->items, elist, item ) {

			if( item->disabled == EINA_FALSE ) {
				edje_object_signal_emit( item->obj, "elm,state,enabled", "elm" );
			}else {
				edje_object_signal_emit( item->obj, "elm,state,disabled", "elm" );
			}

		}

	}

}


static void _item_scale_shrinked_set( Widget_Data* wd, Elm_Quickcontactbadge_Item* add_item )
{
	int item_count;
	Eina_List* elist;
	Elm_Quickcontactbadge_Item* item;

	if( wd == NULL ) {
		return ;
	}

	if( wd->horizontal == EINA_TRUE ) {
		return;
	}

	item_count =  eina_list_count( wd->items );
	item_count -= item_count >> 1;

	if( item_count > QUICKCONTACTBADGE_EXPAND_COUNT + 1 ) {

		_scale_shrinked_set( add_item );

	}else if( item_count == QUICKCONTACTBADGE_EXPAND_COUNT+1 ) {

		EINA_LIST_FOREACH( wd->items, elist, item ) {

			_scale_shrinked_set( item );
		}
	}
}


static void _item_sizing_eval( Elm_Quickcontactbadge_Item* item )
{
	Evas_Coord min_w = -1;
	Evas_Coord min_h = -1;
	Evas_Coord max_w = -1;
	Evas_Coord max_h = -1;

	if( item == NULL ) {
		return ;
	}

	if( item->separator == EINA_FALSE ) {
		elm_coords_finger_size_adjust( 1, &min_w, 1, &min_h );
	}

	edje_object_size_min_restricted_calc( item->obj, &min_w, &min_h, min_w, min_h );

	if( item->separator == EINA_FALSE ) {
		elm_coords_finger_size_adjust( 1, &min_w, 1, &min_h );
	}

	evas_object_size_hint_min_set( item->obj, min_w, min_h );
	evas_object_size_hint_max_set( item->obj, max_w, max_h );

}


static void _update_arrow_obj( Widget_Data* wd, Arrow_Direction arrow_dir, Geometry_Rect* rect )
{
	Evas_Coord arrow_x, arrow_y;

	if( wd == NULL ) {
		return ;
	}

	arrow_x = wd->x;
	arrow_y = wd->y;

	switch( arrow_dir ) {
		case Top_Arrow:
		{
			edje_object_signal_emit( wd->arrow, "elm,top_arrow,show", "elm" );
			arrow_x = QUICKCONTACTBADGE_ICON_LEFT_MARGIN*5;
			arrow_y += _quickcontactbadge_touch_range;
			break;
		}
		case Bottom_Arrow:
		{
			edje_object_signal_emit( wd->arrow, "elm,bottom_arrow,show", "elm" );
			arrow_x = QUICKCONTACTBADGE_ICON_LEFT_MARGIN*5;
			arrow_y -= _quickcontactbadge_touch_range;
			break;
		}
		default:
			fprintf( stderr, "There is something error in arrow direction!!\n" );
	}

	evas_object_move( wd->arrow, arrow_x, arrow_y );
    rect->y = arrow_y+QUICKCONTATBADGE_ICON_TOP_MARGIN;
    rect->x = QUICKCONTACTBADGE_ICON_LEFT_MARGIN;
}


static void _get_obj_size( Evas_Object* quickcontactbadge, double scale  )
{

	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( quickcontactbadge );

	if( wd == NULL ) {
		return ;
	}

	Evas_Object* data_info = edje_object_add( evas_object_evas_get( wd->location ) );
	_elm_theme_object_set(quickcontactbadge, data_info, "quickcontactbadge", "data_info", elm_widget_style_get( quickcontactbadge ) );

	//Arrow
	_quickcontactbadge_arrow_max_size = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "arrow_max_size" ) ) * scale );
	_quickcontactbadge_arrow_size = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "arrow_min_size" ) ) * scale );

	Evas_Coord prev_width = _quickcontactbadge_width_max;

	//Hover
	_quickcontactbadge_width_max = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "hover_width_max_size" ) ) * scale );
	_quickcontactbadge_height_max = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "hover_height_max_size" ) ) * scale );

	//Indicator
	_quickcontactbadge_indicator_height = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "indicator_height_size" ) ) * scale );

	//touch_range
	_quickcontactbadge_touch_range = (Evas_Coord) ( atoi( edje_object_data_get( data_info, "touch_range" ) ) * scale );

	evas_object_del( data_info );

	_size_rate = (float)_quickcontactbadge_width_max / (float) prev_width;

}


static void _sizing_eval( Evas_Object* obj )
{
	Widget_Data* wd;
	Eina_List* elist;
	Elm_Quickcontactbadge_Item* item;
	Geometry_Rect rect;

	wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL || wd->parent == NULL ) {
		return ;
	}

	EINA_LIST_FOREACH( wd->items, elist, item ) {
		_item_sizing_eval( item );
	}

    _update_arrow_obj( wd, 	Top_Arrow, &rect );
	evas_object_move( wd->location, rect.x, rect.y );
	evas_object_resize( wd->location, rect.w, rect.h );
	evas_object_move( wd->hover, rect.x, rect.y );
	evas_object_resize( wd->hover, rect.w, rect.h );
	evas_object_move( wd->layout, 0, rect.y-QUICKCONTACTBADGE_BG_TOP_MARGIN );	
}


static void _del_pre_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	evas_object_event_callback_del_full(
				wd->parent,
				EVAS_CALLBACK_RESIZE,
				_parent_resize,
				obj );

}


static void _del_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	elm_quickcontactbadge_clear( obj );

	if( wd->arrow ) {
		evas_object_del( wd->arrow );
	}

	if( wd->hover ) {
		evas_object_del( wd->hover );
	}

	if( wd->box ) {
		evas_object_del( wd->box );
	}

	if( wd->location ) {
		evas_object_del( wd->location );
	}

	if (wd->layout) {
	    evas_object_del (wd->layout);
	}
    _item_cnt = 0;
	free( wd );

}


static void _theme_hook( Evas_Object* obj )
{
	Eina_List* elist;
	Eina_List* elist_child;
	Eina_List* elist_temp;
	Elm_Quickcontactbadge_Item* item;
	int item_count;

	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	item_count = eina_list_count( wd->items );
	item_count -= item_count >> 1;

	elist = eina_list_append( elist, wd->items );

	EINA_LIST_FOREACH( elist, elist, elist_child ) {
		EINA_LIST_FOREACH( elist_child, elist_temp, item ) {
			if( item->separator == EINA_TRUE ) {
				_elm_theme_object_set(obj, item->obj,
						"quickcontactbadge",
						"separator",
						elm_widget_style_get( obj ) );
				if( wd->horizontal == EINA_TRUE ) {
					edje_object_signal_emit( item->obj, "elm,state,horizontal", "elm" );
				} else {
					edje_object_signal_emit( item->obj, "elm,state,vertical", "elm" );
 				}
			}else {
				_elm_theme_object_set(obj, item->obj,
					"quickcontactbadge",
					"item",
					elm_widget_style_get( obj ) );

				if( item->disabled == EINA_TRUE ) {
					if( item_count > QUICKCONTACTBADGE_EXPAND_COUNT ) {
						edje_object_signal_emit( item->obj, "elm,state,shrinked_disabled", "elm" );
					}else {
						edje_object_signal_emit( item->obj, "elm,state,disabled","elm" );
					}
				}else {
					if( item_count > QUICKCONTACTBADGE_EXPAND_COUNT ) {
						edje_object_signal_emit( item->obj, "elm,state,shrinked", "elm" );
					}else {
						edje_object_signal_emit( item->obj, "elm,state,enabled", "elm" );
					}
				}
			}
			edje_object_message_signal_process( item->obj );
		}
	}

	elm_object_style_set( wd->hover, "quickcontactbadge");
	_elm_theme_object_set(obj, wd->arrow, "quickcontactbadge", "arrow", elm_widget_style_get( obj ) );
	_get_obj_size( obj, edje_scale_get() );

	_sizing_eval( obj );

}


static void _hover_clicked_cb( void* data, Evas_Object* obj, void* event_info )
{
	evas_object_hide( data );
	evas_object_smart_callback_call( data, "ctxpopup,hide", NULL );

}


static void _parent_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	_sizing_eval( data );
}


static void _quickcontactbadge_show( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	if( wd == NULL ) {
		return ;
	}

	if( eina_list_count( wd->items ) < 1 ) {
		return ;
	}

	evas_object_show( wd->arrow );
	evas_object_show( wd->hover );
	evas_object_show( wd->layout );	
}


static void _quickcontactbadge_hide( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	if( wd == NULL ) {
		return ;
	}
	evas_object_hide( wd->arrow );
	evas_object_hide( wd->hover );
	evas_object_hide( wd->layout );	
}


static void _quickcontactbadge_move( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Evas_Coord x, y, w, h;
	Widget_Data* wd = (Widget_Data*) data;

	if( wd == NULL ) {
		return ;
	}

	evas_object_geometry_get( obj, &x, &y, &w, &h );

	wd->x = x;
	wd->y = y;

	_sizing_eval( obj );

}


static void _quickcontactbadge_item_select(
		void* data,
		Evas_Object* obj,
		const char* emission,
		const char* source )
{
	Elm_Quickcontactbadge_Item* item = (Elm_Quickcontactbadge_Item*) data;

	if( item == NULL ) {
		return ;
	}

	if( item->disabled == EINA_TRUE ) {
		return ;
	}

	if( item->func ) {
		item->func( (void*) (item->data), item->quickcontactbadge, item );
	}

}


static void _arrow_obj_add( Evas_Object* quickcontactbadge )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( quickcontactbadge );

	if( wd == NULL ) {
		return ;
	}

	wd->arrow = edje_object_add( evas_object_evas_get( wd->location) );

	evas_object_size_hint_weight_set( wd->arrow, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	_elm_theme_object_set(quickcontactbadge, wd->arrow, "quickcontactbadge", "arrow", elm_widget_style_get( quickcontactbadge ) );
	edje_object_signal_emit( wd->arrow, "elm,bottom_arrow,show", "elm" );


}


static void _item_obj_create( Elm_Quickcontactbadge_Item* item )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( item->quickcontactbadge );

	if( wd == NULL ) {
		return ;
	}

	item->obj = edje_object_add( evas_object_evas_get( wd->location ) );

	evas_object_size_hint_weight_set( item->obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_fill_set( item->obj, EVAS_HINT_FILL, EVAS_HINT_FILL );
	_elm_theme_object_set(item->quickcontactbadge, item->obj, "quickcontactbadge", "item", elm_widget_style_get( item->quickcontactbadge ) );

	edje_object_signal_callback_add(
			item->obj,
			"elm,action,click",
			"",
			_quickcontactbadge_item_select,
			item );

	evas_object_show( item->obj );

}

