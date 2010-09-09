#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup SegmentControl SegmentControl
 * @ingroup Elementary
 *
 * SegmentControl object is a horizontal control made of multiple segments,
 * each segment item functioning as a discrete button. A segmented control affords a compact means to group together a number of controls.
 * A segmented control can display a title or an image. The UISegmentedControl object automatically resizes segment items to fit proportionally
 * within their superview unless they have a specific width set. When you add and remove segments,
 * you can request that the action be animated with sliding and fading effects.
 */
typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
   Evas_Object *box;
   Evas_Object *base;
   Eina_List *seg_ctrl;
   int width, height;
   int id;
   int item_width;

   Elm_Segment_Item *ani_it;
   Ecore_Animator *ani;
   unsigned int count;
   unsigned int insert_index;
   unsigned int del_index;
   unsigned int cur_seg_id;
   double scale_factor;
//   Eina_Bool longpressed : 1;
   Eina_Bool selected : 1;
};

struct _Elm_Segment_Item
{
   Evas_Object *obj;
   Evas_Object *base;
   Evas_Object *icon;
   const char *label;
   Eina_Bool delete_me : 1;
   int segment_id;
//   Ecore_Timer *long_timer;
};

static void _mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _signal_segment_on(void *data);
static void _signal_segment_off(void *data);
static void _theme_hook(Evas_Object *obj);
static void _item_free(Evas_Object *obj, Elm_Segment_Item *it);
static void _del_hook(Evas_Object *obj);
static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data);
static void _segment_resizing(void *data);
static void _object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _update_list(Evas_Object *obj);
static void _refresh_segment_ids(Evas_Object *obj);
static void _state_value_set(Evas_Object *obj);

static Elm_Segment_Item* _item_new(Evas_Object *obj, const char *label, Evas_Object *icon);
static Elm_Segment_Item *_item_find(Evas_Object *obj, unsigned int index);

static int * _animator_animate_add_cb(Evas_Object *obj);
static int * _animator_animate_del_cb(Evas_Object *obj);

static void
_on_focus_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (elm_widget_focus_get(obj))
     evas_object_focus_set((Evas_Object *)wd->seg_ctrl, 1);
   else
     evas_object_focus_set((Evas_Object *)wd->seg_ctrl, 0);
}

static void
_signal_segment_off(void *data)
{
    Elm_Segment_Item *item = (Elm_Segment_Item *) data;
    Widget_Data *wd = elm_widget_data_get(item->obj);
    if (!wd) return;
    
    edje_object_signal_emit(item->base, "elm,action,unfocus", "elm");
    edje_object_signal_emit(item->base, "elm,state,segment,off", "elm");
    edje_object_signal_emit(item->base, "elm,state,text,visible", "elm");

    return;
}
   
static void
_signal_segment_on(void *data)
{
   Elm_Segment_Item *item = (Elm_Segment_Item *) data;
   Elm_Segment_Item *it;
   Eina_List *l;
   
   Widget_Data *wd = elm_widget_data_get(item->obj);
   if (!wd) return;

   wd->selected = EINA_TRUE;

 /*  if (item->long_timer)
     {
       ecore_timer_del(item->long_timer);
       item->long_timer = NULL;
     }*/
   //wd->longpressed = EINA_TRUE;
   edje_object_signal_emit(item->base, "elm,state,segment,on", "elm");
   edje_object_signal_emit(item->base, "elm,state,text,change", "elm");

   if (item->segment_id == wd->cur_seg_id)
	return;

   EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
     {
        if (it->segment_id == wd->cur_seg_id)
	  {
           _signal_segment_off (it);
	     break;
	  }
     }
   wd->cur_seg_id = item->segment_id;
   evas_object_smart_callback_call(item->obj, "changed", (void*)wd->cur_seg_id);

   return;
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Elm_Segment_Item *item = (Elm_Segment_Item *) data;
   Widget_Data *wd = elm_widget_data_get(item->obj);
   if (!wd) return;

   if (item->segment_id == wd->cur_seg_id)
     {
       wd->selected = EINA_TRUE;
       return;
     }
    _signal_segment_on(item);
     wd->selected = EINA_FALSE;
     return;

  /*if(wd->longpressed == EINA_FALSE)
     {
       edje_object_signal_emit(item->base, "elm,action,unfocus", "elm");
       edje_object_signal_emit(item->base, "elm,state,text,visible", "elm");
       edje_object_signal_emit(item->base, "elm,state,text,change", "elm");
      }*/
  /* if (item->long_timer)
     {
       ecore_timer_del(item->long_timer);
       item->long_timer = NULL;
     }*/
}

