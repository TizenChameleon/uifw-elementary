#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup SegmentControl SegmentControl
 *
 * The SegmentControl button allows for 1 or more selectors to be created to select 1
 * of a set of options.
 *
 * Signals that you can add callbacks for are:
 *
 */
typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
   Evas_Object *box;
   Evas_Object *base;
   Eina_List *seg_ctrl;
   Eina_List *queue;
   int width, height;
   int id;
   int item_width;

   Elm_Segment_Item *ani_it;
   Ecore_Animator *ani;
   unsigned int count;
   unsigned int insert_index;
   unsigned int del_index;
   unsigned int cur_seg_id;
};

struct _Elm_Segment_Item
{
   Evas_Object *obj;
   Evas_Object *base;
   Evas_Object *icon;
   const char *label;
	Eina_Bool delete_me : 1;
   int segment_id;
};

static void _sizing_eval(Evas_Object *obj);
static void _signal_segment_on(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _sizing_eval(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _item_free(Evas_Object *obj, Elm_Segment_Item *it);
static void _del_hook(Evas_Object *obj);
static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data);
static Elm_Segment_Item * _item_find(Evas_Object *obj, unsigned int index);
static void _update_list(Evas_Object *obj);
static void _refresh_segment_ids(Evas_Object *obj);
static void _state_value_set(Evas_Object *obj);
static void _queue_list(Evas_Object *obj);

static Elm_Segment_Item* _item_new(Evas_Object *obj, const char *label, Evas_Object *icon);
static Elm_Segment_Item *_item_find(Evas_Object *obj, unsigned int index);

static int * _animator_animate_add_cb(Evas_Object *obj);
static int * _animator_animate_del_cb(Evas_Object *obj);


static void
_signal_segment_on(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Elm_Segment_Item *item = (Elm_Segment_Item *) data;
	Widget_Data *wd = elm_widget_data_get(item->obj);

	if (!wd)
		return;

	edje_object_signal_emit(item->base, "elm,state,segment,on", "elm");
	edje_object_signal_emit(item->base, "elm,state,text,change", "elm");
	Elm_Segment_Item *it;
	Eina_List *l;
	if (item->segment_id == wd->cur_seg_id)
	{
		wd->cur_seg_id = item->segment_id;
		return;
	}
	int i = 0;
	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		if (it->segment_id == wd->cur_seg_id) {
			edje_object_signal_emit(it->base, "elm,state,segment,off", "elm");
			edje_object_signal_emit(it->base, "elm,state,text,visible", "elm");
			 break;
		}
	}
	wd->cur_seg_id = item->segment_id;
   evas_object_smart_callback_call(item->obj, "changed", (void*)wd->cur_seg_id);
}


static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

	evas_object_size_hint_weight_set(wd->base, 1.0, -1.0);
	evas_object_size_hint_align_set(wd->base, 1.0, -1.0);
   evas_object_size_hint_min_set(wd->base, -1, -1);
	evas_object_size_hint_max_set(wd->base, maxw, maxh);
   return;
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   _elm_theme_object_set(obj, obj, "segmented-control", "base", "default");

   return;
}

static void
_item_free(Evas_Object *obj, Elm_Segment_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if(wd->seg_ctrl)
   	wd->seg_ctrl = eina_list_remove(wd->seg_ctrl, it);

   if(it->icon) evas_object_del(it->icon);
   if(it->base) evas_object_del(it->base);
   if(it->label) eina_stringshare_del(it->label);

   if(it)
   	free(it);
   it = NULL;
	return;
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Segment_Item *it;
   Eina_List *l, *clear = NULL;

   EINA_LIST_FOREACH(wd->seg_ctrl, l, it) clear = eina_list_append(clear, it);
   EINA_LIST_FREE(clear, it) _item_free(obj, it);

   if(wd)
   	free(wd);
   wd = NULL;

   return;
}


static void
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
   Widget_Data *wd = data;
   if (!wd) return;
   _els_box_layout(o, priv, 1, 0); /* making box layout non homogenous */

   return;
}

EAPI Evas_Object *
elm_segment_control_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;
	Evas_Coord w, h, x, y;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "segmented-control");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "segmented-control", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->box = evas_object_box_add(e);
	evas_object_box_layout_set(wd->box, _layout, wd, NULL);
	elm_widget_sub_object_add(obj, wd->box);
	edje_object_part_swallow(wd->base, "elm.swallow.content", wd->box);
	evas_object_show(wd->box);

	edje_object_part_geometry_get(wd->base, "layout", &x, &y, &w, &h);
	wd->item_width = wd->width = w;
	wd->height = h;
	wd->id = 0;
	wd->del_index = 0;
	wd->insert_index = 0;

	return obj;
}

