#include "Elementary.h"
#include "elm_priv.h"

#include <errno.h>

/**
 * @addtogroup ActionSheet ActionSheet
 *
 * Display an action sheet
 *
 */

static const double ANIMATION_TIME = 0.4;

/*
 * object hierarchy
 *
 * actionsheet
 *     bg
 *     title
 *     destructive_button
 *     normal_buttons(rect)
 *         normal_button1
 *         normal_button2
 *         ...
 *     cancel_button
 *
 *
 */

typedef struct _Widget_Data
{
	Evas *e;
	Evas_Object *parent, *win, *win_bg, *obj, *layout, *bt_box;

	char *title;

	/* last button delimeters: to use elm_box_pack_after() */
	Evas_Object *bt_cancel, *bt_destructive, *bt_normal;
	Evas_Object *bt_destructive_delim, *bt_normal_delim;

	Eina_List *cb_data_list;

	Evas_Coord show_x, show_y, hide_x, hide_y;
	Evas_Coord curr_x, curr_y;
	Ecore_Timer *layout_timer;

} Widget_Data;

typedef struct _Button_Cb_Data_Wrapper
{
	Evas_Object *obj;	// elm_widget
	void (*cb_func) (void *, Evas_Object *, void *);
	void * cb_data;
	char * button_title;
} Button_Cb_Data_Wrapper;

/*
 * Internal functions
 */

/*
 * hooks
 */
void
_really_del(void *data)
{
	Widget_Data *wd = (Widget_Data *) data;

	evas_object_del(wd->layout);
	evas_object_del(wd->win_bg);

	/* free widget data */
	if(wd->title) free(wd->title);

	// free eina_list
	Eina_List *l;
	Button_Cb_Data_Wrapper *dw;
	EINA_LIST_FOREACH(wd->cb_data_list, l, dw)
		_button_cb_data_wrapper_free(dw);
	eina_list_free(wd->cb_data_list);
		
	free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	// cut sub objects
	elm_widget_sub_object_del(obj, wd->layout);
	elm_widget_sub_object_del(obj, wd->win_bg);
	elm_widget_resize_object_set(obj, NULL);
	elm_widget_hover_object_set(obj, NULL);
	// cut from parent
	elm_widget_sub_object_del(wd->parent, obj);

	edje_object_signal_emit(elm_layout_edje_get(wd->win_bg), "hide", "actionsheet");

}

Button_Cb_Data_Wrapper *
_button_cb_data_wrapper_add(
		Evas_Object *obj, 
		void (*cb_func) (void *, Evas_Object *, void *), 
		void *cb_data, 
		const char* button_title)
{
	Button_Cb_Data_Wrapper *dw;

	dw = (Button_Cb_Data_Wrapper *)calloc(1, sizeof(Button_Cb_Data_Wrapper));
	dw->obj = obj;
	dw->cb_func = cb_func;
	dw->cb_data = cb_data;
	dw->button_title = strdup(button_title);
	
	return dw;
}

void
_button_cb_data_wrapper_free(Button_Cb_Data_Wrapper *dw)
{
	if (dw->button_title) free(dw->button_title);
	free(dw);
}



static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	// do animation	
	Elm_Transit * transit = elm_transit_add(wd->layout);
	elm_transit_fx_insert(transit, elm_fx_transfer_add(wd->layout, wd->show_x, wd->show_y, wd->hide_x, wd->hide_y));
	elm_transit_completion_set(transit, _really_del, wd);
	elm_transit_curve_style_set(transit, ELM_ANIMATOR_CURVE_IN);
	elm_transit_run(transit, ANIMATION_TIME-0.1);
	elm_transit_del(transit);
}

static void
_theme_hook(Evas_Object *obj)
{

}



/*
 * callbacks
 */
static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{

}

static void
_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int x, y, w, h;
	int minw, minh;
	Widget_Data *wd = elm_widget_data_get(obj);

	// win_bg
	evas_object_geometry_get(wd->parent,  &x, &y, &w, &h);	// assume parent object is a window
	//fprintf(stderr, ">>> parent: %d %d %d %d\n", x, y, w, h);
	evas_object_resize(wd->win_bg, w, h);
	evas_object_move(wd->win_bg, x, y);

	// layout
	evas_object_size_hint_weight_set(wd->layout, EVAS_HINT_EXPAND, 0); 
	evas_object_size_hint_align_set(wd->layout, 0.5, 1);

	edje_object_size_min_calc(elm_layout_edje_get(wd->layout), &minw, &minh);
	fprintf(stderr, ">>> layout's min size(by calc): %d %d %d\n", minw, minh, w);

	evas_object_resize(wd->layout, w, minh);

	// Store show/hide location into wd
	//evas_object_move(wd->layout, 0, h - minh);
	wd->show_x = 0;
	wd->show_y = h - minh;
	wd->hide_x = 0;
	wd->hide_y = h;

	evas_object_geometry_get(wd->bt_box, &x, &y, &w, &h);
	fprintf(stderr, ">>> bt_box: %d %d %d %d\n", x, y, w, h);
	evas_object_size_hint_min_get(wd->bt_box, &w, &h);
	fprintf(stderr, ">>> bt_box(size_hint_min): %d %d \n", w, h);

}

