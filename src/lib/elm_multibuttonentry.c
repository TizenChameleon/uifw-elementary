#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Multibuttonentry Multibuttonentry
 * @ingroup Elementary
 *
 * This is a Multibuttonentry.
 */
		

#define MAX_STR 256		
#define MAX_LABEL 20	
#define MIN_ENTRY_WIDTH 50

typedef struct _Multibuttonentry_Item Multibuttonentry_Item;
struct _Multibuttonentry_Item {
	Evas_Object *multibuttonentry;
	Evas_Object *button;
	Evas_Object *hbox;
	Evas_Object *row;

	Evas_Coord rw;
	Evas_Coord w;
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *vbox;
	Evas_Object *entry;
	
	char label[MAX_LABEL];

	Eina_List *items;
	Eina_List *current;
	
	Evas_Coord w_vbox, h_vbox;
	Evas_Coord w_base, h_base;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _view_init(Evas_Object *obj);
static void _view_update(Evas_Object *obj);
static Evas_Object* _add_row(Evas_Object *obj, const char* label);
static Multibuttonentry_Item* _add_button_item(Evas_Object *obj, char *str);

static void _del_row(Evas_Object *obj, Evas_Object *row);





static void
_del_hook(Evas_Object *obj)
{
	Eina_List *rows;
	Eina_List *l;
	Evas_Object *cur_row;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;

	rows = evas_object_box_children_get(wd->vbox);
	EINA_LIST_FOREACH(rows, l, cur_row){
		rows = eina_list_remove(rows, cur_row);
		if(cur_row){
			_del_row(obj, cur_row);
		}
	}
	evas_object_box_remove_all(wd->vbox, 1);

	if (wd->items) {
		Multibuttonentry_Item *item;
		EINA_LIST_FREE(wd->items, item) {
			free(item);
		}
	}
	
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	
	_elm_theme_object_set(obj, wd->base, "multibuttonentry", "base", elm_widget_style_get(obj));
	_view_update(obj);
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
	evas_object_geometry_get(wd->base, NULL, NULL, &wd->w_base, &wd->h_base);
	evas_object_geometry_get(wd->vbox, NULL, NULL, &wd->w_vbox, &wd->h_vbox);
	_view_update(data);
}

static void
_change_current_button(Evas_Object *obj, Evas_Object *btn)
{
	Eina_List *l;
	Multibuttonentry_Item *item;
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
	}
}

static void
_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd) return;

	_change_current_button(data, obj);
	evas_object_smart_callback_call(data, "selected", "test");
}

static void
_event_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !wd->base)	return;

	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
}

static Multibuttonentry_Item*
_add_button_item(Evas_Object *obj, char *str)
{
	Multibuttonentry_Item *item;
	Evas_Object *btn;
	Evas_Object *label;
	Evas_Coord w_label, w_btn, padding_outer, padding_inner;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return NULL;

	// add button
	btn = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj, btn, "multibuttonentry", "btn", elm_widget_style_get(obj)); 
	evas_object_show(btn);

	edje_object_signal_callback_add(btn, "clicked", "elm", _button_clicked, obj);

	// add label
	label = elm_label_add(obj);
	elm_object_style_set(label, "extended/multibuttonentry");
	elm_label_label_set(label, str);
	elm_label_ellipsis_set(label, EINA_TRUE);
	elm_label_wrap_width_set(label, 5000);
	elm_label_text_align_set(label, "middle");
	edje_object_part_swallow(btn, "elm.label", label);
	evas_object_show(label);

	// decide the size of button
	evas_object_size_hint_min_get(label, &w_label, NULL);
	edje_object_part_geometry_get(btn, "left.padding", NULL, NULL, &padding_outer, NULL);
	edje_object_part_geometry_get(btn, "left.inner.padding", NULL, NULL, &padding_inner, NULL); 	
	w_btn = w_label + 2*padding_outer + 2*padding_inner;
	evas_object_resize(btn, w_btn, 0);

	// append item list
	item = ELM_NEW(Multibuttonentry_Item);
	if (item) {
		item->button = btn;
		item->w = w_btn;
		item->rw = w_btn;
		item->multibuttonentry = obj;
		wd->items = eina_list_append(wd->items, item);
	}

	return item;
}

