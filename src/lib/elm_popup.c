#include <Elementary.h>
#include "elm_priv.h"
#include <stdlib.h>

/**
 * @defgroup Popup Popup
 *
 * This is a popup widget. it can be used to display information/ get information from user.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data 
{
	Evas_Object 	*notify;
	Evas_Object	*layout;
	Evas_Object 	*title_area;
	Evas_Object    *title_icon;
	Evas_Object 	*content_area;
	Evas_Object 	*action_area;	
	Evas_Object *parent;
	Evas_Object *parent_app;
	int rot_angle;
	Ecore_Job    *del_job;
	Eina_Bool      delete_me : 1;
};

typedef struct _action_area_Data Action_Area_Data;
struct _action_area_Data 
{
	Evas_Object 	*obj;	
	int response_id;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _elm_popup_buttons_add_valist(Evas_Object *obj,const char *first_button_text,va_list args);
static Evas_Object* _elm_popup_add_button(Evas_Object *obj,char *text,int response_id);
static void _action_area_clicked( void *data, Evas_Object *obj, void *event_info );
static void _block_clicked_cb( void *data, Evas_Object *obj, void *event_info );
static void _show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize_parent(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_parent(void *data, Evas *e, Evas_Object *obj, void *evet_info)
{
	Evas_Object *pop = data;
	Widget_Data *wd = elm_widget_data_get(pop);
	if (!wd) return;
	if (wd->parent == obj)
		{
			evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL, _del_parent, pop);
			wd->parent = NULL;
		}
}

static void
_del_job(void *data)
{
	Evas_Object *obj = data;
	evas_object_del(obj);
}

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;	
	if (!wd->del_job)
   	{
	   if (wd->parent)
		   	{
		   		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL, _del_parent, obj);
	   			wd->del_job = ecore_job_add(_del_job, wd->parent);
    		   wd->parent = NULL;
	    	}
   	}
	free(wd);	
}

static void
_del_pre_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_SHOW, _show, NULL);
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_HIDE, _hide, NULL);
	if(wd->parent_app)
	evas_object_event_callback_del_full(wd->parent_app, EVAS_CALLBACK_RESIZE, _resize_parent, obj);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	elm_layout_theme_set(wd->layout, "popup", "base", elm_widget_style_get(obj));
	elm_notify_orient_set(wd->notify, ELM_NOTIFY_ORIENT_CENTER);
	edje_object_message_signal_process( elm_layout_edje_get(wd->layout));
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;

	edje_object_size_min_calc(elm_layout_edje_get(wd->layout), &minw, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(data);
}

static void _block_clicked_cb( void *data, Evas_Object *obj, void *event_info )
{	
	evas_object_hide((Evas_Object*)data);	
	evas_object_smart_callback_call((Evas_Object *)data, "response", ELM_POPUP_RESPONSE_NONE);		
}

static void
_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{	
	Widget_Data *wd = elm_widget_data_get(obj);   
	 if(wd->parent)
 	  evas_object_show(wd->parent); 	
	elm_layout_theme_set(wd->layout, "popup", "base", elm_widget_style_get(obj));
	_sizing_eval(obj);
	evas_object_show(obj); 	 	

}
static void
_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	
   Widget_Data *wd = elm_widget_data_get(obj);
    if(wd->parent)
 	  evas_object_hide(wd->parent);  
	evas_object_hide(obj); 

}

static void
_resize_parent(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	int rot_angle;
	if(!wd)
		return;
	if(wd->parent)
	{
		rot_angle = elm_win_rotation_get(obj);
		if(wd->rot_angle!=rot_angle)
		{
			elm_win_rotation_with_resize_set(wd->parent,  rot_angle);
			wd->rot_angle = rot_angle;
		}
	}
}

/**
 * Add a new Popup object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Popup
 */
