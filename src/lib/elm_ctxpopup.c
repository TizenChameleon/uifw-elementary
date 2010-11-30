#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Ctxpopup Ctxpopup
 * @ingroup Elementary
 *
 * Contextual popup.
 *
 * Signals that you can add callbacks for are:
 *
 * hide - This is emitted when the ctxpopup is hided.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Ctxpopup_Item {
	Evas_Object *ctxpopup;
	Evas_Object *base;
	const char *label;
	Evas_Object *icon;
	void (*func)(void *data, Evas_Object * obj, void *event_info);
	void *data;
	Eina_Bool disabled :1;
	Eina_Bool separator :1;
};

struct _Widget_Data {
	Evas_Object *parent;
	Evas_Object *base;
	Evas_Object *content;
	Evas_Object *box;
	Evas_Object *arrow;
	Evas_Object *scroller;
	Evas_Object *bg;
	Evas_Object *btn_layout;
	Evas_Object *area_rect;
	Eina_List *items;
	Elm_Ctxpopup_Arrow arrow_dir;
	Elm_Ctxpopup_Arrow arrow_priority[4];
	int btn_cnt;
	Elm_Transit *transit;
	Evas_Coord max_sc_w, max_sc_h;
	char *title;
	Eina_Bool scroller_disabled :1;
	Eina_Bool horizontal :1;
	Eina_Bool visible :1;
	Eina_Bool screen_dimmed_disabled :1;
	Eina_Bool position_forced :1;
	Eina_Bool finished :1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _area_rect_resize(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _area_rect_move(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _area_rect_del(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _bg_clicked_cb(void *data, Evas_Object *obj, const char *emission,
		const char *source);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _ctxpopup_show(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _ctxpopup_hide(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _ctxpopup_move(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _ctxpopup_changed_size_hints(void *data, Evas *e, Evas_Object *obj, 
		void *event_info);
static void _ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object *obj,
		void *event_info);
static void _item_obj_create(Elm_Ctxpopup_Item *item, char *group_name);
static void _item_sizing_eval(Elm_Ctxpopup_Item *item);
static void _ctxpopup_item_select(void *data, Evas_Object *obj,
		const char *emission, const char *source);
static void _separator_obj_add(Evas_Object *obj);
static void _separator_obj_del(Widget_Data *wd, Elm_Ctxpopup_Item *remove_item);
static Elm_Ctxpopup_Arrow _calc_base_geometry(Evas_Object *obj,
		Evas_Coord_Rectangle *rect);
static void _update_arrow_obj(Evas_Object *obj, Elm_Ctxpopup_Arrow arrow_dir);
static void _shift_base_by_arrow(Evas_Object *arrow,
		Elm_Ctxpopup_Arrow arrow_dir, Evas_Coord_Rectangle *rect);
static void _btn_layout_create(Evas_Object *obj);
static int _get_indicator_h(Evas_Object *parent);
static void _delete_area_rect_callbacks(Widget_Data *wd);
static void _adjust_pos_x(Evas_Coord_Point *pos, Evas_Coord_Point *base_size,
		Evas_Coord_Rectangle *area_rect);
static void _adjust_pos_y(int indicator_h, Evas_Coord_Point *pos,
		Evas_Coord_Point *base_size, Evas_Coord_Rectangle *area_rect);
static void _reset_scroller_size(Widget_Data *wd);
static void _hide_ctxpopup(Evas_Object *obj);
static void _content_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _content_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _reset_scroller_size(Widget_Data *wd)
{
	wd->finished = EINA_FALSE;
	wd->max_sc_h = -1;
	wd->max_sc_w = -1;
}

static void _delete_area_rect_callbacks(Widget_Data *wd) 
{
	if (!wd->area_rect) return;

	evas_object_event_callback_del(wd->area_rect, EVAS_CALLBACK_DEL,
			_area_rect_del);
	evas_object_event_callback_del(wd->area_rect, EVAS_CALLBACK_MOVE,
			_area_rect_move);
	evas_object_event_callback_del(wd->area_rect, EVAS_CALLBACK_RESIZE,
			_area_rect_resize);
}

static void _area_rect_resize(void *data, Evas *e, Evas_Object *obj,	void *event_info) 
{
	Widget_Data *wd = elm_widget_data_get(data);
	if(wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(obj);
	}
}

static void _area_rect_move(void *data, Evas *e, Evas_Object *obj, void *event_info) 
{
	Widget_Data *wd = elm_widget_data_get(data);
	if(wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(obj);
	}
}

static void _area_rect_del(void *data, Evas *e, Evas_Object *obj, void *event_info) 
{
	Widget_Data *wd = elm_widget_data_get(data);
	wd->area_rect = NULL;
}

static void _show_effect_done(void *data, Elm_Transit *transit) 
{
	//TODO: Consider implementing effect in edc.
	Widget_Data *wd = data;
	elm_transit_fx_clear(transit);

	if (wd->box)
		elm_transit_fx_insert(transit, elm_fx_color_add(wd->box, 0, 0, 0, 0,
				255, 255, 255, 255));
	if (wd->content)
		elm_transit_fx_insert(transit, elm_fx_color_add(wd->content, 0, 0, 0,
				0, 255, 255, 255, 255));
	if (wd->btn_layout)
		elm_transit_fx_insert(transit, elm_fx_color_add(wd->btn_layout, 0, 0,
				0, 0, 255, 255, 255, 255));
	elm_transit_run(transit, 0.2);
	elm_transit_completion_callback_set(transit, NULL, NULL);
	elm_transit_del(transit);
	wd->transit = NULL;
	edje_object_signal_emit(wd->base, "elm,state,show", "elm");
}

static void _show_effect(Widget_Data* wd) 
{
	//TODO: Consider implementing effect in edc.
	if (wd->transit) {
		elm_transit_stop(wd->transit);
		elm_transit_fx_clear(wd->transit);
	} else {
		wd->transit = elm_transit_add(wd->base);
		elm_transit_curve_style_set(wd->transit, ELM_ANIMATOR_CURVE_OUT);
		elm_transit_completion_callback_set(wd->transit, _show_effect_done, wd);
	}

	elm_transit_fx_insert(wd->transit, elm_fx_color_add(wd->base, 0, 0, 0, 0,
			255, 255, 255, 255));
	elm_transit_fx_insert(wd->transit, elm_fx_wipe_add(wd->base,
			ELM_FX_WIPE_TYPE_SHOW, wd->arrow_dir));

	if(!wd->position_forced)
		elm_transit_fx_insert(wd->transit, elm_fx_color_add(wd->arrow, 0, 0, 0, 0, 255, 255, 255, 255));

	if (wd->box)
		evas_object_color_set(wd->box, 0, 0, 0, 0);
	if (wd->content)
		evas_object_color_set(wd->content, 0, 0, 0, 0);
	if (wd->btn_layout)
		evas_object_color_set(wd->btn_layout, 0, 0, 0, 0);

	elm_transit_run(wd->transit, 0.3);
}

static void _hide_effect_done(void *data, Elm_Transit *transit)
{
	//TODO: Consider implementing effect in edc.
	Widget_Data *wd = elm_widget_data_get(data);
	if(!wd) return ;
	elm_transit_del(transit);
	wd->transit = NULL;
	_hide_ctxpopup(data);
}

static void _hide_effect(Evas_Object *obj)
{
	//TODO: Consider implementing effect in edc.
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;

	if (wd->transit) {
		elm_transit_stop(wd->transit);
		elm_transit_fx_clear(wd->transit);
	} else {
		wd->transit = elm_transit_add(wd->base);
		elm_transit_curve_style_set(wd->transit, ELM_ANIMATOR_CURVE_OUT);
		elm_transit_completion_callback_set(wd->transit, _hide_effect_done, obj);
	}

//	elm_transit_fx_insert(wd->transit, elm_fx_color_add(wd->base, 255, 255, 255, 255, 0, 0, 0, 0));

	switch(wd->arrow_dir) {
		case  ELM_FX_WIPE_DIR_UP:
			elm_transit_fx_insert(wd->transit, elm_fx_wipe_add(wd->base, ELM_FX_WIPE_TYPE_HIDE, ELM_FX_WIPE_DIR_DOWN));
			break;
		case  ELM_FX_WIPE_DIR_LEFT:
			elm_transit_fx_insert(wd->transit, elm_fx_wipe_add(wd->base, ELM_FX_WIPE_TYPE_HIDE, ELM_FX_WIPE_DIR_RIGHT));
			break;
		case  ELM_FX_WIPE_DIR_RIGHT:
			elm_transit_fx_insert(wd->transit, elm_fx_wipe_add(wd->base, ELM_FX_WIPE_TYPE_HIDE, ELM_FX_WIPE_DIR_LEFT));
			break;
		case  ELM_FX_WIPE_DIR_DOWN:
			elm_transit_fx_insert(wd->transit, elm_fx_wipe_add(wd->base, ELM_FX_WIPE_TYPE_HIDE, ELM_FX_WIPE_DIR_UP));
			break;
		default:
			break;
	}

	elm_transit_run(wd->transit, 0.3);
}

static void _separator_obj_del(Widget_Data *wd, Elm_Ctxpopup_Item *remove_item) 
{
	Eina_List *elist, *cur_list, *prev_list;
	Elm_Ctxpopup_Item *separator;

	if ((!remove_item) || (!wd)) return;

	elist = wd->items;
	cur_list = eina_list_data_find_list(elist, remove_item);

	if (!cur_list)	return;

	prev_list = eina_list_prev(cur_list);
	
	if (!prev_list) return;

	separator = (Elm_Ctxpopup_Item *) eina_list_data_get(prev_list);
	
	if (!separator)return;
	
	wd->items = eina_list_remove(wd->items, separator);
	evas_object_del(separator->base);
	free(separator);
}

static void _btn_layout_create(Evas_Object *obj) 
{
	Widget_Data *wd = elm_widget_data_get(obj);

	wd->btn_layout = edje_object_add(evas_object_evas_get(obj));
	elm_widget_sub_object_add(obj, wd->btn_layout);
	edje_object_signal_emit(wd->base, "elm,state,buttons,enable", "elm");
	edje_object_part_swallow(wd->base, "elm.swallow.btns", wd->btn_layout);
}

static void _separator_obj_add(Evas_Object *obj) 
{
	Elm_Ctxpopup_Item *item;

	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;
	if (eina_list_count(wd->items) == 0) return;

	item = ELM_NEW(Elm_Ctxpopup_Item);
	if (!item) return;

	item->base = edje_object_add(evas_object_evas_get(wd->base));
	if (!item->base) {
		free(item);
		return;
	}

	_elm_theme_object_set(obj, item->base, "ctxpopup", "separator",
			elm_widget_style_get(obj));

	if (wd->horizontal)
		edje_object_signal_emit(item->base, "elm,state,horizontal", "elm");
	else
		edje_object_signal_emit(item->base, "elm,state,vertical", "elm");

	evas_object_size_hint_align_set(item->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(item->base);
	item->separator = EINA_TRUE;
	elm_box_pack_end(wd->box, item->base);
	wd->items = eina_list_append(wd->items, item);
}

static void _item_sizing_eval(Elm_Ctxpopup_Item *item) 
{
	Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;
	Evas_Coord x, y, w, h;

	if (!item) return;

	if (!item->separator) elm_coords_finger_size_adjust(1, &min_w, 1, &min_h);

	evas_object_geometry_get(item->base, &x, &y, &w, &h);
	edje_object_size_min_restricted_calc(item->base, &min_w, &min_h, min_w, min_h);
	evas_object_size_hint_min_set(item->base, min_w, min_h);
	evas_object_size_hint_max_set(item->base, max_w, max_h);
}

static void _adjust_pos_x(Evas_Coord_Point *pos, Evas_Coord_Point *base_size,
		Evas_Coord_Rectangle *area_rect) 
{
	pos->x -= (base_size->x / 2);

	if (pos->x < area_rect->x) pos->x = area_rect->x;
	else if ((pos->x + base_size->x) > (area_rect->x + area_rect->w)) 
		pos->x = (area_rect->x + area_rect->w) - base_size->x;

	if (base_size->x > area_rect->w)
		base_size->x -= (base_size->x - area_rect->w);
	if (pos->x < area_rect->x)
		pos->x = area_rect->x;
}

static void _adjust_pos_y(int indicator_h, Evas_Coord_Point *pos,
		Evas_Coord_Point *base_size, Evas_Coord_Rectangle *area_rect) 
{
	pos->y -= (base_size->y / 2);

	if (pos->y < area_rect->y) pos->y = area_rect->y;
	else if ((pos->y + base_size->y) > (area_rect->y + area_rect->h)) 
		pos->y = area_rect->y + area_rect->h - base_size->y;

	if (base_size->y > area_rect->h) 
		base_size->y -= (base_size->y - area_rect->h);
	
	if (pos->y < area_rect->y) pos->y = area_rect->y;
}

static int _get_indicator_h(Evas_Object *parent) 
{
	Ecore_X_Window zone, xwin;
	int h = 0;

	if (elm_win_indicator_state_get(parent) != 1) 
		return 0;

	xwin = elm_win_xwindow_get(parent);
	zone = ecore_x_e_illume_zone_get(xwin);
	ecore_x_e_illume_indicator_geometry_get(zone, NULL, NULL, NULL, &h);

	return h;
}

static void _ctxpopup_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if (!wd) return;
	if(wd->visible)_sizing_eval(obj);
}

static Elm_Ctxpopup_Arrow _calc_base_geometry(Evas_Object *obj, Evas_Coord_Rectangle *rect) 
{
	Widget_Data *wd;
	Evas_Coord_Point pos;
	Evas_Coord_Point base_size;
	Evas_Coord_Point max_size;
	Evas_Coord_Point min_size;
	Evas_Coord_Rectangle area_rect;
	Evas_Coord_Point parent_size;
	Evas_Coord_Point arrow_size;
	Elm_Ctxpopup_Arrow arrow;
	Evas_Coord finger_size;
	Evas_Coord indicator_h;
	Evas_Coord_Point temp;
	int idx;

	wd = elm_widget_data_get(obj);

	if ((!wd) || (!rect)) return ELM_CTXPOPUP_ARROW_DOWN;

	indicator_h = _get_indicator_h(wd->parent);
	finger_size = elm_finger_size_get();

	edje_object_part_geometry_get(wd->arrow, "ctxpopup_arrow", NULL, NULL,
			&arrow_size.x, &arrow_size.y);
	evas_object_resize(wd->arrow, arrow_size.x, arrow_size.y);

	//Initialize Area Rectangle. 
	if (wd->area_rect)
		evas_object_geometry_get(wd->area_rect, &area_rect.x, &area_rect.y,
				&area_rect.w, &area_rect.h);
	else {
		evas_object_geometry_get(wd->parent, NULL, NULL, &parent_size.x, &parent_size.y);
		area_rect.x = 0;
		area_rect.y = 0;
		area_rect.w = parent_size.x;
		area_rect.h = parent_size.y;
	}

	if (area_rect.y < indicator_h) {
		temp.y = indicator_h - area_rect.y;
		area_rect.y = indicator_h;
		area_rect.h -= temp.y;
	}
	evas_object_geometry_get(obj, &pos.x, &pos.y, NULL, NULL);

	//recalc the edje 
	edje_object_size_min_calc(wd->base, &base_size.x, &base_size.y);
	evas_object_smart_calculate(wd->base);

	//Limit to Max Size
	evas_object_size_hint_max_get(obj, &max_size.x, &max_size.y);

	if((max_size.y > 0 ) && (base_size.y > max_size.y)) 
		base_size.y = max_size.y;

	if((max_size.x > 0 ) && (base_size.x > max_size.x)) 
		base_size.x = max_size.x;

	//Limit to Min Size 
	evas_object_size_hint_min_get(obj, &min_size.x, &min_size.y);
	
	if( min_size.y > 0 )  
		if(base_size.y < min_size.y) base_size.y = min_size.y;
		
	if( min_size.x > 0 ) 
		if(base_size.x < min_size.x) base_size.x = min_size.x;

	//In case of position forced. It shows up just like popup.  
	if (wd->position_forced) {
		//TODO: calculate the size of ctxpopup
		rect->x = pos.x;
		rect->y = pos.y;
		rect->w = base_size.x;
		rect->h = base_size.y;
		return ELM_CTXPOPUP_ARROW_UP;
	}

	//Check the Which direction is available.
	//If find a avaialble direction, it adjusts position and size. 
	for (idx = 0; idx < 4; ++idx) {
		switch (wd->arrow_priority[idx]) {
			case ELM_CTXPOPUP_ARROW_DOWN:
				temp.y = pos.y - base_size.y;
				if ((temp.y - arrow_size.y - finger_size) < area_rect.y) continue;
				_adjust_pos_x(&pos, &base_size, &area_rect);
				pos.y -= (base_size.y + finger_size);
				arrow = ELM_CTXPOPUP_ARROW_DOWN;
				break;
			case ELM_CTXPOPUP_ARROW_RIGHT:
				temp.x = (pos.x - base_size.x);
				if ((temp.x - arrow_size.x - finger_size) < area_rect.x) continue;
				_adjust_pos_y(indicator_h, &pos, &base_size, &area_rect);
				pos.x -= (base_size.x + finger_size);
				arrow = ELM_CTXPOPUP_ARROW_RIGHT;
				break;
			case ELM_CTXPOPUP_ARROW_LEFT:
				temp.x = (pos.x + base_size.x);
				if ((temp.x + arrow_size.x + finger_size) > (area_rect.x	+ area_rect.w)) continue;
				_adjust_pos_y(indicator_h, &pos, &base_size, &area_rect);
				pos.x += finger_size;
				arrow = ELM_CTXPOPUP_ARROW_LEFT;
				break;
			case ELM_CTXPOPUP_ARROW_UP:
				temp.y = (pos.y + base_size.y);
				if ((temp.y + arrow_size.y + finger_size) > (area_rect.y + area_rect.h)) continue;
				_adjust_pos_x(&pos, &base_size, &area_rect);
				pos.y += finger_size;
				arrow = ELM_CTXPOPUP_ARROW_UP;
				break;
			default:
				break;
		}
		break;
	}

	//In this case, all directions are invalid because of lack of space.
	if (idx == 4) {
		//TODO 1: Find the largest space direction.
		/*
		 Evas_Coord length[4];

		 length[ ELM_CTXPOPUP_ARROW_DOWN ] = pos.y - area_rect.y;
		 length[ ELM_CTXPOPUP_ARROW_UP ] = ( area_rect.y + area_rect.h ) - pos.y;
		 length[ ELM_CTXPOPUP_ARROW_RIGHT ] = pos.x - area_rect.x;
		 length[ ELM_CTXPOPUP_ARROW_LEFT ] = ( area_rect.x + area_rect.w ) - pos.x;

		 int i, j, idx;
		 for( i = 0; i < 4; ++i ) {
		 for( j = 1; j < 4; ++j ) {
		 if( length[ idx ] < length[ j ] ) {
		 idx = j;
		 }
		 }
		 }
		 */
		//TODO 1: Find the largest space direction.
		Evas_Coord length[2];
		length[0] = pos.y - area_rect.y;
		length[1] = (area_rect.y + area_rect.h) - pos.y;

		if (length[0] > length[1]) idx = ELM_CTXPOPUP_ARROW_DOWN;
		else idx = ELM_CTXPOPUP_ARROW_UP;

		//TODO 2: determine x , y
		switch (idx) {
			case ELM_CTXPOPUP_ARROW_DOWN:
				_adjust_pos_x(&pos, &base_size, &area_rect);
				pos.y -= (base_size.y + finger_size);
				arrow = ELM_CTXPOPUP_ARROW_DOWN;
				if (pos.y < area_rect.y + arrow_size.y) {
					base_size.y -= ((area_rect.y + arrow_size.y) - pos.y);
					pos.y = area_rect.y + arrow_size.y;
				}
				break;
			case ELM_CTXPOPUP_ARROW_RIGHT:
				_adjust_pos_y(indicator_h, &pos, &base_size, &area_rect);
				pos.x -= (base_size.x + finger_size);
				arrow = ELM_CTXPOPUP_ARROW_RIGHT;
				if (pos.x < area_rect.x + arrow_size.x) {
					base_size.x -= ((area_rect.x + arrow_size.x) - pos.x);
					pos.x = area_rect.x + arrow_size.x;
				}
				break;
			case ELM_CTXPOPUP_ARROW_LEFT:
				_adjust_pos_y(indicator_h, &pos, &base_size, &area_rect);
				pos.x += finger_size;
				arrow = ELM_CTXPOPUP_ARROW_LEFT;
				if (pos.x + arrow_size.x + base_size.x > area_rect.x + area_rect.w) {
					base_size.x -= ((pos.x + arrow_size.x + base_size.x)
						- (area_rect.x + area_rect.w));
				}
				break;
			case ELM_CTXPOPUP_ARROW_UP:
				_adjust_pos_x(&pos, &base_size, &area_rect);
				pos.y += finger_size;
				arrow = ELM_CTXPOPUP_ARROW_UP;
				if (pos.y + arrow_size.y + base_size.y > area_rect.y + area_rect.h) {
					base_size.y -= ((pos.y + arrow_size.y + base_size.y) - (area_rect.y + area_rect.h));
				}
				break;
			default:
				break;
		}
	}

	//Final position and size. 
	rect->x = pos.x;
	rect->y = pos.y;
	rect->w = base_size.x;
	rect->h = base_size.y;

	return arrow;
}

static void _update_arrow_obj(Evas_Object *obj, Elm_Ctxpopup_Arrow arrow_dir) 
{
	Evas_Coord x, y;
	Evas_Coord_Rectangle arrow_size;
	Evas_Coord_Rectangle area_rect;
	Evas_Coord parent_w, parent_h;
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	evas_object_geometry_get(wd->arrow, NULL, NULL, &arrow_size.w,
			&arrow_size.h);

	switch (arrow_dir) {
		case ELM_CTXPOPUP_ARROW_LEFT: {
			edje_object_signal_emit(wd->arrow, "elm,state,left", "elm");
			arrow_size.y = (y - (arrow_size.h * 0.5));
			arrow_size.x = (x + elm_finger_size_get());
			break;
		}
		case ELM_CTXPOPUP_ARROW_RIGHT: {
			edje_object_signal_emit(wd->arrow, "elm,state,right", "elm");
			arrow_size.y = (y - (arrow_size.h * 0.5));
			arrow_size.x = (x - elm_finger_size_get() - arrow_size.w);
			break;
		}
		case ELM_CTXPOPUP_ARROW_UP: {
			edje_object_signal_emit(wd->arrow, "elm,state,top", "elm");
			arrow_size.x = (x - (arrow_size.w * 0.5));
			arrow_size.y = (y + elm_finger_size_get());
			break;
		}
		case ELM_CTXPOPUP_ARROW_DOWN: {
			edje_object_signal_emit(wd->arrow, "elm,state,bottom", "elm");
			arrow_size.x = (x - (arrow_size.w * 0.5));
			arrow_size.y = (y - elm_finger_size_get() - arrow_size.h);
			break;
		}
		default:
			break;
	}

	//Adjust arrow position to prevent out of area
	if (wd->area_rect) 
		evas_object_geometry_get(wd->area_rect, &area_rect.x, &area_rect.y,
				&area_rect.w, &area_rect.h);
	else {
		evas_object_geometry_get(wd->parent, NULL, NULL, &parent_w, &parent_h);
		area_rect.x = 0;
		area_rect.y = 0;
		area_rect.w = parent_w;
		area_rect.h = parent_h;
	}

	//TODO: Temporary Code. make it more flexible
	if ((arrow_size.x - (arrow_size.w / 2)) < area_rect.x) 
		arrow_size.x = area_rect.x + (arrow_size.w / 2);
	else if ((arrow_size.x + arrow_size.w) > (area_rect.x + area_rect.w)) 
		arrow_size.x = (area_rect.x + area_rect.w) - arrow_size.w
				- (arrow_size.w / 2);
/*
	//TODO: Temporary Code. make it more flexible
	if ((arrow_size.y - (arrow_size.h / 2)) < area_rect.y) {
		arrow_size.y = arrow_size.y + (arrow_size.h / 2);
	} else if ((arrow_size.y + arrow_size.h) > (area_rect.y + area_rect.h)) {
		arrow_size.y = (area_rect.y + area_rect.h) - arrow_size.h
				- (arrow_size.h / 2);
	}
*/
	evas_object_move(wd->arrow, arrow_size.x, arrow_size.y);
}

static void _sizing_eval(Evas_Object *obj) 
{
	Widget_Data *wd;
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;
	Evas_Coord_Rectangle rect = { 0, 0, 1, 1 };
	Evas_Coord_Point box_size = { 0, 0 };
	Evas_Coord_Point _box_size = { 0, 0 };

	wd = (Widget_Data *) elm_widget_data_get(obj);
	if ((!wd) || (!wd->parent)) return;
	int idx = 0;

	//Box, scroller 
	EINA_LIST_FOREACH(wd->items, elist, item)
	{
		_item_sizing_eval(item);
		evas_object_size_hint_min_get(item->base, &_box_size.x, &_box_size.y);
		if(!wd->horizontal) {
			if(_box_size.x > box_size.x) box_size.x = _box_size.x;
			if(_box_size.y != -1 ) box_size.y += _box_size.y;
		} else {
			if(_box_size.x != -1 ) box_size.x += _box_size.x;
			if(_box_size.y > box_size.y) box_size.y = _box_size.y;
		}
		++idx;
	}

	if(!wd->content) {
		evas_object_size_hint_min_set(wd->box, box_size.x, box_size.y);
		evas_object_size_hint_min_set(wd->scroller, box_size.x, box_size.y);
	}

	//Base
	wd->arrow_dir = _calc_base_geometry(obj, &rect);
	if ((!wd->position_forced) && (wd->arrow_dir != -1)) {
		_update_arrow_obj(obj, wd->arrow_dir);
		_shift_base_by_arrow(wd->arrow, wd->arrow_dir, &rect);
	}

	//resize scroller according to final size. 
	if (!wd->content) {
		evas_object_smart_calculate(wd->scroller);
	}
	
	evas_object_move(wd->base, rect.x, rect.y);
	evas_object_resize(wd->base, rect.w, rect.h);

	if(wd->visible) _show_effect(wd);
}

static void _shift_base_by_arrow(Evas_Object *arrow,
		Elm_Ctxpopup_Arrow arrow_dir, Evas_Coord_Rectangle *rect) 
{
	Evas_Coord arrow_w, arrow_h;
	evas_object_geometry_get(arrow, NULL, NULL, &arrow_w, &arrow_h);

	switch (arrow_dir) {
	case ELM_CTXPOPUP_ARROW_LEFT:
		rect->x += arrow_w;
		break;
	case ELM_CTXPOPUP_ARROW_RIGHT:
		rect->x -= arrow_w;
		break;
	case ELM_CTXPOPUP_ARROW_UP:
		rect->y += arrow_h;
		break;
	case ELM_CTXPOPUP_ARROW_DOWN:
		rect->y -= arrow_h;
		break;
	default:
		break;
	}
}

static void _del_pre_hook(Evas_Object *obj) {

	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if (!wd) return;

	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE,
			_parent_resize, obj);

	if(wd->transit) {
		elm_transit_stop(wd->transit);
		elm_transit_del(wd->transit);
		wd->transit = NULL;
	}

	_delete_area_rect_callbacks(wd);

}

static void _del_hook(Evas_Object *obj) 
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;

	elm_ctxpopup_clear(obj);
	evas_object_del(wd->arrow);
	evas_object_del(wd->base);

	free(wd);
}

static void _theme_hook(Evas_Object *obj) 
{
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;
	char buf[256];

	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;

	//Items
	EINA_LIST_FOREACH(wd->items, elist, item)
	{
		if (item->separator)
		{
			_elm_theme_object_set(obj, item->base, "ctxpopup", "separator",
					elm_widget_style_get(obj));
			if (wd->horizontal)
				edje_object_signal_emit(item->base, "elm,state,horizontal",	"elm");
		}
		else
		{
			if (item->label && item->icon)
			{
				_elm_theme_object_set(obj, item->base, "ctxpopup",
						"icon_text_style_item",
						elm_widget_style_get(obj));
			}
			else if (item->label)
			{
				_elm_theme_object_set(obj, item->base, "ctxpopup",
						"text_style_item",
						elm_widget_style_get(obj));
			}
			else if (item->icon)
			{
				_elm_theme_object_set(obj, item->base, "ctxpopup",
						"icon_style_item",
						elm_widget_style_get(obj));
			}
			if (item->label)
				edje_object_part_text_set(item->base, "elm.text", item->label);

			if (item->disabled)
				edje_object_signal_emit(item->base, "elm,state,disabled", "elm");
		}
		edje_object_message_signal_process(item->base);
	}

	//button layout
	if (wd->btn_layout) {
		sprintf(buf, "buttons%d", wd->btn_cnt);
		_elm_theme_object_set(obj, wd->btn_layout, "ctxpopup", buf,
				elm_widget_style_get(obj));
	}

	_elm_theme_object_set(obj, wd->bg, "ctxpopup", "bg", elm_widget_style_get(obj));
	_elm_theme_object_set(obj, wd->base, "ctxpopup", "base", elm_widget_style_get(obj));
	_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow",
			elm_widget_style_get(obj));

	if (!strncmp(elm_object_style_get(obj), "default", strlen("default")
			* sizeof(char)))
		elm_object_style_set(wd->scroller, "ctxpopup");
	else
		elm_object_style_set(wd->scroller, elm_object_style_get(obj));

	if(wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(obj);
	}
}

static void _bg_clicked_cb(void *data, Evas_Object *obj, const char *emission,
		const char *source) 
{
	evas_object_hide(data);
}

static void _parent_resize(void *data, Evas *e, Evas_Object *obj,
		void *event_info) 
{
	Evas_Coord w, h;

	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(data);
	if (!wd) 	return;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	evas_object_resize(wd->bg, w, h);

   if(wd->visible == EINA_FALSE) return;
	wd->visible = EINA_FALSE;
   _hide_ctxpopup(data);

}

static void _ctxpopup_show(void *data, Evas *e, Evas_Object *obj,
		void *event_info) 
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(wd == NULL) return;

	if ((eina_list_count(wd->items) < 1) && (!wd->content) && (wd->btn_cnt < 1))
		return;

	wd->visible = EINA_TRUE;

	if (!wd->screen_dimmed_disabled) {
		evas_object_show(wd->bg);
		edje_object_signal_emit(wd->bg, "elm,state,show", "elm");
	}

	evas_object_show(wd->base);

	if (!wd->position_forced) {
		evas_object_show(wd->arrow);
	}

	_sizing_eval(obj);

}

static void _hide_ctxpopup(Evas_Object *obj)
{
	//TODO: Consider implementing effect in edc.
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd->screen_dimmed_disabled)
		evas_object_hide(wd->bg);

	if(!wd->position_forced)
		evas_object_hide(wd->arrow);

	evas_object_hide(wd->base);

	_reset_scroller_size(wd);
	evas_object_smart_callback_call(obj, "hide", NULL);
}

static void _ctxpopup_hide(void *data, Evas *e, Evas_Object *obj, void *event_info) 
{
	Widget_Data *wd = (Widget_Data*) elm_widget_data_get(obj);
	if(wd == NULL) return;

	if(wd->visible == EINA_FALSE)	 return;

	wd->visible = EINA_FALSE;

	if (!wd->screen_dimmed_disabled)
		edje_object_signal_emit(wd->bg, "elm,state,hide", "elm");

	_hide_effect(obj);
}

static void _ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object * obj,
		void *event_info) 
{
	Widget_Data *wd;
	Evas_Coord w, h;

	wd = elm_widget_data_get(data);
	if(wd == NULL) return;

	if(!wd->visible) return;
	if(wd->finished) return;

	evas_object_geometry_get(wd->scroller, 0, 0, &w, &h);

	if( w != 0 && h !=0 ) {
		if((w <= wd->max_sc_w) && (h <= wd->max_sc_h) ) {
			_sizing_eval(data);
			wd->finished = EINA_TRUE;
			return ;
		}
	}

	if(wd->max_sc_w < w ) 	wd->max_sc_w = w;
	if(wd->max_sc_h < h ) wd->max_sc_h = h;

	_sizing_eval(data);
}

