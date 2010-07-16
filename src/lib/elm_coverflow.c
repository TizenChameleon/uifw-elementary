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



#include <Elementary.h>
#include "elm_priv.h"

#define COVER_SIZE 180
#define HALF_COVER_SIZE COVER_SIZE / 2
#define TI_BUFFER_SIZE 30
#define SLIDING_DURATION 1.5
#define BOUNCING_DURATION (SLIDING_DURATION*0.75)
#define REFLECTION_COLOR 150 
#define ROTATION_DEGREE 35 
#define CACHED_ITEM_COUNT 50
#define CACHED_ITEM_SIZE (CACHED_ITEM_COUNT * COVER_SIZE )
#define DISTANCE_BETWEEN_ITEMS 130 
#define BEGIN_POS_X -22
#define ROTATION_DURATION 0.25 
#define ZOOM_OUT_DISTANCE   500
#define ZOOM_OUT_DURATION   0.5
#define MOVE_ON_LIMIT 3
#define SELECTED_ITEM_POS_Z  -200

typedef struct _Widget_Data Widget_Data;
typedef struct _Touch_Info_Node TI_Node;
typedef struct _Touch_Info_Queue TI_Queue;
typedef struct _Point Point;
typedef enum _Shift_Direction Shift_Direction;
typedef struct _Float_Ani_Data Float_Ani_Data;
typedef struct _EvasCoord_Ani_Data EvasCoord_Ani_Data;
typedef struct _Select_Ani_Data Select_Ani_Data;

enum _Shift_Direction { Shift_Left, Shift_Right, Shift_None };

struct _Touch_Info_Node {
	Evas_Coord pos;
	TI_Node* prev;
	TI_Node* next;
};

struct _Touch_Info_Queue {
	TI_Node* 	head;
	TI_Node* 	tail;
	unsigned int 	cnt;
};

struct _Coverflow_Item
{
	Widget_Data* wd;
	Evas_Object* base;
	Evas_Object* reflection;
	Elm_Coverflow_Item_Class cic;
	void* data;
	void (*func)( void*, Evas_Object*, void* );
	void* func_data;
	int idx;
	Eina_Bool   visible : 1;
};
/*
struct _Animation_Data {
	Elm_Animator* animator;
	Evas_Coord from;
};
*/
struct _Point {
	Evas_Coord x;
	Evas_Coord y;
	Evas_Coord z;
};

struct _Float_Ani_Data {
	Elm_Animator*  animator;
	float cur;
	float from;
	float to;
};

struct _EvasCoord_Ani_Data {
	Elm_Animator*  animator;
	Evas_Coord cur;
	Evas_Coord from;
	Evas_Coord to;
};

struct _Select_Ani_Data {
	Elm_Animator* animator;
	Evas_Coord orig_pos_x;
	Evas_Coord orig_pos_y;
	Evas_Coord from_x, from_z;
	Evas_Coord to_x, to_z;
	float      orig_rot;
	float from_rot, to_rot;
};



struct _Widget_Data 
{
	Evas_Object* 		parent;
	Evas_Object*    	obj;
	Evas_Object* 		base;
	Eina_List* 		all_item_list;
	Eina_List*		valid_item_list;
	Point           	base_pos;
	Point 	          	cur_pos;
	Evas_Coord     	 	sliding_vector;
	TI_Queue        	ti_queue;
	Float_Ani_Data 		rot_ani_data;
	EvasCoord_Ani_Data      base_ani_data;
	Select_Ani_Data*        select_ani_data;	
	unsigned long   	valid_item_cnt;
	unsigned long   	valid_item_length;
	Evas_Map*               original_map;
	Evas_Map*               reflection_map;
	Elm_Coverflow_Item*     selected_item;
	Evas_Coord      	dist_between_items;

	Eina_Bool       	stack_raise : 1;
	Eina_Bool   		horizontal : 1;
	Eina_Bool   		move_on : 1;
	Eina_Bool       	show : 1;
};




