#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Ctxpopup Ctxpopup
 * @ingroup Elementary
 *
 * Contextual popup.
 */

typedef struct _Widget_Data Widget_Data;
typedef enum {BOTTOM_ARROW, RIGHT_ARROW, LEFT_ARROW, TOP_ARROW, NONE_ARROW } Arrow_Direction;

struct _Ctxpopup_Item
{
    Evas_Object* ctxpopup;
    Evas_Object* obj;
    const char* label;
    Evas_Object* content;
    void (*func) (void* data, Evas_Object* obj, void* event_info);
	const void* data;
	Eina_Bool disabled : 1;
	Eina_Bool separator : 1;
};

struct _Widget_Data
{
	Evas_Object* parent;
	Evas_Object* location;
	Evas_Object* hover;
	Evas_Object* box;
	Evas_Object* arrow;
	Evas_Object* scroller;
	Eina_List* items;
	Evas_Coord x, y;
	Arrow_Direction last_arrow_dir;
	Evas_Coord max_width_size;
	Evas_Coord max_height_size;
	unsigned int expand_cnt;
	Eina_Bool scroller_disabled : 1;
	Eina_Bool horizontal : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object* obj);
static void _del_pre_hook(Evas_Object* obj);
static void _theme_hook(Evas_Object* obj);
static void _sizing_eval(Evas_Object *obj);
static void _hover_clicked_cb(void* data, Evas_Object* obj, void* event_info);
static void _parent_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _ctxpopup_show(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _ctxpopup_hide(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _ctxpopup_move(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _ctxpopup_scroller_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static void _item_obj_create(Elm_Ctxpopup_Item* item);
static void _item_sizing_eval(Elm_Ctxpopup_Item* item);
static void _ctxpopup_item_select(void* data, Evas_Object* obj, const char* emission, const char* source);
static void _separator_obj_add(Evas_Object* obj);
static void _separator_obj_del(Widget_Data* wd, Elm_Ctxpopup_Item* remove_item);
static void _changed_size_hints(void* data, Evas* evas, Evas_Object* obj, void* event_info);
static Arrow_Direction  _calc_best_geometry(Widget_Data* wd,  Evas_Coord_Rectangle* rect);
static void _arrow_obj_add(Evas_Object* obj, const char* group_name);
static void _update_arrow_obj(Evas_Object* obj, Arrow_Direction arrow_dir);
static Evas_Coord  _adjust_arrow_pos_x(Widget_Data* wd);
static Evas_Coord  _adjust_arrow_pos_y(Widget_Data* wd);
static void _scale_shrinked_set(Elm_Ctxpopup_Item* item);
static void _item_scale_shrinked_set(Widget_Data* wd, Elm_Ctxpopup_Item* add_item);
static void _item_scale_normal_set(Widget_Data* wd);
static void _shift_geometry_by_arrow(Evas_Object* arrow, Arrow_Direction arrow_dir, Evas_Coord_Rectangle* rect);

static void 
_changed_size_hints(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	_sizing_eval( data );
}

static void 
_separator_obj_del(Widget_Data* wd, Elm_Ctxpopup_Item* remove_item)
{
	Eina_List* elist;
	Elm_Ctxpopup_Item* separator;

	if((!remove_item) || (!wd)) return;
	elist = wd->items;
	Eina_List* cur_list = eina_list_data_find_list(elist, remove_item);
	if(!cur_list) return ;
	Eina_List* prev_list = eina_list_prev(cur_list);
	if(!prev_list) return ;
	separator = (Elm_Ctxpopup_Item*) eina_list_data_get( prev_list );
	if(!separator) return ;
	wd->items = eina_list_remove(wd->items, separator);
	evas_object_del(separator->obj);
	free(separator);
}

static void 
_separator_obj_add(Evas_Object* obj)
{
	Elm_Ctxpopup_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return ;
	if(eina_list_count( wd->items ) == 0) return;

	item = calloc(1, sizeof(Elm_Ctxpopup_Item));
	if(!item) return ;

	item->obj = edje_object_add(evas_object_evas_get(wd->location));
	if(!item->obj) 
	{
		free( item ); 
		return ;
	}

	evas_object_size_hint_weight_set(item->obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(item->obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	_elm_theme_object_set(obj, item->obj, "ctxpopup", "seperator", elm_widget_style_get(obj));

	if(wd->horizontal) 
		edje_object_signal_emit(item->obj, "elm,state,horizontal", "elm");
	else 
		edje_object_signal_emit(item->obj, "elm,state,vertical", "elm");

	evas_object_show(item->obj);
	item->separator = EINA_TRUE;
	elm_box_pack_end(wd->box, item->obj);
	wd->items = eina_list_append(wd->items, item);
}

static void 
_scale_shrinked_set(Elm_Ctxpopup_Item* item)
{
	if(!item) return;

	if(!item->disabled) 
		edje_object_signal_emit(item->obj, "elm,state,shrinked", "elm");
	else 
		edje_object_signal_emit(item->obj, "elm,state,shrinked_disabled", "elm");
}

static void 
_item_scale_normal_set(Widget_Data* wd)
{
	int item_count;
	Eina_List* elist;
	Elm_Ctxpopup_Item* item;

	if(!wd) return;

	item_count = eina_list_count(wd->items);
	item_count -= item_count/2;

	if(item_count != wd->expand_cnt) return;

	EINA_LIST_FOREACH(wd->items, elist, item) 
	{
		if(!item->disabled) 
			edje_object_signal_emit(item->obj, "elm,state,enabled", "elm");
		else 
			edje_object_signal_emit(item->obj, "elm,state,disabled", "elm");
	}
}



static void 
_item_scale_shrinked_set(Widget_Data* wd, Elm_Ctxpopup_Item* add_item)
{
	int item_count;
	Eina_List* elist;
	Elm_Ctxpopup_Item* item;

	if(!wd) return;
	if(wd->horizontal) return;
	item_count = eina_list_count(wd->items);
	item_count -= item_count/2;

	if(item_count > wd->expand_cnt+1) 
	{
		_scale_shrinked_set( add_item );
	}
	else if( item_count == wd->expand_cnt + 1 )
	{
		EINA_LIST_FOREACH( wd->items, elist, item ) 
		{
			_scale_shrinked_set( item );
		}
	}
}

static void 
_item_sizing_eval( Elm_Ctxpopup_Item* item )
{
	Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;

	if(!item) return ;

	if(!item->separator) 
		elm_coords_finger_size_adjust( 1, &min_w, 1, &min_h );

	edje_object_size_min_restricted_calc( item->obj, &min_w, &min_h, min_w, min_h );

	if(!item->separator) 
		elm_coords_finger_size_adjust( 1, &min_w, 1, &min_h );

	evas_object_size_hint_min_set( item->obj, min_w, min_h );
	evas_object_size_hint_max_set( item->obj, max_w, max_h );
}

#define WORLD_PARENT_W (parent_x+parent_w)
#define WORLD_PARENT_H (parent_y+parent_h)
#define BOX_HALF_W (box_w/2)
#define BOX_HALF_H (box_h/2)

#define ADJUST_POS_X() do {   \
		      x  = wd->x - ( box_w/2 ) ; \
                      if( x < x1 ) {   \
	   		      x = x1;  \
		      }else if( x + box_w > WORLD_PARENT_W) { \
		   	   x = WORLD_PARENT_W - box_w; \
   	              } \
	       }while( 0 )

#define ADJUST_POS_Y() do { \
		y = wd->y - ( box_h/2 ); \
		if( y < y1) {    \
			y = y1;   \
		}else if( y + box_h > WORLD_PARENT_H )     \
		{     \
			y = WORLD_PARENT_H - box_h;     \
		}    \
	}while( 0 )

static Arrow_Direction 
_calc_best_geometry(Widget_Data* wd, Evas_Coord_Rectangle* rect)
{
	Evas_Coord x, y;
	Evas_Coord parent_x, parent_y, parent_w, parent_h;
	Evas_Coord box_w, box_h;
	Arrow_Direction arrow_dir;
	Evas_Coord x1, x2;
	Evas_Coord y1, y2;
	int available_direction[4] = { 1, 1, 1, 1 };
	int idx = 0;
	Evas_Coord orig_box_w;
	Evas_Coord finger_size;
	Evas_Coord arrow_size_w;
	Evas_Coord arrow_size_h;

	if((!wd) || (!rect)) return arrow_dir;
	x = wd->x;
	y = wd->y;
	evas_object_geometry_get(wd->parent, &parent_x, &parent_y, &parent_w, &parent_h);
	evas_object_geometry_get(wd->box, NULL, NULL, &box_w, &box_h);
	if(box_h > wd->max_height_size) box_h = wd->max_height_size;

	orig_box_w = box_w;

	if(box_w > wd->max_width_size) box_w = wd->max_width_size;

	finger_size = elm_finger_size_get();
	edje_object_size_min_calc(wd->arrow, &arrow_size_w, &arrow_size_h);

	//Phase 1: Define x, y Segments and find invalidated direction.
	//Left
	x1 = x - box_w;
	if (x1 - arrow_size_w - finger_size < parent_x) 
	{
		x1 = parent_x;
		available_direction[RIGHT_ARROW]=0;
	}

	//Right
	x2 = x + box_w;
	if(x2 + arrow_size_w + finger_size  > WORLD_PARENT_W) 
	{
		x2 = WORLD_PARENT_W - box_w;
		available_direction[LEFT_ARROW] = 0;
	}

	//Top
	y1 = y - box_h;
	if(y1 - arrow_size_h - finger_size  < parent_y) 
	{
		y1 = parent_y;
		available_direction[BOTTOM_ARROW] = 0;
	}
	//Bottom
	y2 = y + box_h;
	if(y2 + arrow_size_h  + finger_size > WORLD_PARENT_H) 
	{
		y2 = WORLD_PARENT_H - box_h;
		available_direction[TOP_ARROW] = 0;
	}

//ADDITIONAL OPTION: Phase 2: Determine Direction Priority ?

//Phase 3: adjust geometry information.
	for(idx = 0; idx < 4; ++idx) 
	{
		if( available_direction[ idx ] == 0 ) continue;

		//Find the Nearest point to center of box.
		switch(idx) 
		{
			case BOTTOM_ARROW:
					ADJUST_POS_X();
					y = wd->y - box_h - finger_size;
					arrow_dir = BOTTOM_ARROW;
				break;
			case RIGHT_ARROW:
					ADJUST_POS_Y();
					x = wd->x - box_w - finger_size;
					arrow_dir = RIGHT_ARROW;
				break;
			case LEFT_ARROW:
					ADJUST_POS_Y();
					x = wd->x + finger_size;
					arrow_dir = LEFT_ARROW;
				break;
			case TOP_ARROW:
					ADJUST_POS_X();
					y = wd->y + finger_size;
					arrow_dir = TOP_ARROW;
				break;
			default:
				fprintf( stderr, "Not Enough space to show contextual popup!! \n" );
		}
		break;
	}
	rect->x = x;
	rect->y = y;
	rect->w = orig_box_w;
	rect->h = 0;
	return arrow_dir;
}

static Evas_Coord 
_adjust_arrow_pos_x(Widget_Data* wd)
{
	if(!wd) return 0;

	Evas_Coord parent_x, parent_w;
	Evas_Coord arrow_x;
	Evas_Coord arrow_size;
	Evas_Coord half_arrow_size;

	edje_object_size_max_get( wd->arrow, &arrow_size, NULL );
	half_arrow_size = arrow_size * 0.5;
	evas_object_geometry_get( wd->parent, &parent_x, NULL, &parent_w, NULL );
	arrow_x = wd->x;

	if( arrow_x - half_arrow_size < parent_x ) 
		arrow_x = parent_x + half_arrow_size;
	else if( wd->x + arrow_size > WORLD_PARENT_W ) 
		arrow_x = WORLD_PARENT_W - half_arrow_size;

	return arrow_x;

}

static Evas_Coord  
_adjust_arrow_pos_y(Widget_Data* wd)
{
	if(!wd) return 0;

	Evas_Coord parent_y, parent_h;
	Evas_Coord arrow_y;
	Evas_Coord arrow_size;
	Evas_Coord half_arrow_size;

	edje_object_size_max_get( wd->arrow, &arrow_size, NULL );
	half_arrow_size = arrow_size * 0.5;
	
	evas_object_geometry_get( wd->parent, NULL, &parent_y, NULL, &parent_h );

	arrow_y = wd->y;

	if( arrow_y - half_arrow_size < parent_y ) {
		arrow_y = parent_y + half_arrow_size;
	}else if( wd->y + half_arrow_size > WORLD_PARENT_H ) {
		arrow_y = WORLD_PARENT_H - half_arrow_size;
	}

	return arrow_y;
}

static void 
_update_arrow_obj(Evas_Object* obj, Arrow_Direction arrow_dir)
{
	Evas_Coord arrow_x, arrow_y;
	Widget_Data* wd = elm_widget_data_get(obj);

	if(!wd) return;

	arrow_x = wd->x;
	arrow_y = wd->y;

	switch(arrow_dir) 
	{
		case LEFT_ARROW:
		{
			if( wd->last_arrow_dir != LEFT_ARROW ) 
			{
				_arrow_obj_add( obj, "left_arrow" );
				_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "left_arrow", elm_widget_style_get(obj));
				wd->last_arrow_dir = LEFT_ARROW;
			}
			arrow_y = _adjust_arrow_pos_y(wd) ;
			arrow_x += elm_finger_size_get();
			break;
		}
		case RIGHT_ARROW:
		{
			if(wd->last_arrow_dir != RIGHT_ARROW) {
				_arrow_obj_add( obj, "right_arrow" );
				_elm_theme_object_set( obj, wd->arrow, "ctxpopup", "right_arrow", elm_widget_style_get( obj ) );
				wd->last_arrow_dir = RIGHT_ARROW;
			}
			arrow_y = _adjust_arrow_pos_y(wd);
			arrow_y = _adjust_arrow_pos_y(wd);
			arrow_x -= elm_finger_size_get();
			break;
		}
		case TOP_ARROW:
		{
			if(wd->last_arrow_dir != TOP_ARROW) {
				_arrow_obj_add(obj, "top_arrow"); 
				_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "top_arrow", elm_widget_style_get(obj));
				wd->last_arrow_dir = TOP_ARROW;
			}
			arrow_x = _adjust_arrow_pos_x(wd);
			arrow_y += elm_finger_size_get();
			break;
		}
		case BOTTOM_ARROW:
		{
			if(wd->last_arrow_dir != BOTTOM_ARROW) { 
				_arrow_obj_add(obj, "bottom_arrow"); 
				_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "bottom_arrow", elm_widget_style_get(obj));
				wd->last_arrow_dir = BOTTOM_ARROW;
			}
			arrow_x = _adjust_arrow_pos_x(wd);
			arrow_y -= elm_finger_size_get();
			break;
		}
		default:
			break;
	}

	evas_object_move(wd->arrow, arrow_x, arrow_y);
}




