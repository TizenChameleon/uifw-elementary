#include <Elementary.h>
#include "elm_priv.h"
#include <math.h>

/**
 * @defgroup Discpicker Discpicker
 * @ingroup Elementary
 *
 * This is a discpicker.
 */

#define FPS 50			// frame per second
#define FRICTION 0.98		// friction to reduce speed
#define FRICTION2 0.07
#define V_MIN 0.01		// the velocity of spin
#define BOUNCE_RATE 20.0
#define ROW_H 66		// default size of row

typedef enum {
	ANIMATION_TYPE_SPIN = 0,
	ANIMATION_TYPE_SCROLL,
	ANIMATION_TYPE_MAX
} _Animation_Type;

struct _Discpicker_Item {
	Evas_Object *discpicker;
	const char *label;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	void *data;
	void *priv_data;
	int disabled : 1;
	int walking;
  int delete_me : 1;
};

struct _Event_Info {
	unsigned int	timestamp;
	int y;
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	/* objects */
	Evas_Object *base;
	Evas_Object *indi;
	Evas_Object *clip;

	/* items */
	Eina_List *items;
	Eina_List *vitems; // visual items
	Eina_List *cells;
	Eina_List *current;

	/* size of base */
	Evas_Coord x, y, w, h;

	/* size of row */
	Evas_Coord cell_y;
	int row_h;	
	int cell_cnt;
	
	/* for animation */
	struct _Event_Info prev_ev_info;
	Ecore_Timer *timer;
	_Animation_Type animation_type;
	double v;
	double p;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _update_discpicker(Evas_Object *obj);
static void _changed(Evas_Object *obj);
static void _resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _move_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static double _round( double value, int pos);
static double _infinite(double n, int limit);
static int _cell_num_get(int bg_height, int cell_height);
static void _flick_init(Evas_Object *obj);
static void _data_init(Evas_Object *obj);
static void _view_init(Evas_Object *obj);
static void _data_loop(Evas_Object *obj, int distance);
static void _view_loop(Evas_Object *obj);
static void _scroll_start(Evas_Object *obj, double power);
static int _timer_cb(void *data);
static void _determine_data(_Animation_Type animation_type, int distance, int row_h, int nitems, double *v, double *p);
static Eina_List * _determine_current(Evas_Object *obj, _Animation_Type animation_type, Eina_List *items, int nitems, Eina_List *current, double v, int p_old, int p_new);
static Eina_List * _determine_vitems(_Animation_Type animation_type, Eina_List *items, Eina_List *current, int cell_cnt, int p_int);
static void _redraw_cells(_Animation_Type animation_type, Eina_List *cells, Eina_List *items, Eina_List *cur_cell, int x, int cell_y, int row_h, int cell_cnt, double p);
static void _item_del(Elm_Discpicker_Item *item);


static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Discpicker_Item *item;
	Evas_Object *txt;

	if (!wd) return;

	if (wd->cells) EINA_LIST_FREE(wd->cells, txt);
	if (wd->vitems) EINA_LIST_FREE(wd->vitems, item);

	if (wd->items) {
		EINA_LIST_FREE(wd->items, item) {
			if (item->label)
				eina_stringshare_del(item->label);
			free(item);
		}
	}
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_elm_theme_object_set(obj, wd->base, "discpicker", "base", elm_widget_style_get(obj));
	_flick_init(obj);
	_update_discpicker(obj);
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
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

static void
_changed(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Discpicker_Item *item;
	if (!wd || !wd->current) return;

	item = eina_list_data_get(wd->current);
	if (!item) return;

	item->walking++;

	if (item->func)
	  item->func(item->data, obj, item);
	if (!item->delete_me) 
	  evas_object_smart_callback_call(obj, "selected", eina_list_data_get(wd->current));

	item->walking--;

	if (!item->walking && item->delete_me)
	  _item_del(item);
}

static double 
_round( double value, int pos)
{
	double temp = 0.0;
	
	temp = value * pow( 10, pos ); 
	temp = floor( temp + 0.5 );    
	temp *= pow( 10, -pos );   
	
	return temp;
}

static double 
_infinite(double n, int limit) 
{
	if (limit < 1)
		return n;

	while (n < 0)
		n += limit;	
	while (n >= limit)
		n -= limit; 

	return n;
}

static void 
_scroll_start(Evas_Object *obj, double power) 
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	wd->v += power;
	wd->v *= FRICTION2;
	
