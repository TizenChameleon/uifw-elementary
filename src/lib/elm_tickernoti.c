#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup TickerNoti TickerNoti
 * @ingroup Elementary
 *
 * This is a notification widget which can be used to display some short information.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *win;
   Evas_Object *edje_obj;
   Evas_Object *icon;
   Evas_Object *button;

   const char *label;

   int noti_height;
   int angle;

   Elm_Tickernoti_Mode mode;
   Elm_Tickernoti_Orient orient;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);

static void
_del_job(void *data)
{
   evas_object_del(data);
}

static void
_del_hook(Evas_Object *obj)
{
   Evas_Object *p;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   p = elm_widget_parent_get(obj);
   if (wd->win) ecore_job_add (_del_job, p);

   evas_object_del (wd->edje_obj);
   wd->edje_obj = NULL;

   free(wd);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   edje_object_mirrored_set(wd->edje_obj, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   char *data_win_height = NULL;
   Evas_Coord w;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   _elm_theme_object_set (wd->win, wd->edje_obj, "tickernoti",
                          "base", elm_widget_style_get(obj));

   edje_object_scale_set (wd->edje_obj, elm_widget_scale_get(obj) * _elm_config->scale);

   /* tickernoti detail height set */
   data_win_height = (char *)edje_object_data_get (wd->edje_obj, "height");
   if (data_win_height != NULL && elm_scale_get() > 0.0)
     wd->noti_height = (int)(elm_scale_get() * atoi(data_win_height));

   evas_object_geometry_get(wd->win, NULL, NULL, &w, NULL);
   evas_object_resize (wd->win, w, wd->noti_height);

   if (wd->label)
     edje_object_part_text_set(wd->edje_obj, "elm.text", wd->label);
   if (wd->icon)
     edje_object_part_swallow (wd->edje_obj, "icon", wd->icon);
   if (wd->button)
      edje_object_part_swallow (wd->edje_obj, "button", wd->button);
   edje_object_signal_emit (wd->edje_obj, "effect,show", "elm");
   edje_object_message_signal_process(wd->edje_obj);

   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   if (!wd) return;
   edje_object_size_min_calc(wd->win, &minw, &minh);
//TODO:
}

#ifdef HAVE_ELEMENTARY_X
static void
_update_window_hints (Evas_Object *obj)
{
   Ecore_X_Window xwin;
   Ecore_X_Atom _notification_level_atom;
   int level;
/* elm_win_xwindow_get() must call after elm_win_alpha_set() */
   xwin = elm_win_xwindow_get (obj);

   ecore_x_icccm_hints_set(xwin, 0, ECORE_X_WINDOW_STATE_HINT_NONE, 0, 0, 0, 0, 0);
   ecore_x_netwm_window_type_set (xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
   ecore_x_netwm_opacity_set(xwin, 0);
   /* Create atom for notification level */
   _notification_level_atom = ecore_x_atom_get ("_E_ILLUME_NOTIFICATION_LEVEL");

   /* HIGH:150, NORMAL:100, LOW:50 */
   level = 100;

   /* Set notification level of the window */
   ecore_x_window_prop_property_set (xwin, _notification_level_atom, ECORE_X_ATOM_CARDINAL, 32, &level, 1);
}
#endif

static void _hide_cb (void *data, Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   evas_object_hide (wd->win);
   evas_object_smart_callback_call (data, "hide", NULL);
}

static Evas_Object
*_create_window (Evas_Object *parent, const char *name)
{
   Evas_Object *win;

   win = elm_win_add (parent, name, ELM_WIN_BASIC);
   elm_win_title_set (win, name);
   elm_win_borderless_set (win, EINA_TRUE);
   elm_win_autodel_set (win, EINA_TRUE);
   elm_win_alpha_set(win, EINA_TRUE);

#ifdef HAVE_ELEMENTARY_X
/* set top window */
   _update_window_hints (win);
#endif
   return win;
}

static void
_create_tickernoti (Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_X
   Evas_Coord w;
#endif
   Widget_Data *wd = elm_widget_data_get(obj);
   char *data_win_height = NULL;
   Evas *e;

   if (!wd) return;

   evas_object_move (wd->win, 0, 0);
   e = evas_object_evas_get (wd->win);

   wd->edje_obj = edje_object_add (e);
   _elm_theme_object_set (wd->win, wd->edje_obj, "tickernoti", "base", "default");
   elm_win_resize_object_add (wd->win, wd->edje_obj);

   /* tickernoti detail height set */
   data_win_height = (char *)edje_object_data_get (wd->edje_obj, "height");
   if (data_win_height != NULL && elm_scale_get() > 0.0)
     wd->noti_height = (int)(elm_scale_get() * atoi(data_win_height));

#ifdef HAVE_ELEMENTARY_X
   ecore_x_window_size_get (ecore_x_window_root_first_get(), &w, NULL);
   evas_object_resize (wd->win, w, wd->noti_height);
#endif

   edje_object_signal_callback_add(wd->edje_obj, "request,hide", "", _hide_cb, obj);
   evas_object_show (wd->edje_obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
//TODO:
}

static void
_show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

#ifdef HAVE_ELEMENTARY_X
   _update_window_hints (wd->win);
#endif
   evas_object_show (wd->win);
   edje_object_signal_emit (wd->edje_obj, "effect,show", "elm");
   edje_object_message_signal_process(wd->edje_obj);
}

static void
_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_hide (obj);
   evas_object_hide (wd->win);
}

static void _tickernoti_hide_cb (void *data, Evas_Object *obj __UNUSED__,
                                 void *event_info __UNUSED__)
{
   Widget_Data *wd = data;

   if (!wd) return;

   edje_object_signal_emit (wd->edje_obj, "effect,hide", "elm");
   edje_object_message_signal_process(wd->edje_obj);
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
   wd->win = _create_window (parent, "noti-window");

   e = evas_object_evas_get(wd->win);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "tickernoti");
   elm_widget_type_set(obj, "tickernoti");
   elm_widget_sub_object_add(wd->win, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);
   elm_widget_disable_hook_set(obj, _disable_hook);

   wd->orient = ELM_TICKERNOTI_ORIENT_TOP;

   _create_tickernoti (obj);

   evas_object_event_callback_add (obj, EVAS_CALLBACK_SHOW, _show, NULL);
   evas_object_event_callback_add (obj, EVAS_CALLBACK_HIDE, _hide, NULL);

   return obj;
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
   eina_stringshare_replace(&wd->label, label);
   edje_object_part_text_set(wd->edje_obj, "elm.text", wd->label);
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

   return wd->label;
}