static void _ctxpopup_move(void *data, Evas *e, Evas_Object *obj,
		void *event_info) 
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(wd == NULL) return;

	if (wd->visible && !wd->position_forced)
		evas_object_show(wd->arrow);

	_reset_scroller_size(wd);
	_sizing_eval(obj);
}

static void _ctxpopup_item_select(void *data, Evas_Object *obj,
		const char *emission, const char *source) 
{
	Elm_Ctxpopup_Item *item = (Elm_Ctxpopup_Item *) data;

	if (!item) return;
	if (item->disabled) return;

	if (item->func) { 
		_ctxpopup_hide(item->ctxpopup, NULL, item->ctxpopup, NULL);
		item->func(item->data, item->ctxpopup, item);
	}
}

static void _item_obj_create(Elm_Ctxpopup_Item *item, char *group_name) 
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);

	if (!wd) return;
	item->base = edje_object_add(evas_object_evas_get(wd->base));
	_elm_theme_object_set(item->ctxpopup, item->base, "ctxpopup", group_name,
			elm_widget_style_get(item->ctxpopup));
	edje_object_signal_callback_add(item->base, "elm,action,click", "",
			_ctxpopup_item_select, item);
	evas_object_size_hint_align_set(item->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(item->base, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_show(item->base);
}

static void _content_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(data);
	Evas_Coord w, h;

	if(!wd) return;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	evas_object_size_hint_min_set(obj, w, h);
	
	if(wd->visible) _sizing_eval(data);
}

