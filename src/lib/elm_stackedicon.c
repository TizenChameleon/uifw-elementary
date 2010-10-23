#include <Elementary.h>
#include "elm_priv.h"


/**
 * @defgroup Stackedicon Stackedicon
 * @ingroup Elementary
 *
 * This is a Stackedicon.
 */


#define MAX_ITEM_NUM		(9)
#define MAX_MOVE_INTERVAL	(0.2)
#define ELM_MAX(v1, v2)    	(((v1) > (v2)) ? (v1) : (v2))
#define ROT_RIGHT			(5)
#define ROT_LEFT			(-5)
#define MAX_SHOWN_ITEM		(3)			


struct _Stackedicon_Item {
	Evas_Object *parent;
	Evas_Object *ly;
	Evas_Object *ic;	
	Evas_Object *pad;	
	const char *path;
	int index;
	Evas_Coord x, y, w, h;
	Evas_Coord mw, mh;	
	Eina_Bool exist : 1;	
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {	
	Evas_Object *base;
	int interval_x, interval_y;	
	unsigned int time; 
	Ecore_Animator *animator;		
	Eina_List *list;
	Evas_Coord x, y, w, h;
	Eina_Bool visible: 1;

	/*  fake img */
	Evas_Object *fake_img;
	int r, g, b, a;
	int mdx, mdy, mmx, mmy;
	Eina_Bool move_start: 1;
	Eina_Bool on_update;
};

static const char *widtype = NULL;


static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _del_image(void *data);
static void _del_all_image(void *data);
static unsigned int _current_time_get(void);
static void _icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_move_to_zero(Evas_Object *obj);
static Eina_Bool _icon_move_to_zero_cb(void *data);
static void _icon_move_map(void *data, int interval_x, int interval_y);
static void _icon_map_pos(Evas_Object *obj, int index, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
static void _calc_item_size(int w, int h, int iw, int ih, int *res_w, int *res_h);
static void _add_image(Evas_Object *obj, void *data);
static void _fake_img_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _fake_img_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _fake_img_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _add_image_to_buffer(Evas_Object *obj, Evas* e, void *data);
static Evas_Object * _create_fake_image(Evas_Object *obj);
static void _show_all_image(Evas_Object *obj);
static void _hide_all_image(Evas_Object *obj);
static void _hide_hidden_image(Evas_Object *obj);
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
	
	if(wd->animator) {
		ecore_animator_del(wd->animator);
		wd->animator = NULL;
	}

	_del_all_image(wd);
	
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

	_elm_theme_object_set(obj, wd->base, "stackedicon", "base", elm_widget_style_get(obj));
	if(wd->fake_img) edje_object_part_swallow(wd->base, "elm.bg.swallow", wd->fake_img);
	edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);
	
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

	edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minh) minh = h;
	evas_object_size_hint_min_set(obj, minw, minh);
}

static void 
_del_image(void *data)
{
	Elm_Stackedicon_Item *it = (Elm_Stackedicon_Item *)data;
		
	if(it->ly) {
		evas_object_del(it->ly);
		evas_object_del(it->ic);				
		evas_object_del(it->pad);		
		it->ly = NULL;
		it->ic = NULL;	
		it->pad = NULL;			
		it->exist = EINA_FALSE;
	}
}

static void 
_del_all_image(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;	
	Eina_List *l;
	Elm_Stackedicon_Item *it = NULL;
	if(!wd) return;
	
	EINA_LIST_FOREACH(wd->list, l, it)
		if(it && it->exist) _del_image(it);
}

static void 
_icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Stackedicon_Item *it = data;
	if(!it)	return;
	Widget_Data *wd = elm_widget_data_get(it->parent);
	if(!wd) return;
	
	Evas_Coord x, y;
	
	if(it->exist && it->ly){
		evas_object_geometry_get(obj, &x, &y, NULL, NULL);
		_icon_map_pos(it->ly, it->index, x, y, it->w, it->h);	
	}	
}

static void 
_fake_img_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Down *ev = event_info;
	Eina_List *l;
	Elm_Stackedicon_Item *it = NULL;
	if(!wd) return;

	wd->mdx = ev->output.x;
	wd->mdy = ev->output.y;

	it = NULL;
	EINA_LIST_REVERSE_FOREACH(wd->list, l, it) {
		if(it){
			if(!it->exist) _add_image(data, it);
			evas_object_move(it->ly, wd->x + wd->w/2 - it->mw/2, wd->y + wd->h/2 - it->mh/2);	
			if(wd->visible) evas_object_show(it->ly);
		}
	}

	EINA_LIST_REVERSE_FOREACH(wd->list, l, it)
		if(it && it->exist) evas_object_raise(it->ly);

	evas_object_color_set(wd->fake_img, 0, 0, 0, 0);	
	wd->move_start = TRUE;
}

