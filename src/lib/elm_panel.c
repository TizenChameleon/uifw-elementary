#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Panel Panel
 *
 * A panel is a type of animated container that contains subobjects.  It
 * can be expanded or contracted.
 *
 * Orientations are as follows:
 * ELM_PANEL_ORIENT_TOP
 * ELM_PANEL_ORIENT_BOTTOM
 * ELM_PANEL_ORIENT_LEFT
 * ELM_PANEL_ORIENT_RIGHT
 * NOTE: Only LEFT orientation is implemented.
 *
 * THIS WIDGET IS UNDER CONSTRUCTION!
 */

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data 
{
   Evas_Object *scr, *bx;
   Elm_Panel_Orient orient;
   Eina_Bool hidden : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data);
static void _toggle_panel(void *data, Evas_Object *obj, const char *emission, const char *source);

static void 
_del_hook(Evas_Object *obj) 
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void 
_theme_hook(Evas_Object *obj) 
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_object_theme_set(obj, wd->scr, "panel", "base", elm_widget_style_get(obj));
//   scale = (elm_widget_scale_get(obj) * _elm_config->scale);
//   edje_object_scale_set(wd->scr, scale);
   _sizing_eval(obj);
}

static void 
_sizing_eval(Evas_Object *obj) 
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord mw = -1, mh = -1;
   Evas_Coord vw = 0, vh = 0;
   Evas_Coord w, h;
   if (!wd) return;
   evas_object_smart_calculate(wd->bx);
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr), 
                             &mw, &mh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < mw) w = mw;
   if (h < mh) h = mh;
   evas_object_resize(wd->scr, w, h);

   evas_object_size_hint_min_get(wd->bx, &mw, &mh);
   if (w > mw) mw = w;
   if (h > mh) mh = h;
   evas_object_resize(wd->bx, mw, mh);

   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   mw = mw + (w - vw);
   mh = mh + (h - vh);
   evas_object_size_hint_min_set(obj, mw, mh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void 
_resize(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord mw, mh, vw, vh, w, h;
   if (!wd) return;
   elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
   evas_object_size_hint_min_get(wd->bx, &mw, &mh);
   evas_object_geometry_get(wd->bx, NULL, NULL, &w, &h);
   if ((vw >= mw) || (vh >= mh))
     {
        if ((w != vw) || (h != vh)) evas_object_resize(wd->bx, vw, vh);
     }
}

static void 
_layout(Evas_Object *o, Evas_Object_Box_Data *priv, void *data) 
{
   Widget_Data *wd = data;
   if (!wd) return;
   _els_box_layout(o, priv, EINA_TRUE, EINA_FALSE);
}

static void 
_toggle_panel(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__) 
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (wd->hidden) 
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr), 
                                "elm,action,show", "elm");
        wd->hidden = EINA_FALSE;
     }
   else
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr), 
                                "elm,action,hide", "elm");
        wd->hidden = EINA_TRUE;
     }
}

/**
 * Adds a panel object
 *
 * @param parent The parent object
 *
 * @return The panel object, or NULL on failure
 *
 * @ingroup Panel
 */
EAPI Evas_Object *
elm_panel_add(Evas_Object *parent) 
{
   Evas_Object *obj;
   Evas *evas;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   evas = evas_object_evas_get(parent);
   obj = elm_widget_add(evas);
   ELM_SET_WIDTYPE(widtype, "panel");
   elm_widget_type_set(obj, "panel");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->scr = elm_smart_scroller_add(evas);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "panel", "base", "left");
   elm_smart_scroller_bounce_allow_set(wd->scr, 0, 0);
   elm_widget_resize_object_set(obj, wd->scr);
   elm_smart_scroller_policy_set(wd->scr, ELM_SMART_SCROLLER_POLICY_OFF, 
                                 ELM_SMART_SCROLLER_POLICY_OFF);

   wd->hidden = EINA_FALSE;
   wd->orient = ELM_PANEL_ORIENT_LEFT;

   wd->bx = evas_object_box_add(evas);
   evas_object_size_hint_align_set(wd->bx, 0.5, 0.5);
   evas_object_box_layout_set(wd->bx, _layout, wd, NULL);
   elm_widget_sub_object_add(obj, wd->bx);
   elm_smart_scroller_child_set(wd->scr, wd->bx);
   evas_object_show(wd->bx);

   edje_object_signal_callback_add(elm_smart_scroller_edje_object_get(wd->scr), 
                                   "elm,action,panel,toggle", "*", 
                                   _toggle_panel, obj);

   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_RESIZE, _resize, obj);

   _sizing_eval(obj);
   return obj;
}

/**
 * Sets the orientation of the panel
 *
 * @param parent The parent object
 * @param orient The panel orientation.  Can be one of the following:
 * ELM_PANEL_ORIENT_TOP
 * ELM_PANEL_ORIENT_BOTTOM
 * ELM_PANEL_ORIENT_LEFT
 * ELM_PANEL_ORIENT_RIGHT
 *
 * NOTE: Currently all orientations but LEFT are unimplemented.
 *
 * @ingroup Panel
 */
EAPI void 
elm_panel_orient_set(Evas_Object *obj, Elm_Panel_Orient orient) 
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->orient = orient;
   switch (orient) 
     {
     case ELM_PANEL_ORIENT_TOP:
     case ELM_PANEL_ORIENT_BOTTOM:
        break;
     case ELM_PANEL_ORIENT_LEFT:
        elm_smart_scroller_object_theme_set(obj, wd->scr, "panel", "base", "left");
     case ELM_PANEL_ORIENT_RIGHT:
        break;
     default:
        break;
     }
   _sizing_eval(obj);
}

/**
 * Get the orientation of the panel.
 *
 * @param obj The panel object
 * @return The Elm_Panel_Orient, or ELM_PANEL_ORIENT_LEFT on failure.
 *
 * @ingroup Panel
 */
EAPI Elm_Panel_Orient
elm_panel_orient_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_PANEL_ORIENT_LEFT;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_PANEL_ORIENT_LEFT;
   return wd->orient;   
}

/**
 * Set the content of the panel.
 *
 * @param obj The panel object
 * @param content The panel content
 *
 * @ingroup Panel
 */
EAPI void 
elm_panel_content_set(Evas_Object *obj, Evas_Object *content) 
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_box_remove_all(wd->bx, EINA_TRUE);
   if (!content) return;
   evas_object_box_append(wd->bx, content);
   evas_object_show(content);
   _sizing_eval(obj);
}

/**
 * Set the state of the panel.
 *
 * @param obj The panel object
 * @param hidden If true, the panel will run the edje animation to contract
 *
 * @ingroup Panel
 */
EAPI void
elm_panel_hidden_set(Evas_Object *obj, Eina_Bool hidden)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->hidden == hidden) return;
   wd->hidden = hidden;
   _toggle_panel(obj, NULL, "elm,action,panel,toggle", "*");
}

/**
 * Get the state of the panel.
 *
 * @param obj The panel object
 * @param hidden If true, the panel is in the "hide" state
 *
 * @ingroup Panel
 */
EAPI Eina_Bool
elm_panel_hidden_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->hidden;
}

/**
 * Toggle the state of the panel from code
 *
 * @param obj The panel object
 *
 * @ingroup Panel
 */
EAPI void
elm_panel_toggle(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->hidden = !(wd->hidden);
   _toggle_panel(obj, NULL, "elm,action,panel,toggle", "*");
}
