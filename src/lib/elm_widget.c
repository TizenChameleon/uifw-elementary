#include <Elementary.h>
#include "elm_priv.h"

static const char SMART_NAME[] = "elm_widget";

#define API_ENTRY                                    \
  Smart_Data * sd = evas_object_smart_data_get(obj); \
  if ((!sd) || (!_elm_widget_is(obj)))
#define INTERNAL_ENTRY                               \
  Smart_Data * sd = evas_object_smart_data_get(obj); \
  if (!sd) return

#undef elm_widget_text_set_hook_set
#undef elm_widget_text_get_hook_set
#undef elm_widget_content_set_hook_set
#undef elm_widget_content_get_hook_set
#undef elm_widget_content_unset_hook_set

typedef struct _Smart_Data        Smart_Data;
typedef struct _Edje_Signal_Data  Edje_Signal_Data;
typedef struct _Elm_Event_Cb_Data Elm_Event_Cb_Data;
typedef struct _Elm_Translate_String_Data Elm_Translate_String_Data;

struct _Smart_Data
{
   Evas_Object *obj;
   const char  *type;
   Evas_Object *parent_obj;
   Evas_Object *parent2;
   Evas_Coord   x, y, w, h;
   Eina_List   *subobjs;
   Evas_Object *resize_obj;
   Evas_Object *hover_obj;
   Eina_List   *tooltips, *cursors;
   void       (*del_func)(Evas_Object *obj);
   void       (*del_pre_func)(Evas_Object *obj);
   void       (*focus_func)(Evas_Object *obj);
   void       (*activate_func)(Evas_Object *obj);
   void       (*disable_func)(Evas_Object *obj);
   void       (*theme_func)(Evas_Object *obj);
   void       (*translate_func)(Evas_Object *obj);
   Eina_Bool  (*event_func)(Evas_Object       *obj,
                            Evas_Object       *source,
                            Evas_Callback_Type type,
                            void              *event_info);
   void       (*signal_func)(Evas_Object *obj,
                             const char  *emission,
                             const char  *source);
   void       (*callback_add_func)(Evas_Object   *obj,
                                   const char    *emission,
                                   const char    *source,
                                   Edje_Signal_Cb func,
                                   void          *data);
   void       (*callback_del_func)(Evas_Object   *obj,
                                   const char    *emission,
                                   const char    *source,
                                   Edje_Signal_Cb func,
                                   void          *data);
   void       (*changed_func)(Evas_Object *obj);
   Eina_Bool  (*focus_next_func)(const Evas_Object  *obj,
                                 Elm_Focus_Direction dir,
                                 Evas_Object       **next);
   void       (*on_focus_func)(void        *data,
                               Evas_Object *obj);
   void        *on_focus_data;
   void       (*on_change_func)(void        *data,
                                Evas_Object *obj);
   void        *on_change_data;
   void       (*on_show_region_func)(void        *data,
                                     Evas_Object *obj);
   void        *on_show_region_data;
   void       (*focus_region_func)(Evas_Object *obj,
                                   Evas_Coord   x,
                                   Evas_Coord   y,
                                   Evas_Coord   w,
                                   Evas_Coord   h);
   void       (*on_focus_region_func)(const Evas_Object *obj,
                                      Evas_Coord        *x,
                                      Evas_Coord        *y,
                                      Evas_Coord        *w,
                                      Evas_Coord        *h);
   Elm_Widget_Text_Set_Cb text_set_func;
   Elm_Widget_Text_Get_Cb text_get_func;
   Elm_Widget_Content_Set_Cb content_set_func;
   Elm_Widget_Content_Get_Cb content_get_func;
   Elm_Widget_Content_Unset_Cb content_unset_func;
   void        *data;
   Evas_Coord   rx, ry, rw, rh;
   int          scroll_hold;
   int          scroll_freeze;
   double       scale;
   Elm_Theme   *theme;
   const char  *style;
   const char  *access_info;
   unsigned int focus_order;
   Eina_Bool    focus_order_on_calc;

   int          child_drag_x_locked;
   int          child_drag_y_locked;

   Eina_List   *edje_signals;
   Eina_List   *translate_strings;

   Eina_Bool    drag_x_locked : 1;
   Eina_Bool    drag_y_locked : 1;

   Eina_Bool    can_focus : 1;
   Eina_Bool    child_can_focus : 1;
   Eina_Bool    focused : 1;
   Eina_Bool    top_win_focused : 1;
   Eina_Bool    tree_unfocusable : 1;
   Eina_Bool    highlight_ignore : 1;
   Eina_Bool    highlight_in_theme : 1;
   Eina_Bool    disabled : 1;
   Eina_Bool    is_mirrored : 1;
   Eina_Bool    mirrored_auto_mode : 1;   /* This is TRUE by default */
   Eina_Bool    still_in : 1;

   Eina_List   *focus_chain;
   Eina_List   *event_cb;
};

struct _Edje_Signal_Data
{
   Evas_Object   *obj;
   Edje_Signal_Cb func;
   const char    *emission;
   const char    *source;
   void          *data;
};

struct _Elm_Event_Cb_Data
{
   Elm_Event_Cb func;
   const void  *data;
};

struct _Elm_Translate_String_Data
{
   const char *id;
   const char *domain;
   const char *string;
};

/* local subsystem functions */
static void _smart_reconfigure(Smart_Data *sd);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_move(Evas_Object *obj,
                        Evas_Coord   x,
                        Evas_Coord   y);
static void _smart_resize(Evas_Object *obj,
                          Evas_Coord   w,
                          Evas_Coord   h);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_color_set(Evas_Object *obj,
                             int          r,
                             int          g,
                             int          b,
                             int          a);
static void _smart_clip_set(Evas_Object *obj,
                            Evas_Object *clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_calculate(Evas_Object *obj);
static void _smart_member_add(Evas_Object *obj, Evas_Object *child);
static void _smart_member_del(Evas_Object *obj, Evas_Object *child);
static void _smart_init(void);

static void _if_focused_revert(Evas_Object *obj,
                               Eina_Bool    can_focus_only);
static Evas_Object *_newest_focus_order_get(Evas_Object  *obj,
                                            unsigned int *newest_focus_order,
                                            Eina_Bool     can_focus_only);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;
static Eina_List  *widtypes = NULL;

static unsigned int focus_order = 0;

// internal funcs
static inline Eina_Bool
_elm_widget_is(const Evas_Object *obj)
{
   const char *type = evas_object_type_get(obj);
   return type == SMART_NAME;
}

static inline Eina_Bool
_is_focusable(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->can_focus || (sd->child_can_focus);
}

static void
_unfocus_parents(Evas_Object *obj)
{
   for (; obj; obj = elm_widget_parent_get(obj))
     {
        INTERNAL_ENTRY;
        if (!sd->focused) return;
        sd->focused = 0;
     }
}

static void
_focus_parents(Evas_Object *obj)
{
   for (; obj; obj = elm_widget_parent_get(obj))
     {
        INTERNAL_ENTRY;
        if (sd->focused) return;
        sd->focused = 1;
     }
}

static void
_sub_obj_del(void        *data,
             Evas        *e __UNUSED__,
             Evas_Object *obj,
             void        *event_info __UNUSED__)
{
   Smart_Data *sd = data;

   if (_elm_widget_is(obj))
     {
        if (elm_widget_focus_get(obj)) _unfocus_parents(sd->obj);
     }
   if (obj == sd->resize_obj)
     sd->resize_obj = NULL;
   else if (obj == sd->hover_obj)
     sd->hover_obj = NULL;
   else
     sd->subobjs = eina_list_remove(sd->subobjs, obj);
   evas_object_smart_callback_call(sd->obj, "sub-object-del", obj);
}

static void
_sub_obj_hide(void        *data __UNUSED__,
              Evas        *e __UNUSED__,
              Evas_Object *obj,
              void        *event_info __UNUSED__)
{
   elm_widget_focus_hide_handle(obj);
}

static void
_sub_obj_mouse_down(void        *data,
                    Evas        *e __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void        *event_info)
{
   Smart_Data *sd = data;
   Evas_Event_Mouse_Down *ev = event_info;
   if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
     sd->still_in = EINA_TRUE;
}

static void
_sub_obj_mouse_move(void        *data,
                    Evas        *e __UNUSED__,
                    Evas_Object *obj,
                    void        *event_info)
{
   Smart_Data *sd = data;
   Evas_Event_Mouse_Move *ev = event_info;
   if (sd->still_in)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          sd->still_in = EINA_FALSE;
        else
          {
             Evas_Coord x, y, w, h;
             evas_object_geometry_get(obj, &x, &y, &w, &h);
             if ((ev->cur.canvas.x < x) || (ev->cur.canvas.y < y) ||
                 (ev->cur.canvas.x >= (x + w)) || (ev->cur.canvas.y >= (y + h)))
               sd->still_in = EINA_FALSE;
          }
     }
}

static void
_sub_obj_mouse_up(void        *data,
                  Evas        *e __UNUSED__,
                  Evas_Object *obj,
                  void        *event_info __UNUSED__)
{
   Smart_Data *sd = data;
   if (sd->still_in)
     elm_widget_focus_mouse_up_handle(obj);
   sd->still_in = EINA_FALSE;
}

static void
_propagate_x_drag_lock(Evas_Object *obj,
                       int          dir)
{
   INTERNAL_ENTRY;
   if (sd->parent_obj)
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_x_locked += dir;
             _propagate_x_drag_lock(sd->parent_obj, dir);
          }
     }
}

static void
_propagate_y_drag_lock(Evas_Object *obj,
                       int          dir)
{
   INTERNAL_ENTRY;
   if (sd->parent_obj)
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_y_locked += dir;
             _propagate_y_drag_lock(sd->parent_obj, dir);
          }
     }
}

static void
_propagate_event(void        *data,
                 Evas        *e __UNUSED__,
                 Evas_Object *obj,
                 void        *event_info)
{
   INTERNAL_ENTRY;
   Evas_Callback_Type type = (Evas_Callback_Type)(long)data;
   Evas_Event_Flags *event_flags = NULL;

   switch (type)
     {
      case EVAS_CALLBACK_KEY_DOWN:
          {
            Evas_Event_Key_Down *ev = event_info;
            event_flags = &(ev->event_flags);
          }
        break;

      case EVAS_CALLBACK_KEY_UP:
          {
             Evas_Event_Key_Up *ev = event_info;
             event_flags = &(ev->event_flags);
          }
        break;

      case EVAS_CALLBACK_MOUSE_WHEEL:
          {
            Evas_Event_Mouse_Wheel *ev = event_info;
            event_flags = &(ev->event_flags);
          }
        break;

      default:
        break;
     }

   elm_widget_event_propagate(obj, type, event_info, event_flags);
}

static void
_parent_focus(Evas_Object *obj)
{
   API_ENTRY return;
   if (sd->focused) return;

   Evas_Object *o = elm_widget_parent_get(obj);
   sd->focus_order_on_calc = EINA_TRUE;

   if (o) _parent_focus(o);

   if (!sd->focus_order_on_calc)
     return; /* we don't want to override it if by means of any of the
                callbacks below one gets to calculate our order
                first. */

   focus_order++;
   sd->focus_order = focus_order;
   if (sd->top_win_focused)
     {
        sd->focused = EINA_TRUE;
        if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
        if (sd->focus_func) sd->focus_func(obj);
        _elm_widget_focus_region_show(obj);
     }
   sd->focus_order_on_calc = EINA_FALSE;
}

static void
_elm_object_focus_chain_del_cb(void        *data,
                               Evas        *e __UNUSED__,
                               Evas_Object *obj,
                               void        *event_info __UNUSED__)
{
   Smart_Data *sd = data;

   sd->focus_chain = eina_list_remove(sd->focus_chain, obj);
}

// exposed util funcs to elm
void
_elm_widget_type_clear(void)
{
   const char **ptr;

   EINA_LIST_FREE(widtypes, ptr)
     {
        eina_stringshare_del(*ptr);
        *ptr = NULL;
     }
}