EAPI Evas_Object *
elm_popup_add(Evas_Object *parent_app)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;
	Evas_Object *parent;
	Evas_Coord x,y,w,h;
	int rotation=-1;
	int count;
	unsigned int *prop_data = NULL;
	int ret;

	//FIXME: Keep this window always on top
	parent = elm_win_add(parent_app,"popup",ELM_WIN_DIALOG_BASIC);
	elm_win_alpha_set(parent, EINA_TRUE);	
	elm_win_raise(parent);	

	if(parent_app)
		{
			evas_object_geometry_get(parent_app, &x, &y, &w, &h);
			rotation = elm_win_rotation_get(parent_app);
			evas_object_resize(parent, w, h);
			evas_object_move(parent, x, y);
			if(rotation!=-1)
			{
				elm_win_rotation_set(parent, rotation);
			}				
		}
	else
		{
			ecore_x_window_geometry_get(ecore_x_window_root_get(ecore_x_window_focus_get()),&x, &y, &w, &h);	
			ret  = ecore_x_window_prop_property_get (ecore_x_window_root_get(ecore_x_window_focus_get()), ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
			if( ret && prop_data )
			memcpy (&rotation, prop_data, sizeof (int));

			if (prop_data) free (prop_data);
			evas_object_resize(parent, w, h);
			evas_object_move(parent, x, y);
			if(rotation!=-1)
			{
			elm_win_rotation_with_resize_set(parent, rotation);
			}				
		}
	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "popup");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	wd->parent = parent;
	wd->rot_angle = rotation;

	evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL, _del_parent, obj);
	if(parent_app)
	evas_object_event_callback_add(parent_app, EVAS_CALLBACK_RESIZE, _resize_parent, obj);
	wd->parent_app = parent_app;

	wd->notify= elm_notify_add(parent);		
	elm_widget_resize_object_set(obj, wd->notify);
	elm_notify_orient_set(wd->notify, ELM_NOTIFY_ORIENT_CENTER);
	elm_notify_repeat_events_set(wd->notify, EINA_FALSE);
	evas_object_size_hint_weight_set(wd->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->notify, EVAS_HINT_FILL, EVAS_HINT_FILL);

	wd->layout = elm_layout_add(parent);
	evas_object_size_hint_weight_set(wd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->layout , EVAS_HINT_FILL, EVAS_HINT_FILL);
	
	elm_layout_theme_set(wd->layout, "popup", "base", "default");
	elm_notify_content_set(wd->notify, wd->layout);

	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, NULL);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, NULL);
		
	_sizing_eval(obj);

	return obj;
}

/**
 * Add a new Popup object
 *
 * @param parent The parent object
 * @param title The parent object
 * @param desc_text The parent object
 * @param first_button_text The parent object
 * @Varargs response ID for first button, then additional buttons, ending with NULL
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Popup
 */
Evas_Object *
elm_popup_add_with_buttons(Evas_Object *parent, char *title, char *desc_text,int no_of_buttons, char *first_button_text, ... )
{
	Evas_Object *popup;
	popup = elm_popup_add(parent);
	Widget_Data *wd = elm_widget_data_get(popup);
	char buf[255];
	if(desc_text)
		{
			elm_popup_desc_set(popup, desc_text);
		}
	if(title)
		{
			elm_popup_title_label_set(popup, title);
		}
	if(first_button_text)
		{
			va_list args;	
			va_start (args, first_button_text);	
			wd->action_area= elm_layout_add(popup);							
			elm_layout_content_set(wd->layout, "elm.swallow.buttonArea", wd->action_area);		
	 		snprintf(buf,50,"buttons%d",no_of_buttons);	
	 		elm_layout_theme_set(wd->action_area,"popup","action_area",buf);

			edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,visible", "elm");
			if(wd->title_area)
				{
					edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
				}
			_elm_popup_buttons_add_valist (popup,
				first_button_text,
				args);
			 va_end (args);			
		}
	_sizing_eval(popup);
	evas_object_show(popup);
	return popup;
	
}

/**
 * Set the desc of Popup
 *
 * @param obj The popup object
 * @param label The label text
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_desc_set(Evas_Object *obj, const char *text)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *label;
	char buf[255];
	if (!wd) return;

	if(wd->content_area)
		{
			evas_object_del(wd->content_area);
			wd->content_area=NULL;
		}
	else
		{
			int w;			
			label = elm_label_add(obj);
			elm_object_style_set(label, "popup/description");		
			wd->content_area = elm_scroller_add(obj);
			elm_label_line_wrap_set(label, EINA_TRUE);
			edje_object_part_geometry_get((Evas_Object *)elm_layout_edje_get(wd->layout), "elm.swallow.content", NULL, NULL, &w, NULL);
			elm_label_wrap_width_set(label, w);
			elm_scroller_content_set(wd->content_area,label);
			elm_scroller_policy_set(wd->content_area, ELM_SCROLLER_POLICY_OFF,ELM_SCROLLER_POLICY_AUTO);
			elm_scroller_bounce_set(wd->content_area, EINA_FALSE, EINA_FALSE);		
			elm_label_label_set(label, text);
			evas_object_show(label);			
			elm_layout_content_set(wd->layout, "elm.swallow.content", wd->content_area);			
			evas_object_event_callback_add(wd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
		}		
	_sizing_eval(obj);
}

/**
 * Get the label used on the Popup object
 *
 * @param obj The Popup object
 * @return label text
 *
 * @ingroup Popup
 */
