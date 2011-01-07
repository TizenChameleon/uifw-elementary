#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup SegmentControl SegmentControl
 * @ingroup Elementary
 *
 * SegmentControl Widget is a horizontal control made of multiple segments, each segment item
 * functioning as a discrete two state button. A segmented control affords a compact means to group together a number of controls.
 * Only one Segment item can be at selected state. A segmented control item can display combination of Text and any Evas_Object like layout or other widget.
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *obj;
   Evas_Object *base;
   Eina_List *seg_items;
   int item_count;
   Elm_Segment_Item *selected_item;;
   int item_width;
};

struct _Elm_Segment_Item
{
   Evas_Object *base;
   Evas_Object *icon;
   Evas_Object *label;
   int seg_index;
   Widget_Data *wd;
};

static void _item_free(Evas_Object *obj, Elm_Segment_Item *it);
static void _del_hook(Evas_Object *obj);
static void _update_list(Evas_Object *obj);

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord w, h;

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
   Widget_Data *wd = elm_widget_data_get(obj);

   EINA_LIST_FREE(wd->seg_items, it) _item_free(obj, it);

   free(wd);
   return;
}

static void
_theme_hook(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   _elm_theme_object_set(obj, wd->base, "segment_control", "base",
                         elm_widget_style_get(obj));

   EINA_LIST_FOREACH(wd->seg_items, l, it)
     _elm_theme_object_set(obj, it->base, "segment_control", "item", elm_widget_style_get(obj));

   _update_list(obj);
   _sizing_eval(obj);
   return;
}

static void
_item_free(Evas_Object *obj, Elm_Segment_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!it || !wd) return;

   if (wd->selected_item == it) wd->selected_item = NULL;
   if (wd->seg_items) wd->seg_items = eina_list_remove(wd->seg_items, it);

   if (it->icon) evas_object_del(it->icon);
   if (it->label) evas_object_del(it->label);
   if (it->base) evas_object_del(it->base);
   free(it);
   it = NULL;

   return;
}

static void
_segment_off(Elm_Segment_Item *it)
{
   if (!it) return;

   edje_object_signal_emit(it->base, "elm,state,segment,normal", "elm");
   if (it->label) elm_object_style_set(it->label, "segment_normal");

   if (it->wd->selected_item == it) it->wd->selected_item = NULL;

   return;
}

static void
_segment_on(Elm_Segment_Item *it)
{
   if (!it || (it == it->wd->selected_item)) return;

   if (it->wd->selected_item) _segment_off(it->wd->selected_item);

   edje_object_signal_emit(it->base, "elm,state,segment,selected", "elm");

   if (it->label) elm_object_style_set(it->label, "segment_selected");

   it->wd->selected_item = it;
   evas_object_smart_callback_call(it->wd->obj, "changed", (void*) it->seg_index);

   return;
}

static void
_position_items(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Segment_Item *it;
   int bx, by, bw, bh, position;

   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->item_count = eina_list_count(wd->seg_items);
   if (wd->item_count <= 0) return;

   evas_object_geometry_get(wd->base, &bx, &by, &bw, &bh);
   wd->item_width = bw / wd->item_count;

   position = bx;
   EINA_LIST_FOREACH(wd->seg_items, l, it)
   {
      evas_object_move(it->base, bx, by );
      evas_object_resize(it->base, wd->item_width, bh );
      bx += wd->item_width;
   }
   return;
}

static void
_on_move_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord law = 0, lah = 0;
   Eina_List *l;
   Elm_Segment_Item *it = NULL;
   const char *lbl_area;

   Widget_Data *wd = elm_widget_data_get((Evas_Object *) data);
   if (!wd) return;
   _position_items((Evas_Object *) data);

   EINA_LIST_FOREACH(wd->seg_items, l, it)
   {
      lbl_area = edje_object_data_get(it->base, "label.wrap.part");
      if (it->label && lbl_area)
        {
           edje_object_part_geometry_get(it->base, lbl_area, NULL, NULL, &law, &lah);
           elm_label_wrap_width_set(it->label, law);
           elm_label_wrap_height_set(it->label, lah);
        }
   }
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Elm_Segment_Item *it = (Elm_Segment_Item *) data;
   if (!it) return;
   if (it == it->wd->selected_item) return;
   _segment_on(it);

   return;
}

static void
_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Elm_Segment_Item *it = (Elm_Segment_Item *) data;
   if (!it) return;
   if (it == it->wd->selected_item) return;

   edje_object_signal_emit(it->base, "elm,state,segment,pressed", "elm");
   if (it->label) elm_object_style_set(it->label, "segment_pressed");

   return;
}

