/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup Toolbar Toolbar
 *
 * @addtogroup Toolbar Toolbar
 * For listing a selection of items in a list within a "bar", each item having an icon and label.
 * This is more or less intended for use when selecting different modes - much like a tab widget,
 * but this is just the bar piece.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *scr, *bx;
   Eina_List *items;
   int icon_size;
   Eina_Bool scrollable : 1;
   Eina_Bool homogeneous : 1;
   double align;
};

struct _Elm_Toolbar2_Item
{
   Evas_Object *obj;
   Evas_Object *base;
   Evas_Object *icon;
   void (*func) (void *data, Evas_Object *obj, void *event_info);
   void (*del_cb) (void *data, Evas_Object *obj, void *event_info);
   const void *data;
};

static void _item_show(Elm_Toolbar2_Item *it);
static void _del_pre_hook(Evas_Object *obj);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void press_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void press_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data);

static void _item_show(Elm_Toolbar2_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(it->obj);
   Evas_Coord x, y, w, h, bx, by, bw, bh;

   if (!wd) return;
   evas_object_geometry_get(wd->bx, &bx, &by, &bw, &bh);
   evas_object_geometry_get(it->base, &x, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scr, x - bx, y - by, w, h);


}

static void _del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Toolbar2_Item *it;

   if (!wd) return;
   EINA_LIST_FREE(wd->items, it)
     {
	if (it->del_cb) it->del_cb((void *)it->data, it->obj, it);
	if (it->icon) evas_object_del(it->icon);
	evas_object_del(it->base);
	free(it);
     }
}

static void _del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   free(wd);
}

static void _theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const Eina_List *l;
   Elm_Toolbar2_Item *it;
   const char *style = elm_widget_style_get(obj);
   int ms = 0;
   int scale = 0;

   if (!wd) return;
   scale = (elm_widget_scale_get(obj) * _elm_config->scale);
   edje_object_scale_set(wd->scr, scale);
   EINA_LIST_FOREACH(wd->items, l, it)
   {
	   Evas_Coord mw, mh;
	   Evas_Coord ic_w, ic_h;

	   edje_object_scale_set(it->base, scale);

		
	   _elm_theme_object_set(obj, it->base, "toolbar2", "item", style);
	   if (it->icon)
	   {

		   evas_object_size_hint_min_get(it->icon, &ic_w, &ic_h);

		   ms = ((double)wd->icon_size * _elm_config->scale);
		   if (ic_w > 0 && ic_h > 0)
		   {
			   if (ic_h > wd->icon_size) ic_h = wd->icon_size;
			   evas_object_size_hint_min_set(it->icon, ic_w, ic_h);
			   evas_object_size_hint_max_set(it->icon, ic_w, ic_h);
		   }
		   else {
			   evas_object_size_hint_min_set(it->icon, ms, ms);
			   evas_object_size_hint_max_set(it->icon, ms, ms);
		   }

		   evas_object_size_hint_align_set(it->icon, 0.5, 0.5);
		   edje_object_part_swallow(it->base, "elm.swallow.icon", it->icon);
		   evas_object_show(it->icon);
		   elm_widget_sub_object_add(obj, it->icon);
	   }
/*
	   mw = mh = -1;
	   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
	   edje_object_size_min_restricted_calc(it->base, &mw, &mh, mw, mh);
	   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
*/
	   evas_object_size_hint_weight_set(it->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	   evas_object_size_hint_align_set(it->base, 0.5, 0.5);

	   if (ic_w > 0 && ic_h > 0) {
		   evas_object_size_hint_min_set(it->base, ic_w, ic_h);
	   }
	   else {
		   //		   evas_object_size_hint_min_set(it->base, mw, mh);
		   ms = ((double)wd->icon_size * _elm_config->scale);

		   evas_object_size_hint_min_set(it->base, ms, ms);
	   }
   }
   _sizing_eval(obj);
}

static void _sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	Evas_Coord vw = 0, vh = 0;
	Evas_Coord w, h;

	if (!wd) return;

	evas_object_smart_calculate(wd->bx);
	edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr), &minw, &minh);
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	if (w < minw) w = minw;
	if (h < minh) h = minh;

	evas_object_resize(wd->scr, w, h);

	evas_object_size_hint_min_get(wd->bx, &minw, &minh);
	if (w > minw) minw = w;

	evas_object_resize(wd->bx, minw, 66);
	if (w > minw) minw = w;
	elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);

	if (wd->scrollable)
		minw = w - vw;
	else
		minw = minw + (w - vw);

	minh = minh + (h - vh);

	evas_object_size_hint_min_set(obj, vw, vh);
	evas_object_size_hint_max_set(obj, -1, -1);
}

