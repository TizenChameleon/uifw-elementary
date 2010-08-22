/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup PageControl PageControl
 * @ingroup Elementary
 *
 *  A page control is a succession of dots centered in the control.
 *  Each dot corresponds to a page in the application’s document (or other data-model entity),
 *  with the white dot indicating the currently viewed page.
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _Page_Item Page_Item;

struct _Widget_Data
{
	Evas_Object *base;
	Evas_Object *hbox;
	int page_count;
	Eina_List *page_list;
	unsigned int cur_page_id;
	Evas_Object *parent;
	double scale_factor;
};

struct _Page_Item
{
	Evas_Object *obj;
	Evas_Object *base;
	int page_id;
};


static void 
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	_elm_theme_object_set(obj, wd->base, "pagecontrol", "base", elm_widget_style_get(obj));
}

static void 
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;

	if (!wd)
		return;
   
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	evas_object_size_hint_min_set(obj, -1, -1);
	evas_object_size_hint_max_set(obj, -1, -1);
}

static void 
_item_free(Evas_Object *obj, Page_Item *it)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return;

	if (wd->page_list)
		wd->page_list = eina_list_remove(wd->page_list, it);

	if (it->base)
		evas_object_del(it->base);

	if (it)
		free(it);
	it = NULL;
	return;
}

static void 
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Page_Item *it;
	Eina_List *l, *clear = NULL;

	EINA_LIST_FOREACH(wd->page_list, l, it) clear = eina_list_append(clear, it);
	EINA_LIST_FREE(clear, it) _item_free(obj, it);

	if (wd)
		free(wd);
	wd = NULL;

	return;
}

static Page_Item *
_page_find(Evas_Object *obj, unsigned int index)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return NULL;

	Page_Item *it;
	Eina_List *l;

	int i = 0;
	EINA_LIST_FOREACH(wd->page_list, l, it)
	{
		if (i == index) return it;
		i++;
	}
	return NULL;
}

static void 
_indicator_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *wd_obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(wd_obj);
	if (!wd)
		return;

	Page_Item *it;
	Eina_List *l;

	int page_id = 0;
	EINA_LIST_FOREACH(wd->page_list, l, it)
	{
		if (it->base == obj)
			page_id = it->page_id;	
	}
	
	if (page_id == wd->cur_page_id) return;

	it = _page_find(wd_obj, wd->cur_page_id);
	if (!it) return;
	
	edje_object_signal_emit(it->base, "elm,state,indicator,off", "elm");
	
	it = _page_find(wd_obj, page_id);
	if (!it) return;
	
	edje_object_signal_emit(it->base, "elm,state,indicator,on", "elm");
	wd->cur_page_id = page_id;
	evas_object_smart_callback_call(it->obj, "changed", NULL);
}

static Page_Item *
_create_item(Evas_Object *obj, unsigned int page_id)
{
	Page_Item *it;
	Evas_Coord mw, mh;
	it = calloc(1, sizeof(Page_Item));
	if (!it)
		return NULL;
	
	it->obj = obj;
	it->page_id = page_id;

	it->base = edje_object_add(evas_object_evas_get(obj));

	char pi_name[128];
	sprintf(pi_name, "default_%d", page_id+1);
	_elm_theme_object_set(obj, it->base, "page", "item", pi_name);
	edje_object_size_min_restricted_calc(it->base, &mw, &mh, 0, 0);
	evas_object_size_hint_weight_set(it->base, 1.0, 1.0);
	evas_object_size_hint_align_set(it->base, -1.0, -1.0);

	evas_object_resize(it->base, mw, mh);
	evas_object_size_hint_min_set(it->base, mw, mh);
	evas_object_size_hint_max_set(it->base, mw, mh);

	edje_object_signal_callback_add(it->base, "clicked", "indicator_clicked", _indicator_clicked_cb, obj);

	return it;
}

static void 
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
	Widget_Data *wd = data;

	if (!wd)
		return;

	_els_box_layout(o, priv, 1, 0); /* making box layout non homogenous */
	return;
}

/**
 * Add a new pagecontrol to the parent
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup PageControl
 */
EAPI Evas_Object *
elm_page_control_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "pagecontrol");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "pagecontrol", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->scale_factor = elm_scale_get();
	if ( wd->scale_factor == 0.0 ) 
	{
		wd->scale_factor = 1.0;
	}

	wd->hbox = evas_object_box_add(e);
	evas_object_size_hint_weight_set(wd->hbox, 0, 0);

	evas_object_box_layout_set(wd->hbox, _layout, wd, NULL);
	elm_widget_sub_object_add(obj, wd->hbox);

	edje_object_part_swallow(wd->base, "elm.swallow.page", wd->hbox);

	evas_object_show(wd->hbox);

	wd->parent = parent;
	wd->page_count = 0;
	wd->cur_page_id = 0;

	_sizing_eval(obj);

	return obj;
}

/**
 * The number of pages for the pagecontrol to show as dots.
 * @param obj The pagecontrol object
 * @param page_count  Number of pages
 *
 * @ingroup PageControl
 */
EAPI void 
elm_page_control_page_count_set(Evas_Object *obj, unsigned int page_count)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return;
	if (!page_count)
	return;

	Page_Item *it;
	Evas_Coord mw, mh;

	int i;
	for (i = 0; i < page_count; i++)
	{
		it = _create_item(obj, i);
		wd->page_list = eina_list_append(wd->page_list, it);
		if (i == 0)
		{
			edje_object_signal_emit(it->base, "elm,state,indicator,on", "elm");
			evas_object_geometry_get(it->base, NULL, NULL, &mw, &mh);
		}

		evas_object_show(it->base);

		evas_object_box_append(wd->hbox, it->base);
		evas_object_smart_calculate(wd->hbox);
	}
   
	int width = mw*page_count;
	evas_object_resize(wd->hbox, width, mh);
	evas_object_size_hint_min_set(wd->hbox, width, mh);
	evas_object_size_hint_max_set(wd->hbox, width, mh);
	evas_object_smart_calculate(wd->hbox);
	wd->page_count = page_count;
}

/**
 * Set current/displayed page to given page number or id.
 * @param obj The pagecontrol object
 * @param page_id  Page number or Page Id
 *
 * @ingroup PageControl
 */
EAPI void 
elm_page_control_page_id_set(Evas_Object *obj, unsigned int page_id)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if(page_id >= wd->page_count || page_id == wd->cur_page_id) return;

	Page_Item *it;
	it = _page_find(obj, wd->cur_page_id);
	if (!it) return;

	edje_object_signal_emit(it->base, "elm,state,indicator,off", "elm");
	it = _page_find(obj, page_id);
	if (!it) return;

	edje_object_signal_emit(it->base, "elm,state,indicator,on", "elm");
	wd->cur_page_id=page_id;
}

/**
 * Get current/displayed page number or id.
 * @param obj The pagecontrol object
 * @return The current/displayed page id/number.
 *
 * @ingroup PageControl
 */
EAPI unsigned int 
elm_page_control_page_id_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)	
		return -1;

	return wd->cur_page_id;
}

