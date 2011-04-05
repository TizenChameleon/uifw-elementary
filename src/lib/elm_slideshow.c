#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Slideshow Slideshow
 *
 * This object display a list of object (generally a list of images) and some actions like
 * next/previous are used to navigate. The animations are defined in the theme,
 * consequently new animations can be added without having to update the
 * applications.
 *
 * The slideshow use 2 callbacks to create and delete the objects displayed. When an item
 * is displayed the function itc->func.get() is called. This function should create the object,
 * for example the object can be an evas_object_image or a photocam. When a object is no more
 * displayed the function itc->func.del() is called, the user can delete the dana associated to the item.
 *
 * Signals that you can add callbacks for are:
 *
 * "changed" - when the slideshow switch to another item
 */

typedef struct _Widget_Data Widget_Data;

struct _Elm_Slideshow_Item
{
   Elm_Widget_Item base;

   Eina_List *l, *l_built;

   const Elm_Slideshow_Item_Class *itc;
};

struct _Widget_Data
{
   Evas_Object *slideshow;

   // list of Elm_Slideshow_Item*
   Eina_List *items;
   Eina_List *items_built;

   Elm_Slideshow_Item *current;
   Elm_Slideshow_Item *previous;

   Eina_List *transitions;
   const char *transition;

   Ecore_Timer *timer;
   double timeout;
   Eina_Bool loop:1;

