#include <Elementary.h>
#include "elm_priv.h"

#ifndef MIN
# define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

#ifndef MAX
# define MAX(a,b) ((a) < (b)) ? (b) : (a)
#endif

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
   Evas_Object *base;
   Evas_Object *shelf, *panel, *virtualkeypad, *sliding_win;
   Evas_Object *content;
   Evas_Object *scroller;
   Evas_Object *layout;
   int is_visible;
#ifdef HAVE_ELEMENTARY_X
   Ecore_Event_Handler *prop_hdl;
   Ecore_X_Virtual_Keyboard_State vkb_state;
#endif
   struct
   {
      Ecore_Animator *animator; // animaton timer
      double start; // time started
      Evas_Coord auto_x, auto_y; // desired delta
      Evas_Coord x, y; // current delta
   } delta;
   Ecore_Job *show_region_job;
};

/* Enum to identify conformant swallow parts */
typedef enum _Conformant_Part_Type Conformant_Part_Type;
enum _Conformant_Part_Type
{
   ELM_CONFORM_INDICATOR_PART      = 1,
   ELM_CONFORM_SOFTKEY_PART        = 2,
   ELM_CONFORM_VIRTUAL_KEYPAD_PART = 4,
   ELM_CONFORM_SLIDING_WIN_PART    = 8
};

#define SUB_TYPE_COUNT 2
static char *sub_type[SUB_TYPE_COUNT] = { "scroller", "genlist" };

/* local function prototypes */
static const char *widtype = NULL;
static void _del_pre_hook(Evas_Object *obj);
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _content_set_hook(Evas_Object *obj,
                              const char *part,
                              Evas_Object *content);
static Evas_Object *_content_get_hook(const Evas_Object *obj,
                                      const char *part);
static Evas_Object *_content_unset_hook(Evas_Object *obj, const char *part);
static void _swallow_conformant_parts(Evas_Object *obj);
#ifdef HAVE_ELEMENTARY_X
static void _conformant_part_size_set(Evas_Object *obj,
                                      Evas_Object *sobj,
                                      Evas_Coord sx,
                                      Evas_Coord sy,
                                      Evas_Coord sw,
                                      Evas_Coord sh);
static void _conformant_part_sizing_eval(Evas_Object *obj,
                                         Conformant_Part_Type part_type);
static void _conformant_move_resize_event_cb(void *data,
                                             Evas *e,
                                             Evas_Object *obj,
                                             void *event_info);
#endif
static void _sizing_eval(Evas_Object *obj);
static Eina_Bool _prop_change(void *data, int type, void *event);
static void _changed_size_hints(void *data, Evas *e,
                                Evas_Object *obj,
                                void *event_info);

/* local functions */
static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
#ifdef HAVE_ELEMENTARY_X
   if (wd->prop_hdl) ecore_event_handler_del(wd->prop_hdl);
#endif
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   free(wd);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   edje_object_mirrored_set(wd->base, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _elm_theme_object_set(obj, wd->base, "conformant", "base",
                         elm_widget_style_get(obj));

   edje_object_scale_set(wd->base, elm_widget_scale_get(obj)
                         * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   if (part && strcmp(part, "default")) return;
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content == content) return;
   if (wd->content) evas_object_del(wd->content);
   wd->content = content;
   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_part_swallow(wd->base, "elm.swallow.content", content);
     }
   _sizing_eval(obj);
}

static Evas_Object *
_content_get_hook(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   if (part && strcmp(part, "default")) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->content;
}

