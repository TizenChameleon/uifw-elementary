#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Editfield Editfield
 *
 * This is a editfield. It can contain a simple label and icon objects.
 */

#define ERASER_PADDING (10)

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object *base;	
	Evas_Object *entry;
	Evas_Object *ricon;
	Evas_Object *licon;
	Evas_Object *eraser;
	const char *label;
	const char *guide_text;
	Eina_Bool needs_size_calc:1;
	Eina_Bool show_guide_text:1;
	Eina_Bool editing:1;
	Eina_Bool single_line : 1;
	Eina_Bool eraser_visible : 1;
	Evas_Event_Mouse_Down down_ev;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _on_focus_hook(void *data, Evas_Object *obj);
static Eina_Bool _empty_entry(Evas_Object *entry);
static void _eraser_drag_end(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _eraser_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _eraser_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _eraser_init(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->label) eina_stringshare_del(wd->label);
	free(wd);
}

static void
_on_focus_hook(void *data, Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char* text;
	if (!elm_widget_focus_get(obj) && !(elm_widget_disabled_get(obj)) ) {	
		
		wd->editing = EINA_FALSE;
		edje_object_signal_emit(wd->base, "elm,action,unfocus", "elm");
		edje_object_signal_emit(wd->base, "elm,state,over,show", "elm");
		
		// aqua feature
		text = elm_entry_entry_get(wd->entry);
		edje_object_part_text_set(wd->base, "elm.content.no.edit", text);
		edje_object_signal_emit(wd->base, "elm,action,no,edit", "elm");
		
		if(_empty_entry(wd->entry)) {
			if(wd->guide_text) {
				edje_object_part_text_set(wd->base, "elm.guidetext", wd->guide_text);
				edje_object_signal_emit(wd->base, "elm,state,guidetext,visible", "elm");
				wd->show_guide_text = EINA_TRUE;
			}
		}
	}
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	_elm_theme_object_set(obj, wd->base, "editfield", "base", elm_widget_style_get(obj));
	edje_object_part_swallow(wd->base, "elm.swallow.content", wd->entry);

	// aqua feature
	if(wd->ricon)	
		edje_object_part_swallow(wd->base, "right_icon", wd->ricon);
	if(wd->licon)
		edje_object_part_swallow(wd->base, "left_icon", wd->licon);	

	if(wd->eraser)
		edje_object_part_swallow(wd->base, "eraser", wd->eraser);
	
	_sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (elm_widget_disabled_get(obj)) {
		elm_object_disabled_set(wd->entry, EINA_TRUE);
		edje_object_signal_emit(wd->base, "elm,action,dim", "elm");		
	}
	else {
		elm_object_disabled_set(wd->entry, EINA_FALSE);
		edje_object_signal_emit(wd->base, "elm,action,dimup", "elm");		
	}
}

static void
_changed_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (wd->needs_size_calc)
	{
		_sizing_eval(obj);
		wd->needs_size_calc = 0;
	}
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;

	edje_object_size_min_calc(wd->base, &minw, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_request_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;
	if (wd->needs_size_calc) return;
	wd->needs_size_calc = 1;
	evas_object_smart_changed(obj);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	double weight_x;
	evas_object_size_hint_weight_get(data, &weight_x, NULL);
	if (weight_x == EVAS_HINT_EXPAND)
		_request_sizing_eval(data);
}

static Eina_Bool
_empty_entry(Evas_Object *entry)
{
	const char* text;
	char *strip_text;
	int len = 0;

	text = elm_entry_entry_get(entry);
	if(!text) return EINA_FALSE;

	strip_text = elm_entry_markup_to_utf8(text);

	if (strip_text) {
		len = strlen(strip_text);
		free(strip_text);
	}

	if(len == 0) {
		return EINA_TRUE;
	}
	else {
		return EINA_FALSE;
	}
}

static void
_entry_changed_cb(void *data, Evas_Object *obj, void* event_info)
{
	const char *text;
	Evas_Object *ef_obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(ef_obj);
	
	if(!wd || !ef_obj) return;

	if(!_empty_entry(wd->entry)) {	
		
		// aqua feature
		text = elm_entry_entry_get(wd->entry);
		edje_object_part_text_set(wd->base, "elm.content.no.edit", text);	
		
		if(wd->guide_text) {
			edje_object_signal_emit(wd->base, "elm,state,guidetext,hidden", "elm");
			wd->show_guide_text = EINA_FALSE;
		}
	}
}

