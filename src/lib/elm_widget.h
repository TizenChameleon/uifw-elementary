#ifndef ELM_WIDGET_H
#define ELM_WIDGET_H

/* DO NOT USE THIS HEADER UNLESS YOU ARE PREPARED FOR BREAKING OF YOUR
 * CODE. THIS IS ELEMENTARY'S INTERNAL WIDGET API (for now) AND IS NOT
 * FINAL. CALL elm_widget_api_check(ELM_INTERNAL_API_VERSION) TO CHECK IT
 * AT RUNTIME
 *
 * How to make your own widget? like this:
 *
 * #include <Elementary.h>
 * #include "elm_priv.h"
 *
 * typedef struct _Widget_Data Widget_Data;
 *
 * struct _Widget_Data
 * {
 *   Evas_Object *sub;
 *   // add any other widget data here too
 * };
 *
 * static const char *widtype = NULL;
 * static void _del_hook(Evas_Object *obj);
 * static void _theme_hook(Evas_Object *obj);
 * static void _disable_hook(Evas_Object *obj);
 * static void _sizing_eval(Evas_Object *obj);
 * static void _on_focus_hook(void *data, Evas_Object *obj);
 *
 * static const char SIG_CLICKED[] = "clicked";
 * static const Evas_Smart_Cb_Description _signals[] = {
 *   {SIG_CLICKED, ""},
 *   {NULL, NULL}
 * };
 *
 * static void
 * _del_hook(Evas_Object *obj)
 * {
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    if (!wd) return;
 *    // delete hook - on delete of object delete object struct etc.
 *    free(wd);
 * }
 *
 * static void
 * _on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
 * {
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    if (!wd) return;
 *    // handle focus going in and out - optional, but if you want to, set
 *    // this hook and handle it (eg emit a signal to an edje obj)
 *    if (elm_widget_focus_get(obj))
 *      {
 *         edje_object_signal_emit(wd->sub, "elm,action,focus", "elm");
 *         evas_object_focus_set(wd->sub, EINA_TRUE);
 *      }
 *    else
 *      {
 *         edje_object_signal_emit(wd->sub, "elm,action,unfocus", "elm");
 *         evas_object_focus_set(wd->sub, EINA_FALSE);
 *      }
 * }
 *
 * static void
 * _theme_hook(Evas_Object *obj)
 * {
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    if (!wd) return;
 *    // handle change in theme/scale etc.
 *    elm_widget_theme_object_set(obj, wd->sub, "mywidget", "base",
 *                                elm_widget_style_get(obj));
 * }
 *
 * static void
 * _disable_hook(Evas_Object *obj)
 * {
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    if (!wd) return;
 *    // optional, but handle if the widget gets disabled or not
 *    if (elm_widget_disabled_get(obj))
 *      edje_object_signal_emit(wd->sub, "elm,state,disabled", "elm");
 *    else
 *      edje_object_signal_emit(wd->sub, "elm,state,enabled", "elm");
 * }
 *
 * static void
 * _sizing_eval(Evas_Object *obj)
 * {
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
 *    if (!wd) return;
 *    elm_coords_finger_size_adjust(1, &minw, 1, &minh);
 *    edje_object_size_min_restricted_calc(wd->sub, &minw, &minh, minw, minh);
 *    elm_coords_finger_size_adjust(1, &minw, 1, &minh);
 *    evas_object_size_hint_min_set(obj, minw, minh);
 *    evas_object_size_hint_max_set(obj, maxw, maxh);
 * }
 *
 * // actual api to create your widget. add more to manipulate it as needed
 * // mark your calls with EAPI to make them "external api" calls.
 * EAPI Evas_Object *
 * elm_mywidget_add(Evas_Object *parent)
 * {
 *    Evas_Object *obj;
 *    Evas *e;
 *    Widget_Data *wd;
 *
 *    // ALWAYS call this - this checks that your widget matches that of
 *    // elementary and that the api hasn't broken. if it has this returns
 *    // false and you need to handle this error gracefully
 *    if (!elm_widget_api_check(ELM_INTERNAL_API_VERSION)) return NULL;
 *
 *    // standard widget setup and allocate wd, create obj given parent etc.
 *    ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
 *
 *    // give it a type name and set up a mywidget type string if needed
 *    ELM_SET_WIDTYPE(widtype, "mywidget");
 *    elm_widget_type_set(obj, "mywidget");
 *    // tell the parent widget that we are a sub object
 *    elm_widget_sub_object_add(parent, obj);
 *    // setup hooks we need (some are optional)
 *    elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
 *    elm_widget_data_set(obj, wd);
 *    elm_widget_del_hook_set(obj, _del_hook);
 *    elm_widget_theme_hook_set(obj, _theme_hook);
 *    elm_widget_disable_hook_set(obj, _disable_hook);
 *    // this widget can focus (true means yes it can, false means it can't)
 *    elm_widget_can_focus_set(obj, EINA_TRUE);
 *
 *    // for this widget we will add 1 sub object that is an edje object
 *    wd->sub = edje_object_add(e);
 *    // set the theme. this follows a scheme for group name like this:
 *    //   "elm/WIDGETNAME/ELEMENT/STYLE"
 *    // so here it will be:
 *    //   "elm/mywidget/base/default"
 *    // changing style changes style name from default (all widgets start
 *    // with the default style) and element is for your widget internal
 *    // structure as you see fit
 *    elm_widget_theme_object_set(obj, wd->sub, "mywidget", "base", "default");
 *    // listen to a signal from the edje object to produce widget smart
 *    // callback (like click)
 *    edje_object_signal_callback_add(wd->sub, "elm,action,click", "",
 *                                    _signal_clicked, obj);
 *    // set this sub object as the "resize object". widgets get 1 resize
 *    // object that is resized along with the object wrapper.
 *    elm_widget_resize_object_set(obj, wd->sub);
 *
 *    // evaluate sizing of the widget (minimum size calc etc.). optional but
 *    // not a bad idea to do here. it will get queued for later anyway
 *    _sizing_eval(obj);
 *
 *    // register the smart callback descriptions so we can have some runtime
 *    // info as to what the smart callback strings mean
 *    evas_object_smart_callbacks_descriptions_set(obj, _signals);
 *    return obj;
 * }
 *
 * // example - do "whatever" to the widget (here just emit a signal)
 * EAPI void
 * elm_mywidget_whatever(Evas_Object *obj)
 * {
 *    // check if type is correct - check will return if it fails
 *    ELM_CHECK_WIDTYPE(obj, widtype);
 *    // get widget data - type is correct and sane by this point, so this
 *    // should never fail
 *    Widget_Data *wd = elm_widget_data_get(obj);
 *    // do whatever you like
 *    edje_object_signal_emit(wd->sub, "elm,state,action,whatever", "elm");
 * }
 *
 * // you can add more - you need to see elementary's code to know how to
 * // handle all cases. remember this api is not stable and may change. it's
 * // internal
 *
 */

