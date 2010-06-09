/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#define ZC_DEFAULT_RANGE 5
/**
 * @addtogroup Zoom Controls
 *
 * This is a zoom controls. Press it and run some function.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *base;
   int count;
   int range;
   int downdefaultstate;
   int updefaultstate;
   int downdisabledstate;
   int updisabledstate;
   int downdisabledfocusedstate;
   int updisabledfocusedstate;
};


static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
//static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
//static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _signal_down_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_up_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
//static void _signal_unpressed(void *data, Evas_Object *obj, const char *emission, const char *source);
//static void _on_focus_hook(void *data, Evas_Object *obj);
//static void _set_label(Evas_Object *obj, const char *label);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	free(wd);
}
/*
static void
_on_focus_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *top = elm_widget_top_get(obj);
   if (elm_widget_focus_get(obj))
   {
      if(wd->focusedstate)
      {
      	_set_label(obj, wd->focusedlabel);
      }
     edje_object_signal_emit(obj, "elm,action,focus", "elm");
   }
   else
   {
   	if(wd->defaultlabel)
   		_set_label(obj, wd->defaultlabel);
   	else
   		 _set_label(obj, wd->label);
     edje_object_signal_emit(obj, "elm,action,unfocus", "elm");
   }
}
*/
static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_set(wd->base, "zoomcontrols", "base", elm_widget_style_get(obj));
	edje_object_message_signal_process(wd->base);
	edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);
	_sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	//if (elm_widget_disabled_get(obj))
	//{
		if(wd->downdisabledstate)
		{
			edje_object_signal_emit(obj, "elm,state,down,disabled", "elm");
		}
		else if(wd->downdefaultstate)
		{
			edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
		}
		
		if(wd->updisabledstate)
		{
			edje_object_signal_emit(obj, "elm,state,up,disabled", "elm");
		}
		else if(wd->updefaultstate)
		{
			edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
		}
	//}
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord w, h;
   
   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);   
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   evas_object_size_hint_min_get(obj, &w, &h);
   if (w > minw) minw = w;
   if (h > minw) minh = h;

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}
/*
static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (obj != wd->icon) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   if (sub == wd->icon)
     {
   	edje_object_signal_emit(wd->btn, "elm,state,icon,hidden", "elm");
   	evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
	wd->icon = NULL;
	edje_object_message_signal_process(wd->btn);
	_sizing_eval(obj);
     }
}
*/
static void
_signal_down_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	//strcpy(wd->label, "down");
	if((wd->downdefaultstate == 0) && (wd->downdisabledstate == 1))
	{
		return;
	}
	else
	{
		edje_object_signal_emit(wd->base, "elm,state,down,selected", "elm");
	}

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count > -(wd->range))
	{
		if(wd->count == wd->range)
		{
			wd->updisabledstate = 0;
			wd->updefaultstate = 1;
			edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
		 
		}
		wd->count--;	
	        printf("smart callback call (down)\n");
		evas_object_smart_callback_call(data, "down_clicked", NULL);
	}

	printf("count %d\n", wd->count);

	if(wd->count == -(wd->range))
	{ 
		printf("down disabled \n");
		wd->downdisabledstate = 1;
		wd->downdefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,down,disabled", "elm");
	}
	//edje_object_message_signal_process(wd->base);
}
static void
_signal_up_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	//strcpy(wd->label, "up");
	if((wd->updefaultstate == 0) && (wd->updisabledstate == 1))
	{
		return;
	}
	else
	{
		edje_object_signal_emit(wd->base, "elm,state,up,selected", "elm");
	}

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count < wd->range)
	{
		if(wd->count == -(wd->range))
		{
			wd->downdisabledstate = 0;
			wd->downdefaultstate = 1;
			edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
		}
		wd->count++;
		printf("smart callback call (up)\n");
		evas_object_smart_callback_call(data, "up_clicked", NULL);

	}

	printf("count %d\n", wd->count);
		
	//edje_object_message_signal_process(wd->base);
}
static void
_signal_down_unclicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count == -(wd->range))
	{ 
		printf("down enabled \n");
		wd->downdisabledstate = 1;
		wd->downdefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,down,disabled", "elm");
	}
	else if(wd->count > -(wd->range))
	{
		edje_object_signal_emit(wd->base, "elm,state,down,enabled", "elm");
	}     
}
static void
_signal_up_unclicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);

	if(wd->range <= 0)
		wd->range = ZC_DEFAULT_RANGE;
	
	if(wd->count == wd->range)
	{
		printf("up enabled\n");
		wd->updisabledstate = 1;
		wd->updefaultstate = 0;
		edje_object_signal_emit(wd->base, "elm,state,up,disabled", "elm");
	}
	else if(wd->count < wd->range)
	{
		edje_object_signal_emit(wd->base, "elm,state,up,enabled", "elm");
	}     
}
/*
static int
_autorepeat_send(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);

   evas_object_smart_callback_call(data, "repeated", NULL);

   return ECORE_CALLBACK_RENEW;
}

static int
_autorepeat_initial_send(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_CANCEL;

   _autorepeat_send(data);
   wd->timer = ecore_timer_add(wd->ar_interval, _autorepeat_send, data);
   wd->repeating = 1;

   return ECORE_CALLBACK_CANCEL;
}

static void
_signal_pressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;
#if 0   
	if(wd->highlightedstate)
	{
		_set_label(data, wd->clickedlabel);
	}
	if (wd->autorepeat)
	{
		if (wd->ar_threshold <= 0.0)
			_autorepeat_initial_send(data); // call immediately
		else
			wd->timer = ecore_timer_add(wd->ar_threshold, _autorepeat_initial_send, data);
	}
#endif
}

static void
_signal_default_text_set(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	   if (!wd) return;
		if(wd->defaultlabel)
			_set_label(data, wd->defaultlabel);
		else
			 _set_label(data, wd->label);
		return;
}
*/
/*
static void
_signal_unpressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;
#if 0
	if(wd->defaultlabel)
		_set_label(data, wd->defaultlabel);
	else
		_set_label(data, wd->label);
	evas_object_smart_callback_call(data, "unpressed", NULL);

	if (wd->timer)
	{
		ecore_timer_del(wd->timer);
		wd->timer = NULL;
	}
	wd->repeating = 0;
#endif
}
*/
/**
 * Add a new button to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Button
 */