static Elm_Segment_Item*
_item_new(Evas_Object *obj, const char *label, Evas_Object *icon)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Elm_Segment_Item *it;
   it = calloc(1, sizeof(   Elm_Segment_Item));
   if (!it) return NULL;
	Evas_Coord mw, mh;

   if(obj) it->obj = obj;
   it->delete_me = EINA_FALSE;
   it->segment_id = wd->id;

   it->base = edje_object_add(evas_object_evas_get(obj));
   _elm_theme_object_set(obj, it->base, "segment", "base", elm_object_style_get(it->base));

   if (it->label) eina_stringshare_del(it->label);
   if (label)
	 {
			it->label = eina_stringshare_add(label);
	 }
   else
	 {
			it->label = NULL;
	 }

   if ((it->icon != icon) && (it->icon))
	 elm_widget_sub_object_del(obj, it->icon);
   it->icon = icon;
   if (icon)
	 {
		elm_widget_sub_object_add(obj, icon);
		Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

		elm_coords_finger_size_adjust(1, &minw, 1, &minh);
		elm_coords_finger_size_adjust(1, &minw, 1, &minh);

		evas_object_size_hint_weight_set(it->base, 1.0, -1.0);
		evas_object_size_hint_align_set(it->base, 1.0, -1.0);
		evas_object_size_hint_min_set(it->base, -1, -1);
		evas_object_size_hint_max_set(it->base, maxw, maxh);
	 }

	   return it;
}


static void _update_list(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	Elm_Segment_Item *it;
	Eina_List *l;
	int i = 0;
	wd->count = eina_list_count(wd->seg_ctrl);

	if(wd->count == 1)
	{
		it = _item_find(obj, 0);
		_elm_theme_object_set(obj, it->base, "segment", "base", "single");
		edje_object_signal_emit(it->base, "elm,state,segment,on", "elm");
		edje_object_signal_emit(it->base, "elm,state,text,visible", "elm");
		edje_object_signal_emit(it->base, "elm,state,text,change", "elm");
		edje_object_message_signal_process(it->base);

		edje_object_part_text_set(it->base, "elm.text", it->label);

		edje_object_part_swallow(it->base, "elm.swallow.content", it->icon);
		edje_object_signal_emit(it->base, "elm,state,icon,visible", "elm");
		return;
	}

	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		if(i==0)
		{
			_elm_theme_object_set(obj, it->base, "segment", "base", "first");
		}
		else if(i==wd->count-1)
		{
			_elm_theme_object_set(obj, it->base, "segment", "base", "last");
		}
		else
		{
			_elm_theme_object_set(obj, it->base, "segment", "base", "default");
		}

		edje_object_signal_emit(it->base, "elm,state,text,visible", "elm");
		edje_object_message_signal_process(it->base);

		edje_object_part_text_set(it->base, "elm.text", it->label);

		edje_object_part_swallow(it->base, "elm.swallow.content", it->icon);
		edje_object_signal_emit(it->base, "elm,state,icon,visible", "elm");
		if(wd->cur_seg_id == i)
		{
			edje_object_signal_emit(it->base, "elm,state,segment,on", "elm");
			edje_object_signal_emit(it->base, "elm,state,text,change", "elm");
		}
		i++;
	}
}


static void _refresh_segment_ids(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	Elm_Segment_Item *it;
	Eina_List *l;

	if (wd->insert_index && wd->cur_seg_id >= wd->insert_index)
	{
		++wd->cur_seg_id;
		wd->insert_index = 0;
	}
	if (wd->del_index)
	{
		if (wd->cur_seg_id >= wd->del_index)
			--wd->cur_seg_id;
		wd->del_index =0;
	}

	int i = 0;
	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		it->segment_id = i;
		i++;
	}
}

