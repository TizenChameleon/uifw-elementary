#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Hoversel
 *
 * A hoversel is a button that pops up a list of items (automatically
 * choosing the direction to display) that have a lable and/or an icon to
 * select from. It is a convenience widget to avoid the need to do all the
 * piecing together yourself. It is intended for a small number of items in
 * the hoversel menu (no more than 8), though is capable of many more.
 *
 * Signals that you can add callbacks for are:
 *
 * clicked  - the user clicked the hoversel button and popped up the sel
 *
 * selected - an item in the hoversel list is selected. event_info is the item
 * selected - Elm_Hoversel_Item
 *
 * dismissed - the hover is dismissed
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *btn, *hover;
   Evas_Object *hover_parent;
   Eina_List *items;
   Eina_Bool horizontal : 1;
};

struct _Elm_Hoversel_Item
{
   Evas_Object *obj;
   const char *label;
   const char *icon_file;
   const char *icon_group;
   Elm_Icon_Type icon_type;
   Evas_Smart_Cb func;
   Evas_Smart_Cb del_cb;
   void *data;
};

static const char *widtype = NULL;
static void _del_pre_hook(Evas_Object *obj);
static void _del_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _parent_del(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_pre_hook(Evas_Object *obj)
{
   Elm_Hoversel_Item *it;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_hoversel_hover_end(obj);
   elm_hoversel_hover_parent_set(obj, NULL);
   EINA_LIST_FREE(wd->items, it)
     {
	if (it->del_cb) it->del_cb((void *)it->data, it->obj, it);
	eina_stringshare_del(it->label);
	eina_stringshare_del(it->icon_file);
	eina_stringshare_del(it->icon_group);
	free(it);
     }
}

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
   char buf[4096];
   if (!wd) return;
   elm_hoversel_hover_end(obj);
   if (wd->horizontal)
     snprintf(buf, sizeof(buf), "hoversel_horizontal/%s", elm_widget_style_get(obj));
   else
     snprintf(buf, sizeof(buf), "hoversel_vertical/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->btn, buf);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_disabled_get(obj))
     elm_widget_disabled_set(wd->btn, 1);
   else
     elm_widget_disabled_set(wd->btn, 0);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   evas_object_size_hint_min_get(wd->btn, &minw, &minh);
   evas_object_size_hint_max_get(wd->btn, &maxw, &maxh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_hover_clicked(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_hoversel_hover_end(data);
}

static void
_item_clicked(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Hoversel_Item *it = data;
   Evas_Object *obj2 = it->obj;

   elm_hoversel_hover_end(obj2);
   if (it->func) it->func(it->data, obj2, it);
   evas_object_smart_callback_call(obj2, "selected", it);
}

static void
_activate(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *bt, *bx, *ic;
   const Eina_List *l;
   const Elm_Hoversel_Item *it;
   char buf[4096];

   if (!wd) return;
   if (elm_widget_disabled_get(obj)) return;
   wd->hover = elm_hover_add(obj);
   if (wd->horizontal)
     snprintf(buf, sizeof(buf), "hoversel_horizontal/%s", elm_widget_style_get(obj));
   else
     snprintf(buf, sizeof(buf), "hoversel_vertical/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->hover, buf);
   evas_object_smart_callback_add(wd->hover, "clicked", _hover_clicked, obj);
   elm_hover_parent_set(wd->hover, wd->hover_parent);
   elm_hover_target_set(wd->hover, wd->btn);

   bx = elm_box_add(wd->hover);
   elm_box_homogenous_set(bx, 1);
   
   elm_box_horizontal_set(bx, wd->horizontal);
   
   if (wd->horizontal)
     snprintf(buf, sizeof(buf), "hoversel_horizontal_entry/%s",
              elm_widget_style_get(obj));
   else
     snprintf(buf, sizeof(buf), "hoversel_vertical_entry/%s",
              elm_widget_style_get(obj));
   EINA_LIST_FOREACH(wd->items, l, it)
     {
	bt = elm_button_add(wd->hover);
	elm_object_style_set(bt, buf);
	elm_button_label_set(bt, it->label);
	if (it->icon_file)
	  {
	     ic = elm_icon_add(obj);
	     elm_icon_scale_set(ic, 0, 1);
	     if (it->icon_type == ELM_ICON_FILE)
	       elm_icon_file_set(ic, it->icon_file, it->icon_group);
	     else if (it->icon_type == ELM_ICON_STANDARD)
	       elm_icon_standard_set(ic, it->icon_file);
	     elm_button_icon_set(bt, ic);
	     evas_object_show(ic);
	  }
	evas_object_size_hint_weight_set(bt, 1.0, 0.0);
	evas_object_size_hint_align_set(bt, -1.0, -1.0);
	elm_box_pack_end(bx, bt);
	evas_object_smart_callback_add(bt, "clicked", _item_clicked, it);
	evas_object_show(bt);
     }

   if (wd->horizontal)
     elm_hover_content_set(wd->hover,
                           elm_hover_best_content_location_get(wd->hover,
                                                               ELM_HOVER_AXIS_HORIZONTAL),
                           bx);
   else
     elm_hover_content_set(wd->hover,
                           elm_hover_best_content_location_get(wd->hover,
                                                               ELM_HOVER_AXIS_VERTICAL),
                           bx);
   evas_object_show(bx);

   evas_object_show(wd->hover);
   evas_object_smart_callback_call(obj, "clicked", NULL);

//   if (wd->horizontal) evas_object_hide(wd->btn);
}

static void
_button_clicked(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _activate(data);
}

static void
_parent_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->hover_parent = NULL;
}

/**
 * Add a new Hoversel object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Hoversel
 */
EAPI Evas_Object *
elm_hoversel_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   char buf[4096];

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "hoversel");
   elm_widget_type_set(obj, "hoversel");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);

   wd->btn = elm_button_add(parent);
   if (wd->horizontal)
     snprintf(buf, sizeof(buf), "hoversel_horizontal/%s", elm_widget_style_get(obj));
   else
     snprintf(buf, sizeof(buf), "hoversel_vertical/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->btn, buf);
   elm_widget_resize_object_set(obj, wd->btn);
   evas_object_event_callback_add(wd->btn, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				  _changed_size_hints, obj);
   evas_object_smart_callback_add(wd->btn, "clicked", _button_clicked, obj);
   _sizing_eval(obj);
   return obj;
}

/**
 * Set the Hover parent
 *
 * Sets the hover parent object. Should probably be the window that the hoversel
 * is in.  See Hover objects for more information.
 *
 * @param obj The hoversel object
 * @param parent The parent to use
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_hover_parent_set(Evas_Object *obj, Evas_Object *parent)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->hover_parent)
     evas_object_event_callback_del_full(wd->hover_parent, EVAS_CALLBACK_DEL,
                                    _parent_del, obj);
   wd->hover_parent = parent;
   if (wd->hover_parent)
     evas_object_event_callback_add(wd->hover_parent, EVAS_CALLBACK_DEL,
                                    _parent_del, obj);
}

/**
 * Set the hoversel button label
 *
 * This sets the label of the button that is always visible (before it is
 * clicked and expanded). Also see elm_button_label_set().
 *
 * @param obj The hoversel object
 * @param label The label text.
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_label_set(Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_button_label_set(wd->btn, label);
}

/**
 * Get the hoversel button label
 *
 * @param obj The hoversel object
 * @return The label text.
 *
 * @ingroup Hoversel
 */
EAPI const char *
elm_hoversel_label_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->btn)) return NULL;
   return elm_button_label_get(wd->btn);
}

