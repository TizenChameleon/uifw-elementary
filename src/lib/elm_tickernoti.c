#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *win;
   Evas_Object *edje_obj;
   Evas_Object *icon;
   Evas_Object *button;
   Ecore_Event_Handler *rotation_event_handler;
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
static void _update_geometry_on_rotation(Evas_Object *obj, int angle, int *x, int *y, int *w);

static const char SIG_CLICKED[] = "clicked";
static const char SIG_HIDDEN[] = "hide";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CLICKED, ""},
       {SIG_HIDDEN, ""},
       {NULL, NULL}
};

static void
_del_job(void *data)
{
   evas_object_del(data);
}

static void
_del_hook(Evas_Object *obj)
{
   Evas_Object *parent;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   parent = elm_widget_parent_get(obj);
   if (wd->rotation_event_handler)
     ecore_event_handler_del(wd->rotation_event_handler);
   if (wd->win) ecore_job_add(_del_job, parent);
   evas_object_del(wd->edje_obj);
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

   _elm_theme_object_set(wd->win, wd->edje_obj, "tickernoti",
                          "base", elm_widget_style_get(obj));

   /* tickernoti detail height set */
   data_win_height = (char *)edje_object_data_get(wd->edje_obj, "height");
   if (data_win_height != NULL && elm_scale_get() > 0.0)
     wd->noti_height = (int)(elm_scale_get() * atoi(data_win_height));

   evas_object_geometry_get(wd->win, NULL, NULL, &w, NULL);
   evas_object_resize(wd->win, w, wd->noti_height);

   edje_object_signal_emit(wd->edje_obj, "effect,show", "elm");/*goes too late*/
   edje_object_message_signal_process(wd->edje_obj);
   edje_object_scale_set(wd->edje_obj, elm_widget_scale_get(obj) * _elm_config->scale);

   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;

   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->edje_obj, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
}

#ifdef HAVE_ELEMENTARY_X
static void
_update_window_hints(Evas_Object *obj)
{
   Ecore_X_Window xwin;
   Ecore_X_Atom _notification_level_atom;
   int level;
   // elm_win_xwindow_get() must call after elm_win_alpha_set()
   xwin = elm_win_xwindow_get(obj);

   ecore_x_icccm_hints_set(xwin, 0, ECORE_X_WINDOW_STATE_HINT_NONE, 0, 0, 0, 0, 0);
   ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
   ecore_x_netwm_opacity_set(xwin, 0);
   // Create atom for notification level
   _notification_level_atom = ecore_x_atom_get("_E_ILLUME_NOTIFICATION_LEVEL");

   // HIGH:150, NORMAL:100, LOW:50
   level = 100;

   // Set notification level of the window
   ecore_x_window_prop_property_set(xwin, _notification_level_atom, ECORE_X_ATOM_CARDINAL, 32, &level, 1);
}
#endif

static void _hide_cb(void *data, Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   evas_object_hide(wd->win);
   evas_object_smart_callback_call(data, SIG_HIDDEN, NULL);
}

static void _clicked_cb(void *data, Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
}