void
_elm_widget_focus_region_show(const Evas_Object *obj)
{
   Evas_Coord x, y, w, h, ox, oy;
   Smart_Data *sd2;
   Evas_Object *o;

   API_ENTRY return;

   o = elm_widget_parent_get(obj);
   if (!o) return;

   elm_widget_focus_region_get(obj, &x, &y, &w, &h);
   evas_object_geometry_get(obj, &ox, &oy, NULL, NULL);
   while (o)
     {
        Evas_Coord px, py;
        sd2 = evas_object_smart_data_get(o);
        if (sd2->focus_region_func)
          {
             sd2->focus_region_func(o, x, y, w, h);
             elm_widget_focus_region_get(o, &x, &y, &w, &h);
          }
        else
          {
             evas_object_geometry_get(o, &px, &py, NULL, NULL);
             x += ox - px;
             y += oy - py;
             ox = px;
             oy = py;
          }
        o = elm_widget_parent_get(o);
     }
}

/**
 * @defgroup Widget Widget
 *
 * @internal
 * Exposed api for making widgets
 */
EAPI void
elm_widget_type_register(const char **ptr)
{
   widtypes = eina_list_append(widtypes, (void *)ptr);
}

/**
 * @defgroup Widget Widget
 *
 * @internal
 * Disposed api for making widgets
 */
EAPI void
elm_widget_type_unregister(const char **ptr)
{
   widtypes = eina_list_remove(widtypes, (void *)ptr);
}

EAPI Eina_Bool
elm_widget_api_check(int ver)
{
   if (ver != ELM_INTERNAL_API_VERSION)
     {
        CRITICAL("Elementary widget api versions do not match");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EAPI Evas_Object *
elm_widget_add(Evas *evas)
{
   Evas_Object *obj;
   _smart_init();
   obj = evas_object_smart_add(evas, _e_smart);
   elm_widget_mirrored_set(obj, elm_config_mirrored_get());
   return obj;
}

EAPI void
elm_widget_del_hook_set(Evas_Object *obj,
                        void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->del_func = func;
}

EAPI void
elm_widget_del_pre_hook_set(Evas_Object *obj,
                            void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->del_pre_func = func;
}

EAPI void
elm_widget_focus_hook_set(Evas_Object *obj,
                          void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->focus_func = func;
}

EAPI void
elm_widget_activate_hook_set(Evas_Object *obj,
                             void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->activate_func = func;
}

EAPI void
elm_widget_disable_hook_set(Evas_Object *obj,
                            void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->disable_func = func;
}

EAPI void
elm_widget_theme_hook_set(Evas_Object *obj,
                          void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->theme_func = func;
}

EAPI void
elm_widget_translate_hook_set(Evas_Object *obj,
                              void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->translate_func = func;
}

EAPI void
elm_widget_event_hook_set(Evas_Object *obj,
                          Eina_Bool  (*func)(Evas_Object       *obj,
                                             Evas_Object       *source,
                                             Evas_Callback_Type type,
                                             void              *event_info))
{
   API_ENTRY return;
   sd->event_func = func;
}

EAPI void
elm_widget_text_set_hook_set(Evas_Object *obj,
                             Elm_Widget_Text_Set_Cb func)
{
   API_ENTRY return;
   sd->text_set_func = func;
}

EAPI void
elm_widget_text_get_hook_set(Evas_Object *obj,
                             Elm_Widget_Text_Get_Cb func)
{
   API_ENTRY return;
   sd->text_get_func = func;
}

EAPI void
elm_widget_content_set_hook_set(Evas_Object *obj,
                                Elm_Widget_Content_Set_Cb func)
{
   API_ENTRY return;
   sd->content_set_func = func;
}

EAPI void
elm_widget_content_get_hook_set(Evas_Object *obj,
                                Elm_Widget_Content_Get_Cb func)
{
   API_ENTRY return;
   sd->content_get_func = func;
}

EAPI void
elm_widget_content_unset_hook_set(Evas_Object *obj,
                                  Elm_Widget_Content_Unset_Cb func)
{
   API_ENTRY return;
   sd->content_unset_func = func;
}

EAPI void
elm_widget_changed_hook_set(Evas_Object *obj,
                            void       (*func)(Evas_Object *obj))
{
   API_ENTRY return;
   sd->changed_func = func;
}

EAPI void
elm_widget_signal_emit_hook_set(Evas_Object *obj,
                                void       (*func)(Evas_Object *obj,
                                                   const char *emission,
                                                   const char *source))
{
   API_ENTRY return;
   sd->signal_func = func;
}

EAPI void
elm_widget_signal_callback_add_hook_set(Evas_Object *obj,
                                        void       (*func)(Evas_Object   *obj,
                                                           const char    *emission,
                                                           const char    *source,
                                                           Edje_Signal_Cb func_cb,
                                                           void          *data))
{
   API_ENTRY return;
   sd->callback_add_func = func;
}

EAPI void
elm_widget_signal_callback_del_hook_set(Evas_Object *obj,
                                        void       (*func)(Evas_Object   *obj,
                                                           const char    *emission,
                                                           const char    *source,
                                                           Edje_Signal_Cb func_cb,
                                                           void          *data))
{
   API_ENTRY return;
   sd->callback_del_func = func;
}

EAPI Eina_Bool
elm_widget_theme(Evas_Object *obj)
{
   const Eina_List *l;
   Evas_Object *child;
   Elm_Tooltip *tt;
   Elm_Cursor *cur;
   Eina_Bool ret = EINA_TRUE;

   API_ENTRY return EINA_FALSE;
   EINA_LIST_FOREACH(sd->subobjs, l, child) ret &= elm_widget_theme(child);
   if (sd->resize_obj && _elm_widget_is(sd->resize_obj))
     ret &= elm_widget_theme(sd->resize_obj);
   if (sd->hover_obj) ret &= elm_widget_theme(sd->hover_obj);
   EINA_LIST_FOREACH(sd->tooltips, l, tt) elm_tooltip_theme(tt);
   EINA_LIST_FOREACH(sd->cursors, l, cur) elm_cursor_theme(cur);
   if (sd->theme_func) sd->theme_func(obj);

   return ret;
}

EAPI void
elm_widget_theme_specific(Evas_Object *obj,
                          Elm_Theme   *th,
                          Eina_Bool    force)
{
   const Eina_List *l;
   Evas_Object *child;
   Elm_Tooltip *tt;
   Elm_Cursor *cur;
   Elm_Theme *th2, *thdef;

   API_ENTRY return;
   thdef = elm_theme_default_get();
   if (!th) th = thdef;
   if (!force)
     {
        th2 = sd->theme;
        if (!th2) th2 = thdef;
        while (th2)
          {
             if (th2 == th)
               {
                  force = EINA_TRUE;
                  break;
               }
             if (th2 == thdef) break;
             th2 = th2->ref_theme;
             if (!th2) th2 = thdef;
          }
     }
   if (!force) return;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     elm_widget_theme_specific(child, th, force);
   if (sd->resize_obj) elm_widget_theme(sd->resize_obj);
   if (sd->hover_obj) elm_widget_theme(sd->hover_obj);
   EINA_LIST_FOREACH(sd->tooltips, l, tt) elm_tooltip_theme(tt);
   EINA_LIST_FOREACH(sd->cursors, l, cur) elm_cursor_theme(cur);
   if (sd->theme_func) sd->theme_func(obj);
}

/**
 * @internal
 *
 * Set hook to get next object in object focus chain.
 *
 * @param obj The widget object.
 * @param func The hook to be used with this widget.
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_next_hook_set(Evas_Object *obj,
                               Eina_Bool  (*func)(const Evas_Object   *obj,
                                                   Elm_Focus_Direction dir,
                                                   Evas_Object       **next))
{
   API_ENTRY return;
   sd->focus_next_func = func;
}

/**
 * Returns the widget's mirrored mode.
 *
 * @param obj The widget.
 * @return mirrored mode of the object.
 *
 **/
EAPI Eina_Bool
elm_widget_mirrored_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->is_mirrored;
}

/**
 * Sets the widget's mirrored mode.
 *
 * @param obj The widget.
 * @param mirrored EINA_TRUE to set mirrored mode. EINA_FALSE to unset.
 */
EAPI void
elm_widget_mirrored_set(Evas_Object *obj,
                        Eina_Bool    mirrored)
{
   API_ENTRY return;
   if (sd->is_mirrored != mirrored)
     {
        sd->is_mirrored = mirrored;
        elm_widget_theme(obj);
     }
}

/**
 * @internal
 * Resets the mirrored mode from the system mirror mode for widgets that are in
 * automatic mirroring mode. This function does not call elm_widget_theme.
 *
 * @param obj The widget.
 * @param mirrored EINA_TRUE to set mirrored mode. EINA_FALSE to unset.
 */
void
_elm_widget_mirrored_reload(Evas_Object *obj)
{
   API_ENTRY return;
   Eina_Bool mirrored = elm_config_mirrored_get();
   if (elm_widget_mirrored_automatic_get(obj) && (sd->is_mirrored != mirrored))
     {
        sd->is_mirrored = mirrored;
     }
}

/**
 * Returns the widget's mirrored mode setting.
 *
 * @param obj The widget.
 * @return mirrored mode setting of the object.
 *
 **/
EAPI Eina_Bool
elm_widget_mirrored_automatic_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->mirrored_auto_mode;
}

/**
 * Sets the widget's mirrored mode setting.
 * When widget in automatic mode, it follows the system mirrored mode set by
 * elm_mirrored_set().
 * @param obj The widget.
 * @param automatic EINA_TRUE for auto mirrored mode. EINA_FALSE for manual.
 */
EAPI void
elm_widget_mirrored_automatic_set(Evas_Object *obj,
                                  Eina_Bool    automatic)
{
   API_ENTRY return;
   if (sd->mirrored_auto_mode != automatic)
     {
        sd->mirrored_auto_mode = automatic;

        if (automatic)
          {
             elm_widget_mirrored_set(obj, elm_config_mirrored_get());
          }
     }
}

EAPI void
elm_widget_on_focus_hook_set(Evas_Object *obj,
                             void       (*func)(void *data,
                                                Evas_Object *obj),
                             void        *data)
{
   API_ENTRY return;
   sd->on_focus_func = func;
   sd->on_focus_data = data;
}

EAPI void
elm_widget_on_change_hook_set(Evas_Object *obj,
                              void       (*func)(void *data,
                                                 Evas_Object *obj),
                              void        *data)
{
   API_ENTRY return;
   sd->on_change_func = func;
   sd->on_change_data = data;
}

EAPI void
elm_widget_on_show_region_hook_set(Evas_Object *obj,
                                   void       (*func)(void *data,
                                                      Evas_Object *obj),
                                   void        *data)
{
   API_ENTRY return;
   sd->on_show_region_func = func;
   sd->on_show_region_data = data;
}

