#include <Elementary.h>
#include "elm_priv.h"


/**
 * @defgroup Stackedicon Stackedicon
 * @ingroup Elementary
 *
 * This is a Stackedicon.
 */


#define MAX_ITEM_NUM		(9)
#define MAX_MOVE_INTERVAL	(1.8)
#define ELM_MAX(v1, v2)    	(((v1) > (v2)) ? (v1) : (v2))
#define ROT_RIGHT			(5)
#define ROT_LEFT			(-5)

struct _Stackedicon_Item {
	Evas_Object *parent;
	Evas_Object *ly;
	Evas_Object *ic;	
	Evas_Object *pad;	
	const char *path;
	int index;
	Evas_Coord x, y, w, h;
	Evas_Coord mw, mh;	
	Eina_Bool on_hold : 1;
	Eina_Bool on_show : 1;	
	int mdx, mdy, mmx, mmy;	
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {	
	Evas_Object *bx;
	double ewma;
	int interval_x;
	int interval_y;	
	unsigned int time; 
	Ecore_Animator *animator;		
	Eina_List *list;
	Evas_Coord x, y, w, h;
	Eina_Bool on_expanded : 1;
};

static const char *widtype = NULL;



static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static unsigned int _current_time_get(void);
static void _icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_move_to_zero(void *data);
static int _icon_move_to_zero_cb(void *data);
static void _icon_move_map(void *data, int flag, int interval_x, int interval_y);
static void _icon_map_pos(void *data, Evas_Coord x, Evas_Coord y);
static void _del_image(void *data);
static void _del_all_image(void *data);
static void _add_image(Evas_Object *obj, void *data);
static void _show_all_image(Evas_Object *obj);
static void _hide_all_image(Evas_Object *obj);
static void _update_stackedicon(Evas_Object *obj);
static void _resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _show_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);
static void _hide_cb(void *data, Evas * e, Evas_Object * obj, void *event_info);
static void _event_init(Evas_Object *obj);



static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Elm_Stackedicon_Item *it;
	if(!wd) return;

	_del_all_image(wd);
	
	if(wd->animator) {
		ecore_animator_del(wd->animator);
		wd->animator = NULL;
	}
	
	if(wd->list){
		EINA_LIST_FOREACH(wd->list, l, it)
			if(it) free(it);				
		eina_list_free(wd->list);
		wd->list = NULL;
	}
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Elm_Stackedicon_Item *it;
	if (!wd) return;
	
	EINA_LIST_FOREACH(wd->list, l, it) {
		if(it->ly)	_elm_theme_object_set(obj, it->ly, "stackedicon", "icon", elm_widget_style_get(obj));
		if(it->ic)	edje_object_part_swallow(it->ly, "contents", it->ic);
		if(it->pad)	edje_object_part_swallow(it->ly, "shadow", it->pad);
		edje_object_scale_set(it->ly, elm_widget_scale_get(obj) * _elm_config->scale);
	}
	_update_stackedicon(obj);
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	Evas_Coord w, h;
	if (!wd) return;

	edje_object_size_min_restricted_calc(wd->bx, &minw, &minh, minw, minh);
	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minh) minh = h;
	evas_object_size_hint_min_set(obj, minw, minh);
}

static void _del_image(void *data)
{
	Elm_Stackedicon_Item *it = (Elm_Stackedicon_Item *)data;
			
	if(it->ly) {
		evas_object_del(it->ly);
		evas_object_del(it->ic);				
		evas_object_del(it->pad);		
		it->ly = NULL;
		it->ic = NULL;	
		it->pad = NULL;			
		it->on_show = EINA_FALSE;
	}
}

static void _del_all_image(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	int i = 0;
			
	for(i =0; i < eina_list_count(wd->list); i++)
	{
		Elm_Stackedicon_Item *it = NULL;
		it = (Elm_Stackedicon_Item *)eina_list_nth(wd->list, i);

		if(it != NULL){
			if(it->on_show == EINA_TRUE){	
				_del_image(it);
			}
		}
	}
}

static void _icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Stackedicon_Item *it = data;
	if(!it)	return;
	Widget_Data *wd = elm_widget_data_get(it->parent);
	if(!wd) return;
	
	Evas_Coord x, y;
	
	if(it->on_show != EINA_FALSE){
		if(it->ly) {
			evas_object_geometry_get(obj, &x, &y, NULL, NULL);
			_icon_map_pos(it, x, y);
		}
	}	
}

static void _icon_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Stackedicon_Item *it = data;
	Evas_Event_Mouse_Down *ev = event_info;
	if(!it)	return;

	it->on_hold = EINA_TRUE;	
	it->mdx = ev->output.x;
	it->mdy = ev->output.y;
}