static void _content_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	elm_ctxpopup_content_unset(data);
}

/**
 * Get the icon object for the given item.
 *
 * @param[in] item 	Ctxpopup item
 * @return 		Icon object or NULL if the item does not have icon
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_item_icon_get(Elm_Ctxpopup_Item *item) 
{
	if (!item) return NULL;
	return item->icon;
}

/**
 * Disable or Enable the scroller for contextual popup.
 *
 * @param[in] obj 		Ctxpopup object
 * @param[in] disabled  disable or enable
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_scroller_disabled_set(Evas_Object *obj,
		Eina_Bool disabled) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd)	return;
	if (wd->scroller_disabled == disabled)	return;

	if (disabled)
		elm_object_scroll_freeze_push(wd->scroller);
	else
		elm_object_scroll_freeze_pop(wd->scroller);

	wd->scroller_disabled = disabled;
}

/**
 * Get the label for the given item.
 *
 * @param[in] item 	 	Ctxpopup item
 * @return 		Label or NULL if the item does not have label
 *
 * @ingroup Ctxpopup
 *
 */
EAPI const char *
elm_ctxpopup_item_label_get(Elm_Ctxpopup_Item *item) 
{
	if (!item) return NULL;
	return item->label;
}

/**
 * Add a new ctxpopup object to the parent.
 *
 * @param[in] parent	window object
 * @return 		New object or NULL if it cannot be created
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_add(Evas_Object *parent) 
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;
	Evas_Coord x, y, w, h;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);

	if (!e)
		return NULL;

	obj = elm_widget_add(e);
	ELM_SET_WIDTYPE(widtype, "ctxpopup");
	elm_widget_type_set(obj, "ctxpopup");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;

	//Background
	wd->bg = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->bg);
	_elm_theme_object_set(obj, wd->bg, "ctxpopup", "bg", "default");
	evas_object_geometry_get(parent, &x, &y, &w, &h);
	evas_object_move(wd->bg, x, y);
	evas_object_resize(wd->bg, w, h);
	edje_object_signal_callback_add(wd->bg, "elm,action,click", "",
			_bg_clicked_cb, obj);

	//Base
	wd->base = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->base);
	_elm_theme_object_set(obj, wd->base, "ctxpopup", "base", "default");

	//Scroller
	wd->scroller = elm_scroller_add(obj);
	elm_object_style_set(wd->scroller, "ctxpopup");
	evas_object_size_hint_align_set(wd->scroller, EVAS_HINT_FILL,
			EVAS_HINT_FILL);
	elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
	evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE,
			_ctxpopup_scroller_resize, obj);

	//Box
	wd->box = elm_box_add(obj);
	evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	elm_scroller_content_set(wd->scroller, wd->box);
	edje_object_part_swallow(wd->base, "elm.swallow.scroller", wd->scroller);

	//Arrow
	wd->arrow = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->arrow);
	_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow", "default");

	wd->arrow_priority[0] = ELM_CTXPOPUP_ARROW_DOWN;
	wd->arrow_priority[1] = ELM_CTXPOPUP_ARROW_RIGHT;
	wd->arrow_priority[2] = ELM_CTXPOPUP_ARROW_LEFT;
	wd->arrow_priority[3] = ELM_CTXPOPUP_ARROW_UP;

	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _ctxpopup_show, NULL);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _ctxpopup_hide, NULL);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _ctxpopup_move, NULL);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _ctxpopup_changed_size_hints, NULL);
	
	return obj;
}

/**
 * Clear all items in given ctxpopup object.
 *
 * @param[in] obj 		Ctxpopup object
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_clear(Evas_Object *obj) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;

	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, elist, item)
	{
		if (item->label)
		eina_stringshare_del(item->label);
		if (item->icon)
		evas_object_del(item->icon);
		wd->items = eina_list_remove(wd->items, item);
		free(item);
	}

	evas_object_hide(wd->arrow);
	evas_object_hide(wd->base);
}

/**
 * Change the mode to horizontal or vertical.
 *
 * @param[in] obj   	Ctxpopup object
 * @param horizontal 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_horizontal_set(Evas_Object *obj, Eina_Bool horizontal) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;

	if (wd->horizontal == horizontal) return;
	wd->horizontal = horizontal;
	if (!horizontal) {
		elm_box_horizontal_set(wd->box, EINA_FALSE);
		elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
		EINA_LIST_FOREACH	(wd->items, elist, item)
		edje_object_signal_emit(item->base, "elm,state,vertical", "elm");
	}
	else
	{
		elm_box_horizontal_set(wd->box, EINA_TRUE);
		elm_scroller_bounce_set(wd->scroller, EINA_TRUE, EINA_FALSE);
		EINA_LIST_FOREACH(wd->items, elist, item)
		edje_object_signal_emit(item->base, "elm,state,horizontal", "elm");
	}	
}

/**
 * Get the value of current horizontal mode.
 *
 * @param[in] obj 	 	Ctxpopup object
 * @return 	 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode.
 *
 * @ingroup Ctxpopup
 */