/**
 * @internal
 *
 * Set the hook to use to show the focused region.
 *
 * Whenever a new widget gets focused or it's needed to show the focused
 * area of the current one, this hook will be called on objects that may
 * want to move their children into their visible area.
 * The area given in the hook function is relative to the @p obj widget.
 *
 * @param obj The widget object
 * @param func The function to call to show the specified area.
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_region_hook_set(Evas_Object *obj,
                                 void       (*func)(Evas_Object *obj,
                                                    Evas_Coord x,
                                                    Evas_Coord y,
                                                    Evas_Coord w,
                                                    Evas_Coord h))
{
   API_ENTRY return;
   sd->focus_region_func = func;
}

/**
 * @internal
 *
 * Set the hook to retrieve the focused region of a widget.
 *
 * This hook will be called by elm_widget_focus_region_get() whenever
 * it's needed to get the focused area of a widget. The area must be relative
 * to the widget itself and if no hook is set, it will default to the entire
 * object.
 *
 * @param obj The widget object
 * @param func The function used to retrieve the focus region.
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_on_focus_region_hook_set(Evas_Object *obj,
                                    void       (*func)(const Evas_Object *obj,
                                                       Evas_Coord *x,
                                                       Evas_Coord *y,
                                                       Evas_Coord *w,
                                                       Evas_Coord *h))
{
   API_ENTRY return;
   sd->on_focus_region_func = func;
}

EAPI void
elm_widget_data_set(Evas_Object *obj,
                    void        *data)
{
   API_ENTRY return;
   sd->data = data;
}

EAPI void *
elm_widget_data_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->data;
}

EAPI void
elm_widget_sub_object_add(Evas_Object *obj,
                          Evas_Object *sobj)
{
   API_ENTRY return;
   double scale, pscale = elm_widget_scale_get(sobj);
   Elm_Theme *th, *pth = elm_widget_theme_get(sobj);
   Eina_Bool mirrored, pmirrored = elm_widget_mirrored_get(obj);

   if (sobj == sd->parent_obj)
     {
        elm_widget_sub_object_del(sobj, obj);
        WRN("You passed a parent object of obj = %p as the sub object = %p!", obj, sobj);
     }

   if (_elm_widget_is(sobj))
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sobj);
        if (sd2)
          {
             if (sd2->parent_obj == obj)
               return;
             if (sd2->parent_obj)
               elm_widget_sub_object_del(sd2->parent_obj, sobj);
             sd2->parent_obj = obj;
             _elm_widget_top_win_focused_set(sobj, sd->top_win_focused);
             if (!sd->child_can_focus && (_is_focusable(sobj)))
               sd->child_can_focus = EINA_TRUE;
          }
     }
   else
     {
        void *data = evas_object_data_get(sobj, "elm-parent");
        if (data)
          {
             if (data == obj) return;
             evas_object_event_callback_del(sobj, EVAS_CALLBACK_DEL,
                                            _sub_obj_del);
          }
     }
   sd->subobjs = eina_list_append(sd->subobjs, sobj);
   evas_object_data_set(sobj, "elm-parent", obj);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
   if (_elm_widget_is(sobj))
     evas_object_event_callback_add(sobj, EVAS_CALLBACK_HIDE, _sub_obj_hide, sd);
   evas_object_smart_callback_call(obj, "sub-object-add", sobj);
   scale = elm_widget_scale_get(sobj);
   th = elm_widget_theme_get(sobj);
   mirrored = elm_widget_mirrored_get(sobj);
   if ((scale != pscale) || (th != pth) || (pmirrored != mirrored)) elm_widget_theme(sobj);
   if (elm_widget_focus_get(sobj)) _focus_parents(obj);
}

EAPI void
elm_widget_sub_object_del(Evas_Object *obj,
                          Evas_Object *sobj)
{
   Evas_Object *sobj_parent;
   API_ENTRY return;
   if (!sobj) return;

   sobj_parent = evas_object_data_del(sobj, "elm-parent");
   if (sobj_parent != obj)
     {
        static int abort_on_warn = -1;
        ERR("removing sub object %p (%s) from parent %p (%s), "
            "but elm-parent is different %p (%s)!",
            sobj, elm_widget_type_get(sobj), obj, elm_widget_type_get(obj),
            sobj_parent, elm_widget_type_get(sobj_parent));
        if (EINA_UNLIKELY(abort_on_warn == -1))
          {
             if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
             else abort_on_warn = 0;
          }
        if (abort_on_warn == 1) abort();
     }
   if (_elm_widget_is(sobj))
     {
        if (elm_widget_focus_get(sobj))
          {
             elm_widget_tree_unfocusable_set(sobj, EINA_TRUE);
             elm_widget_tree_unfocusable_set(sobj, EINA_FALSE);
          }
        if ((sd->child_can_focus) && (_is_focusable(sobj)))
          {
             Evas_Object *subobj;
             const Eina_List *l;
             sd->child_can_focus = EINA_FALSE;
             EINA_LIST_FOREACH(sd->subobjs, l, subobj)
               {
                  if (_is_focusable(subobj))
                    {
                       sd->child_can_focus = EINA_TRUE;
                       break;
                    }
               }
          }
        Smart_Data *sd2 = evas_object_smart_data_get(sobj);
        if (sd2)
          {
             sd2->parent_obj = NULL;
             if (sd2->resize_obj == sobj)
               sd2->resize_obj = NULL;
             else
               sd->subobjs = eina_list_remove(sd->subobjs, sobj);
          }
        else
          sd->subobjs = eina_list_remove(sd->subobjs, sobj);
     }
   else
     sd->subobjs = eina_list_remove(sd->subobjs, sobj);
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL,
                                       _sub_obj_del, sd);
   if (_elm_widget_is(sobj))
     evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_HIDE,
                                         _sub_obj_hide, sd);
   evas_object_smart_callback_call(obj, "sub-object-del", sobj);
}

EAPI const Eina_List *
elm_widget_sub_object_list_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return (const Eina_List *)sd->subobjs;
}

EAPI void
elm_widget_resize_object_set(Evas_Object *obj,
                             Evas_Object *sobj)
{
   API_ENTRY return;
   // orphan previous resize obj
   if (sd->resize_obj)
     {
        evas_object_clip_unset(sd->resize_obj);
        evas_object_data_del(sd->resize_obj, "elm-parent");
        if (_elm_widget_is(sd->resize_obj))
          {
             Smart_Data *sd2 = evas_object_smart_data_get(sd->resize_obj);
             if (sd2) sd2->parent_obj = NULL;
             evas_object_event_callback_del_full(sd->resize_obj,
                                                 EVAS_CALLBACK_HIDE,
                                                 _sub_obj_hide, sd);
          }
        evas_object_event_callback_del_full(sd->resize_obj, EVAS_CALLBACK_DEL,
                                            _sub_obj_del, sd);
        evas_object_event_callback_del_full(sd->resize_obj,
                                            EVAS_CALLBACK_MOUSE_DOWN,
                                            _sub_obj_mouse_down, sd);
        evas_object_event_callback_del_full(sd->resize_obj,
                                            EVAS_CALLBACK_MOUSE_MOVE,
                                            _sub_obj_mouse_move, sd);
        evas_object_event_callback_del_full(sd->resize_obj,
                                            EVAS_CALLBACK_MOUSE_UP,
                                            _sub_obj_mouse_up, sd);
        evas_object_smart_member_del(sd->resize_obj);

        if (_elm_widget_is(sd->resize_obj))
          {
             if (elm_widget_focus_get(sd->resize_obj)) _unfocus_parents(obj);
          }
     }

   sd->resize_obj = sobj;
   if (!sobj) return;

   // orphan new resize obj
   evas_object_data_del(sobj, "elm-parent");
   if (_elm_widget_is(sobj))
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sobj);
        if (sd2) sd2->parent_obj = NULL;
        evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_HIDE,
                                            _sub_obj_hide, sd);
     }
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL,
                                       _sub_obj_del, sd);
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_MOUSE_DOWN,
                                       _sub_obj_mouse_down, sd);
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_MOUSE_MOVE,
                                       _sub_obj_mouse_move, sd);
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_MOUSE_UP,
                                       _sub_obj_mouse_up, sd);
   evas_object_smart_member_del(sobj);
   if (_elm_widget_is(sobj))
     {
        if (elm_widget_focus_get(sobj)) _unfocus_parents(obj);
     }

   // set the resize obj up
   if (_elm_widget_is(sobj))
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sobj);
        if (sd2)
          {
             sd2->parent_obj = obj;
             sd2->top_win_focused = sd->top_win_focused;
          }
        evas_object_event_callback_add(sobj, EVAS_CALLBACK_HIDE,
                                       _sub_obj_hide, sd);
     }
   evas_object_smart_member_add(sobj, obj);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL,
                                  _sub_obj_del, sd);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _sub_obj_mouse_down, sd);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_MOUSE_MOVE,
                                  _sub_obj_mouse_move, sd);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_MOUSE_UP,
                                  _sub_obj_mouse_up, sd);
   _smart_reconfigure(sd);
   evas_object_data_set(sobj, "elm-parent", obj);
   evas_object_smart_callback_call(obj, "sub-object-add", sobj);
   if (_elm_widget_is(sobj))
     {
        if (elm_widget_focus_get(sobj)) _focus_parents(obj);
     }
}

EAPI void
elm_widget_hover_object_set(Evas_Object *obj,
                            Evas_Object *sobj)
{
   API_ENTRY return;
   if (sd->hover_obj)
     {
        evas_object_event_callback_del_full(sd->hover_obj, EVAS_CALLBACK_DEL,
                                            _sub_obj_del, sd);
     }
   sd->hover_obj = sobj;
   if (sd->hover_obj)
     {
        evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL,
                                       _sub_obj_del, sd);
        _smart_reconfigure(sd);
     }
}

EAPI void
elm_widget_can_focus_set(Evas_Object *obj,
                         Eina_Bool    can_focus)
{
   API_ENTRY return;

   can_focus = !!can_focus;

   if (sd->can_focus == can_focus) return;
   sd->can_focus = can_focus;
   if (sd->can_focus)
     {
        evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_KEY_DOWN);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_UP,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_KEY_UP);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_MOUSE_WHEEL);
     }
   else
     {
        evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_DOWN,
                                       _propagate_event);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_UP,
                                       _propagate_event);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                       _propagate_event);
     }
}

EAPI Eina_Bool
elm_widget_can_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->can_focus;
}

EAPI Eina_Bool
elm_widget_child_can_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->child_can_focus;
}

/**
 * @internal
 *
 * This API makes the widget object and its children to be unfocusable.
 *
 * This API can be helpful for an object to be deleted.
 * When an object will be deleted soon, it and its children may not
 * want to get focus (by focus reverting or by other focus controls).
 * Then, just use this API before deleting.
 *
 * @param obj The widget root of sub-tree
 * @param tree_unfocusable If true, set the object sub-tree as unfocusable
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_tree_unfocusable_set(Evas_Object *obj,
                                Eina_Bool    tree_unfocusable)
{
   API_ENTRY return;

   tree_unfocusable = !!tree_unfocusable;
   if (sd->tree_unfocusable == tree_unfocusable) return;
   sd->tree_unfocusable = tree_unfocusable;
   elm_widget_focus_tree_unfocusable_handle(obj);
}

/**
 * @internal
 *
 * This returns true, if the object sub-tree is unfocusable.
 *
 * @param obj The widget root of sub-tree
 * @return EINA_TRUE if the object sub-tree is unfocusable
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_tree_unfocusable_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->tree_unfocusable;
}

/**
 * @internal
 *
 * Get the list of focusable child objects.
 *
 * This function retruns list of child objects which can get focus.
 *
 * @param obj The parent widget
 * @retrun list of focusable child objects.
 *
 * @ingroup Widget
 */
EAPI Eina_List *
elm_widget_can_focus_child_list_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;

   const Eina_List *l;
   Eina_List *child_list = NULL;
   Evas_Object *child;

   if (sd->subobjs)
     {
        EINA_LIST_FOREACH(sd->subobjs, l, child)
          {
             if ((elm_widget_can_focus_get(child)) &&
                 (evas_object_visible_get(child)) &&
                 (!elm_widget_disabled_get(child)))
               child_list = eina_list_append(child_list, child);
             else if (elm_widget_is(child))
               {
                  Eina_List *can_focus_list;
                  can_focus_list = elm_widget_can_focus_child_list_get(child);
                  if (can_focus_list)
                    child_list = eina_list_merge(child_list, can_focus_list);
               }
          }
     }
   return child_list;
}

EAPI void
elm_widget_highlight_ignore_set(Evas_Object *obj,
                                Eina_Bool    ignore)
{
   API_ENTRY return;
   sd->highlight_ignore = !!ignore;
}

EAPI Eina_Bool
elm_widget_highlight_ignore_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->highlight_ignore;
}

EAPI void
elm_widget_highlight_in_theme_set(Evas_Object *obj,
                                  Eina_Bool    highlight)
{
   API_ENTRY return;
   sd->highlight_in_theme = !!highlight;
   /* FIXME: if focused, it should switch from one mode to the other */
}

EAPI Eina_Bool
elm_widget_highlight_in_theme_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->highlight_in_theme;
}

EAPI Eina_Bool
elm_widget_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->focused;
}

EAPI Evas_Object *
elm_widget_focused_object_get(const Evas_Object *obj)
{
   const Evas_Object *subobj;
   const Eina_List *l;
   API_ENTRY return NULL;

   if (!sd->focused) return NULL;
   EINA_LIST_FOREACH(sd->subobjs, l, subobj)
     {
        Evas_Object *fobj = elm_widget_focused_object_get(subobj);
        if (fobj) return fobj;
     }
   return (Evas_Object *)obj;
}

EAPI Evas_Object *
elm_widget_top_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (sd->parent_obj) return elm_widget_top_get(sd->parent_obj);
   return (Evas_Object *)obj;
}

EAPI Eina_Bool
elm_widget_is(const Evas_Object *obj)
{
   return _elm_widget_is(obj);
}