static void
_swallow_item_objects(Elm_Segment_Item *it)
{
   Evas_Coord law = 0, lah = 0;
   const char *lbl_area;

   if (!it) return;

   if (it->icon)
     {
        edje_object_part_swallow(it->base, "elm.swallow.icon", it->icon);
        edje_object_signal_emit(it->base, "elm,state,icon,visible", "elm");
     }
   else
     edje_object_signal_emit(it->base, "elm,state,icon,hidden", "elm");

   if (it->label)
     {
        edje_object_part_swallow(it->base, "elm.swallow.label", it->label);
        edje_object_signal_emit(it->base, "elm,state,text,visible", "elm");

        lbl_area = edje_object_data_get(it->base, "label.wrap.part");
        if (lbl_area)
          {
             edje_object_part_geometry_get(it->base, lbl_area, NULL, NULL, &law, &lah );
             elm_label_wrap_width_set(it->label, law);
             elm_label_wrap_height_set(it->label, lah);
          }
     }
   else
     edje_object_signal_emit(it->base, "elm,state,text,hidden", "elm");
}

static void
_update_list(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Segment_Item *it = NULL;
   int index = 0;

   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   _position_items(obj);

   if (wd->item_count == 1)
     {
        it = (Elm_Segment_Item *) eina_list_nth(wd->seg_items, 0);
        it->seg_index = 0;

        //Set the segment type
        edje_object_signal_emit(it->base, "elm,type,segment,single", "elm");

        //Set the segment state
        if (wd->selected_item == it)
          {
             edje_object_signal_emit(it->base, "elm,state,segment,selected", "elm");
             if (it->label) elm_object_style_set(it->label, "segment_selected");
          }
        else
          edje_object_signal_emit(it->base, "elm,state,segment,normal", "elm");
        _swallow_item_objects(it);
        return;
     }

   EINA_LIST_FOREACH(wd->seg_items, l, it)
   {
      it->seg_index = index;

      //Set the segment type
      if (index == 0)
        edje_object_signal_emit(it->base, "elm,type,segment,left", "elm");
      else if (index == wd->item_count-1)
        edje_object_signal_emit(it->base, "elm,type,segment,right", "elm");
      else
        edje_object_signal_emit(it->base, "elm,type,segment,middle", "elm");

       //Set the segment state
      if (wd->selected_item == it )
        {
           edje_object_signal_emit(it->base, "elm,state,segment,selected", "elm");
           if (it->label) elm_object_style_set(it->label, "segment_selected");
        }
      else
        edje_object_signal_emit(it->base, "elm,state,segment,normal", "elm");

      _swallow_item_objects(it);
      index++;
   }
}

static Elm_Segment_Item *
_item_find(Evas_Object *obj, int index)
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = (Elm_Segment_Item *) eina_list_nth(wd->seg_items, index);
   return it;
}

static Elm_Segment_Item*
_item_new(Evas_Object *obj, Evas_Object *icon, const char *label )
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = calloc(1, sizeof(Elm_Segment_Item));
   if (!it) return NULL;
   it->wd = wd;

   it->base = edje_object_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(it->base, obj);
   elm_widget_sub_object_add(obj, it->base);
   _elm_theme_object_set(obj, it->base, "segment_control", "item", elm_object_style_get(obj));

   if (label)
     {
        it->label = elm_label_add(obj);
        elm_widget_sub_object_add(it->base, it->label);
        elm_object_style_set(it->label, "segment_normal");
        elm_label_label_set(it->label, label);
        elm_label_ellipsis_set(it->label, EINA_TRUE);
        evas_object_show(it->label);
     }

   it->icon = icon;
   if (it->icon) elm_widget_sub_object_add(it->base, it->icon);

   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, it);
   evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up,
                                  it);
   evas_object_show(it->base);

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

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "segment_control");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   wd->obj = obj;

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "segment_control", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _on_move_resize,
                                  obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _on_move_resize, obj);
   wd->item_count = 0;
   wd->selected_item = NULL;

   _sizing_eval(obj);

   return obj;
}

