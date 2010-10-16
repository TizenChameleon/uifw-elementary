#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Gridbox Gridbox
 * @ingroup Elementary
 *
 * This is a gridbox widget to show multiple objects in a grid
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *tbl, *scr;
   Evas_Coord itemsize_h, itemsize_v;
   Evas_Coord minw, minh;
   Evas_Coord horizontal, vertical;
   int x, y;
   Eina_Bool homogeneous:1;
};

static void _del_hook(Evas_Object * obj);

static void _sizing_eval(Evas_Object * obj, int mode);

static void _changed_size_hints(void *data, Evas * e, Evas_Object * obj,
				void *event_info);
static void _changed_size_min(void *data, Evas * e, Evas_Object * obj,
			      void *event_info);
static void _sub_del(void *data, Evas_Object * obj, void *event_info);

static void _show_event(void *data, Evas *e, Evas_Object * obj, void *event_info);

static void
_del_pre_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   evas_object_event_callback_del_full(wd->tbl,
				       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
   evas_object_event_callback_del_full(wd->tbl, EVAS_CALLBACK_RESIZE,
				       _changed_size_min, obj);
   evas_object_del(wd->tbl);
   wd->tbl = NULL;
   evas_object_del(wd->scr);
   wd->scr = NULL;
}

static void
_del_hook(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   free(wd);
}

static Eina_Bool
_arrange_table(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   Eina_List *l, *l_temp = NULL;

   Evas_Object *item;

   int i = 0;

   if (wd)
     {
	if (wd->tbl)
	  {
	     int index =
		eina_list_count(evas_object_table_children_get(wd->tbl));
	     if (!index)
		return EINA_FALSE;

	     elm_gridbox_item_size_set(obj, wd->itemsize_h, wd->itemsize_v);
	     l = evas_object_table_children_get(wd->tbl);

	     EINA_LIST_FOREACH(l, l_temp, item)
	     {
		evas_object_table_unpack(wd->tbl, item);
		elm_widget_sub_object_del(wd->tbl, item);
		evas_object_table_pack(wd->tbl, item, i % wd->x, i / wd->x, 1,
				       1);
		i++;
	     }
	  }
     }

   return EINA_TRUE;
}

static void
_sizing_eval(Evas_Object * obj, int mode)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   Evas_Coord w, h;

   evas_object_size_hint_min_get(wd->scr, &minw, &minh);
   evas_object_size_hint_max_get(wd->scr, &maxw, &maxh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_geometry_get(wd->scr, NULL, NULL, &w, &h);

   if (w < minw)
      w = minw;
   if (h < minh)
      h = minh;
   if ((maxw >= 0) && (w > maxw))
      w = maxw;
   if ((maxh >= 0) && (h > maxh))
      h = maxh;
   evas_object_resize(obj, w, h);

   wd->minw = w;
   wd->minh = h;
   if (w < wd->itemsize_h || h < wd->itemsize_v)
      return;
   if (wd->homogeneous)
     {
	wd->x = w / wd->itemsize_h;
	wd->y = h / wd->itemsize_v;
	wd->horizontal = (wd->minw - wd->x * wd->itemsize_h) / wd->x;
	wd->vertical = (wd->minh - wd->y * wd->itemsize_v) / wd->y;
	elm_gridbox_padding_set(obj, wd->horizontal, wd->vertical);
     }

   if (!mode)
      _arrange_table(obj);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object * obj, void *event_info)
{
   _sizing_eval(data, 0);
}

static void
_changed_size_min(void *data, Evas *e, Evas_Object * obj, void *event_info)
{
   _sizing_eval(data, 0);
}

static void
_sub_del(void *data, Evas_Object * obj, void *event_info)
{
   _sizing_eval(obj, 1);
}

static void
_show_event(void *data, Evas *e, Evas_Object * obj, void *event_info)
{
   _sizing_eval(data, 0);
}

static void
_freeze_on(void *data, Evas_Object * obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
      return;
   evas_object_smart_callback_call(wd->scr, "scroll-freeze-on", NULL);
}

static void
_freeze_off(void *data, Evas_Object * obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
      return;
   evas_object_smart_callback_call(wd->scr, "scroll-freeze-off", NULL);
}

/**
 * Add a new gridbox to the parent
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Gridbox
 */
