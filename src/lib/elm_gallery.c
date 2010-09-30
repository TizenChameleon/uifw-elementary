/**
  *
  * @defgroup Gallery
  * @ingroup Elementary
  *
  * This is an gallery.
  *
  */


#include <Elementary.h>
#include "elm_priv.h"


#define INIT_ITEM_NUM		(2)
#define GROUP_ITEM_NUM		(3)
#define PADDING_WIDTH 		(25)

#define MULTI_TOUCH_SET	(1)

#define DEFAULT_STATUS       0
#define TRAN_STATUS             1
#define WHITE_DIM_STATUS   2

typedef struct Widget_Data {
	Evas_Object *obj;	
	Evas_Object *sc;
	Evas_Object *tb;
	Evas_Object *rect;
	const char *widget_name;
	int init_num;
	int padding_w;
	Eina_List *list;
	Ecore_Idler *eidler;	
	Evas_Coord x, y, w, h;
	int prex;	
	int item_type;
	int item_count;
} Widget_Data;

static void gallery_add(Evas_Object *obj);
static void gallery_del(Evas_Object *obj);
static void gallery_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void gallery_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void gallery_show(Evas_Object *obj);
static void gallery_hide(Evas_Object *obj);
static void gallery_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void gallery_clip_set(Evas_Object *obj, Evas_Object *clip);
static void gallery_clip_unset(Evas_Object *obj);
static Evas_Smart *gallery_smart_get(void);
static Evas_Object *gallery_new(Evas_Object *parent);
static void _update_gallery(Evas_Object *obj);
static void _resize_gallery(Evas_Object *obj);
static void _gallery_scroller_cb(void *data, Evas_Object *obj, void *event_info);
static void _gallery_animate_stop_cb(void *data, Evas_Object *obj, void *event_info);
static void _gallery_move_right_cb(Evas_Object *obj, int index);
static void _gallery_move_left_cb(Evas_Object *obj, int index);
static void _gallery_clear_all_image(Evas_Object *obj);
static void _gallery_add_image(Evas_Object *obj, int index, void *data);
static void _gallery_del_image(void *data);
static void _gallery_del_image_force(void *data);
static int _gallery_idle_handler_cb(void* data);
static void _gallery_auto_blight(Evas_Object *obj);
static void _gallery_set_all_status(Evas_Object *obj,int status);

	
static void gallery_add(Evas_Object *obj)
{
	Widget_Data *wd;

	wd = (Widget_Data *)calloc(1, sizeof(Widget_Data));
	if (!wd) {
		errno = ENOMEM;
	} else {
		evas_object_smart_data_set(obj, wd);
			
		wd->sc = elm_scroller_add(obj);
		evas_object_size_hint_weight_set(wd->sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_scroller_bounce_set(wd->sc, 1, 0);
		elm_scroller_policy_set(wd->sc, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
		evas_object_smart_callback_add(wd->sc, "scroll", _gallery_scroller_cb, wd);
		evas_object_smart_callback_add(wd->sc, "scroll,anim,stop", _gallery_animate_stop_cb, wd);
		
		wd->tb = elm_table_add(obj);
		evas_object_size_hint_weight_set(wd->tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(wd->tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_scroller_content_set(wd->sc, wd->tb);

		wd->widget_name = eina_stringshare_add("gallery");
		wd->init_num = INIT_ITEM_NUM;
		wd->obj = obj;
		wd->eidler = NULL;
		wd->padding_w = PADDING_WIDTH;
		wd->item_type =  DEFAULT_STATUS;
		wd->item_count = GROUP_ITEM_NUM;
	}
}

static void gallery_del(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);

	if(wd->eidler){
		ecore_idler_del(wd->eidler);
		wd->eidler = NULL;
	}
	
	if(wd->sc){
		evas_object_del(wd->sc);
		wd->sc = NULL;
	}
	
	if(wd->list){
		eina_list_free(wd->list);
		wd->list = NULL;
	}
	
	if(wd) free(wd);
}

static void gallery_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) {
		errno = EINVAL;
		return;
	}

	wd->x = x;
	wd->y = y;
	evas_object_move(wd->sc, x, y);
}

static void gallery_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) {
		errno = EINVAL;
		return;
	}

	wd->w = w;
	wd->h = h;
		
	evas_object_resize(wd->sc, w, h);	

	wd->padding_w = (w - ((w-2)/(wd->item_count +1))*wd->item_count )/4;

	if(!(wd->item_type == WHITE_DIM_STATUS))
	{
		elm_table_padding_set(wd->tb, wd->padding_w, 0);
		elm_scroller_page_size_set(wd->sc, (wd->w -2)/(wd->item_count +1) + wd->padding_w, wd->h);
	}
	else
	{
		elm_table_padding_set(wd->tb, 0, 0);
		elm_scroller_page_size_set(wd->sc, (wd->w)/(wd->item_count ) , wd->h);
	}
	_resize_gallery(obj);
	_update_gallery(obj);	
}

