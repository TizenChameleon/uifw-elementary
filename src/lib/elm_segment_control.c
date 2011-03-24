#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup SegmentControl SegmentControl
 * @ingroup Elementary
 *
 * SegmentControl Widget is a horizontal control made of multiple segments, each segment item
 * functioning similar to discrete two state button. A segmented control affords a compact means to group together a number of controls.
 * Only one Segment item can be at selected state. A segmented control item can display combination of Text and any Evas_Object like layout or other widget.
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *obj;
   Evas_Object *base;
   Eina_List *seg_items;
   int item_count;
   Elm_Segment_Item *selected_item;
   int item_width;
};

struct _Elm_Segment_Item
{
   Elm_Widget_Item base;
   Evas_Object *icon;
   Evas_Object *label;
   int seg_index;
};

static const char *widtype = NULL;
static void _sizing_eval(Evas_Object *obj);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _item_free(Elm_Segment_Item *it);
static void _segment_off(Elm_Segment_Item *it);
static void _segment_on(Elm_Segment_Item *it);
static void _position_items(Widget_Data *wd);
static void _on_move_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__);
static void _mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__);
static void _mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__);
static void _swallow_item_objects(Elm_Segment_Item *it);
static void _update_list(Widget_Data *wd);
static Elm_Segment_Item * _item_find(const Evas_Object *obj, int index);
static Elm_Segment_Item* _item_new(Evas_Object *obj, Evas_Object *icon, const char *label );

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd;
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord w, h;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   evas_object_size_hint_min_get(obj, &w, &h);
   if (w > minw) minw = w;
   if (h > minh) minh = h;
   evas_object_size_hint_min_set(obj, minw, minh);
}

static void
_del_hook(Evas_Object *obj)
{
   Elm_Segment_Item *it;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   EINA_LIST_FREE(wd->seg_items, it) _item_free(it);

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Segment_Item *it;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   _elm_theme_object_set(obj, wd->base, "segment_control", "base", elm_widget_style_get(obj));
   edje_object_scale_set(wd->base, elm_widget_scale_get(wd->base) *_elm_config->scale);

   EINA_LIST_FOREACH(wd->seg_items, l, it)
     {
        _elm_theme_object_set(obj, it->base.view, "segment_control", "item", elm_widget_style_get(obj));
        edje_object_scale_set(it->base.view, elm_widget_scale_get(it->base.view) *_elm_config->scale);
     }

   _update_list(wd);
   _sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
   _update_list(wd);
}

#if 0
static void *
_elm_list_data_get(const Eina_List *list)
{
   Elm_Segment_Item *it = eina_list_data_get(list);

   if (it) return NULL;

   edje_object_signal_emit(it->base.view, "elm,state,segment,selected", "elm");
   return it->base.view;
}

/* TODO Can focus stay on Evas_Object which is not a elm_widget ?? */
static Eina_Bool
_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   static int count=0;
   Widget_Data *;
   const Eina_List *items;
   void *(*list_data_get) (const Eina_List *list);

   wd = elm_widget_data_get(obj)
   if ((!wd)) return EINA_FALSE;

   /* Focus chain */
   /* TODO: Change this to use other chain */
   if ((items = elm_widget_focus_custom_chain_get(obj)))
     list_data_get = eina_list_data_get;
   else
     {
        items = wd->seg_items;
        list_data_get = _elm_list_data_get;
        if (!items) return EINA_FALSE;
     }
   return elm_widget_focus_list_next_get(obj, items, list_data_get, dir, next);
}
#endif

static void
_item_free(Elm_Segment_Item *it)
{
   Widget_Data *wd;

   if (!it) return;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   if(wd->selected_item == it) wd->selected_item = NULL;
   if(wd->seg_items) wd->seg_items = eina_list_remove(wd->seg_items, it);

   elm_widget_item_pre_notify_del(it);

   if (it->icon) evas_object_del(it->icon);
   if (it->label) evas_object_del(it->label);

   elm_widget_item_del(it);
}

static void
_segment_off(Elm_Segment_Item *it)
{
   Widget_Data *wd;
   char buf[4096];

   if (!it) return;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   edje_object_signal_emit(it->base.view, "elm,state,segment,normal", "elm");
   if (it->label)
     {
        snprintf(buf, sizeof(buf), "%s/segment_normal", elm_widget_style_get(wd->obj));
        elm_object_style_set(it->label, buf);
     }

   if (wd->selected_item == it) wd->selected_item = NULL;
}

