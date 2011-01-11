/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/*
 * SLP
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */

/**
 *
 * @defgroup Actionslider Actionslider
 * @ingroup Elementary
 *
 * This is an actionslider.
 */

#include <Elementary.h>
#include <math.h>
#include "elm_priv.h"

/**
 * internal data structure of actionslider object
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object	*as;		// actionslider
   Evas_Object	*icon;		// an icon for a button or a bar
   Evas_Object	*icon_fake;		// an icon for a button or a bar

   // setting
   Elm_Actionslider_Magnet_Pos	magnet_position, enabled_position;
   const char *text_left, *text_right, *text_center, *text_button;

   // status
   Eina_Bool	mouse_down;
   Eina_Bool	mouse_hold;

   // icon animation
   Ecore_Animator	*icon_animator;
   double		final_position;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

/*
 * callback functions
 */
static void _icon_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _icon_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/*
 * internal functions
 */
static Eina_Bool _icon_animation(void *data);

static const char *widtype = NULL;

#define SIG_CHANGED "position"
#define SIG_SELECTED "selected"

static const Evas_Smart_Cb_Description _signals[] =
{
   {SIG_CHANGED, ""},
   {SIG_SELECTED, ""},
   {NULL, NULL}
};


static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->icon) 
     {
        evas_object_del(wd->icon);
        wd->icon = NULL;
     }
   if (wd->icon_fake) 
     {
        evas_object_del(wd->icon_fake);
        wd->icon_fake = NULL;
     }
   if (wd->text_left) eina_stringshare_del(wd->text_left);
   if (wd->text_right) eina_stringshare_del(wd->text_right);
   if (wd->text_center) eina_stringshare_del(wd->text_center);
   if (wd->text_button) eina_stringshare_del(wd->text_button);
   if (wd->as) 
     {
        evas_object_smart_member_del(wd->as);
        evas_object_del(wd->as);
        wd->as = NULL;
     }
   free(wd);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;

   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(wd->icon, minw, minh);
   evas_object_size_hint_max_set(wd->icon, -1, -1);

   minw = -1;
   minh = -1;
   elm_coords_finger_size_adjust(3, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->as, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (edje_object_part_swallow_get(wd->as, "elm.swallow.icon") == NULL) 
     edje_object_part_unswallow(wd->as, wd->icon);
   if (edje_object_part_swallow_get(wd->as, "elm.swallow.space") == NULL) 
     edje_object_part_unswallow(wd->as, wd->icon_fake);

   _elm_theme_object_set(obj, wd->as, "actionslider", "base", elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->icon, "actionslider", "icon", elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->icon_fake, "actionslider", "icon", elm_widget_style_get(obj));
   edje_object_part_swallow(wd->as, "elm.swallow.icon", wd->icon);
   edje_object_part_swallow(wd->as, "elm.swallow.space", wd->icon_fake);
   edje_object_part_text_set(wd->as, "elm.text.left", wd->text_left);
   edje_object_part_text_set(wd->as, "elm.text.right", wd->text_right);
   edje_object_part_text_set(wd->as, "elm.text.center", wd->text_center);
   edje_object_message_signal_process(wd->as);
   //edje_object_scale_set(wd->as, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
//   Widget_Data *wd = elm_widget_data_get(obj);
/*
   TODO
   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->btn, "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(wd->btn, "elm,state,enabled", "elm");
*/
}

static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
//   Widget_Data *wd = elm_widget_data_get(obj);
//   Evas_Object *sub = event_info;
}

static void
_icon_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get((Evas_Object *)data);

   wd->mouse_down = EINA_TRUE;
}

