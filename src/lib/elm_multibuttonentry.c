#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Multibuttonentry Multibuttonentry
 * @ingroup Elementary
 *
 * This is a Multibuttonentry.
 */
		

#define MAX_STR 256		


typedef enum _Multibuttonentry_Pos
{
 MULTIBUTONENTRY_POS_START,
 MULTIBUTONENTRY_POS_END,
 MULTIBUTONENTRY_POS_BEFORE,
 MULTIBUTONENTRY_POS_AFTER,
 MULTIBUTONENTRY_POS_NUM
}Multibuttonentry_Pos;

struct _Multibuttonentry_Item {
	Evas_Object *multibuttonentry;
	Evas_Object *button;
	void *data;
	Evas_Coord vw, rw; // vw: visual width, real width
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *box;
	Evas_Object *entry;
	Evas_Object *label;

	Eina_List *items;
	Eina_List *current;

	Evas_Coord w_box, h_box;
};


static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void	_event_init(Evas_Object *obj);
static void	_set_label(Evas_Object *obj, const char* str);
static void	_change_current_button(Evas_Object *obj, Evas_Object *btn);
static void	_del_button_obj(Evas_Object *obj, Evas_Object *btn);
static void	_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void	_del_button(Evas_Object *obj);
static Elm_Multibuttonentry_Item*	_add_button_item(Evas_Object *obj, char *str, Multibuttonentry_Pos pos, const Elm_Multibuttonentry_Item *reference, void *data);
static void	_add_button(Evas_Object *obj, char *str);
static void	_evas_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info );
static void	_view_init(Evas_Object *obj);


static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;

	if(wd->box){
		elm_box_unpack_all(wd->box);
		wd->box = NULL;
	}

	if (wd->items) {
		Elm_Multibuttonentry_Item *item;
		EINA_LIST_FREE(wd->items, item) {
			_del_button_obj(obj, item->button);
			free(item);
		}
		wd->items = NULL;
	}
	wd->current = NULL;
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	_elm_theme_object_set(obj, wd->base, "multibuttonentry", "base", elm_widget_style_get(obj));
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
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
	Widget_Data *wd = elm_widget_data_get(data);	
	if (!wd) return;
	evas_object_geometry_get(wd->box, NULL, NULL, &wd->w_box, &wd->h_box);
	_sizing_eval(obj);
}

static void
_event_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->base)	return;

	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
}

static void
_set_label(Evas_Object *obj, const char* str)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd)	return;

	if(!wd->label){
		if(!(wd->label = elm_label_add(obj))) return;
		elm_object_style_set(wd->label, "extended/multibuttonentry");
		elm_label_ellipsis_set(wd->label, EINA_TRUE);
		elm_label_wrap_width_set(wd->label, 100);
		elm_label_text_align_set(wd->label, "left");
		evas_object_size_hint_weight_set(wd->label, 0.0, 0.0);
		evas_object_size_hint_align_set(wd->label, EVAS_HINT_FILL, EVAS_HINT_FILL);
		if(wd->box)	elm_box_pack_start(wd->box, wd->label);
		evas_object_show(wd->label);
	}

	elm_label_label_set(wd->label, str);
}

static void
_change_current_button(Evas_Object *obj, Evas_Object *btn)
{
	Eina_List *l;
	Elm_Multibuttonentry_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *label;
	if (!wd) return;

	// change the state of previous button to "default"
	if(wd->current){
		item = eina_list_data_get(wd->current);
		edje_object_signal_emit(item->button,"default", "");
		label = edje_object_part_swallow_get(item->button, "elm.label");
		if(label)	elm_label_text_color_set(label, 0, 0, 0, 255);	
	}
	
	// change the current
	EINA_LIST_FOREACH(wd->items, l, item) {
		if(item->button == btn){
			wd->current = l;
			break;
		}
	}

	// chagne the state of current button to "focused"
	if(wd->current){
		item = eina_list_data_get(wd->current);
		edje_object_signal_emit(item->button,"focused", "");
		label = edje_object_part_swallow_get(item->button, "elm.label");
		if(label)	elm_label_text_color_set(label, 255, 255, 255, 255);	

		evas_object_smart_callback_call(obj, "selected", item);
	}
}