static void
_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Elm_Segment_Item *item = (Elm_Segment_Item *) data;
   Widget_Data *wd = elm_widget_data_get(item->obj);

   if (!wd) return;

   edje_object_signal_emit(item->base, "elm,action,focus", "elm");

/* if (item->long_timer) ecore_timer_del(item->long_timer);
   item->long_timer = ecore_timer_add(0.3, _signal_segment_on, item);*/
}

static void
_theme_hook(Evas_Object *obj)
{
   _elm_theme_object_set(obj, obj, "segmented-control", "base", elm_widget_style_get(obj));

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
//   if (it->long_timer) ecore_timer_del(it->long_timer);

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

   if(wd) free(wd);
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

static void
_segment_resizing(void *data)
{
   Widget_Data *wd = elm_widget_data_get((Evas_Object *)data);
   if (!wd) return;
   Evas_Coord w = 0, h = 0;

   evas_object_geometry_get(wd->base, NULL, NULL, &w, &h);
   wd->item_width = wd->width = w;
   wd->height = h;
   _state_value_set((Evas_Object *)data);
}

static void 
_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd;
   if(!data) return;
   wd = elm_widget_data_get((Evas_Object *)data);
   if(!wd) return;

   ecore_job_add(_segment_resizing, (Evas_Object *)data);
}

static Elm_Segment_Item*
_item_new(Evas_Object *obj, const char *label, Evas_Object *icon)
{
   Elm_Segment_Item *it; 
   Evas_Coord mw, mh; 
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = calloc(1, sizeof(   Elm_Segment_Item));
   if (!it) return NULL;

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

   edje_object_size_min_restricted_calc(obj, &mw, &mh, 0, 0);
   evas_object_size_hint_weight_set(obj, 1.0, 1.0);
   evas_object_size_hint_align_set(obj, -1.0, -1.0);

   return it;
}


static void 
_update_list(Evas_Object *obj)
{
   Elm_Segment_Item *it;
   Eina_List *l;
   int i = 0;
 
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

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

	i++;
     }
}


static void 
_refresh_segment_ids(Evas_Object *obj)
{
   Elm_Segment_Item *it;
   Eina_List *l;
   int i = 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
 
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

   EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
     {
        it->segment_id = i;
	i++;
     }
}

static void 
_state_value_set(Evas_Object *obj)
{
   Elm_Segment_Item *it;
   Eina_List *l;
   Evas_Coord mw, mh, x, y;
   int w1=0, w2, i=0;
   unsigned int count ;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   
   count = eina_list_count(wd->seg_ctrl);
   if (count > 0)
     wd->item_width = wd->width/count;
   if (wd->ani_it)
     {
        evas_object_geometry_get(wd->ani_it->base, &x, &y, &w1, NULL);
	if (wd->ani_it->delete_me)
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
   int w;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;

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
   int w;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;

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
	wd->id = eina_list_count(wd->seg_ctrl);
	return ECORE_CALLBACK_CANCEL;
     }
}

static Elm_Segment_Item *
_item_find(Evas_Object *obj, unsigned int index)
{
   Elm_Segment_Item *it;
   Eina_List *l;
   int i = 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

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
   Elm_Segment_Item *it;
   Eina_List *l;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)
   return NULL;

   EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
     {
        if (it == item) {
    	   return it;
	}
     }
   return NULL;
}


