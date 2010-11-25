#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup TickerNoti TickerNoti
 * @ingroup Elementary
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *win_indi,
			   *win_detail,
			   *edje_indi,
			   *edje_detail,
			   *icon_indi,
			   *icon_detail,
			   *button_detail;

   const char *label_indi,
		 	  *label_detail;

   int indicator_height,
	   angle;

   Elm_Tickernoti_Mode mode;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void
_del_job(void *data)
{
	Evas_Object *obj = data;
	evas_object_del(obj);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Evas_Object *p = elm_widget_parent_get(obj);
   if (p == wd->win_indi) {
	   ecore_job_add (_del_job, p);
   }

   evas_object_del (wd->edje_indi);
   wd->edje_indi = NULL;
   evas_object_del (wd->edje_detail);
   wd->edje_detail = NULL;
   evas_object_del (wd->win_detail);
   wd->win_detail = NULL;

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   _elm_theme_object_set (wd->win_indi, wd->edje_indi, "tickernoti", "base", elm_widget_style_get(obj));
   _elm_theme_object_set (wd->edje_detail, wd->edje_detail, "tickernoti", "2line", elm_widget_style_get(obj));

   edje_object_scale_set (wd->edje_indi, elm_widget_scale_get(obj) * _elm_config->scale);
   edje_object_scale_set (wd->edje_detail, elm_widget_scale_get(obj) * _elm_config->scale);

   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
//   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;

/*
   edje_object_size_min_calc(wd->win_detail, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_size_hint_align_set(obj, maxw, maxh);
*/
}

static void
_make_notification_window (Evas_Object *obj)
{
	Ecore_X_Window xwin;

	/* elm_win_xwindow_get() must call after elm_win_alpha_set() */
	xwin = elm_win_xwindow_get (obj);
	ecore_x_netwm_window_type_set (xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
}

static void _detail_show_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
	evas_object_smart_callback_call ((Evas_Object *)data, "detail,show", NULL);
}

static void _detail_hide_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
	evas_object_smart_callback_call ((Evas_Object *)data, "detail,hide", NULL);
}

static Evas_Object 
*_create_window (Evas_Object *parent, const char *name)
{
	Evas_Object *win;

	win = elm_win_add (parent, name, ELM_WIN_BASIC);

	/* Property */
	elm_win_title_set (win, name);
	elm_win_borderless_set (win, EINA_TRUE);
	elm_win_autodel_set (win, EINA_TRUE);
	elm_win_alpha_set (win, EINA_TRUE);

	/* set top window */
	_make_notification_window (win);

	return win;
}

static void 
_create_tickernoti_indi (Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Evas *e;
   Evas_Coord w, h;

   char *data_win_height = NULL; 

   ecore_x_window_size_get (ecore_x_window_root_first_get(), &w, &h);

   evas_object_move (wd->win_indi, 0, 0);

   e = evas_object_evas_get (wd->win_indi);

   wd->edje_indi = edje_object_add (e);
   _elm_theme_object_set (wd->win_indi, wd->edje_indi, "tickernoti", "base", "default");
   elm_win_resize_object_add (wd->win_indi, wd->edje_indi);

   /* tickernoti indicator height set */
   data_win_height = (char *)edje_object_data_get (wd->edje_indi, "height");
   if (data_win_height != NULL && elm_scale_get() > 0.0) 
	   wd->indicator_height = (int)(elm_scale_get() * atoi(data_win_height));

   evas_object_resize (wd->win_indi, w, wd->indicator_height);
	
   edje_object_signal_callback_add (wd->edje_indi, "request,detail,show", "", _detail_show_cb, obj);
   evas_object_show (wd->edje_indi);
}

static void 
_create_tickernoti_detail (Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Evas *e;
   Evas_Coord w, h;

   ecore_x_window_size_get (ecore_x_window_root_first_get(), &w, &h);

   evas_object_resize (wd->win_detail, w, h);
   evas_object_move (wd->win_detail, 0, 0);

   e = evas_object_evas_get (wd->win_detail);

   wd->edje_detail = edje_object_add (e);
   _elm_theme_object_set (wd->win_detail, wd->edje_detail, "tickernoti", "2line", "default");
   elm_win_resize_object_add (wd->win_detail, wd->edje_detail);

   edje_object_signal_callback_add(wd->edje_detail, "request,detail,hide", "", _detail_hide_cb, obj);
   evas_object_show (wd->edje_detail);
}

static void
_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{  
   Widget_Data *wd = elm_widget_data_get(obj);   
   if (!wd) return;

   if (wd->mode == ELM_TICKERNOTI_DEFAULT) {
	   evas_object_hide (wd->win_detail);
	   _make_notification_window (wd->win_indi);
	   evas_object_show (wd->win_indi);
	   edje_object_signal_emit (wd->edje_indi, "effect,show", "bg_1line");
   }
   else if (wd->mode == ELM_TICKERNOTI_DETAILVIEW) {
	   evas_object_hide (wd->win_indi);
	   _make_notification_window (wd->win_detail);
	   evas_object_show (wd->win_detail);
	   edje_object_signal_emit (wd->edje_detail, "effect,show", "bg_2line");
   }
}

static void
_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_hide (obj); 
   evas_object_hide (wd->win_indi);
   evas_object_hide (wd->win_detail);
}