static void
_signal_mouse_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if(!wd) return;

	if(strcmp(source, "left_icon") && strcmp(source, "right_icon") && strcmp(source, "over_change_bg"))
	{
		edje_object_signal_emit(wd->base, "elm,action,focus", "elm");	
		edje_object_signal_emit(wd->base, "elm,state,over,hide", "elm");

		// aqua feature
		edje_object_signal_emit(wd->base, "elm,action,edit", "elm");	
		elm_widget_focus_set(wd->entry, 1);	
		
		if(wd->editing == EINA_FALSE) {
			elm_entry_cursor_end_set(wd->entry);
			evas_object_smart_callback_call(wd->entry, "clicked", NULL);   // after completing of guide, should be eleminated
		}

		if(!_empty_entry(wd->entry)) {
			if(wd->guide_text) {
				edje_object_signal_emit(wd->base, "elm,state,guidetext,hidden", "elm");
				wd->show_guide_text = EINA_FALSE;
			}
		}

		evas_object_smart_callback_call(data, "clicked", NULL);

		wd->editing = EINA_TRUE;
	}
}

static void
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);	
	Evas_Coord w, h;
	if (!wd) return;

	evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
	evas_object_geometry_get(wd->eraser, NULL, NULL, &w, NULL);

	evas_object_size_hint_min_set(wd->eraser, w, h - ERASER_PADDING);
	evas_object_resize(wd->eraser, w, h - ERASER_PADDING);
}

static void
_eraser_drag_end(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || !wd->entry) return;
	elm_entry_entry_set(wd->entry, "");
}

static void
_eraser_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;	
	memcpy(&wd->down_ev, event_info, sizeof(Evas_Event_Mouse_Down));
}

static void
_eraser_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Up *ev = event_info;
	unsigned int dur;
	Evas_Coord distance;
	double pos = 0.0;
	if (!wd) return;

	edje_object_part_drag_value_get(wd->base, "eraser", &pos, NULL);
	if(pos == 1.0){
		edje_object_signal_emit(wd->base, "elm,state,eraser,flick", "elm");
		return;
	}

	dur = ev->timestamp - wd->down_ev.timestamp;
	distance = wd->down_ev.canvas.x - ev->canvas.x;
	
	if ((dur && dur > 1000) || (distance < 10 && distance > -10)) {
		edje_object_signal_emit(wd->base, "elm,state,eraser,drag", "elm");
		return;
	}

	if(((float)distance / dur) > 0.5)	
		edje_object_signal_emit(wd->base, "elm,state,eraser,flick", "elm");
	else
		edje_object_signal_emit(wd->base, "elm,state,eraser,drag", "elm");
}

static void
_eraser_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *eraser;
	if (!wd) return;

	wd->single_line = EINA_TRUE;
	wd->eraser_visible = EINA_TRUE;
	wd->eraser = edje_object_add(evas_object_evas_get(obj));
	elm_widget_sub_object_add(obj, wd->eraser);
	_elm_theme_object_set(obj, wd->eraser, "editfield/eraser", "base", "default");
	edje_object_part_swallow(wd->base, "eraser", wd->eraser);

	evas_object_event_callback_add(wd->eraser, EVAS_CALLBACK_MOUSE_DOWN, _eraser_mouse_down, obj);
	evas_object_event_callback_add(wd->eraser, EVAS_CALLBACK_MOUSE_UP, _eraser_mouse_up, obj);
	edje_object_signal_callback_add(wd->base, "drag", "end", _eraser_drag_end, obj);
}

/**
 * Add a new editfield object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "editfield");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	elm_widget_disable_hook_set(obj, _disable_hook);
	elm_widget_changed_hook_set(obj, _changed_hook);
	elm_widget_can_focus_set(obj, EINA_TRUE);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "editfield", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "*", 
					_signal_mouse_clicked, obj);
	wd->editing = EINA_FALSE;
	wd->entry = elm_entry_add(obj);
	elm_object_style_set(wd->entry, "editfield");
	evas_object_size_hint_weight_set(wd->entry, 0, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->entry, 0, EVAS_HINT_FILL);
	evas_object_event_callback_add(wd->entry,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	edje_object_part_swallow(wd->base, "elm.swallow.content", wd->entry);

	evas_object_smart_callback_add(wd->entry, "changed", _entry_changed_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);

	_eraser_init(obj);
	_sizing_eval(obj);

	return obj;
}

/**
 * Set the label of editfield
 *
 * @param obj The editfield object
 * @param label The label text
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_label_set(Evas_Object *obj, const char *label)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;
	if (wd->label) eina_stringshare_del(wd->label);
	if (label)
	{
		wd->label = eina_stringshare_add(label);
		edje_object_signal_emit(wd->base, "elm,state,text,visible", "elm");
		// aqua feature
		edje_object_signal_emit(wd->base, "elm,state,left,icon,hide", "elm");
	}
	else
	{
		wd->label = NULL;
		edje_object_signal_emit(wd->base, "elm,state,text,hidden", "elm");
		// aqua feature
		edje_object_signal_emit(wd->base, "elm,state,left,icon,show", "elm");
	}

	edje_object_message_signal_process(wd->base);
	edje_object_part_text_set(wd->base, "elm.text", label);

	_sizing_eval(obj);
}

/**
 * Get the label used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI const char*
elm_editfield_label_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->label;
}

/**
 * Get the label used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_guide_text_set(Evas_Object *obj, const char *text)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->guide_text) eina_stringshare_del(wd->guide_text);
	if (text)
	{
		wd->guide_text = eina_stringshare_add(text);
		edje_object_part_text_set(wd->base, "elm.guidetext", wd->guide_text);
		wd->show_guide_text = EINA_TRUE;
	}
	else
	{
		wd->guide_text = NULL;
	}
}

/**
 * Get the guidance text used on the editfield object
 *
 * @param obj The editfield object
 * @return label text
 *
 * @ingroup Editfield
 */