EAPI Eina_Bool elm_ctxpopup_horizontal_get(Evas_Object *obj) 
{
	ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if (!wd) return EINA_FALSE;
	return wd->horizontal;
}

/**
 * reset the icon on the given item. 
 *
 * @param[in] item 	 	Ctxpopup item
 * @param[in] icon		Icon object to be set
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_item_icon_set(Elm_Ctxpopup_Item *item, Evas_Object *icon) 
{
	Widget_Data *wd;

	if (!item) return;
	wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
	if (!wd) return;
	if (item->icon == icon) return;
	if (item->icon) {
		elm_widget_sub_object_del(item->base, item->icon);
		evas_object_del(item->icon);
	}
	item->icon = icon;
	edje_object_part_swallow(item->base, "elm.swallow.icon", item->icon);
	edje_object_message_signal_process(item->base);

	if (wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(item->ctxpopup);
	}
}

/**
 * reset the label on the given item. 
 *
 * @param[in] item 	 	Ctxpopup item
 * @param[in] label		Label to be set
 * 
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_item_label_set(Elm_Ctxpopup_Item *item,
		const char *label) 
{
	Widget_Data *wd;

	if (!item)
		return;

	if (item->label) {
		eina_stringshare_del(item->label);
		item->label = NULL;
	}

	item->label = eina_stringshare_add(label);
	edje_object_message_signal_process(item->base);
	edje_object_part_text_set(item->base, "elm.text", label);

	wd = elm_widget_data_get(item->ctxpopup);
	if (!wd)
		return;

	if (wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(item->ctxpopup);
	}
}

/**
 * Add a new item in given ctxpopup object.
 *
 * @param[in] obj 	 	Ctxpopup object
 * @param[in] icon		Icon to be set
 * @param[in] label   Label to be set
 * @param[in] func		Callback function to call when this item click is clicked
 * @param[in] data    User data for callback function
 * @return 		Added ctxpopup item
 * 
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_item_add(Evas_Object *obj, Evas_Object *icon, const char *label,
		Evas_Smart_Cb func, void *data) 
{
	ELM_CHECK_WIDTYPE(obj, widtype)NULL;
	Elm_Ctxpopup_Item *item;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd)
		return NULL;

	_separator_obj_add(obj);

	item = ELM_NEW(Elm_Ctxpopup_Item);
	if (!item)
		return NULL;

	item->func = func;
	item->data = data;
	item->ctxpopup = obj;
	item->separator = EINA_FALSE;

	if (icon && label)
		_item_obj_create(item, "icon_text_style_item");
	else if (icon)
		_item_obj_create(item, "icon_style_item");
	else
		_item_obj_create(item, "text_style_item");

	wd->items = eina_list_append(wd->items, item);
	elm_box_pack_end(wd->box, item->base);
	elm_ctxpopup_item_icon_set(item, icon);
	elm_ctxpopup_item_label_set(item, label);

	return item;
}

/**
 * Delete the given item in ctxpopup object.
 *
 * @param item[in]  Ctxpopup item to be deleted
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_item_del(Elm_Ctxpopup_Item *item) 
{
	Widget_Data *wd;
	Evas_Object *obj;

	if (!item)
		return;

	obj = item->ctxpopup;

	if (item->label)
		eina_stringshare_del(item->label);
	if (item->icon)
		evas_object_del(item->icon);
	if (item->base)
		evas_object_del(item->base);

	wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
	if (wd) {
		_separator_obj_del(wd, item);
		wd->items = eina_list_remove(wd->items, item);
	}
	free(item);
	if (eina_list_count(wd->items) == 0) {
		_ctxpopup_hide(obj, NULL, obj, NULL);
	}
}

/**
 * Disable or Enable the given item. Once an item is disabled, the click event will be never happend for the item.
 *
 * @param[in] item		Ctxpopup item to be disabled
 * @param[in] disabled 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_item_disabled_set(Elm_Ctxpopup_Item *item,
		Eina_Bool disabled) 
{
	Widget_Data *wd;

	if (!item) return;
	if (disabled == item->disabled) return;

	wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
	
	if (disabled)
		edje_object_signal_emit(item->base, "elm,state,disabled", "elm");
	else
		edje_object_signal_emit(item->base, "elm,state,enabled", "elm");

	edje_object_message_signal_process(item->base);
	item->disabled = disabled;
}

/**
 * Disable or Enable background dimmed function 
 * @param[in] obj		Ctxpopup object
 * @param[in] dimmed 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_screen_dimmed_disabled_set(Evas_Object *obj,
		Eina_Bool disabled) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd)
		return;

	wd->screen_dimmed_disabled = disabled;

	if (wd->visible) {
		if (!disabled) {
			evas_object_show(wd->bg);
		}
	}
}

/**
 * Append additional button in ctxpoppup bottom layout.
 * @param[in] obj		Ctxpopup object
 * @param[in] label  Button label
 * @param[in] func   Button clicked event callback function
 * @param[in] data   Button clicked event callback function data
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_button_append(Evas_Object *obj, const char *label,
		Evas_Smart_Cb func, const void *data) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	char buf[256];
	Evas_Object *btn;
	Evas_Coord w, h;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd) return;
	if (!wd->btn_layout) _btn_layout_create(obj);

	++wd->btn_cnt;
	sprintf(buf, "buttons%d", wd->btn_cnt);
	_elm_theme_object_set(obj, wd->btn_layout, "ctxpopup", buf,
			elm_widget_style_get(obj));

	btn = elm_button_add(obj);
	elm_object_style_set(btn, "text_only/style1");
	elm_button_label_set(btn, label);
	evas_object_smart_callback_add(btn, "clicked", func, data);
	sprintf(buf, "actionbtn%d", wd->btn_cnt);
	edje_object_part_swallow(wd->btn_layout, buf, btn);

	edje_object_part_geometry_get(wd->btn_layout, buf, NULL, NULL, &w, &h);
	evas_object_size_hint_max_set(wd->btn_layout, -1, h);

	if (wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(obj);
	}
}

/**
 * Set the priority of arrow direction
 *
 *  This functions gives user to set the priority of ctxpopup box showing position.
 *
 * @param[in] obj		Ctxpopup object
 * @param[in] first    1st priority of arrow direction
 * @param[in] second 2nd priority of arrow direction
 * @param[in] third   3th priority of arrow direction
 * @param[in] fourth 4th priority of arrow direction
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_arrow_priority_set(Evas_Object *obj,
		Elm_Ctxpopup_Arrow first, Elm_Ctxpopup_Arrow second,
		Elm_Ctxpopup_Arrow third, Elm_Ctxpopup_Arrow fourth) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	if (!wd)
		return;
	wd->arrow_priority[0] = first;
	wd->arrow_priority[1] = second;
	wd->arrow_priority[2] = third;
	wd->arrow_priority[3] = fourth;
}

/**
 * Swallow the user content
 *
 * @param[in] obj		Ctxpopup object
 * @param[in] content 	Content to be swallowed
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_content_set(Evas_Object *obj, Evas_Object *content) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	Evas_Coord w, h;
	if(!wd || !content) return;

	evas_object_event_callback_add(content, EVAS_CALLBACK_DEL, _content_del, obj);
	evas_object_event_callback_add(content, EVAS_CALLBACK_RESIZE, _content_resize, obj);

	evas_object_geometry_get(content, NULL, NULL, &w, &h);	
	
	if((w > 0) || (h > 0)) evas_object_size_hint_min_set(content, w, h);
	
	edje_object_part_swallow(wd->base, "elm.swallow.content", content);
	elm_widget_sub_object_add(obj, content);
	edje_object_signal_emit(wd->base, "elm,state,content,enable", "elm");
	elm_ctxpopup_scroller_disabled_set(obj, EINA_TRUE);
	wd->content = content;

	if(wd->visible) _sizing_eval(obj);
}

/**
 * Unswallow the user content
 *
 * @param[in] obj		Ctxpopup object
 * @return 		The unswallowed content
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_content_unset(Evas_Object *obj) 
{
	ELM_CHECK_WIDTYPE(obj, widtype)NULL;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	Evas_Object *content;

	content = wd->content;
	
	if(!content) return NULL;

	edje_object_part_unswallow(wd->base, content);
	elm_widget_sub_object_del(obj, content);
	evas_object_event_callback_del(content, EVAS_CALLBACK_DEL, _content_del);
	evas_object_event_callback_del(content, EVAS_CALLBACK_RESIZE, _content_resize); 
	edje_object_signal_emit(wd->base, "elm,state,content,disable", "elm");

	elm_ctxpopup_scroller_disabled_set(obj, EINA_FALSE);

	if (wd->visible)
		_sizing_eval(obj);

	return content;
}

/**
 * Change the origin of the ctxpopup position.
 *
 * Basically, ctxpopup position is computed internally. When user call evas_object_move,
 * Ctxpopup will be showed up with that position which is indicates the arrow point.
 *
 * @param[in] obj		Ctxpopup object
 * @param[in] forced	EINA_TRUE is left-top. EINA_FALSE is indicates arrow point.
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_position_forced_set(Evas_Object *obj, Eina_Bool forced) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	wd->position_forced = forced;

	if (forced) evas_object_hide(wd->arrow);

	if (wd->visible) {
		_reset_scroller_size(wd);
		_sizing_eval(obj);
	}
}

/**
 * Get the status of the position forced
 *
 * @param[in] obj		Ctxpopup objet
 * @return			value of position forced
 *
 * @ingroup Ctxpopup
 */