static void _icon_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Stackedicon_Item *it = data;
	if(!it)	return;
	Widget_Data *wd = elm_widget_data_get(it->parent);
	Evas_Event_Mouse_Move *ev = event_info;
	if(!wd || !ev->buttons) return;
	
	if(it->on_hold == EINA_TRUE){
		it->mmx = ev->cur.output.x;
		it->mmy = ev->cur.output.y;

		wd->interval_x = 1.5*(it->mmx - it->mdx);
		wd->interval_y = 1.5*(it->mmy - it->mdy);		

		_icon_move_map(wd, 1, wd->x + wd->interval_x, wd->y +  wd->interval_y);
	}	
}

static void _icon_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Stackedicon_Item *it = data;
	if(!it)	return;
	Widget_Data *wd = elm_widget_data_get(it->parent);
	double interval = 0.0;
	if(!wd) return;

	interval = sqrt(wd->interval_x*wd->interval_x + wd->interval_y*wd->interval_y);
	
	if(((double)(interval/it->h) > MAX_MOVE_INTERVAL) && (wd->on_expanded == EINA_FALSE)){
		wd->on_expanded = EINA_TRUE;
		wd->interval_x = 0;
		wd->interval_y = 0;
		
		_icon_move_map(wd, 0, wd->x, wd->y);		

		evas_object_smart_callback_call(it->parent, "expanded", NULL);		
	}else{		
		wd->on_expanded= EINA_FALSE;	
		it->on_hold = EINA_FALSE;	
		it->mdx = 0;
		it->mdy = 0;
		it->mmx = 0;
		it->mmx = 0;	
		
		if(wd->animator) {
			ecore_animator_del(wd->animator);
			wd->animator = NULL;
		}
		wd->time = _current_time_get();
		wd->animator= ecore_animator_add(_icon_move_to_zero_cb, wd);	
	}	
}

static unsigned int _current_time_get(void)
{
	struct timeval timev;

	gettimeofday(&timev, NULL);
	return ((timev.tv_sec * 1000) + ((timev.tv_usec) / 1000));
}

static void _icon_move_to_zero(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	double t;
	int x, y;
	if(!wd) return;
	
	t = ELM_MAX(0.0, _current_time_get() - wd->time) / 100;

	if (t <= 1.0){
		x = (1 * sin((t / 2.0) * (M_PI / 2)) * wd->interval_x);
		y = (1 * sin((t / 2.0) * (M_PI / 2)) * wd->interval_y);				
	}else{
		x = wd->interval_x;
		y = wd->interval_y;
	}
	
	if( y == wd->interval_y){
		ecore_animator_del(wd->animator);
		wd->animator = NULL;
		wd->interval_x = 0;
		wd->interval_y = 0;		
		_icon_move_map(wd, 0, wd->x, wd->y);
	}else{	
		_icon_move_map(wd, 0, wd->x + wd->interval_x - x, wd->y + wd->interval_y - y);
	}
}

static int _icon_move_to_zero_cb(void *data)
{	
	_icon_move_to_zero(data);
	
	return EXIT_FAILURE;
}

static void _icon_move_map(void *data, int flag, int interval_x, int interval_y)
{
	Widget_Data *wd = (Widget_Data *)data;
	int i = 0;
	int num = eina_list_count(wd->list);
	int x = 0, y = 0;
	if(!wd) return;
	
	for(i =0; i  < num; i++)
	{
		Elm_Stackedicon_Item *it = NULL;
		it = (Elm_Stackedicon_Item *)eina_list_nth(wd->list, i);

		if(it != NULL){
			x = wd->x  + wd->w/2 - it->mw/2 + ((interval_x - wd->x)/num)*(num -i);
			y = wd->y + wd->h/2 - it->mh/2 + ((interval_y - wd->y)/num)*(num -i);
			evas_object_move(it->ly, x, y);
		}
	}
}

static void _icon_map_pos(void *data, Evas_Coord x, Evas_Coord y)
{
	Elm_Stackedicon_Item *it = data;
	Evas_Map *m;
	int degree = 0;

	if((it->index % 3) == 1)
		degree = ROT_RIGHT;
	else if ((it->index % 3) == 2)
		degree = ROT_LEFT;
		
	m = evas_map_new(4);
	evas_map_util_points_populate_from_geometry(m, x, y, it->w, it->h, 0);
	evas_map_util_3d_rotate(m, 0, 0, degree, x + it->w/2, y + it->h/2, 0);
	evas_map_util_3d_perspective(m, x + it->w/2, y + it->h/2, 0, 10000);
	evas_map_smooth_set(m, 1);
	evas_map_alpha_set(m, 1);	
	evas_object_map_set(it->ly, m);
	evas_object_map_enable_set(it->ly, 1);
	//evas_object_show(it->ly);
	evas_map_free(m);	
}