static void _parent_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _coverflow_show( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _coverflow_hide( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _coverflow_move( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _coverflow_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _del_pre_hook( Evas_Object* obj );
static void _del_hook( Evas_Object* obj );
static void _sizing_eval( Widget_Data* wd );
static void _render_visible_items( Widget_Data* wd );
static void _mouse_down_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _mouse_move_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static void _mouse_up_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info );
static TI_Node* _create_tinode( const Evas_Coord pos );
static void _clear_ti_queue( TI_Queue* ti_queue ); 
static void _push_back_tinode( TI_Queue* ti_queue, TI_Node* tinode );
static void _sliding_completion_cb( void* data );
static void _sliding_operation_cb( void* data, Elm_Animator* animator, const double key_frame );
static void _update_items( Widget_Data* wd, Shift_Direction dir, Eina_Bool stack_raise );
static Evas_Map* _create_original_map();
static Evas_Map* _create_reflection_map( const Evas_Coord reflection_size );
static void _init_item_obj( Evas_Object* obj );
static Eina_Bool _left_shift_item( Widget_Data* wd, Elm_Coverflow_Item* item );
static Eina_Bool _right_shift_item( Widget_Data* wd, Elm_Coverflow_Item* item );
static void _create_item_obj( Widget_Data* wd, Elm_Coverflow_Item* item );
static void _remove_item_obj( Widget_Data* wd, Elm_Coverflow_Item* item );
static void _calc_base_pos( Widget_Data* wd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h );
static void _direct_bounce( Widget_Data* wd, Evas_Coord to );
static void _left_bounce_cb( void* data );
static void _right_bounce_cb( void* data );
static Eina_Bool _bounce_to_left( Widget_Data* wd );
static void _rotation_operation_cb( void* data, Elm_Animator* animator, const double key_frame );
static void _rotation_completion_cb( void* data );
static void _update_rotation_degree( Widget_Data* wd, float degree );
static void _zoom_out_completion_cb( void* data );
static void _zoom_out_operation_cb( void* data, Elm_Animator* animator, const double key_frame );


inline static TI_Node* _create_tinode( const Evas_Coord pos )
{
	TI_Node* tinode = (TI_Node*) calloc( 1, sizeof( TI_Node ) );

	if( tinode ) {
		tinode->pos = pos;
	}

	return tinode;
}



static void _push_back_tinode( TI_Queue* ti_queue, TI_Node* tinode )
{
	if( tinode == NULL ) {
		return ;
	}

	tinode->next = NULL;

	if( ti_queue->head == NULL ) {
		++ti_queue->cnt;
		tinode->prev = NULL;
		ti_queue->head = tinode;
		ti_queue->tail = tinode;
		return ;
	}

	ti_queue->tail->next = tinode;
	tinode->prev = ti_queue->tail;
	ti_queue->tail = tinode;

	if( ti_queue->cnt >= TI_BUFFER_SIZE ) {
		TI_Node* head = ti_queue->head;
		ti_queue->head = head->next;
		ti_queue->head->prev = NULL;
		free( head );
		return ;
	}

	++ti_queue->cnt;
}



static void _clear_ti_queue( TI_Queue* ti_queue ) 
{
	TI_Node* head = ti_queue->head; 
	TI_Node* tinode;

	while( head ) {
		tinode = head->next;
		free( head );
		head = tinode;
	}

	ti_queue->head = ti_queue->tail = NULL;
	ti_queue->cnt = 0;
	
}



static void _coverflow_show( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	int idx = 0;
	Eina_List* elist;
	Elm_Coverflow_Item* item;

	EINA_LIST_FOREACH( wd->valid_item_list, elist, item ) {
		
		evas_object_show( item->base );
		evas_object_show( item->reflection );

		if( ++idx == wd->valid_item_cnt ) {
			break;
		}

	}

	evas_object_show( wd->base );
	wd->show = EINA_TRUE;

	_render_visible_items( wd );

}

static void _coverflow_hide( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	int idx = 0;
	Eina_List* elist;
	Elm_Coverflow_Item* item;

	EINA_LIST_FOREACH( wd->valid_item_list, elist, item ) {
		
		evas_object_hide( item->base );
		evas_object_hide( item->reflection );

		if( ++idx == wd->valid_item_cnt ) {
			break;
		}
	}

	evas_object_hide( wd->base );
	
}

static void _coverflow_move( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	Evas_Coord x, y, w, h;
	evas_object_geometry_get( obj, &x, &y, &w, &h );
	evas_object_move( wd->base, x, y );
		
	wd->valid_item_cnt = w / COVER_SIZE + 2 + CACHED_ITEM_COUNT * 2;

	_calc_base_pos( wd, x, y, w, h );	
	_update_items( wd, Shift_None, wd->stack_raise ); 
	_render_visible_items( wd );


}

static void _calc_base_pos( Widget_Data* wd, 
		            Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h )
{
	Evas_Coord valid_item_size;
	
	if( wd->valid_item_cnt < eina_list_count( wd->valid_item_list ) ) {
		valid_item_size = wd->valid_item_cnt * wd->dist_between_items;
	}else {
		valid_item_size = eina_list_count( wd->valid_item_list ) * wd->dist_between_items;
	}

	//horizontal mode 
	if( valid_item_size < w ) {
		wd->cur_pos.x = wd->base_pos.x = (w >> 1) - (valid_item_size >>1) + x; 
	}else {
		wd->cur_pos.x = wd->base_pos.x = x;
	}

	wd->cur_pos.y = wd->base_pos.y = y + ( h >> 1 ) - HALF_COVER_SIZE;

}


static void _parent_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{

}


static void _coverflow_resize( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	Evas_Coord x, y, w, h;
	evas_object_geometry_get( obj, &x, &y, &w, &h );
	evas_object_resize( wd->base, w, h );

	wd->valid_item_cnt = w / COVER_SIZE + 2 + CACHED_ITEM_COUNT * 2;

	_calc_base_pos( wd, x, y, w, h );

	//TODO: re-create items. 
	
	_update_items( wd, Shift_None, wd->stack_raise );
	_render_visible_items( wd );

}




static void _del_pre_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd->base_ani_data.animator ) {
		elm_animator_stop( wd->base_ani_data.animator );
		elm_animator_del( wd->base_ani_data.animator );
	}
	
}




static void _del_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	_clear_ti_queue( &wd->ti_queue );

	evas_map_free( wd->original_map );
	evas_map_free( wd->reflection_map );

	elm_coverflow_clear( obj );
	

	evas_object_del( wd->base );


}




static void _sizing_eval( Widget_Data* wd )
{
/*	Eina_List* elist;
	Elm_Coverflow_Item* item;

	EINA_LIST_FOREACH( wd->all_item_list, elist, item ) {
		evas_object_resize( item->obj, COVER_SIZE, COVER_SIZE );
		evas_object_resize( item->reflection, COVER_SIZE, COVER_SIZE );
	}
*/
}

inline static Evas_Map* _create_original_map()
{
	Evas_Map* map = evas_map_new( 4 );

	if( map == NULL ) {
		return NULL;
	}

	evas_map_smooth_set( map, EINA_TRUE );

	return map;
}