static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Coord mw, mh, vw, vh, x, y, w, h;
	const Eina_List *l;
	Elm_Toolbar2_Item *it;

	if (!wd) return;

	elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
	evas_object_size_hint_min_get(wd->bx, &mw, &mh);
	evas_object_geometry_get(wd->bx, NULL, NULL, &w, &h);

	if (vw >= mw)
	{
		if (w != vw) evas_object_resize(wd->bx, vw, h);
	}
}

static void press_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = event_info;
//	printf("\nDown %d\n", ev->flags);
	//Evas_Event_Mouse_Down *ev = event_info;
	Elm_Toolbar2_Item *it = (Elm_Toolbar2_Item *)data;
	edje_object_signal_emit(it->base, "elm,state,selected", "elm");
	_item_show(it);
}

static void press_up_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{

	Elm_Toolbar2_Item *it = (Elm_Toolbar2_Item *)data;

	edje_object_signal_emit(it->base, "elm,state,unselected", "elm");

	Evas_Event_Mouse_Up *ev = event_info;
//	printf("\nUp %d\n", ev->flags);
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) {
//		printf("\nReturn\n");
		return;
	}

//	printf("\nSend\n");
	if (it->func) it->func((void *)(it->data), it->obj, it);
	evas_object_smart_callback_call(it->obj, "clicked", it);
}



static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
   Widget_Data *wd = data;

   _els_box_layout(o, priv, 1, wd->homogeneous);
}

/**
 * Add a new Toolbar object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *elm_toolbar2_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "toolbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_bounce_allow_set(wd->scr, 0, 0);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "toolbar2", "base", "default");
   elm_widget_resize_object_set(obj, wd->scr);
   elm_smart_scroller_policy_set(wd->scr,
				 ELM_SMART_SCROLLER_POLICY_AUTO,
				 ELM_SMART_SCROLLER_POLICY_OFF);

   wd->icon_size = 50;
   wd->scrollable = EINA_TRUE;
   wd->homogeneous = EINA_TRUE;
   wd->align = 0.5;

   wd->bx = evas_object_box_add(e);
   evas_object_size_hint_align_set(wd->bx, wd->align, 0.5);
   evas_object_box_layout_set(wd->bx, _layout, wd, NULL);
   elm_widget_sub_object_add(obj, wd->bx);
   elm_smart_scroller_child_set(wd->scr, wd->bx);
   evas_object_show(wd->bx);

   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_RESIZE, _resize, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the size of icon of Toolbar object
 *
 * @param obj The Toolbar object
 * @param icon_size The size of icon to be set
 *
 * @ingroup Toolbar
 */
EAPI void elm_toolbar2_icon_size_set(Evas_Object *obj, int icon_size)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (icon_size > 50) return;
   if (wd->icon_size == icon_size) return;
   wd->icon_size = icon_size;
   _theme_hook(obj);
}

/**
 * Get the size of icon of Toolbar object
 *
 * @param obj The Toolbar object
 * @return The size of icon to be set
 *
 * @ingroup Toolbar
 */
EAPI int elm_toolbar2_icon_size_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0;
   return wd->icon_size;
}

/**
 * Add items to Toolbar object
 *
 * @param obj The Toolbar object
 * @param icon The icon
 * @param label The Label
 * @param func The function
 * @param data The data
 * @return TheToolbar item
 *
 * @ingroup Toolbar
 */
