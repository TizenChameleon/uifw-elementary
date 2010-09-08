#include <Elementary.h>
#include "elm_priv.h"


/**
 * @defgroup Notify notify
 *
 * Display a window in a particular region of the application (top, bottom ...).
 * A timeout can be set to automatically close the window.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *notify, *content, *parent;

   Elm_Notify_Orient orient;
   Eina_Bool repeat_events;
   Evas_Object *block_events;

   int timeout;
   Ecore_Timer *timer;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _signal_block_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _calc(Evas_Object *obj);
static void _content_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_pre_hook(Evas_Object *obj)
{
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, _resize, obj);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE, _resize, obj);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_SHOW, _show, obj);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_HIDE, _hide, obj);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_notify_parent_set(obj, NULL);
   elm_notify_repeat_events_set(obj, EINA_TRUE);
   if (wd->timer)
     {
	ecore_timer_del(wd->timer);
	wd->timer = NULL;
     }
   free(wd);
}

static void
_notify_theme_apply(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *style = elm_widget_style_get(obj);

   switch (wd->orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
        _elm_theme_object_set(obj, wd->notify, "notify", "top", style);
        break;
      case ELM_NOTIFY_ORIENT_CENTER:
        _elm_theme_object_set(obj, wd->notify, "notify", "center", style);
        break;
      case ELM_NOTIFY_ORIENT_BOTTOM:
        _elm_theme_object_set(obj, wd->notify, "notify", "bottom", style);
        break;
      case ELM_NOTIFY_ORIENT_LEFT:
        _elm_theme_object_set(obj, wd->notify, "notify", "left", style);
        break;
      case ELM_NOTIFY_ORIENT_RIGHT:
        _elm_theme_object_set(obj, wd->notify, "notify", "right", style);
        break;
      case ELM_NOTIFY_ORIENT_TOP_LEFT:
        _elm_theme_object_set(obj, wd->notify, "notify", "top_left", style);
        break;
      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
        _elm_theme_object_set(obj, wd->notify, "notify", "top_right", style);
        break;
      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
        _elm_theme_object_set(obj, wd->notify, "notify", "bottom_left", style);
        break;
      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
        _elm_theme_object_set(obj, wd->notify, "notify", "bottom_right", style);
        break;
      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }
}

static void
_block_events_theme_apply(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *style = elm_widget_style_get(obj);
   _elm_theme_object_set(obj, wd->block_events, "notify", "block_events", style);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _notify_theme_apply(obj);
   if (wd->block_events) _block_events_theme_apply(obj);
   edje_object_scale_set(wd->notify, elm_widget_scale_get(obj) *
                         _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord x,y,w,h;
   if (!wd) return;
   if (!wd->parent) return;
   evas_object_geometry_get(wd->parent, &x, &y, &w, &h);
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _calc(data);
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
	evas_object_event_callback_del_full(sub, EVAS_CALLBACK_RESIZE,
					     _content_resize, obj);
	wd->content = NULL;
     }
}

static void
_signal_block_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, "block,clicked", NULL);
}

static void
_resize(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   _calc(obj);
}

static void
_content_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _calc(data);
}

static void
_calc(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord x, y, w, h;

   if (!wd) return;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   edje_object_size_min_get(wd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(wd->notify, &minw, &minh, minw, minh);

   if (wd->content)
     {
	int offx = (w - minw) / 2;
	int offy = (h - minh) / 2;

	switch (wd->orient)
	  {
	   case ELM_NOTIFY_ORIENT_TOP:
             evas_object_move(wd->notify, x + offx, y);
             break;
	   case ELM_NOTIFY_ORIENT_CENTER:
             evas_object_move(wd->notify, x + offx, y + offy);
             break;
	   case ELM_NOTIFY_ORIENT_BOTTOM:
             evas_object_move(wd->notify, x + offx, y + h - minh);
             break;
	   case ELM_NOTIFY_ORIENT_LEFT:
             evas_object_move(wd->notify, x, y + offy);
             break;
	   case ELM_NOTIFY_ORIENT_RIGHT:
             evas_object_move(wd->notify, x + w - minw, y + offy);
             break;
	   case ELM_NOTIFY_ORIENT_TOP_LEFT:
             evas_object_move(wd->notify, x, y);
             break;
	   case ELM_NOTIFY_ORIENT_TOP_RIGHT:
             evas_object_move(wd->notify, x + w - minw, y);
             break;
	   case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
             evas_object_move(wd->notify, x, y + h - minh);
             break;
	   case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
             evas_object_move(wd->notify, x + w - minw, y + h - minh);
             break;
           case ELM_NOTIFY_ORIENT_LAST:
             break;
	  }
        evas_object_resize(wd->notify, minw, minh);
     }
    _sizing_eval(obj);
}

static Eina_Bool
_timer_cb(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->timer = NULL;
   evas_object_hide(obj);
   return ECORE_CALLBACK_CANCEL;
}

static void
_show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_show(wd->notify);
   if (!wd->repeat_events)
     evas_object_show(wd->block_events);
   if (wd->timer)
     {
	ecore_timer_del(wd->timer);
	wd->timer = NULL;
     }
   if (wd->timeout > 0)
     wd->timer = ecore_timer_add(wd->timeout, _timer_cb, obj);
}

static void
_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_hide(wd->notify);
   if (!wd->repeat_events)
     evas_object_hide(wd->block_events);
   if (wd->timer)
     {
	ecore_timer_del(wd->timer);
	wd->timer = NULL;
     }
}

static void
_parent_del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->parent = NULL;
   evas_object_hide(obj);
}

static void
_parent_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->parent = NULL;
   evas_object_hide(obj);
}

/**
 * Add a new notify to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Notify
 */