static void 
_sizing_eval(Evas_Object* obj)
{
	Widget_Data* wd;
	Eina_List* elist;
	Elm_Ctxpopup_Item* item;
	Evas_Coord_Rectangle rect;

	wd = (Widget_Data*) elm_widget_data_get(obj);

	if((!wd) || (!wd->parent)) return; 

	EINA_LIST_FOREACH(wd->items, elist, item) 
	{
		_item_sizing_eval(item);
	}

	Arrow_Direction arrow_dir = _calc_best_geometry(wd, &rect);
	_update_arrow_obj(obj,	arrow_dir);
	_shift_geometry_by_arrow(wd->arrow, arrow_dir, &rect);

	evas_object_move(wd->location, rect.x, rect.y);
	evas_object_resize(wd->location, rect.w, rect.h);
	evas_object_move(wd->hover, rect.x, rect.y);
	evas_object_resize(wd->hover, rect.w, rect.h);

}

static void 
_shift_geometry_by_arrow(Evas_Object* arrow, Arrow_Direction arrow_dir, Evas_Coord_Rectangle* rect)
{
	Evas_Coord arrow_size_w, arrow_size_h;	
	edje_object_size_min_calc(arrow, &arrow_size_w, &arrow_size_h);

	switch(arrow_dir) {
		case LEFT_ARROW:
			rect->x += arrow_size_w;
			break;
		case RIGHT_ARROW:
			rect->x -= arrow_size_w;
			break;
		case TOP_ARROW:
			rect->y += arrow_size_h;
			break;
		case BOTTOM_ARROW:
			rect->y -= arrow_size_h;
			break;
	}
}