inline static Evas_Map* _create_reflection_map( const Evas_Coord reflection_size )
{
	Evas_Map* map = _create_original_map();

	evas_map_point_image_uv_set( map, 0, 0, COVER_SIZE );
	evas_map_point_image_uv_set( map, 1, COVER_SIZE, COVER_SIZE );
	evas_map_point_image_uv_set( map, 2, COVER_SIZE, COVER_SIZE-reflection_size );
	evas_map_point_image_uv_set( map, 3, 0, COVER_SIZE-reflection_size );

	evas_map_point_color_set( map, 0, 
		REFLECTION_COLOR, REFLECTION_COLOR, REFLECTION_COLOR, REFLECTION_COLOR );

	evas_map_point_color_set( map, 1, 
		REFLECTION_COLOR, REFLECTION_COLOR, REFLECTION_COLOR, REFLECTION_COLOR );
	
	evas_map_point_color_set( map, 2, 
		0, 0, 0, 0 );
	
	evas_map_point_color_set( map, 3, 
		0, 0, 0, 0 );

	return map;

}






static void _render_visible_items( Widget_Data* wd )
{
	Evas_Coord reflection_size = COVER_SIZE / 3;

	Eina_List* elist;
	Elm_Coverflow_Item* item;
	Evas_Coord x, y, z = wd->cur_pos.z;
	int idx = 0;

	EINA_LIST_FOREACH( wd->valid_item_list, elist, item ) {

		++idx;

		if( item->visible == EINA_FALSE ) {
			continue;
		}

		if( item == wd->selected_item ) {
			continue;
		}

		evas_object_geometry_get( item->base, &x, &y, NULL, NULL );

		//Geometry 
		//ToDo: Check coord_set . 
		evas_map_util_points_populate_from_object_full( wd->original_map, item->base, z );

		evas_map_point_coord_set( wd->reflection_map, 0, 
				    x, y + COVER_SIZE, z );
		evas_map_point_coord_set( wd->reflection_map, 1, 
				    x + COVER_SIZE, y + COVER_SIZE, z );
		evas_map_point_coord_set( wd->reflection_map, 2,
				    x + COVER_SIZE, y + COVER_SIZE + reflection_size, z );
		evas_map_point_coord_set( wd->reflection_map, 3,
				    x, y + COVER_SIZE + reflection_size, z );

		//Rotation
		evas_map_util_3d_rotate( wd->original_map, 0, wd->rot_ani_data.cur, 0, 
				x + HALF_COVER_SIZE, y + HALF_COVER_SIZE, z );

		evas_map_util_3d_rotate( wd->reflection_map, 0, wd->rot_ani_data.cur, 0,
				x + HALF_COVER_SIZE, y + HALF_COVER_SIZE, z );

		//Perspective 
		evas_map_util_3d_perspective( wd->original_map, 
					      x + HALF_COVER_SIZE, 
					      y + HALF_COVER_SIZE, 0, 750 );

		evas_map_util_3d_perspective( wd->reflection_map, 
				              x + HALF_COVER_SIZE, 
				              y + HALF_COVER_SIZE, 0, 750 );

		evas_object_map_set( item->base, wd->original_map );
		evas_object_map_set( item->reflection, wd->reflection_map );

		evas_object_show( item->base );
		evas_object_show( item->reflection );

		if( idx == wd->valid_item_cnt ) {
			break;
		}

	}

	evas_object_raise( wd->base );

}

static void _mouse_down_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	if( wd->base_ani_data.animator ) {
		elm_animator_stop( wd->base_ani_data.animator );
		elm_animator_del( wd->base_ani_data.animator );
		wd->base_ani_data.animator = NULL;
	}

	_clear_ti_queue( &wd->ti_queue );

	Evas_Event_Mouse_Down* ev = event_info;
	_push_back_tinode( &wd->ti_queue, _create_tinode( ev->output.x ) );

	wd->move_on = EINA_FALSE;

}


inline static void _remove_item_obj( Widget_Data* wd, Elm_Coverflow_Item* item )
{
	if( item == NULL ) {
		return ;
	}

	evas_object_del( item->base );
	evas_object_del( item->reflection );

	wd->valid_item_length -= wd->dist_between_items;

}

	

static Eina_Bool _left_shift_item( Widget_Data* wd, Elm_Coverflow_Item* item )
{
	//valid items are less than cached item count !
	if(  eina_list_count( wd->all_item_list ) - item->idx <= wd->valid_item_cnt ) {
		return EINA_FALSE;
	}
		
	_remove_item_obj( wd, item );


	//Generate
	Elm_Coverflow_Item* gen_item = (Elm_Coverflow_Item*) eina_list_data_get( 
			eina_list_nth_list( wd->all_item_list, item->idx + wd->valid_item_cnt ) );

	_create_item_obj( wd, gen_item );

	//shift item.
	wd->valid_item_list = eina_list_next(wd->valid_item_list);

	return EINA_TRUE;
}


static Eina_Bool _right_shift_item( Widget_Data* wd, Elm_Coverflow_Item* item ) {

	//valid items are less than cached item count !
	if( ((Elm_Coverflow_Item*) eina_list_data_get( wd->valid_item_list ))->idx  <= 0  ) {
		return EINA_FALSE;
	}
 
	_remove_item_obj( wd, item );

	//Generate
	Elm_Coverflow_Item* gen_item = (Elm_Coverflow_Item*) eina_list_data_get( 
			eina_list_nth_list( wd->all_item_list, item->idx - wd->valid_item_cnt ) );

	_create_item_obj( wd, gen_item );

	//shift_item. 
	wd->valid_item_list = eina_list_prev(wd->valid_item_list);

	return EINA_TRUE;
}


static void _update_items( Widget_Data* wd, Shift_Direction dir, Eina_Bool stack_raise )