static void _add_image(Evas_Object *obj, void *data)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *ly = NULL;
	Evas_Object *ic = NULL;
	Evas_Object *pad = NULL;	
	int w, h;
	if (!wd) return;

	Elm_Stackedicon_Item *it = (Elm_Stackedicon_Item *)data;

	ly = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj, ly, "stackedicon", "icon", elm_widget_style_get(obj));
	evas_object_size_hint_weight_set(ly, 1.0, 1.0);
	elm_widget_sub_object_add(obj, ly); 
	
	ic = evas_object_image_add(evas_object_evas_get(obj));
	evas_object_image_load_size_set(ic, wd->w/2, wd->h/2);
	evas_object_image_file_set(ic, it->path, NULL);
	evas_object_image_size_get(ic, &w, &h);

	if(w>h){
		if((wd->w -2)*h/w > wd->h -2){
			evas_object_image_fill_set(ic, 0, 0, (wd->h -2)*w/h, wd->h -2);
			evas_object_size_hint_min_set(ly, (wd->h -2)*w/h, wd->h -2);
			evas_object_resize(ly, (wd->h -2)*w/h, wd->h -2);	

			it->w = (wd->h -2)*w/h;
			it->h = wd->h -2;			
		}else{
			evas_object_image_fill_set(ic, 0, 0, wd->w -2, (wd->w -2)*h/w);
			evas_object_size_hint_min_set(ly, wd->w-2, (wd->w -2)*h/w);
			evas_object_resize(ly, wd->w-2, (wd->w -2)*h/w);	

			it->w = wd->w-2;
			it->h = (wd->w -2)*h/w;						
		}		
	}else{
		if((wd->h -2)*w/h > wd->w -2){
			evas_object_image_fill_set(ic, 0, 0, wd->w -2, (wd->w -2)*(wd->h -2)/((wd->h -2)*w/h));
			evas_object_size_hint_min_set(ly, wd->w -2, (wd->w -2)*(wd->h -2)/((wd->h -2)*w/h));
			evas_object_resize(ly, wd->w -2, (wd->w -2)*(wd->h -2)/((wd->h -2)*w/h));

			it->w = wd->w-2;
			it->h = (wd->w -2)*(wd->h -2)/((wd->h -2)*w/h);				
		}else{
			evas_object_image_fill_set(ic, 0, 0, (wd->h -2)*w/h, wd->h -2);
			evas_object_size_hint_min_set(ly, (wd->h -2)*w/h, wd->h -2);
			evas_object_resize(ly, (wd->h -2)*w/h, wd->h -2);	

			it->w = (wd->h -2)*w/h;
			it->h = wd->h -2;				
		}
	}
	evas_object_image_filled_set(ic, 1);
	edje_object_part_swallow(ly, "contents", ic);
	
	pad = evas_object_rectangle_add(evas_object_evas_get(obj));
	evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(pad, 0, 0, 0, it->index*25);
	edje_object_part_swallow(ly, "shadow", pad);
	
	it->mw = it->w;
	it->mh = it->h;
	
	evas_object_event_callback_add(ly, EVAS_CALLBACK_MOVE, _icon_move_cb, it);	

	if(it->index == 0){
		evas_object_event_callback_add(ly, EVAS_CALLBACK_MOUSE_DOWN, _icon_mouse_down_cb, it);
		evas_object_event_callback_add(ly, EVAS_CALLBACK_MOUSE_MOVE, _icon_mouse_move_cb, it);
		evas_object_event_callback_add(ly, EVAS_CALLBACK_MOUSE_UP, _icon_mouse_up_cb, it);
	}
	
	it->ly = ly;
	it->ic = ic;		
	it->pad = pad;
	
	it->on_show = EINA_TRUE;
}

static void _update_stackedicon(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Elm_Stackedicon_Item *it = NULL;
	if (!wd || ((wd->w == 1) && (wd->h == 1))) return;

	EINA_LIST_REVERSE_FOREACH(wd->list, l, it) {
		if(it != NULL){
			if(it->on_show == EINA_FALSE){	
				_add_image(obj, it);
			}
			evas_object_move(it->ly, wd->x + wd->w/2 - it->mw/2, wd->y + wd->h/2 - it->mh/2);	
		}
	}
}

