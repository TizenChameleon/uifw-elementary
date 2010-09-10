//FIXME: need to remove
//#define BOUNCING_SUPPORT

Evas_Object* _elm_smart_webview_add(Evas *evas, Eina_Bool tiled);
void         _elm_smart_webview_events_feed_set(Evas_Object* obj, Eina_Bool feed);
Eina_Bool    _elm_smart_webview_events_feed_get(Evas_Object* obj);
void         _elm_smart_webview_bounce_allow_set(Evas_Object* obj, Eina_Bool horiz, Eina_Bool vert);
void         _elm_smart_webview_scheme_callback_set(Evas_Object* obj, const char *scheme, Elm_WebView_Mime_Cb func);
void         _elm_smart_webview_default_layout_width_set(Evas_Object *obj, int width);
#ifdef BOUNCING_SUPPORT
void         _elm_smart_webview_container_set(Evas_Object *obj, Evas_Object *container);
#endif