	if(wd->timer){
		ecore_timer_del(wd->timer);
		wd->timer = NULL;	
	}
	wd->timer = ecore_timer_add(1/FPS, (void *)_timer_cb, obj);
}

static void 
_data_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	int nitems = eina_list_count(wd->items);

	wd->animation_type = nitems >= (wd->cell_cnt - 2) ? ANIMATION_TYPE_SPIN : ANIMATION_TYPE_SCROLL;
	wd->v = 0.0;
	wd->p = 0.0;

	wd->current = wd->items;
}

static void 
_view_init(Evas_Object *obj)
{	
	int i=0;
	Evas_Object *txt;
	Widget_Data *wd = elm_widget_data_get(obj);	
	if (!wd) return;
	
	// remove cell list
	if (wd->cells)
		EINA_LIST_FREE(wd->cells, txt)
			if (txt) evas_object_del(txt);
	
	// create cell list
	for(i=0; i<wd->cell_cnt; i++){
		txt = edje_object_add(evas_object_evas_get(obj));
		_elm_theme_object_set(obj, txt, "discpicker/item", "base", elm_widget_style_get(obj));
		evas_object_resize(txt, wd->w, wd->row_h);
		evas_object_clip_set(txt, wd->clip);
		elm_widget_sub_object_add(obj, txt);
		evas_object_show(txt);
		
		wd->cells = eina_list_append(wd->cells, txt);
	}
	
	// move cell and set item
	_view_loop(obj);
}


static void _determine_data(_Animation_Type animation_type, int distance, int row_h, int nitems, double *v, double *p)
{
	double v_sign = *v > 0.0 ? 1.0 : -1.0; 
	double v_abs = *v * v_sign;

	if (row_h <= 0) return;

	if(!distance){ // set the position using velocity
		if (*v == 0.0) {
		}
		else if (v_abs > V_MIN) {
			*v *= FRICTION;
		}
		else {
			if (v_abs < V_MIN) {
				*v = v_sign * V_MIN;
			} 
	
			if ((v_sign > 0 && (int)(*p) != (int)(*p + *v)) || (v_sign <= 0 && ceil(*p) != ceil(*p + *v)) || *p * (*p + *v) < 0) {
				*p = _round((*p + *v), 0);
				*v = 0.0; 
			}
		} 
		*p += *v;
	}else{ // set the position directly
		*p += (double)distance/row_h;
	}


	switch(animation_type){
			case ANIMATION_TYPE_SPIN:
				*p = _infinite(*p, nitems);
				break;
			case ANIMATION_TYPE_SCROLL:
				// bounce: set the v again if p has value of beyound the range of cells.
				if (*p < 0.0)
					*v = -(*p) / BOUNCE_RATE;
				else if (*p >= nitems - 1.0)
					*v = (nitems - 1.0 - *p) / BOUNCE_RATE;
				break;
			default:
				break;
		}
	
}

static Eina_List * _determine_current(Evas_Object *obj, _Animation_Type animation_type, Eina_List *items, int nitems, Eina_List *current, double v, int p_old, int p_new)
{
	Eina_List *ptr = current;
	int gap = 0;
	
	if (!obj || !items) return ptr;

	switch(animation_type){
		case ANIMATION_TYPE_SPIN:
			if(!ptr) break;
			if(v > 0.0){ 
				gap = p_new - p_old;
				if(gap < 0){
					gap = p_new;
					ptr = items;
				}
				if(gap > 0) while(gap--)	ptr = eina_list_next(ptr);
							
				if (p_new < p_old) 
						evas_object_smart_callback_call(obj, "overflowed", eina_list_data_get(ptr));
							
			}else{ 
				int gap = p_old - p_new;
				if(gap < 0){
					gap = (nitems - 1) - p_new;
					ptr = eina_list_last(items);
				}
				if(gap > 0) while(gap--)	ptr = eina_list_prev(ptr);

				if (p_new > p_old)
						evas_object_smart_callback_call(obj, "underflowed", eina_list_data_get(ptr));
			} 
			
			break;
		case ANIMATION_TYPE_SCROLL:
			{
				int cnt = p_new;
				
				if(cnt < 0)
					ptr = NULL;
				else{
					ptr = items;
					while(cnt--){
						if(!ptr) break;
						ptr = eina_list_next(ptr);
					}
				}
			}
			break;
		default:
			break;
	}

	return ptr;

}