/**
 * Add a tickernoti object to @p parent
 *
 * @param parent The parent object
 *
 * @return The tickernoti object, or NULL upon failure
 *
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   wd->win_indi = _create_window (parent, "indi");
   wd->win_detail = _create_window (parent, "detail");

   if (!parent)
	   parent = wd->win_indi;

   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "tickernoti");
   elm_widget_type_set(obj, "tickernoti");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->edje_indi = NULL;
   wd->edje_detail = NULL;
   wd->icon_indi = NULL;
   wd->icon_detail = NULL;
   wd->button_detail = NULL;

   wd->label_indi = NULL;
   wd->label_detail = NULL;

   wd->indicator_height = 0;
   wd->angle = 0;

   wd->mode = ELM_TICKERNOTI_DEFAULT;

   _create_tickernoti_indi (obj);
   _create_tickernoti_detail (obj);

   evas_object_event_callback_add (obj, EVAS_CALLBACK_SHOW, _show, NULL);
   evas_object_event_callback_add (obj, EVAS_CALLBACK_HIDE, _hide, NULL);

   return obj;
}


/**
 * Set the icon object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param icon The icon object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void 
elm_tickernoti_icon_set (const Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!icon) return;
   edje_object_part_swallow (wd->edje_indi, "icon", icon);
   wd->icon_indi = icon;
}

/**
 * Get the icon object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The icon object inside the tickernoti
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_icon_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon_indi;
}

/**
 * Set the label on the tickernoti object
 *
 * @param obj The tickernoti object
 * @param label The label will be used on the tickernoti object
 *
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_label_set (Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!label) label = "";
   eina_stringshare_replace(&wd->label_indi, label);
   edje_object_part_text_set(wd->edje_indi, "text", label);
   _sizing_eval(obj);
}

/**
 * Get the label used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The string inside the label
 * @ingroup TickerNoti
 */
EAPI const char *
elm_tickernoti_label_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label_indi;
}

/**
 * Set the detail label on the tickernoti object
 *
 * @param obj The tickernoti object
 * @param label The label will be used on the tickernoti object
 *
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_detailview_label_set (Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!label) label = "";
   eina_stringshare_replace(&wd->label_detail, label);
   edje_object_part_text_set(wd->edje_detail, "text", label);
   _sizing_eval(obj);
}

/**
 * Get the detail label used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The string inside the label
 * @ingroup TickerNoti
 */
EAPI const char *
elm_tickernoti_detailview_label_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label_detail;
}

/**
 * Set the button object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param button The button object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void 
elm_tickernoti_detailview_button_set (const Evas_Object *obj, Evas_Object *button)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!button) return;
   edje_object_part_swallow (wd->edje_detail, "button", button);
   wd->button_detail = button;
}

/**
 * Get the button object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The button object inside the tickernoti
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_detailview_button_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->button_detail;
}

/**
 * Set the detail icon object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param icon The icon object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void 
elm_tickernoti_detailview_icon_set (const Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!icon) return;
   edje_object_part_swallow (wd->edje_detail, "icon", icon);
   wd->icon_detail = icon;
}

/**
 * Get the detail icon object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The icon object inside the tickernoti
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_detailview_icon_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon_detail;
}

/**
 * Get the rotation used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The rotation angle 
 * @ingroup TickerNoti
 */
EAPI int
elm_tickernoti_rotation_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->angle;
}

/**
 * Set the rotation used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param angle The rotation angle will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_rotation_set (const Evas_Object *obj, int angle)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Evas_Coord x, y, w, h;
   Evas_Coord root_w, root_h;

   /* 
	* manual calculate win_tickernoti_indi window position & size 
	*  - win_indi is not full size window (480 x 27)
	*/
   ecore_x_window_size_get (ecore_x_window_root_first_get(), &root_w, &root_h);
   evas_object_geometry_get (wd->win_indi, &x, &y, &w, &h);

   /* rotate win */
   switch (angle) {
	   case 90:
		   w = root_h;
		   h = wd->indicator_height;
		   x = 0;
		   y = 0;
		   break;

	   case -90:
		   w = root_h; 
		   h = wd->indicator_height;
		   x = root_w-h;
		   y = 0;
		   break;

	   case 180:
		   w = root_w;
		   h = wd->indicator_height;
		   x = 0;
		   y = root_h-h;
		   break;

	   default:
	   case 0:
		   w = root_w;
		   h = wd->indicator_height;
		   x = 0;
		   y = 0;
		   break;
   }

	/* indicator */
	elm_win_rotation_with_resize_set (wd->win_indi, angle);
	evas_object_move (wd->win_indi, x, y);
	evas_object_resize (wd->win_indi, w, h);
	if (evas_object_visible_get (wd->win_indi)) {
		_make_notification_window (wd->win_indi);
	}
	
	/* detail */
	elm_win_rotation_with_resize_set (wd->win_detail, angle);
	if (evas_object_visible_get (wd->win_detail)) {
		_make_notification_window (wd->win_detail);
	}
}

/**
 * Get the view mode on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The view mode
 * @ingroup TickerNoti
 */
EAPI Elm_Tickernoti_Mode
elm_tickernoti_mode_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->mode;
}

/**
 * Set the view mode used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param mode The view mode will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_mode_set (const Evas_Object *obj, Elm_Tickernoti_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->mode = mode;
}

/**
 * Get the detail view window(elm_win) on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return detail view window(elm_win) object
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_detailview_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->win_detail;
}

