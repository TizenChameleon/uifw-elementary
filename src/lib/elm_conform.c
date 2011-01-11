#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Conformant Conformant
 * @ingroup Elementary
 * 
 * The aim is to provide a widget that can be used in elementary apps to 
 * account for space taken up by the indicator & softkey windows when running 
 * the illume2 module of E17.
 */

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
   Evas_Object *base;
   Evas_Object *shelf, *panel, *virtualkeypad;
   Evas_Object *content;
   Evas_Object *scroller;
   Ecore_Event_Handler *prop_hdl;
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Virtual_Keyboard_State vkb_state;
#endif
};

/* local function prototypes */
static const char *widtype = NULL;
static void
_del_hook(Evas_Object *obj);
static void
_theme_hook(Evas_Object *obj);
static void
_swallow_conformant_parts(Evas_Object *obj);
static void
_sizing_eval(Evas_Object *obj);
static Eina_Bool
_prop_change(void *data, int type, void *event);

/* local functions */
static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->prop_hdl) ecore_event_handler_del(wd->prop_hdl);
   if (wd->shelf) evas_object_del(wd->shelf);
   if (wd->virtualkeypad) evas_object_del(wd->virtualkeypad);
   if (wd->panel) evas_object_del(wd->panel);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_theme_object_set(obj, wd->base, "conformant", "base",
                         elm_widget_style_get(obj));
   _swallow_conformant_parts(obj);

   if (wd->content) edje_object_part_swallow(wd->base, "elm.swallow.content",
                                             wd->content);
   edje_object_scale_set(wd->base, elm_widget_scale_get(obj)
            * _elm_config->scale);
   _sizing_eval(obj);
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

static void
_swallow_conformant_parts(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   int sh = -1;
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window zone, xwin;

   xwin = elm_win_xwindow_get(obj);
   zone = ecore_x_e_illume_zone_get(xwin);

   ecore_x_e_illume_indicator_geometry_get(zone, NULL, NULL, NULL, &sh);
#endif
   if (sh < 0) sh = 0;
   if (!wd->shelf)
      wd->shelf = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(wd->shelf, 0, 0, 0, 0);
   evas_object_size_hint_min_set(wd->shelf, -1, sh);
   evas_object_size_hint_max_set(wd->shelf, -1, sh);
   edje_object_part_swallow(wd->base, "elm.swallow.shelf", wd->shelf);

   wd->scroller = NULL;
   sh = -1;
#ifdef HAVE_ELEMENTARY_X
   ecore_x_e_illume_keyboard_geometry_get(zone, NULL, NULL, NULL, &sh);
#endif
   if (sh < 0) sh = 0;
   if (!wd->virtualkeypad)
      wd->virtualkeypad = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(wd->virtualkeypad, 0, 0, 0, 0);
   evas_object_size_hint_min_set(wd->virtualkeypad, -1, sh);
   evas_object_size_hint_max_set(wd->virtualkeypad, -1, sh);
   edje_object_part_swallow(wd->base, "elm.swallow.virtualkeypad",
                            wd->virtualkeypad);

   sh = -1;
#ifdef HAVE_ELEMENTARY_X
   ecore_x_e_illume_softkey_geometry_get(zone, NULL, NULL, NULL, &sh);
#endif
   if (sh < 0) sh = 0;
   if (!wd->panel)
      wd->panel = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(wd->panel, 0, 0, 0, 0);
   evas_object_size_hint_min_set(wd->panel, -1, sh);
   evas_object_size_hint_max_set(wd->panel, -1, sh);
   edje_object_part_swallow(wd->base, "elm.swallow.panel", wd->panel);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
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
      evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
               _changed_size_hints, obj);
      wd->content = NULL;
      _sizing_eval(obj);
   }
}

static void
_content_resize_event_cb(void *data, Evas *e, Evas_Object *obj,
                         void *event_info)
{
   Evas_Object *focus_obj;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->vkb_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) return;
   focus_obj = elm_widget_focused_object_get(obj);
   if (focus_obj)
   {
      Evas_Coord x, y, w, h;

      elm_widget_show_region_get(focus_obj, &x, &y, &w, &h);

      if (h < _elm_config->finger_size)
         h = _elm_config->finger_size;
      else
         h = 1 + h; //elm_widget_show_region_set expects some change, to redo the job.
      elm_widget_show_region_set(focus_obj, x, y, w, h);
   }
}

