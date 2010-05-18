#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Mapbuf Mapbuf
 *
 * This holds 1 content object and uses an Evas Map to move/resize etc. it.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *content;
   Eina_Bool enabled : 1;
   Eina_Bool alpha : 1;
   Eina_Bool smooth : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord maxw = -1, maxh = -1;
   if (!wd) return;
   if (wd->content)
     {
        evas_object_size_hint_min_get(wd->content, &minw, &minh);
        evas_object_size_hint_max_get(wd->content, &maxw, &maxh);
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   if (sub == wd->content)
     {
	evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
	wd->content = NULL;
	_sizing_eval(obj);
     }
}

static void
_mapbuf(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   if (wd->enabled)
     {
        Evas_Map *m;

        m = evas_map_new(4);
        evas_map_util_points_populate_from_geometry(m, x, y, w, h, 0);
        evas_map_smooth_set(m, wd->smooth);
        evas_map_alpha_set(m, wd->alpha);
        evas_object_map_set(wd->content, m);
        evas_object_map_enable_set(wd->content, wd->enabled);
        evas_map_free(m);
     }
   else
     {
        evas_object_map_set(wd->content, NULL);
        evas_object_map_enable_set(wd->content, 0);
        evas_object_move(wd->content, x, y);
        evas_object_resize(wd->content, w, h);
     }
}

static void
_configure(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content)
     {
        Evas_Coord x, y, w, h;
        
        evas_object_geometry_get(obj, &x, &y, &w, &h);
        if (!wd->enabled) evas_object_move(wd->content, x, y);
        evas_object_resize(wd->content, w, h);
        _mapbuf(obj);
     }
}

static void
_move(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   _configure(obj);
}

static void
_resize(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{    
   _configure(obj);
}

/**
 * Add a new mapbuf to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Mapbuf
 */
EAPI Evas_Object *
elm_mapbuf_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "mapbuf");
   elm_widget_type_set(obj, "mapbuf");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, NULL);

   wd->enabled = 0;
   wd->alpha = 1;
   wd->smooth = 1;
   
   _sizing_eval(obj);
   return obj;
}

/**
 * Set the mapbuf front content
 *
 * @param obj The mapbuf object
 * @param content The content will be filled in this mapbuf object
 *
 * @ingroup Mapbuf
 */
EAPI void
elm_mapbuf_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content == content) return;
   if ((wd->content != content) && (wd->content))
     {
        elm_widget_sub_object_del(obj, wd->content);
        evas_object_smart_member_del(wd->content);
     }
   wd->content = content;
   if (content)
     {
	elm_widget_sub_object_add(content, obj);
        evas_object_smart_member_add(content, obj);
	evas_object_event_callback_add(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	_sizing_eval(obj);
     }
   _configure(obj);
}

/**
 * Set the mapbuf enabled state
 *
 * @param obj The mapbuf object
 * @param enabled The value to set the enabled state to
 *
 * @ingroup Mapbuf
 */
EAPI void
elm_mapbuf_enabled_set(Evas_Object *obj, Eina_Bool enabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->enabled == enabled) return;
   wd->enabled = enabled;
   _configure(obj);
}

/**
 * Get the mapbuf enabled state
 *
 * @param obj The mapbuf object
 * @return The value that the enabled state is set to
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool
elm_mapbuf_enabled_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->enabled;
}

/**
 * Sets the mapbuf smooth state
 *
 * @param obj The mapbuf object
 * @param smooth The value of the smooth state of @p obj
 *
 * @ingroup Mapbuf
 */
EAPI void
elm_mapbuf_smooth_set(Evas_Object *obj, Eina_Bool smooth)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->smooth == smooth) return;
   wd->smooth = smooth;
   _configure(obj);
}

/**
 * Gets the mapbuf smooth state
 *
 * @param obj The mapbuf object
 * @return The value of the smooth state of @p obj
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool
elm_mapbuf_smooth_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->smooth;
}

/**
 * Enables/disables the mapbuf alpha channel
 *
 * @param obj The mapbuf object
 * @param alpha The state of the alpha channel
 *
 * @ingroup Mapbuf
 */
EAPI void
elm_mapbuf_alpha_set(Evas_Object *obj, Eina_Bool alpha)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->alpha == alpha) return;
   wd->alpha = alpha;
   _configure(obj);
}

/**
 * Gets the state of the mapbuf alpha channel
 *
 * @param obj The mapbuf object
 * @return The state of the alpha channel
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool
elm_mapbuf_alpha_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->alpha;
}