EAPI Evas_Object *
elm_widget_parent_widget_get(const Evas_Object *obj)
{
   Evas_Object *parent;

   if (_elm_widget_is(obj))
     {
        Smart_Data *sd = evas_object_smart_data_get(obj);
        if (!sd) return NULL;
        parent = sd->parent_obj;
     }
   else
     {
        parent = evas_object_data_get(obj, "elm-parent");
        if (!parent) parent = evas_object_smart_parent_get(obj);
     }

   while (parent)
     {
        Evas_Object *elm_parent;
        if (_elm_widget_is(parent)) break;
        elm_parent = evas_object_data_get(parent, "elm-parent");
        if (elm_parent) parent = elm_parent;
        else parent = evas_object_smart_parent_get(parent);
     }
   return parent;
}

EAPI Evas_Object *
elm_widget_parent2_get(const Evas_Object *obj)
{
   if (_elm_widget_is(obj))
     {
        Smart_Data *sd = evas_object_smart_data_get(obj);
        if (sd) return sd->parent2;
     }
   return NULL;
}

EAPI void
elm_widget_parent2_set(Evas_Object *obj, Evas_Object *parent)
{
   API_ENTRY return;
   sd->parent2 = parent;
}

EAPI void
elm_widget_event_callback_add(Evas_Object *obj,
                              Elm_Event_Cb func,
                              const void  *data)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(func);
   Elm_Event_Cb_Data *ecb = ELM_NEW(Elm_Event_Cb_Data);
   ecb->func = func;
   ecb->data = data;
   sd->event_cb = eina_list_append(sd->event_cb, ecb);
}

EAPI void *
elm_widget_event_callback_del(Evas_Object *obj,
                              Elm_Event_Cb func,
                              const void  *data)
{
   API_ENTRY return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);
   Eina_List *l;
   Elm_Event_Cb_Data *ecd;
   EINA_LIST_FOREACH(sd->event_cb, l, ecd)
     if ((ecd->func == func) && (ecd->data == data))
       {
          free(ecd);
          sd->event_cb = eina_list_remove_list(sd->event_cb, l);
          return (void *)data;
       }
   return NULL;
}

EAPI Eina_Bool
elm_widget_event_propagate(Evas_Object       *obj,
                           Evas_Callback_Type type,
                           void              *event_info,
                           Evas_Event_Flags  *event_flags)
{
   API_ENTRY return EINA_FALSE; //TODO reduce.
   if (!_elm_widget_is(obj)) return EINA_FALSE;
   Evas_Object *parent = obj;
   Elm_Event_Cb_Data *ecd;
   Eina_List *l, *l_prev;

   while (parent &&
          (!(event_flags && ((*event_flags) & EVAS_EVENT_FLAG_ON_HOLD))))
     {
        sd = evas_object_smart_data_get(parent);
        if ((!sd) || (!_elm_widget_is(obj)))
          return EINA_FALSE; //Not Elm Widget

        if (sd->event_func && (sd->event_func(parent, obj, type, event_info)))
          return EINA_TRUE;

        EINA_LIST_FOREACH_SAFE(sd->event_cb, l, l_prev, ecd)
          {
             if (ecd->func((void *)ecd->data, parent, obj, type, event_info) ||
                 (event_flags && ((*event_flags) & EVAS_EVENT_FLAG_ON_HOLD)))
               return EINA_TRUE;
          }
        parent = sd->parent_obj;
     }

   return EINA_FALSE;
}

/**
 * @internal
 *
 * Set custom focus chain.
 *
 * This function i set one new and overwrite any previous custom focus chain
 * with the list of objects. The previous list will be deleted and this list
 * will be managed. After setted, don't modity it.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param objs Chain of objects to pass focus
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_set(Evas_Object *obj,
                                  Eina_List   *objs)
{
   API_ENTRY return;
   if (!sd->focus_next_func)
     return;

   elm_widget_focus_custom_chain_unset(obj);

   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(objs, l, o)
     {
        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);
     }

   sd->focus_chain = objs;
}

/**
 * @internal
 *
 * Get custom focus chain
 *
 * @param obj The container widget
 * @ingroup Widget
 */
EAPI const Eina_List *
elm_widget_focus_custom_chain_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return (const Eina_List *)sd->focus_chain;
}

/**
 * @internal
 *
 * Unset custom focus chain
 *
 * @param obj The container widget
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_unset(Evas_Object *obj)
{
   API_ENTRY return;
   Eina_List *l, *l_next;
   Evas_Object *o;

   EINA_LIST_FOREACH_SAFE(sd->focus_chain, l, l_next, o)
     {
        evas_object_event_callback_del_full(o, EVAS_CALLBACK_DEL,
                                            _elm_object_focus_chain_del_cb, sd);
        sd->focus_chain = eina_list_remove_list(sd->focus_chain, l);
     }
}

/**
 * @internal
 *
 * Append object to custom focus chain.
 *
 * @note If relative_child equal to NULL or not in custom chain, the object
 * will be added in end.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param child The child to be added in custom chain
 * @param relative_child The relative object to position the child
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_append(Evas_Object *obj,
                                     Evas_Object *child,
                                     Evas_Object *relative_child)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(child);
   if (!sd->focus_next_func) return;

   evas_object_event_callback_del_full(child, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);

   if (!relative_child)
     sd->focus_chain = eina_list_append(sd->focus_chain, child);
   else
     sd->focus_chain = eina_list_append_relative(sd->focus_chain,
                                                 child, relative_child);
}

/**
 * @internal
 *
 * Prepend object to custom focus chain.
 *
 * @note If relative_child equal to NULL or not in custom chain, the object
 * will be added in begin.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param child The child to be added in custom chain
 * @param relative_child The relative object to position the child
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_prepend(Evas_Object *obj,
                                      Evas_Object *child,
                                      Evas_Object *relative_child)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(child);

   if (!sd->focus_next_func) return;

   evas_object_event_callback_del_full(child, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);

   if (!relative_child)
     sd->focus_chain = eina_list_prepend(sd->focus_chain, child);
   else
     sd->focus_chain = eina_list_prepend_relative(sd->focus_chain,
                                                  child, relative_child);
}

/**
 * @internal
 *
 * Give focus to next object in object tree.
 *
 * Give focus to next object in focus chain of one object sub-tree.
 * If the last object of chain already have focus, the focus will go to the
 * first object of chain.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction to cycle the focus
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_cycle(Evas_Object        *obj,
                       Elm_Focus_Direction dir)
{
   Evas_Object *target = NULL;
   if (!_elm_widget_is(obj))
     return;
   elm_widget_focus_next_get(obj, dir, &target);
   if (target)
     elm_widget_focus_steal(target);
}

/**
 * @internal
 *
 * Give focus to near object in one direction.
 *
 * Give focus to near object in direction of one object.
 * If none focusable object in given direction, the focus will not change.
 *
 * @param obj The reference widget
 * @param x Horizontal component of direction to focus
 * @param y Vertical component of direction to focus
 *
 * @ingroup Widget
 */
//FIXME: If x, y indicates the elements of the directional vector,
//It would be better if these values are the normalized value(float x, float y)
//or degree.
EINA_DEPRECATED EAPI void
elm_widget_focus_direction_go(Evas_Object *obj __UNUSED__,
                              int          x __UNUSED__,
                              int          y __UNUSED__)
{
   return; /* TODO */
}

/**
 * @internal
 *
 * Get next object in focus chain of object tree.
 *
 * Get next object in focus chain of one object sub-tree.
 * Return the next object by reference. If don't have any candidate to receive
 * focus before chain end, the first candidate will be returned.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction os focus chain
 * @param next The next object in focus chain
 * @return EINA_TRUE if don't need focus chain restart/loop back
 *         to use 'next' obj.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_next_get(const Evas_Object  *obj,
                          Elm_Focus_Direction dir,
                          Evas_Object       **next)
{
   if (!next)
     return EINA_FALSE;
   *next = NULL;

   API_ENTRY return EINA_FALSE;

   /* Ignore if disabled */
   if ((!evas_object_visible_get(obj))
       || (elm_widget_disabled_get(obj))
       || (elm_widget_tree_unfocusable_get(obj)))
     return EINA_FALSE;

   /* Try use hook */
   if (sd->focus_next_func)
     return sd->focus_next_func(obj, dir, next);

   if (!elm_widget_can_focus_get(obj))
     return EINA_FALSE;

   /* Return */
   *next = (Evas_Object *)obj;
   return !elm_widget_focus_get(obj);
}

/**
 * @internal
 *
 * Get next object in focus chain of object tree in list.
 *
 * Get next object in focus chain of one object sub-tree ordered by one list.
 * Return the next object by reference. If don't have any candidate to receive
 * focus before list end, the first candidate will be returned.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction os focus chain
 * @param items list with ordered objects
 * @param list_data_get function to get the object from one item of list
 * @param next The next object in focus chain
 * @return EINA_TRUE if don't need focus chain restart/loop back
 *         to use 'next' obj.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_list_next_get(const Evas_Object  *obj,
                               const Eina_List    *items,
                               void *(*list_data_get)(const Eina_List * list),
                               Elm_Focus_Direction dir,
                               Evas_Object       **next)
{
   Eina_List *(*list_next)(const Eina_List * list) = NULL;

   if (!next)
     return EINA_FALSE;
   *next = NULL;

   if (!_elm_widget_is(obj))
     return EINA_FALSE;

   if (!items)
     return EINA_FALSE;

   /* Direction */
   if (dir == ELM_FOCUS_PREVIOUS)
     {
        items = eina_list_last(items);
        list_next = eina_list_prev;
     }
   else if (dir == ELM_FOCUS_NEXT)
     list_next = eina_list_next;
   else
     return EINA_FALSE;

   const Eina_List *l = items;

   /* Recovery last focused sub item */
   if (elm_widget_focus_get(obj))
     for (; l; l = list_next(l))
       {
          Evas_Object *cur = list_data_get(l);
          if (elm_widget_focus_get(cur)) break;
       }

   const Eina_List *start = l;
   Evas_Object *to_focus = NULL;

   /* Interate sub items */
   /* Go to end of list */
   for (; l; l = list_next(l))
     {
        Evas_Object *tmp = NULL;
        Evas_Object *cur = list_data_get(l);

        if (elm_widget_parent_get(cur) != obj)
          continue;

        /* Try Focus cycle in subitem */
        if (elm_widget_focus_next_get(cur, dir, &tmp))
          {
             *next = tmp;
             return EINA_TRUE;
          }
        else if ((tmp) && (!to_focus))
          to_focus = tmp;
     }

   l = items;

   /* Get First possible */
   for (; l != start; l = list_next(l))
     {
        Evas_Object *tmp = NULL;
        Evas_Object *cur = list_data_get(l);

        if (elm_widget_parent_get(cur) != obj)
          continue;

        /* Try Focus cycle in subitem */
        elm_widget_focus_next_get(cur, dir, &tmp);
        if (tmp)
          {
             *next = tmp;
             return EINA_FALSE;
          }
     }

   *next = to_focus;
   return EINA_FALSE;
}

EAPI void
elm_widget_signal_emit(Evas_Object *obj,
                       const char  *emission,
                       const char  *source)
{
   API_ENTRY return;
   if (!sd->signal_func) return;
   sd->signal_func(obj, emission, source);
}

static void
_edje_signal_callback(void        *data,
                      Evas_Object *obj __UNUSED__,
                      const char  *emission,
                      const char  *source)
{
   Edje_Signal_Data *esd = data;
   esd->func(esd->data, esd->obj, emission, source);
}

EAPI void
elm_widget_signal_callback_add(Evas_Object   *obj,
                               const char    *emission,
                               const char    *source,
                               Edje_Signal_Cb func,
                               void          *data)
{
   Edje_Signal_Data *esd;
   API_ENTRY return;
   if (!sd->callback_add_func) return;
   EINA_SAFETY_ON_NULL_RETURN(func);

   esd = ELM_NEW(Edje_Signal_Data);
   if (!esd) return;

   esd->obj = obj;
   esd->func = func;
   esd->emission = eina_stringshare_add(emission);
   esd->source = eina_stringshare_add(source);
   esd->data = data;
   sd->edje_signals = eina_list_append(sd->edje_signals, esd);
   sd->callback_add_func(obj, emission, source, _edje_signal_callback, esd);
}