EAPI Eina_Bool elm_ctxpopup_position_forced_get(Evas_Object *obj) 
{
	ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	return wd->position_forced;
}

EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_icon_add(Evas_Object *obj, Evas_Object *icon, Evas_Smart_Cb func,
		void *data) 
{
	return elm_ctxpopup_item_add(obj, icon, NULL, func, data);
}

EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_label_add(Evas_Object *obj, const char *label, Evas_Smart_Cb func,
		void *data) 
{
	return elm_ctxpopup_item_add(obj, NULL, label, func, data);
}

/**
 * Set the area of ctxpopup will show up. Ctxpopup will not be out of this area. 
 * The responsibility of the area object is to user.
 *
 * @param[in] obj		Ctxpopup objet
 * @param[in] area		area object
 *
 * @ingroup Ctxpopup
 */
EAPI void elm_ctxpopup_area_set(Evas_Object *obj, Evas_Object *area) 
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	_delete_area_rect_callbacks(wd);

	if (area) {
		evas_object_event_callback_add(area, EVAS_CALLBACK_DEL, _area_rect_del,
				obj);
		evas_object_event_callback_add(area, EVAS_CALLBACK_MOVE,
				_area_rect_move, obj);
		evas_object_event_callback_add(area, EVAS_CALLBACK_RESIZE,
				_area_rect_resize, obj);
		wd->area_rect = area;
	}
}

EAPI void elm_ctxpopup_title_set(Evas_Object *obj, const char *title )
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(!wd) return;
	if(!title) return;

	if(wd->title) eina_stringshare_del(wd->title);
	wd->title = eina_stringshare_add(title);
	edje_object_part_text_set(wd->base, "elm.title", title);
}

EAPI const char* elm_ctxpopup_title_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	return wd->title; 
}
	