static void
_icon_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Object *as = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(as);
   double pos = 0.0;

   elm_actionslider_hold(as, EINA_FALSE);
   if (wd->mouse_down == EINA_FALSE) return;

   edje_object_part_drag_value_get(wd->as, "elm.swallow.icon", &pos, NULL);

   if (pos == 0.0) 
     evas_object_smart_callback_call(as, SIG_CHANGED, "left");
   else if (pos == 1.0) 
     evas_object_smart_callback_call(as, SIG_CHANGED, "right");
   else if (pos >= 0.495 && pos <= 0.505) 
     evas_object_smart_callback_call(as, SIG_CHANGED, "center");

/*
 * TODO
if (wd->type == ELM_ACTIONSLIDER_TYPE_BAR_GREEN ||
wd->type == ELM_ACTIONSLIDER_TYPE_BAR_RED ) {
if (pos == 1.0) {
//edje_object_signal_emit(wd->as, "elm,show,bar,text,center", "elm");
edje_object_signal_emit(wd->as, "elm,show,text,center", "elm");
} else {
//edje_object_signal_emit(wd->as, "elm,hide,bar,text,center", "elm");
edje_object_signal_emit(wd->as, "elm,hide,text,center", "elm");
}
}
*/
}

static void
_icon_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get((Evas_Object *)data);
   double position = 0.0;

   wd->mouse_down = EINA_FALSE;

   if (wd->mouse_hold == EINA_FALSE) 
     {
        if (wd->magnet_position == ELM_ACTIONSLIDER_MAGNET_LEFT) 
          wd->final_position = 0.0;
        else if (wd->magnet_position == ELM_ACTIONSLIDER_MAGNET_RIGHT) 
          wd->final_position = 1.0;
        else if (wd->magnet_position == ELM_ACTIONSLIDER_MAGNET_CENTER) 
          wd->final_position = 0.5; 
        else if ( wd->magnet_position == ELM_ACTIONSLIDER_MAGNET_BOTH) 
          {
             edje_object_part_drag_value_get(wd->as, "elm.swallow.icon", &position, NULL);
             if (position <= 0.5) 
               wd->final_position = 0.0;
             else 
               wd->final_position = 1.0;
          }
        wd->icon_animator = ecore_animator_add(_icon_animation, wd);
     }
}

static Eina_Bool
_icon_animation(void *data)
{
   Widget_Data *wd = (Widget_Data *)data;
   double cur_position = 0.0, new_position = 0.0;
   double move_amount = 0.05;
   Eina_Bool flag_finish_animation = EINA_FALSE;

   edje_object_part_drag_value_get(wd->as, "elm.swallow.icon", &cur_position, NULL);

   if ( (wd->final_position == 0.0) ||(wd->final_position == 0.5 && cur_position >= wd->final_position) ) 
     {
        new_position = cur_position - move_amount;
        if (new_position <= wd->final_position) 
          {
             new_position = wd->final_position;
             flag_finish_animation = EINA_TRUE;
          }
     } 
   else if ((wd->final_position == 1.0) || (wd->final_position == 0.5 && cur_position < wd->final_position) ) 
     {
        new_position = cur_position + move_amount;
        if (new_position >= wd->final_position) 
          {
             new_position = wd->final_position;
             flag_finish_animation = EINA_TRUE;
             /*
             // TODO
             if (wd->type == ELM_ACTIONSLIDER_TYPE_BAR_GREEN ||
               wd->type == ELM_ACTIONSLIDER_TYPE_BAR_RED ) {
               edje_object_signal_emit(wd->as, "elm,show,bar,text,center", "elm");
             }
             */
          }
     }
   edje_object_part_drag_value_set(wd->as, "elm.swallow.icon", new_position, 0.5);

   if (flag_finish_animation)
     {
        if ((!wd->final_position) &&
            (wd->enabled_position & ELM_ACTIONSLIDER_MAGNET_LEFT))
          evas_object_smart_callback_call(data, SIG_SELECTED,
                                          (void *)wd->text_left);
        else if ((wd->final_position == 0.5) &&
                 (wd->enabled_position & ELM_ACTIONSLIDER_MAGNET_CENTER))
          evas_object_smart_callback_call(data, SIG_SELECTED,
                                          (void *)wd->text_center);
        else if ((wd->final_position == 1) &&
                 (wd->enabled_position & ELM_ACTIONSLIDER_MAGNET_RIGHT))
          evas_object_smart_callback_call(data, SIG_SELECTED,
                                          (void *)wd->text_right);
        return EINA_FALSE;
     }
   return EINA_TRUE;

}