static Evas_Object *
_content_unset_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Evas_Object *content;
   if (part && strcmp(part, "default")) return NULL;
   wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->content)) return NULL;
   content = wd->content;
   elm_widget_sub_object_del(obj, wd->content);
   evas_object_event_callback_del_full(wd->content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
   edje_object_part_unswallow(wd->base, wd->content);
   wd->content = NULL;
   return content;
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord mw = -1, mh = -1;

   if (!wd) return;
   edje_object_size_min_calc(wd->base, &mw, &mh);
   evas_object_size_hint_min_set(obj, mw, mh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

#ifdef HAVE_ELEMENTARY_X
static void
_conformant_part_size_set(Evas_Object *obj, Evas_Object *sobj, Evas_Coord sx,
                          Evas_Coord sy, Evas_Coord sw, Evas_Coord sh)
{
   Evas_Coord cx, cy, cw, ch;
   Evas_Coord part_height = 0, part_width = 0;

   evas_object_geometry_get(obj, &cx, &cy, &cw, &ch);

   /* Part overlapping with conformant */
   if ((cx < (sx + sw)) && ((cx + cw) > sx)
            && (cy < (sy + sh)) && ((cy + ch) > sy))
     {
        part_height = MIN((cy + ch), (sy + sh)) - MAX(cy, sy);
        part_width = MIN((cx + cw), (sx + sw)) - MAX(cx, sx);
     }

   evas_object_size_hint_min_set(sobj, part_width, part_height);
   evas_object_size_hint_max_set(sobj, part_width, part_height);
}

static void
_conformant_part_sizing_eval(Evas_Object *obj, Conformant_Part_Type part_type)
{
   Ecore_X_Window zone, xwin;
   Evas_Object *top;
   Eina_Bool ret;
   int sx = -1, sy = -1, sw = -1, sh = -1;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   top = elm_widget_top_get(obj);
   xwin = elm_win_xwindow_get(top);

   zone = ecore_x_e_illume_zone_get(xwin);

   if (part_type & ELM_CONFORM_INDICATOR_PART)
     {
        ret = ecore_x_e_illume_indicator_geometry_get(zone, &sx, &sy, &sw, &sh);
        if (!ret) //There is NO information of the indicator geometry, reset the geometry.
           _conformant_part_size_set(obj, wd->shelf, 0, 0, 0, 0);
        else
           _conformant_part_size_set(obj, wd->shelf, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_VIRTUAL_KEYPAD_PART)
     {
        edje_object_part_swallow(wd->base, "elm.swallow.virtualkeypad",
                               wd->virtualkeypad);
        ret = ecore_x_e_illume_keyboard_geometry_get(zone, &sx, &sy, &sw, &sh);

        if (!ret) //There is NO information of the keyboard geometry, reset the geometry.
          _conformant_part_size_set(obj, wd->virtualkeypad, 0, 0, 0, 0);
        else
          _conformant_part_size_set(obj,wd->virtualkeypad, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_SOFTKEY_PART)
     {
        ret = ecore_x_e_illume_softkey_geometry_get(zone, &sx, &sy, &sw, &sh);
        if (!ret) //There is NO information of the softkey geometry, reset the geometry.
          _conformant_part_size_set(obj,wd->panel, 0, 0, 0, 0);
        else
          _conformant_part_size_set(obj, wd->panel, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_SLIDING_WIN_PART)
     {
        //If the virtual keypad uses the swallow area
        ret = ecore_x_e_illume_keyboard_geometry_get(zone, &sx, &sy, &sw, &sh);
        if (ret && sh > 0) return;

        edje_object_part_swallow(wd->base, "elm.swallow.virtualkeypad",
                               wd->sliding_win);
        ret = ecore_x_e_illume_sliding_win_geometry_get(zone, &sx, &sy, &sw, &sh);

        if (!ret) //There is NO information of the sliding win geometry, reset the geometry.
          _conformant_part_size_set(obj,wd->sliding_win, 0, 0, 0, 0);
        else
          _conformant_part_size_set(obj, wd->sliding_win, sx, sy, sw, sh);
     }
}
#endif

static void
_swallow_conformant_parts(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   wd->scroller = NULL;
   if (!wd->shelf)
     {
        wd->shelf = evas_object_rectangle_add(evas_object_evas_get(obj));
        elm_widget_sub_object_add(obj, wd->shelf);
        evas_object_size_hint_min_set(wd->shelf, -1, 0);
        evas_object_size_hint_max_set(wd->shelf, -1, 0);
     }
#ifdef HAVE_ELEMENTARY_X
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_INDICATOR_PART);
#endif

   evas_object_color_set(wd->shelf, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.shelf", wd->shelf);

   if (!wd->virtualkeypad)
     {
        wd->virtualkeypad = evas_object_rectangle_add(evas_object_evas_get(obj));
        elm_widget_sub_object_add(obj, wd->virtualkeypad);
        evas_object_size_hint_min_set(wd->virtualkeypad, -1, 0);
        evas_object_size_hint_max_set(wd->virtualkeypad, -1, 0);
     }
#ifdef HAVE_ELEMENTARY_X
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_VIRTUAL_KEYPAD_PART);
#endif
   evas_object_color_set(wd->virtualkeypad, 0, 0, 0, 0);

   if (!wd->sliding_win)
     {
        wd->sliding_win = evas_object_rectangle_add(evas_object_evas_get(obj));
        elm_widget_sub_object_add(obj, wd->sliding_win);
        evas_object_size_hint_min_set(wd->sliding_win, -1, 0);
        evas_object_size_hint_max_set(wd->sliding_win, -1, 0);
     }
#ifdef HAVE_ELEMENTARY_X
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_SLIDING_WIN_PART);
#endif
   evas_object_color_set(wd->sliding_win, 0, 0, 0, 0);

   if (!wd->panel)
     {
        wd->panel = evas_object_rectangle_add(evas_object_evas_get(obj));
        elm_widget_sub_object_add(obj, wd->panel);
        evas_object_size_hint_min_set(wd->panel, -1, 0);
        evas_object_size_hint_max_set(wd->panel, -1, 0);
     }
#ifdef HAVE_ELEMENTARY_X
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_SOFTKEY_PART);
#endif

   evas_object_color_set(wd->panel, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.panel", wd->panel);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Object *sub = event_info;

   if (!wd) return;
   if (sub == wd->content)
     {
        evas_object_event_callback_del_full(sub,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, data);
        wd->content = NULL;
        _sizing_eval(data);
     }
}

#ifdef HAVE_ELEMENTARY_X
static void
_conformant_move_resize_event_cb(void *data __UNUSED__, Evas *e __UNUSED__,
                                 Evas_Object *obj, void *event_info __UNUSED__)
{
   Conformant_Part_Type part_type;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   part_type =  (ELM_CONFORM_INDICATOR_PART |
                 ELM_CONFORM_SOFTKEY_PART |
                 ELM_CONFORM_VIRTUAL_KEYPAD_PART |
                 ELM_CONFORM_SLIDING_WIN_PART);
   _conformant_part_sizing_eval(obj, part_type);
}
#endif

static void
_show_region_job(void *data)
{
   Evas_Object *focus_obj;
   Evas_Object *conformant = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(conformant);

   if (!wd) return;

   focus_obj = elm_widget_focused_object_get(conformant);
   if (focus_obj)
     {
        Evas_Coord x, y, w, h;

        elm_widget_show_region_get(focus_obj, &x, &y, &w, &h);

        if (h < _elm_config->finger_size)
          h = _elm_config->finger_size;

        elm_widget_show_region_set(focus_obj, x, y, w, h, EINA_TRUE);
     }

   wd->show_region_job = NULL;
}

// showing the focused/important region.
static void
_content_resize_event_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj
                         __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *conformant = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(conformant);

   if (!wd) return;
#ifdef HAVE_ELEMENTARY_X
   if ((wd->vkb_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
            && (!wd->is_visible)) return;
#endif

   if (wd->show_region_job) ecore_job_del(wd->show_region_job);
   wd->show_region_job = ecore_job_add(_show_region_job, conformant);
}

#ifdef HAVE_ELEMENTARY_X
static void
_update_autoscroll_objs(void *data)
{
   const char *type;
   int i;
   Evas_Object *sub, *top_scroller = NULL;
   Evas_Object *conformant = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;

   sub = elm_widget_focused_object_get(conformant);
   //Look up for Top most scroller in the Focus Object hierarchy inside Conformant.

   while (sub)
     {
        type = elm_widget_type_get(sub);
        if (!strcmp(type, "conformant")) break;
        for (i = 0; i < SUB_TYPE_COUNT; i++)
          if (!strcmp(type, sub_type[i]))
            {
               top_scroller = sub;
               break;
            }
        sub = elm_object_parent_widget_get(sub);
     }

   //If the scroller got changed by app, replace it.
   if (top_scroller != wd->scroller)
     {
        if (wd->scroller) evas_object_event_callback_del(wd->scroller,
                                                   EVAS_CALLBACK_RESIZE,
                                                   _content_resize_event_cb);
        wd->scroller = top_scroller;
        if (wd->scroller) evas_object_event_callback_add(wd->scroller,
                                                   EVAS_CALLBACK_RESIZE,
                                                   _content_resize_event_cb,
                                                   data);
     }
}

static Eina_Bool
_prop_change(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Property *ev;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_PASS_ON;
   ev = event;
   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ZONE)
     {
        Conformant_Part_Type part_type;

        part_type =  (ELM_CONFORM_INDICATOR_PART |
                      ELM_CONFORM_SOFTKEY_PART |
                      ELM_CONFORM_VIRTUAL_KEYPAD_PART |
                      ELM_CONFORM_SLIDING_WIN_PART);
        _conformant_part_sizing_eval(data, part_type);
     }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_INDICATOR_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_SOFTKEY_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_SOFTKEY_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_VIRTUAL_KEYPAD_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_SLIDING_WIN_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_SLIDING_WIN_PART);
   else if (ev->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
     {
        Ecore_X_Window zone;

        zone = ecore_x_e_illume_zone_get(ev->win);
        wd->vkb_state = ecore_x_e_virtual_keyboard_state_get(zone);
        if (wd->vkb_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
          {
             evas_object_size_hint_min_set(wd->virtualkeypad, -1, 0);
             evas_object_size_hint_max_set(wd->virtualkeypad, -1, 0);
          }
        else
          _update_autoscroll_objs(data);
     }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_SLIDING_WIN_STATE)
     {
        Ecore_X_Window zone;

        zone = ecore_x_e_illume_zone_get(ev->win);
        wd->is_visible = ecore_x_e_illume_sliding_win_state_get(zone);

        if (!wd->is_visible)
          {
             evas_object_size_hint_min_set(wd->sliding_win, -1, 0);
             evas_object_size_hint_max_set(wd->sliding_win, -1, 0);
          }
        else
          _update_autoscroll_objs(data);
     }

   return ECORE_CALLBACK_PASS_ON;
}
#endif

EAPI Evas_Object *
elm_conformant_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "conformant");
   elm_widget_type_set(obj, "conformant");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "conformant", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   wd->layout = elm_layout_add(obj);
   edje_object_part_swallow(wd->base, "elm.swallow.content", wd->layout);
   elm_layout_theme_set(wd->layout, "conformant", "layout", "content");

   _swallow_conformant_parts(obj);

#ifdef HAVE_ELEMENTARY_X
   Evas_Object *top = elm_widget_top_get(obj);
   Ecore_X_Window xwin = elm_win_xwindow_get(top);

   if ((xwin) && (!elm_win_inlined_image_object_get(top)))
     {
        wd->prop_hdl = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                               _prop_change, obj);
        wd->vkb_state = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
     }
   // FIXME: get kbd region prop

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                       _conformant_move_resize_event_cb, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                       _conformant_move_resize_event_cb, obj);
#endif
   evas_object_smart_callback_add(wd->layout, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);
   return obj;
}

EAPI void
elm_conformant_content_set(Evas_Object *obj, Evas_Object *content)
{
   _content_set_hook(obj, NULL, content);
}

EAPI Evas_Object *
elm_conformant_content_get(const Evas_Object *obj)
{
   return _content_get_hook(obj, NULL);
}

EAPI Evas_Object *
elm_conformant_content_unset(Evas_Object *obj)
{
   return _content_unset_hook(obj, NULL);
}

EAPI Evas_Object *
elm_conformant_content_area_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   /*Finger waggle warning*/
   _elm_dangerous_call_check(__FUNCTION__);

   return wd->layout;
}