static void
_segment_on(Elm_Segment_Item *it)
{
   Widget_Data *wd;
   char buf[4096];

   if (!it) return;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;
   if (it == wd->selected_item) return;

   if (wd->selected_item) _segment_off(wd->selected_item);

   edje_object_signal_emit(it->base.view, "elm,state,segment,selected", "elm");
   if (it->label)
     {
        snprintf(buf, sizeof(buf), "%s/segment_selected", elm_widget_style_get(wd->obj));
        elm_object_style_set(it->label, buf);
     }

   wd->selected_item = it;
   evas_object_smart_callback_call(wd->obj, "changed", (void*) it->seg_index);
}

static void
_position_items(Widget_Data *wd)
{
   Eina_List *l;
   Elm_Segment_Item *it;
   int bx, by, bw, bh, position;

   wd->item_count = eina_list_count(wd->seg_items);
   if (wd->item_count <= 0) return;

   evas_object_geometry_get(wd->base, &bx, &by, &bw, &bh);
   wd->item_width = bw / wd->item_count;

   position = bx;
   EINA_LIST_FOREACH(wd->seg_items, l, it)
     {
        evas_object_move(it->base.view, bx, by );
        evas_object_resize(it->base.view, wd->item_width, bh );
        bx += wd->item_width;
     }
}

static void
_on_move_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   Widget_Data *wd;
   Evas_Coord law = 0, lah = 0;
   Eina_List *l;
   Elm_Segment_Item *it = NULL;
   const char *lbl_area;

   wd = elm_widget_data_get(data);
   if (!wd) return;

   _position_items(wd);

   EINA_LIST_FOREACH(wd->seg_items, l, it)
     {
        lbl_area = edje_object_data_get(it->base.view, "label.wrap.part");
        if (it->label && lbl_area)
          {
             edje_object_part_geometry_get(it->base.view, lbl_area, NULL, NULL, &law, &lah);
             elm_label_wrap_width_set(it->label, law);
             elm_label_wrap_height_set(it->label, lah);
          }
     }
}

static void
_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
          void *event_info)
{
   Widget_Data *wd;
   Elm_Segment_Item *it;
   Evas_Event_Mouse_Up *ev;
   Evas_Coord x, y, w, h;
   char buf[4096];

   it = data;
   if (!it) return;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   if (elm_widget_disabled_get(wd->obj)) return;

   if (it == wd->selected_item) return;

   ev = event_info;
   evas_object_geometry_get(it->base.view, &x, &y, &w, &h);

   if((ev->output.x > x) && (ev->output.x < (x+w)) && (ev->output.y > y) && (ev->output.y < (y+h)))
     _segment_on(it);
   else
     {
        edje_object_signal_emit(it->base.view, "elm,state,segment,normal", "elm");
        if (it->label)
          {
             snprintf(buf, sizeof(buf), "%s/segment_normal", elm_widget_style_get(obj));
             elm_object_style_set(it->label, buf);
          }
     }
}

static void
_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
            void *event_info __UNUSED__)
{
   Widget_Data *wd;
   Elm_Segment_Item *it;
   char buf[4096];

   it = data;
   if (!it) return;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   if (elm_widget_disabled_get(wd->obj)) return;

   if (it == wd->selected_item) return;

   edje_object_signal_emit(it->base.view, "elm,state,segment,pressed", "elm");
   if (it->label)
     {
        snprintf(buf, sizeof(buf), "%s/segment_pressed", elm_widget_style_get(wd->obj));
        elm_object_style_set(it->label, buf);
     }
}