static Eina_List * _determine_vitems(_Animation_Type animation_type, Eina_List *items, Eina_List *current, int cell_cnt, int p_int)
{
	int gap = cell_cnt>>1;
	Eina_List *tmp = NULL;
	Elm_Discpicker_Item *item = NULL;
	Eina_List *ptr = NULL;


	switch(animation_type){
		case ANIMATION_TYPE_SPIN:
			if(!current) break;

			if(p_int >= gap){
				tmp = current;
			}else{
				gap = gap - p_int - 1;
				tmp = eina_list_last(items);
			}
			
			if(gap > 0) while(gap--)	tmp = eina_list_prev(tmp);

			while(cell_cnt--){
				item = eina_list_data_get(tmp);
				ptr = eina_list_append(ptr, item);
				tmp = eina_list_next(tmp);
				if(!tmp)	tmp = items;
			}
			break;
		case ANIMATION_TYPE_SCROLL:
				gap = gap + p_int + 1;
				if(gap <= 0){
					ptr = NULL;
				}else{
					tmp = items;
					if(gap > cell_cnt){
						int start = gap - cell_cnt;
						while(start--){
							if(!tmp) break;
							tmp = eina_list_next(tmp);
						}
					}
					
					while(gap--){
						if(!tmp) break;
						item = eina_list_data_get(tmp);
						ptr = eina_list_append(ptr, item);
						tmp = eina_list_next(tmp);	
					}
				}
			break;
		default:
			break;
	}

	return ptr;
}

static void _redraw_cells(_Animation_Type animation_type, Eina_List *cells, Eina_List *items, Eina_List *cur_cell, int x, int cell_y, int row_h, int cell_cnt, double p)
{
	Eina_List *l;
	Evas_Object *txt;
	Elm_Discpicker_Item *item = NULL;
	int nitems = eina_list_count(items);
	int p_int = (int)(p);
	double	p_odd = p - p_int;
	int i = 0;
	

	EINA_LIST_FOREACH(cells, l, txt)
	{
		int y = cell_y + i * row_h - p_odd * row_h;
		int row = p_int + i - cell_cnt/2; 	

		evas_object_move(txt, x,  y);
		
		switch(animation_type){
			case ANIMATION_TYPE_SPIN:		
				if(cur_cell){
					item = eina_list_data_get(cur_cell);
					if(item)	edje_object_part_text_set(txt, "elm.label", item->label);
					else edje_object_part_text_set(txt, "elm.label", "");
					cur_cell = eina_list_next(cur_cell);
				}		
				break;
			case ANIMATION_TYPE_SCROLL:
				if (row >= 0 && row < nitems) {
					int count = row;
					cur_cell = items;

					while(count--){
						if(!cur_cell)	break;
						cur_cell = eina_list_next(cur_cell);					
					}
					
					if(cur_cell){
						item = eina_list_data_get(cur_cell);
						if(item)	edje_object_part_text_set(txt, "elm.label", item->label);
						else edje_object_part_text_set(txt, "elm.label", "");
					}else{
						edje_object_part_text_set(txt, "elm.label", "");
					}
				}else {
					edje_object_part_text_set(txt, "elm.label", "");
				}
				break;
			default:
				break;
		}
		
		if(item){
			if (item->disabled) {
				edje_object_signal_emit(txt, "elm,item,disabled", "elm");
			} else {
				if (i != (cell_cnt / 2))
					edje_object_signal_emit(txt, "elm,item,unselected", "elm");
				else
					edje_object_signal_emit(txt, "elm,item,selected", "elm");
			}
		}
		i++;
	}
}