/**
 * Set the action button object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param button The button object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_button_set (Evas_Object *obj, Evas_Object *button)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!button) return;
   edje_object_part_swallow (wd->edje_obj, "button", button);
   wd->button = button;
   evas_object_smart_callback_add (wd->button, "clicked", _tickernoti_hide_cb, wd);
}

/**
 * Get the action button object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The button object inside the tickernoti
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_button_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->button;
}

/**
 * Set the icon object of the tickernoti object
 *
 * @param obj The tickernotil object
 * @param icon The icon object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_icon_set (Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (!icon) return;
   edje_object_part_swallow (wd->edje_obj, "icon", icon);
   wd->icon = icon;
}

/**
 * Get the icon object of the tickernoti object
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
   return wd->icon;
}

/**
 * Get the rotation of tickernoti object
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
 * Set the rotation angle for the tickernoti object
 *
 * @param obj The tickernoti object
 * @param angle The rotation angle(in degree) will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_rotation_set (Evas_Object *obj, int angle)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord w, x = 0, y = 0;

   if (!wd) return;
   if (angle%90 != 0) return;

   wd->angle = angle%360;
   if (wd->angle < 0)
     wd->angle = 360 + wd->angle;

#ifdef HAVE_ELEMENTARY_X
   Evas_Coord root_w, root_h;

   /*
   * manual calculate win_tickernoti_indi window position & size
   *  - win_indi is not full size window (480 x 27)
   */
   ecore_x_window_size_get (ecore_x_window_root_first_get(), &root_w, &root_h);

   /* rotate win */
   switch (wd->angle)
     {
      case 90:
         w = root_h;
         if (wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           x = root_w-wd->noti_height;
         break;
      case 270:
         w = root_h;
         if (!(wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM))
           x = root_w-wd->noti_height;
         break;
      case 180:
         w = root_w;
         if (!wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           y = root_h - wd->noti_height;
         break;
       case 0:
      default:
         w = root_w;
         if (wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           y = root_h - wd->noti_height;
         break;
     }
#endif
/* detail */
   elm_win_rotation_with_resize_set (wd->win, wd->angle);
   evas_object_move (wd->win, x, y);

   evas_object_resize (wd->win, w, wd->noti_height);
#ifdef HAVE_ELEMENTARY_X
   _update_window_hints (wd->win);
#endif
}

/**
 * Set the orientation of the tickernoti object
 *
 * @param obj The tickernoti object
 * @param orient The orientation of tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_orientation_set (Evas_Object *obj, Elm_Tickernoti_Orient orient)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

#ifdef HAVE_ELEMENTARY_X
   Evas_Coord root_w, root_h;
#endif
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

#ifdef HAVE_ELEMENTARY_X
   ecore_x_window_size_get (ecore_x_window_root_first_get(), &root_w, &root_h);
#endif

   switch (orient) {
      case ELM_TICKERNOTI_ORIENT_BOTTOM:
#ifdef HAVE_ELEMENTARY_X
         evas_object_move (wd->win, 0, root_h - wd->noti_height);
#endif
         wd->orient = ELM_TICKERNOTI_ORIENT_BOTTOM;
         break;
      case ELM_TICKERNOTI_ORIENT_TOP:
      default:
#ifdef HAVE_ELEMENTARY_X
         evas_object_move (wd->win, 0, 0);
#endif
         wd->orient = ELM_TICKERNOTI_ORIENT_TOP;
         break;
   }
}

/**
 * Get the orientation of the tickernoti object
 *
 * @param obj The tickernotil object
 * @return The orientation of tickernotil object
 * @ingroup TickerNoti
 */
EAPI Elm_Tickernoti_Orient
elm_tickernoti_orientation_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return -1;
   return wd->orient;
}

// ################### Below APIs are going to be removed. ###########################
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
   elm_tickernoti_label_set (obj, label);
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
   return elm_tickernoti_label_get (obj);
}