#ifndef ELM_INTERNAL_API_ARGESFSDFEFC
# warning "You are using an internal elementary API. This API is not stable"
# warning "and is subject to change. You use this at your own risk."
# warning "Remember to call elm_widget_api_check(ELM_INTERNAL_API_VERSION);"
# warning "in your widgets before you call any other elm_widget calls to do"
# warning "a correct runtime version check. Also remember - you don't NEED"
# warning "to make an Elementary widget is almost ALL cases. You can easily"
# warning "make a smart object with Evas's API and do everything you need"
# warning "there. You only need a widget if you want to seamlessly be part"
# warning "of the focus tree and want to transparently become a container"
# warning "for any number of child Elementary widgets"
# error "ERROR. Compile aborted."
#endif
#define ELM_INTERNAL_API_VERSION 7000

typedef struct _Elm_Tooltip Elm_Tooltip;
typedef struct _Elm_Cursor Elm_Cursor;
typedef struct _Elm_Widget_Item Elm_Widget_Item; /**< base structure for all widget items that are not Elm_Widget themselves */

struct _Elm_Widget_Item
{
   /* ef1 ~~ efl, el3 ~~ elm */
#define ELM_WIDGET_ITEM_MAGIC 0xef1e1301
   EINA_MAGIC;

   Evas_Object   *widget; /**< the owner widget that owns this item */
   Evas_Object   *view; /**< the base view object */
   const void    *data; /**< item specific data */
   Evas_Smart_Cb  del_cb; /**< used to notify the item is being deleted */
   /* widget variations should have data from here and on */
   /* @todo: TODO check if this is enough for 1.0 release, maybe add padding! */
};