static void 
_data_loop(Evas_Object *obj, int distance)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || wd->row_h <= 0 || !wd->items) return;
	
	int nitems = eina_list_count(wd->items);
	int p_old = (int)wd->p;
	Eina_List *previous = wd->current;


	/* 1. determine v and p */
	_determine_data(wd->animation_type, distance, wd->row_h, nitems, &wd->v, &wd->p);

	/* 2. determine current */
	wd->current = _determine_current(obj, wd->animation_type, wd->items, nitems, wd->current, wd->v, p_old, (int)wd->p);

	if(previous != wd->current){
			// TOBE:  1. sound, 2. changed signal
	}
}

static void 
_view_loop(Evas_Object *obj)
{
	Elm_Discpicker_Item *item = NULL;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return;


	/* 1. determine visual items */
	if (wd->vitems) EINA_LIST_FREE(wd->vitems, item);
	wd->vitems = _determine_vitems(wd->animation_type, wd->items, wd->current, wd->cell_cnt, (int)(wd->p));

	
	/* 2. redraw cells with vitems */
	_redraw_cells(wd->animation_type, wd->cells, wd->items, wd->vitems, wd->x, wd->cell_y, wd->row_h, wd->cell_cnt, wd->p);
}

static int 
_timer_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return 0;

	if (wd->v == 0.0) {
		if(wd->timer){
			ecore_timer_del(wd->timer);
			wd->timer = NULL;	
		}
		_changed(obj);
		return 0;
	}

	_data_loop(obj, 0);
	_view_loop(obj);

	return 1;
}

static void
_flick_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *bg;
	if(!wd)	return;

	bg = (Evas_Object *)edje_object_part_object_get(wd->base, "elm.bg");
	if(!bg) return;
	
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_UP, _mouse_up, obj);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, obj);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, obj);
}

static void
_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Down *ev = event_info;
	
	if (!wd) return;	
	
	if(wd->timer){
		ecore_timer_del(wd->timer);
		wd->timer = NULL;	
	}

	wd->prev_ev_info.timestamp = ev->timestamp;
	wd->prev_ev_info.y = ev->canvas.y;
}

static void
_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Move *ev = event_info;
	unsigned int dur;
	Evas_Coord distance;
	
	if (!wd || !ev->buttons) return;

	if(wd->prev_ev_info.timestamp != 0){
		dur = ev->timestamp - wd->prev_ev_info.timestamp;
		distance = wd->prev_ev_info.y - ev->cur.canvas.y;

		if(dur >0){
			wd->v = (double)distance/dur;
			_data_loop(data, distance);
			_view_loop(data);
		}
	}

	wd->prev_ev_info.timestamp = ev->timestamp;
	wd->prev_ev_info.y = ev->cur.canvas.y;
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Up *ev = event_info;
	unsigned int dur;
	Evas_Coord distance;
	double cur_v = 0.0;
	double	p_odd = wd->p - (int)(wd->p);

	if (!wd) return;

	dur = ev->timestamp - wd->prev_ev_info.timestamp;
	distance = wd->prev_ev_info.y - ev->canvas.y;
	
	if(dur <= 0 || abs(distance) <= ((elm_finger_size_get()>>1))){
		if(p_odd < 0.5)	cur_v = -V_MIN;
		else cur_v = V_MIN;
	}else 
		cur_v = (double)distance/dur;
	
	_scroll_start(data, cur_v);

	wd->prev_ev_info.timestamp = 0;
	wd->prev_ev_info.y = 0;
}

static int 
_cell_num_get(int bg_height, int cell_height)
{
	if(bg_height <=0 || cell_height <= 0) return 0;
	
	int visual_min_cnt = bg_height/cell_height + 1;
	
	if(visual_min_cnt % 2 == 0)
		visual_min_cnt ++;

	visual_min_cnt += 2;
	
	return visual_min_cnt;
}