static void _state_value_set(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	Elm_Segment_Item *it;
	Eina_List *l;
   Evas_Coord w, h, mw, mh, x, y, xx, yy;
   int w1=0, w2;


	unsigned int count = eina_list_count(wd->seg_ctrl);

	if(count > 0)
		wd->item_width = wd->width/count;


	if(wd->ani_it)
	{
		evas_object_geometry_get(wd->ani_it->base, &x, &y, &w1, NULL);
		if(wd->ani_it->delete_me)
		{
			w1-=(wd->item_width/15);
			if( w1< 0) w1 = 0;
		}
		else
		{
			w1+=(wd->item_width/15);
			if( w1 > wd->item_width )
					w1 = wd->item_width;
		}

			w2 = (wd->width-w1)/(count -1);
	}
	else
			w2 = wd->item_width;

	int i=0;

	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		edje_object_size_min_restricted_calc(it->base, &mw, &mh, 0, 0);
		evas_object_size_hint_weight_set(it->base, 1.0, 1.0);
		evas_object_size_hint_align_set(it->base, -1.0, -1.0);

		if(wd->ani_it  && it == wd->ani_it)
		{
			evas_object_resize(it->base, w1, wd->height);
			evas_object_size_hint_min_set(it->base, w1, wd->height);
			evas_object_size_hint_max_set(it->base, w1, wd->height);
		}
		else
		{
			evas_object_resize(it->base, w2, wd->height);
			evas_object_size_hint_min_set(it->base, w2, wd->height);
			evas_object_size_hint_max_set(it->base, w2, wd->height);
		}

		++i;
	}

	return;
}


static int *
_animator_animate_add_cb(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	int w;
	evas_object_geometry_get(wd->ani_it->base, NULL, NULL, &w, NULL);

	if( w <  wd->item_width )
	{
	 _state_value_set(obj);
	 evas_object_geometry_get(wd->ani_it->base, NULL, NULL, &w, NULL);
			return ECORE_CALLBACK_RENEW;
	}
	else
	 {
		 ecore_animator_del(wd->ani);
		 wd->ani = NULL;
		 wd->ani_it = NULL;
			return ECORE_CALLBACK_CANCEL;
	 }
}


static int *
_animator_animate_del_cb(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	int w;
	evas_object_geometry_get(wd->ani_it->base, NULL, NULL, &w, NULL);
	if( w >  0 )
	{
	 _state_value_set(obj);
	 evas_object_geometry_get(wd->ani_it->base, NULL, NULL, &w, NULL);
			return ECORE_CALLBACK_RENEW;
	}
	else
	 {
		_item_free(obj, wd->ani_it );
		_refresh_segment_ids(obj);
		 ecore_animator_del(wd->ani);
		 wd->ani = NULL;
		 wd->ani_it = NULL;
		_update_list(obj);
			return ECORE_CALLBACK_CANCEL;
	 }
}

static void _queue_list(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	Elm_Segment_Item *it;
	Eina_List *l;
	int i = 0;
	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		i++;
	}
	wd->count = eina_list_count(wd->seg_ctrl);
}

EAPI Elm_Segment_Item *
elm_segment_control_add_segment(Evas_Object *obj, Evas_Object *icon, const char *label, Eina_Bool animate)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   Evas_Object *seg;
   Elm_Segment_Item *it;

   it = _item_new(obj, label, icon);
	if(!it)
		return;

	wd->seg_ctrl = eina_list_append(wd->seg_ctrl, it);

	_update_list(obj);

	  edje_object_signal_callback_add(it->base, "elm,action,segment,click", "elm", _signal_segment_on, it);
		++wd->id;
	   _refresh_segment_ids(obj);

	if(animate && it->segment_id)
	   {
			if(wd->ani_it == NULL)
			{
				evas_object_resize(it->base, 1, wd->height);
				wd->ani_it = it;
				wd->ani = ecore_animator_add( _animator_animate_add_cb, obj );
			}
			else
			{
				wd->queue = eina_list_append(wd->queue, it);
			}
	   }
	   else
	      _state_value_set(obj);
	evas_object_show( it->base);

	evas_object_box_append(wd->box, it->base);
	evas_object_smart_calculate(wd->box);

   return it;
}


static Elm_Segment_Item *
_item_find(Evas_Object *obj, unsigned int index)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return NULL;

	Elm_Segment_Item *it;
	Eina_List *l;

	int i = 0;
	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		if (i == index) {
			return it;
		}
		i++;
	}
	return NULL;
}


static Elm_Segment_Item *
_item_search(Evas_Object *obj, Elm_Segment_Item *item)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return NULL;

	Elm_Segment_Item *it;
	Eina_List *l;

	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		if (it == item) {
			return it;
		}
	}
	return NULL;
}

