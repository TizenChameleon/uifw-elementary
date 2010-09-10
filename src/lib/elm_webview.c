/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
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
#ifdef BOUNCING_SUPPORT
   Evas_Object *container;
#endif
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
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord w, h;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   printf("sizing eval : %d, %d\n", w, h);
#ifdef BOUNCING_SUPPORT
   evas_object_resize(wd->container, w, h);
#endif
   evas_object_resize(wd->webkit, w, h);
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
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
#ifdef ELM_EWEBKIT
EAPI Evas_Object *
elm_webview_add(Evas_Object *parent, Eina_Bool tiled)
{
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

   wd->webkit = _elm_smart_webview_add(e, tiled);
#ifdef BOUNCING_SUPPORT
   wd->container = elm_smart_webview_container_add(e);
   _elm_smart_webview_container_child_set(wd->container, wd->webkit);
#endif
   _elm_smart_webview_widget_set(wd->webkit, obj);
   evas_object_event_callback_add(wd->webkit, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				  _changed_size_hints, obj);

#ifdef BOUNCING_SUPPORT
   elm_widget_resize_object_set(obj, wd->container);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);
#else
   elm_widget_resize_object_set(obj, wd->webkit);
#endif
   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   return obj;
}

EAPI Evas_Object *
elm_webview_webkit_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   return wd->webkit;
}

EAPI void
elm_webview_events_feed_set(Evas_Object *obj, Eina_Bool feed)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_events_feed_set(wd->webkit, feed);
}

EAPI Eina_Bool
elm_webview_events_feed_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return _elm_smart_webview_events_feed_get(wd->webkit);
}

EAPI void
elm_webview_auto_fitting_set(Evas_Object *obj, Eina_Bool enable)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_auto_fitting_set(wd->webkit, enable);
}

EAPI Eina_Bool
elm_webview_auto_fitting_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return _elm_smart_webview_auto_fitting_get(wd->webkit);
}

EAPI Evas_Object *
elm_webview_minimap_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return _elm_smart_webview_minimap_get(wd->webkit);
}

EAPI void
elm_webview_uri_set(Evas_Object *obj, const char *uri)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_uri_set(wd->webkit, uri);
}

/**
 * Set bouncing behavior(Not supported yet)
 *
 * When scrolling, the WebView may "bounce" when reaching an edge of contents
 * This is a visual way to indicate the end has been reached. This is enabled
 * by default for both axes. This will set if it is enabled for that axis with
 * the boolean parameers for each axis.
 *
 * @param obj The WebView object
 * @param h_bounce Will the WebView bounce horizontally or not
 * @param v_bounce Will the WebView bounce vertically or not
 *
 * @ingroup WebView
 */
EAPI void
elm_webview_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_bounce_allow_set(wd->webkit, h_bounce, v_bounce);
}

/**
 * Add callback to treat scheme
 *
 * When user click link, the WebView may have different action by scheme.
 * This is a way to choose proper action each scheme.
 *
 * @param obj The WebView object
 * @param scheme The scheme which user want to receive
 * @param scheme_callback callback when user choose link which involved @scheme
 *
 * @ingroup WebView
 */
EAPI void
elm_webview_scheme_callback_set(Evas_Object *obj, const char *scheme, Elm_WebView_Mime_Cb func)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_scheme_callback_set(wd->webkit, scheme, func);
}

/**
 * Set default layout width
 *
 * If you want to load webpage with specific layout width, you can set it using this API.
 * If you do not set it, the default layout width will be 1024.
 *
 * @param obj Webview object
 * @param width width size that you want to set
 *
 * @ingroup WebView
 *
 */
EAPI void
elm_webview_default_layout_width_set(Evas_Object *obj, int width)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_smart_webview_default_layout_width_set(wd->webkit, width);
}

#endif