static void 
_fake_img_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Move *ev = event_info;
	if(!wd || !ev->buttons) return;
	
	if(wd->move_start == TRUE){
		evas_object_smart_callback_call(data, "drag,start", NULL);
		_show_all_image(data);
		wd->move_start = FALSE;
	}

	wd->mmx = ev->cur.output.x;
	wd->mmy = ev->cur.output.y;

	wd->interval_x = wd->mmx - wd->mdx;
	wd->interval_y = wd->mmy - wd->mdy; 	

	_icon_move_map(wd, wd->x + wd->interval_x, wd->y +  wd->interval_y);
}

static void 
_fake_img_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	double interval = 0.0;
	if(!wd) return;

	interval = sqrt(wd->interval_x*wd->interval_x + wd->interval_y*wd->interval_y);
	
	if(((double)(interval/wd->h) > MAX_MOVE_INTERVAL)){
		wd->interval_x = 0;
		wd->interval_y = 0;
		
		_icon_move_map(wd, wd->x, wd->y);		
		_hide_hidden_image(data);
		evas_object_smart_callback_call(data, "expanded", NULL);	
		evas_object_smart_callback_call(data, "drag,stop", NULL);
	}else{			
		wd->mdx = 0;
		wd->mdy = 0;
		wd->mmx = 0;
		wd->mmx = 0;	
		
		if(wd->animator) {
			ecore_animator_del(wd->animator);
			wd->animator = NULL;
		}
		wd->time = _current_time_get();
		wd->animator= ecore_animator_add(_icon_move_to_zero_cb, data);	
	}	
}

static unsigned int 
_current_time_get(void)
{
	struct timeval timev;

	gettimeofday(&timev, NULL);
	return ((timev.tv_sec * 1000) + ((timev.tv_usec) / 1000));
}

static void 
_icon_move_to_zero(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
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
		_icon_move_map(wd, wd->x, wd->y);
		_hide_hidden_image(obj);
		evas_object_smart_callback_call(obj, "clicked", NULL);
		evas_object_smart_callback_call(obj, "drag,stop", NULL);
	}else{	
		_icon_move_map(wd, wd->x + wd->interval_x - x, wd->y + wd->interval_y - y);
	}
}

static Eina_Bool 
_icon_move_to_zero_cb(void *data)
{
    Evas_Object *obj = (Evas_Object *)data;
	_icon_move_to_zero(obj);
	
	return EXIT_FAILURE;
}

static void 
_icon_move_map(void *data, int interval_x, int interval_y)
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

static void 
_icon_map_pos(Evas_Object *obj, int index, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
	Evas_Map *m;
	int degree = 0;

	if((index % 3) == 1)
		degree = ROT_RIGHT;
	else if ((index % 3) == 2)
		degree = ROT_LEFT;

	m = evas_map_new(4);
	evas_map_util_points_populate_from_geometry(m, x, y, w, h, 0);
	evas_map_util_3d_rotate(m, 0, 0, degree, x + w/2, y + h/2, 0);
	evas_map_util_3d_perspective(m, x + w/2, y + h/2, 0, 10000);
	evas_map_smooth_set(m, 1);
	evas_map_alpha_set(m, 1);
	evas_object_map_set(obj, m);
	evas_object_map_enable_set(obj, 1);
	evas_map_free(m);	
}

static void 
_calc_item_size(int w, int h, int iw, int ih, int *res_w, int *res_h)
{
	if(iw>ih){
		if(w*ih/iw > h){
			*res_w = h*iw/ih;
			*res_h = h;						
		}else{
			*res_w = w;
			*res_h = w*ih/iw;						
		}		
	}else{
		if(h*iw/ih > w){
			*res_w = w;
			*res_h = w*h/(h*iw/ih);		
		}else{
			*res_w = h*iw/ih;
			*res_h = h;				
		}
	}
}