static void
_swallow_item_objects(Elm_Segment_Item *it)
{
   Evas_Coord law = 0, lah = 0;
   const char *lbl_area;

   if (!it) return;

   if (it->icon)
     {
        edje_object_part_swallow(it->base.view, "elm.swallow.icon", it->icon);
        edje_object_signal_emit(it->base.view, "elm,state,icon,visible", "elm");
     }
   else
     edje_object_signal_emit(it->base.view, "elm,state,icon,hidden", "elm");

   if (it->label)
     {
        edje_object_part_swallow(it->base.view, "elm.swallow.label", it->label);
        edje_object_signal_emit(it->base.view, "elm,state,text,visible", "elm");

        lbl_area = edje_object_data_get(it->base.view, "label.wrap.part");
        if (lbl_area)
          {
             edje_object_part_geometry_get(it->base.view, lbl_area, NULL, NULL, &law, &lah );
             elm_label_wrap_width_set(it->label, law);
             elm_label_wrap_height_set(it->label, lah);
          }
     }
   else
     edje_object_signal_emit(it->base.view, "elm,state,text,hidden", "elm");
}

static void
_update_list(Widget_Data *wd)
{
   Eina_List *l;
   Elm_Segment_Item *it = NULL;
   int index = 0;
   char buf[4096];

   _position_items(wd);

   if (wd->item_count == 1)
     {
        it = eina_list_nth(wd->seg_items, 0);
        it->seg_index = 0;

        //Set the segment type
        edje_object_signal_emit(it->base.view, "elm,type,segment,single", "elm");

        //Set the segment state
        if (wd->selected_item == it)
          {
             edje_object_signal_emit(it->base.view, "elm,state,segment,selected", "elm");
             if (it->label)
               {
                  snprintf(buf, sizeof(buf), "%s/segment_selected", elm_widget_style_get(wd->obj));
                  elm_object_style_set(it->label, buf);
               }
          }
        else
          edje_object_signal_emit(it->base.view, "elm,state,segment,normal", "elm");

        if (elm_widget_disabled_get(wd->obj))
          {
             edje_object_signal_emit(it->base.view, "elm,state,disabled", "elm");
             if (it->label)
               {
                  snprintf(buf, sizeof(buf), "%s/segment_disabled", elm_widget_style_get(wd->obj));
                  elm_object_style_set(it->label, buf);
               }
          }

        _swallow_item_objects(it);

        return;
     }

   EINA_LIST_FOREACH(wd->seg_items, l, it)
     {
        it->seg_index = index;

        //Set the segment type
        if(index == 0)
          edje_object_signal_emit(it->base.view, "elm,type,segment,left", "elm");
        else if(index == wd->item_count-1)
          edje_object_signal_emit(it->base.view, "elm,type,segment,right", "elm");
        else
          edje_object_signal_emit(it->base.view, "elm,type,segment,middle", "elm");

        //Set the segment state
        if (wd->selected_item == it)
          {
             edje_object_signal_emit(it->base.view, "elm,state,segment,selected", "elm");
             if (it->label)
               {
                  snprintf(buf, sizeof(buf), "%s/segment_selected", elm_widget_style_get(wd->obj));
                  elm_object_style_set(it->label, buf);
               }
          }
        else
          edje_object_signal_emit(it->base.view, "elm,state,segment,normal", "elm");

        if (elm_widget_disabled_get(wd->obj))
           {
              edje_object_signal_emit(it->base.view, "elm,state,disabled", "elm");
              if (it->label)
                {
                   snprintf(buf, sizeof(buf), "%s/segment_disabled", elm_widget_style_get(wd->obj));
                   elm_object_style_set(it->label, buf);
                }
           }

        _swallow_item_objects(it);

        index++;
     }
}

static Elm_Segment_Item *
_item_find(const Evas_Object *obj, int index)
{
   Widget_Data *wd;
   Elm_Segment_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = eina_list_nth(wd->seg_items, index);
   return it;
}

static Elm_Segment_Item*
_item_new(Evas_Object *obj, Evas_Object *icon, const char *label )
{
   Elm_Segment_Item *it;
   Widget_Data *wd;
   char buf[4096];

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = elm_widget_item_new(obj, Elm_Segment_Item);
   if (!it) return NULL;
   elm_widget_item_data_set(it, wd);

   it->base.view = edje_object_add(evas_object_evas_get(obj));
   edje_object_scale_set(it->base.view, elm_widget_scale_get(it->base.view) *_elm_config->scale);
   evas_object_smart_member_add(it->base.view, obj);
   elm_widget_sub_object_add(obj, it->base.view);
   _elm_theme_object_set(obj, it->base.view, "segment_control", "item", elm_object_style_get(obj));

   if (label)
     {
        it->label = elm_label_add(obj);
        elm_widget_sub_object_add(it->base.view, it->label);
        snprintf(buf, sizeof(buf), "%s/segment_normal", elm_widget_style_get(obj));
        elm_object_style_set(it->label, buf);
        elm_label_label_set(it->label, label);
        elm_label_ellipsis_set(it->label, EINA_TRUE);
        evas_object_show(it->label);
     }

   it->icon = icon;
   if (it->icon) elm_widget_sub_object_add(it->base.view, it->icon);
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, it);
   evas_object_event_callback_add(it->base.view, EVAS_CALLBACK_MOUSE_UP, _mouse_up, it);
   evas_object_show(it->base.view);

   return it;
}

