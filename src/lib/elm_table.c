#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Table Table
 *
 * Arranges widgets in a table where items can also span multiple
 * columns or rows - even overlap (and then be raised or lowered
 * accordingly to adjust stacking if they do overlap).
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *tbl;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _theme_hook(Evas_Object *obj);

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_event_callback_del_full
     (wd->tbl, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
   evas_object_del(wd->tbl);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static Eina_Bool
_elm_table_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const Eina_List *items;
   void *(*list_data_get) (const Eina_List *list);
   Eina_List *(*list_free) (Eina_List *list);

   if ((!wd) || (!wd->tbl))
     return EINA_FALSE;

   /* Focus chain */
   /* TODO: Change this to use other chain */
   if ((items = elm_widget_focus_custom_chain_get(obj)))
     {
        list_data_get = eina_list_data_get;
        list_free = NULL;
     }
   else
     {
        items = evas_object_table_children_get(wd->tbl);
        list_data_get = eina_list_data_get;
        list_free = eina_list_free;

        if (!items) return EINA_FALSE;
     }

   Eina_Bool ret = elm_widget_focus_list_next_get(obj, items, list_data_get,
                                                   dir, next);

   if (list_free)
     list_free((Eina_List *)items);

   return ret;
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->tbl))
     return;

   evas_object_table_mirrored_set(wd->tbl, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord w, h;
   if (!wd) return;
   evas_object_size_hint_min_get(wd->tbl, &minw, &minh);
   evas_object_size_hint_max_get(wd->tbl, &maxw, &maxh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   if ((maxw >= 0) && (w > maxw)) w = maxw;
   if ((maxh >= 0) && (h > maxh)) h = maxh;
   evas_object_resize(obj, w, h);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   _sizing_eval(obj);
}

/**
 * Add a new table to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Table
 */
EAPI Evas_Object *
elm_table_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "table");
   elm_widget_type_set(obj, "table");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_focus_next_hook_set(obj, _elm_table_focus_next_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_highlight_ignore_set(obj, EINA_FALSE);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->tbl = evas_object_table_add(e);
   evas_object_event_callback_add(wd->tbl, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);
   elm_widget_resize_object_set(obj, wd->tbl);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   return obj;
}

/**
 * Set the homogeneous layout in the table
 *
 * @param obj The layout object
 * @param homogeneous A boolean to set (or no) layout homogeneous
 * in the table
 * (1 = homogeneous,  0 = no homogeneous)
 *
 * @ingroup Table
 */
EAPI void
elm_table_homogeneous_set(Evas_Object *obj, Eina_Bool homogeneous)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_table_homogeneous_set(wd->tbl, homogeneous);
}

EINA_DEPRECATED EAPI void
elm_table_homogenous_set(Evas_Object *obj, Eina_Bool homogenous)
{
   elm_table_homogeneous_set(obj, homogenous);
}

/**
 * Get the current table homogeneous mode.
 *
 * @param obj The table object
 * @return a boolean to set (or no) layout homogeneous in the table
 * (1 = homogeneous,  0 = no homogeneous)
 *
 * @ingroup Table
 */
EAPI Eina_Bool
elm_table_homogeneous_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return evas_object_table_homogeneous_get(wd->tbl);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_table_homogenous_get(const Evas_Object *obj)
{
   return elm_table_homogeneous_get(obj);
}

/**
 * Set padding between cells.
 *
 * @param obj The layout object.
 * @param horizontal set the horizontal padding.
 * @param vertical set the vertical padding.
 *
 * @ingroup Table
 */
EAPI void
elm_table_padding_set(Evas_Object *obj, Evas_Coord horizontal, Evas_Coord vertical)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_table_padding_set(wd->tbl, horizontal, vertical);
}

/**
 * Get padding between cells.
 *
 * @param obj The layout object.
 * @param horizontal set the horizontal padding.
 * @param vertical set the vertical padding.
 *
 * @ingroup Table
 */
EAPI void
elm_table_padding_get(const Evas_Object *obj, Evas_Coord *horizontal, Evas_Coord *vertical)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_table_padding_get(wd->tbl, horizontal, vertical);
}

/**
 * Add a subobject on the table with the coordinates passed
 *
 * @param obj The table object
 * @param subobj The subobject to be added to the table
 * @param x Coordinate to X axis
 * @param y Coordinate to Y axis
 * @param w Horizontal length
 * @param h Vertical length
 *
 * @ingroup Table
 */
EAPI void
elm_table_pack(Evas_Object *obj, Evas_Object *subobj, int x, int y, int w, int h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_widget_sub_object_add(obj, subobj);
   evas_object_table_pack(wd->tbl, subobj, x, y, w, h);
}

/**
 * Remove child from table.
 *
 * @param obj The table object
 * @param subobj The subobject
 *
 * @ingroup Table
 */
EAPI void
elm_table_unpack(Evas_Object *obj, Evas_Object *subobj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_widget_sub_object_del(obj, subobj);
   evas_object_table_unpack(wd->tbl, subobj);
}

/**
 * Faster way to remove all child objects from a table object.
 *
 * @param obj The table object
 * @param clear If true, it will delete just removed children
 *
 * @ingroup Table
 */
EAPI void
elm_table_clear(Evas_Object *obj, Eina_Bool clear)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_table_clear(wd->tbl, clear);
}