/**
 * This sets the hoversel to expand horizontally.  The initial button
 * will display horizontally regardless of this setting.
 *
 * @param obj The hoversel object
 * @param horizontal If true, the hover will expand horizontally to the right.
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->horizontal = !!horizontal;
}


/**
 * This returns whether the hoversel is set to expand horizontally.
 *
 * @param obj The hoversel object
 * @return If true, the hover will expand horizontally to the right.
 *
 * @ingroup Hoversel
 */
EAPI Eina_Bool
elm_hoversel_horizontal_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->horizontal;
}

/**
 * Set the icon of the hoversel button
 *
 * Sets the icon of the button that is always visible (before it is clicked
 * and expanded). Also see elm_button_icon_set().
 *
 * @param obj The hoversel object
 * @param icon The icon object
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_button_icon_set(wd->btn, icon);
}

/**
 * Get the icon of the hoversel button
 *
 * Get the icon of the button that is always visible (before it is clicked
 * and expanded). Also see elm_button_icon_get().
 *
 * @param obj The hoversel object
 * @return The icon object
 *
 * @ingroup Hoversel
 */
EAPI Evas_Object *
elm_hoversel_icon_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->btn)) return NULL;
   return elm_button_icon_get(wd->btn);
}

/**
 * This triggers the hoversel popup from code, the same as though the
 * user clicked the button.
 *
 * @param obj The hoversel object
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_hover_begin(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->hover) return;
   _activate(obj);
}

/**
 * This ends the hoversel popup as though the user clicked outside the hover.
 *
 * @param obj The hoversel object
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_hover_end(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!wd->hover) return;
   evas_object_del(wd->hover);
   wd->hover = NULL;
   evas_object_smart_callback_call(obj, "dismissed", NULL);
}

/**
 * Returns whether the hoversel is expanded.
 *
 * @param obj The hoversel object
 * @return  This will return EINA_TRUE if the hoversel
 * is expanded or EINA_FALSE if it is not expanded.
 *
 * @ingroup Hoversel
 */
EAPI Eina_Bool
elm_hoversel_expanded_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return (wd->hover) ? EINA_TRUE : EINA_FALSE;
}
  