{
	Evas_Coord body_x, body_y, body_w, body_h;
	evas_object_geometry_get( wd->base, &body_x, &body_y, &body_w, &body_h );	

	Eina_List* elist;
	Elm_Coverflow_Item* item;
	int idx = 0;
	int shift_cnt = 0;

	Evas_Coord cur_pos; 
	Evas_Object* prev_obj = NULL;

	EINA_LIST_FOREACH( wd->valid_item_list, elist, item ) {

		//horizontal mode
		cur_pos = idx++ * wd->dist_between_items + wd->cur_pos.x + BEGIN_POS_X;
		evas_object_move( item->base, cur_pos, wd->cur_pos.y ); 

		//Clipping!
		if( cur_pos + COVER_SIZE < body_x || cur_pos - COVER_SIZE > body_x + body_w ) {
			item->visible = EINA_FALSE;
			evas_object_hide( item->base );
			evas_object_hide( item->reflection );
		}else {
			item->visible = EINA_TRUE;
		}
		
		if( item->visible == EINA_FALSE ) {

			switch( dir ) {
				case Shift_Left:
					//Shift Left Side item!
					if( cur_pos + COVER_SIZE < body_x - CACHED_ITEM_SIZE ) {

						if( _left_shift_item( wd, item ) == EINA_TRUE ) {
							++shift_cnt;
						}
					}
					break;
				case Shift_Right:
					//Shift Right Side item!
					 if( cur_pos > body_x + body_w + CACHED_ITEM_SIZE ) {

						if( _right_shift_item( wd, item ) == EINA_TRUE ) {
							--shift_cnt;
						}
					 }
					 break;
				case Shift_None:
				default:
					 break;
			}		
		
		//Z Order
		}else {

			if( stack_raise == EINA_TRUE ) {
				evas_object_raise( item->base );
				evas_object_raise( item->reflection );
			}else {					
				evas_object_stack_above( prev_obj, item->reflection );
				evas_object_stack_above( item->reflection, item->base );
			}

		}

		prev_obj = item->base;

		if( idx == wd->valid_item_cnt ) {
			break ;
		}

	}

	//The base position should be shifted by size of removed items. 
	if( shift_cnt ) {
		shift_cnt *= wd->dist_between_items;
		wd->cur_pos.x += shift_cnt;
		wd->base_ani_data.from += shift_cnt;
	}

}

static void _rotation_operation_cb( void* data, Elm_Animator* animator, const double key_frame )
{
	Widget_Data* wd = (Widget_Data*) data;
	wd->rot_ani_data.cur = wd->rot_ani_data.from + wd->rot_ani_data.to * key_frame;
}

static void _rotation_completion_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;
	elm_animator_del( wd->rot_ani_data.animator );
	wd->rot_ani_data.animator = NULL;
}

static void _update_rotation_degree( Widget_Data* wd, float degree )
{
	if( wd->rot_ani_data.cur == degree ) {
		return ;
	}

	if( wd->rot_ani_data.animator ) {
		elm_animator_stop( wd->rot_ani_data.animator );
	}else {
		wd->rot_ani_data.animator = elm_animator_add( wd->obj );
		elm_animator_operation_callback_set( wd->rot_ani_data.animator, _rotation_operation_cb, wd );
		elm_animator_completion_callback_set( wd->rot_ani_data.animator, _rotation_completion_cb, wd );
		elm_animator_duration_set( wd->rot_ani_data.animator, ROTATION_DURATION );
	}

	wd->rot_ani_data.from = wd->rot_ani_data.cur;
	wd->rot_ani_data.to = degree - wd->rot_ani_data.cur;
	elm_animator_animate( wd->rot_ani_data.animator );

}

static void _mouse_move_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Evas_Event_Mouse_Move* ev = ( Evas_Event_Mouse_Move* ) event_info;

	if( ev->buttons == 0 ) {
		return ;
	}

	Widget_Data* wd = (Widget_Data*) data;

	if( wd->move_on == EINA_FALSE ) {
		if( abs( ev->cur.output.x - wd->ti_queue.head->pos ) < MOVE_ON_LIMIT ) {
			return ;
		}else {
			wd->move_on = EINA_TRUE;
		}
	}

#ifdef SKIP_EVENT
	static int skip_count = 0;
		
	++skip_count;

	if( skip_count < 4 ) {
		++skip_count;
		return ;
	}
	skip_count = 0;
#endif 

	Evas_Coord vector; 

	//horizontal mode
	vector =  ev->cur.output.x - ev->prev.output.x; 
	wd->cur_pos.x += vector;

	if( wd->rot_ani_data.cur > (180-ROTATION_DEGREE)/2+ROTATION_DEGREE) {
		wd->stack_raise = EINA_FALSE;
	}else {
		wd->stack_raise = EINA_TRUE;
	}

	if( vector > 0 ) {
		_update_items(wd, Shift_Right, wd->stack_raise );
		wd->rot_ani_data.cur += 3;
		if( wd->rot_ani_data.cur > 180 - ROTATION_DEGREE ) {
			wd->rot_ani_data.cur = 180 - ROTATION_DEGREE;
		}
	}else {
		_update_items(wd, Shift_Left, wd->stack_raise );
		wd->rot_ani_data.cur -= 3;
		if( wd->rot_ani_data.cur < ROTATION_DEGREE ) {
			wd->rot_ani_data.cur = ROTATION_DEGREE;
		}
	}

	//TODO: if Drag Direction is changed, let's clear queue!
	
	_push_back_tinode( &wd->ti_queue, _create_tinode( ev->cur.output.x ) );
	_render_visible_items( wd );

}


static void _direct_bounce( Widget_Data* wd, Evas_Coord to )
{
	wd->sliding_vector = to - wd->cur_pos.x;
	wd->base_ani_data.animator = elm_animator_add( wd->obj );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data.animator, _sliding_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _sliding_completion_cb, wd );
	elm_animator_duration_set( wd->base_ani_data.animator, BOUNCING_DURATION );
	elm_animator_animate( wd->base_ani_data.animator );
}