EAPI void *
elm_widget_signal_callback_del(Evas_Object   *obj,
                               const char    *emission,
                               const char    *source,
                               Edje_Signal_Cb func)
{
   Edje_Signal_Data *esd;
   Eina_List *l;
   void *data = NULL;
   API_ENTRY return NULL;
   if (!sd->callback_del_func) return NULL;

   EINA_LIST_FOREACH(sd->edje_signals, l, esd)
     {
        if ((esd->func == func) && (!strcmp(esd->emission, emission)) &&
            (!strcmp(esd->source, source)))
          {
             sd->edje_signals = eina_list_remove_list(sd->edje_signals, l);
             eina_stringshare_del(esd->emission);
             eina_stringshare_del(esd->source);
             data = esd->data;
             free(esd);

             sd->callback_del_func
               (obj, emission, source, _edje_signal_callback, esd);
             return data;
          }
     }

   return data;
}

EAPI void
elm_widget_focus_set(Evas_Object *obj,
                     int          first)
{
   API_ENTRY return;
   if (!sd->focused)
     {
        focus_order++;
        sd->focus_order = focus_order;
        sd->focused = EINA_TRUE;
        if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
     }
   if (sd->focus_func)
     {
        sd->focus_func(obj);
        return;
     }
   else
     {
        if (first)
          {
             if ((_is_focusable(sd->resize_obj)) &&
                 (!elm_widget_disabled_get(sd->resize_obj)))
               {
                  elm_widget_focus_set(sd->resize_obj, first);
               }
             else
               {
                  const Eina_List *l;
                  Evas_Object *child;
                  EINA_LIST_FOREACH(sd->subobjs, l, child)
                    {
                       if ((_is_focusable(child)) &&
                           (!elm_widget_disabled_get(child)))
                         {
                            elm_widget_focus_set(child, first);
                            break;
                         }
                    }
               }
          }
        else
          {
             const Eina_List *l;
             Evas_Object *child;
             EINA_LIST_REVERSE_FOREACH(sd->subobjs, l, child)
               {
                  if ((_is_focusable(child)) &&
                      (!elm_widget_disabled_get(child)))
                    {
                       elm_widget_focus_set(child, first);
                       break;
                    }
               }
             if (!l)
               {
                  if ((_is_focusable(sd->resize_obj)) &&
                      (!elm_widget_disabled_get(sd->resize_obj)))
                    {
                       elm_widget_focus_set(sd->resize_obj, first);
                    }
               }
          }
     }
}

EAPI Evas_Object *
elm_widget_parent_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->parent_obj;
}

EAPI void
elm_widget_focused_object_clear(Evas_Object *obj)
{
   API_ENTRY return;
   if (!sd->focused) return;
   if (sd->resize_obj && elm_widget_focus_get(sd->resize_obj))
     elm_widget_focused_object_clear(sd->resize_obj);
   else
     {
        const Eina_List *l;
        Evas_Object *child;
        EINA_LIST_FOREACH(sd->subobjs, l, child)
          {
             if (elm_widget_focus_get(child))
               {
                  elm_widget_focused_object_clear(child);
                  break;
               }
          }
     }
   sd->focused = EINA_FALSE;
   if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
   if (sd->focus_func) sd->focus_func(obj);
}

EAPI void
elm_widget_focus_steal(Evas_Object *obj)
{
   Evas_Object *parent, *parent2, *o;
   API_ENTRY return;

   if (sd->focused) return;
   if (sd->disabled) return;
   if (!sd->can_focus) return;
   if (sd->tree_unfocusable) return;
   parent = obj;
   for (;;)
     {
        o = elm_widget_parent_get(parent);
        if (!o) break;
        sd = evas_object_smart_data_get(o);
        if (sd->disabled || sd->tree_unfocusable) return;
        if (sd->focused) break;
        parent = o;
     }
   if ((!elm_widget_parent_get(parent)) &&
       (!elm_widget_parent2_get(parent)))
      elm_widget_focused_object_clear(parent);
   else
     {
        parent2 = elm_widget_parent_get(parent);
        if (!parent2) parent2 = elm_widget_parent2_get(parent);
        parent = parent2;
        sd = evas_object_smart_data_get(parent);
        if (sd)
          {
             if ((sd->resize_obj) && (elm_widget_focus_get(sd->resize_obj)))
                elm_widget_focused_object_clear(sd->resize_obj);
             else
               {
                  const Eina_List *l;
                  Evas_Object *child;
                  EINA_LIST_FOREACH(sd->subobjs, l, child)
                    {
                       if (elm_widget_focus_get(child))
                         {
                            elm_widget_focused_object_clear(child);
                            break;
                         }
                    }
               }
          }
     }
   _parent_focus(obj);
   return;
}

EAPI void
elm_widget_focus_restore(Evas_Object *obj)
{
   Evas_Object *newest = NULL;
   unsigned int newest_focus_order = 0;
   API_ENTRY return;

   newest = _newest_focus_order_get(obj, &newest_focus_order, EINA_TRUE);
   if (newest)
     {
        elm_object_focus_set(newest, EINA_FALSE);
        elm_object_focus_set(newest, EINA_TRUE);
     }
}

void
_elm_widget_top_win_focused_set(Evas_Object *obj, Eina_Bool top_win_focused)
{
   const Eina_List *l;
   Evas_Object *child;
   API_ENTRY return;

   if (sd->top_win_focused == top_win_focused) return;
   if (sd->resize_obj)
     _elm_widget_top_win_focused_set(sd->resize_obj, top_win_focused);
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        _elm_widget_top_win_focused_set(child, top_win_focused);
     }
   sd->top_win_focused = top_win_focused;
}

Eina_Bool
_elm_widget_top_win_focused_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->top_win_focused;
}

EAPI void
elm_widget_activate(Evas_Object *obj)
{
   API_ENTRY return;
   elm_widget_change(obj);
   if (sd->activate_func) sd->activate_func(obj);
}

EAPI void
elm_widget_change(Evas_Object *obj)
{
   API_ENTRY return;
   elm_widget_change(elm_widget_parent_get(obj));
   if (sd->on_change_func) sd->on_change_func(sd->on_change_data, obj);
}

EAPI void
elm_widget_disabled_set(Evas_Object *obj,
                        Eina_Bool    disabled)
{
   API_ENTRY return;

   if (sd->disabled == disabled) return;
   sd->disabled = !!disabled;
   elm_widget_focus_disabled_handle(obj);
   if (sd->disable_func) sd->disable_func(obj);
}

EAPI Eina_Bool
elm_widget_disabled_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->disabled;
}

EAPI void
elm_widget_show_region_set(Evas_Object *obj,
                           Evas_Coord   x,
                           Evas_Coord   y,
                           Evas_Coord   w,
                           Evas_Coord   h,
                           Eina_Bool    forceshow)
{
   Evas_Object *parent_obj, *child_obj;
   Evas_Coord px, py, cx, cy;

   API_ENTRY return;

   evas_smart_objects_calculate(evas_object_evas_get(obj));

   if (!forceshow && (x == sd->rx) && (y == sd->ry) &&
       (w == sd->rw) && (h == sd->rh)) return;
   sd->rx = x;
   sd->ry = y;
   sd->rw = w;
   sd->rh = h;
   if (sd->on_show_region_func)
     sd->on_show_region_func(sd->on_show_region_data, obj);

   do
     {
        parent_obj = sd->parent_obj;
        child_obj = sd->obj;
        if ((!parent_obj) || (!_elm_widget_is(parent_obj))) break;
        sd = evas_object_smart_data_get(parent_obj);
        if (!sd) break;

        evas_object_geometry_get(parent_obj, &px, &py, NULL, NULL);
        evas_object_geometry_get(child_obj, &cx, &cy, NULL, NULL);

        x += (cx - px);
        y += (cy - py);
        sd->rx = x;
        sd->ry = y;
        sd->rw = w;
        sd->rh = h;

        if (sd->on_show_region_func)
          {
             sd->on_show_region_func(sd->on_show_region_data, parent_obj);
          }
     }
   while (parent_obj);
}

EAPI void
elm_widget_show_region_get(const Evas_Object *obj,
                           Evas_Coord        *x,
                           Evas_Coord        *y,
                           Evas_Coord        *w,
                           Evas_Coord        *h)
{
   API_ENTRY return;
   if (x) *x = sd->rx;
   if (y) *y = sd->ry;
   if (w) *w = sd->rw;
   if (h) *h = sd->rh;
}