static void
_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Elm_Multibuttonentry_Item *item;
	Evas_Object *label;
	const char *str = NULL;
	if (!wd) return;

	_change_current_button(data, obj);
}

static void
_del_button_obj(Evas_Object *obj, Evas_Object *btn)
{
	Eina_List *l;
	Elm_Multibuttonentry_Item *item;
	Evas_Object *label;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !btn)	return;
	
	if(btn){	
		// del label
		label = edje_object_part_swallow_get(btn, "elm.label");
		edje_object_part_unswallow(btn, label);
		evas_object_del(label);				

		// del button
		evas_object_del(btn);		
	}
}

static void
_item_del(Elm_Multibuttonentry_Item *item)
{
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;
	Widget_Data *wd;

	wd = elm_widget_data_get(item->multibuttonentry);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			wd->items = eina_list_remove(wd->items, _item);
			elm_box_unpack(wd->box, _item->button);
			_del_button_obj(_item->multibuttonentry, _item->button);
			free(_item);
			if(wd->current == l)	
				wd->current = NULL;
			break;
		}
	}
}

static void
_del_button(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object* last_row;
	Evas_Object *hbox;
	Evas_Object *last_btn;
	Elm_Multibuttonentry_Item *item;
	if(!wd || !wd->items) return;

	if(!wd->current){
		// let the last button focus
		item = eina_list_data_get(eina_list_last(wd->items));	
		if(item->button)	_change_current_button(obj, item->button);				
	}else{
		item = eina_list_data_get(wd->current);
		if(item){	
			_item_del(item);
		}
	}
}

static Elm_Multibuttonentry_Item*
_add_button_item(Evas_Object *obj, char *str, Multibuttonentry_Pos pos, const Elm_Multibuttonentry_Item *reference, void *data)
{
	Elm_Multibuttonentry_Item *item;
	Evas_Object *btn;
	Evas_Object *label;
	Evas_Coord w_label, h_label, w_btn, h_btn, padding_outer, padding_inner;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->box || !wd->entry) return NULL;

	// add button
	btn = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj, btn, "multibuttonentry", "btn", elm_widget_style_get(obj));
	edje_object_signal_callback_add(btn, "clicked", "elm", _button_clicked, obj);
	evas_object_size_hint_weight_set(btn, 0.0, 0.0);

	switch(pos){
		case MULTIBUTONENTRY_POS_START:
			elm_box_pack_after(wd->box, btn, wd->label);
			break;
		case MULTIBUTONENTRY_POS_END:
			elm_box_pack_before(wd->box, btn, wd->entry);
			break;
		case MULTIBUTONENTRY_POS_BEFORE:
			if(reference)	elm_box_pack_before(wd->box, btn, reference->button);
			else	elm_box_pack_before(wd->box, btn, wd->entry);
			break;
		case MULTIBUTONENTRY_POS_AFTER:
			if(reference)	elm_box_pack_after(wd->box, btn, reference->button);
			else	elm_box_pack_before(wd->box, btn, wd->entry);
			break;
		default:
			break;
	}

	evas_object_show(btn);

	// add label
	label = elm_label_add(obj);
	elm_object_style_set(label, "extended/multibuttonentry");
	elm_label_label_set(label, str);
	elm_label_ellipsis_set(label, EINA_TRUE);
	elm_label_wrap_width_set(label, 5000);
	edje_object_part_swallow(btn, "elm.label", label);
	evas_object_show(label);

	// decide the size of button
	evas_object_size_hint_min_get(label, &w_label, &h_label);
	edje_object_part_geometry_get(btn, "elm.base", NULL, NULL, NULL, &h_btn);
	edje_object_part_geometry_get(btn, "left.padding", NULL, NULL, &padding_outer, NULL);
	edje_object_part_geometry_get(btn, "left.inner.padding", NULL, NULL, &padding_inner, NULL); 	
	w_btn = w_label + 2*padding_outer + 2*padding_inner;	


	// append item list
	item = ELM_NEW(Elm_Multibuttonentry_Item);
	if (item) {
		item->multibuttonentry = obj;
		item->button = btn;
		item->data = data;
		item->rw = w_btn;
		item->vw =(wd->w_box < w_btn) ? wd->w_box : w_btn;		
		wd->items = eina_list_append(wd->items, item);
	}


	evas_object_resize(btn, item->vw, h_btn);

	if(item->rw != item->vw){
		evas_object_resize(label, item->vw - 4*padding_outer - 4*padding_inner, h_label);	
		elm_label_wrap_width_set(label, item->vw - 4*padding_outer - 6*padding_inner ); 
	}

	evas_object_size_hint_min_set(btn, item->vw, h_btn);

	evas_object_smart_callback_call(obj, "added", item);

	return item;
}