static void _sliding_completion_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;
	elm_animator_del( wd->base_ani_data.animator );
	wd->base_ani_data.animator = NULL;
}

static void _right_bounce_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;

	Evas_Coord w;
	evas_object_geometry_get( wd->obj, NULL, NULL, &w, NULL );

	wd->sliding_vector = w - wd->cur_pos.x + wd->valid_item_length;
	wd->base_ani_data.from = wd->cur_pos.x;
	wd->stack_raise = EINA_FALSE;

	elm_animator_duration_set( wd->base_ani_data.animator, BOUNCING_DURATION * 0.5  );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _sliding_completion_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

}



static void _left_bounce_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;

	wd->sliding_vector = -wd->cur_pos.x;
	wd->base_ani_data.from = wd->cur_pos.x;
	wd->stack_raise = EINA_FALSE;

	elm_animator_duration_set( wd->base_ani_data.animator, BOUNCING_DURATION * 0.5  );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _sliding_completion_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

}



static void _sliding_operation_cb( void* data, Elm_Animator* animator, const double key_frame )
{
	Widget_Data* wd = (Widget_Data*) data;
	
	wd->cur_pos.x = wd->base_ani_data.from + (Evas_Coord) ( (double) wd->sliding_vector * key_frame );

	if( wd->sliding_vector > 0 ) {
		_update_items( wd, Shift_Right, wd->stack_raise );
	}else {
		_update_items( wd, Shift_Left, wd->stack_raise );
	}

	_render_visible_items( wd );

}


static Eina_Bool _bounce_to_right( Widget_Data* wd )
{
	Evas_Coord w;
	evas_object_geometry_get( wd->obj, NULL, NULL, &w, NULL );
	
	Elm_Coverflow_Item* item =  eina_list_data_get( wd->valid_item_list );	
	Evas_Coord a = item->idx * wd->dist_between_items;
		
	Evas_Coord right_outside_length = wd->cur_pos.x + (Evas_Coord) wd->valid_item_length - w;
	Evas_Coord sliding_length = wd->sliding_vector;

	if( right_outside_length + sliding_length <= 0 ) {
		return EINA_FALSE;
	}

	Evas_Coord bounce_pos  = (right_outside_length + sliding_length) / 2;
	
	wd->sliding_vector = bounce_pos - right_outside_length;

	wd->base_ani_data.animator = elm_animator_add( wd->obj );
	elm_animator_duration_set( wd->base_ani_data.animator, SLIDING_DURATION * 0.5 );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data.animator, _sliding_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _sliding_completion_cb, wd );
//	elm_animator_completion_callback_set( wd->base_ani_data.animator, _right_bounce_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

	return EINA_TRUE;

}



static Eina_Bool _bounce_to_left( Widget_Data* wd )
{
	Elm_Coverflow_Item* item = eina_list_data_get( wd->valid_item_list );
	Evas_Coord left_outside_length = wd->cur_pos.x -item->idx * wd->dist_between_items;
	Evas_Coord sliding_length = wd->sliding_vector;

	if( left_outside_length + sliding_length < 0 ) {
		return EINA_FALSE;
	}

	Evas_Coord bounce_pos  = (left_outside_length + sliding_length) / 2;
	
	wd->sliding_vector = bounce_pos - left_outside_length;

	wd->base_ani_data.animator = elm_animator_add( wd->obj );
	elm_animator_duration_set( wd->base_ani_data.animator, SLIDING_DURATION * 0.5 );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data.animator, _sliding_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _left_bounce_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

	return EINA_TRUE;

}



static void _mouse_up_ev( void* data, Evas* evas, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;

	if( wd->move_on == EINA_FALSE ) {
		return ;
	}

	if( wd->ti_queue.cnt == 0 ) {
		return ;
	}

	wd->base_ani_data.from = wd->cur_pos.x;
	wd->sliding_vector = wd->ti_queue.tail->pos - wd->ti_queue.head->pos;

	//TODO:Please check animator in direct_bouce and update_rotation_degree, bounce_to_left..

	//Rotation 
	if( wd->sliding_vector > 0 ) {
		wd->stack_raise = EINA_FALSE;
		_update_rotation_degree( wd, 180 - ROTATION_DEGREE );
	}else {
		wd->stack_raise = EINA_TRUE;
		_update_rotation_degree( wd, ROTATION_DEGREE );
	}

	//Check Bouncing
	Evas_Coord w;
	evas_object_geometry_get( obj, NULL, NULL, &w, NULL );
	
	//bounce to left directly  
	if( wd->cur_pos.x > 0 ) {
		_direct_bounce( wd, 0 );
		return ;
	//bounce to right directly
	}else if( wd->cur_pos.x + (Evas_Coord) wd->valid_item_length < w ) {
		_direct_bounce( wd, w - (Evas_Coord) wd->valid_item_length );
		return ;
	}

	if( _bounce_to_left( wd ) == EINA_TRUE ) {
		return ;
	}
/*
	if( _bounce_to_right( wd ) == EINA_TRUE ) {
		return ;
	}
*/
	//sliding effect
	wd->base_ani_data.animator = elm_animator_add( obj );
	elm_animator_duration_set( wd->base_ani_data.animator, SLIDING_DURATION );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data.animator, _sliding_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, _sliding_completion_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

}



