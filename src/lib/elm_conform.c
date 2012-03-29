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
   Evas_Object *indicator;
   Evas_Object *softkey;
   Evas_Object *virtualkeypad;
   Evas_Object *clipboard;
   Evas_Object *content;
   Evas_Object *scroller;
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
   ELM_CONFORM_CLIPBOARD_PART    = 8
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
static void _sizing_eval(Evas_Object *obj);
static void _show_region_job(void *data);
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
   if (wd->show_region_job) ecore_job_del(wd->show_region_job);
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
   _swallow_conformant_parts(obj);

   if (wd->content)
     edje_object_part_swallow(wd->base, "elm.swallow.content", wd->content);
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
   evas_object_event_callback_del_full(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
   edje_object_part_unswallow(wd->base, content);
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

/* Example of env vars:
 * ILLUME_KBD="0, 0, 800, 301"
 * ILLUME_IND=0,0,800,32
 * ILLUME_STK="0,568,800,32
 */
static Eina_Bool
_conformant_part_geometry_env_get(const char *part, int *sx, int *sy, int *sw, int *sh)
{
   const char delimiters[] = " ,;";
   char *env_val, *token;
   char buf[PATH_MAX];
   int tsx, tsy, tsw;

   if (!(env_val = getenv(part))) return EINA_FALSE;

   /* strtok would modify env var if not copied to a buffer */
   strncpy(buf, env_val, sizeof(buf));

   token = strtok(buf, delimiters);
   if (!token) return EINA_FALSE;
   tsx = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   tsy = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   tsw = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   *sh = atoi(token);

   *sx = tsx;
   *sy = tsy;
   *sw = tsw;

   return EINA_TRUE;
}

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
   Ecore_X_Window zone = 0, xwin;
   Evas_Object *top;
   int sx = -1, sy = -1, sw = -1, sh = -1;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   top = elm_widget_top_get(obj);
#ifdef HAVE_ELEMENTARY_X
   xwin = elm_win_xwindow_get(top);
   if (xwin)
     zone = ecore_x_e_illume_zone_get(xwin);
#endif

   if (part_type & ELM_CONFORM_INDICATOR_PART)
     {
        if ((!_conformant_part_geometry_env_get("ILLUME_IND",
                                                &sx, &sy, &sw, &sh)) && (xwin))
          {
#ifdef HAVE_ELEMENTARY_X
           //No information of the indicator geometry, reset the geometry.
             if (!ecore_x_e_illume_indicator_geometry_get(zone, &sx, &sy, &sw, &sh))
               sx = sy = sw = sh = 0;
#else
             ;
#endif
          }
        _conformant_part_size_set(obj, wd->indicator, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_VIRTUAL_KEYPAD_PART)
     {
        if ((!_conformant_part_geometry_env_get("ILLUME_KBD",
                                                &sx, &sy, &sw, &sh)) && (xwin))
          {
#ifdef HAVE_ELEMENTARY_X
           //No information of the keyboard geometry, reset the geometry.
           if (!ecore_x_e_illume_keyboard_geometry_get(zone, &sx, &sy, &sw, &sh))
             sx = sy = sw = sh = 0;
#else
             ;
#endif
          }
        _conformant_part_size_set(obj, wd->virtualkeypad, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_SOFTKEY_PART)
     {
        if ((!_conformant_part_geometry_env_get("ILLUME_STK",
                                                &sx, &sy, &sw, &sh)) && (xwin))
          {
#ifdef HAVE_ELEMENTARY_X
           //No information of the softkey geometry, reset the geometry.
           if (!ecore_x_e_illume_softkey_geometry_get(zone, &sx, &sy, &sw, &sh))
             sx = sy = sw = sh = 0;
#else
             ;
#endif
          }
        _conformant_part_size_set(obj, wd->softkey, sx, sy, sw, sh);
     }
   if (part_type & ELM_CONFORM_CLIPBOARD_PART)
     {
        if ((!_conformant_part_geometry_env_get("ILLUME_CB",
                                                &sx, &sy, &sw, &sh)) && (xwin))
          {
#ifdef HAVE_ELEMENTARY_X
           //No information of the clipboard geometry, reset the geometry.
           if (!ecore_x_e_illume_clipboard_geometry_get(zone, &sx, &sy, &sw, &sh))
             sx = sy = sw = sh = 0;
#else
             ;
#endif
          }
        _conformant_part_size_set(obj, wd->clipboard, sx, sy, sw, sh);
     }
}

static void
_swallow_conformant_parts(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas *e = evas_object_evas_get(obj);
   wd->scroller = NULL;

   //Indicator
   if (!wd->indicator)
     {
        wd->indicator = evas_object_rectangle_add(e);
        elm_widget_sub_object_add(obj, wd->indicator);
        evas_object_size_hint_min_set(wd->indicator, -1, 0);
        evas_object_size_hint_max_set(wd->indicator, -1, 0);
     }
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_INDICATOR_PART);
   evas_object_color_set(wd->indicator, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.indicator", wd->indicator);

   //Virtual Keyboard
   if (!wd->virtualkeypad)
     {
        wd->virtualkeypad = evas_object_rectangle_add(e);
        elm_widget_sub_object_add(obj, wd->virtualkeypad);
        evas_object_size_hint_min_set(wd->virtualkeypad, -1, 0);
        evas_object_size_hint_max_set(wd->virtualkeypad, -1, 0);
     }
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_VIRTUAL_KEYPAD_PART);
   evas_object_color_set(wd->virtualkeypad, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.virtualkeypad",
                            wd->virtualkeypad);
   //Clipboard
   if (!wd->clipboard)
     {
        wd->clipboard = evas_object_rectangle_add(e);
        elm_widget_sub_object_add(obj, wd->clipboard);
        evas_object_size_hint_min_set(wd->clipboard, -1, 0);
        evas_object_size_hint_max_set(wd->clipboard, -1, 0);
     }
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_CLIPBOARD_PART);
   evas_object_color_set(wd->clipboard, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.clipboard",
                            wd->clipboard);

   //Softkey
   if (!wd->softkey)
     {
        wd->softkey = evas_object_rectangle_add(e);
        elm_widget_sub_object_add(obj, wd->softkey);
        evas_object_size_hint_min_set(wd->softkey, -1, 0);
        evas_object_size_hint_max_set(wd->softkey, -1, 0);
     }
   else
     _conformant_part_sizing_eval(obj, ELM_CONFORM_SOFTKEY_PART);
   evas_object_color_set(wd->softkey, 0, 0, 0, 0);
   edje_object_part_swallow(wd->base, "elm.swallow.softkey", wd->softkey);
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
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
if (!wd) return;
   if (sub == wd->content)
     {
        evas_object_event_callback_del_full(sub,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
        wd->content = NULL;
        _sizing_eval(obj);
     }
}

/* unused now - but meant to be for making sure the focused widget is always
 * visible when the vkbd comes and goes by moving the conformant obj (and thus
 * its children) to  show the focused widget (and if focus changes follow)

static Evas_Object *
_focus_object_get(const Evas_Object *obj)
{
   Evas_Object *win, *foc;

   win = elm_widget_top_get(obj);
   if (!win) return NULL;
   foc = elm_widget_top_get(win);
}

static void
_focus_object_region_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   evas_object_geometry_get(obj, x, y, w, h);
}

static void
_focus_change_del(void *data, Evas_Object *obj, void *event_info)
{
   // called from toplevel when the focused window shanges
}

static void
_autoscroll_move(Evas_Object *obj)
{
   // move conformant edje by delta to show focused widget
}

static void
_autoscroll_mode_enable(Evas_Object *obj)
{
// called when autoscroll mode should be on - content area smaller than
// its min size
// 1. get focused object
// 2. if not in visible conformant area calculate delta needed to
//    get it in
// 3. store delta and call _autoscroll_move() which either asanimates
//    or jumps right there
}

static void
_autoscroll_mode_disable(Evas_Object *obj)
{
// called when autoscroll mode should be off - set delta to 0 and
// call _autoscroll_move()
}
 */

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
                 ELM_CONFORM_CLIPBOARD_PART);
   _conformant_part_sizing_eval(obj, part_type);
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
   if (wd->vkb_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) return;