static void 
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);	
	Evas_Coord w, h;
	int y_mid;

	if (!wd || wd->row_h <= 0) return;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	y_mid = wd->y + ((h - wd->row_h)>>1);

	wd->w = w;
	wd->h = h;
	wd->cell_cnt = _cell_num_get(wd->h, wd->row_h);
	wd->cell_y = y_mid - wd->row_h * (wd->cell_cnt>>1);

	evas_object_resize(wd->indi, w, wd->row_h);
	evas_object_move(wd->indi, wd->x, y_mid);

	int nitems = eina_list_count(wd->items);
	wd->animation_type = nitems >= (wd->cell_cnt - 2) ? ANIMATION_TYPE_SPIN : ANIMATION_TYPE_SCROLL;

	_view_init(data);	
}

static void 
_move_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Coord x, y;
	int y_mid;

	if (!wd) return;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);

	wd->x = x;
	wd->y = y;
	y_mid = wd->y + ((wd->h - wd->row_h)>>1);
	evas_object_move(wd->indi, wd->x, y_mid);

	int nitems = eina_list_count(wd->items);
	wd->animation_type = nitems >= (wd->cell_cnt - 2) ? ANIMATION_TYPE_SPIN : ANIMATION_TYPE_SCROLL;

	_view_init(data);
}

static void
_update_discpicker(Evas_Object *obj)
{
	_data_init(obj);
	_view_init(obj);
	_sizing_eval(obj);
}

static void
_item_del(Elm_Discpicker_Item *item)
{
	Eina_List *l;
	Elm_Discpicker_Item *_item;
	Widget_Data *wd;

	wd = elm_widget_data_get(item->discpicker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			wd->items = eina_list_remove(wd->items, _item);
			if (!wd->current)
				wd->current = wd->items;

			wd->animation_type = eina_list_count(wd->items) >= (wd->cell_cnt - 2) ? ANIMATION_TYPE_SPIN : ANIMATION_TYPE_SCROLL;
			_data_loop(item->discpicker, 0);
			_view_loop(item->discpicker);

			free(_item);
			break;
		}
	}
}


/**
 * Add a new discpicker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Discpicker
 */
EAPI Evas_Object *
elm_discpicker_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "discpicker");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	
	wd->clip = evas_object_rectangle_add(e);
	elm_widget_resize_object_set(obj, wd->clip);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "discpicker", "base", "default");
	elm_widget_hover_object_set(obj, wd->base);
	evas_object_clip_set(wd->base, wd->clip);
	evas_object_show(wd->base);

	wd->indi = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->indi);
	_elm_theme_object_set(obj, wd->indi, "discpicker/indicator", "base", "default");
	evas_object_clip_set(wd->indi, wd->clip);
	evas_object_pass_events_set(wd->indi, EINA_TRUE);
	evas_object_show(wd->indi);
	
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOVE, _move_cb, obj);

	wd->row_h = ROW_H;

	_flick_init(obj);
	_update_discpicker(obj);

	return obj;
}

/**
 * Set the height of row
 *
 * @param obj The discpicker object
 * @param row_height The height of row
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_row_height_set(Evas_Object *obj, unsigned int row_height)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int y_mid;
	if (!wd) return;

	if(row_height <= 0){
		fprintf(stderr, "Invalid height of row\n");
		return;
	}

	wd->row_h = row_height;
	wd->cell_cnt = _cell_num_get(wd->h, wd->row_h);
	y_mid = wd->y + ((wd->h - wd->row_h)>>1);
	wd->cell_y = y_mid - wd->row_h * (wd->cell_cnt>>1);

	_view_init(obj);
}

/**
 * Select next item of discpicker
 *
 * @param obj The discpicker object
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_next(Evas_Object *obj)
{
	_scroll_start(obj, V_MIN);
}

/**
 * Select previous item of discpicker
 *
 * @param obj The discpicker object
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_prev(Evas_Object *obj)
{
	_scroll_start(obj, -V_MIN);
}

/**
 * Append item to the end of discpicker
 *
 * @param obj The discpicker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_item_append(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Discpicker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Discpicker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->discpicker = obj;
		wd->items = eina_list_append(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_discpicker(obj);
		}
		_view_loop(obj);
	}
	return item;
}

/**
 * Prepend item at start of discpicker
 *
 * @param obj The discpicker object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_item_prepend(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Discpicker_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Discpicker_Item);
	if (item) {
		if (label)
			item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->discpicker = obj;
		wd->items = eina_list_prepend(wd->items, item);
		if (!wd->current) {
			wd->current = wd->items;
			_update_discpicker(obj);
		}
		_view_loop(obj);
	}
	return item; 
}

/**
 * Get a list of items in the discpicker
 *
 * @param obj The discpicker object
 * @return The list of items, or NULL if none
 *
 * @ingroup Discpicker
 */