static void 
_del_pre_hook(Evas_Object* obj)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return;
	evas_object_event_callback_del_full(wd->parent,	EVAS_CALLBACK_RESIZE, _parent_resize, obj);
}


static void 
_del_hook( Evas_Object* obj )
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return;
	elm_ctxpopup_clear(obj);
	free( wd );
}



static void 
_theme_hook(Evas_Object* obj)
{
	Eina_List* elist;
	Eina_List* elist_child;
	Eina_List* elist_temp;
	Elm_Ctxpopup_Item* item;
	int item_count;

	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd)	return;

	item_count = eina_list_count(wd->items);
	item_count -= item_count /2;
	elist = eina_list_append(elist, wd->items);

	EINA_LIST_FOREACH(elist, elist, elist_child) 
	{
		EINA_LIST_FOREACH( elist_child, elist_temp, item ) 
		{
			if(item->separator) 
			{
				_elm_theme_object_set( obj, item->obj, "ctxpopup", "separator",	elm_widget_style_get(obj));
				if( wd->horizontal) 
					edje_object_signal_emit(item->obj, "elm,state,horizontal", "elm");
				else 
					edje_object_signal_emit(item->obj, "elm,state,vertical", "elm");
			}else 
			{
				_elm_theme_object_set(obj, item->obj, "ctxpopup", "item", elm_widget_style_get(obj));

				edje_object_part_text_set(item->obj, "elm.text", item->label);
				
				if(item->label)
				{
					edje_object_part_text_set(item->obj, "elm.text", item->label);
				}else if(item->content)
				{
					edje_object_signal_emit(item->obj, "elm,state,enable_icon", "elm");
				}

				if(item->disabled) 
				{
					if(item_count > wd->expand_cnt) 
						edje_object_signal_emit(item->obj, "elm,state,shrinked_disabled", "elm");
					else 
						edje_object_signal_emit( item->obj, "elm,state,disabled","elm" );
				}else 
				{
					if(item_count > wd->expand_cnt) 
						edje_object_signal_emit( item->obj, "elm,state,shrinked", "elm" );
					else 
						edje_object_signal_emit( item->obj, "elm,state,enabled", "elm" );
				}
			}
			edje_object_message_signal_process( item->obj );
		}
	}

	if(wd->horizontal) 
		elm_object_style_set( wd->scroller, "ctxpopup_hbar");
	else 
		elm_object_style_set( wd->scroller, "ctxpopup_vbar");

	_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow", elm_widget_style_get(obj));

	_sizing_eval(obj);
}