EAPI void
elm_segment_control_insert_segment_at(Evas_Object *obj, Evas_Object *icon, const char *label, unsigned int index, Eina_Bool animate)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   Elm_Segment_Item *it, *it_rel;

   it = _item_new(obj, label, icon);
   it_rel = _item_find(obj, index);
   if (!it_rel)
     {
      wd->seg_ctrl = eina_list_append(wd->seg_ctrl, it);
     }
   else
   {
      if (!it) return;
   		wd->seg_ctrl = eina_list_prepend_relative(wd->seg_ctrl, it, it_rel);
   }
	edje_object_signal_callback_add(it->base, "elm,action,segment,click", "elm", _signal_segment_on, it);
   wd->insert_index = index;
	++wd->id;
   _refresh_segment_ids(obj);

	_update_list(obj);


	if(animate && it->segment_id)
   {
		if(wd->ani_it == NULL)
		{
			wd->ani_it = it;
			evas_object_resize(it->base, 1, wd->height);
			wd->ani = ecore_animator_add( _animator_animate_add_cb, obj );
		}
		else
		{
			wd->queue = eina_list_append(wd->queue, it);
		}
   }
   else
      _state_value_set(obj);

	evas_object_show( it->base);

	 if(index >= wd->id-1)
	   {
	   	evas_object_box_append(wd->box,  it->base);
	   }
	   else
	   {
	   	evas_object_box_insert_at(wd->box,  it->base, index);
	   }

		evas_object_smart_calculate(wd->box);

   return;
}


EAPI void
elm_segment_control_delete_segment(Evas_Object *obj, Elm_Segment_Item *item, Eina_Bool animate)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   if(!item) return;

   Elm_Segment_Item *it;
   it = _item_search(obj, item);

   if(!it)
   	return;
   wd->del_index = it->segment_id;

	if(animate && it->segment_id)
	{
		if(wd->ani_it == NULL)
		{
			it->delete_me = EINA_TRUE;
			wd->ani_it = it;
			wd->ani = ecore_animator_add( _animator_animate_del_cb, obj );
		}
		else
		{
			wd->queue = eina_list_append(wd->queue, it);
		}
	}
	else
	{
		evas_object_box_remove(wd->box, it->base);
		evas_object_smart_calculate(wd->box);
		_item_free(obj, it);
	   _refresh_segment_ids(obj);
		_state_value_set(obj);
		_update_list(obj);
	}
	--wd->id;
   return;
}


EAPI void
elm_segment_control_delete_segment_at(Evas_Object *obj,  unsigned int index, Eina_Bool animate)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;
   Elm_Segment_Item *it, *it_rel;

   it = _item_find(obj, index);

   if(!it)
   	return;

   wd->del_index = index;
	--wd->id;
	if(animate && it->segment_id)
		{
			if(wd->ani_it == NULL)
			{
				wd->ani_it = it;
				it->delete_me = EINA_TRUE;
				wd->ani = ecore_animator_add( _animator_animate_del_cb, obj );
			}
			else
			{
				wd->queue = eina_list_append(wd->queue, it);
			}
		}
		else
		{
			evas_object_box_remove(wd->box, it->base);
			evas_object_smart_calculate(wd->box);
			_item_free(obj, it);
		   _refresh_segment_ids(obj);
			_state_value_set(obj);
		}

	_update_list(obj);
   return;
}



EAPI const char *
elm_segment_control_get_segment_label_at(Evas_Object *obj, unsigned int index)
{
	Elm_Segment_Item *it_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   it_rel = _item_find(obj, index);

   if(it_rel)
      return it_rel->label;

   return NULL;
}


EAPI Evas_Object *
elm_segment_control_get_segment_icon_at(Evas_Object *obj, unsigned int index)
{
	Elm_Segment_Item *seg_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   seg_rel = _item_find(obj, index);

   if(seg_rel)
     	return seg_rel->icon;

   return NULL;
}

EAPI Elm_Segment_Item *
elm_segment_control_selected_segment_get(const Evas_Object *obj, int *value)
{
   Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->seg_ctrl) return NULL;

	Elm_Segment_Item *it;
	Eina_List *l;

	EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
	{
		if(it->segment_id == wd->cur_seg_id)
			{
				* value = wd->cur_seg_id;
				return it;
			}
	}
	return NULL;
}

EAPI int
elm_segment_control_get_segment_count(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return 0;

   return wd->id;
}