/**
 * Add a new segmentcontrol to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Evas_Object *
elm_segment_control_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "segmented-control");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
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

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _object_resize, obj);
   wd->id = 0;
   wd->del_index = 0;
   wd->insert_index = 0;
   wd->cur_seg_id = -1;
   wd->selected = EINA_FALSE;

   return obj;
}

/**
 * Add a new segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param icon The icon object for added segment item
 * @param label The label for added segment item 
 * @param animate If 1 the action be animated with sliding effects default 0.
 * @return The new segment item or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_item_add(Evas_Object *obj, Evas_Object *icon, const char *label, Eina_Bool animate)
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   it = _item_new(obj, label, icon);
   if(!it) return NULL;

   wd->seg_ctrl = eina_list_append(wd->seg_ctrl, it);
   wd->id = eina_list_count(wd->seg_ctrl);
   _update_list(obj);
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, it);
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up, it);
   wd->insert_index = 0;
   wd->del_index = 0;
   _refresh_segment_ids(obj);

   if(animate && it->segment_id && wd->ani_it == NULL)
     {
        evas_object_resize(it->base, 1, wd->height);
	wd->ani_it = it;
	wd->ani = ecore_animator_add( _animator_animate_add_cb, obj );
     }
   else
      _state_value_set(obj);
   evas_object_show( it->base);

   evas_object_box_append(wd->box, it->base);
   evas_object_smart_calculate(wd->box);

   return it;
}

/**
 * Add a new segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param icon The icon object for added segment item
 * @param label The label for added segment item 
 * @param animate If 1 the action be animated with sliding effects default 0.
 * @return The new segment item or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_add_segment(Evas_Object *obj, Evas_Object *icon, const char *label, Eina_Bool animate)
{
   EAPI Elm_Segment_Item * it;
   it = elm_segment_control_item_add(obj, icon, label, animate);

    return it;
}

/**
 * Insert a new segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param icon The icon object for added segment item
 * @param label The label for added segment item
 * @param index The position at which segment item to be inserted
 * @param animate If 1 the action be animated with sliding effects default 0.
 * @return The new segment item or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_item_insert_at(Evas_Object *obj, Evas_Object *icon, const char *label, unsigned int index, Eina_Bool animate)
{
   Elm_Segment_Item *it, *it_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

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
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, it);
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up, it);
   wd->insert_index = index;
   wd->id = eina_list_count(wd->seg_ctrl);
   _refresh_segment_ids(obj);

   _update_list(obj);
   if(animate && it->segment_id && wd->ani_it == NULL)
     {
        wd->ani_it = it;
	evas_object_resize(it->base, 1, wd->height);
	wd->ani = ecore_animator_add( _animator_animate_add_cb, obj );
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

   return it ;
}

/**
 * Insert a new segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param icon The icon object for added segment item
 * @param label The label for added segment item
 * @param index The position at which segment item to be inserted
 * @param animate If 1 the action be animated with sliding effects default 0.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI void
elm_segment_control_insert_segment_at(Evas_Object *obj, Evas_Object *icon, const char *label, unsigned int index, Eina_Bool animate)
{
   Elm_Segment_Item *it;
   it = elm_segment_control_item_insert_at(obj, icon, label, index, animate);

   return;
}

/**
 * Delete a segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param item The Segment item to be deleted
 * @param animate If 1 the action be animated with sliding effects default 0.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_del(Evas_Object *obj, Elm_Segment_Item *item, Eina_Bool animate)
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   if(!item) return;

   it = _item_search(obj, item);
   if(!it) return;

   wd->del_index = it->segment_id;
   if(animate && it->segment_id && wd->ani_it == NULL)
     {
        it->delete_me = EINA_TRUE;
	wd->ani_it = it;
	wd->ani = ecore_animator_add( _animator_animate_del_cb, obj );
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
   wd->id = eina_list_count(wd->seg_ctrl);
   return;
}

/**
 * Delete a segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param item The Segment item to be deleted
 * @param animate If 1 the action be animated with sliding effects default 0.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI void
elm_segment_control_delete_segment(Evas_Object *obj, Elm_Segment_Item *item, Eina_Bool animate)
{
   elm_segment_control_item_del(obj, item, animate);

   return;
}

/**
 * Delete a segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param index The position at which segment item to be deleted
 * @param animate If 1 the action be animated with sliding effects default 0.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI void
elm_segment_control_item_del_at(Evas_Object *obj,  unsigned int index, Eina_Bool animate)
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return;

   it = _item_find(obj, index);

   if(!it) return;

   wd->del_index = index;
   if(animate && it->segment_id)
     {
        if(wd->ani_it == NULL)
	{
   	   wd->ani_it = it;
	   it->delete_me = EINA_TRUE;
	   wd->ani = ecore_animator_add( _animator_animate_del_cb, obj );
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
   wd->id = eina_list_count(wd->seg_ctrl);
   return;
}

/**
 * Delete a segment item to segmentcontrol
 * @param obj The SegmentControl object
 * @param index The position at which segment item to be deleted
 * @param animate If 1 the action be animated with sliding effects default 0.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI void
elm_segment_control_delete_segment_at(Evas_Object *obj,  unsigned int index, Eina_Bool animate)
{
   elm_segment_control_item_del_at( obj, index, animate);

   return;
}

/**
 * Get the label of a segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @param index The index of the segment item
 * @return The label of the segment item
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI const char *
elm_segment_control_item_label_get(Evas_Object *obj, unsigned int index)
{
   Elm_Segment_Item *it_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   it_rel = _item_find(obj, index);

   if(it_rel) return it_rel->label;

   return NULL;
}

/**
 * Get the label of a segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @param index The index of the segment item
 * @return The label of the segment item
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI const char *
elm_segment_control_get_segment_label_at(Evas_Object *obj, unsigned int index)
{
   char *label;
   label = elm_segment_control_item_label_get( obj, index);
  
   return label;
}

/**
 * Get the icon of a segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @param index The index of the segment item
 * @return The icon object or NULL if it is not found.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Evas_Object *
elm_segment_control_item_icon_get(Evas_Object *obj, unsigned int index)
{
   Elm_Segment_Item *seg_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return NULL;

   seg_rel = _item_find(obj, index);

   if(seg_rel) return seg_rel->icon;

   return NULL;
}

/**
 * Get the icon of a segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @param index The index of the segment item
 * @return The icon object or NULL if it is not found.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Evas_Object *
elm_segment_control_get_segment_icon_at(Evas_Object *obj, unsigned int index)
{
   Evas_Object *icon;
   icon = elm_segment_control_item_icon_get( obj, index);

   return icon;
}

/**
 * Get the currently selected segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @return The selected Segment item
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_item_selected_get(const Evas_Object *obj)
{
   Elm_Segment_Item *it;
   Eina_List *l;
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd || !wd->seg_ctrl) return NULL;

   EINA_LIST_FOREACH(wd->seg_ctrl, l, it)
     {
       if(it->segment_id == wd->cur_seg_id)
       return it;
      }
    return NULL;
 }

/**
 * Get the currently selected segment item of segmentcontrol
 * @param obj The SegmentControl object
 * @param value The current segment item id
 * @return The selected Segment item
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_selected_segment_get(const Evas_Object *obj, int *value)
{
   Elm_Segment_Item *it;
   it = elm_segment_control_item_selected_get( obj);
   *value = it->segment_id;
   
    return it;
 }

/**
 * Get the count of segments of segmentcontrol
 * @param obj The SegmentControl object
 * @return The count of Segment items
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI int
elm_segment_control_item_count_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if(!wd) return 0;

   return wd->id;
}

/**
 * Get the count of segments of segmentcontrol
 * @param obj The SegmentControl object
 * @return The count of Segment items
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI int
elm_segment_control_get_segment_count(Evas_Object *obj)
{
   int id;
   id = elm_segment_control_item_count_get( obj);

   return id;
}

/**
 * Get the base object of segment item in segmentcontrol
 * @param obj The Segment item
 * @return obj The base object of the segment item
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Evas_Object *
elm_segment_control_item_object_get(Elm_Segment_Item *it)
{
   if (!it) return NULL;
   
   return it->base;
}

/**
 * Select/unselect a particular segment item of segmentcontrol
 * @param item The Segment item that is to be selected or unselected.
 * @param select If 1 the segment item is selected and if 0 it will be unselected.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI void
elm_segment_control_item_selected_set( Elm_Segment_Item *item, Eina_Bool select)
{
   Widget_Data *wd = elm_widget_data_get(item->obj);
   if(!wd) return NULL;

   if(select)
     {
      if(item->segment_id == wd->cur_seg_id)
          return;
		   
      edje_object_signal_emit(item->base, "elm,action,focus", "elm");
      edje_object_signal_emit(item->base, "elm,state,text,visible", "elm");

      _signal_segment_on(item);

      }
   else if(item->segment_id == wd->cur_seg_id)
      {
       wd->selected = EINA_FALSE;
       wd->cur_seg_id = -1;
       _signal_segment_off(item); 
      } 

   return;

}

/**
 * Get a particular indexed segment item of segmentcontrol
 * @param obj The Segment control object.
 * @param index The index of the segment item.
 * @return The corresponding Segment item.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI Elm_Segment_Item *
elm_segment_control_item_get_at(Evas_Object *obj, unsigned int index)
{
   Elm_Segment_Item *it;
   it = _item_find(obj, index);

   return it;
}

/**
 * Select the index of a Segment item of Segmentcontrol
 * @param item The Segment item.
 * @return The corresponding index of the Segment item.
 *
 * @ingroup SegmentControl SegmentControl
 */

EAPI int
elm_segment_control_item_index_get(Elm_Segment_Item *item)
{
   Widget_Data *wd = elm_widget_data_get(item->obj);
   if(!wd) return NULL;

   return item->segment_id;
}