static void 
_hover_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	evas_object_hide(data);
	evas_object_smart_callback_call(data, "ctxpopup,hide", NULL);
}

static void 
_parent_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	_sizing_eval(data);
}

static void 
_ctxpopup_show(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;
	if(eina_list_count(wd->items) < 1) return;
	evas_object_show(wd->arrow);
	evas_object_show(wd->hover);
}

static void 
_ctxpopup_hide(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;
	evas_object_hide(wd->arrow);
	evas_object_hide(wd->hover);
}

static void 
_ctxpopup_scroller_resize(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	_sizing_eval(data);
}

static void 
_ctxpopup_move(void* data, Evas* evas, Evas_Object* obj, void* event_info)
{
	Evas_Coord x, y, w, h;
	Widget_Data* wd = (Widget_Data*) data;
	if(!wd) return;
	evas_object_geometry_get( obj, &x, &y, &w, &h );
	wd->x = x;
	wd->y = y;
	_sizing_eval(obj);
}

static void 
_ctxpopup_item_select(void* data, Evas_Object* obj,	const char* emission, const char* source )
{
	Elm_Ctxpopup_Item* item = (Elm_Ctxpopup_Item*) data;
	if(!item) return;
	if(item->disabled)	return;
	if(item->func) 	item->func((void*) (item->data), item->ctxpopup, item);
}