EAPI Evas_Object *
elm_notify_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "notify");
   elm_widget_type_set(obj, "notify");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->repeat_events = EINA_TRUE;

   wd->notify = edje_object_add(e);
   wd->orient = -1;
   elm_notify_orient_set(obj, ELM_NOTIFY_ORIENT_TOP);

   elm_notify_parent_set(obj, parent);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _resize, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the content of the notify widget
 *
 * Once the content object is set, a previously set one will be deleted.
 * If you want to keep that old content object, use the
 * elm_notify_content_unset() function.
 *
 * @param obj The notify object
 * @param content The content will be filled in this notify object
 *
 * @ingroup Notify
 */
EAPI void
elm_notify_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content == content) return;
   if (wd->content) evas_object_del(wd->content);
   wd->content = content;

   if (content)
     {
	elm_widget_sub_object_add(obj, content);
	evas_object_event_callback_add(content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				       _changed_size_hints, obj);
	evas_object_event_callback_add(content, EVAS_CALLBACK_RESIZE,
				       _content_resize, obj);
	edje_object_part_swallow(wd->notify, "elm.swallow.content", content);
     }
   _sizing_eval(obj);
   _calc(obj);
}

/**
 * Unset the content of the notify widget
 *
 * Unparent and return the content object which was set for this widget
 *
 * @param obj The notify object
 * @return The content that was being used
 *
 * @ingroup Notify
 */
EAPI Evas_Object *
elm_notify_content_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *content;
   if (!wd) return NULL;
   if (!wd->content) return NULL;
   content = wd->content;
   elm_widget_sub_object_del(obj, wd->content);
   edje_object_part_unswallow(wd->notify, wd->content);
   wd->content = NULL;
   return content;
}

/**
 * Return the content of the notify widget
 *
 * @param obj The notify object
 * @return The content that is being used
 *
 * @ingroup Notify
 */
EAPI Evas_Object *
elm_notify_content_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->content)) return NULL;
   return wd->content;
}

/**
 * Set the notify parent
 *
 * @param obj The notify object
 * @param content The new parent
 *
 * @ingroup Notify
 */
EAPI void
elm_notify_parent_set(Evas_Object *obj, Evas_Object *parent)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->parent)
     {
	evas_object_event_callback_del_full(wd->parent,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE,
                                            _changed_size_hints, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_MOVE,
                                            _changed_size_hints, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL,
                                            _parent_del, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_HIDE,
                                            _parent_hide, obj);
	wd->parent = NULL;
     }
   
   if (parent)
     {
	wd->parent = parent;
	evas_object_event_callback_add(parent,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE,
                                       _changed_size_hints, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_MOVE,
                                       _changed_size_hints, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL,
				       _parent_del, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_HIDE,
                                       _parent_hide, obj);
	edje_object_part_swallow(wd->notify, "elm.swallow.parent", parent);
	_sizing_eval(obj);
     }
   _calc(obj);
}

/**
 * Set the orientation
 *
 * @param obj The notify object
 * @param orient The new orientation
 */
EAPI void
elm_notify_orient_set(Evas_Object *obj, Elm_Notify_Orient orient)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->orient == orient) return;
   wd->orient = orient;
   _notify_theme_apply(obj);
   _resize(obj, NULL, obj, NULL);
}

/**
 * Return the orientation
 * @param obj the notify objects
 */
EAPI Elm_Notify_Orient
elm_notify_orient_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->orient;
}

/**
 * Set the time before the notify window is hidden. <br>
 * Set a value < 0 to disable the timer
 *
 * @param obj The notify object
 * @param time the new timeout
 */
EAPI void
elm_notify_timeout_set(Evas_Object *obj, int timeout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->timeout = timeout;
   elm_notify_timer_init(obj);
}

/**
 * Return the timeout value (in seconds)
 * @param obj the notify object
 */
EAPI int
elm_notify_timeout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->timeout;
}

/**
 * Re-init the timer
 * @param obj The notify object
 */
EAPI void
elm_notify_timer_init(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->timer) ecore_timer_del(wd->timer);
   wd->timer = NULL;
   if (wd->timeout > 0)
     wd->timer = ecore_timer_add(wd->timeout, _timer_cb, obj);
}

/**
 * When true if the user clicks outside the window the events will be
 * catch by the others widgets, else the events are block and the signal
 * dismiss will be sent when the user click outside the window.
 *
 * @note The default value is EINA_TRUE.
 *
 * @param obj The notify object
 * @param repeats EINA_TRUE Events are repeats, else no
 */
EAPI void
elm_notify_repeat_events_set(Evas_Object *obj, Eina_Bool repeat)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (repeat == wd->repeat_events) return;
   wd->repeat_events = repeat;
   if (!repeat)
     {
	wd->block_events = edje_object_add(evas_object_evas_get(obj));
	_block_events_theme_apply(obj);
        elm_widget_resize_object_set(obj, wd->block_events);
	edje_object_signal_callback_add(wd->block_events, "elm,action,clicked", "elm", _signal_block_clicked, obj);
     }
   else
     evas_object_del(wd->block_events);
}

/**
 * Return true if events are repeat below the notify object
 * @param obj the notify object
 */
EAPI Eina_Bool
elm_notify_repeat_events_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->repeat_events;
}