EAPI Evas_Object* elm_coverflow_add( Evas_Object* parent )
{
	Evas* evas;
	Evas_Object* obj;
	Widget_Data* wd;
	evas = evas_object_evas_get( parent );

	if( evas == NULL ) {
		return NULL;
	}

	obj = ( Evas_Object* ) elm_widget_add( evas );

	if( obj == NULL ) {
		return NULL;
	}

	elm_widget_type_set( obj, "coverflow" );
	elm_widget_sub_object_add( parent, obj );

	wd = calloc( 1, sizeof( Widget_Data ) );

	if( wd == NULL ) {
		return NULL;
	}

	elm_widget_data_set( obj, wd );
	elm_widget_del_pre_hook_set( obj, _del_pre_hook );
	elm_widget_del_hook_set( obj, _del_hook );

	wd->parent = parent;
	wd->obj = obj;
	wd->horizontal = EINA_TRUE;
	wd->rot_ani_data.cur = ROTATION_DEGREE;
	wd->dist_between_items = DISTANCE_BETWEEN_ITEMS;
	wd->stack_raise = EINA_TRUE;
	wd->original_map = _create_original_map();
	wd->reflection_map = _create_reflection_map( COVER_SIZE / 3 );

	//body
	wd->base = evas_object_rectangle_add( evas );
	evas_object_color_set( wd->base, 0, 0, 0, 0 );
	evas_object_resize( wd->base, 99999999, 9999999 );
	evas_object_size_hint_weight_set( wd->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_align_set( wd->base, EVAS_HINT_FILL, EVAS_HINT_FILL );
	evas_object_event_callback_add( wd->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_ev, wd );
	evas_object_event_callback_add( wd->base, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_ev, wd );
	evas_object_event_callback_add( wd->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up_ev, wd );
	evas_object_repeat_events_set( wd->base, EINA_TRUE );

	//Set widget base callbacks. 
	evas_object_event_callback_add( parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_SHOW, _coverflow_show, wd );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_HIDE, _coverflow_hide, wd );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_MOVE, _coverflow_move, wd );
	evas_object_event_callback_add( obj, EVAS_CALLBACK_RESIZE, _coverflow_resize, wd );

	//calculate count of initial valid item.
	Evas_Coord w, h;
	evas_object_geometry_get( wd->parent, NULL, NULL, &w, &h );
	wd->valid_item_cnt = w / COVER_SIZE + 2 + CACHED_ITEM_COUNT * 2;

	return obj;

}




EAPI void elm_coverflow_clear( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return ;
	}

	Eina_List* elist;
	Elm_Coverflow_Item* item;

	EINA_LIST_FOREACH( wd->all_item_list, elist, item ) {
		_remove_item_obj( wd, item );
		wd->all_item_list = eina_list_remove( wd->all_item_list, item );
		free( item );
	}

	wd->valid_item_length = 0;

}



/*
EAPI void elm_coverflow_vertical_set( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );

}

EAPI void elm_coverflow_vertical_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );

}
*/


static Evas_Object* _TEMP_CREATE_ICON( Evas_Object* parent, int idx  )
{
	char file_path[ 256 ];
	sprintf( file_path, "/usr/share/beat_winset_test/icon/coverflow_img/%d_raw.png", idx+1  );
/*	
	Evas_Object* icon = elm_icon_add( parent );
	elm_icon_file_set( icon, file_path, NULL );
	elm_icon_scale_set( icon, 1, 1 );
	return icon; */

	Evas_Object* img = evas_object_image_add( evas_object_evas_get( parent ) );
	evas_object_image_load_size_set( img, 80, 80 );
	evas_object_image_fill_set( img, 0, 0, 80, 80 );
	evas_object_image_file_set( img, file_path, NULL );
	evas_object_image_smooth_scale_set( img, EINA_TRUE );

	return img;	
}

static void _selected_item_completion_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;
	elm_animator_del( wd->select_ani_data->animator );

	free( wd->select_ani_data );
	wd->select_ani_data = NULL;
	wd->selected_item = NULL;

	_update_items( wd, Shift_None, wd->stack_raise ); 

	evas_object_show( wd->base );
	evas_object_raise( wd->base );	

}


static void _selected_item_operation_cb( void* data, Elm_Animator* animator, const double key_frame )
{
	Widget_Data* wd = (Widget_Data*) data;
	
	Evas_Coord reflection_size = COVER_SIZE / 3;

	Elm_Coverflow_Item* item = wd->selected_item;

	Evas_Coord x = wd->select_ani_data->from_x + wd->select_ani_data->to_x * key_frame;
	Evas_Coord y = wd->select_ani_data->orig_pos_y;
	Evas_Coord z = wd->select_ani_data->from_z + wd->select_ani_data->to_z * key_frame;
	float rot_degree =
		wd->select_ani_data->from_rot + wd->select_ani_data->to_rot * key_frame;

	//Geometry 
	//ToDo: Check coord_set . 
	evas_map_point_coord_set( wd->original_map, 0, 
			    x, y, z );
	evas_map_point_coord_set( wd->original_map, 1, 
			    x + COVER_SIZE, y, z );
	evas_map_point_coord_set( wd->original_map, 2,
			    x + COVER_SIZE, y + COVER_SIZE, z );
	evas_map_point_coord_set( wd->original_map, 3,
			    x, y + COVER_SIZE, z );

	evas_map_point_coord_set( wd->reflection_map, 0, 
			    x, y + COVER_SIZE, z );
	evas_map_point_coord_set( wd->reflection_map, 1, 
			    x + COVER_SIZE, y + COVER_SIZE, z );
	evas_map_point_coord_set( wd->reflection_map, 2,
			    x + COVER_SIZE, y + COVER_SIZE + reflection_size, z );
	evas_map_point_coord_set( wd->reflection_map, 3,
			    x, y + COVER_SIZE + reflection_size, z );

	//Rotation
	evas_map_util_3d_rotate( wd->original_map, 0, rot_degree, 0, 
			x + HALF_COVER_SIZE, y + HALF_COVER_SIZE, z );

	evas_map_util_3d_rotate( wd->reflection_map, 0, rot_degree, 0,
			x + HALF_COVER_SIZE, y + HALF_COVER_SIZE, z );

	//Perspective 
	evas_map_util_3d_perspective( wd->original_map, 
				      x + HALF_COVER_SIZE, 
				      y + HALF_COVER_SIZE, 0, 750 );

	evas_map_util_3d_perspective( wd->reflection_map, 
			              x + HALF_COVER_SIZE, 
			              y + HALF_COVER_SIZE, 0, 750 );

	evas_object_map_set( item->base, wd->original_map );
	evas_object_map_set( item->reflection, wd->reflection_map );
	
}