static Evas_Object
*_create_window(Evas_Object *parent, const char *name)
{
   Evas_Object *win;

   win = elm_win_add(parent, name, ELM_WIN_BASIC);
   elm_win_title_set(win, name);
   elm_win_borderless_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_alpha_set(win, EINA_TRUE);
   evas_object_size_hint_weight_set(win, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(win, EVAS_HINT_FILL, EVAS_HINT_FILL);

#ifdef HAVE_ELEMENTARY_X
   // set top window
   _update_window_hints(win);
#endif
   return win;
}

static void
_win_rotated(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int x = 0, y = 0, w = 0, angle = 0;

   if (!wd) return;
   angle = elm_win_rotation_get(wd->win);
   if (angle % 90) return;
   angle %= 360;
   if (angle < 0) angle += 360;
   wd->angle = angle;
   _update_geometry_on_rotation(obj, wd->angle, &x, &y, &w);
   evas_object_move(wd->win, x, y);
   evas_object_resize(wd->win, w, wd->noti_height);
#ifdef HAVE_ELEMENTARY_X
   _update_window_hints(wd->win);
#endif
}

static Eina_Bool
_prop_change(void *data, int type __UNUSED__, void *event)
{
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Event_Window_Property *ev;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_PASS_ON;
   ev = event;
   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
     {
        if (ev->win == elm_win_xwindow_get(wd->win))
          {
             _win_rotated(data);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
#endif
}

static void
_create_tickernoti(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_X
   Evas_Coord w;
#endif
   Widget_Data *wd = elm_widget_data_get(obj);
   char *data_win_height = NULL;
   Evas *e;

   if (!wd) return;

   evas_object_move(wd->win, 0, 0);
   e = evas_object_evas_get(wd->win);

   wd->edje_obj = edje_object_add(e);
   _elm_theme_object_set(wd->win, wd->edje_obj, "tickernoti", "base", "default");
   elm_win_resize_object_add(wd->win, wd->edje_obj);

   // tickernoti height setting
   data_win_height = (char *)edje_object_data_get(wd->edje_obj, "height");
   if (data_win_height != NULL && elm_scale_get() > 0.0)
     wd->noti_height = (int)(elm_scale_get() * atoi(data_win_height));

#ifdef HAVE_ELEMENTARY_X
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, NULL);
   evas_object_size_hint_min_set(wd->edje_obj, w, wd->noti_height);
   evas_object_resize(wd->win, w, wd->noti_height);
   wd->rotation_event_handler = ecore_event_handler_add(
            ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, obj);
#endif

   edje_object_signal_callback_add(wd->edje_obj, "request,hide", "", _hide_cb, obj);
   edje_object_signal_callback_add(wd->edje_obj, "clicked", "", _clicked_cb, obj);
   evas_object_show(wd->edje_obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
//TODO: To stop the event in case of being disabled
}

static void
_show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

#ifdef HAVE_ELEMENTARY_X
   _update_window_hints(wd->win);
#endif
   evas_object_show(wd->win);
   edje_object_signal_emit(wd->edje_obj, "effect,show", "elm");
   edje_object_message_signal_process(wd->edje_obj);
}

static void
_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_hide(wd->win);
}

static void _tickernoti_hide_cb(void *data, Evas_Object *obj __UNUSED__,
                                 void *event_info __UNUSED__)
{
   Widget_Data *wd = data;

   if (!wd) return;

   edje_object_signal_emit(wd->edje_obj, "effect,hide", "elm");
   edje_object_message_signal_process(wd->edje_obj);
}

static void
_update_geometry_on_rotation(Evas_Object *obj, int angle, int *x, int *y, int *w)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

#ifdef HAVE_ELEMENTARY_X
   Evas_Coord root_w, root_h;

   /*
   * manually calculate win_tickernoti_indi window position & size
   *  - win_indi is not full size window
   */
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w, &root_h);
   // rotate win
   switch(angle)
     {
      case 90:
         *w = root_h;
         if (wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           *x = root_w - wd->noti_height;
         break;
      case 270:
         *w = root_h;
         if (!(wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM))
           *x = root_w - wd->noti_height;
         break;
      case 180:
         *w = root_w;
         if (!wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           *y = root_h - wd->noti_height;
         break;
       case 0:
      default:
         *w = root_w;
         if (wd->orient == ELM_TICKERNOTI_ORIENT_BOTTOM)
           *y = root_h - wd->noti_height;
         break;
     }
#endif
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   if (sub == wd->icon)
     wd->icon = NULL;
   if (sub == wd->button)
     wd->button = NULL;
}

static void
_elm_tickernoti_label_set(Evas_Object *obj, const char *part, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (part && strcmp(part, "default")) return;
   eina_stringshare_replace(&wd->label, label);
   edje_object_part_text_set(wd->edje_obj, "elm.text", wd->label);
   _sizing_eval(obj);
}

const char *
_elm_tickernoti_label_get(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (part && strcmp(part, "default")) return NULL;
   if (!wd) return NULL;
   return wd->label;
}

static void
_elm_tickernoti_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->icon == icon) return;
   if (wd->icon) evas_object_del(wd->icon);
   wd->icon = icon;
   if (icon)
     {
        elm_widget_sub_object_add(obj, icon);
        edje_object_part_swallow(wd->edje_obj, "icon", icon);
     }
}