#define ELM_NEW(t) calloc(1, sizeof(t))

EAPI Eina_Bool        elm_widget_api_check(int ver);
EAPI Evas_Object     *elm_widget_add(Evas *evas);
EAPI void             elm_widget_del_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_del_pre_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_focus_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_activate_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_disable_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_theme_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_event_hook_set(Evas_Object *obj, Eina_Bool (*func) (Evas_Object *obj, Evas_Object *source, Evas_Callback_Type type, void *event_info));
EAPI void             elm_widget_changed_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void             elm_widget_signal_emit_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj, const char *emission, const char *source));
EAPI void             elm_widget_signal_callback_add_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data));
EAPI void             elm_widget_signal_callback_del_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data));
EAPI void             elm_widget_theme(Evas_Object *obj);
EAPI void             elm_widget_theme_specific(Evas_Object *obj, Elm_Theme *th, Eina_Bool force);
EAPI void             elm_widget_focus_next_hook_set(Evas_Object *obj, Eina_Bool (*func) (const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next));
EAPI void             elm_widget_on_focus_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data);
EAPI void             elm_widget_on_change_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data);
EAPI void             elm_widget_on_show_region_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data);
EAPI void             elm_widget_focus_region_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h));
EAPI void             elm_widget_on_focus_region_hook_set(Evas_Object *obj, void (*func) (const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h));
EAPI void             elm_widget_data_set(Evas_Object *obj, void *data);
EAPI void            *elm_widget_data_get(const Evas_Object *obj);
EAPI void             elm_widget_sub_object_add(Evas_Object *obj, Evas_Object *sobj);
EAPI void             elm_widget_sub_object_del(Evas_Object *obj, Evas_Object *sobj);
EAPI void             elm_widget_resize_object_set(Evas_Object *obj, Evas_Object *sobj);
EAPI void             elm_widget_hover_object_set(Evas_Object *obj, Evas_Object *sobj);
EAPI void             elm_widget_signal_emit(Evas_Object *obj, const char *emission, const char *source);
EAPI void             elm_widget_signal_callback_add(Evas_Object *obj, const char *emission, const char *source, void (*func) (void *data, Evas_Object *o, const char *emission, const char *source), void *data);
EAPI void            *elm_widget_signal_callback_del(Evas_Object *obj, const char *emission, const char *source, void (*func) (void *data, Evas_Object *o, const char *emission, const char *source));
EAPI void             elm_widget_can_focus_set(Evas_Object *obj, Eina_Bool can_focus);
EAPI Eina_Bool        elm_widget_can_focus_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_child_can_focus_get(const Evas_Object *obj);
EAPI void             elm_widget_highlight_ignore_set(Evas_Object *obj, Eina_Bool ignore);
EAPI Eina_Bool        elm_widget_highlight_ignore_get(const Evas_Object *obj);
EAPI void             elm_widget_highlight_in_theme_set(Evas_Object *obj, Eina_Bool highlight);
EAPI Eina_Bool        elm_widget_highlight_in_theme_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_focus_get(const Evas_Object *obj);
EAPI Evas_Object     *elm_widget_focused_object_get(const Evas_Object *obj);
EAPI Evas_Object     *elm_widget_top_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_is(const Evas_Object *obj);
EAPI Evas_Object     *elm_widget_parent_widget_get(const Evas_Object *obj);
EAPI void             elm_widget_event_callback_add(Evas_Object *obj, Elm_Event_Cb func, const void *data);
EAPI void            *elm_widget_event_callback_del(Evas_Object *obj, Elm_Event_Cb func, const void *data);
EAPI Eina_Bool        elm_widget_event_propagate(Evas_Object *obj, Evas_Callback_Type type, void *event_info, Evas_Event_Flags *event_flags);
EAPI void             elm_widget_focus_custom_chain_set(Evas_Object *obj, Eina_List *objs);
EAPI void             elm_widget_focus_custom_chain_unset(Evas_Object *obj);
EAPI const Eina_List *elm_widget_focus_custom_chain_get(const Evas_Object *obj);
EAPI void             elm_widget_focus_custom_chain_append(Evas_Object *obj, Evas_Object *child, Evas_Object *relative_child);
EAPI void             elm_widget_focus_custom_chain_prepend(Evas_Object *obj, Evas_Object *child, Evas_Object *relative_child);
EAPI void             elm_widget_focus_cycle(Evas_Object *obj, Elm_Focus_Direction dir);
EAPI void             elm_widget_focus_direction_go(Evas_Object *obj, int x, int y);
EAPI Eina_Bool        elm_widget_focus_next_get(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next);
EAPI Eina_Bool        elm_widget_focus_list_next_get(const Evas_Object *obj, const Eina_List *items, void *(*list_data_get) (const Eina_List *list), Elm_Focus_Direction dir, Evas_Object **next);
EAPI void             elm_widget_focus_set(Evas_Object *obj, int first);
EAPI void             elm_widget_focused_object_clear(Evas_Object *obj);
EAPI Evas_Object     *elm_widget_parent_get(const Evas_Object *obj);
EAPI void             elm_widget_focus_steal(Evas_Object *obj);
EAPI void             elm_widget_activate(Evas_Object *obj);
EAPI void             elm_widget_change(Evas_Object *obj);
EAPI void             elm_widget_disabled_set(Evas_Object *obj, int disabled);
EAPI int              elm_widget_disabled_get(const Evas_Object *obj);
EAPI void             elm_widget_show_region_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
EAPI void             elm_widget_show_region_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
EAPI void             elm_widget_focus_region_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
EAPI void             elm_widget_scroll_hold_push(Evas_Object *obj);
EAPI void             elm_widget_scroll_hold_pop(Evas_Object *obj);
EAPI int              elm_widget_scroll_hold_get(const Evas_Object *obj);
EAPI void             elm_widget_scroll_freeze_push(Evas_Object *obj);
EAPI void             elm_widget_scroll_freeze_pop(Evas_Object *obj);
EAPI int              elm_widget_scroll_freeze_get(const Evas_Object *obj);
EAPI void             elm_widget_scale_set(Evas_Object *obj, double scale);
EAPI double           elm_widget_scale_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_mirrored_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);
EAPI void             elm_widget_mirrored_set(Evas_Object *obj, Eina_Bool mirrored) EINA_ARG_NONNULL(1);
EAPI Eina_Bool        elm_widget_mirrored_automatic_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);
EAPI void             elm_widget_mirrored_automatic_set(Evas_Object *obj, Eina_Bool automatic) EINA_ARG_NONNULL(1);
EAPI void             elm_widget_theme_set(Evas_Object *obj, Elm_Theme *th);
EAPI Elm_Theme       *elm_widget_theme_get(const Evas_Object *obj);
EAPI void             elm_widget_style_set(Evas_Object *obj, const char *style);
EAPI const char      *elm_widget_style_get(const Evas_Object *obj);
EAPI void             elm_widget_type_set(Evas_Object *obj, const char *type);
EAPI const char      *elm_widget_type_get(const Evas_Object *obj);
EAPI void             elm_widget_tooltip_add(Evas_Object *obj, Elm_Tooltip *tt);
EAPI void             elm_widget_tooltip_del(Evas_Object *obj, Elm_Tooltip *tt);
EAPI void             elm_widget_cursor_add(Evas_Object *obj, Elm_Cursor *cur);
EAPI void             elm_widget_cursor_del(Evas_Object *obj, Elm_Cursor *cur);
EAPI void             elm_widget_drag_lock_x_set(Evas_Object *obj, Eina_Bool lock);
EAPI void             elm_widget_drag_lock_y_set(Evas_Object *obj, Eina_Bool lock);
EAPI Eina_Bool        elm_widget_drag_lock_x_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_drag_lock_y_get(const Evas_Object *obj);
EAPI int              elm_widget_drag_child_locked_x_get(const Evas_Object *obj);
EAPI int              elm_widget_drag_child_locked_y_get(const Evas_Object *obj);
EAPI Eina_Bool        elm_widget_theme_object_set(Evas_Object *obj, Evas_Object *edj, const char *wname, const char *welement, const char *wstyle);
EAPI void             elm_widget_type_register(const char **ptr);
EAPI Eina_Bool        elm_widget_type_check(const Evas_Object *obj, const char *type);
EAPI Eina_List       *elm_widget_stringlist_get(const char *str);
EAPI void             elm_widget_stringlist_free(Eina_List *list);
EAPI void             elm_widget_focus_hide_handle(Evas_Object *obj);
EAPI void             elm_widget_focus_mouse_down_handle(Evas_Object *obj);