static void
_del_button_item(Evas_Object *obj, Evas_Object *btn)
{
	Eina_List *l;
	Multibuttonentry_Item *item;
	Evas_Object *label;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !btn)	return;
	
	EINA_LIST_FOREACH(wd->items, l, item) {
		if(item->button == btn){
			wd->items = eina_list_remove(wd->items, item);

			// del label
			label = edje_object_part_swallow_get(btn, "elm.label");
			edje_object_part_unswallow(btn, label);
			evas_object_del(label);				

			// del button
			evas_object_del(btn);

			// del item
			free(item);

			wd->current = NULL;
			
			break;
		}
	}

}


static void
_set_label(Evas_Object *obj, Evas_Object *row, const char* label)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd || !row)	return;

	strncpy(wd->label, label, MAX_LABEL);
	wd->label[MAX_LABEL - 1]= 0;
	edje_object_part_text_set(row, "elm.label", wd->label);
}

static Evas_Object*
_add_row(Evas_Object *obj, const char* label)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *row;
	Evas_Object *hbox;	
	if(!wd || !wd->base || !wd->vbox)	return NULL;

	// row
	row = edje_object_add(evas_object_evas_get(obj));
	_elm_theme_object_set(obj, row, "multibuttonentry", "row", elm_widget_style_get(obj));	
	evas_object_show(row);
	evas_object_box_append(wd->vbox, row);
	evas_object_box_align_set(wd->vbox, 0.0, 0.0);

	// label
	if(label){
		_set_label(obj, row, label);
	}

	// hbox	
	hbox = evas_object_box_add( evas_object_evas_get(obj));
	evas_object_box_layout_set(hbox, evas_object_box_layout_horizontal, NULL, NULL);	
	evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_box_align_set(hbox, 0.0, 0.0);
	edje_object_part_swallow(row, "elm.hbox", hbox);
	evas_object_show(hbox);
	
	// add entry
	edje_object_part_swallow(row, "elm.entry", wd->entry);

	return row;
}


static void
_del_row(Evas_Object *obj, Evas_Object *row)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *hbox;
	Evas_Object *entry;
	if(!wd || !wd->vbox || !row)	return;
	
	hbox = edje_object_part_swallow_get(row, "elm.hbox");
	entry = edje_object_part_swallow_get(row, "elm.entry");

	if(hbox){
		evas_object_box_remove_all(hbox, 1);
		edje_object_part_unswallow(row, hbox);
		evas_object_del(hbox);
	}

	if(entry){
		edje_object_part_unswallow(row, entry);
	}

	evas_object_box_remove(wd->vbox, row);
	evas_object_del(row);
}