/**
 * Create new SegmentControl.
 * @param [in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Evas_Object *
elm_segment_control_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "segment_control");
   elm_widget_type_set(obj, "segment_control");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);

#if 0
   /* TODO Can focus stay on Evas_Object which is not a elm_widget ?? */
   elm_widget_focus_next_hook_set(obj, _focus_next_hook);
#endif

   wd->obj = obj;

   wd->base = edje_object_add(e);
   edje_object_scale_set(wd->base, elm_widget_scale_get(wd->base) *_elm_config->scale);
   _elm_theme_object_set(obj, wd->base, "segment_control", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _on_move_resize, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _on_move_resize, obj);
   _sizing_eval(obj);

   return obj;
}

/**
 * Add new segment item to SegmentControl.
 * @param [in] obj The SegmentControl object
 * @param [in] icon Any Objects like icon, Label, layout etc
 * @param [in] label The label for added segment item. Note that, NULL is different from empty string "".
 * @return The new segment item or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Elm_Segment_Item *
elm_segment_control_item_add(Evas_Object *obj, Evas_Object *icon,
                             const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Segment_Item *it;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(obj, icon, label);
   if (!it) return NULL;

   wd->seg_items = eina_list_append(wd->seg_items, it);
   _update_list(wd);

   return it;
}

/**
 * Insert a new segment item to SegmentControl.
 * @param [in] obj The SegmentControl object
 * @param [in] icon Any Objects like icon, Label, layout etc
 * @param [in] label The label for added segment item. Note that, NULL is different from empty string "".
 * @param [in] index Segment item location. Value should be between 0 and
 *              Existing total item count( @see elm_segment_control_item_count_get() )
 * @return The new segment item or NULL if it cannot be created
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Elm_Segment_Item *
elm_segment_control_item_insert_at(Evas_Object *obj, Evas_Object *icon,
                                   const char *label, int index)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Segment_Item *it, *it_rel;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (index < 0) index = 0;

   it = _item_new(obj, icon, label);
   if (!it) return NULL;

   it_rel = _item_find(obj, index);
   if (it_rel)
     wd->seg_items = eina_list_prepend_relative(wd->seg_items, it, it_rel);
   else
     wd->seg_items = eina_list_append(wd->seg_items, it);

   _update_list(wd);
   return it;
}

/**
 * Delete a segment item from SegmentControl
 * @param [in] obj The SegmentControl object
 * @param [in] it The segment item to be deleted
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_del(Elm_Segment_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it);
   Widget_Data *wd;

   wd = elm_widget_item_data_get(it);
   if(!wd) return;

   _item_free(it);
   _update_list(wd);
}

/**
 * Delete a segment item of given index from SegmentControl
 * @param [in] obj The SegmentControl object
 * @param [in] index The position at which segment item to be deleted
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_del_at(Evas_Object *obj, int index)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Elm_Segment_Item *it;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = _item_find(obj, index);
   if (!it) return;
   _item_free(it);
   _update_list(wd);
}

/**
 * Get the label of a segment item.
 * @param [in] obj The SegmentControl object
 * @param [in] index The index of the segment item
 * @return The label of the segment item
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI const char*
elm_segment_control_item_label_get(Evas_Object *obj, int index)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Segment_Item *it;

   it = _item_find(obj, index);
   if (it && it->label) return elm_label_label_get(it->label);

   return NULL;
}

/**
 * Set the label of a segment item.
 * @param [in] it The SegmentControl Item
 * @param [in] label New label text.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_label_set(Elm_Segment_Item* it, const char* label)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it);
   Widget_Data *wd;
   char buf[4096];

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   if (!label && !it->label) return; //No label, return
   if (label && !it->label) // Create Label Object
     {
        it->label = elm_label_add(it->base.view);
        elm_widget_sub_object_add(it->base.view, it->label);
        elm_label_label_set(it->label, label);
        elm_label_ellipsis_set(it->label, EINA_TRUE);
        evas_object_show(it->label);

        if(wd->selected_item == it )
          {
             snprintf(buf, sizeof(buf), "%s/segment_selected", elm_widget_style_get(wd->obj));
             elm_object_style_set(it->label, buf);
          }
        else
          {
             snprintf(buf, sizeof(buf), "%s/segment_normal", elm_widget_style_get(wd->obj));
             elm_object_style_set(it->label, buf);
          }
     }
   else if (!label && it->label) // Delete Label Object
     {
        evas_object_del(it->label);
        it->label = NULL;
     }
   else // Update the text
     elm_label_label_set(it->label, label);

   _swallow_item_objects( it );
}

/**
 * Get the icon of a segment item of SegmentControl
 * @param [in] obj The SegmentControl object
 * @param [in] index The index of the segment item
 * @return The icon object.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Evas_Object *
elm_segment_control_item_icon_get(const Evas_Object *obj, int index)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Segment_Item *it;

   it = _item_find(obj, index);
   if (it) return it->icon;

   return NULL;
}

/**
 * Set the Icon to the segment item
 * @param [in] it The SegmentControl Item
 * @param [in] icon Objects like Layout, Icon, Label etc...
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_icon_set(Elm_Segment_Item *it, Evas_Object *icon)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it);

   //Remove the existing icon
   if (it->icon)
     {
        edje_object_part_unswallow(it->base.view, it->icon);
        evas_object_del(it->icon);
        it->icon = NULL;
     }

   it->icon = icon;
   if (it->icon) elm_widget_sub_object_add(it->base.view, it->icon);
   _swallow_item_objects( it );
}

/**
 * Get the Segment items count from SegmentControl
 * @param [in] obj The SegmentControl object
 * @return Segment items count.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI int
elm_segment_control_item_count_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return 0;

   return eina_list_count(wd->seg_items);
}

/**
 * Get the base object of segment item.
 * @param [in] it The Segment item
 * @return obj The base object of the segment item.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Evas_Object*
elm_segment_control_item_object_get(const Elm_Segment_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, NULL);

   return it->base.view;
}

/**
 * Get the selected segment item in the SegmentControl
 * @param [in] obj The SegmentControl object
 * @return Selected Segment Item. NULL if none of segment item is selected.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Elm_Segment_Item*
elm_segment_control_item_selected_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return wd->selected_item;
}

/**
 * Select/unselect a particular segment item of SegmentControl
 * @param [in] it The Segment item that is to be selected or unselected.
 * @param [in] select Passing EINA_TRUE will select the segment item and EINA_FALSE will unselect.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI void
elm_segment_control_item_selected_set(Elm_Segment_Item *it, Eina_Bool select)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it);
   Widget_Data *wd;

   wd = elm_widget_item_data_get(it);
   if (!wd) return;

   if (it == wd->selected_item)
     {
        if (select) return;  //already in selected selected state.

        //unselect case
        _segment_off(it);
     }
   else
     _segment_on(it);

   return;
}

/**
 * Get the Segment Item from the specified Index.
 * @param [in] obj The Segment Control object.
 * @param [in] index The index of the segment item.
 * @return The Segment item.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Elm_Segment_Item *
elm_segment_control_item_get(const Evas_Object *obj, int index)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Segment_Item *it;

   it = _item_find(obj, index);

   return it;
}

/**
 * Get the index of a Segment item in the SegmentControl
 * @param [in] it The Segment Item.
 * @return Segment Item index.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI int
elm_segment_control_item_index_get(const Elm_Segment_Item *it)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, -1);

   return it->seg_index;
}

//////////////////////////////////  BEGIN  //////////////////////////////////////////////
/////////////////////////// OLD SLP APIs - TO BE DEPRECATED /////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

EAPI int
elm_segment_control_get_segment_count(Evas_Object *obj)
{
   fprintf(stderr, "=============================> Warning!!! <========================\n");
   fprintf(stderr, "==> elm_segment_control_get_segment_count() is deprecated. <=======\n");
   fprintf(stderr, "===> Please use elm_segment_control_item_count_get() instead. <====\n");
   fprintf(stderr, "===================================================================\n");
   return elm_segment_control_item_count_get(obj);
}

EAPI Elm_Segment_Item *
elm_segment_control_selected_segment_get(const Evas_Object *obj, int *value)
{
   Elm_Segment_Item *it;
   it = elm_segment_control_item_selected_get(obj);
   if (!it) return NULL;
   *value = it->seg_index;

   fprintf(stderr, "=============================> Warning!!! <===========================\n");
   fprintf(stderr, "==> elm_segment_control_selected_segment_get() is deprecated. <=======\n");
   fprintf(stderr, "===> Please use elm_segment_control_item_selected_get() instead. <====\n");
   fprintf(stderr, "======================================================================\n");
   return it;
}

EAPI Evas_Object *
elm_segment_control_get_segment_icon_at(Evas_Object *obj, unsigned int index)
{
   fprintf(stderr, "=============================> Warning!!! <==========================\n");
   fprintf(stderr, "==> elm_segment_control_get_segment_icon_at() is deprecated. <=======\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_icon_get() instead. <=====\n");
   fprintf(stderr, "=====================================================================\n");
   return elm_segment_control_item_icon_get(obj, index);
}

EAPI const char *
elm_segment_control_get_segment_label_at(Evas_Object *obj, unsigned int index)
{
   fprintf(stderr, "=============================> Warning!!! <===========================\n");
   fprintf(stderr, "==> elm_segment_control_get_segment_label_at() is deprecated. <=======\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_label_get() instead. <=====\n");
   fprintf(stderr, "======================================================================\n");
   return elm_segment_control_item_label_get(obj, index);
}

EAPI void
elm_segment_control_delete_segment_at(Evas_Object *obj, unsigned int index,
                                      Eina_Bool animate)
{
   fprintf(stderr, "=============================> Warning!!! <========================\n");
   fprintf(stderr, "==> elm_segment_control_delete_segment_at() is deprecated. <=======\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_del_at() instead. <=====\n");
   fprintf(stderr, "===================================================================\n");
   elm_segment_control_item_del_at(obj, index);
}

EAPI void
elm_segment_control_delete_segment(Evas_Object *obj, Elm_Segment_Item *item,
                                   Eina_Bool animate)
{
   fprintf(stderr, "=============================> Warning!!! <=====================\n");
   fprintf(stderr, "==> elm_segment_control_delete_segment() is deprecated. <=======\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_del() instead. <=====\n");
   fprintf(stderr, "================================================================\n");
   elm_segment_control_item_del(item);
}

EAPI void
elm_segment_control_insert_segment_at(Evas_Object *obj, Evas_Object *icon,
                                      const char *label, unsigned int index,
                                      Eina_Bool animate)
{
   fprintf(stderr, "=============================> Warning!!! <===========================\n");
   fprintf(stderr, "==> elm_segment_control_insert_segment_at() is deprecated. <==========\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_insert_at() instead. <=====\n");
   fprintf(stderr, "======================================================================\n");
   elm_segment_control_item_insert_at(obj, icon, label, index);
}

EAPI Elm_Segment_Item *
elm_segment_control_add_segment(Evas_Object *obj, Evas_Object *icon,
                                const char *label, Eina_Bool animate)
{
   fprintf(stderr, "=============================> Warning!!! <=====================\n");
   fprintf(stderr, "==> elm_segment_control_add_segment() is deprecated. <==========\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_add() instead. <=====\n");
   fprintf(stderr, "================================================================\n");
   return elm_segment_control_item_add(obj, icon, label);
}

EAPI Evas_Object *
elm_segment_control_item_label_object_set(Elm_Segment_Item *item, char *label)
{
   fprintf(stderr, "=============================> Warning!!! <===============================\n");
   fprintf(stderr, "==> elm_segment_control_item_label_object_set() is deprecated. <==========\n");
   fprintf(stderr, "=====> Please use elm_segment_control_item_label_set() instead. <=========\n");
   fprintf(stderr, "==========================================================================\n");
   elm_segment_control_item_label_set(item, label);
   if (item) return item->label;
   else return NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// OLD SLP APIs - TO BE DEPRECATED /////////////////////////////
///////////////////////////////////  END ////////////////////////////////////////////////