static void _unselect_item( Widget_Data* wd, Elm_Coverflow_Item* item )
{
	Select_Ani_Data* ani_data = wd->select_ani_data;

	ani_data->from_x = ani_data->from_x + ani_data->to_x;
	ani_data->to_x = ani_data->orig_pos_x - ani_data->from_x;
	ani_data->from_z = SELECTED_ITEM_POS_Z;
	ani_data->to_z = -SELECTED_ITEM_POS_Z;
	ani_data->from_rot = 0;
	ani_data->to_rot = wd->rot_ani_data.cur;

	elm_animator_completion_callback_set( ani_data->animator, _selected_item_completion_cb, wd );
	elm_animator_animate( ani_data->animator );
}






static void _zoom_in_completion_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;

	elm_animator_del( wd->base_ani_data.animator );
	wd->base_ani_data.animator = NULL;


}




static void _zoom_in_operation_cb( void* data, Elm_Animator* animator, const double key_frame )
{
	Widget_Data* wd = (Widget_Data*) data;
	wd->cur_pos.z = ZOOM_OUT_DISTANCE -ZOOM_OUT_DISTANCE * key_frame; 
	_render_visible_items( wd );
}


inline static void _zoom_in( Widget_Data* wd ) 
{
	wd->base_ani_data.animator = elm_animator_add( wd->obj );
	elm_animator_duration_set( wd->base_ani_data.animator, ZOOM_OUT_DURATION );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data. animator,
			                     _zoom_in_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator,
			                     _zoom_in_completion_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );
}


static void _zoom_in_ev( void* data, Evas* e, Evas_Object* obj, void* event_info )
{
	Widget_Data* wd = (Widget_Data*) data;
	
	evas_object_del( obj );
	
	_zoom_in( wd );	
	_unselect_item( wd, wd->selected_item );
	
}


static void _zoom_out_completion_cb( void* data )
{
	Widget_Data* wd = (Widget_Data*) data;

	elm_animator_del( wd->base_ani_data.animator );

	//Create Zoom In Event Delegator
	Evas_Coord x, y, w, h;
	evas_object_geometry_get( wd->base, &x, &y, &w, &h );
	Evas_Object* rect = evas_object_rectangle_add( evas_object_evas_get( wd->obj ) );
	evas_object_color_set( rect, 0, 0, 0, 0 );
	evas_object_move( rect, x, y );
	evas_object_resize( rect, w, h );
	evas_object_show( rect );
	evas_object_event_callback_add( rect, EVAS_CALLBACK_MOUSE_UP, _zoom_in_ev, wd );

}

static void _zoom_out_operation_cb( void* data, Elm_Animator* animator, const double key_frame )
{
	Widget_Data* wd = (Widget_Data*) data;
	wd->cur_pos.z = ZOOM_OUT_DISTANCE * key_frame; 

	_render_visible_items( wd );
}