static void
_add_button(Evas_Object *obj, char *str)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;

	//remove entry text
	elm_scrolled_entry_entry_set(wd->entry, "");

	// add button
	Elm_Multibuttonentry_Item *item = _add_button_item(obj, str, MULTIBUTONENTRY_POS_END, NULL, NULL);
}

static void
_evas_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info )
{
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;
	Widget_Data *wd = elm_widget_data_get(data);
	static char str[MAX_STR];	
	if(!wd || !wd->base || !wd->box) return;
	
	strncpy(str, elm_scrolled_entry_entry_get(wd->entry), MAX_STR);
	str[MAX_STR - 1]= 0;

	if((strcmp(str, "") == 0) && ((strcmp(ev->keyname, "BackSpace") == 0)||(strcmp(ev->keyname, "BackSpace(") == 0))){ 
		_del_button(data);	
	}else if((strcmp(str, "") != 0) && (strcmp(ev->keyname, "KP_Enter") == 0 ||strcmp(ev->keyname, "Return") == 0 )){
		_add_button(data, str);
	}else{
		//
	}
}

static void 
_view_init(Evas_Object *obj)
{	
	Widget_Data *wd = elm_widget_data_get(obj);	
	if (!wd) return;
	
	if(!wd->box){
		if(!(wd->box = elm_box_add(obj))) return;
		elm_box_extended_set(wd->box, EINA_TRUE);
		elm_box_homogenous_set(wd->box, EINA_FALSE);
		edje_object_part_swallow(wd->base, "box.swallow", wd->box);
		evas_object_show(wd->box);
	}

	if(!wd->label){
		_set_label(obj, "To: ");	
	}
	
	if(!wd->entry){
		Evas_Coord h_label;
		if(!(wd->entry = elm_scrolled_entry_add(obj))) return;
		elm_scrolled_entry_single_line_set(wd->entry, EINA_TRUE);
		elm_scrolled_entry_entry_set(wd->entry, "");
		elm_scrolled_entry_cursor_end_set(wd->entry);
		evas_object_event_callback_add(elm_scrolled_entry_entry_object_get(wd->entry), EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, obj);
		evas_object_geometry_get(wd->entry, NULL, NULL, NULL, &h_label);
		evas_object_size_hint_min_set(wd->entry, 20, h_label);
		evas_object_size_hint_weight_set(wd->entry, 1.0, 1.0);
		evas_object_size_hint_align_set(wd->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_box_pack_end(wd->box, wd->entry);
		evas_object_show(wd->entry);
	}
}

/**
 * Add a new multibuttonentry to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Multibuttonentry
 */
EAPI Evas_Object *
elm_multibuttonentry_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "multibuttonentry");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);
	
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "multibuttonentry", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);
	evas_object_show(wd->base);
	
	_event_init(obj);
	_view_init(obj);

	return obj;
}

/**
 * Get the label
 *
 * @param obj The multibuttonentry object
 * @return The label, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const char *
elm_multibuttonentry_label_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	if(wd->label){
		return elm_label_label_get(wd->label);
	}

	return NULL;
}

/**
 * Set the label
 *
 * @param obj The multibuttonentry object
 * @param label The text label string
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_label_set(Evas_Object *obj, const char *label)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *row;
	if (!wd) return;

	_set_label(obj, label);	
}

/**
 * Get the entry of the multibuttonentry object
 *
 * @param obj The multibuttonentry object
 * @return entry object
 *
 * @ingroup Multibuttonentry
 */