static void
_add_button(Evas_Object *obj, char *str)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object* last_row;
	Evas_Object *hbox;
	Evas_Object *last_btn;
	Evas_Object *btn;
	Evas_Coord x_row, w_row, x_last_btn, w_last_btn, w_new_btn;
	Evas_Coord row_rel2, last_btn_rel2;
	if(!wd) return;

	if(!(last_row = eina_list_data_get(eina_list_last(evas_object_box_children_get(wd->vbox))))) return;
	if(!(hbox = edje_object_part_swallow_get(last_row, "elm.hbox"))) return;
	last_btn = eina_list_data_get(eina_list_last(evas_object_box_children_get(hbox)));


	//remove entry text
	elm_entry_entry_set(wd->entry, "");

	// add button
	Multibuttonentry_Item *item = _add_button_item(obj, str);
	btn = item->button;
	edje_object_signal_emit(last_row,"default", "");

	// position
	evas_object_geometry_get(last_row, &x_row, NULL, &w_row, NULL);
	evas_object_geometry_get(last_btn, &x_last_btn, NULL, &w_last_btn, NULL);
	evas_object_geometry_get(btn, NULL, NULL, &w_new_btn, NULL);
	
	row_rel2 = x_row + w_row;
	last_btn_rel2 = x_last_btn + w_last_btn;
		

	if((last_btn_rel2 < row_rel2) && (row_rel2 - last_btn_rel2 > w_new_btn))
	{
		evas_object_box_append(hbox, btn);
		item->hbox = hbox;
		item->row = last_row;
	
		if((row_rel2 - (last_btn_rel2 + w_new_btn)) < MIN_ENTRY_WIDTH){
			edje_object_part_unswallow(last_row, wd->entry);
			_add_row(obj, NULL);
			_view_update(obj);
		}
	}
	else if(!evas_object_box_children_get(hbox))
	{	
		Evas_Coord w_label, h_label;
		edje_object_part_geometry_get(last_row, "elm.label", NULL, NULL, &w_label, &h_label);	
		evas_object_resize(btn, w_row - w_label, 0);
		item->rw = w_row - w_label;
		evas_object_box_append(hbox, btn);
		item->hbox = hbox;
		item->row = last_row;

		if(w_new_btn > w_row - w_label){
			Evas_Coord w, h, padding_outer, padding_inner, base_w, base_h;
			Evas_Object *label;

			edje_object_part_geometry_get(btn, "elm.base", NULL, NULL, &base_w, &base_h);	
			label = edje_object_part_swallow_get(btn, "elm.label");
			
			edje_object_part_geometry_get(btn, "left.padding", NULL, NULL, &padding_outer, NULL);
			edje_object_part_geometry_get(btn, "left.inner.padding", NULL, NULL, &padding_inner, NULL); 
			edje_object_part_geometry_get(btn, "elm.label", NULL, NULL, &w, &h); 

			evas_object_resize(btn, w_row - w_label, 0);
			item->rw = w_row - w_label;
			evas_object_resize(label, item->rw - 4*padding_outer - 4*padding_inner, h_label); 	
			elm_label_wrap_width_set(label, item->rw - 4*padding_outer - 6*padding_inner ); 
		}

		edje_object_part_unswallow(last_row, wd->entry);
		_add_row(obj, NULL);
		_view_update(obj);
	}
	else
	{
		Evas_Coord w_label, h_label;
		Evas_Object *row;
			
		edje_object_part_unswallow(last_row, wd->entry);	
		row = _add_row(obj, NULL);
		hbox = edje_object_part_swallow_get(row, "elm.hbox");
		edje_object_part_geometry_get(row, "elm.label", NULL, NULL, &w_label, &h_label);



		if(w_new_btn > w_row - w_label){
			Evas_Coord w, h, padding_outer, padding_inner, base_w, base_h;
			Evas_Object *label;

			edje_object_part_geometry_get(btn, "elm.base", NULL, NULL, &base_w, &base_h);	
			label = edje_object_part_swallow_get(btn, "elm.label");
			
			edje_object_part_geometry_get(btn, "left.padding", NULL, NULL, &padding_outer, NULL);
			edje_object_part_geometry_get(btn, "left.inner.padding", NULL, NULL, &padding_inner, NULL); 
			edje_object_part_geometry_get(btn, "elm.label", NULL, NULL, &w, &h); 

			evas_object_resize(btn, w_row - w_label, 0);
			item->rw = w_row - w_label;
			evas_object_resize(label, item->rw - 4*padding_outer - 4*padding_inner, h_label); 	
			elm_label_wrap_width_set(label, item->rw - 4*padding_outer - 6*padding_inner ); 
			
			
			edje_object_part_unswallow(last_row, wd->entry);
			_add_row(obj, NULL);
		}


		evas_object_box_append(hbox, btn);
		item->hbox = hbox;
		item->row = row;
			
		_view_update(obj);
	}	
}