/**
 * @internal
 *
 * Get the focus region of the given widget.
 *
 * The focus region is the area of a widget that should brought into the
 * visible area when the widget is focused. Mostly used to show the part of
 * an entry where the cursor is, for example. The area returned is relative
 * to the object @p obj.
 * If the @p obj doesn't have the proper on_focus_region_hook set, this
 * function will return the full size of the object.
 *
 * @param obj The widget object
 * @param x Where to store the x coordinate of the area
 * @param y Where to store the y coordinate of the area
 * @param w Where to store the width of the area
 * @param h Where to store the height of the area
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_region_get(const Evas_Object *obj,
                            Evas_Coord        *x,
                            Evas_Coord        *y,
                            Evas_Coord        *w,
                            Evas_Coord        *h)
{
   Smart_Data *sd;

   if (!obj) return;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !_elm_widget_is(obj) || !sd->on_focus_region_func)
     {
        evas_object_geometry_get(obj, NULL, NULL, w, h);
        if (x) *x = 0;
        if (y) *y = 0;
        return;
     }
   sd->on_focus_region_func(obj, x, y, w, h);
}

EAPI void
elm_widget_scroll_hold_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold++;
   if (sd->scroll_hold == 1)
     evas_object_smart_callback_call(obj, "scroll-hold-on", obj);
   if (sd->parent_obj) elm_widget_scroll_hold_push(sd->parent_obj);
   // FIXME: on delete/reparent hold pop
}

EAPI void
elm_widget_scroll_hold_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold--;
   if (!sd->scroll_hold)
     evas_object_smart_callback_call(obj, "scroll-hold-off", obj);
   if (sd->parent_obj) elm_widget_scroll_hold_pop(sd->parent_obj);
   if (sd->scroll_hold < 0) sd->scroll_hold = 0;
}

EAPI int
elm_widget_scroll_hold_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_hold;
}

EAPI void
elm_widget_scroll_freeze_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze++;
   if (sd->scroll_freeze == 1)
     evas_object_smart_callback_call(obj, "scroll-freeze-on", obj);
   if (sd->parent_obj) elm_widget_scroll_freeze_push(sd->parent_obj);
   // FIXME: on delete/reparent freeze pop
}

EAPI void
elm_widget_scroll_freeze_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze--;
   if (!sd->scroll_freeze)
     evas_object_smart_callback_call(obj, "scroll-freeze-off", obj);
   if (sd->parent_obj) elm_widget_scroll_freeze_pop(sd->parent_obj);
   if (sd->scroll_freeze < 0) sd->scroll_freeze = 0;
}

EAPI int
elm_widget_scroll_freeze_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_freeze;
}

EAPI void
elm_widget_scale_set(Evas_Object *obj,
                     double       scale)
{
   API_ENTRY return;
   if (scale <= 0.0) scale = 0.0;
   if (sd->scale != scale)
     {
        sd->scale = scale;
        elm_widget_theme(obj);
     }
}

EAPI double
elm_widget_scale_get(const Evas_Object *obj)
{
   API_ENTRY return 1.0;
   // FIXME: save walking up the tree by storing/caching parent scale
   if (sd->scale == 0.0)
     {
        if (sd->parent_obj)
          return elm_widget_scale_get(sd->parent_obj);
        else
          return 1.0;
     }
   return sd->scale;
}

EAPI void
elm_widget_theme_set(Evas_Object *obj,
                     Elm_Theme   *th)
{
   API_ENTRY return;
   if (sd->theme != th)
     {
        if (sd->theme) elm_theme_free(sd->theme);
        sd->theme = th;
        if (th) th->ref++;
        elm_widget_theme(obj);
     }
}

EAPI void
elm_widget_text_part_set(Evas_Object *obj, const char *part, const char *label)
{
   API_ENTRY return;

   if (!sd->text_set_func)
     return;

   sd->text_set_func(obj, part, label);
}

EAPI const char *
elm_widget_text_part_get(const Evas_Object *obj, const char *part)
{
   API_ENTRY return NULL;

   if (!sd->text_get_func)
     return NULL;

   return sd->text_get_func(obj, part);
}

EAPI void
elm_widget_domain_translatable_text_part_set(Evas_Object *obj, const char *part, const char *domain, const char *label)
{
   const char *str;
   Eina_List *l;
   Elm_Translate_String_Data *ts = NULL;
   API_ENTRY return;

   str = eina_stringshare_add(part);
   EINA_LIST_FOREACH(sd->translate_strings, l, ts)
      if (ts->id == str)
        break;
      else
        ts = NULL;

   if (!ts && !label)
     eina_stringshare_del(str);
   else if (!ts)
     {
        ts = malloc(sizeof(Elm_Translate_String_Data));
        if (!ts) return;

        ts->id = str;
        ts->domain = eina_stringshare_add(domain);
        ts->string = eina_stringshare_add(label);
        sd->translate_strings = eina_list_append(sd->translate_strings, ts);
     }
   else
     {
        if (label)
          {
             eina_stringshare_replace(&ts->domain, domain);
             eina_stringshare_replace(&ts->string, label);
          }
        else
          {
             sd->translate_strings = eina_list_remove_list(
                                                sd->translate_strings, l);
             eina_stringshare_del(ts->id);
             eina_stringshare_del(ts->domain);
             eina_stringshare_del(ts->string);
             free(ts);
          }
        eina_stringshare_del(str);
     }

#ifdef HAVE_GETTEXT
   if (label && label[0])
     label = dgettext(domain, label);
#endif
   elm_widget_text_part_set(obj, part, label);
}

EAPI const char *
elm_widget_translatable_text_part_get(const Evas_Object *obj, const char *part)
{
   const char *str, *ret = NULL;
   Eina_List *l;
   Elm_Translate_String_Data *ts;
   API_ENTRY return NULL;

   str = eina_stringshare_add(part);
   EINA_LIST_FOREACH(sd->translate_strings, l, ts)
      if (ts->id == str)
        {
           ret = ts->string;
           break;
        }
   eina_stringshare_del(str);
   return ret;
}

EAPI void
elm_widget_translate(Evas_Object *obj)
{
   const Eina_List *l;
   Evas_Object *child;
#ifdef HAVE_GETTEXT
   Elm_Translate_String_Data *ts;
#endif

   API_ENTRY return;
   EINA_LIST_FOREACH(sd->subobjs, l, child) elm_widget_translate(child);
   if (sd->resize_obj) elm_widget_translate(sd->resize_obj);
   if (sd->hover_obj) elm_widget_translate(sd->hover_obj);
   if (sd->translate_func) sd->translate_func(obj);

#ifdef HAVE_GETTEXT
   EINA_LIST_FOREACH(sd->translate_strings, l, ts)
     {
        const char *s = dgettext(ts->domain, ts->string);
        elm_widget_text_part_set(obj, ts->id, s);
     }
#endif
}

EAPI void
elm_widget_content_part_set(Evas_Object *obj, const char *part, Evas_Object *content)
{
   API_ENTRY return;

   if (!sd->content_set_func)  return;
   sd->content_set_func(obj, part, content);
}

EAPI Evas_Object *
elm_widget_content_part_get(const Evas_Object *obj, const char *part)
{
   API_ENTRY return NULL;

   if (!sd->content_get_func) return NULL;
   return sd->content_get_func(obj, part);
}

EAPI Evas_Object *
elm_widget_content_part_unset(Evas_Object *obj, const char *part)
{
   API_ENTRY return NULL;

   if (!sd->content_unset_func) return NULL;
   return sd->content_unset_func(obj, part);
}

EAPI void
elm_widget_access_info_set(Evas_Object *obj, const char *txt)
{
   API_ENTRY return;
   if (sd->access_info) eina_stringshare_del(sd->access_info);
   if (!txt) sd->access_info = NULL;
   else sd->access_info = eina_stringshare_add(txt);
}

EAPI const char *
elm_widget_access_info_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->access_info;
}

EAPI Elm_Theme *
elm_widget_theme_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (!sd->theme)
     {
        if (sd->parent_obj)
          return elm_widget_theme_get(sd->parent_obj);
        else
          return NULL;
     }
   return sd->theme;
}

EAPI Eina_Bool
elm_widget_style_set(Evas_Object *obj,
                     const char  *style)
{
   API_ENTRY return EINA_FALSE;

   if (eina_stringshare_replace(&sd->style, style))
     return elm_widget_theme(obj);

   return EINA_TRUE;
}

EAPI const char *
elm_widget_style_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (sd->style) return sd->style;
   return "default";
}

EAPI void
elm_widget_type_set(Evas_Object *obj,
                    const char  *type)
{
   API_ENTRY return;
   eina_stringshare_replace(&sd->type, type);
}

EAPI const char *
elm_widget_type_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (sd->type) return sd->type;
   return "";
}

EAPI void
elm_widget_tooltip_add(Evas_Object *obj,
                       Elm_Tooltip *tt)
{
   API_ENTRY return;
   sd->tooltips = eina_list_append(sd->tooltips, tt);
}

EAPI void
elm_widget_tooltip_del(Evas_Object *obj,
                       Elm_Tooltip *tt)
{
   API_ENTRY return;
   sd->tooltips = eina_list_remove(sd->tooltips, tt);
}

EAPI void
elm_widget_cursor_add(Evas_Object *obj,
                      Elm_Cursor  *cur)
{
   API_ENTRY return;
   sd->cursors = eina_list_append(sd->cursors, cur);
}

EAPI void
elm_widget_cursor_del(Evas_Object *obj,
                      Elm_Cursor  *cur)
{
   API_ENTRY return;
   sd->cursors = eina_list_remove(sd->cursors, cur);
}

EAPI void
elm_widget_drag_lock_x_set(Evas_Object *obj,
                           Eina_Bool    lock)
{
   API_ENTRY return;
   if (sd->drag_x_locked == lock) return;
   sd->drag_x_locked = lock;
   if (sd->drag_x_locked) _propagate_x_drag_lock(obj, 1);
   else _propagate_x_drag_lock(obj, -1);
}

EAPI void
elm_widget_drag_lock_y_set(Evas_Object *obj,
                           Eina_Bool    lock)
{
   API_ENTRY return;
   if (sd->drag_y_locked == lock) return;
   sd->drag_y_locked = lock;
   if (sd->drag_y_locked) _propagate_y_drag_lock(obj, 1);
   else _propagate_y_drag_lock(obj, -1);
}

EAPI Eina_Bool
elm_widget_drag_lock_x_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->drag_x_locked;
}

EAPI Eina_Bool
elm_widget_drag_lock_y_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->drag_y_locked;
}

EAPI int
elm_widget_drag_child_locked_x_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_x_locked;
}

EAPI int
elm_widget_drag_child_locked_y_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_y_locked;
}

EAPI Eina_Bool
elm_widget_theme_object_set(Evas_Object *obj,
                            Evas_Object *edj,
                            const char  *wname,
                            const char  *welement,
                            const char  *wstyle)
{
   API_ENTRY return EINA_FALSE;
   return _elm_theme_object_set(obj, edj, wname, welement, wstyle);
}

EAPI Eina_Bool
elm_widget_is_check(const Evas_Object *obj)
{
   static int abort_on_warn = -1;
   if (elm_widget_is(obj))
      return EINA_TRUE;

   ERR("Passing Object: %p.", obj);
   if (abort_on_warn == -1)
     {
        if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
        else abort_on_warn = 0;
     }
   if (abort_on_warn == 1) abort();
   return EINA_FALSE;
}

EAPI Eina_Bool
elm_widget_type_check(const Evas_Object *obj,
                      const char        *type,
                      const char        *func)
{
   const char *provided, *expected = "(unknown)";
   static int abort_on_warn = -1;
   provided = elm_widget_type_get(obj);
   if (EINA_LIKELY(provided == type)) return EINA_TRUE;
   if (type) expected = type;
   if ((!provided) || (!provided[0]))
     {
        provided = evas_object_type_get(obj);
        if ((!provided) || (!provided[0]))
          provided = "(unknown)";
     }
   ERR("Passing Object: %p in function: %s, of type: '%s' when expecting type: '%s'", obj, func, provided, expected);
   if (abort_on_warn == -1)
     {
        if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
        else abort_on_warn = 0;
     }
   if (abort_on_warn == 1) abort();
   return EINA_FALSE;
}

static Evas_Object *
_widget_name_find(const Evas_Object *obj, const char *name, int recurse)
{
   Eina_List *l;
   Evas_Object *child;
   const char *s;
   INTERNAL_ENTRY NULL;

   if (!_elm_widget_is(obj)) return NULL;
   if (sd->resize_obj)
     {
        s = evas_object_name_get(sd->resize_obj);
        if ((s) && (!strcmp(s, name))) return sd->resize_obj;
        if ((recurse != 0) &&
            ((child = _widget_name_find(sd->resize_obj, name, recurse - 1))))
          return child;
     }
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        s = evas_object_name_get(child);
        if ((s) && (!strcmp(s, name))) return child;
        if ((recurse != 0) &&
            ((child = _widget_name_find(child, name, recurse - 1))))
          return child;
     }
   if (sd->hover_obj)
     {
        s = evas_object_name_get(sd->hover_obj);
        if ((s) && (!strcmp(s, name))) return sd->hover_obj;
        if ((recurse != 0) &&
            ((child = _widget_name_find(sd->hover_obj, name, recurse - 1))))
          return child;
     }
   return NULL;
}

EAPI Evas_Object *
elm_widget_name_find(const Evas_Object *obj, const char *name, int recurse)
{
   API_ENTRY return NULL;
   if (!name) return NULL;
   return _widget_name_find(obj, name, recurse);
}

/**
 * @internal
 *
 * Split string in words
 *
 * @param str Source string
 * @return List of const words
 *
 * @see elm_widget_stringlist_free()
 * @ingroup Widget
 */
EAPI Eina_List *
elm_widget_stringlist_get(const char *str)
{
   Eina_List *list = NULL;
   const char *s, *b;
   if (!str) return NULL;
   for (b = s = str; 1; s++)
     {
        if ((*s == ' ') || (!*s))
          {
             char *t = malloc(s - b + 1);
             if (t)
               {
                  strncpy(t, b, s - b);
                  t[s - b] = 0;
                  list = eina_list_append(list, eina_stringshare_add(t));
                  free(t);
               }
             b = s + 1;
          }
        if (!*s) break;
     }
   return list;
}

EAPI void
elm_widget_stringlist_free(Eina_List *list)
{
   const char *s;
   EINA_LIST_FREE(list, s) eina_stringshare_del(s);
}

EAPI void
elm_widget_focus_hide_handle(Evas_Object *obj)
{
   if (!_elm_widget_is(obj))
     return;
   _if_focused_revert(obj, EINA_TRUE);
}

EAPI void
elm_widget_focus_mouse_up_handle(Evas_Object *obj)
{
   Evas_Object *o = obj;
   do
     {
        if (_elm_widget_is(o)) break;
        o = evas_object_smart_parent_get(o);
     }
   while (o);
   if (!o) return;
   if (!_is_focusable(o)) return;
   elm_widget_focus_steal(o);
}

EAPI void
elm_widget_focus_tree_unfocusable_handle(Evas_Object *obj)
{
   API_ENTRY return;

   //FIXME: Need to check whether the object is unfocusable or not.

   if (!elm_widget_parent_get(obj))
     elm_widget_focused_object_clear(obj);
   else
     _if_focused_revert(obj, EINA_TRUE);
}

EAPI void
elm_widget_focus_disabled_handle(Evas_Object *obj)
{
   API_ENTRY return;

   elm_widget_focus_tree_unfocusable_handle(obj);
}

EAPI unsigned int
elm_widget_focus_order_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->focus_order;
}

/**
 * @internal
 *
 * Allocate a new Elm_Widget_Item-derived structure.
 *
 * The goal of this structure is to provide common ground for actions
 * that a widget item have, such as the owner widget, callback to
 * notify deletion, data pointer and maybe more.
 *
 * @param widget the owner widget that holds this item, must be an elm_widget!
 * @param alloc_size any number greater than sizeof(Elm_Widget_Item) that will
 *        be used to allocate memory.
 *
 * @return allocated memory that is already zeroed out, or NULL on errors.
 *
 * @see elm_widget_item_new() convenience macro.
 * @see elm_widget_item_del() to release memory.
 * @ingroup Widget
 */