#endif

   if (wd->show_region_job) ecore_job_del(wd->show_region_job);
   wd->show_region_job = ecore_job_add(_show_region_job, conformant);
}

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
                      ELM_CONFORM_CLIPBOARD_PART);
        _conformant_part_sizing_eval(data, part_type);
     }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_INDICATOR_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_SOFTKEY_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_SOFTKEY_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_VIRTUAL_KEYPAD_PART);
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_GEOMETRY)
     _conformant_part_sizing_eval(data, ELM_CONFORM_CLIPBOARD_PART);
   else if (ev->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
     {
        Ecore_X_Window zone;

        printf("Keyboard Geometry Changed\n");
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
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_STATE)
     {
        Ecore_X_Window zone;
        Ecore_X_Illume_Clipboard_State state;

        zone = ecore_x_e_illume_zone_get(ev->win);
        state = ecore_x_e_illume_clipboard_state_get(zone);

        if (state != ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
          {
             evas_object_size_hint_min_set(wd->clipboard, -1, 0);
             evas_object_size_hint_max_set(wd->clipboard, -1, 0);
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
   elm_widget_content_set_hook_set(obj, _content_set_hook);
   elm_widget_content_get_hook_set(obj, _content_get_hook);
   elm_widget_content_unset_hook_set(obj, _content_unset_hook);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "conformant", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

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
#endif

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                       _conformant_move_resize_event_cb, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                       _conformant_move_resize_event_cb, obj);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);
   return obj;
}