static void
_elm_tickernoti_button_set(Evas_Object *obj, Evas_Object *button)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->button == button) return;
   if (wd->button) evas_object_del(wd->button);
   wd->button = button;
   if (button)
     {
        elm_widget_sub_object_add(obj, button);
        edje_object_part_swallow(wd->edje_obj, "button", button);
        evas_object_smart_callback_add(wd->button, "clicked", _tickernoti_hide_cb, wd);
     }
}

static void
_elm_tickernoti_content_part_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !part) return;
   if (!part || !strcmp(part, "icon"))
     {
        _elm_tickernoti_icon_set(obj, content);
        return;
     }
   else if (!strcmp(part, "button"))
     {
        _elm_tickernoti_button_set(obj, content);
        return;
     }
}

static Evas_Object *
_elm_tickernoti_icon_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon;
}

static Evas_Object *
_elm_tickernoti_button_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->button;
}

static Evas_Object *
_elm_tickernoti_content_part_get_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !part) return NULL;
   if (!part || !strcmp(part, "icon"))
     return _elm_tickernoti_icon_get(obj);
   else if (!strcmp(part, "button"))
     return _elm_tickernoti_button_get(obj);
   return NULL;
}

static Evas_Object *
_elm_tickernoti_icon_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Evas_Object *icon;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->icon) return NULL;
   icon = wd->icon;
   elm_widget_sub_object_del(obj, wd->icon);
   edje_object_part_unswallow(wd->edje_obj, icon);
   wd->icon = NULL;
   return icon;
}

static Evas_Object *
_elm_tickernoti_button_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Evas_Object *button;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->button) return NULL;
   button = wd->button;
   elm_widget_sub_object_del(obj, wd->button);
   edje_object_part_unswallow(wd->edje_obj, button);
   wd->button = NULL;
   return button;
}

static Evas_Object *
_elm_tickernoti_content_part_unset_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !part) return NULL;
   if (!strcmp(part, "icon"))
     return _elm_tickernoti_icon_unset(obj);
   else if (!strcmp(part, "button"))
     return _elm_tickernoti_button_unset(obj);
   return NULL;
}

EAPI Evas_Object *
elm_tickernoti_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   wd->win = _create_window(parent, "noti-window");

   e = evas_object_evas_get(wd->win);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "tickernoti");
   elm_widget_type_set(obj, widtype);
   elm_widget_sub_object_add(wd->win, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);
   elm_widget_disable_hook_set(obj, _disable_hook);

   wd->orient = ELM_TICKERNOTI_ORIENT_TOP;

   _create_tickernoti(obj);
   elm_widget_text_set_hook_set(obj, _elm_tickernoti_label_set);
   elm_widget_text_get_hook_set(obj, _elm_tickernoti_label_get);
   elm_widget_content_set_hook_set(obj, _elm_tickernoti_content_part_set_hook);
   elm_widget_content_get_hook_set(obj, _elm_tickernoti_content_part_get_hook);
   elm_widget_content_unset_hook_set(obj, _elm_tickernoti_content_part_unset_hook);
   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, NULL);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, NULL);
   evas_object_smart_callbacks_descriptions_set(obj, _signals);
   return obj;
}

EAPI int
elm_tickernoti_rotation_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->angle;
}

EAPI void
elm_tickernoti_rotation_set(Evas_Object *obj, int angle)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (angle % 90) return;
   angle %= 360;
   if (angle < 0) angle += 360;
   wd->angle = angle;
   elm_win_rotation_set(wd->win, angle);
   _win_rotated(obj);
}