/**
 * Add a new actionslider to the parent.
 *
 * @param[in] parent The parent object
 * @return The new actionslider object or NULL if it cannot be created
 *
 * @ingroup Actionslider
 */
EAPI Evas_Object *
elm_actionslider_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd = NULL;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (e == NULL) return NULL;
   ELM_SET_WIDTYPE(widtype, "actionslider");
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "actionslider");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);

   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);

   wd->mouse_down = EINA_FALSE;
   wd->mouse_hold = EINA_FALSE;
   wd->enabled_position = ELM_ACTIONSLIDER_MAGNET_ALL;

   // load background edj
   wd->as = edje_object_add(e);
   if(wd->as == NULL) 
     {
        printf("Cannot load actionslider edj!\n");
        return NULL;
     }
   _elm_theme_object_set(obj, wd->as, "actionslider", "base", "default");
   elm_widget_resize_object_set(obj, wd->as);

   // load icon
   wd->icon = edje_object_add(e);
   if (wd->icon == NULL) 
     {
        printf("Cannot load acitionslider icon!\n");
        return NULL;
     }
   evas_object_smart_member_add(wd->icon, obj);
   _elm_theme_object_set(obj, wd->icon, "actionslider", "icon", "default");
   edje_object_part_swallow(wd->as, "elm.swallow.icon", wd->icon);

   wd->icon_fake = edje_object_add(e);
   evas_object_smart_member_add(wd->icon_fake, obj);
   _elm_theme_object_set(obj, wd->icon_fake, "actionslider", "icon", "default");
   edje_object_part_swallow(wd->as, "elm.swallow.space", wd->icon_fake);

   // event callbacks
   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_event_callback_add(wd->icon, EVAS_CALLBACK_MOUSE_DOWN, _icon_down_cb, obj);
   evas_object_event_callback_add(wd->icon, EVAS_CALLBACK_MOUSE_MOVE, _icon_move_cb, obj);
   evas_object_event_callback_add(wd->icon, EVAS_CALLBACK_MOUSE_UP, _icon_up_cb, obj);

   return obj;
}

/*
EAPI Evas_Object *
elm_actionslider_add_with_set(Evas_Object *parent, Elm_Actionslider_Icon_Pos pos, Elm_Actionslider_Magnet_Pos magnet, const char* label_left, const char* label_center, const char* label_right)
{
   Evas_Object *obj;

   obj = elm_actionslider_add(parent);

   elm_actionslider_icon_set(obj, pos);
   elm_actionslider_magnet_set(obj, magnet);
   if (label_left != NULL)
     elm_actionslider_label_set(obj, ELM_ACTIONSLIDER_LABEL_LEFT, label_left);
   if (label_center != NULL)
     elm_actionslider_label_set(obj, ELM_ACTIONSLIDER_LABEL_CENTER, label_center);
   if (label_right != NULL)
     elm_actionslider_label_set(obj, ELM_ACTIONSLIDER_LABEL_RIGHT, label_right);

   return obj;
}
*/

/**
 * Set actionslider indicator position. 
 *
 * @param[in] obj The actionslider object. 
 * @param[in] pos The position of the indicator.
 * (ELM_ACTIONSLIDER_INDICATOR_LEFT, ELM_ACTIONSLIDER_INDICATOR_RIGHT,
 *  ELM_ACTIONSLIDER_INDICATOR_CENTER)
 *
 * @ingroup Actionslider
 */