   struct {
        const char *current;
        Eina_List *list; //list of const char *
   } layout;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static Eina_Bool _timer_cb(void *data);
static void _on_focus_hook(void *data, Evas_Object *obj);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")))
     {
        elm_slideshow_previous(obj);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")))
     {
        elm_slideshow_next(obj);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   if ((!strcmp(ev->keyname, "Return")) ||
       (!strcmp(ev->keyname, "KP_Enter")) ||
       (!strcmp(ev->keyname, "space")))
     {
        if (wd->timeout)
          {
             if (wd->timer)
               {
                  ecore_timer_del(wd->timer);
                  wd->timer = NULL;
               }
             else
               elm_slideshow_timeout_set(obj, wd->timeout);
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static void
_del_hook(Evas_Object *obj)
{
   const char *layout;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_slideshow_clear(obj);
   elm_widget_stringlist_free(wd->transitions);
   if (wd->timer) ecore_timer_del(wd->timer);
   EINA_LIST_FREE(wd->layout.list, layout)
      eina_stringshare_del(layout);
   free(wd);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->slideshow, "elm,action,focus", "elm");
        evas_object_focus_set(wd->slideshow, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(wd->slideshow, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->slideshow, EINA_FALSE);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_mirrored_set(wd->slideshow, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _elm_theme_object_set(obj, wd->slideshow, "slideshow", "base", elm_widget_style_get(obj));
   edje_object_scale_set(wd->slideshow, elm_widget_scale_get(obj) *
                         _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   if (!wd) return;
   edje_object_size_min_calc(wd->slideshow, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, minw, minh);
}


static Elm_Slideshow_Item* _item_prev_get(Elm_Slideshow_Item* item)
{
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   Elm_Slideshow_Item* prev = eina_list_data_get(eina_list_prev(item->l));
   if((!prev) && (wd->loop))
     prev = eina_list_data_get(eina_list_last(item->l));
   return prev;
}

static Elm_Slideshow_Item* _item_next_get(Elm_Slideshow_Item* item)
{
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   Elm_Slideshow_Item* next = eina_list_data_get(eina_list_next(item->l));
   if((!next) && (wd->loop))
     next = eina_list_data_get(wd->items);
   return next;
}


static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ;
}

static void
_item_realize(Elm_Slideshow_Item *item)
{
   Elm_Slideshow_Item *_item;
   Evas_Object *obj = item->base.widget;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if ((!item->base.view) && (item->itc->func.get))
     {
        item->base.view = item->itc->func.get((void*)item->base.data, obj);
        evas_object_smart_member_add(item->base.view, obj);
        item->l_built = eina_list_append(NULL, item);
        wd->items_built = eina_list_merge(wd->items_built, item->l_built);
        evas_object_hide(item->base.view);
     }
   else if (item->l_built)
     wd->items_built = eina_list_demote_list(wd->items_built, item->l_built);

   //pre-create previous and next item
   _item = _item_next_get(item);
   if ((_item) && (!_item->base.view) && (_item->itc->func.get))
     {
        _item->base.view = _item->itc->func.get((void*)_item->base.data, obj);
        evas_object_smart_member_add(_item->base.view, obj);
        _item->l_built = eina_list_append(NULL, _item);
        wd->items_built = eina_list_merge(wd->items_built, _item->l_built);
        evas_object_hide(_item->base.view);
     }
   else if ((_item) && (_item->l_built))
     wd->items_built = eina_list_demote_list(wd->items_built, _item->l_built);

   _item = _item_prev_get(item);
   if ((_item) && (!_item->base.view) && (_item->itc->func.get))
     {
        _item->base.view = _item->itc->func.get((void*)_item->base.data, obj);
        evas_object_smart_member_add(_item->base.view, obj);
        _item->l_built = eina_list_append(NULL, _item);
        wd->items_built = eina_list_merge(wd->items_built, _item->l_built);
        evas_object_hide(_item->base.view);
     }
   else if ((_item) && (_item->l_built))
     wd->items_built = eina_list_demote_list(wd->items_built, _item->l_built);

   //delete unused items
   while (eina_list_count(wd->items_built) > 3)
     {
        _item = eina_list_data_get(wd->items_built);
        wd->items_built = eina_list_remove_list(wd->items_built, wd->items_built);
        if(item->itc->func.del)
          item->itc->func.del((void*)item->base.data, _item->base.view);
        evas_object_del(_item->base.view);
        _item->base.view = NULL;
     }
}

static void
_end(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Slideshow_Item *item;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   item = wd->previous;
   if(item)
     {
        edje_object_part_unswallow(NULL, item->base.view);
        evas_object_hide(item->base.view);
        wd->previous = NULL;
     }


   item = wd->current;
   if ((!item) || (!item->base.view)) return;

   _item_realize(item);
   edje_object_part_unswallow(NULL, item->base.view);
   evas_object_show(item->base.view);

   edje_object_signal_emit(wd->slideshow, "anim,end", "slideshow");
   edje_object_part_swallow(wd->slideshow, "elm.swallow.1", item->base.view);
}


static Eina_Bool
_timer_cb(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->timer = NULL;
   elm_slideshow_next(obj);
   return ECORE_CALLBACK_CANCEL;
}



/**
 * Add a new slideshow to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Slideshow
 */
EAPI Evas_Object *
elm_slideshow_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "slideshow");
   elm_widget_type_set(obj, "slideshow");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_event_hook_set(obj, _event_hook);

   wd->current = NULL;
   wd->previous = NULL;

   wd->slideshow = edje_object_add(e);
   _elm_theme_object_set(obj, wd->slideshow, "slideshow", "base", "default");
   evas_object_smart_member_add(wd->slideshow, obj);
   elm_widget_resize_object_set(obj, wd->slideshow);
   evas_object_show(wd->slideshow);

   wd->transitions = elm_widget_stringlist_get(edje_object_data_get(wd->slideshow, "transitions"));
   if (eina_list_count(wd->transitions) > 0)
     wd->transition = eina_stringshare_add(eina_list_data_get(wd->transitions));

   wd->layout.list = elm_widget_stringlist_get(edje_object_data_get(wd->slideshow, "layouts"));
   if (eina_list_count(wd->layout.list) > 0)
     wd->layout.current = eina_list_data_get(wd->layout.list);

   edje_object_signal_callback_add(wd->slideshow, "end", "slideshow", _end, obj);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);
   return obj;
}

/**
 * Add a object in the list. The object can be a evas object image or a elm photo for example.
 *
 * @param obj The slideshow object
 * @aram itc Callbacks used to create the object and delete the data associated when the item is deleted.
 * @param data Data used by the user to identified the item
 * @return Returns The slideshow item
 *
 * @ingroup Slideshow
 */
EAPI Elm_Slideshow_Item*
elm_slideshow_item_add(Evas_Object *obj, const Elm_Slideshow_Item_Class *itc, const void *data)
{
   Elm_Slideshow_Item *item;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   item = elm_widget_item_new(obj, Elm_Slideshow_Item);
   item->base.data = data;
   item->itc = itc;
   item->l = eina_list_append(item->l, item);

   wd->items = eina_list_merge(wd->items, item->l);

   if (!wd->current) elm_slideshow_show(item);

   return item;
}

/**
 * Go to the item
 *
 * @param obj The slideshow object
 * @param item The item
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_show(Elm_Slideshow_Item *item)
{
   char buf[1024];
   Elm_Slideshow_Item *next = NULL;
   Widget_Data *wd;
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   wd = elm_widget_data_get(item->base.widget);
   if (!wd)
     return;
   if (item == wd->current)
     return;

   next = item;
   _end(item->base.widget, item->base.widget, NULL, NULL);

   if (wd->timer) ecore_timer_del(wd->timer);
   if (wd->timeout > 0.0)
     wd->timer = ecore_timer_add(wd->timeout, _timer_cb, item->base.widget);
   _item_realize(next);
   edje_object_part_swallow(wd->slideshow, "elm.swallow.2", next->base.view);
   evas_object_show(next->base.view);
   snprintf(buf, sizeof(buf), "%s,next", wd->transition);
   edje_object_signal_emit(wd->slideshow, buf, "slideshow");
   wd->previous = wd->current;
   wd->current = next;
   evas_object_smart_callback_call(item->base.widget, "changed", wd->current);
}

/**
 * Go to the next item
 *
 * @param obj The slideshow object
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_next(Evas_Object *obj)
{
   char buf[1024];
   Elm_Slideshow_Item *next = NULL;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (wd->current)
     next = _item_next_get(wd->current);

   if ((!next) || (next == wd->current)) return;

   _end(obj, obj, NULL, NULL);

   if (wd->timer) ecore_timer_del(wd->timer);
   if (wd->timeout > 0.0)
     wd->timer = ecore_timer_add(wd->timeout, _timer_cb, obj);

   _item_realize(next);

   edje_object_part_swallow(wd->slideshow, "elm.swallow.2", next->base.view);
   evas_object_show(next->base.view);

   snprintf(buf, sizeof(buf), "%s,next", wd->transition);
   edje_object_signal_emit(wd->slideshow, buf, "slideshow");

   wd->previous = wd->current;
   wd->current = next;
   evas_object_smart_callback_call(obj, "changed", wd->current);
}

/**
 * Go to the previous item
 *
 * @param obj The slideshow object
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_previous(Evas_Object *obj)
{
   char buf[1024];
   Elm_Slideshow_Item *prev = NULL;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (wd->current)
     prev = _item_prev_get(wd->current);

   if ((!prev) ||  (prev == wd->current)) return;

   _end(obj, obj, NULL, NULL);

   if (wd->timer) ecore_timer_del(wd->timer);
   if (wd->timeout > 0.0)
     wd->timer = ecore_timer_add(wd->timeout, _timer_cb, obj);

   _item_realize(prev);

   edje_object_part_swallow(wd->slideshow, "elm.swallow.2", prev->base.view);
   evas_object_show(prev->base.view);

   snprintf(buf, 1024, "%s,previous", wd->transition);
   edje_object_signal_emit(wd->slideshow, buf, "slideshow");

   wd->previous = wd->current;
   wd->current = prev;
   evas_object_smart_callback_call(obj, "changed", wd->current);
}

/**
 * Returns the list of transitions available.
 *
 * @param obj The slideshow object
 * @return Returns the list of transitions (list of const char*)
 *
 * @ingroup Slideshow
 */
EAPI const Eina_List *
elm_slideshow_transitions_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->transitions;
}

/**
 * Returns the list of layouts available.
 *
 * @param obj The slideshow object
 * @return Returns the list of layout (list of const char*)
 *
 * @ingroup Slideshow
 */
EAPI const Eina_List *
elm_slideshow_layouts_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->layout.list;
}

/**
 * Set the transition to use
 *
 * @param obj The slideshow object
 * @param transition the new transition
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_transition_set(Evas_Object *obj, const char *transition)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->transition, transition);
}

/**
 * Returns the transition to use
 *
 * @param obj The slideshow object
 * @return the transition set
 *
 * @ingroup Slideshow
 */
EAPI const char *
elm_slideshow_transition_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->transition;
}