static void
_update_autoscroll_objs(void *data)
{
   char *type;
   Evas_Object *sub, *top_scroller = NULL;
   Evas_Object *conformant = (Evas_Object *) data;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   sub = elm_widget_focused_object_get(conformant);

   //Look up for Top most scroller in the Focus Object hierarchy inside Conformant.
   while (sub)
   {
      type = elm_widget_type_get(sub);
      if (!strcmp(type, "conformant")) break;
      if (!strcmp(type, "scroller") || !strcmp(type, "genlist"))
         top_scroller = sub;

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
#ifdef HAVE_ELEMENTARY_X
   int indicator_height=0;
   Ecore_X_Virtual_Keyboard_State virt_keypad_state = ECORE_X_VIRTUAL_KEYBOARD_STATE_UNKNOWN;
   Ecore_X_Event_Window_Property *ev;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_PASS_ON;
   ev = event;
   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ZONE)
   {
      Ecore_X_Window zone;
      int sh = -1;

      zone = ecore_x_e_illume_zone_get(ev->win);
      ecore_x_e_illume_indicator_geometry_get(zone, NULL, NULL, NULL, &sh);
      if (sh < 0) sh = indicator_height;

      evas_object_size_hint_min_set(wd->shelf, -1, sh);
      evas_object_size_hint_max_set(wd->shelf, -1, sh);
      sh = -1;
      zone = ecore_x_e_illume_zone_get(ev->win);
      ecore_x_e_illume_keyboard_geometry_get(zone, NULL, NULL, NULL, &sh);
      if (sh < 0) sh = 0;
      evas_object_size_hint_min_set(wd->virtualkeypad, -1, sh);
      evas_object_size_hint_max_set(wd->virtualkeypad, -1, sh);
      sh = -1;
      ecore_x_e_illume_softkey_geometry_get(zone, NULL, NULL, NULL, &sh);
      if (sh < 0) sh = 0;
      evas_object_size_hint_min_set(wd->panel, -1, sh);
      evas_object_size_hint_max_set(wd->panel, -1, sh);
   }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_GEOMETRY)
   {
      Ecore_X_Window zone;
      int sh = -1;

      zone = ecore_x_e_illume_zone_get(ev->win);
      ecore_x_e_illume_indicator_geometry_get(zone, NULL, NULL, NULL, &sh);
      if (sh < 0) sh = 0;
      evas_object_size_hint_min_set(wd->shelf, -1, sh);
      evas_object_size_hint_max_set(wd->shelf, -1, sh);
   }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_SOFTKEY_GEOMETRY)
   {
      Ecore_X_Window zone;
      int sh = -1;

      zone = ecore_x_e_illume_zone_get(ev->win);
      ecore_x_e_illume_softkey_geometry_get(zone, NULL, NULL, NULL, &sh);
      if (sh < 0) sh = 0;
      evas_object_size_hint_min_set(wd->panel, -1, sh);
      evas_object_size_hint_max_set(wd->panel, -1, sh);
   }
   else if (ev->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY)
   {
      Ecore_X_Window zone;
      int ky = -1, kh = -1;

      zone = ecore_x_e_illume_zone_get(ev->win);
      ecore_x_e_illume_keyboard_geometry_get(zone, NULL, &ky, NULL, &kh);
      if (kh < 0) kh = 0;
      evas_object_size_hint_min_set(wd->virtualkeypad, -1, kh);
      evas_object_size_hint_max_set(wd->virtualkeypad, -1, kh);
   }
   else if (ev->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
   {
      Ecore_X_Window zone;
      zone = ecore_x_e_illume_zone_get(ev->win);
      wd->vkb_state = ecore_x_e_virtual_keyboard_state_get(zone);

      if(wd->vkb_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
      {
         evas_object_size_hint_min_set(wd->virtualkeypad, -1, 0);
         evas_object_size_hint_max_set(wd->virtualkeypad, -1, 0);
      }
      else
      _update_autoscroll_objs(data);
   }
#endif

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * Add a new Conformant object
 * 
 * @param parent The parent object
 * @return The new conformant object or NULL if it cannot be created
 * 
 * @ingroup Conformant
 */
EAPI Evas_Object *
elm_conformant_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *evas;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);

   evas = evas_object_evas_get(parent);

   obj = elm_widget_add(evas);
   ELM_SET_WIDTYPE(widtype, "conformant");
   elm_widget_type_set(obj, "conformant");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->base = edje_object_add(evas);
   _elm_theme_object_set(obj, wd->base, "conformant", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);
   _swallow_conformant_parts(obj);

#ifdef HAVE_ELEMENTARY_X
   wd->prop_hdl = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
            _prop_change, obj);
   wd->vkb_state = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
#endif

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the content of the conformant widget
 *
 * Once the content object is set, a previously set one will be deleted.
 * If you want to keep that old content object, use the
 * elm_conformat_content_unset() function.
 *
 * @param obj The conformant object
 * @return The content that was being used
 *
 * @ingroup Conformant
 */
EAPI void
elm_conformant_content_set(Evas_Object *obj, Evas_Object *content)
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
      edje_object_part_swallow(wd->base, "elm.swallow.content", content);
   }
   _sizing_eval(obj);
}

/**
 * Unset the content of the conformant widget
 *
 * Unparent and return the content object which was set for this widget;
 *
 * @param obj The conformant object
 * @return The content that was being used
 *
 * @ingroup Conformant
 */
EAPI Evas_Object *
elm_conformant_content_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *content;
   if (!wd) return NULL;
   if (!wd->content) return NULL;
   content = wd->content;
   elm_widget_sub_object_del(obj, wd->content);
   edje_object_part_unswallow(wd->base, wd->content);
   wd->content = NULL;
   return content;
}