static void gallery_show(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) {
		errno = EINVAL;
		return;
	}

	evas_object_show(wd->tb);	
	evas_object_show(wd->sc);	
 
	_update_gallery(obj);	

	if(wd->eidler){
		ecore_idler_del(wd->eidler);
		wd->eidler = NULL;
	}
	
	wd->eidler = ecore_idler_add(_gallery_idle_handler_cb, wd); 		
}

static void gallery_hide(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) {
		errno = EINVAL;
		return;
	}
	_gallery_clear_all_image(obj);	
	evas_object_hide(wd->sc);
}

static void gallery_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) {
		errno = EINVAL;
		return;
	}
	evas_object_color_set(wd->sc, r, g, b, a);
}


static void gallery_clip_set(Evas_Object *obj, Evas_Object *clip)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;
	evas_object_clip_set(wd->sc, clip);
}

static void gallery_clip_unset(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;
	evas_object_clip_unset(wd->sc);

}

static Evas_Smart *gallery_smart_get(void)
{
	static Evas_Smart *s = NULL;
	static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION("UI_gallery");

	if (!s) {
		sc.add = gallery_add;
		sc.del = gallery_del;
		sc.move	= gallery_move;
		sc.resize = gallery_resize;
		sc.show	= gallery_show;
		sc.hide	= gallery_hide;
		sc.color_set = gallery_color_set;
		sc.clip_set = gallery_clip_set;
		sc.clip_unset =	gallery_clip_unset;
		s = evas_smart_class_new(&sc);
	}

	return s;
}

static Evas_Object *gallery_new(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	e = evas_object_evas_get(parent);
	if (!e) {
		errno = EINVAL;
		return NULL;
	} else {
		obj = evas_object_smart_add(e, gallery_smart_get());
	}
	return obj;
}

static void _gallery_scroller_cb(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *)data;
	Evas_Coord ix, iy, iw, ih;

	elm_scroller_region_get(obj, &ix, &iy, &iw, &ih);

	int index;
	if(!(wd->item_type == WHITE_DIM_STATUS))
	{			
		 index = ix/((wd->w -2)/(wd->item_count +1)  + wd->padding_w);
	}
	else
	{
		 index = ix/((wd->w)/(wd->item_count ) );
	}

	printf("scroller %d\n",index);
	
	if(wd->prex < ix){
		_gallery_move_right_cb(wd->obj, index);
	}else
		_gallery_move_left_cb(wd->obj, index);

	wd->prex = ix;

	if(wd->item_type == WHITE_DIM_STATUS )
	{
		_gallery_auto_blight(wd->obj);
		printf("scroller blight\n");
	}
}

static int _gallery_idle_handler_cb(void* data)
{
	Widget_Data *wd = (Widget_Data *)data;
	Elm_Gallery_Item *it = NULL;
	Evas_Coord ix, iy, iw, ih;
 
	elm_scroller_region_get(wd->sc, &ix, &iy, &iw, &ih);

	int index;
	if(!(wd->item_type == WHITE_DIM_STATUS))
	{
		 index = ix/((wd->w -2)/(wd->item_count +1)  + wd->padding_w);
	}
	else
	{
		 index = ix/((wd->w )/(wd->item_count ));
	}
	
	it = (Elm_Gallery_Item *)eina_list_nth(wd->list, index +2);
	
	if(it != NULL){
		evas_object_smart_callback_call(wd->obj, "select", it);
	}

	if(wd->eidler){
		ecore_idler_del(wd->eidler);
		wd->eidler = NULL;
	}
	
	return 0; 
}