EAPI Evas_Object *
elm_gridbox_add(Evas_Object * parent)
{
   Evas_Object *obj;

   Evas *e;

   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "gridbox");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);

   wd->scr = elm_scroller_add(parent);
   elm_widget_resize_object_set(obj, wd->scr);
   elm_scroller_bounce_set(wd->scr, 0, 1);

   wd->tbl = evas_object_table_add(e);
   evas_object_size_hint_weight_set(wd->tbl, 0.0, 0.0);
   elm_scroller_content_set(wd->scr, wd->tbl);
   evas_object_show(wd->tbl);

   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				  _changed_size_hints, obj);
   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_RESIZE,
				  _changed_size_min, obj);
   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_SHOW, _show_event,
				  obj);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);

   _sizing_eval(obj, 0);

   return obj;
}

/**
 * Set padding between cells.
 *
 * @param[in] obj The layout object.
 * @param[in] horizontal set the horizontal padding.
 * @param[in] vertical set the vertical padding.
 *
 * @ingroup Gridbox
 */
EAPI void
elm_gridbox_padding_set(Evas_Object * obj, Evas_Coord horizontal,
			Evas_Coord vertical)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
      return;

   wd->horizontal = horizontal;
   wd->vertical = vertical;
   if (wd->tbl)
      evas_object_table_padding_set(wd->tbl, horizontal, vertical);
}

/**
 * Set gridbox item size
 *
 * @param[in] obj The gridbox object
 * @param[in] h_pagesize The horizontal item size
 * @param[in] v_pagesize The vertical item size
 *
 * @ingroup Gridbox
 */
EAPI void
elm_gridbox_item_size_set(Evas_Object * obj, Evas_Coord h_itemsize,
			  Evas_Coord v_itemsize)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   Evas_Coord minw = -1, minh = -1;

   Evas_Coord w, h;

   if (!wd)
      return;

   wd->itemsize_h = h_itemsize;
   wd->itemsize_v = v_itemsize;
   evas_object_size_hint_min_get(wd->scr, &minw, &minh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw)
      w = minw;
   if (h < minh)
      h = minh;
   wd->x = w / h_itemsize;
   wd->y = h / v_itemsize;

   if (!wd->x)
      wd->x = 1;
   if (!wd->y)
      wd->y = 1;
   _sizing_eval(obj, 1);
}

/**
 * Add a subobject on the gridbox
 *
 * @param[in] obj The table object
 * @param[in] subobj The subobject to be added to the gridbox
 *
 * @ingroup Gridbox
 */
EAPI void
elm_gridbox_pack(Evas_Object * obj, Evas_Object * subobj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   int index = eina_list_count(evas_object_table_children_get(wd->tbl));

   evas_object_size_hint_min_set(subobj, wd->itemsize_h, wd->itemsize_v);
   evas_object_size_hint_max_set(subobj, wd->itemsize_h, wd->itemsize_v);
   elm_widget_sub_object_add(obj, subobj);
   evas_object_table_pack(wd->tbl, subobj, index % wd->x, index / wd->x, 1, 1);
}

/**
 * Unpack a subobject on the gridbox
 *
 * @param[in] obj The gridbox object
 * @param[in] subobj The subobject to be removed to the gridbox
 *
 * @ingroup Gridbox
 */
EAPI Eina_Bool
elm_gridbox_unpack(Evas_Object * obj, Evas_Object * subobj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   Eina_Bool ret = 0;

   elm_widget_sub_object_del(wd->tbl, subobj);
   ret = evas_object_table_unpack(wd->tbl, subobj);

   _arrange_table(obj);

   return ret;
}

/**
 * Get the list of children for the gridbox.
 *
 * @param[in] obj The gridbox object
 *
 * @ingroup Gridbox
 */
EAPI Eina_List *
elm_gridbox_children_get(Evas_Object * obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   Eina_List *new_list = NULL;

   new_list = evas_object_table_children_get(wd->tbl);

   return new_list;
}

/**
 * Set homogenous paddding layout
 *
 * @param[in] obj The gridbox object
 * @param[in] homogenous The homogenous flag (1 = on, 0 = off)
 *
 * @ingroup Gridbox
 */
EAPI void
elm_gridbox_homogenous_padding_set(Evas_Object * obj, Eina_Bool homogenous)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   wd->homogeneous = homogenous;
}