EAPI Evas_Object *
elm_zoomcontrols_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "zoomcontrols");
   elm_widget_sub_object_add(parent, obj);
   
   
   //elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   //elm_widget_can_focus_set(obj, 1 );

   
   wd->base = edje_object_add(e);
   _elm_theme_set(wd->base, "zoomcontrols", "base", "default");

   //memset(wd->label, 0x00, sizeof(wd->label));
   
   wd->count = 0;
   wd->range = 0;
   wd->downdefaultstate = 1;
   wd->updefaultstate = 1;
   wd->downdisabledstate = 0;
   wd->updisabledstate = 0;
   wd->downdisabledfocusedstate = 0;
   wd->updisabledfocusedstate = 0;

   edje_object_signal_callback_add(wd->base, "elm,action,down,click", "", _signal_down_clicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,up,click", "", _signal_up_clicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,down,unclick", "", _signal_down_unclicked, obj);
   edje_object_signal_callback_add(wd->base, "elm,action,up,unclick", "", _signal_up_unclicked, obj);
   
   //edje_object_signal_callback_add(wd->base, "elm,action,press", "",
   //                                _signal_pressed, obj);
   //edje_object_signal_callback_add(wd->base, "elm,action,unpress", "",
   //                                _signal_unpressed, obj);
   //edje_object_signal_callback_add(wd->btn, "elm,action,default,text,set", "",
   //		_signal_default_text_set, obj);
   elm_widget_resize_object_set(obj, wd->base);

   //evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _sizing_eval(obj);
   return obj;
}

EAPI void
elm_zoomcontrols_range_set(Evas_Object *obj, int range)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd)
		return;
	
	if(range <= 0)
		return;

	wd->range = range;
}

/**
 * Set the label used in the button
 *
 * @param obj The button object
 * @param label The text will be written on the button
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_label_set(Evas_Object *obj, const char *label)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord mw, mh;

   if (!wd) return;
   if (wd->label) eina_stringshare_del(wd->label);
   if (label)
     {
			wd->label = eina_stringshare_add(label);
			edje_object_signal_emit(wd->btn, "elm,state,text,visible", "elm");
     }
   else
     {
			wd->label = NULL;
			edje_object_signal_emit(wd->btn, "elm,state,text,hidden", "elm");
     }
   edje_object_message_signal_process(wd->btn);
   edje_object_part_text_set(wd->btn, "elm.text", label);
   _sizing_eval(obj);
}

static void
_set_label(Evas_Object *obj, const char *label)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   edje_object_message_signal_process(wd->btn);
   edje_object_part_text_set(wd->btn, "elm.text", label);
   _sizing_eval(obj);
}
*/