EAPI Elm_Toolbar2_Item *elm_toolbar2_item_add(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), const void *data)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int ms = 0;
	Evas_Coord mw, mh;
	Evas_Coord ic_w, ic_h;
	Elm_Toolbar2_Item *it;

	if (!wd) return NULL;
	it = ELM_NEW(Elm_Toolbar2_Item);
	if (!it) return NULL;
	wd->items = eina_list_append(wd->items, it);

	it->obj = obj;
	it->icon = icon;
	it->func = func;
	it->data = data;
	it->base = edje_object_add(evas_object_evas_get(obj));

	/* Temp */
	Elm_Theme *th = NULL;
	th = elm_theme_new();

	_elm_theme_object_set(obj, it->base, "toolbar2", "item", elm_widget_style_get(obj));

	evas_object_event_callback_add(it->icon, EVAS_CALLBACK_MOUSE_DOWN,  press_down_cb, it);
	evas_object_event_callback_add(it->icon, EVAS_CALLBACK_MOUSE_UP,  press_up_cb, it);

	elm_widget_sub_object_add(obj, it->base);

	ic_w = 0;
	ic_h = 0;
	if (it->icon)
	{
	
		evas_object_size_hint_min_get(it->icon, &ic_w, &ic_h);

		if (ic_w > 0 && ic_h > 0)
		{
			if (ic_h > wd->icon_size) ic_h = wd->icon_size;
			evas_object_size_hint_min_set(it->icon, ic_w, ic_h);
			evas_object_size_hint_max_set(it->icon, ic_w, ic_h);
		}
		else {
			ms = ((double)wd->icon_size * _elm_config->scale);

			evas_object_size_hint_min_set(it->icon, ms, ms);
			evas_object_size_hint_max_set(it->icon, ms, ms);
		}

//		evas_object_size_hint_weight_set(it->icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
//		evas_object_size_hint_align_set(it->icon, 0.5, 0.5);

		edje_object_part_swallow(it->base, "elm.swallow.icon", it->icon);
		evas_object_show(it->icon);
		elm_widget_sub_object_add(obj, it->icon);
	}
/*
	mw = mh = -1;

	elm_coords_finger_size_adjust(1, &mw, 1, &mh);
	edje_object_size_min_restricted_calc(it->base, &mw, &mh, mw, mh);
	elm_coords_finger_size_adjust(1, &mw, 1, &mh);
*/
	evas_object_size_hint_weight_set(it->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(it->base, 0.5, 0.5);

	if (ic_w > 0 && ic_h > 0) {
		evas_object_size_hint_min_set(it->base, ic_w, ic_h);
	}
	else {
//		evas_object_size_hint_min_set(it->base, mw, mh);
		ms = ((double)wd->icon_size * _elm_config->scale);

		evas_object_size_hint_min_set(it->base, ms, ms);
	}
	evas_object_box_append(wd->bx, it->base);
	evas_object_show(it->base);
	_sizing_eval(obj);
	return it;
}

/**
 * Get icon of item of Toolbar object
 *
 * @param item TheToolbar item
 * @return The icon object
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object *elm_toolbar2_item_icon_get(Elm_Toolbar2_Item *item)
{
   if (!item) return NULL;
   return item->icon;
}


/**
 * Delete the Toolbar item
 *
 * @param it TheToolbar item
 *
 * @ingroup Toolbar
 */
EAPI void elm_toolbar2_item_del(Elm_Toolbar2_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(it->obj);
   Evas_Object *obj2 = it->obj;

   if ((!wd) || (!it)) return;
   if (it->del_cb) it->del_cb((void *)it->data, it->obj, it);
   wd->items = eina_list_remove(wd->items, it);
   if (it->icon) evas_object_del(it->icon);
   evas_object_del(it->base);
   free(it);
   _theme_hook(obj2);
}

/**
 * Set the function called when a toolbar item is freed.
 *
 * @param it The item to set the callback on
 * @param func The function called
 *
 * @ingroup Toolbar
 */
EAPI void elm_toolbar2_item_del_cb_set(Elm_Toolbar2_Item *it, void (*func)(void *data, Evas_Object *obj, void *event_info))
{
   if(!it) return;

   it->del_cb = func;
}

/**
 * Whether toolbar is scrollable
 *
 * @param obj The Toolbar
 * @param scrollable The scrollable flag (1 if toolbar is scrollable, 0 otherwise)
 *
 * @ingroup Toolbar
 */
EAPI void elm_toolbar2_scrollable_set(Evas_Object *obj, Eina_Bool scrollable)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->scrollable = scrollable;
   _sizing_eval(obj);
}


/**
 * Whether toolbar is homogenous
 *
 * @param obj The Toolbar
 * @param homogenous The homogenous flag (1 if toolbar is homogenous, 0 otherwise)
 *
 * @ingroup Toolbar
 *
 */
EAPI void
elm_toolbar2_homogenous_set(Evas_Object *obj, Eina_Bool homogenous)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->homogeneous = !!homogenous;
   evas_object_smart_calculate(wd->bx);
}


/**
 * Set if the alignment of the items.
 *
 * @param obj The toolbar object
 * @param align The new alignment. (left) 0.0 ... 1.0 (right)
 *
 * @ingroup Toolbar
 */
EAPI void elm_toolbar2_align_set(Evas_Object *obj, double align)
{
   Eina_List *l;
   Elm_Toolbar2_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if(wd->align != align)
     evas_object_size_hint_align_set(wd->bx, align, 0.5);
    wd->align = align;
}
