//FIXME need to remove include and macro
#include "els_webview.h"

#ifdef BOUNCING_SUPPORT
Evas_Object * elm_smart_webview_container_add(Evas *evas);
void _elm_smart_webview_container_child_set(Evas_Object *obj, Evas_Object *child);
void _elm_smart_webview_container_bounce_add(Evas_Object *obj, int dx, int dy);
void _elm_smart_webview_container_mouse_up(Evas_Object *obj);
void _elm_smart_webview_container_decelerated_flick_get(Evas_Object *obj, int *dx, int *dy);
Eina_Bool _elm_smart_webview_container_scroll_adjust(Evas_Object *obj, int *dx, int *dy);
#endif