static void _gallery_animate_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = (Widget_Data *)data;
	if (!wd) return;
	
	if(wd->eidler){
		ecore_idler_del(wd->eidler);
		wd->eidler = NULL;
	}

	wd->eidler = ecore_idler_add(_gallery_idle_handler_cb, wd); 	
}

static void _resize_gallery(Evas_Object *obj)
{
	Widget_Data *wd;
	int count = 0;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	if(wd->rect){
		elm_table_unpack(wd->tb, wd->rect);
		evas_object_del(wd->rect);
		wd->rect = NULL;
	}

	count = eina_list_count(wd->list);

	wd->rect = evas_object_rectangle_add(evas_object_evas_get(obj));
	
	if(!(wd->item_type == WHITE_DIM_STATUS))
		evas_object_size_hint_min_set(wd->rect, ((wd->w -2)/(wd->item_count +1)) *count, 1);
	else evas_object_size_hint_min_set(wd->rect, ((wd->w )/(wd->item_count )) *count, 1);
	
	elm_table_pack(wd->tb, wd->rect, 0, 0, count, 1);	

	if(!(wd->item_type == WHITE_DIM_STATUS))
	{
		evas_object_size_hint_min_set(wd->tb, ((wd->w -2)/(wd->item_count +1)  + wd->padding_w) *count - wd->padding_w, wd->h -2);
		elm_scroller_content_min_limit(wd->sc, ((wd->w -2)/(wd->item_count +1)  + wd->padding_w) *count - wd->padding_w, wd->h -2);
	}
	else
	{
		evas_object_size_hint_min_set(wd->tb, ((wd->w )/(wd->item_count )  ) *count, wd->h );
		elm_scroller_content_min_limit(wd->sc, ((wd->w )/(wd->item_count ) ) *count , wd->h);
	}
}

static void gallery_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Gallery_Item *it = data;
	Evas_Event_Mouse_Down *ev = event_info;
			
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
		it->on_hold = EINA_TRUE;
	else 
		it->on_hold = EINA_FALSE;


	Widget_Data *wd = evas_object_smart_data_get(it->parent);
	if(wd->item_type ==  WHITE_DIM_STATUS)
	{
		edje_object_signal_emit(it->ly, "set_show_red_border", "elm");
	}

	Evas_Coord ix, iy, iw, ih;		
	Evas_Coord x, y, w, h;
	evas_object_geometry_get(obj, &x, &y, &w, &h);
	elm_scroller_region_get(wd->sc, &ix, &iy, &iw, &ih);

	int index;
	if(!(wd->item_type == WHITE_DIM_STATUS))
		 index = (ix+x)/((wd->w -2)/(wd->item_count +1)  + wd->padding_w);
	else
	{
		index = (ix+x)/((wd->w )/(wd->item_count));	
		
		printf("select find index %d\n",index);
		if(wd->prex < ix){
			_gallery_move_right_cb(wd->obj, index);
		}else
			_gallery_move_left_cb(wd->obj, index);
		
		printf("all print point : %d %d %d %d %d %d %d %d\n",x,y,w,h,ix,iy,iw,ih);
		if(index>0)
			elm_scroller_region_bring_in(wd->sc,(index-1)*iw/(wd->item_count) ,y,w*(wd->item_count),h);
	}
}

static void gallery_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Gallery_Item *it = data;
	Evas_Event_Mouse_Move *ev = event_info;
		
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
	{
		if (!it->on_hold) it->on_hold = EINA_TRUE;
	}
}

static void gallery_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Elm_Gallery_Item *it = data;
	Evas_Event_Mouse_Up *ev = event_info;
	Widget_Data *wd = evas_object_smart_data_get(it->parent);
	Evas_Coord x, y, w, h;
	Evas_Coord ix, iy, iw, ih;
	
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) it->on_hold = EINA_TRUE;
		else it->on_hold = EINA_FALSE;

	if (it->on_hold)
	{
		it->on_hold= EINA_FALSE;
		return;
	}

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	elm_scroller_region_get(wd->sc, &ix, &iy, &iw, &ih);

	int index;
	if(!(wd->item_type == WHITE_DIM_STATUS))
		 index = (ix+x)/((wd->w -2)/(wd->item_count +1)  + wd->padding_w);
	else  index = (ix+x)/((wd->w )/(wd->item_count));
	
	it = (Elm_Gallery_Item *)eina_list_nth(wd->list, index);
	
	if(it != NULL){
		evas_object_smart_callback_call(wd->obj, "select", it);
	}
	
}