EAPI void
elm_tickernoti_orient_set(Evas_Object *obj, Elm_Tickernoti_Orient orient)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

#ifdef HAVE_ELEMENTARY_X
   Evas_Coord root_w, root_h;
#endif
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (orient >= ELM_TICKERNOTI_ORIENT_LAST) return;

#ifdef HAVE_ELEMENTARY_X
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w, &root_h);
#endif

   switch(orient) {
      case ELM_TICKERNOTI_ORIENT_BOTTOM:
#ifdef HAVE_ELEMENTARY_X
         evas_object_move(wd->win, 0, root_h - wd->noti_height);
#endif
         wd->orient = ELM_TICKERNOTI_ORIENT_BOTTOM;
         break;
      case ELM_TICKERNOTI_ORIENT_TOP:
      default:
#ifdef HAVE_ELEMENTARY_X
         evas_object_move(wd->win, 0, 0);
#endif
         wd->orient = ELM_TICKERNOTI_ORIENT_TOP;
         break;
   }
#ifdef HAVE_ELEMENTARY_X
   _update_window_hints(wd->win);
#endif
}

EAPI Elm_Tickernoti_Orient
elm_tickernoti_orient_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ELM_TICKERNOTI_ORIENT_LAST;
   return wd->orient;
}

EAPI Evas_Object *
elm_tickernoti_win_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->win;
}

EAPI void
elm_tickernoti_detailview_label_set(Evas_Object *obj, const char *label)
{
   _elm_tickernoti_label_set(obj, NULL, label);
}

EAPI const char *
elm_tickernoti_detailview_label_get(const Evas_Object *obj)
{
   return _elm_tickernoti_label_get(obj, NULL);
}

EAPI void
elm_tickernoti_detailview_button_set(Evas_Object *obj, Evas_Object *button)
{
   _elm_tickernoti_button_set(obj, button);
}

EAPI Evas_Object *
elm_tickernoti_detailview_button_get(const Evas_Object *obj)
{
   return _elm_tickernoti_button_get(obj);
}

EAPI void
elm_tickernoti_detailview_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   _elm_tickernoti_icon_set(obj, icon);
}

EAPI Evas_Object *
elm_tickernoti_detailview_icon_get(const Evas_Object *obj)
{
   return _elm_tickernoti_icon_get(obj);
}

EAPI Elm_Tickernoti_Mode
elm_tickernoti_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->mode;
}

EAPI void
elm_tickernoti_mode_set(Evas_Object *obj, Elm_Tickernoti_Mode mode)
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

EAPI Evas_Object *
elm_tickernoti_detailview_get(const Evas_Object *obj)
{
   return elm_tickernoti_win_get(obj);
}

EAPI void
elm_tickernoti_orientation_set(Evas_Object *obj, Elm_Tickernoti_Orient orient)
{
   elm_tickernoti_orient_set(obj, orient);
}

EAPI Elm_Tickernoti_Orient
elm_tickernoti_orientation_get(const Evas_Object *obj)
{
   return elm_tickernoti_orient_get(obj);
}

EAPI void
elm_tickernoti_label_set(Evas_Object *obj, const char *label)
{
   _elm_tickernoti_label_set(obj, NULL, label);
}

EAPI const char *
elm_tickernoti_label_get(const Evas_Object *obj)
{
   return _elm_tickernoti_label_get(obj, NULL);
}

EAPI void
elm_tickernoti_button_set(Evas_Object *obj, Evas_Object *button)
{
   _elm_tickernoti_button_set(obj, button);
}

EAPI Evas_Object *
elm_tickernoti_button_get(const Evas_Object *obj)
{
   return _elm_tickernoti_button_get(obj);
}

EAPI void
elm_tickernoti_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   _elm_tickernoti_icon_set(obj, icon);
}

EAPI Evas_Object *
elm_tickernoti_icon_get(const Evas_Object *obj)
{
   return _elm_tickernoti_icon_get(obj);
}