EAPI Elm_Widget_Item *
_elm_widget_item_new(Evas_Object *widget,
                     size_t       alloc_size)
{
   if (!_elm_widget_is(widget))
     return NULL;

   Elm_Widget_Item *item;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(alloc_size < sizeof(Elm_Widget_Item), NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!_elm_widget_is(widget), NULL);

   item = calloc(1, alloc_size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

   EINA_MAGIC_SET(item, ELM_WIDGET_ITEM_MAGIC);
   item->widget = widget;
   return item;
}

EAPI void
_elm_widget_item_free(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   _elm_access_item_unregister(item);

   if (item->del_func)
     item->del_func((void *)item->data, item->widget, item);

   if (item->view)
     evas_object_del(item->view);

   if (item->access)
     {
        _elm_access_clear(item->access);
        free(item->access);
     }

   if (item->access_info)
     eina_stringshare_del(item->access_info);

   EINA_MAGIC_SET(item, EINA_MAGIC_NONE);
   free(item);
}

/**
 * @internal
 *
 * Releases widget item memory, calling back del_cb() if it exists.
 *
 * If there is a Elm_Widget_Item::del_cb, then it will be called prior
 * to memory release. Note that elm_widget_item_pre_notify_del() calls
 * this function and then unset it, thus being useful for 2 step
 * cleanup whenever the del_cb may use any of the data that must be
 * deleted from item.
 *
 * The Elm_Widget_Item::view will be deleted (evas_object_del()) if it
 * is presented!
 *
 * @param item a valid #Elm_Widget_Item to be deleted.
 * @see elm_widget_item_del() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   //Widget item delete callback
   if (item->del_pre_func)
     {
        if (item->del_pre_func((Elm_Object_Item *)item))
          _elm_widget_item_free(item);
     }
   else
     _elm_widget_item_free(item);
}

/**
 * @internal
 *
 * Set the function to notify to widgets when item is being deleted by user.
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_del_pre_hook_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del_pre_hook_set(Elm_Widget_Item *item, Elm_Widget_Del_Pre_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->del_pre_func = func;
}

/**
 * @internal
 *
 * Notify object will be deleted without actually deleting it.
 *
 * This function will callback Elm_Widget_Item::del_cb if it is set
 * and then unset it so it is not called twice (ie: from
 * elm_widget_item_del()).
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_pre_notify_del() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_pre_notify_del(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (!item->del_func) return;
   item->del_func((void *)item->data, item->widget, item);
   item->del_func = NULL;
}

/**
 * @internal
 *
 * Set the function to notify when item is being deleted.
 *
 * This function will complain if there was a callback set already,
 * however it will set the new one.
 *
 * The callback will be called from elm_widget_item_pre_notify_del()
 * or elm_widget_item_del() will be called with:
 *   - data: the Elm_Widget_Item::data value.
 *   - obj: the Elm_Widget_Item::widget evas object.
 *   - event_info: the item being deleted.
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_del_cb_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del_cb_set(Elm_Widget_Item *item,
                            Evas_Smart_Cb    func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if ((item->del_func) && (item->del_func != func))
     WRN("You're replacing a previously set del_cb %p of item %p with %p",
         item->del_func, item, func);

   item->del_func = func;
}

/**
 * @internal
 *
 * Set user-data in this item.
 *
 * User data may be used to identify this item or just store any
 * application data. It is automatically given as the first parameter
 * of the deletion notify callback.
 *
 * @param item a valid #Elm_Widget_Item to store data in.
 * @param data user data to store.
 * @see elm_widget_item_del_cb_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_data_set(Elm_Widget_Item *item,
                          const void      *data)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if ((item->data) && (item->data != data))
     DBG("Replacing item %p data %p with %p", item, item->data, data);
   item->data = data;
}

/**
 * @internal
 *
 * Retrieves user-data of this item.
 *
 * @param item a valid #Elm_Widget_Item to get data from.
 * @see elm_widget_item_data_set()
 * @ingroup Widget
 */
EAPI void *
_elm_widget_item_data_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return (void *)item->data;
}

EAPI void
_elm_widget_item_disabled_set(Elm_Widget_Item *item, Eina_Bool disabled)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (item->disabled == disabled) return;
   item->disabled = !!disabled;
   if (item->disable_func) item->disable_func(item);
}

EAPI Eina_Bool
_elm_widget_item_disabled_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->disabled;
}

EAPI void
_elm_widget_item_disable_hook_set(Elm_Widget_Item *item,
                                  Elm_Widget_Disable_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->disable_func = func;
}

typedef struct _Elm_Widget_Item_Tooltip Elm_Widget_Item_Tooltip;

struct _Elm_Widget_Item_Tooltip
{
   Elm_Widget_Item            *item;
   Elm_Tooltip_Item_Content_Cb func;
   Evas_Smart_Cb               del_cb;
   const void                 *data;
};

static Evas_Object *
_elm_widget_item_tooltip_label_create(void        *data,
                                      Evas_Object *obj __UNUSED__,
                                      Evas_Object *tooltip,
                                      void        *item __UNUSED__)
{
   Evas_Object *label = elm_label_add(tooltip);
   if (!label)
     return NULL;
   elm_object_style_set(label, "tooltip");
   elm_object_text_set(label, data);
   return label;
}

static Evas_Object *
_elm_widget_item_tooltip_trans_label_create(void        *data,
                                            Evas_Object *obj __UNUSED__,
                                            Evas_Object *tooltip,
                                            void        *item __UNUSED__)
{
   Evas_Object *label = elm_label_add(tooltip);
   if (!label)
     return NULL;
   elm_object_style_set(label, "tooltip");
   elm_object_translatable_text_set(label, data);
   return label;
}

static void
_elm_widget_item_tooltip_label_del_cb(void        *data,
                                      Evas_Object *obj __UNUSED__,
                                      void        *event_info __UNUSED__)
{
   eina_stringshare_del(data);
}

/**
 * @internal
 *
 * Set the text to be shown in the widget item.
 *
 * @param item Target item
 * @param text The text to set in the content
 *
 * Setup the text as tooltip to object. The item can have only one tooltip,
 * so any previous tooltip data is removed.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_text_set(Elm_Widget_Item *item,
                                  const char      *text)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(text);

   text = eina_stringshare_add(text);
   _elm_widget_item_tooltip_content_cb_set
     (item, _elm_widget_item_tooltip_label_create, text,
     _elm_widget_item_tooltip_label_del_cb);
}

EAPI void
_elm_widget_item_tooltip_translatable_text_set(Elm_Widget_Item *item,
                                               const char      *text)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(text);

   text = eina_stringshare_add(text);
   _elm_widget_item_tooltip_content_cb_set
     (item, _elm_widget_item_tooltip_trans_label_create, text,
     _elm_widget_item_tooltip_label_del_cb);
}

static Evas_Object *
_elm_widget_item_tooltip_create(void        *data,
                                Evas_Object *obj,
                                Evas_Object *tooltip)
{
   Elm_Widget_Item_Tooltip *wit = data;
   return wit->func((void *)wit->data, obj, tooltip, wit->item);
}

static void
_elm_widget_item_tooltip_del_cb(void        *data,
                                Evas_Object *obj,
                                void        *event_info __UNUSED__)
{
   Elm_Widget_Item_Tooltip *wit = data;
   if (wit->del_cb) wit->del_cb((void *)wit->data, obj, wit->item);
   free(wit);
}

/**
 * @internal
 *
 * Set the content to be shown in the tooltip item
 *
 * Setup the tooltip to item. The item can have only one tooltip,
 * so any previous tooltip data is removed. @p func(with @p data) will
 * be called every time that need show the tooltip and it should
 * return a valid Evas_Object. This object is then managed fully by
 * tooltip system and is deleted when the tooltip is gone.
 *
 * @param item the widget item being attached a tooltip.
 * @param func the function used to create the tooltip contents.
 * @param data what to provide to @a func as callback data/context.
 * @param del_cb called when data is not needed anymore, either when
 *        another callback replaces @func, the tooltip is unset with
 *        elm_widget_item_tooltip_unset() or the owner @a item
 *        dies. This callback receives as the first parameter the
 *        given @a data, and @c event_info is the item.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_content_cb_set(Elm_Widget_Item            *item,
                                        Elm_Tooltip_Item_Content_Cb func,
                                        const void                 *data,
                                        Evas_Smart_Cb               del_cb)
{
   Elm_Widget_Item_Tooltip *wit;

   ELM_WIDGET_ITEM_CHECK_OR_GOTO(item, error_noitem);

   if (!func)
     {
        _elm_widget_item_tooltip_unset(item);
        return;
     }

   wit = ELM_NEW(Elm_Widget_Item_Tooltip);
   if (!wit) goto error;
   wit->item = item;
   wit->func = func;
   wit->data = data;
   wit->del_cb = del_cb;

   elm_object_sub_tooltip_content_cb_set
     (item->view, item->widget, _elm_widget_item_tooltip_create, wit,
     _elm_widget_item_tooltip_del_cb);

   return;

error_noitem:
   if (del_cb) del_cb((void *)data, NULL, item);
   return;
error:
   if (del_cb) del_cb((void *)data, item->widget, item);
}

/**
 * @internal
 *
 * Unset tooltip from item
 *
 * @param item widget item to remove previously set tooltip.
 *
 * Remove tooltip from item. The callback provided as del_cb to
 * elm_widget_item_tooltip_content_cb_set() will be called to notify
 * it is not used anymore.
 *
 * @see elm_widget_item_tooltip_content_cb_set()
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_unset(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_tooltip_unset(item->view);
}

/**
 * @internal
 *
 * Sets a different style for this item tooltip.
 *
 * @note before you set a style you should define a tooltip with
 *       elm_widget_item_tooltip_content_cb_set() or
 *       elm_widget_item_tooltip_text_set()
 *
 * @param item widget item with tooltip already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_style_set(Elm_Widget_Item *item,
                                   const char      *style)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_tooltip_style_set(item->view, style);
}

EAPI Eina_Bool
_elm_widget_item_tooltip_window_mode_set(Elm_Widget_Item *item, Eina_Bool disable)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_object_tooltip_window_mode_set(item->view, disable);
}

EAPI Eina_Bool
_elm_widget_item_tooltip_window_mode_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_object_tooltip_window_mode_get(item->view);
}

/**
 * @internal
 *
 * Get the style for this item tooltip.
 *
 * @param item widget item with tooltip already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a tooltip set, then NULL is returned.
 *
 * @ingroup Widget
 */
EAPI const char *
_elm_widget_item_tooltip_style_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_object_tooltip_style_get(item->view);
}

EAPI void
_elm_widget_item_cursor_set(Elm_Widget_Item *item,
                            const char      *cursor)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_sub_cursor_set(item->view, item->widget, cursor);
}

EAPI const char *
_elm_widget_item_cursor_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_object_cursor_get(item->view);
}

EAPI void
_elm_widget_item_cursor_unset(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_cursor_unset(item->view);
}

/**
 * @internal
 *
 * Sets a different style for this item cursor.
 *
 * @note before you set a style you should define a cursor with
 *       elm_widget_item_cursor_set()
 *
 * @param item widget item with cursor already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_cursor_style_set(Elm_Widget_Item *item,
                                  const char      *style)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_cursor_style_set(item->view, style);
}

/**
 * @internal
 *
 * Get the style for this item cursor.
 *
 * @param item widget item with cursor already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a cursor set, then NULL is returned.
 *
 * @ingroup Widget
 */
EAPI const char *
_elm_widget_item_cursor_style_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_object_cursor_style_get(item->view);
}