/**
 * The slideshow can go to the next item automatically after a few seconds.
 * This method set the timeout to use. A timeout <=0 disable the timer.
 *
 * @param obj The slideshow object
 * @param timeout The new timeout
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_timeout_set(Evas_Object *obj, double timeout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->timeout = timeout;
   if (wd->timer) ecore_timer_del(wd->timer);
   wd->timer = NULL;
   if (timeout > 0.0)
     wd->timer = ecore_timer_add(timeout, _timer_cb, obj);
}

/**
 * Returns the timeout value
 *
 * @param obj The slideshow object
 * @return Returns the timeout
 *
 * @ingroup Slideshow
 */
EAPI double
elm_slideshow_timeout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1.0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1.0;
   return wd->timeout;
}

/**
 * Set if the first item should follow the last and vice versa
 *
 * @param obj The slideshow object
 * @param loop if EINA_TRUE, the first item will follow the last and vice versa
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_loop_set(Evas_Object *obj, Eina_Bool loop)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->loop = loop;
}

/**
 * Returns the current layout name
 *
 * @param obj The slideshow object
 * @returns Returns the layout name
 *
 * @ingroup Slideshow
 */
EAPI const char *
elm_slideshow_layout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->layout.current;
}

/**
 * Set the layout
 *
 * @param obj The slideshow object
 * @param layout the new layout
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_layout_set(Evas_Object *obj, const char *layout)
{
   char buf[PATH_MAX];
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->layout.current = layout;
   snprintf(buf, sizeof(buf), "layout,%s", layout);
   edje_object_signal_emit(wd->slideshow, buf, "slideshow");
}

/**
 * Return if the first item should follow the last and vice versa
 *
 * @param obj The slideshow object
 * @returns Returns the loop flag
 *
 * @ingroup Slideshow
 */