static void _gallery_add_image(Evas_Object *obj, int index, void *data)
{
	Widget_Data *wd = NULL;
	Evas_Object *ly = NULL;
	Evas_Object *ic = NULL;
	int w, h;
	
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	Elm_Gallery_Item *it = (Elm_Gallery_Item *)data;

	ly = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj,ly,  "gallery", "base", "default");

	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ly);
		
	ic = evas_object_image_add(evas_object_evas_get(obj));
	if(wd->item_type == WHITE_DIM_STATUS)
		evas_object_image_load_size_set(ic, wd->w/(wd->item_count) , wd->h/(wd->item_count) );
	else evas_object_image_load_size_set(ic, wd->w/(wd->item_count+1) , wd->h/(wd->item_count+1) );
	
	evas_object_image_file_set(ic, it->path, NULL);
	evas_object_image_size_get(ic, &w, &h);	


	if(w>h){
		evas_object_image_fill_set(ic, 0, 0, (wd->w -2)/(wd->item_count +1), ((wd->w -2)/(wd->item_count +1) )*h/w);
		if(!(wd->item_type == WHITE_DIM_STATUS))
		{
			evas_object_size_hint_min_set(ly, (wd->w -2)/(wd->item_count+1) , ((wd->w -2)/(wd->item_count+1) )*h/w);
			evas_object_size_hint_max_set(ly, (wd->w -2)/(wd->item_count+1) , ((wd->w -2)/(wd->item_count+1) )*h/w);
		}
	}else{
		if((wd->h -2)*w/h > (wd->w -2)/(wd->item_count +1)){
			evas_object_image_fill_set(ic, 0, 0, (wd->w -2)/(wd->item_count +1), ((wd->w -2)/(wd->item_count +1))*(wd->h -2)/((wd->h -2)*w/h));
			if(!(wd->item_type == WHITE_DIM_STATUS))
			{
				evas_object_size_hint_min_set(ly, (wd->w -2)/(wd->item_count+1) , ((wd->w -2)/(wd->item_count+1) )*(wd->h -2 )/((wd->h -2)*w/h));
				evas_object_size_hint_max_set(ly, (wd->w -2)/(wd->item_count+1) , ((wd->w -2)/(wd->item_count+1) )*(wd->h -2 )/((wd->h -2)*w/h));
			}
		}else{
			evas_object_image_fill_set(ic, 0, 0, (wd->h -2)*w/h, wd->h -2);
			if(!(wd->item_type == WHITE_DIM_STATUS))
			{
				evas_object_size_hint_min_set(ly, (wd->h -2)*w/h, wd->h -2);
				evas_object_size_hint_max_set(ly, (wd->h -2)*w/h, wd->h -2);
			}
		}
	}

	if(wd->item_type == WHITE_DIM_STATUS)
	{
		evas_object_size_hint_min_set(ly,  (wd->w )/(wd->item_count ) ,  ((wd->w )/(wd->item_count ) )*h/w);
		evas_object_size_hint_max_set(ly, (wd->w )/(wd->item_count ),  ((wd->w )/(wd->item_count ) )*h/w);
	}
	
	evas_object_image_filled_set(ic, 1);
	edje_object_part_swallow(ly, "contents", ic);
	evas_object_show(ic);				

	evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_DOWN, gallery_down_cb, it);
	evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_MOVE, gallery_move_cb, it);
	evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_UP, gallery_up_cb, it);
	
	it->w = (wd->w -2)/(wd->item_count +1) ;
	it->h = ((wd->w -2)/(wd->item_count +1) )*h/w;

	it->mw = it->w;
	it->mh = it->h;

	it->on_highquality = EINA_TRUE;
	it->index = index;
	
	elm_table_pack(wd->tb, ly, index, 1, 1, 1);	

	if(wd->item_type ==  DEFAULT_STATUS)
		edje_object_signal_emit(ly, "set_bg_default", "elm");
	if(wd->item_type ==  TRAN_STATUS)
	{
		edje_object_signal_emit(ly, "set_bg_tran", "elm");
		edje_object_signal_emit(ly, "set_border_hide", "elm");
	}
	else  // WHITE_DIM_STATUS
	{
		edje_object_signal_emit(ly, "set_bg_default", "elm");
	}

	it->ly = ly;
	it->obj = ic;		
	it->on_show = EINA_TRUE;	
}




