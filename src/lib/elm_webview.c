#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup WebView WebView
 * @ingroup Elementary
 *
 * TODO
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *webkit;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   /*evas_object_event_callback_del_full
        (wd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
   evas_object_box_remove_all(wd->box, 0);
   */
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   //_els_webview_del(wd->webkit);
   free(wd);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord w, h;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   printf("sizing eval : %d, %d\n", w, h);
   //evas_object_resize(obj, w, h);
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

static void
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
{
   Widget_Data *wd = data;
   if (!wd) return;
   //_els_box_layout(o, priv, wd->horizontal, wd->homogeneous);
}
/**
 * Add a new box to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Box
 */
EAPI Evas_Object *
elm_webview_add(Evas_Object *parent)
{
#ifdef ELM_EWEBKIT
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "webview");
   elm_widget_type_set(obj, "webview");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);

   wd->webkit = _els_webview_add(obj, EINA_TRUE);
   //TODO:evas_object_box_layout_set(wd->box, _layout, wd, NULL);
   evas_object_event_callback_add(wd->webkit, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				  _changed_size_hints, obj);
   elm_widget_resize_object_set(obj, wd->webkit);
   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   printf("elm_webview_add is called\n");
   return obj;
#else
   return NULL;
#endif
}

EAPI Evas_Object *
elm_webview_widget_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   return wd->webkit;
}

EAPI void
elm_webview_events_feed_set(Evas_Object *obj, Eina_Bool feed)
{
}

EAPI void
elm_webview_events_feed_get(Evas_Object *obj)
{
}

EAPI void
elm_webview_auto_fitting_set(Eina_Bool enable)
{
}

EAPI Eina_Bool
elm_webview_auto_fitting_get()
{
   return EINA_FALSE;
}

EAPI Evas_Object *
elm_webview_minimap_get(Evas_Object *obj)
{
   return NULL;
}

EAPI void
elm_webview_uri_set(Evas_Object *obj, const char *uri)
{
}