EAPI Eina_Bool
elm_slideshow_loop_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->loop;
}

/**
 * Delete all the items
 *
 * @param obj The slideshow object
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_clear(Evas_Object *obj)
{
   Elm_Slideshow_Item *item;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->previous = NULL;
   wd->current = NULL;
   EINA_LIST_FREE(wd->items_built, item)
     {
        if (item->itc->func.del)
          item->itc->func.del((void*)item->base.data, item->base.view);
        evas_object_del(item->base.view);
        item->base.view = NULL;
     }

   EINA_LIST_FREE(wd->items, item)
     {
        elm_widget_item_del(item);
     }
}


/**
 * Delete the item
 *
 * @param item The slideshow item
 *
 * @ingroup Slideshow
 */
EAPI void
elm_slideshow_item_del(Elm_Slideshow_Item *item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item);
   Widget_Data *wd = elm_widget_data_get(item->base.widget);
   if (!wd) return;
   if (wd->previous == item) wd->previous = NULL;
   if (wd->current == item)
     {
        Eina_List *l = eina_list_data_find_list(wd->items, item);
        Eina_List *l2 = eina_list_next(l);
        wd->current = NULL;
        if (!l2)
          l2 = eina_list_nth_list(wd->items, eina_list_count(wd->items) - 1);
        if (l2)
          elm_slideshow_show(eina_list_data_get(l2));
     }

   wd->items = eina_list_remove_list(wd->items, item->l);
   wd->items_built = eina_list_remove_list(wd->items_built, item->l_built);

   if ((item->base.view) && (item->itc->func.del))
     item->itc->func.del((void*)item->base.data, item->base.view);
   if (item->base.view)
     evas_object_del(item->base.view);
   free(item);
}

/**
 * Returns the list of items
 * @param obj The slideshow object
 * @return Returns the list of items (list of Elm_Slideshow_Item).
 *
 * @ingroup Slideshow
 */
EAPI const Eina_List *
elm_slideshow_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}


/**
 * Returns the current item displayed
 *
 * @param obj The slideshow object
 * @return Returns the current item displayed
 *
 * @ingroup Slideshow
 */
EAPI Elm_Slideshow_Item *
elm_slideshow_item_current_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->current;
}

/**
 * Returns the evas object associated to an item
 *
 * @param item The slideshow item
 * @return Returns the evas object associated to this item
 *
 * @ingroup Slideshow
 */
EAPI Evas_Object *
elm_slideshow_item_object_get(const Elm_Slideshow_Item * item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return item->base.view;
}

/**
 * Returns the data associated to an item
 *
 * @param item The slideshow item
 * @return Returns the data associated to this item
 *
 * @ingroup Slideshow
 */
EAPI void *
elm_slideshow_item_data_get(const Elm_Slideshow_Item * item)
{
   ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(item, NULL);
   return elm_widget_item_data_get(item);
}