EAPI Elm_Widget_Item *_elm_widget_item_new(Evas_Object *parent, size_t alloc_size);
EAPI void             _elm_widget_item_del(Elm_Widget_Item *item);
EAPI void             _elm_widget_item_pre_notify_del(Elm_Widget_Item *item);
EAPI void             _elm_widget_item_del_cb_set(Elm_Widget_Item *item, Evas_Smart_Cb del_cb);
EAPI void             _elm_widget_item_data_set(Elm_Widget_Item *item, const void *data);
EAPI void            *_elm_widget_item_data_get(const Elm_Widget_Item *item);
EAPI void             _elm_widget_item_tooltip_text_set(Elm_Widget_Item *item, const char *text);
EAPI void             _elm_widget_item_tooltip_content_cb_set(Elm_Widget_Item *item, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb);
EAPI void             _elm_widget_item_tooltip_unset(Elm_Widget_Item *item);
EAPI void             _elm_widget_item_tooltip_style_set(Elm_Widget_Item *item, const char *style);
EAPI const char      *_elm_widget_item_tooltip_style_get(const Elm_Widget_Item *item);
EAPI void             _elm_widget_item_cursor_set(Elm_Widget_Item *item, const char *cursor);
EAPI const char      *_elm_widget_item_cursor_get(const Elm_Widget_Item *item);
EAPI void             _elm_widget_item_cursor_unset(Elm_Widget_Item *item);
EAPI void             _elm_widget_item_cursor_style_set(Elm_Widget_Item *item, const char *style);
EAPI const char      *_elm_widget_item_cursor_style_get(const Elm_Widget_Item *item);
EAPI void             _elm_widget_item_cursor_engine_only_set(Elm_Widget_Item *item, Eina_Bool engine_only);
EAPI Eina_Bool        _elm_widget_item_cursor_engine_only_get(const Elm_Widget_Item *item);