static void 
_arrow_obj_add(Evas_Object* obj, const char* group_name)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return;

	if(wd->arrow) 
	{
		elm_widget_sub_object_del(obj, wd->arrow);
		evas_object_del(wd->arrow);
		wd->arrow = NULL;
	}
	wd->arrow = edje_object_add(evas_object_evas_get( wd->location));
	elm_widget_sub_object_add(obj, wd->arrow);
	evas_object_size_hint_weight_set(wd->arrow, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	
	if(evas_object_visible_get(obj)) evas_object_show(wd->arrow);
}

static void 
_item_obj_create(Elm_Ctxpopup_Item* item)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(item->ctxpopup);
	if(!wd) return;
	item->obj = edje_object_add(evas_object_evas_get(wd->location));
	evas_object_size_hint_weight_set(item->obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(item->obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	_elm_theme_object_set(item->ctxpopup, item->obj, "ctxpopup", "item", elm_widget_style_get( item->ctxpopup));
	edje_object_signal_callback_add(item->obj,"elm,action,click","",_ctxpopup_item_select,item);
	evas_object_show(item->obj);
}


/**
 * Get the icon object for the given item.
 *
 * @param item 	Ctxpopup item
 * @return 		Icon object or NULL if the item does not have icon
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object* 
elm_ctxpopup_item_icon_get( Elm_Ctxpopup_Item* item )
{
	if(!item) return NULL;
	return item->content;
}

/**
 * Disable or Enable the scroller for contextual popup.
 *
 * @param obj 		Ctxpopup object
 * @param disabled  disable or enable
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_scroller_disabled_set(Evas_Object* obj, Eina_Bool disabled)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
	if(!wd) return;
	if(wd->scroller_disabled == disabled) return;

	if(disabled)
		elm_object_scroll_freeze_push( wd->scroller );
	else 
		elm_object_scroll_freeze_pop( wd->scroller );

	wd->scroller_disabled = disabled;
}

/**
 * Get the label for the given item.
 *
 * @param item 	 	Ctxpopup item
 * @return 		Label or NULL if the item does not have label
 *
 * @ingroup Ctxpopup
 *
 */
EAPI const char* 
elm_ctxpopup_item_label_get(Elm_Ctxpopup_Item* item)
{
	if(!item) return NULL;
	return item->label;
}


/**
 * Add a new ctxpopup object to the parent.
 *
 * @param parent 	Parent object
 * @return 		New object or NULL if it cannot be created
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object* 
elm_ctxpopup_add(Evas_Object* parent)
{
	Evas_Object* obj;
	Evas* e;
	Widget_Data* wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	ELM_SET_WIDTYPE(widtype, "ctxpopup");
	elm_widget_type_set(obj, "ctxpopup");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	wd->location = elm_icon_add(obj);
	wd->parent = parent;
	wd->last_arrow_dir = NONE_ARROW;
	//make it flexible?
	wd->expand_cnt = 4;
	wd->max_width_size = 460;
	wd->max_height_size = 360;

	//Scroller
	wd->scroller = elm_scroller_add(obj);
	wd->scroller_disabled = EINA_FALSE;
	elm_object_style_set( wd->scroller, "ctxpopup_vbar");
	elm_scroller_content_min_limit( wd->scroller, EINA_TRUE, EINA_TRUE );
	elm_scroller_policy_set( wd->scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_ON );
	evas_object_size_hint_weight_set( wd->scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND );
	evas_object_size_hint_align_set( wd->scroller, EVAS_HINT_FILL, EVAS_HINT_FILL );
	elm_scroller_bounce_set( wd->scroller, EINA_FALSE, EINA_TRUE );
	evas_object_show(wd->scroller);

	//Box
	wd->box = elm_box_add(obj);
	evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(wd->box);
	elm_scroller_content_set(wd->scroller, wd->box);

	//Hoversel
	wd->hover = elm_hover_add(obj);
	elm_hover_parent_set(wd->hover, parent);
	elm_hover_target_set(wd->hover, wd->location);
	elm_object_style_set(wd->hover, "ctxpopup");
	evas_object_size_hint_weight_set( wd->hover, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set( wd->hover, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add( wd->hover, "clicked", _hover_clicked_cb, obj);
	elm_hover_content_set(wd->hover,elm_hover_best_content_location_get(wd->hover, ELM_HOVER_AXIS_VERTICAL),wd->scroller);

	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _ctxpopup_show, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _ctxpopup_hide, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _ctxpopup_move, wd);
	evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE, _ctxpopup_scroller_resize, obj);

	_sizing_eval(obj);
	return obj;
}

/**
 * Clear all items in given ctxpopup object.
 *
 * @param obj 		Ctxpopup object
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_clear(Evas_Object* obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Eina_List* elist;
	Elm_Ctxpopup_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
	if(!wd) return;

	EINA_LIST_FOREACH(wd->items, elist, item) 
	{
		if(item->label) eina_stringshare_del(item->label);
		if(item->content) evas_object_del(item->content);
		wd->items = eina_list_remove(wd->items, item);
		free(item);
	}
	evas_object_hide(wd->arrow);
	evas_object_hide(wd->hover);
}

/**
 * Change the mode to horizontal or vertical.
 *
 * @param obj   	Ctxpopup object
 * @param horizontal 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_horizontal_set(Evas_Object* obj, Eina_Bool horizontal)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	ELM_CHECK_WIDTYPE(obj, widtype);
	if(!wd) return;

	if(wd->horizontal == horizontal) return;
	wd->horizontal = horizontal;
	if(!horizontal) 
	{
		elm_object_style_set(wd->scroller, "ctxpopup_vbar");
		elm_box_horizontal_set(wd->box, EINA_FALSE);
		elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
		elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_ON, ELM_SCROLLER_POLICY_OFF);
	}else 
	{
		elm_object_style_set(wd->scroller, "ctxpopup_hbar");
		elm_box_horizontal_set(wd->box, EINA_TRUE);
		elm_scroller_bounce_set(wd->scroller, EINA_TRUE, EINA_FALSE);
		elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_ON, ELM_SCROLLER_POLICY_OFF);
	}
}

/**
 * Get the value of current horizontal mode.
 *
 * @param obj 	 	Ctxpopup object
 * @return 	 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode.
 *
 * @ingroup Ctxpopup
 */
EAPI Eina_Bool 
elm_ctxpopup_horizontal_get( Evas_Object* obj )
{
	ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;

	Widget_Data* wd = ( Widget_Data* ) elm_widget_data_get( obj );
	if(!wd) return EINA_FALSE;


	if( wd == NULL ) {
		return EINA_FALSE;
	}

	return wd->horizontal;
}

/**
 * reset the icon on the given item. This function is only for icon item.
 *
 * @param obj 	 	Ctxpopup item
 * @param icon		Icon object to be set
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_item_icon_set( Elm_Ctxpopup_Item* item, Evas_Object* icon )
{
	Widget_Data* wd;

	if((!item) || (!icon)) return;
	if(item->label) 
	{ 
		fprintf( stderr, "Cannot add icon in label item! - %p\n", item );
		return ;
	}
	wd = (Widget_Data*) elm_widget_data_get(item->ctxpopup);
	if(!wd) return;
	if(item->content == icon) return;
	if(item->content)  {
			elm_widget_sub_object_del(item->obj, item->content);
	}
	item->content = icon;
	elm_icon_scale_set(icon, EINA_TRUE, EINA_TRUE);
	edje_object_part_swallow(item->obj, "elm.swallow.content", item->content);
	edje_object_signal_emit(item->obj, "elm,state,enable_icon", "elm");
	evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, item->ctxpopup);
	edje_object_message_signal_process(item->obj);
	_sizing_eval(item->ctxpopup);
}

/**
 * reset the label on the given item. This function is only for label item.
 *
 * @param obj 	 	Ctxpopup item
 * @param label		Label to be set
 * 
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_item_label_set(Elm_Ctxpopup_Item* item, const char* label) 
{
	if((!item) || (!label)) return;
	if(item->content) fprintf( stderr, "You cannot add label in icon item! - %p\n", item);
	if(item->label) eina_stringshare_del(item->label);
	if(label) 
	{
		item->label = eina_stringshare_add(label);
		edje_object_signal_emit(item->obj, "elm,state,text,visible", "elm");
	}else 
	{
		item->label = NULL;
		edje_object_signal_emit(item->obj, "elm,state,text,hidden", "elm");
	}
	edje_object_message_signal_process(item->obj);
	edje_object_part_text_set(item->obj, "elm.text", label);
	_sizing_eval(item->ctxpopup);
}

/**
 * Add a new item as an icon in given ctxpopup object.
 *
 * @param obj 	 	Ctxpopup object
 * @param icon		Icon to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data          User data for callback function
 * @return 		Added ctxpopup item
 * 
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item* 
elm_ctxpopup_icon_add(Evas_Object* obj, Evas_Object* icon, void (*func ) (void* data, Evas_Object* obj, void* event_info ), const void* data)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Elm_Ctxpopup_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return NULL;
	
	_separator_obj_add(obj);
	
	item = calloc(1, sizeof(Elm_Ctxpopup_Item));
	if(!item) return NULL;

	item->func = func;
	item->data = data;
	item->ctxpopup = obj;
	item->separator = EINA_FALSE;
	_item_obj_create(item);
	wd->items = eina_list_append(wd->items, item);
	elm_ctxpopup_item_icon_set(item, icon);
	_item_scale_shrinked_set(wd, item);
	elm_box_pack_end(wd->box, item->obj);
	_sizing_eval(obj);
	return item;
}

/**
 * Add a new item as an label in given ctxpopup object.
 *
 * @param obj 	 	Ctxpopup object
 * @param icon		label to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data          User data for callback function
 * @return 		Added ctxpopup item
 *
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item* 
elm_ctxpopup_label_add(Evas_Object* obj, const char* label, void (*func ) (void* data, Evas_Object* obj, void* event_info ), const void* data)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Elm_Ctxpopup_Item* item;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get( obj );
	if(!wd) return NULL;

	_separator_obj_add(obj);
	item = calloc(1, sizeof(Elm_Ctxpopup_Item));
	if(!item) return NULL;

	item->func = func;
	item->data = data;
	item->ctxpopup = obj;
	item->separator = EINA_FALSE;
	_item_obj_create(item);
	wd->items = eina_list_append(wd->items, item);
	elm_ctxpopup_item_label_set(item, label);
	_item_scale_shrinked_set(wd, item);
	elm_box_pack_end(wd->box, item->obj);
	_sizing_eval(obj);
	return item;
}

/**
 * Delete the given item in ctxpopup object.
 *
 * @param item 	 	Ctxpopup item to be deleted
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_item_del(Elm_Ctxpopup_Item* item)
{
	Widget_Data* wd;

	if(!item) return;
	if(item->label) eina_stringshare_del(item->label);
	if(item->content) evas_object_del(item->content);
	if(item->obj) evas_object_del(item->obj);
	
	wd = (Widget_Data*) elm_widget_data_get(item->ctxpopup);
	if(wd) 
	{
		_separator_obj_del( wd, item );
		wd->items = eina_list_remove( wd->items, item );
	}
	free(item);
	_item_scale_normal_set(wd);
	if(eina_list_count(wd->items) < 1) 
	{
		evas_object_hide(wd->arrow);
		evas_object_hide(wd->hover);
	}
}

/**
 * Disable or Enable the given item. Once an item is disabled, the click event will be never happend for the item.
 *
 * @param item		Ctxpopup item to be disabled
 * @param disabled 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void 
elm_ctxpopup_item_disabled_set(Elm_Ctxpopup_Item* item, Eina_Bool disabled)
{
	Widget_Data* wd;
	int item_count;

	if(!item) return;
	if(disabled == item->disabled) return;
	wd = (Widget_Data*) elm_widget_data_get(item->ctxpopup);
	item_count = eina_list_count(wd->items);
	item_count -= item_count/2;
	
	if(disabled) 
	{
		if(item_count > wd->expand_cnt) 
			edje_object_signal_emit(item->obj, "elm,state,shrinked_disabled", "elm");
		else 
			edje_object_signal_emit(item->obj, "elm,state,disabled", "elm");
	}else 
	{
		if(item_count > wd->expand_cnt) 
			edje_object_signal_emit(item->obj, "elm,state,shrinked", "elm");
		else 
			edje_object_signal_emit(item->obj, "elm,state,enabled", "elm");
	}

	edje_object_message_signal_process(item->obj);
	item->disabled = disabled;
}