static void _gallery_auto_blight(Evas_Object *obj)
{
	Widget_Data *wd;
	Elm_Gallery_Item *it;
	Eina_List *l;
	Evas_Coord x, y, w, h;
	
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	int start_pos=-1;
	int end_pos=-1;
	int ret_pos = -1;
	int count=0;
	EINA_LIST_FOREACH(wd->list, l, it) 
	{		
		Elm_Gallery_Item *inner_it = (Elm_Gallery_Item *)it;
		
		if(inner_it->obj) 
		{
			evas_object_geometry_get(it->obj, &x, &y, &w, &h);
			if(!((x+w < 0) || (x > wd->w)))
			{
				if(start_pos==-1)
					start_pos=count;
				else end_pos= count;
			}
		}		
		count++;
	}

	if(start_pos!=-1)
	{
		if(end_pos!=-1)
		{
			ret_pos =(start_pos+end_pos)/2;
		}
		else 
		{
			ret_pos =start_pos;
		}
	}
	
	count =0;
	EINA_LIST_FOREACH(wd->list, l, it) 
	{
		Elm_Gallery_Item *inner_it = (Elm_Gallery_Item *)it;
		if(inner_it->obj) 
		{
			if(ret_pos==count)
			{
				edje_object_signal_emit(inner_it->ly, "set_bg_white", "elm");
				printf("auto blight select %d start %d end %d \n",ret_pos,start_pos,end_pos);
			}
			else 
			{
				edje_object_signal_emit(inner_it->ly, "set_bg_default", "elm");				
				printf("auto nonblight select %d start %d end %d \n",ret_pos,start_pos,end_pos);
			}
		}
		count++;
	}	
	printf("auto blight\n");		
}

static void _gallery_set_all_status(Evas_Object *obj,int status)
{
	Widget_Data *wd;
	Elm_Gallery_Item *it;
	Eina_List *l;	
	
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;
	
	EINA_LIST_FOREACH(wd->list, l, it) 
	{		
		Elm_Gallery_Item *inner_it = (Elm_Gallery_Item *)it;
		if(status ==  DEFAULT_STATUS)
		{
			edje_object_signal_emit(inner_it->ly, "set_bg_default", "elm");
		}
		if(status ==  TRAN_STATUS)
		{
			edje_object_signal_emit(inner_it->ly, "set_bg_tran", "elm");	
			edje_object_signal_emit(inner_it->ly, "set_border_hide", "elm");	
		}	
	}
	if(status == WHITE_DIM_STATUS)
	{
		_gallery_auto_blight(obj);
		printf("blight\n");
	}	
}


static void _gallery_del_image_force(void *data)
{
	Elm_Gallery_Item *it = (Elm_Gallery_Item *)data;
	Widget_Data *wd = evas_object_smart_data_get(it->parent);
	Evas_Coord x, y, w, h;

	if(it->on_show == EINA_TRUE){
		if(it->obj) {
			evas_object_geometry_get(it->obj, &x, &y, &w, &h);

			elm_table_unpack(wd->tb, it->ly);
			evas_object_del(it->ly);
			evas_object_del(it->obj);
			it->obj = NULL;
			it->on_show = EINA_FALSE;
		}
	}
	
}
static void _gallery_del_image(void *data)
{
	Elm_Gallery_Item *it = (Elm_Gallery_Item *)data;
	Widget_Data *wd = evas_object_smart_data_get(it->parent);
	Evas_Coord x, y, w, h;

	if(it->on_show == EINA_TRUE){
		if(it->obj) {
			evas_object_geometry_get(it->obj, &x, &y, &w, &h);

			if((x+w < 0) || (x > wd->w)){
				elm_table_unpack(wd->tb, it->ly);
				evas_object_del(it->ly);
				evas_object_del(it->obj);
				it->obj = NULL;
				it->on_show = EINA_FALSE;
			}
		}
	}
	
}