/* debug function. don't use it unless you are tracking parenting issues */
EAPI void             elm_widget_tree_dump(const Evas_Object *top);
EAPI void             elm_widget_tree_dot_dump(const Evas_Object *top, FILE *output);

/**
 * Convenience macro to create new widget item, doing casts for you.
 * @see _elm_widget_item_new()
 * @param parent a valid elm_widget variant.
 * @param type the C type that extends Elm_Widget_Item
 */
#define elm_widget_item_new(parent, type) \
  (type *)_elm_widget_item_new((parent), sizeof(type))
/**
 * Convenience macro to delete widget item, doing casts for you.
 * @see _elm_widget_item_del()
 * @param item a valid item.
 */
#define elm_widget_item_del(item) \
  _elm_widget_item_del((Elm_Widget_Item *)item)
/**
 * Convenience macro to notify deletion of widget item, doing casts for you.
 * @see _elm_widget_item_pre_notify_del()
 */
#define elm_widget_item_pre_notify_del(item) \
  _elm_widget_item_pre_notify_del((Elm_Widget_Item *)item)
/**
 * Convenience macro to set deletion callback of widget item, doing casts for you.
 * @see _elm_widget_item_del_cb_set()
 */
#define elm_widget_item_del_cb_set(item, del_cb) \
  _elm_widget_item_del_cb_set((Elm_Widget_Item *)item, del_cb)

/**
 * Set item's data
 * @see _elm_widget_item_data_set()
 */
#define elm_widget_item_data_set(item, data) \
  _elm_widget_item_data_set((Elm_Widget_Item *)item, data)
/**
 * Get item's data
 * @see _elm_widget_item_data_get()
 */
#define elm_widget_item_data_get(item) \
  _elm_widget_item_data_get((const Elm_Widget_Item *)item)