EAPI const char*
elm_editfield_guide_text_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return wd->guide_text;
}

/**
 * Get the entry of the editfield object
 *
 * @param obj The editfield object
 * @return entry object
 *
 * @ingroup Editfield
 */

EAPI Evas_Object *
elm_editfield_entry_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->entry;
}

/**
 * Set the left side icon.
 *
 * @param obj The editfield object
 * @param icon The icon object
 * @return 1 if setting is done, 0 if there is no swallow part for the icon.
 *
 * @ingroup Editfield
 */
EAPI Eina_Bool 
elm_editfield_left_icon_set(Evas_Object *obj, Evas_Object *icon)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if ((wd->licon != icon) && (wd->licon))
		elm_widget_sub_object_del(obj, wd->licon);
	
	if (icon)
	{
		if ( !(edje_object_part_swallow(wd->base, "left_icon", icon)) )
			return EINA_FALSE;		
		wd->licon = icon;
		elm_widget_sub_object_add(obj, icon);
		evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				_changed_size_hints, obj);
		edje_object_signal_emit(wd->base, "elm,state,left,icon,show", "elm");
		edje_object_signal_emit(wd->base, "elm,state,text,hidden", "elm");
		_sizing_eval(obj);
	}	
	return EINA_TRUE;
}

/**
 * Get the left side icon
 *
 * @param obj The editfield object
 * @return icon object
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_left_icon_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->licon;
}

/**
 * Set the right side icon.
 *
 * @param obj The editfield object
 * @param icon The icon object
 * @return 1 if setting is done, 0 if there is no swallow part for the icon.
 *
 * @ingroup Editfield
 */
EAPI Eina_Bool 
elm_editfield_right_icon_set(Evas_Object *obj, Evas_Object *icon)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	
	if ((wd->ricon != icon) && (wd->ricon))
		elm_widget_sub_object_del(obj, wd->ricon);
	
	if (icon)
	{
		if ( !(edje_object_part_swallow(wd->base, "right_icon", icon)) )
			return EINA_FALSE;				
		wd->ricon = icon;
		elm_widget_sub_object_add(obj, icon);
		evas_object_event_callback_add(icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				_changed_size_hints, obj);		
		edje_object_signal_emit(wd->base, "elm,state,right,icon,show", "elm");
		_sizing_eval(obj);
	}	
	return EINA_TRUE;
}

/**
 * Get the right side icon
 *
 * @param obj The editfield object
 * @return icon object
 *
 * @ingroup Editfield
 */
EAPI Evas_Object *
elm_editfield_right_icon_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->ricon;
}

/**
 * Set entry object style as single-line or multi-line.
 *
 * @param obj The editfield object  
 * @param single_line 1 if single-line , 0 if multi-line
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->single_line = single_line;
	elm_entry_single_line_set(wd->entry, single_line);

	if(wd->single_line && wd->eraser_visible)
		edje_object_signal_emit(wd->base, "elm,state,eraser,show", "elm");
	else
		edje_object_signal_emit(wd->base, "elm,state,eraser,hide", "elm");
}

/**
 * Set enable user to clean all of text.
 *
 * @param obj The editfield object  
 * @param visible If true, the eraser is visible and user can clean all of text by using eraser.  
 * If false, the eraser is invisible.
 *
 * @ingroup Editfield
 */
EAPI void
elm_editfield_eraser_set(Evas_Object *obj, Eina_Bool visible)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->eraser_visible = visible;

	if(wd->single_line && wd->eraser_visible)
		edje_object_signal_emit(wd->base, "elm,state,eraser,show", "elm");
	else
		edje_object_signal_emit(wd->base, "elm,state,eraser,hide", "elm");
}