static inline int
_move_layout(Widget_Data *wd, const int until, const int offset)
{
	wd->curr_y += offset;
	if(offset < 0) {
		if(wd->curr_y < until) wd->curr_y = until;
	} else {
		if(wd->curr_y > until) wd->curr_y = until;
	}
	evas_object_move(wd->layout, wd->curr_x, wd->curr_y);
	if(wd->curr_y == until) {
		edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,actionsheet,event_block_off", "elm");
		return 0;
	}
	else 
		return 1;
}

static void
_show_animation_end_cb(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	// unblock event 
	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,actionsheet,event_block_off", "elm");
}


static int
_show_layout(void *data)
{
	Widget_Data *wd = (Widget_Data *)data;
	return _move_layout(wd, wd->show_y, -90);
}

static int
_hide_layout(void *data)
{
	
}

static void
_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int x, y, w, h;
	Widget_Data *wd = elm_widget_data_get(obj);

	_resize(wd, wd->e, obj, obj);

	evas_object_show(wd->win_bg);
	evas_object_show(wd->layout);

	// move to hide position
	evas_object_move(wd->layout, wd->hide_x, wd->hide_y);
	// block events
	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,actionsheet,event_block_on", "elm");
	
	// animate to show position
	wd->curr_x = wd->hide_x;
	wd->curr_y = wd->hide_y;

	
	//ecore_timer_add(0.033, _show_layout, wd);
	
	Elm_Transit * transit = elm_transit_add(wd->layout);
	elm_transit_fx_insert(transit, elm_fx_transfer_add(wd->layout, wd->hide_x, wd->hide_y, wd->show_x, wd->show_y));
	elm_transit_completion_set(transit, _show_animation_end_cb, wd);
	elm_transit_curve_style_set(transit, ELM_ANIMATOR_CURVE_OUT);
	elm_transit_run(transit, ANIMATION_TIME);
	elm_transit_del(transit);

	edje_object_signal_emit(elm_layout_edje_get(wd->layout), "show", "actionsheet");
	edje_object_signal_emit(elm_layout_edje_get(wd->win_bg), "show", "actionsheet");

}

static void
_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	evas_object_hide(wd->layout);

}

static void
_button_cb_wrapper(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Button_Cb_Data_Wrapper *dw = (Button_Cb_Data_Wrapper *)data;
	dw->cb_func(dw->cb_data, dw->obj, dw->button_title);
}


/**
 * Add a new actionsheet to the parent
 * Object constructor
 *
 * @param 	parent	The parent object
 * @return	The new actionsheet object, or null if it can't be created.
 *
 * @ingroup	ActionSheet
 */
EAPI Evas_Object *
elm_actionsheet_add (Evas_Object *parent)
{
	Evas_Object *obj, *win, *win_bgimg;
	Evas *e;
	Widget_Data *wd;
	Evas_Coord x,y,w,h;

	/* widget data */
	wd = ELM_NEW(Widget_Data);	// allocate memory

	/* widget object */
	wd->e = evas_object_evas_get(parent);
	obj = elm_widget_add(wd->e);	// get a new smart object(=widget)
	elm_widget_type_set(obj, "actionsheet");	// set a smart object's type name

	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);	// store wd pointer to smart_data->data 

	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);	


	/* actionsheet layout */
	wd->layout = elm_layout_add(parent);
	elm_layout_theme_set(wd->layout, "actionsheet", "base", "default");
	evas_object_size_hint_weight_set(wd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->layout, 0.5, 1);
	elm_widget_sub_object_add(obj, wd->layout);
	elm_widget_hover_object_set(obj, wd->layout);

	/* win_bg */
	wd->win_bg = elm_layout_add(parent);
	elm_layout_theme_set(wd->win_bg, "actionsheet", "bg", "default");
	elm_widget_sub_object_add(obj, wd->win_bg);
	elm_widget_resize_object_set(obj, wd->win_bg);

	/* bt_box */
	wd->bt_box = elm_box_add(wd->layout);
	evas_object_size_hint_weight_set(wd->bt_box, 0, EVAS_HINT_EXPAND);
	elm_layout_content_set(wd->layout, "v_sw", wd->bt_box);
	evas_object_show(wd->bt_box);

	/* set Widget_Data */
	wd->parent = parent;
	wd->obj = obj;