static void 
_add_image(Evas_Object *obj, void *data)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it = (Elm_Stackedicon_Item *)data;
	Evas_Object *ly = NULL;
	Evas_Object *ic = NULL;
	Evas_Object *pad = NULL;	
	int iw, ih;
	if (!wd || !it) return;

	ly = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj, ly, "stackedicon", "icon", elm_widget_style_get(obj));
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_widget_sub_object_add(obj, ly); 
	
	ic = evas_object_image_add(evas_object_evas_get(obj));
	evas_object_image_load_size_set(ic, wd->w/2, wd->h/2);
	evas_object_image_file_set(ic, it->path, NULL);
	evas_object_image_size_get(ic, &iw, &ih);

	_calc_item_size(wd->w - 2, wd->h - 2, iw, ih, &it->w, &it->h);

	evas_object_image_fill_set(ic, 0, 0, it->w, it->h);
	evas_object_size_hint_min_set(ly, it->w, it->h);
	evas_object_resize(ly, it->w, it->h);

	evas_object_image_filled_set(ic, 1);
	edje_object_part_swallow(ly, "contents", ic);
	
	pad = evas_object_rectangle_add(evas_object_evas_get(obj));
	evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(pad, 0, 0, 0, it->index*25);
	edje_object_part_swallow(ly, "shadow", pad);
	
	evas_object_event_callback_add(ly, EVAS_CALLBACK_MOVE, _icon_move_cb, it);	

	it->mw = it->w;
	it->mh = it->h;
	it->ly = ly;
	it->ic = ic;		
	it->pad = pad;
	it->exist = EINA_TRUE;
}

static void 
_add_image_to_buffer(Evas_Object *obj, Evas* e, void *data)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it = (Elm_Stackedicon_Item *)data;
	Evas_Object *ly = NULL;
	Evas_Object *ic = NULL;
	int iw, ih, res_w, res_h;
	if (!wd || !it) return;

	// FIXME: add an opaque rectangle because of alpha bug of evas_map.
	Evas_Object* rect = evas_object_rectangle_add( e );
	evas_object_resize( rect, 1, 1);
	evas_object_move(rect, wd->w/2, wd->h/2);
	evas_object_color_set( rect, 0, 0, 0, 255 );
	evas_object_show( rect );	

	ly = edje_object_add(e);
	_elm_theme_object_set(obj, ly, "stackedicon", "icon", elm_widget_style_get(obj));
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	ic = evas_object_image_add(e);
	evas_object_image_alpha_set(ic, EINA_TRUE);
	evas_object_image_load_size_set(ic, wd->w/2, wd->h/2);
	evas_object_image_file_set(ic, it->path, NULL);
	evas_object_image_size_get(ic, &iw, &ih);

	_calc_item_size(wd->w - 2, wd->h - 2, iw, ih, &res_w, &res_h);

	evas_object_image_fill_set(ic, 0, 0, res_w, res_h);		
	evas_object_image_filled_set(ic, 1);
	edje_object_part_swallow(ly, "contents", ic);

	evas_object_resize(ly, res_w, res_h);
	evas_object_move(ly, (wd->w - res_w)/2, (wd->h - res_h)/2);		
	evas_object_show(ly);	

	_icon_map_pos(ly, it->index, (wd->w - res_w)/2, (wd->h - res_h)/2, res_w, res_h);	
}

static Evas_Object * 
_create_fake_image(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *eo = NULL;
	Eina_List *l = NULL;
	Elm_Stackedicon_Item *it = NULL;
	if (!wd) return NULL;

	// create fake_img
	eo = evas_object_image_add(evas_object_evas_get(obj));
	if(!eo) return NULL;
	elm_widget_sub_object_add(obj, eo);
	evas_object_image_alpha_set(eo,EINA_TRUE);
	evas_object_image_data_set(eo, NULL);
	evas_object_image_size_set(eo, wd->w, wd->h);
	evas_object_image_fill_set(eo, 0, 0, wd->w, wd->h);	
	edje_object_part_swallow(wd->base, "elm.bg.swallow", eo);

	// create ecore_evas (buffer)
	Ecore_Evas* ee = ecore_evas_buffer_new( wd->w, wd->h );
	Evas* e = ecore_evas_get( ee );

	// add shown icons
	EINA_LIST_REVERSE_FOREACH(wd->list, l, it) {
		if(it->index >= MAX_SHOWN_ITEM) continue;
		if(it) _add_image_to_buffer(obj, e, it);
	}
	ecore_evas_show( ee );


	// copy buffer to data(mem)
	unsigned char* data = (unsigned char*) calloc( 1, sizeof( unsigned char ) * 4 * wd->w * wd->h );
	memcpy( data, (unsigned char*) ecore_evas_buffer_pixels_get( ee ), sizeof( unsigned char ) * 4 * wd->w * wd->h );
	ecore_evas_free( ee );
	
	// copy data to fake_img 
	evas_object_image_data_copy_set(eo, data);
	evas_object_image_data_update_add(eo, 0, 0, wd->w, wd->h);
	evas_object_resize(eo, wd->w, wd->h);

	evas_object_color_get(eo, &wd->r, &wd->g, &wd->b, &wd->a);

	// add mouse events callback
	evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_DOWN, _fake_img_mouse_down_cb, obj);
	evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_MOVE, _fake_img_mouse_move_cb, obj);
	evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_UP, _fake_img_mouse_up_cb, obj);

	return eo;
}	