/**
 * Add new segment item to SegmentControl item.
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
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = _item_new(obj, icon, label);
   if (!it) return NULL;

   wd->seg_items = eina_list_append(wd->seg_items, it);
   _update_list(obj);

   return it;
}

/**
 * Insert a new segment item to SegmentControl item.
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
   Elm_Segment_Item *it, *it_rel;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (index < 0 || index > wd->item_count) return NULL;

   it = _item_new(obj, icon, label);
   if (!it) return NULL;

   it_rel = _item_find(obj, index);
   if (it_rel)
     wd->seg_items = eina_list_prepend_relative(wd->seg_items, it, it_rel);
   else
     wd->seg_items = eina_list_append(wd->seg_items, it);

   _update_list(obj);
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

elm_segment_control_item_del(Evas_Object *obj, Elm_Segment_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !it) return;

   _item_free(obj, it);
   _update_list(obj);

   return;
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
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = _item_find(obj, index);
   if (!it) return;
   _item_free(obj, it);
   _update_list(obj);

   return;
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
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

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
   if (!it) return;

   if (!label && !it->label) return; //No label, return

   if (label && !it->label) // Create Label Object
     {
        it->label = elm_label_add(it->base);
        elm_widget_sub_object_add(it->base, it->label);
        elm_label_label_set(it->label, label);
        elm_label_ellipsis_set(it->label, EINA_TRUE);
        evas_object_show(it->label);

        if (it->wd->selected_item == it)
          elm_object_style_set(it->label, "segment_selected");
        else
          elm_object_style_set(it->label, "segment_normal");
     }
   else if (!label && it->label) // Delete Label Object
     {
        evas_object_del(it->label);
        it->label = NULL;
     }
   else // Update the text
     elm_label_label_set(it->label, label);

   _swallow_item_objects( it );
   return;
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
elm_segment_control_item_icon_get(Evas_Object *obj, int index)
{
   Elm_Segment_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

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
   if (!it) return;

   //Remove the existing icon
   if (it->icon)
     {
        edje_object_part_unswallow(it->base, it->icon);
        evas_object_del(it->icon);
     }

   it->icon = icon;
   if (it->icon) elm_widget_sub_object_add(it->base, it->icon);
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
elm_segment_control_item_count_get(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;

   return wd->item_count;
}

/**
 * Get the base object of segment item.
 * @param [in] it The Segment item
 * @return obj The base object of the segment item.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI Evas_Object*
elm_segment_control_item_object_get(Elm_Segment_Item *it)
{
   if (!it) return NULL;
   return it->base;
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
   Widget_Data *wd = elm_widget_data_get(obj);
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
   if (!it) return;

   if (it == it->wd->selected_item)
     {
        if (select) return; //already in selected selected state.

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
elm_segment_control_item_get_at(Evas_Object *obj, int index)
{
   Elm_Segment_Item *it;
   it = _item_find(obj, index);

   return it;
}

/**
 * Get the index of a Segment item in the SegmentControl
 * @param [in] it The Segment Item.
 * @return Segment Item.
 *
 * @ingroup SegmentControl SegmentControl
 */
EAPI int
elm_segment_control_item_index_get(Elm_Segment_Item *it)
{
   if (!it) return -1;
   return it->seg_index;
}

//////////////////////////////////  BEGIN  //////////////////////////////////////////////
/////////////////////////// OLD SLP APIs - TO BE DEPRECATED /////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

EAPI int
elm_segment_control_get_segment_count(Evas_Object *obj)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_count_get()\ninstead of elm_segment_control_get_segment_count()\n#####\n");
   return elm_segment_control_item_count_get(obj);
}

EAPI Elm_Segment_Item *
elm_segment_control_selected_segment_get(const Evas_Object *obj, int *value)
{
   Elm_Segment_Item *it;
   it = elm_segment_control_item_selected_get(obj);
   if (!it) return NULL;
   *value = it->seg_index;

   printf(
          "#####\nWARNING: Use elm_segment_control_item_selected_get() &\nelm_segment_control_item_index_get()\n instead of elm_segment_control_selected_segment_get()\n#####\n");
   return it;
}

EAPI Evas_Object *
elm_segment_control_get_segment_icon_at(Evas_Object *obj, unsigned int index)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_icon_get()\ninstead of elm_segment_control_get_segment_icon_at()\n#####\n");
   return elm_segment_control_item_icon_get(obj, index);
}

EAPI const char *
elm_segment_control_get_segment_label_at(Evas_Object *obj, unsigned int index)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_label_get()\n instead of elm_segment_control_get_segment_label_at() \n#####\n");
   return elm_segment_control_item_label_get(obj, index);
}

EAPI void
elm_segment_control_delete_segment_at(Evas_Object *obj, unsigned int index,
                                      Eina_Bool animate)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_del_at()\ninstead of elm_segment_control_delete_segment_at() \n#####\n");
   elm_segment_control_item_del_at(obj, index);
}

EAPI void
elm_segment_control_delete_segment(Evas_Object *obj, Elm_Segment_Item *item,
                                   Eina_Bool animate)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_del()\ninstead of elm_segment_control_delete_segment()\n#####\n");
   elm_segment_control_item_del(obj, item);
}

EAPI void
elm_segment_control_insert_segment_at(Evas_Object *obj, Evas_Object *icon,
                                      const char *label, unsigned int index,
                                      Eina_Bool animate)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_insert_at()\ninstead of elm_segment_control_insert_segment_at()\n#####\n");
   elm_segment_control_item_insert_at(obj, icon, label, index);
}

EAPI Elm_Segment_Item *
elm_segment_control_add_segment(Evas_Object *obj, Evas_Object *icon,
                                const char *label, Eina_Bool animate)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_add()\n instead of elm_segment_control_add_segment()\n#####\n");
   return elm_segment_control_item_add(obj, icon, label);
}

EAPI Evas_Object *
elm_segment_control_item_label_object_set(Elm_Segment_Item *item, char *label)
{
   printf(
          "#####\nWARNING: Use elm_segment_control_item_label_set()\n instead of elm_segment_control_item_label_object_set()\n#####\n");
   elm_segment_control_item_label_set(item, label);
   if (item) return item->label;
   else return NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// OLD SLP APIs - TO BE DEPRECATED /////////////////////////////
///////////////////////////////////  END ////////////////////////////////////////////////