EAPI const Eina_List *
elm_discpicker_items_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->items;
}

/**
 * Get the first item in the discpicker
 *
 * @param obj The discpicker object
 * @return The first item, or NULL if none
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_first_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the discpicker
 *
 * @param obj The discpicker object
 * @return The last item, or NULL if none
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_last_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the selected item in the discpicker
 *
 * @param obj The discpicker object
 * @return The selected item, or NULL if none
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_selected_item_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->current) return NULL;
	return eina_list_data_get(wd->current);
}

/**
 * Set the selected state of an item
 *
 * @param item The item
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_item_selected_set(Elm_Discpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Discpicker_Item *_item;
	int cnt = 0;

	if (!item) return;
	wd = elm_widget_data_get(item->discpicker);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			_data_loop(item->discpicker, (double)(cnt - wd->p)*wd->row_h);
			_view_loop(item->discpicker);
			_changed(item->discpicker);
		}
		cnt++;
	}
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_item_del(Elm_Discpicker_Item *item)
{
	if (!item) return;
	if (item->walking > 0)
	  item->delete_me = EINA_TRUE;
	else
	  _item_del(item);
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Discpicker
 */
EAPI const char *
elm_discpicker_item_label_get(Elm_Discpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Discpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->discpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item)
			return item->label;

	return NULL;
}

/**
 * Set the label of a given item
 *
 * @param item The item
 * @param label The text label string in UTF-8
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_item_label_set(Elm_Discpicker_Item *item, const char *label)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Discpicker_Item *_item;

	if (!item || !label) return;
	wd = elm_widget_data_get(item->discpicker);
	if (!wd || !wd->items) return;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			eina_stringshare_del(item->label);
			item->label = eina_stringshare_add(label);
			_view_loop(item->discpicker);
		}
}

/**
 * Get the previous item in the discpicker
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_item_prev(Elm_Discpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Discpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->discpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			l = eina_list_prev(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}

/**
 * Get the next item in the discpicker
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Discpicker
 */
EAPI Elm_Discpicker_Item *
elm_discpicker_item_next(Elm_Discpicker_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Discpicker_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->discpicker);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			l = eina_list_next(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}

/**
 * Get private data of item
 *
 * @param item The item
 * @return Private data of the item @p item
 *
 * @ingroup Discpicker
 */
EAPI void *
elm_discpicker_item_data_get(Elm_Discpicker_Item *item)
{
	if (!item) return NULL;
	return item->priv_data;
}

/**
 * Set private data of item
 *
 * @param item The item
 * @param data data
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_item_data_set(Elm_Discpicker_Item *item, void *data)
{
	if (!item) return;
	item->priv_data = data;
}

/**
 * Get disabled flag of item
 *
 * @param item The item
 * @return disabled flag
 *
 * @ingroup Discpicker
 */
EAPI Eina_Bool
elm_discpicker_item_disabled_get(Elm_Discpicker_Item *item)
{
	if (!item) return EINA_FALSE;
	return item->disabled ? EINA_TRUE : EINA_FALSE;
}

/**
 * Set disabled flag of item
 *
 * @param item The item
 * @param disabled  disabled flag
 *
 * @ingroup Discpicker
 */
EAPI void
elm_discpicker_item_disabled_set(Elm_Discpicker_Item *item, Eina_Bool disabled)
{
	if (!item) return;
	item->disabled = disabled ? 1 : 0;
	_view_loop(item->discpicker);
}