static void _remove_unused_row(Evas_Object *obj)
{
	Eina_List *rows;
	Eina_List *l;
	Evas_Object *cur_row;
	Evas_Object *last_rows;
	Evas_Object* hbox;
	Widget_Data *wd = elm_widget_data_get(obj);
	if(!wd) return;


	rows = evas_object_box_children_get(wd->vbox);
	EINA_LIST_FOREACH(rows, l, cur_row){
		hbox = edje_object_part_swallow_get(cur_row, "elm.hbox");

		if(hbox){
			if(rows != l && !evas_object_box_children_get(hbox)){
				int space;
				edje_object_part_geometry_get(eina_list_data_get(eina_list_prev(l)), "elm.space", NULL, NULL, &space, NULL);

				Evas_Object *prev_row = eina_list_data_get(eina_list_prev(l));
				hbox = edje_object_part_swallow_get(prev_row, "elm.hbox");
				
				if(hbox && eina_list_count(evas_object_box_children_get(hbox)) == 0){
					edje_object_signal_emit(cur_row,"empty", "");
					rows = eina_list_remove(rows, cur_row);
					_del_row(obj, cur_row);
				}else if((eina_list_data_get(eina_list_last(evas_object_box_children_get(wd->vbox))) == cur_row) && (space < MIN_ENTRY_WIDTH)){
					break;
				}else{
					edje_object_signal_emit(cur_row,"empty", "");
					rows = eina_list_remove(rows, cur_row);
					_del_row(obj, cur_row);
				}

			}
		}
	}

	last_rows = eina_list_data_get(eina_list_last(evas_object_box_children_get(wd->vbox)));
	if(last_rows) edje_object_part_swallow(last_rows, "elm.entry", wd->entry);

}

static void
_del_button(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object* last_row;
	Evas_Object *hbox;
	Evas_Object *last_btn;
	if(!wd) return;


	if(!wd->current){
		// let the last button focus
		if(!(last_row = eina_list_data_get(eina_list_last(evas_object_box_children_get(wd->vbox))))) return;
		if(!(hbox = edje_object_part_swallow_get(last_row, "elm.hbox"))) return;
		last_btn = eina_list_data_get(eina_list_last(evas_object_box_children_get(hbox)));

		if(last_btn)	{
			_change_current_button(obj, last_btn);				
		}else{
			if(!(last_row = eina_list_data_get(eina_list_prev(eina_list_last(evas_object_box_children_get(wd->vbox))))) )return;
			if(!(hbox = edje_object_part_swallow_get(last_row, "elm.hbox"))) return;
			last_btn = eina_list_data_get(eina_list_last(evas_object_box_children_get(hbox)));
			if(last_btn)
				_change_current_button(obj, last_btn);		
		}
		
	}else{
		Eina_List *l;
		Multibuttonentry_Item *item;
		Eina_List *rows;
		Evas_Object* cur_row;
		Evas_Object* cur_box;
		Evas_Coord w_entry;
		Evas_Coord pre_space, next_space;
		Eina_List *next;


		if(!(rows = evas_object_box_children_get(wd->vbox))) return;


		next = eina_list_next(wd->current);
		item = eina_list_data_get(wd->current);

		EINA_LIST_FOREACH(rows, l, cur_row){
			if(cur_row == item->row){
				rows = l;
				break;
			}
		}				
		
		cur_row = item->row;
		if(!(cur_box = edje_object_part_swallow_get(cur_row, "elm.hbox"))) return;
		

		edje_object_part_geometry_get(cur_row, "elm.space", NULL, NULL, &pre_space, NULL);	
		edje_object_part_geometry_get(eina_list_data_get(eina_list_next(rows)), "elm.space", NULL, NULL, &next_space, NULL);	
		
		if(item){
			pre_space += item->rw;
			evas_object_box_remove(item->hbox, item->button);
			_del_button_item(obj, item->button);
			if(cur_box)
				if(eina_list_count(evas_object_box_children_get(cur_box)) == 0)
						edje_object_signal_emit(cur_row,"empty", "");
			//_remove_unused_row(obj);
		}

	
		l = next;
		EINA_LIST_FOREACH(l, l, item) {	
			if(item->hbox != cur_box){	
				if(!cur_box) break;
				
				if(item->w < pre_space){	
					next_space += item->rw;
					evas_object_box_remove(item->hbox, item->button);
					evas_object_resize(item->button, item->w, 0);
					evas_object_box_append(cur_box, item->button);
					pre_space -= item->w;
					item->hbox = cur_box;
					item->row = cur_row;
				}
				else if((eina_list_count(evas_object_box_children_get(cur_box)) == 0)){
					next_space += item->rw;
					evas_object_box_remove(item->hbox, item->button);
					evas_object_resize(item->button, item->rw, 0);
					evas_object_box_append(cur_box, item->button);
					pre_space -= item->w;
					item->hbox = cur_box;
					item->row = cur_row;
				}
				else{
						
					if(!(rows = eina_list_next(rows)))	break;
					if(!(cur_row = eina_list_data_get(rows)))	break;
					if(!(cur_box = edje_object_part_swallow_get(cur_row, "elm.hbox"))) break;
					
					pre_space = next_space;
					edje_object_part_geometry_get(eina_list_data_get(eina_list_next(rows)), "elm.space", NULL, NULL, &next_space, NULL);
				}


				if(cur_box)
					if(eina_list_count(evas_object_box_children_get(cur_box)) == 0)
						edje_object_signal_emit(cur_row,"empty", "");

				
				_remove_unused_row(obj);
			}
		}

		_remove_unused_row(obj);
		
	}
}