static void 
_update_stackedicon(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || ((wd->w == 1) && (wd->h == 1))) return;

	if(!wd->fake_img)
	{
		wd->fake_img = _create_fake_image(obj);
	}
	else if(wd->on_update){
		wd->on_update = FALSE;
		elm_widget_sub_object_del(obj, wd->fake_img);
		edje_object_part_unswallow(wd->base, wd->fake_img);
		evas_object_del(wd->fake_img);
		wd->fake_img = NULL;
		wd->fake_img = _create_fake_image(obj);
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
			if(it->exist == EINA_TRUE){	
				evas_object_show(it->ly);			
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
			if(it->exist == EINA_TRUE){	
				evas_object_hide(it->ly);			
			}
		}
	}
}

static void _hide_hidden_image(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Elm_Stackedicon_Item *it = NULL;
	if (!wd) return;
			
	EINA_LIST_REVERSE_FOREACH(wd->list, l, it) {
		if(it->ly) evas_object_hide(it->ly);
	}
	evas_object_color_set(wd->fake_img, wd->r, wd->g, wd->b, wd->a);
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
	
	_update_stackedicon(data);	
}

static void
_show_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	wd->visible = EINA_TRUE;
	_update_stackedicon(data);	
}

static void
_hide_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	wd->visible = EINA_FALSE;
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
	
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "stackedicon", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);
	
	wd->w = 1;
	wd->h = 1;
	
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
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it;
	if (!wd) return NULL;
	if(eina_list_count(wd->list) >= MAX_ITEM_NUM) return NULL;
	
	it = (Elm_Stackedicon_Item *)calloc(1, sizeof(Elm_Stackedicon_Item));
	it->path = eina_stringshare_add(path);
	it->parent = obj;
	it->ly = NULL;
	it->ic = NULL;
	it->pad = NULL;
	it->index = eina_list_count(wd->list);
	it->exist = EINA_FALSE;	
	wd->list = eina_list_append(wd->list, it);

	if(it->index < MAX_SHOWN_ITEM){
		wd->on_update = TRUE;
		_update_stackedicon(obj);
	}

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
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Stackedicon_Item *it;
	if (!wd) return NULL;
	if(eina_list_count(wd->list) >= MAX_ITEM_NUM) return NULL;
	
	it = (Elm_Stackedicon_Item *)calloc(1, sizeof(Elm_Stackedicon_Item));
	it->path = eina_stringshare_add(path);
	it->parent = obj;
	it->ly = NULL;
	it->ic = NULL;
	it->pad = NULL;
	it->index = eina_list_count(wd->list);
	it->exist = EINA_FALSE;	
	wd->list = eina_list_prepend(wd->list, it);

	if(it->index < MAX_SHOWN_ITEM){
		wd->on_update = TRUE;
		_update_stackedicon(obj);
	}

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
	if(!it)return;
	ELM_CHECK_WIDTYPE(it->parent, widtype);
	Evas_Object *obj = it->parent;
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *l;
	Elm_Stackedicon_Item *_it = NULL;
	int i = 0;
	if (!wd) return;

	if(it->index < MAX_SHOWN_ITEM) wd->on_update = TRUE;

	if(it->exist == EINA_TRUE) _del_image(it);
	wd->list = eina_list_remove(wd->list, it);
	free(it);

	EINA_LIST_FOREACH(wd->list, l, _it)
		if(_it->ly) _it->index = i++;

	_update_stackedicon(obj);
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
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	
	return wd->list;
}