EAPI const char* 
elm_popup_desc_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return elm_label_label_get(wd->content_area);
}

/**
 * Set the title of Popup
 *
 * @param obj The popup object
 * @param label The title text
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_title_label_set(Evas_Object *obj, const char *text)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	char buf[255];
	if (!wd) return;

	if(wd->title_area)
		{
			evas_object_del(wd->title_area);
			wd->title_area=NULL;
		}
		
	wd->title_area= elm_label_add(obj);	
	elm_object_style_set(wd->title_area,"popup/title");
	elm_label_label_set(wd->title_area, text);
	elm_layout_content_set(wd->layout, "elm.swallow.title", wd->title_area);		
	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,visible", "elm");
	if(wd->action_area)
		{
			edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
		}
	if(wd->title_icon)
		{
			edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,icon,visible", "elm");
		}
		
	_sizing_eval(obj);
}

/**
 * Get the title of Popup object
 *
 * @param obj The Popup object
 * @return title text
 *
 * @ingroup Popup
 */
EAPI const char* 
elm_popup_title_label_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return elm_label_label_get(wd->title_area);
}

/**
 * Set the icon in the title area of Popup
 *
 * @param obj The popup object
 * @param label The title icon
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_title_icon_set(Evas_Object *obj, Evas_Object *icon)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	char buf[255];
	if (!wd) return;

	if(wd->title_icon)
		{
			evas_object_del(wd->title_icon);
			wd->title_icon=NULL;
		}
		
	wd->title_icon= icon;	
	elm_layout_content_set(wd->layout, "elm.swallow.title.icon", wd->title_icon);		
	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,icon,visible", "elm");	
	_sizing_eval(obj);
}
/**
 * Get the title icon of Popup object
 *
 * @param obj The Popup object
 * @return icon title
 *
 * @ingroup Popup
 */
 
EAPI Evas_Object* 
elm_popup_title_icon_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return wd->title_icon;
}

/**
 * Set the content of Popup
 *
 * @param obj The popup object
 * @param content The content widget
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_content_set(Evas_Object *obj, Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);	
	if (!wd) return;

	if(wd->content_area)
		{
			evas_object_del(wd->content_area);
			wd->content_area=NULL;
		}			
	wd->content_area= content;							
	elm_layout_content_set(wd->layout, "elm.swallow.content", wd->content_area);	
	evas_object_event_callback_add(wd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	_sizing_eval(obj);
}

/**
 * Get the content of Popup object
 *
 * @param obj The Popup object
 * @return content packed in popup widget
 *
 * @ingroup Popup
 */
EAPI Evas_Object*
elm_popup_content_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return wd->content_area;
}

static void _action_area_clicked( void *data, Evas_Object *obj, void *event_info )
{
	Action_Area_Data *adata = (Action_Area_Data *)data;
	if (!adata) return;	
	evas_object_smart_callback_call(adata->obj, "response", adata->response_id);	
	evas_object_hide(adata->obj);
}

static Evas_Object* _elm_popup_add_button(Evas_Object *obj,char *text,int response_id)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	Evas_Object *btn;
	Action_Area_Data *adata = malloc(sizeof(Action_Area_Data));	
	btn = elm_button_add(obj);
	elm_object_style_set(btn, "circulargrey");
	elm_button_label_set(btn,text);
	adata->response_id = response_id;
	adata->obj = obj;
	evas_object_smart_callback_add(btn, "clicked", _action_area_clicked, adata);	
	return btn;
}