/**
 * This will remove all the children items from the hoversel. (should not be
 * called while the hoversel is active; use elm_hoversel_expanded_get()
 * to check first).
 *
 * @param obj The hoversel object
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_clear(Evas_Object *obj)
{
   Elm_Hoversel_Item *it;
   Eina_List *l, *ll;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   EINA_LIST_FOREACH_SAFE(wd->items, l, ll, it) elm_hoversel_item_del(it);
}

/**
 * Get the list of items within the given hoversel.
 *
 * @param obj The hoversel object
 * @return Returns a list of Elm_Hoversel_Item*
 *
 * @ingroup Hoversel
 */
EAPI const Eina_List *
elm_hoversel_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}

/**
 * Add an item to the hoversel button
 *
 * This adds an item to the hoversel to show when it is clicked. Note: if you
 * need to use an icon from an edje file then use elm_hoversel_item_icon_set()
 * right after the this function, and set icon_file to NULL here.
 *
 * @param obj The hoversel object
 * @param label The text label to use for the item (NULL if not desired)
 * @param icon_file An image file path on disk to use for the icon or standard
 * icon name (NULL if not desired)
 * @param icon_type The icon type if relevant
 * @param func Convenience function to call when this item is selected
 * @param data Data to pass to item-related functions
 * @return A handle to the item added.
 *
 * @ingroup Hoversel
 */
EAPI Elm_Hoversel_Item *
elm_hoversel_item_add(Evas_Object *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Elm_Hoversel_Item *it = calloc(1, sizeof(Elm_Hoversel_Item));
   if (!it) return NULL;
   wd->items = eina_list_append(wd->items, it);
   it->obj = obj;
   it->label = eina_stringshare_add(label);
   it->icon_file = eina_stringshare_add(icon_file);
   it->icon_type = icon_type;
   it->func = func;
   it->data = (void *)data;
   return it;
}

/**
 * Delete an item from the hoversel
 *
 * This deletes the item from the hoversel (should not be called while the
 * hoversel is active; use elm_hoversel_expanded_get()
 * to check first).
 *
 * @param it The item to delete
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_item_del(Elm_Hoversel_Item *it)
{
   if (!it) return;
   Widget_Data *wd = elm_widget_data_get(it->obj);
   if (it->del_cb) it->del_cb((void *)it->data, it->obj, it);
   if (!wd) return;
   elm_hoversel_hover_end(it->obj);
   wd->items = eina_list_remove(wd->items, it);
   eina_stringshare_del(it->label);
   eina_stringshare_del(it->icon_file);
   eina_stringshare_del(it->icon_group);
   free(it);
}

/**
 * Set the function called when an item within the hoversel
 * is freed. That function will receive these parameters:
 *
 * void *item_data
 * Evas_Object *the_item_object
 * Elm_Hoversel_Item *the_object_struct
 *
 * @param it The item to set the callback on
 * @param func The function called
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_item_del_cb_set(Elm_Hoversel_Item *it, Evas_Smart_Cb func)
{
   if (!it) return;
   it->del_cb = func;
}

/**
 * This returns the data pointer supplied with elm_hoversel_item_add() that
 * will be passed to associated function callbacks.
 *
 * @param it The item to get the data from
 * @return The data pointer set with elm_hoversel_item_add()
 *
 * @ingroup Hoversel
 */
EAPI void *
elm_hoversel_item_data_get(Elm_Hoversel_Item *it)
{
   if (!it) return NULL;
   return it->data;
}

/**
 * This returns the label text of the given hoversel item.
 *
 * @param it The item to get the label
 * @return The label text of the hoversel item
 *
 * @ingroup Hoversel
 */
EAPI const char *
elm_hoversel_item_label_get(Elm_Hoversel_Item *it)
{
   if (!it) return NULL;
   return it->label;
}

/**
 * This sets the icon for the given hoversel item. The icon can be loaded from
 * the standard set, from an image file, or from an edje file.
 *
 * @param it The item to set the icon
 * @param icon_file An image file path on disk to use for the icon or standard
 * icon name
 * @param icon_group The edje group to use if @p icon_file is an edje file. Set this
 * to NULL if the icon is not an edje file
 * @param icon_type The icon type
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_item_icon_set(Elm_Hoversel_Item *it, const char *icon_file, const char *icon_group, Elm_Icon_Type icon_type)
{
   if (!it) return;
   eina_stringshare_replace(&it->icon_file, icon_file);
   eina_stringshare_replace(&it->icon_group, icon_group);
   it->icon_type = icon_type;
}

/**
 * Get the icon object of the hoversel item
 *
 * @param it The item to get the icon from
 * @param icon_file The image file path on disk used for the icon or standard
 * icon name
 * @param icon_group The edje group used if @p icon_file is an edje file. NULL
 * if the icon is not an edje file
 * @param icon_type The icon type
 *
 * @ingroup Hoversel
 */
EAPI void
elm_hoversel_item_icon_get(Elm_Hoversel_Item *it, const char **icon_file, const char **icon_group, Elm_Icon_Type *icon_type)
{
   if (!it) return;
   if (icon_file) *icon_file = it->icon_file;
   if (icon_group) *icon_group = it->icon_group;
   if (icon_type) *icon_type = it->icon_type;
}