EAPI void
elm_actionslider_indicator_pos_set(Evas_Object *obj, Elm_Actionslider_Indicator_Pos pos)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd = elm_widget_data_get(obj);
   double position = 0.0;

   if (pos == ELM_ACTIONSLIDER_INDICATOR_LEFT) position = 0.0;
   else if (pos == ELM_ACTIONSLIDER_INDICATOR_RIGHT) position = 1.0;
   else if (pos == ELM_ACTIONSLIDER_INDICATOR_CENTER) position = 0.5;
   else position = 0.0;

   edje_object_part_drag_value_set(wd->as, "elm.swallow.icon", position, 0.5);
}

/**
 * Get actionslider indicator position.
 *
 * @param obj The actionslider object.
 * @return The position of the indicator.
 *
 * @ingroup Actionslider
 */
EAPI Elm_Actionslider_Indicator_Pos
elm_actionslider_indicator_pos_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_ACTIONSLIDER_INDICATOR_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);
   double position;
   if (!wd) return ELM_ACTIONSLIDER_INDICATOR_NONE;

   edje_object_part_drag_value_get(wd->as, "elm.swallow.icon", &position, NULL);
   if (position < 0.3)
     return ELM_ACTIONSLIDER_INDICATOR_LEFT;
   else if (position < 0.7)
     return ELM_ACTIONSLIDER_INDICATOR_CENTER;
   else
     return ELM_ACTIONSLIDER_INDICATOR_RIGHT;
}
/**
 * Set actionslider magnet position. 
 *
 * @param[in] obj The actionslider object. 
 * @param[in] pos The position of the magnet.
 * (ELM_ACTIONSLIDER_MAGNET_LEFT, ELM_ACTIONSLIDER_MAGNET_RIGHT,
 *  ELM_ACTIONSLIDER_MAGNET_BOTH, ELM_ACTIONSLIDER_MAGNET_CENTER)
 *
 * @ingroup Actionslider
 */
EAPI void
elm_actionslider_magnet_pos_set(Evas_Object *obj, Elm_Actionslider_Magnet_Pos pos)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->magnet_position = pos;
}

/**
 * Get actionslider magnet position.
 *
 * @param obj The actionslider object.
 * @return The positions with magnet property.
 *
 * @ingroup Actionslider
 */
EAPI Elm_Actionslider_Magnet_Pos
elm_actionslider_magnet_pos_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_ACTIONSLIDER_MAGNET_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_ACTIONSLIDER_MAGNET_NONE;
   return wd->magnet_position;
}

/**
 * Set actionslider enabled position.
 *
 * All the positions are enabled by default.
 *
 * @param obj The actionslider object.
 * @param pos Bit mask indicating the enabled positions.
 * Example: use (ELM_ACTIONSLIDER_MAGNET_LEFT | ELM_ACTIONSLIDER_MAGNET_RIGHT)
 * to enable both positions, so the user can select it.
 *
 * @ingroup Actionslider
 */
EAPI void
elm_actionslider_enabled_pos_set(Evas_Object *obj, Elm_Actionslider_Magnet_Pos pos)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->enabled_position = pos;
}

/**
 * Get actionslider enabled position.
 *
 * All the positions are enabled by default.
 *
 * @param obj The actionslider object.
 * @return The enabled positions.
 *
 * @ingroup Actionslider
 */
EAPI Elm_Actionslider_Magnet_Pos
elm_actionslider_enabled_pos_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_ACTIONSLIDER_MAGNET_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_ACTIONSLIDER_MAGNET_NONE;
   return wd->enabled_position;
}

/**
 * Set actionslider label.
 *
 * @param[in] obj The actionslider object
 * @param[in] pos The position of the label.
 * (ELM_ACTIONSLIDER_LABEL_LEFT, ELM_ACTIONSLIDER_LABEL_RIGHT)
 * @param label The label which is going to be set.
 *
 * @ingroup Actionslider
 */