static void _elm_popup_buttons_add_valist(Evas_Object *obj,const char *first_button_text,va_list args)
{
	const char *text;	
	char buf[50];
	int response=0;
	int index=0;
	if(first_button_text==NULL)
		return;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	text = first_button_text;	
	response = va_arg (args, int);
	Evas_Object *btn;
	 while (text != NULL)
	 	{
	 		btn = _elm_popup_add_button(obj,text,response);
			++index;			
			snprintf(buf,50,"actionbtn%d",index);				
			elm_layout_content_set(wd->action_area,buf,btn);
			evas_object_event_callback_add(wd->action_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
			
			text = va_arg (args, char*);
			if (text == NULL)
				break;
			response = va_arg (args, int);
	 	}		 
}

static void _elm_popup_timeout( void *data, Evas_Object *obj, void *event_info )
{	
	Widget_Data *wd = elm_widget_data_get((Evas_Object *)data);
	evas_object_hide((Evas_Object*)data);	
	evas_object_smart_callback_call((Evas_Object *)data, "response", ELM_POPUP_RESPONSE_TIMEOUT);		
}

/**
 * Set the content of Popup
 *
 * @param obj The popup object
 * @param content The content widget
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_buttons_add(Evas_Object *obj,int no_of_buttons, char *first_button_text,  ...)

{
	Widget_Data *wd = elm_widget_data_get(obj);	
	char buf[50];
	if (!wd) return;	
	va_list args;	

	va_start (args, first_button_text);	

	if(wd->action_area)
		{
			evas_object_del(wd->action_area);
			wd->action_area=NULL;
		}
	wd->action_area= elm_layout_add(obj);			
	elm_layout_content_set(wd->layout, "elm.swallow.buttonArea", wd->action_area);		
	evas_object_size_hint_weight_set(wd->action_area, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->action_area, EVAS_HINT_FILL, EVAS_HINT_FILL);
	 snprintf(buf,50,"buttons%d",no_of_buttons);	
	 elm_layout_theme_set(wd->action_area,"popup","action_area",buf);
		
	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,visible", "elm");

	if(wd->title_area)
		{
			edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
		}

	_elm_popup_buttons_add_valist (obj,
				first_button_text,
				args);
	
	 va_end (args);
	_sizing_eval(obj);
}

/**
 * Set the time before the popup window is hidden. <br>
 *
 * @param obj The popup object
 * @param timeout The timeout
 *
 * @ingroup Popup
 */
EAPI void 
elm_popup_timeout_set(Evas_Object *popup, int timeout)
{	
	Widget_Data *wd = elm_widget_data_get(popup);
	if (!wd) 	return;	
	elm_notify_timeout_set(wd->notify, timeout);
	evas_object_smart_callback_add(wd->notify, "notify,timeout", _elm_popup_timeout, popup);
}

/**
 * Set the mode of popup, by default ELM_POPUP_TYPE_NONE is set i.e, popup <br> 
 * will not close when clicked outside. if ELM_POPUP_TYPE_ALERT is set, popup will close<br> 
 * when clicked outside, and ELM_POPUP_RESPONSE_NONE is sent along with response signal.
 *
 * @param obj The popup object
 * @param mode  Elm_Popup_Mode
 *
 * @ingroup Popup
 */
 
EAPI void elm_popup_set_mode(Evas_Object *popup, Elm_Popup_Mode mode)
{
	Widget_Data *wd = elm_widget_data_get(popup);
	if (!wd) 	return;	
	evas_object_smart_callback_add(wd->notify, "block,clicked", _block_clicked_cb, popup);
}

/**
 * Hides the popup by emitting response signal
 * @param obj The popup object
 * @param response_id  response ID of the signal to be emitted along with response signal
 *
 * @ingroup Popup
 */
 
EAPI void elm_popup_response(Evas_Object *popup, int  response_id)
{
	Widget_Data *wd = elm_widget_data_get(popup);
	if (!wd) 	return;	
	evas_object_hide(popup);	
	evas_object_smart_callback_call((Evas_Object *)popup, "response", response_id);	
}

/**
 * the direction from which popup will appear as well as the location of popup can be controlled by this API
 * @param obj The popup object
 * @param Elm_Popup_Orient  the orientation of the popup
 *
 * @ingroup Popup
 */
 
EAPI void elm_popup_orient_set(Evas_Object *popup, Elm_Popup_Orient orient)
{
	Widget_Data *wd = elm_widget_data_get(popup);
	if (!wd) 	return;	
	Elm_Notify_Orient notify_orient;
	switch (orient)
     {
			case ELM_POPUP_ORIENT_TOP:
			notify_orient = ELM_NOTIFY_ORIENT_TOP;
			break;
			case ELM_POPUP_ORIENT_CENTER:
			notify_orient = ELM_NOTIFY_ORIENT_CENTER;
			break;
			case ELM_POPUP_ORIENT_BOTTOM:
			notify_orient = ELM_NOTIFY_ORIENT_BOTTOM;
			break;
			case ELM_POPUP_ORIENT_LEFT:
			notify_orient = ELM_NOTIFY_ORIENT_LEFT;
			break;
			case ELM_POPUP_ORIENT_RIGHT:
			notify_orient = ELM_NOTIFY_ORIENT_RIGHT;
			break;
			case ELM_POPUP_ORIENT_TOP_LEFT:
			notify_orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
			break;
			case ELM_POPUP_ORIENT_TOP_RIGHT:
			notify_orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
			break;
			case ELM_POPUP_ORIENT_BOTTOM_LEFT:
			notify_orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
			break;
			case ELM_POPUP_ORIENT_BOTTOM_RIGHT:
			notify_orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
        	break;
     }
	elm_notify_orient_set(wd->notify, notify_orient);
}