static void _gallery_clear_all_image(Evas_Object *obj)
{
	Widget_Data *wd;
	Elm_Gallery_Item *it;
	Eina_List *l;

	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->list, l, it) 
	{
		_gallery_del_image_force(it);
	}
}

static void _update_gallery(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	Evas_Coord x, y, w, h;	
	int i = 0;
	int index = 0;
		
	if (!wd) {
		errno = EINVAL;
		return;
	}
 
	if((wd->w == 1) && (wd->h == 1)) return;

	for(i =wd->init_num -3; i< wd->init_num + 4; i++)
	{
		Elm_Gallery_Item *it = NULL;
		it = (Elm_Gallery_Item *)eina_list_nth(wd->list, i);

		if(it != NULL){
			if(it->on_show == EINA_FALSE){	
				_gallery_add_image(obj, i, it);
			}
		}
	}

	if(wd->init_num > 2)
		index = wd->init_num -2;
	else
		index = 0;
	
	elm_scroller_region_get(wd->sc, &x, &y, &w, &h);
	if(!(wd->item_type == WHITE_DIM_STATUS))
		elm_scroller_region_show(wd->sc, index*((wd->w -2)/(wd->item_count +1)  +wd->padding_w), y, w, h);	
	else elm_scroller_region_show(wd->sc, index*((wd->w )/(wd->item_count )), y, w, h);	
}

static void _gallery_move_right_cb(Evas_Object *obj, int index)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	Elm_Gallery_Item *nit = NULL;

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + wd->item_count );
	if(nit != NULL){		
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index + wd->item_count , nit);
		}
	}

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + 2);
	if(nit != NULL){		
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index + 2, nit);
		}
	}

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + 3);
	if(nit != NULL){		
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index + 3, nit);
		}
	}	

	if(index > wd->item_count *2){
		Elm_Gallery_Item *pit = NULL;
		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - wd->item_count  -2);
		if(pit != NULL){
			_gallery_del_image(pit);
		}

		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - wd->item_count  -3);
		if(pit != NULL){
			_gallery_del_image(pit);	
		}

		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - wd->item_count  -4);
		if(pit != NULL){
			_gallery_del_image(pit);	
		}		
	}

}

static void _gallery_move_left_cb(Evas_Object *obj, int index)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return;

	Elm_Gallery_Item *nit = NULL;

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - wd->item_count );
	if(nit != NULL){
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index - wd->item_count , nit);
		}
	}

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - 2);
	if(nit != NULL){
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index - 2, nit);
		}
	}

	nit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index - 3);
	if(nit != NULL){
		if(nit->on_show == EINA_FALSE){
			_gallery_add_image(obj, index - 3, nit);
		}
	}
	
	if(index + wd->item_count + 4 < eina_list_count(wd->list)){
		Elm_Gallery_Item *pit = NULL;
		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + wd->item_count  +2);
		if(pit != NULL){
			_gallery_del_image(pit);
		}

		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + wd->item_count  +3);
		if(pit != NULL){
			_gallery_del_image(pit);
		}

		pit = (Elm_Gallery_Item *)eina_list_nth(wd->list, index + wd->item_count  +4);
		if(pit != NULL){
			_gallery_del_image(pit);
		}		
	}
}


/**
  * This function  makes a new gallery object
  * @param parent The parent object
  * @return  new gallery object 
  *
  * @ingroup Gallery
  */
EAPI Evas_Object *elm_gallery_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Widget_Data *wd;
	if (!parent) {
		errno = EINVAL;
		return NULL;
	}

	if ((obj = gallery_new(parent))) {
		if (!(wd = evas_object_smart_data_get(obj))) {
			evas_object_del(obj);
			return NULL;
		}
	}
	
	elm_widget_sub_object_add (parent, obj);
	
	return obj;
}


/**
  * The function appends an item and return the item class.
  * @param obj The gallery object
  * @param photo_file image file name
  * @return new Elm_Gallery_Item
  *
  * @ingroup Gallery
  */