EAPI void 
elm_actionslider_label_set(Evas_Object *obj, Elm_Actionslider_Label_Pos pos, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if(label == NULL) label = "";

   if (pos == ELM_ACTIONSLIDER_LABEL_RIGHT) 
     {
        if (wd->text_right)  eina_stringshare_del(wd->text_right);
        wd->text_right = eina_stringshare_add(label);
        edje_object_part_text_set(wd->as, "elm.text.right", label);
     } 
   else if (pos == ELM_ACTIONSLIDER_LABEL_LEFT) 
     {
        if (wd->text_left)  eina_stringshare_del(wd->text_left);
        wd->text_left = eina_stringshare_add(label);
        edje_object_part_text_set(wd->as, "elm.text.left", label);
     } 
   else if (pos == ELM_ACTIONSLIDER_LABEL_CENTER) 
     {
        if (wd->text_center)  eina_stringshare_del(wd->text_center);
        wd->text_center = eina_stringshare_add(label);
        edje_object_part_text_set(wd->as, "elm.text.center", label);
     }
   else if (pos == ELM_ACTIONSLIDER_LABEL_BUTTON)
     {
        if (wd->text_button) eina_stringshare_del(wd->text_button);
        wd->text_button = eina_stringshare_add(label);
        edje_object_part_text_set(wd->icon, "elm.text.button", label);

        /* Resize button width */
        Evas_Object *txt;
        txt = (Evas_Object *)edje_object_part_object_get (wd->icon, "elm.text.button");
        if (txt != NULL) 
          {
             evas_object_text_text_set (txt, wd->text_button);

             Evas_Coord x,y,w,h;
             evas_object_geometry_get (txt, &x,&y,&w,&h);

             char *data_left = NULL, *data_right = NULL;
             int pad_left = 0, pad_right = 0;

             data_left = (char *)edje_object_data_get (wd->icon, "left");
             data_right = (char *)edje_object_data_get (wd->icon, "right");

             if (data_left) pad_left = atoi(data_left);
             if (data_right) pad_right = atoi(data_right);

             evas_object_size_hint_min_set (wd->icon, w + pad_left + pad_right, 0);
             evas_object_size_hint_min_set (wd->icon_fake, w + pad_left + pad_right, 0);
          }
     }

}

/**
 * Get actionslider labels.
 *
 * @param obj The actionslider object
 * @param left_label A char** to place the left_label of @p obj into
 * @param center_label A char** to place the center_label of @p obj into
 * @param right_label A char** to place the right_label of @p obj into
 *
 * @ingroup Actionslider
 */
EAPI void
elm_actionslider_labels_get(const Evas_Object *obj, const char **left_label, const char **center_label, const char **right_label)
{
   if (left_label) *left_label= NULL;
   if (center_label) *center_label= NULL;
   if (right_label) *right_label= NULL;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (left_label) *left_label = wd->text_left;
   if (center_label) *center_label = wd->text_center;
   if (right_label) *right_label = wd->text_right;
}

/**
 * Set the label used on the indicator object.
 *
 * @param obj The actionslider object
 * @param label The label which is going to be set.
 *
 * @ingroup Actionslider
 */
EAPI void 
elm_actionslider_indicator_label_set(Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   eina_stringshare_replace(&wd->text_button, label);
   edje_object_part_text_set(wd->as, "elm.text.button", wd->text_button);
}

/**
 * Get the label used on the indicator object.
 *
 * @param obj The actionslider object
 * @return The indicator label
 *
 * @ingroup Actionslider
 */
EAPI const char *
elm_actionslider_indicator_label_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->text_button;
}

/**
 * Hold actionslider object movement.
 *
 * @param[in] obj The actionslider object
 * @param[in] flag Actionslider hold/release
 * (EINA_TURE = hold/EIN_FALSE = release)
 *
 * @ingroup Actionslider
 */
EAPI void
elm_actionslider_hold(Evas_Object *obj, Eina_Bool flag)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   wd->mouse_hold = flag;
}