/**
 * Set the button object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param button The button object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_detailview_button_set (Evas_Object *obj, Evas_Object *button)
{
   elm_tickernoti_button_set (obj, button);
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
   return elm_tickernoti_button_get (obj);
}



/**
 * Set the detail icon object used on the tickernoti object
 *
 * @param obj The tickernotil object
 * @param icon The icon object will be used on the tickernoti object
 * @ingroup TickerNoti
 */
EAPI void
elm_tickernoti_detailview_icon_set (Evas_Object *obj, Evas_Object *icon)
{
   elm_tickernoti_icon_set (obj, icon);
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
   return elm_tickernoti_icon_get (obj);
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
elm_tickernoti_mode_set (Evas_Object *obj, Elm_Tickernoti_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   switch(mode){
      case ELM_TICKERNOTI_DEFAULT:
      case ELM_TICKERNOTI_DETAILVIEW:
         wd->mode = mode;
         break;
      default:
         break;
   }
}

/**
 * Get the detail view window(elm_win) on the tickernoti object
 *
 * @param obj The tickernotil object
 * @return detail view window(elm_win) object
 * @ingroup TickerNoti
 */
EAPI Evas_Object *
elm_tickernoti_win_get (const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->win;
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
   return elm_tickernoti_win_get (obj);
}