/**
 * Convenience function to set widget item tooltip as a text string.
 * @see _elm_widget_item_tooltip_text_set()
 */
#define elm_widget_item_tooltip_text_set(item, text) \
  _elm_widget_item_tooltip_text_set((Elm_Widget_Item *)item, text)
/**
 * Convenience function to set widget item tooltip.
 * @see _elm_widget_item_tooltip_content_cb_set()
 */
#define elm_widget_item_tooltip_content_cb_set(item, func, data, del_cb) \
  _elm_widget_item_tooltip_content_cb_set((Elm_Widget_Item *)item, \
                                          func, data, del_cb)
/**
 * Convenience function to unset widget item tooltip.
 * @see _elm_widget_item_tooltip_unset()
 */
#define elm_widget_item_tooltip_unset(item) \
  _elm_widget_item_tooltip_unset((Elm_Widget_Item *)item)
/**
 * Convenience function to change item's tooltip style.
 * @see _elm_widget_item_tooltip_style_set()
 */
#define elm_widget_item_tooltip_style_set(item, style) \
  _elm_widget_item_tooltip_style_set((Elm_Widget_Item *)item, style)
/**
 * Convenience function to query item's tooltip style.
 * @see _elm_widget_item_tooltip_style_get()
 */
#define elm_widget_item_tooltip_style_get(item) \
  _elm_widget_item_tooltip_style_get((const Elm_Widget_Item *)item)
/**
 * Convenience function to set widget item cursor.
 * @see _elm_widget_item_cursor_set()
 */
#define elm_widget_item_cursor_set(item, cursor) \
  _elm_widget_item_cursor_set((Elm_Widget_Item *)item, cursor)
/**
 * Convenience function to get widget item cursor.
 * @see _elm_widget_item_cursor_get()
 */
#define elm_widget_item_cursor_get(item) \
  _elm_widget_item_cursor_get((const Elm_Widget_Item *)item)
/**
 * Convenience function to unset widget item cursor.
 * @see _elm_widget_item_cursor_unset()
 */
#define elm_widget_item_cursor_unset(item) \
  _elm_widget_item_cursor_unset((Elm_Widget_Item *)item)
/**
 * Convenience function to change item's cursor style.
 * @see _elm_widget_item_cursor_style_set()
 */
#define elm_widget_item_cursor_style_set(item, style) \
  _elm_widget_item_cursor_style_set((Elm_Widget_Item *)item, style)
/**
 * Convenience function to query item's cursor style.
 * @see _elm_widget_item_cursor_style_get()
 */
#define elm_widget_item_cursor_style_get(item) \
  _elm_widget_item_cursor_style_get((const Elm_Widget_Item *)item)
/**
 * Convenience function to change item's cursor engine_only.
 * @see _elm_widget_item_cursor_engine_only_set()
 */
#define elm_widget_item_cursor_engine_only_set(item, engine_only) \
  _elm_widget_item_cursor_engine_only_set((Elm_Widget_Item *)item, engine_only)
/**
 * Convenience function to query item's cursor engine_only.
 * @see _elm_widget_item_cursor_engine_only_get()
 */
#define elm_widget_item_cursor_engine_only_get(item) \
  _elm_widget_item_cursor_engine_only_get((const Elm_Widget_Item *)item)

/**
 * Cast and ensure the given pointer is an Elm_Widget_Item or return NULL.
 */
#define ELM_WIDGET_ITEM(item) \
   (((item) && (EINA_MAGIC_CHECK(item, ELM_WIDGET_ITEM_MAGIC))) ? \
       ((Elm_Widget_Item *)(item)) : NULL)