#ifdef SKIP
#endif

	/* set callbacks */
	/* win cb */

	/* obj cb */

	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, obj);

	_resize(wd, wd->e, obj, obj);
	

	fprintf(stderr, ">>>>> after:adding layout\n");
	return obj;
}

/**
 * @brief			Get title of button with given index
 * @param[in]		obj	is an actionsheet object.
 * @param[in]		index	is the index of button
 * @param[out]		title	is button's title
 * @return			length of the string
 * @retval			-EINVAL		Invalid index
 */
EAPI int
elm_actionsheet_button_title_get (
		Evas_Object *obj,
		const int index,
		char **title )
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	
	return 0;
}

/**
 * @brief		Add a button with given title.
 * 				Can add normal button only.
 * @param[in]	obj		An actionsheet.
 * @param[in]	type	The type of the button to be added.
 * @param[in]	title	Title string.
 * @return		void
 */
EAPI void 
elm_actionsheet_button_add (
		Evas_Object *obj, 
		const Elm_Actionsheet_Button_Type type, 
		const char *title,
		void (*cb_func) (void *data, Evas_Object *obj, void *event_info),
		void *cb_data)
{
	Evas_Object **target = NULL;
	Evas_Object **insert_after = NULL;
	Evas_Object *ly_bt;
	char *button_style;
	Widget_Data *wd;
	int x, y, w, h;	

	wd = (Widget_Data *) elm_widget_data_get(obj);
		
	switch(type) {
		case ELM_ACTIONSHEET_BT_CANCEL:
			target = &(wd->bt_cancel);	
			button_style = "cancel";
			break;
		case ELM_ACTIONSHEET_BT_DESTRUCTIVE:
			target = &(wd->bt_destructive);
			button_style = "destructive";
			break;
		case ELM_ACTIONSHEET_BT_NORMAL:
			target = &(wd->bt_normal);
			button_style = "normal";
			break;
		default:
			return;
	}
	// case: cancel, destructive
	// if button object is already exist, just change title
	if ( type != ELM_ACTIONSHEET_BT_NORMAL && *target != NULL  ) {
		elm_button_label_set(*target, title);
	}
	else {

		*target = elm_layout_add(obj);
		elm_layout_theme_set(*target, "actionsheet", "button", button_style);
		edje_object_part_text_set(elm_layout_edje_get(*target), "text", title);
		evas_object_size_hint_weight_set(*target, 0, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(*target, EVAS_HINT_FILL, 0);
		evas_object_show(*target);

		// set callback wrapper
		Button_Cb_Data_Wrapper *dw = _button_cb_data_wrapper_add(obj, cb_func, cb_data, title);
		edje_object_signal_callback_add(elm_layout_edje_get(*target), "elm,action,press", "", _button_cb_wrapper, dw);
		// store data wrapper into eina list (to free all)
		wd->cb_data_list = eina_list_append(wd->cb_data_list, dw);
		
		switch (type) {
			case ELM_ACTIONSHEET_BT_DESTRUCTIVE:
				elm_box_pack_start(wd->bt_box, *target);
				if(NULL == wd->bt_normal_delim) {
						wd->bt_normal_delim = *target;
				}
				break;

			case ELM_ACTIONSHEET_BT_NORMAL:
				if(NULL == wd->bt_normal_delim) {
					elm_box_pack_start(wd->bt_box, *target);
				}
				else {
					elm_box_pack_after(wd->bt_box, *target, wd->bt_normal_delim);
				}
				wd->bt_normal_delim = *target;
				break;

			case ELM_ACTIONSHEET_BT_CANCEL:
				elm_box_pack_end(wd->bt_box, *target);
				break;

			default:
				fprintf(stderr, ">>> wrong button type\n");
				evas_object_del(*target);
				return;
		}
		int box_w, box_h;
		evas_object_size_hint_min_get(wd->bt_box, &box_w, &box_h);
		edje_object_size_min_calc(elm_layout_edje_get(*target), &w, &h);
		//evas_object_size_hint_min_get(*target, &w, &h);
		fprintf(stderr, ">>> calc. box's h: %d + %d\n", box_h, h);
		evas_object_size_hint_min_set(wd->bt_box, box_w>w?box_w:w, box_h+h);

	}

	_resize(wd, wd->e, obj, obj);
}


/**
 * @brief		Set actionsheet title
 * 				Can add normal button only.
 * @param[in]	obj	is an actionsheet.
 * @param[in]	title	title string. If NULL, unset existing title.
 * @return		void
 */
EAPI void 
elm_actionsheet_title_set (
		Evas_Object *obj, 
		const char *title)
{
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

}

/* end of file */