EAPI Elm_Gallery_Item *elm_gallery_item_append(Evas_Object *obj, const char *photo_file)
{
	Widget_Data *wd;
	Elm_Gallery_Item *it;
	if (!obj || !(wd = evas_object_smart_data_get(obj))) {
		errno = EINVAL;
		return NULL;
	}
	
	it = (Elm_Gallery_Item *)calloc(1, sizeof(Elm_Gallery_Item));
	it->path = eina_stringshare_add(photo_file);
	it->parent = obj;
	it->obj = NULL;		
	it->on_show = EINA_FALSE;
			
	wd->list = eina_list_append(wd->list, it);

	_resize_gallery(obj);

	return it;
}


/**
  * The function prepends an item and return the item class.
  * @param obj The gallery object
  * @param photo_file image file name
  * @return new Elm_Gallery_Item
  *
  * @ingroup Gallery
  */
EAPI Elm_Gallery_Item *elm_gallery_item_prepend(Evas_Object *obj, const char *photo_file)
{
	Widget_Data *wd;
	Elm_Gallery_Item *it;
	if (!obj || !(wd = evas_object_smart_data_get(obj))) {
		errno = EINVAL;
		return NULL;
	}

	it = (Elm_Gallery_Item *)calloc(1, sizeof(Elm_Gallery_Item));
	it->path = eina_stringshare_add(photo_file);
	it->parent = obj;
	it->obj = NULL;		
	it->on_show = EINA_FALSE;
	
	wd->list = eina_list_prepend(wd->list, it);
	
	_resize_gallery(obj);
	
	return it;
}


/**
  * The function deletes the item
  * @param it Elm_Gallery_Item
  *
  * @ingroup Gallery
  */
EAPI void elm_gallery_item_del(Elm_Gallery_Item *it)
{
	Widget_Data *wd;

	if (!it || !(wd = evas_object_smart_data_get(it->parent))) {
		errno = EINVAL;
		return;
	}

	wd->list = eina_list_remove(wd->list, it);

	_resize_gallery(wd->obj);
}


/**
  * The function gets the file name of certain item using it's item class
  * @param it Elm_Gallery_Item
  * @return (char*) file name path
  *
  * @ingroup Gallery
  */
EAPI const char *elm_gallery_item_file_get(Elm_Gallery_Item *it)
{
	if (!it) {
		errno = EINVAL;
		return NULL;
	}
	
	return it->path;
}


EAPI void elm_gallery_item_show(Elm_Gallery_Item *it)
{
	Widget_Data *wd;
	Elm_Gallery_Item *_it;
	Eina_List *l;
	int index = 0;
	
	if (!it || !it->parent|| (!(wd = evas_object_smart_data_get(it->parent)))) {
		errno = EINVAL;
		return;
	}	
   
	EINA_LIST_FOREACH(wd->list, l, _it) 
	{
		if (_it == it) {
			break;
		}
		index++;
	}
	
	wd->init_num = index;

	_update_gallery(it->parent);

	if(wd->eidler){
		ecore_idler_del(wd->eidler);
		wd->eidler = NULL;
	}
	
	wd->eidler = ecore_idler_add(_gallery_idle_handler_cb, wd); 	
}


/**
  * The function gets the list of item class
  * @param obj The gallery object
  * @return new Eina_List of All item
  *
  * @ingroup Gallery
  */
EAPI Eina_List *elm_gallery_item_list_get(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return NULL;
	
	return wd->list;
}



/**
  * The function set the type value 
  * @param parent The parent object
  * @param type Type 1 : image swticher type Type 2 : gallery type
  *
  * @ingroup Gallery
  */
EAPI void elm_gallery_set_type(Evas_Object *obj,int type)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return NULL;

	wd->item_type = type;
	
	_gallery_set_all_status(obj,type);
}



/**
  *The function determines a max number of images to be shown in a row
  * @param parent The parent object
  * @param max_value the value of max
  *
  * @ingroup Gallery
  */  
EAPI void elm_gallery_set_max_count(Evas_Object *obj,int max_value)
{
	Widget_Data *wd;
	wd = evas_object_smart_data_get(obj);
	if (!wd) return NULL;

	wd->item_count = max_value;
}