/**
 * Set the label for each state of button
 *
 * @param obj The button object
 * @param label The text will be written on the button
 * @param state The state of button
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_label_set_for_state(Evas_Object *obj, const char *label, UIControlState state)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord mw, mh;

   if (!wd) return;
   if(label == NULL) return;

   if(state == UIControlStateDefault)
	{
   	wd->defaultstate = UIControlStateDefault;
      if (wd->defaultlabel) eina_stringshare_del(wd->defaultlabel);
		wd->defaultlabel = eina_stringshare_add(label);
   }
   if(state == UIControlStateHighlighted)
   {
   	wd->highlightedstate = UIControlStateHighlighted;
      if (wd->clickedlabel) eina_stringshare_del(wd->clickedlabel);
		wd->clickedlabel = eina_stringshare_add(label);
   	return;
   }
   if(state == UIControlStateFocused)
   {
   	wd->focusedstate = UIControlStateFocused;
      if (wd->focusedlabel) eina_stringshare_del(wd->focusedlabel);
		wd->focusedlabel = eina_stringshare_add(label);
   	return;
   }
   if(state == UIControlStateDisabled)
	{
   	wd->disabledstate = UIControlStateDisabled;
      if (wd->disabledlabel) eina_stringshare_del(wd->disabledlabel);
		wd->disabledlabel = eina_stringshare_add(label);
   	return;
   }
}
*/
/**
 * get the label of button
 *
 * @param obj The button object
 * @return The title of button
 *
 * @ingroup Button
 */
/*
EAPI const char*
elm_button_label_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label;
}
*/
/**
 * get the label of button for each state
 *
 * @param obj The button object
 * @param state The state of button
 * @return The title of button for state
 *
 * @ingroup Button
 */
/*
EAPI const char*
elm_button_label_get_for_state(Evas_Object *obj, UIControlState state)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   if(state == UIControlStateDefault)
      return wd->defaultlabel;
   else if(state == UIControlStateHighlighted)
   	 return wd->highlightedstate;
   else if(state == UIControlStateFocused)
   	 return wd->focusedlabel;
   else if(state == UIControlStateDisabled)
   	 return wd->disabledlabel;
   else
   	return NULL;
}
*/
/**
 * Set the icon used for the button
 *
 * @param obj The button object
 * @param icon  The image for the button
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((wd->icon != icon) && (wd->icon))
     elm_widget_sub_object_del(obj, wd->icon);
   if ((icon) && (wd->icon != icon))
     {
	wd->icon = icon;
	elm_widget_sub_object_add(obj, icon);
	evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	edje_object_part_swallow(wd->btn, "elm.swallow.content", icon);
	edje_object_signal_emit(wd->btn, "elm,state,icon,visible", "elm");
	edje_object_message_signal_process(wd->btn);
	_sizing_eval(obj);
     }
   else
     wd->icon = icon;
}

EAPI Evas_Object *
elm_button_icon_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon;
}
*/
/**
 * Set the button style
 *
 * @param obj The button object
 * @param style The style for the button
 *
 * DEPRECATED. use elm_object_style_set() instead
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_style_set(Evas_Object *obj, const char *style)
{
   elm_widget_style_set(obj, style);
}
*/
/**
 * Turn on/off the autorepeat event generated when the user keeps pressing on the button
 *
 * @param obj The button object
 * @param on  A bool to turn on/off the event
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_autorepeat_set(Evas_Object *obj, Eina_Bool on)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->timer) {
	   ecore_timer_del(wd->timer);
	   wd->timer = NULL;
   }
   wd->autorepeat = on;
}
*/
/**
 * Set the initial timeout before the autorepeat event is generated
 *
 * @param obj The button object
 * @param t   Timeout
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_autorepeat_initital_timeout_set(Evas_Object *obj, double t)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->ar_threshold == t)
     return;

   if (wd->timer)
     {
		ecore_timer_del(wd->timer);
		wd->timer = NULL;
     }

   wd->ar_threshold = t;
}*/

/**
 * Set the interval between each generated autorepeat event
 *
 * @param obj The button object
 * @param t   Interval
 *
 * @ingroup Button
 */
/*
EAPI void
elm_button_autorepeat_gap_timeout_set(Evas_Object *obj, double t)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->ar_interval == t)
     return;

   if (wd->timer)
     {
	ecore_timer_del(wd->timer);
	wd->timer = NULL;
     }

   wd->ar_interval = t;
   if (wd->repeating)
     {
			wd->timer = ecore_timer_add(t, _autorepeat_send, obj);
     }
}
*/