static void
_evas_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info )
{
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;
	Widget_Data *wd = elm_widget_data_get(data);
	static char str[MAX_STR];	
	if(!wd || !wd->base || !wd->vbox || wd->entry != obj) return;

	strncpy(str, elm_entry_entry_get(wd->entry), MAX_STR);
	str[MAX_STR - 1]= 0;

	//printf("\n>>>>>>[%s][%d]key val = %s\n", __FUNCTION__, __LINE__, ev->keyname);
	
	if((strcmp(str, "") == 0) && ((strcmp(ev->keyname, "BackSpace") == 0)||(strcmp(ev->keyname, "BackSpace(") == 0))){ 
		_del_button(data);	
	}
	else if((strcmp(str, "") != 0) && (strcmp(ev->keyname, "KP_Enter") == 0 ||strcmp(ev->keyname, "Return") == 0 )){
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

	if(!wd->entry){
		wd->entry = elm_entry_add(obj);
		elm_entry_single_line_set(wd->entry, EINA_TRUE);
		elm_entry_ellipsis_set(wd->entry, EINA_TRUE);
		elm_entry_entry_set(wd->entry, "");
		elm_entry_cursor_end_set(wd->entry);
		//elm_entry_background_color_set(wd->entry, 255, 0, 255, 255);
		evas_object_event_callback_add(wd->entry, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, obj);		
	}
	
	if(!wd->vbox){
		wd->vbox = evas_object_box_add(evas_object_evas_get(obj));
		evas_object_box_layout_set(wd->vbox, evas_object_box_layout_vertical, NULL, NULL);	
		edje_object_part_swallow(wd->base, "vbox.swallow", wd->vbox);
		evas_object_show(wd->vbox);
		_add_row(obj, "To:");
	}
}

static void 
_view_update(Evas_Object *obj)
{	
	Widget_Data *wd = elm_widget_data_get(obj);	
	Eina_List *l;
	Evas_Object *item;
	Eina_List* list;	
	if (!wd || !wd->vbox) return;

	list = evas_object_box_children_get(wd->vbox);
	EINA_LIST_FOREACH(list, l, item)
		evas_object_resize(item, wd->w_vbox, wd->h_vbox);
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
	_view_update(obj);

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

	if(wd->label)
		return wd->label;

	return NULL;
}

/**
 * Set the label
 *
 * @param obj The multibuttonentry object
 * @param label The text label string in UTF-8
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_label_set(Evas_Object *obj, const char *label)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Object *row;
	if (!wd || !wd->vbox) return;

	if(!(row = eina_list_data_get(evas_object_box_children_get(wd->vbox)))) return;
	_set_label(obj, row, label);	
}