/**
 * @internal
 *
 * Set if the cursor set should be searched on the theme or should use
 * the provided by the engine, only.
 *
 * @note before you set if should look on theme you should define a cursor
 * with elm_object_cursor_set(). By default it will only look for cursors
 * provided by the engine.
 *
 * @param item widget item with cursor already set.
 * @param engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_cursor_engine_only_set(Elm_Widget_Item *item,
                                        Eina_Bool        engine_only)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   elm_object_cursor_theme_search_enabled_set(item->view, engine_only);
}

/**
 * @internal
 *
 * Get the cursor engine only usage for this item cursor.
 *
 * @param item widget item with cursor already set.
 * @return engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well. If
 *         the object does not have a cursor set, then EINA_FALSE is returned.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
_elm_widget_item_cursor_engine_only_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_object_cursor_theme_search_enabled_get(item->view);
}

// smart object funcs
static void
_smart_reconfigure(Smart_Data *sd)
{
   if (sd->resize_obj)
     {
        evas_object_move(sd->resize_obj, sd->x, sd->y);
        evas_object_resize(sd->resize_obj, sd->w, sd->h);
     }
   if (sd->hover_obj)
     {
        evas_object_move(sd->hover_obj, sd->x, sd->y);
        evas_object_resize(sd->hover_obj, sd->w, sd->h);
     }
}

EAPI void
_elm_widget_item_content_part_set(Elm_Widget_Item *item,
                                 const char *part,
                                 Evas_Object *content)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (!item->content_set_func) return;
   item->content_set_func((Elm_Object_Item *)item, part, content);
}

EAPI Evas_Object *
_elm_widget_item_content_part_get(const Elm_Widget_Item *item,
                                  const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   if (!item->content_get_func) return NULL;
   return item->content_get_func((Elm_Object_Item *)item, part);
}

EAPI Evas_Object *
_elm_widget_item_content_part_unset(Elm_Widget_Item *item,
                                    const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   if (!item->content_unset_func) return NULL;
   return item->content_unset_func((Elm_Object_Item *)item, part);
}

EAPI void
_elm_widget_item_text_part_set(Elm_Widget_Item *item,
                              const char *part,
                              const char *label)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (!item->text_set_func) return;
   item->text_set_func((Elm_Object_Item *)item, part, label);
}

EAPI void
_elm_widget_item_signal_emit(Elm_Widget_Item *item,
                             const char *emission,
                             const char *source)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (item->signal_emit_func)
     item->signal_emit_func((Elm_Object_Item *)item, emission, source);
}

EAPI const char *
_elm_widget_item_text_part_get(const Elm_Widget_Item *item,
                               const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   if (!item->text_get_func) return NULL;
   return item->text_get_func((Elm_Object_Item *)item, part);
}

EAPI void
_elm_widget_item_content_set_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Content_Set_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->content_set_func = func;
}

EAPI void
_elm_widget_item_content_get_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Content_Get_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->content_get_func = func;
}

EAPI void
_elm_widget_item_content_unset_hook_set(Elm_Widget_Item *item,
                                        Elm_Widget_Content_Unset_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->content_unset_func = func;
}

EAPI void
_elm_widget_item_text_set_hook_set(Elm_Widget_Item *item,
                                   Elm_Widget_Text_Set_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->text_set_func = func;
}

EAPI void
_elm_widget_item_text_get_hook_set(Elm_Widget_Item *item,
                                   Elm_Widget_Text_Get_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->text_get_func = func;
}

EAPI void
_elm_widget_item_signal_emit_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Signal_Emit_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   item->signal_emit_func = func;
}

EAPI void
_elm_widget_item_access_info_set(Elm_Widget_Item *item, const char *txt)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (item->access_info) eina_stringshare_del(item->access_info);
   if (!txt) item->access_info = NULL;
   else item->access_info = eina_stringshare_add(txt);
}

static void
_smart_add(Evas_Object *obj)
{
   Smart_Data *sd;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->mirrored_auto_mode = EINA_TRUE; /* will follow system locale settings */
   evas_object_smart_data_set(obj, sd);
   elm_widget_can_focus_set(obj, EINA_TRUE);
}

static Evas_Object *
_newest_focus_order_get(Evas_Object  *obj,
                        unsigned int *newest_focus_order,
                        Eina_Bool     can_focus_only)
{
   const Eina_List *l;
   Evas_Object *child, *ret, *best;

   API_ENTRY return NULL;

   if (!evas_object_visible_get(obj)
       || (elm_widget_disabled_get(obj))
       || (elm_widget_tree_unfocusable_get(obj)))
     return NULL;

   best = NULL;
   if (*newest_focus_order < sd->focus_order)
     {
        *newest_focus_order = sd->focus_order;
        best = obj;
     }
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        ret = _newest_focus_order_get(child, newest_focus_order, can_focus_only);
        if (!ret) continue;
        best = ret;
     }
   if (can_focus_only)
     {
        if ((!best) || (!elm_widget_can_focus_get(best)))
          return NULL;
     }
   return best;
}

static void
_if_focused_revert(Evas_Object *obj,
                   Eina_Bool    can_focus_only)
{
   Evas_Object *top;
   Evas_Object *newest = NULL;
   unsigned int newest_focus_order = 0;

   INTERNAL_ENTRY;

   if (!sd->focused) return;
   if (!sd->parent_obj) return;

   top = elm_widget_top_get(sd->parent_obj);
   if (top)
     {
        newest = _newest_focus_order_get(top, &newest_focus_order, can_focus_only);
        if (newest)
          {
             elm_object_focus_set(newest, EINA_FALSE);
             elm_object_focus_set(newest, EINA_TRUE);
          }
     }
}

static void
_smart_del(Evas_Object *obj)
{
   Evas_Object *sobj;
   Edje_Signal_Data *esd;
   Elm_Translate_String_Data *ts;

   INTERNAL_ENTRY;

   if (sd->del_pre_func) sd->del_pre_func(obj);
   if (sd->resize_obj)
     {
        sobj = sd->resize_obj;
        sd->resize_obj = NULL;
        evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
        evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
        evas_object_del(sobj);
        sd->resize_obj = NULL;
     }
   if (sd->hover_obj)
     {
        sobj = sd->hover_obj;
        sd->hover_obj = NULL;
        evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
        evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
        evas_object_del(sobj);
        sd->hover_obj = NULL;
     }
   EINA_LIST_FREE(sd->subobjs, sobj)
     {
        evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
        evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
        evas_object_del(sobj);
     }
   sd->tooltips = eina_list_free(sd->tooltips); /* should be empty anyway */
   sd->cursors = eina_list_free(sd->cursors); /* should be empty anyway */
   EINA_LIST_FREE(sd->edje_signals, esd)
     {
        eina_stringshare_del(esd->emission);
        eina_stringshare_del(esd->source);
        free(esd);
     }
   EINA_LIST_FREE(sd->translate_strings, ts)
     {
        eina_stringshare_del(ts->id);
        eina_stringshare_del(ts->domain);
        eina_stringshare_del(ts->string);
        free(ts);
     }
   sd->event_cb = eina_list_free(sd->event_cb); /* should be empty anyway */
   if (sd->del_func) sd->del_func(obj);
   if (sd->style) eina_stringshare_del(sd->style);
   if (sd->type) eina_stringshare_del(sd->type);
   if (sd->theme) elm_theme_free(sd->theme);
   sd->data = NULL;
   _if_focused_revert(obj, EINA_TRUE);
   if (sd->access_info) eina_stringshare_del(sd->access_info);
   free(sd);
   evas_object_smart_data_set(obj, NULL);
}

static void
_smart_move(Evas_Object *obj,
            Evas_Coord   x,
            Evas_Coord   y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   _smart_reconfigure(sd);
}

static void
_smart_resize(Evas_Object *obj,
              Evas_Coord   w,
              Evas_Coord   h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   _smart_reconfigure(sd);
}

static void
_smart_show(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;
   INTERNAL_ENTRY;
   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE(list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_show(o);
          }
     }
}

static void
_smart_hide(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;
   INTERNAL_ENTRY;

   list = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(list, o)
     {
        if (evas_object_data_get(o, "_elm_leaveme")) continue;
        evas_object_hide(o);
     }
}

static void
_smart_color_set(Evas_Object *obj,
                 int          r,
                 int          g,
                 int          b,
                 int          a)
{
   Eina_List *list;
   Evas_Object *o;
   INTERNAL_ENTRY;
   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE(list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_color_set(o, r, g, b, a);
          }
     }
}

static void
_smart_clip_set(Evas_Object *obj,
                Evas_Object *clip)
{
   Eina_List *list;
   Evas_Object *o;
   INTERNAL_ENTRY;
   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE(list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_clip_set(o, clip);
          }
     }
}

static void
_smart_clip_unset(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;
   INTERNAL_ENTRY;
   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE(list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_clip_unset(o);
          }
     }
}

static void
_smart_calculate(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   if (sd->changed_func) sd->changed_func(obj);
}

static void
_smart_member_add(Evas_Object *obj, Evas_Object *child)
{
   int r, g, b, a;

   if (evas_object_data_get(child, "_elm_leaveme")) return;

   evas_object_color_get(obj, &r, &g, &b, &a);
   evas_object_color_set(child, r, g, b, a);

   evas_object_clip_set(child, evas_object_clip_get(obj));

   if (evas_object_visible_get(obj))
     evas_object_show(child);
   else
     evas_object_hide(child);
}

static void
_smart_member_del(Evas_Object *obj __UNUSED__, Evas_Object *child)
{
   if (evas_object_data_get(child, "_elm_leaveme")) return;
   evas_object_clip_unset(child);
}

/* never need to touch this */
static void
_smart_init(void)
{
   if (_e_smart) return;
     {
        static const Evas_Smart_Class sc =
          {
             SMART_NAME,
             EVAS_SMART_CLASS_VERSION,
             _smart_add,
             _smart_del,
             _smart_move,
             _smart_resize,
             _smart_show,
             _smart_hide,
             _smart_color_set,
             _smart_clip_set,
             _smart_clip_unset,
             _smart_calculate,
             _smart_member_add,
             _smart_member_del,
             NULL,
             NULL,
             NULL,
             NULL
          };
        _e_smart = evas_smart_class_new(&sc);
     }
}

/* happy debug functions */
#ifdef ELM_DEBUG
static void
_sub_obj_tree_dump(const Evas_Object *obj,
                   int                lvl)
{
   int i;

   for (i = 0; i < lvl * 3; i++)
     putchar(' ');

   if (_elm_widget_is(obj))
     {
        Eina_List *l;
        INTERNAL_ENTRY;
        printf("+ %s(%p)\n",
               sd->type,
               obj);
        if (sd->resize_obj)
          _sub_obj_tree_dump(sd->resize_obj, lvl + 1);
        EINA_LIST_FOREACH(sd->subobjs, l, obj)
          {
             if (obj != sd->resize_obj)
               _sub_obj_tree_dump(obj, lvl + 1);
          }
     }
   else
     printf("+ %s(%p)\n", evas_object_type_get(obj), obj);
}

static void
_sub_obj_tree_dot_dump(const Evas_Object *obj,
                       FILE              *output)
{
   if (!_elm_widget_is(obj))
     return;
   INTERNAL_ENTRY;

   Eina_Bool visible = evas_object_visible_get(obj);
   Eina_Bool disabled = elm_widget_disabled_get(obj);
   Eina_Bool focused = elm_widget_focus_get(obj);
   Eina_Bool can_focus = elm_widget_can_focus_get(obj);

   if (sd->parent_obj)
     {
        fprintf(output, "\"%p\" -- \"%p\" [ color=black", sd->parent_obj, obj);

        if (focused)
          fprintf(output, ", style=bold");

        if (!visible)
          fprintf(output, ", color=gray28");

        fprintf(output, " ];\n");
     }

   fprintf(output, "\"%p\" [ label = \"{%p|%s|%s|visible: %d|"
                   "disabled: %d|focused: %d/%d|focus order:%d}\"", obj, obj, sd->type,
           evas_object_name_get(obj), visible, disabled, focused, can_focus,
           sd->focus_order);

   if (focused)
     fprintf(output, ", style=bold");

   if (!visible)
     fprintf(output, ", fontcolor=gray28");

   if ((disabled) || (!visible))
     fprintf(output, ", color=gray");

   fprintf(output, " ];\n");

   Eina_List *l;
   Evas_Object *o;
   EINA_LIST_FOREACH(sd->subobjs, l, o)
     _sub_obj_tree_dot_dump(o, output);
}
#endif

EAPI void
elm_widget_tree_dump(const Evas_Object *top)
{
#ifdef ELM_DEBUG
   if (!_elm_widget_is(top))
     return;
   _sub_obj_tree_dump(top, 0);
#else
   return;
   (void)top;
#endif
}

EAPI void
elm_widget_tree_dot_dump(const Evas_Object *top,
                         FILE              *output)
{
#ifdef ELM_DEBUG
   if (!_elm_widget_is(top))
     return;
   fprintf(output, "graph " " { node [shape=record];\n");
   _sub_obj_tree_dot_dump(top, output);
   fprintf(output, "}\n");
#else
   return;
   (void)top;
   (void)output;
#endif
}