static void _show_all_image(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	int i = 0;
			
	for(i =0; i < eina_list_count(wd->list); i++)
	{
		Elm_Stackedicon_Item *it = NULL;
		it = (Elm_Stackedicon_Item *)eina_list_nth(wd->list, i);

		if(it != NULL){
			if(it->on_show == EINA_TRUE){	
				evas_object_show(it->ly);
				//evas_object_show(it->ic);
				//evas_object_show(it->pad);				
			}
		}
	}
}

static void _hide_all_image(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	int i = 0;
			
	for(i =0; i < eina_list_count(wd->list); i++)
	{
		Elm_Stackedicon_Item *it = NULL;
		it = (Elm_Stackedicon_Item *)eina_list_nth(wd->list, i);

		if(it != NULL){
			if(it->on_show == EINA_TRUE){	
				evas_object_hide(it->ly);
				//evas_object_hide(it->ic);
				//evas_object_hide(it->pad);				
			}
		}
	}
}

static void
_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Coord w, h;	
	if (!wd) return;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);	
	wd->w = w;
	wd->h = h;
	evas_object_resize(wd->bx, w, h);	
	_update_stackedicon(data);
}

static void
_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Coord x, y;	
	if (!wd) return;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);	
	wd->x = x;
	wd->y = y;
	evas_object_move(wd->bx, x, y);
	_update_stackedicon(data);	
}

static void
_show_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	_update_stackedicon(data);	
	_show_all_image(data);
}

static void
_hide_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	_hide_all_image(data);
}

static void
_event_init(Evas_Object *obj)
{		
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move_cb, obj);	
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show_cb, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide_cb, obj);
}

/**
 * Add a new stackedicon to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Stackedicon
 */
EAPI Evas_Object *
elm_stackedicon_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	ELM_SET_WIDTYPE(widtype, "stackedicon");
	elm_widget_type_set(obj, "stackedicon");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
		
	wd->bx = elm_box_add(obj);
	elm_widget_resize_object_set(obj, wd->bx);
	evas_object_size_hint_weight_set(wd->bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	wd->w = 1;
	wd->h = 1;
	wd->on_expanded = EINA_FALSE;
	
	_event_init(obj);
	_sizing_eval(obj);

	return obj;
}

/**
 * This appends a path to the stackedicon
 *
 * @param 	obj	The stackedicon object
 * @param 	path	The image full path
 * @return	The new item or NULL if it cannot be created
 *
 * @ingroup Stackedicon
 */
EAPI Elm_Stackedicon_Item *elm_stackedicon_item_append(Evas_Object *obj, const char *path)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it;
	if (!wd) return NULL;
	if(eina_list_count(wd->list) >= MAX_ITEM_NUM) return NULL;
	
	it = (Elm_Stackedicon_Item *)calloc(1, sizeof(Elm_Stackedicon_Item));
	it->path = eina_stringshare_add(path);
	it->parent = obj;
	it->ly = NULL;
	it->ic = NULL;
	it->index = eina_list_count(wd->list);
	it->on_show = EINA_FALSE;	
	wd->list = eina_list_append(wd->list, it);

	//_update_stackedicon(obj);

	return it;
}

/**
 * This prepends a path to the stackedicon
 *
 * @param 	obj	The stackedicon object
 * @param 	path	The image full path
 * @return	The new item or NULL if it cannot be created
 *
 * @ingroup Stackedicon
 */
EAPI Elm_Stackedicon_Item *elm_stackedicon_item_prepend(Evas_Object *obj, const char *path)
{	
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it;
	if (!wd) return NULL;
	if(eina_list_count(wd->list) >= MAX_ITEM_NUM) return NULL;
	
	it = (Elm_Stackedicon_Item *)calloc(1, sizeof(Elm_Stackedicon_Item));
	it->path = eina_stringshare_add(path);
	it->parent = obj;
	it->ly = NULL;
	it->ic = NULL;
	it->index = eina_list_count(wd->list);
	it->on_show = EINA_FALSE;	
	wd->list = eina_list_prepend(wd->list, it);

	//_update_stackedicon(obj);

	return it;
}

/**
 * This delete a path at the stackedicon
 *
 * @param 	Elm_Stackedicon_Item	The delete item
 *
 * @ingroup Stackedicon
 */
EAPI void elm_stackedicon_item_del(Elm_Stackedicon_Item *it)
{
	Widget_Data *wd;
	if (!it || !(wd = elm_widget_data_get(it->parent)))
		return;

	wd->list = eina_list_remove(wd->list, it);
}

/**
 * Get item list from the stackedicon
 *
 * @param 	obj	The stackedicon object
 * @return	The item list or NULL if it cannot be created 
 *
 * @ingroup Stackedicon
 */
EAPI Eina_List *elm_stackedicon_item_list_get(Evas_Object *obj)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	
	return wd->list;
}