#define ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, ...) \
   do { \
      if (!item) { \
         CRITICAL("Elm_Widget_Item " # item " is NULL!"); \
         return __VA_ARGS__; \
      } \
      if (!EINA_MAGIC_CHECK(item, ELM_WIDGET_ITEM_MAGIC)) { \
         EINA_MAGIC_FAIL(item, ELM_WIDGET_ITEM_MAGIC); \
         return __VA_ARGS__; \
      } \
   } while (0)

#define ELM_WIDGET_ITEM_CHECK_OR_GOTO(item, label) \
   do { \
      if (!item) { \
         CRITICAL("Elm_Widget_Item " # item " is NULL!"); \
         goto label; \
      } \
      if (!EINA_MAGIC_CHECK(item, ELM_WIDGET_ITEM_MAGIC)) { \
         EINA_MAGIC_FAIL(item, ELM_WIDGET_ITEM_MAGIC); \
         goto label; \
      } \
   } while (0)

#define ELM_SET_WIDTYPE(widtype, type) \
   do { \
      if (!widtype) { \
         widtype = eina_stringshare_add(type); \
         elm_widget_type_register(&widtype); \
      } \
   } while (0)

#define ELM_CHECK_WIDTYPE(obj, widtype) \
   if (!elm_widget_type_check((obj), (widtype))) return

#define ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_RETURN(it, ...)                \
   ELM_WIDGET_ITEM_CHECK_OR_RETURN((Elm_Widget_Item *)it, __VA_ARGS__); \
   ELM_CHECK_WIDTYPE(it->base.widget, widtype) __VA_ARGS__;

#define ELM_WIDGET_ITEM_WIDTYPE_CHECK_OR_GOTO(it, label)                \
   ELM_WIDGET_ITEM_CHECK_OR_GOTO((Elm_Widget_Item *)it, label);         \
   if (!elm_widget_type_check((it->base.widget), (widtype))) goto label;

#define ELM_WIDGET_STANDARD_SETUP(wdat, wdtype, par, evas, ob, ret) \
   do { \
      EINA_SAFETY_ON_NULL_RETURN_VAL((par), (ret)); \
      evas = evas_object_evas_get(par); if (!(evas)) return (ret); \
      wdat = ELM_NEW(wdtype); if (!(wdat)) return (ret); \
      ob = elm_widget_add(evas); if (!(ob)) { free(wdat); return (ret); } \
   } while (0)

/**
 * The drag and drop API.
 * Currently experimental, and will change when it does dynamic type
 * addition RSN.
 *
 * Here so applications can start to use it, if they ask elm nicely.
 *
 * And yes, elm_widget, should probably be elm_experimental...
 * Complaints about this code should go to /dev/null, or failing that nash.
 */
typedef struct _Elm_Selection_Data Elm_Selection_Data;

typedef Eina_Bool (*Elm_Drop_Cb) (void *d, Evas_Object *o, Elm_Selection_Data *data);

typedef enum _Elm_Sel_Type
{
   ELM_SEL_PRIMARY,
   ELM_SEL_SECONDARY,
   ELM_SEL_CLIPBOARD,
   ELM_SEL_XDND,

   ELM_SEL_MAX,
} Elm_Sel_Type;

typedef enum _Elm_Sel_Format
{
   /** Plain unformated text: Used for things that don't want rich markup */
   ELM_SEL_FORMAT_TEXT   = 0x01,
   /** Edje textblock markup, including inline images */
   ELM_SEL_FORMAT_MARKUP = 0x02,
   /** Images */
   ELM_SEL_FORMAT_IMAGE	 = 0x04,
   /** Vcards */
   ELM_SEL_FORMAT_VCARD =  0x08,
   /** Raw HTMLish things for widgets that want that stuff (hello webkit!) */
   ELM_SEL_FORMAT_HTML = 0x10,
} Elm_Sel_Format;

struct _Elm_Selection_Data
{
   int                   x, y;
   Elm_Sel_Format        format;
   void                 *data;
   int                   len;
};

Eina_Bool            elm_selection_set(Elm_Sel_Type selection, Evas_Object *widget, Elm_Sel_Format format, const char *buf);
Eina_Bool            elm_selection_clear(Elm_Sel_Type selection, Evas_Object *widget);
Eina_Bool            elm_selection_get(Elm_Sel_Type selection, Elm_Sel_Format format, Evas_Object *widget, Elm_Drop_Cb datacb, void *udata);
Eina_Bool            elm_drop_target_add(Evas_Object *widget, Elm_Sel_Type, Elm_Drop_Cb, void *);
Eina_Bool            elm_drop_target_del(Evas_Object *widget);
Eina_Bool            elm_drag_start(Evas_Object *, Elm_Sel_Format, const char *, void (*)(void *,Evas_Object*),void*);

#endif