inline static void _zoom_out( Widget_Data* wd )
{
	wd->base_ani_data.animator = elm_animator_add( wd->obj );
	elm_animator_duration_set( wd->base_ani_data.animator, ZOOM_OUT_DURATION );
	elm_animator_curve_style_set( wd->base_ani_data.animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( wd->base_ani_data.animator, 
			                     _zoom_out_operation_cb, wd );
	elm_animator_completion_callback_set( wd->base_ani_data.animator, 
			                 _zoom_out_completion_cb, wd );
	elm_animator_animate( wd->base_ani_data.animator );

}


static void _select_item( Widget_Data* wd, Elm_Coverflow_Item* item )
{
	wd->selected_item = item;
	evas_object_raise( item->base );
	evas_object_raise( item->reflection );

	Select_Ani_Data* ani_data = calloc( 1, sizeof( Select_Ani_Data ) );

   	evas_object_geometry_get( item->base, &ani_data->orig_pos_x, &ani_data->orig_pos_y, NULL, NULL );
	ani_data->from_x = ani_data->orig_pos_x;

	Evas_Coord w;
	evas_object_geometry_get( wd->base, NULL, NULL, &w, NULL );
	ani_data->to_x = w / 2 - COVER_SIZE / 2 - ani_data->from_x;

	ani_data->from_z = 0;
	ani_data->to_z = SELECTED_ITEM_POS_Z;
	ani_data->from_rot = wd->rot_ani_data.cur;
	ani_data->to_rot = -wd->rot_ani_data.cur;

	ani_data->animator = elm_animator_add( wd->obj );
	elm_animator_duration_set( ani_data->animator, ZOOM_OUT_DURATION );
	elm_animator_curve_style_set( ani_data->animator, ELM_ANIMATOR_CURVE_OUT );
	elm_animator_operation_callback_set( ani_data->animator, _selected_item_operation_cb, wd );
	elm_animator_animate( ani_data->animator );

	wd->select_ani_data = ani_data;
}



static void _item_clicked_ev( void* data, Evas* e, Evas_Object* obj, void* event_info )
{
	Elm_Coverflow_Item* item = (Elm_Coverflow_Item*) data;

	Widget_Data* wd = item->wd;

	if( wd->move_on == EINA_TRUE ) {
		return ;
	}

	if( evas_object_visible_get( wd->base ) == EINA_FALSE ) {
		return;
	}
		
	evas_object_hide( wd->base );

	if( wd->base_ani_data.animator ) {
		elm_animator_stop( wd->base_ani_data.animator );
		elm_animator_del( wd->base_ani_data.animator );
	}

	_zoom_out( wd );
	_select_item( wd, item );

}


static void _create_item_obj( Widget_Data* wd, Elm_Coverflow_Item* item )
{
	if( item == NULL ) {
		return ;
	}
	
	//Original Object
	item->base = _TEMP_CREATE_ICON( wd->parent, (int) item->data );
	_init_item_obj( item->base );
//	evas_object_smart_callback_add( item->base, "clicked", _item_clicked_ev, item );
	evas_object_event_callback_add( item->base, EVAS_CALLBACK_MOUSE_UP, _item_clicked_ev, item );

	item->wd = wd;

	//Reflection Object
	item->reflection = _TEMP_CREATE_ICON( wd->parent, (int) item->data );
	_init_item_obj( item->reflection );

	wd->valid_item_length += wd->dist_between_items;
}


inline static void _init_item_obj( Evas_Object* obj )
{
	evas_object_size_hint_weight_set( obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_align_set( obj, EVAS_HINT_FILL, EVAS_HINT_FILL );
	evas_object_resize( obj, COVER_SIZE, COVER_SIZE );
	evas_object_map_enable_set( obj, EINA_TRUE );

}



EAPI Elm_Coverflow_Item* elm_coverflow_item_append( 
		Evas_Object* obj,
		Elm_Coverflow_Item_Class* cic,
		void* data,
		void (*func)(void*data, Evas_Object* obj, void* event_info), 
		void* func_data )
{
	if( cic == NULL ) {
		return NULL;
	}
	
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );

	if( wd == NULL ) {
		return NULL;
	}

	Elm_Coverflow_Item* item = calloc( 1, sizeof( Elm_Coverflow_Item ) );

	if( item == NULL ) {
		return NULL;
	}

	item->cic.CoverflowItemGenFunc = cic->CoverflowItemGenFunc;
	item->cic.CoverflowItemDelFunc = cic->CoverflowItemDelFunc;
	item->data = data;
	item->func = func;
	item->func_data = func_data;
	item->idx = eina_list_count( wd->all_item_list );

	wd->all_item_list = eina_list_append( wd->all_item_list, item ); 

	//If the item count is less than valid item count 
	if( eina_list_count( wd->all_item_list ) <= wd->valid_item_cnt ) {

		_create_item_obj( wd, item );
		
		//For First item. 
		wd->valid_item_list = wd->all_item_list;

		//Exactly It does not need to update_items.
		if( wd->show == EINA_TRUE ) {
			_update_items( wd, Shift_Left, EINA_FALSE );
		}

	}

	return item;
}



/*
EAPI Elm_Coverflow_Item* elm_coverflow_item_prepend( Evas_Object* obj, Evas_Object* item )
{
	Elm_Coverflow_Item* item;
	fprintf( stderr, "Sorry, It does not support yet!\n" );

	return item;
}



EAPI Elm_Coverflow_Item* elm_coverflow_item_insert_before( Evas_Object* obj, Elm_Coverflow_Item* before_item, Evas_Object* item ) {
	Elm_Coverflow_Item* item;
	fprintf( stderr, "Sorry, It does not support yet!\n" );

	return item;
}


EAPI Elm_Coverflow_Item* elm_coverflow_item_insert_after( Evas_Object* obj, Elm_Coverflow_Item* after_item, Evas_Object* item ) {

	Elm_Coverflow_Item* item;
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	
	return item;
}


EAPI Elm_Coverflow_Item* elm_coverflow_first_item_get( Evas_Object* obj )
{
fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}


EAPI Elm_Coverflow_Item* elm_coverflow_last_item_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );

	return NULL;
}



EAPI void elm_coverflow_item_del( Evas_Object* obj, Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );

	if( --wd->item_cnt == 0 ) {
		wd->visible_item_head = NULL;
	}

}


EAPI unsigned int elm_coverflow_item_count_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return 0;
}


EAPI void elm_coverflow_item_disabled_set( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}

EAPI Eina_Bool elm_coverflow_item_disabled_get( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return EINA_FALSE;
}


EAPI const Eina_List* elm_coverflow_items_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}




EAPI void elm_coverflow_multiselect_set( Evas_Object* obj, Eina_Bool multi )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}

EAPI Eina_Bool elm_coverflow_multiselect_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );

	return EINA_FALSE;
}

EAPI Eina_List* elm_coverflow_selected_items_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}


EAPI unsigned int elm_coverflow_selected_items_count_get( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return 0;
}


EAPI void elm_coverflow_multiselected_cancel( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}


EAPI void elm_coverflow_multiselected_showup( Evas_Object* obj )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}



EAPI Elm_Coverflow_Item* elm_coverflow_item_next_get( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}


EAPI Elm_Coverflow_Item* elm_coverflow_item_prev_get( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}


EAPI void elm_coverflow_item_bring_in( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}


EAPI void elm_coverflow_item_top_bring_in( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}


EAPI void elm_coverflow_item_middle_bring_in( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
}


EAPI Evas_Object* elm_coverflow_item_object_get( Elm_Coverflow_Item* item )
{
	fprintf( stderr, "Sorry, It does not support yet!\n" );
	return NULL;
}

*/