EAPI Evas_Object *
elm_multibuttonentry_entry_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)	return NULL;
   
   return wd->entry;
}

/**
 * Prepend a new item to the multibuttonentry 
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_start(Evas_Object *obj, const char *label, void *data)
{
	Elm_Multibuttonentry_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_START, NULL, data);
	return item; 
}

/**
 * Append a new item to the multibuttonentry 
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_end(Evas_Object *obj, const char *label, void *data)
{
	Elm_Multibuttonentry_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_END, NULL, data);
	return item; 
}

/**
 * Add a new item to the multibuttonentry before the indicated object
 *
 * reference.
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_before(Evas_Object *obj, const char *label, Elm_Multibuttonentry_Item *before, void *data)
{
	Elm_Multibuttonentry_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_BEFORE, before, data);
	return item; 
}

/**
 * Add a new item to the multibuttonentry after the indicated object
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_after(Evas_Object *obj, const char *label, Elm_Multibuttonentry_Item *after, void *data)
{
	Elm_Multibuttonentry_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_AFTER, after, data);
	return item; 
}

/**
 * Get a list of items in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The list of items, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const Eina_List *
elm_multibuttonentry_items_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->items;
}

/**
 * Get the first item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The first item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_first_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The last item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_last_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->items) return NULL;
	return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the selected item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The selected item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_selected_get(Evas_Object *obj)
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
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_selected_set(Elm_Multibuttonentry_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;
	int cnt = 0;

	if (!item) return;
	wd = elm_widget_data_get(item->multibuttonentry);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->items, l, _item) {
		if (_item == item) {
			_change_current_button(item->multibuttonentry, item->button);
		}
	}
}

/**
 * Remove all items in the multibuttonentry.
 *
 * @param obj The multibuttonentry object
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_items_del(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	if (wd->items){
		Elm_Multibuttonentry_Item *item;
		EINA_LIST_FREE(wd->items, item) {
			elm_box_unpack(wd->box, item->button);
			_del_button_obj(obj, item->button);
			free(item);
		}
		wd->items = NULL;
	}
	wd->current = NULL;
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_del(Elm_Multibuttonentry_Item *item)
{
	if (!item) return;
	 _item_del(item);
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const char *
elm_multibuttonentry_item_label_get(Elm_Multibuttonentry_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->multibuttonentry);
	if (!wd || !wd->items) return NULL;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item){
			Evas_Object *label = edje_object_part_swallow_get(_item->button, "elm.label");
			if(label)	return elm_label_label_get(label);
		}

	return NULL;
}

/**
 * Set the label of a given item
 *
 * @param item The item
 * @param label The text label string
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_label_set(Elm_Multibuttonentry_Item *item, const char *str)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;

	if (!item || !str) return;
	wd = elm_widget_data_get(item->multibuttonentry);
	if (!wd || !wd->items) return;

	EINA_LIST_FOREACH(wd->items, l, _item)
		if (_item == item) {
			Evas_Object *label = edje_object_part_swallow_get(_item->button, "elm.label");
			if(label)	elm_label_label_set(label, str);
			break;
		}
}

/**
 * Get the previous item in the multibuttonentry
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_prev(Elm_Multibuttonentry_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->multibuttonentry);
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
 * Get the next item in the multibuttonentry
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_next(Elm_Multibuttonentry_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Multibuttonentry_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->multibuttonentry);
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
 * @return The data pointer stored, or NULL if none was stored
 *
 * @ingroup Multibuttonentry
 */
EAPI void *
elm_multibuttonentry_item_data_get(Elm_Multibuttonentry_Item *item)
{
	if (!item) return NULL;
	return item->data;
}

/**
 * Set private data of item
 *
 * @param item The item
 * @param data The ponter to the data to be attached
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_data_set(Elm_Multibuttonentry_Item *item, void *data)
{
	if (!item) return;
	item->data = data;
}

