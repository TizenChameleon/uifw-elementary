#include <Elementary.h>
#include "elm_priv.h"
#include "els_scroller.h"

/**
 * @defgroup Genlist Genlist
 * @ingroup Elementary
 *
 * The aim was to have  more expansive list that the simple list in
 * Elementary that could have more flexible items and allow many more entries
 * while still being fast and low on memory usage. At the same time it was
 * also made to be able to do tree structures. But the price to pay is more
 * complexity when it comes to usage. If all you want is a simple list with
 * icons and a single label, use the normal List object.
 *
 * Signals that you can add callbacks for are:
 *
 * clicked - This is called when a user has double-clicked an item. The
 * event_info parameter is the genlist item that was double-clicked.
 *
 * selected - This is called when a user has made an item selected. The
 * event_info parameter is the genlist item that was selected.
 *
 * unselected - This is called when a user has made an item unselected. The
 * event_info parameter is the genlist item that was unselected.
 *
 * expanded -  This is called when elm_genlist_item_expanded_set() is called
 * and the item is now meant to be expanded. The event_info parameter is the
 * genlist item that was indicated to expand. It is the job of this callback
 * to then fill in the child items.
 *
 * contracted - This is called when elm_genlist_item_expanded_set() is called
 * and the item is now meant to be contracted. The event_info parameter is
 * the genlist item that was indicated to contract. It is the job of this
 * callback to then delete the child items
 *
 * expand,request - This is called when a user has indicated they want to
 * expand a tree branch item. The callback should decide if the item can
 * expand (has any children) and then call elm_genlist_item_expanded_set()
 * appropriately to set the state. The event_info parameter is the genlist
 * item that was indicated to expand.
 *
 * contract,request - This is called when a user has indicated they want to
 * contract a tree branch item. The callback should decide if the item can
 * contract (has any children) and then call elm_genlist_item_expanded_set()
 * appropriately to set the state. The event_info parameter is the genlist
 * item that was indicated to contract.
 *
 * realized - This is called when the item in the list is created as a real
 * evas object. event_info parameter is the genlist item that was created.
 * The object may be deleted at any time, so it is up to the caller to
 * not use the object pointer from elm_genlist_item_object_get() in a way
 * where it may point to freed objects.
 *
 * drag,start,up - This is called when the item in the list has been dragged
 * (not scrolled) up.
 *
 * drag,start,down - This is called when the item in the list has been dragged
 * (not scrolled) down.
 *
 * drag,start,left - This is called when the item in the list has been dragged
 * (not scrolled) left.
 *
 * drag,start,right - This is called when the item in the list has been dragged
 * (not scrolled) right.
 *
 * drag,stop - This is called when the item in the list has stopped being
 * dragged.
 *
 * drag - This is called when the item in the list is being dragged.
 *
 * longpressed - This is called when the item is pressed for a certain amount
 * of time. By default it's 1 second.
 *
 * Genlist has a fairly large API, mostly because it's relatively complex,
 * trying to be both expansive, powerful and efficient. First we will begin
 * an overview o the theory behind genlist.
 *
 * Evas tracks every object you create. Every time it processes an event
 * (mouse move, down, up etc.) it needs to walk through objects and find out
 * what event that affects. Even worse every time it renders display updates,
 * in order to just calculate what to re-draw, it needs to walk through many
 * many many objects. Thus, the more objects you keep active, the more
 * overhead Evas has in just doing its work. It is advisable to keep your
 * active objects to the minimum working set you need. Also remember that
 * object creation and deletion carries an overhead, so there is a
 * middle-ground, which is not easily determined. But don't keep massive lists
 * of objects you can't see or use. Genlist does this with list objects. It
 * creates and destroys them dynamically as you scroll around. It groups them
 * into blocks so it can determine the visibility etc. of a whole block at
 * once as opposed to having to walk the whole list. This 2-level list allows
 * for very large numbers of items to be in the list (tests have used up to
 * 2,000,000 items). Also genlist employs a queue for adding items. As items
 * may be different sizes, every item added needs to be calculated as to its
 * size and thus this presents a lot of overhead on populating the list, this
 * genlist employs a queue. Any item added is queued and spooled off over
 * time, actually appearing some time later, so if your list has many members
 * you may find it takes a while for them to all appear, with your process
 * consuming a lot of CPU while it is busy spooling.
 *
 * Genlist also implements a tree structure, but it does so with callbacks to
 * the application, with the application filling in tree structures when
 * requested (allowing for efficient building of a very deep tree that could
 * even be used for file-management). See the above smart signal callbacks for
 * details.
 *
 * An item in the genlist world can have 0 or more text labels (they can be
 * regular text or textblock ??that's up to the style to determine), 0 or
 * more icons (which are simply objects swallowed into the genlist item) and
 * 0 or more boolean states that can be used for check, radio or other
 * indicators by the edje theme style. An item may be one of several styles
 * (Elementary provides 2 by default - ?œdefault??and ?œdouble_label?? but this
 * can be extended by system or application custom themes/overlays/extensions).
 *
 * In order to implement the ability to add and delete items on the fly,
 * Genlist implements a class/callback system where the application provides
 * a structure with information about that type of item (genlist may contain
 * multiple different items with different classes, states and styles).
 * Genlist will call the functions in this struct (methods) when an item is
 * ?œrealized??(that is created dynamically while scrolling). All objects will
 * simply be deleted  when no longer needed with evas_object_del(). The
 * Elm_Genlist_Item_Class structure contains the following members:
 *
 * item_style - This is a constant string and simply defines the name of the
 * item style. It must be specified and the default should be ?œdefault??
 *
 * func.label_get - This function is called when an actual item object is
 * created. The data parameter is the data parameter passed to
 * elm_genlist_item_append() and related item creation functions. The obj
 * parameter is the genlist object and the part parameter is the string name
 * of the text part in the edje design that is listed as one of the possible
 * labels that can be set. This function must return a strudup()'ed string as
 * the caller will free() it when done.
 *
 * func.icon_get - This function is called when an actual item object is
 * created. The data parameter is the data parameter passed to
 * elm_genlist_item_append() and related item creation functions. The obj
 * parameter is the genlist object and the part parameter is the string name
 * of the icon part in the edje design that is listed as one of the possible
 * icons that can be set. This must return NULL for no object or a valid
 * object. The object will be deleted by genlist on shutdown or when the item
 * its unrealized.
 *
 * func.state_get - This function is called when an actual item object is
 * created. The data parameter is the data parameter passed to
 * elm_genlist_item_append() and related item creation functions. The obj
 * parameter is the genlist object and the part parameter is the string name
 * of the state part in the edje design that is listed as one of the possible
 * states that can be set. Return 0 for false or 1 for true. Genlist will
 * emit a signal to the edje object with ?œelm,state,XXX,active???œelm??when
 * true (the default is false), where XXX is the name of the part.
 *
 * func.del - This is called when elm_genlist_item_del() is called on an
 * item, elm_genlist_clear() is called on the genlist, or
 * elm_genlist_item_subitems_clear() is called to clear sub-items. This is
 * intended for use when actual genlist items are deleted, so any backing
 * data attached to the item (e.g. its data parameter on creation) can be
 * deleted.
 *
 * Items can be added by several calls. All of them return a Elm_Genlist_Item
 * handle that is an internal member inside the genlist. They all take a data
 * parameter that is meant to be used for a handle to the applications
 * internal data (eg the struct with the original item data). The parent
 * parameter is the parent genlist item this belongs to if it is a tree, and
 * NULL if there is no parent. The flags can be a bitmask of
 * ELM_GENLIST_ITEM_NONE and ELM_GENLIST_ITEM_SUBITEMS. If
 * ELM_GENLIST_ITEM_SUBITEMS is set then this item is displayed as a item
 * that is able to expand and have child items. The func parameter is a
 * convenience callback that is called when the item is selected and the data
 * parameter will be the func_data parameter, obj be the genlist object and
 * vent_info will be the genlist item.
 *
 * elm_genlist_item_append() appends an item to the end of the list, or if
 * there is a parent, to the end of all the child items of the parent.
 * elm_genlist_item_prepend() is the same but prepends to the beginning of
 * the list or children list. elm_genlist_item_insert_before() inserts at
 * item before another item and elm_genlist_item_insert_after() inserts after
 * the indicated item.
 *
 * The application can clear the list with elm_genlist_clear() which deletes
 * all the items in the list and elm_genlist_item_del() will delete a specific
 * item. elm_genlist_item_subitems_clear() will clear all items that are
 * children of the indicated parent item.
 *
 * If the application wants multiple items to be able to be selected,
 * elm_genlist_multi_select_set() can enable this. If the list is
 * single-selection only (the default), then elm_genlist_selected_item_get()
 * will return the selected item, if any, or NULL I none is selected. If the
 * list is multi-select then elm_genlist_selected_items_get() will return a
 * list (that is only valid as long as no items are modified (added, deleted,
 * selected or unselected).
 *
 * To help inspect list items you can jump to the item at the top of the list
 * with elm_genlist_first_item_get() which will return the item pointer, and
 * similarly elm_genlist_last_item_get() gets the item at the end of the list.
 * elm_genlist_item_next_get() and elm_genlist_item_prev_get() get the next
 * and previous items respectively relative to the indicated item. Using
 * these calls you can walk the entire item list/tree. Note that as a tree
 * the items are flattened in the list, so elm_genlist_item_parent_get() will
 * let you know which item is the parent (and thus know how to skip them if
 * wanted).
 *
 * There are also convenience functions. elm_genlist_item_genlist_get() will
 * return the genlist object the item belongs to.  elm_genlist_item_show()
 * will make the scroller scroll to show that specific item so its visible.
 * elm_genlist_item_data_get() returns the data pointer set by the item
 * creation functions.
 *
 * If an item changes (state of boolean changes, label or icons change),
 * then use elm_genlist_item_update() to have genlist update the item with
 * the new state. Genlist will re-realize the item thus call the functions
 * in the _Elm_Genlist_Item_Class for that item.
 *
 * To programmatically (un)select an item use elm_genlist_item_selected_set().
 * To get its selected state use elm_genlist_item_selected_get(). Similarly
 * to expand/contract and item and get its expanded state, use
 * elm_genlist_item_expanded_set() and elm_genlist_item_expanded_get(). And
 * again to make an item disabled (unable to be selected and appear
 * differently) use elm_genlist_item_disabled_set() to set this and
 * elm_genlist_item_disabled_get() to get the disabled state.
 *
 * In general to indicate how the genlist should expand items horizontally to
 * fill the list area, use elm_genlist_horizontal_mode_set(). Valid modes are
 * ELM_LIST_LIMIT and ELM_LIST_SCROLL . The default is ELM_LIST_SCROLL. This
 * mode means that if items are too wide to fit, the scroller will scroll
 * horizontally. Otherwise items are expanded to fill the width of the
 * viewport of the scroller. If it is ELM_LIST_LIMIT, Items will be expanded
 * to the viewport width and limited to that size. This can be combined with
 * a different style that uses edjes' ellipsis feature (cutting text off like
 * this: ?œtex...??.
 *
 * Items will only call their selection func and callback when first becoming
 * selected. Any further clicks will do nothing, unless you enable always
 * select with elm_genlist_always_select_mode_set(). This means even if
 * selected, every click will make the selected callbacks be called.
 * elm_genlist_no_select_mode_set() will turn off the ability to select
 * items entirely and they will neither appear selected nor call selected
 * callback functions.
 *
 * Remember that you can create new styles and add you own theme augmentation
 * per application with elm_theme_extension_add(). If you absolutely must
 * have a specific style that overrides any theme the user or system sets up
 * you can use elm_theme_overlay_add() to add such a file.
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _Item_Block Item_Block;
typedef struct _Pan Pan;
typedef struct _Edit_Data Edit_Data;

typedef enum _Elm_Genlist_Item_Move_effect_Mode
 {
	ELM_GENLIST_ITEM_MOVE_EFFECT_NONE		= 0,
	ELM_GENLIST_ITEM_MOVE_EFFECT_EXPAND		= (1 << 0),
	ELM_GENLIST_ITEM_MOVE_EFFECT_CONTRACT		= (1 << 1),
	ELM_GENLIST_ITEM_MOVE_EFFECT_EDIT_MODE		= (1 << 2),
} Elm_Genlist_Item_Move_effect_Mode;

typedef enum _Elm_Genlist_Item_Pinchzoom_effect_Mode
 {
	ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE		= 0,
	ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_EXPAND		= (1 << 0),
	ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT		= (1 << 1),
	ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH	= (1 << 2),
	ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_EXPAND_FINISH	= (1 << 3),
} Elm_Genlist_Item_Pinchzoom_effect_Mode;


#define GROUP_ALIGN_NORTH 1
#define GROUP_ALIGN_WEST 2
struct _Widget_Data
{
   Evas_Object *obj, *scr, *pan_smart;
   Eina_Inlist *items, *blocks, *group_items;
   Pan *pan;
   Evas_Coord pan_x, pan_y, minw, minh;
   Ecore_Job *calc_job, *update_job;
   Ecore_Idler *queue_idler;
   Eina_List *queue, *selected, *menuopened;
   Elm_Genlist_Item *show_item;
   Elm_List_Mode mode;
   Eina_Bool on_hold : 1;
   Eina_Bool multi : 1;
   Eina_Bool always_select : 1;
   Eina_Bool longpressed : 1;
   Eina_Bool wasselected : 1;
   Eina_Bool no_select : 1;
   Eina_Bool bring_in : 1;
   Eina_Bool compress : 1;
   Eina_Bool homogeneous : 1;
   Eina_Bool clear_me : 1;
   int walking;
   int item_width;
   int item_height;
   int max_items_per_block;
   double longpress_timeout;
   int edit_mode;
   int select_all_minh;
   Eina_Bool animate_edit_controls :1;
   Edit_Data *ed;

   Evas_Coord td1_x, td1_y, tu1_x, tu1_y;
   Evas_Coord td2_x, td2_y, tu2_x, tu2_y;
   Evas_Coord d1_x, d1_y, d2_x, d2_y;
   Evas_Coord acc_x1, acc_y1, acc_x2, acc_y2;
   Evas_Coord prev_multi_x, prev_multi_y;
   Eina_Bool multi_down : 1;
   Eina_Bool multi_touch : 1;
   Eina_List *edit_field;
   Eina_Bool selct_all : 1;
   Elm_Genlist_Item *select_all_item;

   Eina_Bool effect_mode : 1;
   Eina_Bool edit_mode_effect_mode : 1;   
   Eina_Bool pinch_zoom : 1;
   Eina_Bool pinch_zoom_reserve : 1;   
   int move_effect_mode;
   int pinchzoom_effect_mode;
   int pinch_it;
   int max_git_num; 	
   Ecore_Animator *item_moving_effect_timer;
   Evas_Object *alpha_bg;
   Evas_Object *point_rect;
   Elm_Genlist_Item *expand_item;
   int expand_item_cnt;
   int contract_pan_y;
   Evas_Coord start_y_pos;
   double effect_start;
   Eina_Bool queue_exception : 1;
   int item_count;
};
struct _Edit_Data
{
  Elm_Genlist_Edit_Class  *ec;
  Elm_Genlist_Item *del_item;
  Elm_Genlist_Item *reorder_item;
  Elm_Genlist_Item *reorder_rel;
  Evas_Object *del_confirm;
};

struct _Item_Block
{
   EINA_INLIST;
   int count;
   int num;
   Widget_Data *wd;
   Eina_List *items;
   Evas_Coord x, y, w, h, minw, minh;
   Eina_Bool want_unrealize : 1;
   Eina_Bool realized : 1;
   Eina_Bool changed : 1;
   Eina_Bool updateme : 1;
   Eina_Bool showme : 1;
   Evas_Coord reoder_y;
};

struct _Elm_Genlist_Item
{
   EINA_INLIST;
   Widget_Data *wd;
   Item_Block *block;
   Eina_List *items;
   Evas_Coord x, y, w, h, minw, minh, edx;
   const Elm_Genlist_Item_Class *itc;
   const void *data;
   Elm_Genlist_Item *parent;
   Elm_Genlist_Item_Flags flags;
   Elm_Genlist_GroupItem *group_item;
   struct
     {
        Evas_Smart_Cb func;
        const void *data;
     } func;

   Evas_Object *base, *spacer, *edit_obj;
   Eina_List *labels, *icons, *states, *icon_objs, *edit_icon_objs;
   Ecore_Timer *long_timer, *edit_long_timer;
   Evas_Coord dx, dy, scrl_x, scrl_y;
   Evas_Coord reoder_cavas_x, reoder_cavas_y;

   Elm_Genlist_Item *rel;
   int relcount;
   int walking;
   Eina_Bool before : 1;

   Eina_Bool want_unrealize : 1;
   Eina_Bool realized : 1;
   Eina_Bool selected : 1;
   Eina_Bool hilighted : 1;
   Eina_Bool expanded : 1;
   int expanded_depth;
   Eina_Bool disabled : 1;
   Eina_Bool display_only : 1;
   Eina_Bool mincalcd : 1;
   Eina_Bool queued : 1;
   Eina_Bool showme : 1;
   Eina_Bool delete_me : 1;
   Eina_Bool delete_check : 1;
   Eina_Bool del_confirm_state : 1;
   Eina_Bool down : 1;
   Eina_Bool dragging : 1;
   Eina_Bool updateme : 1;
   Eina_Bool reordering : 1;
   Eina_Bool menuopened : 1;
   Eina_Bool select_all_item : 1;
   Eina_Bool reorder_check: 1;
   Eina_Bool renamed : 1;

   int pad_left, pad_right;

   Evas_Coord old_scrl_y;
   Evas_Coord old_pad_left;
   int list_expanded;
   Eina_Bool effect_done : 1;   
};

#define ELM_GENLIST_ITEM_FROM_INLIST(item)      \
  ((item) ? EINA_INLIST_CONTAINER_GET(item, Elm_Genlist_Item) : NULL)

struct _Pan
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Widget_Data *wd;
};

struct _Elm_Genlist_GroupItem
{
   EINA_INLIST;
   Widget_Data *wd;
   Eina_List *items;
   Evas_Coord x, y, w, h, minw, minh;
   const Elm_Genlist_Item_Class *itc;
   const void *data;
   struct
     {
	Evas_Smart_Cb func;
	const void *data;
     } func;

   Evas_Object *base;
   Eina_List *labels, *icons, *states, *icon_objs;
   int align;
   Eina_Bool realized : 1;
   Eina_Bool delete_me : 1;
   Eina_Bool visible : 1;
   Eina_Bool mincalcd : 1;

   Evas_Coord old_y, finish_y;
   int num;
   Eina_Bool down : 1;
   Eina_Bool update_finish_y : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
//static void _show_region_hook(void *data, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _item_unrealize(Elm_Genlist_Item *it);
static void _item_block_unrealize(Item_Block *itb);
static void _calc_job(void *data);
static void _groupitem_remove(Elm_Genlist_GroupItem *git, Eina_Bool update_items);
static void _groupitem_unrealize(Elm_Genlist_GroupItem *git);
static Eina_Bool _edit_mode_reset(Widget_Data *wd);
static void _edit_controls_eval( Elm_Genlist_Item *it );
static void _move_edit_controls( Elm_Genlist_Item *it, int itx, int ity );
static Eina_Bool _item_moving_effect_timer_cb(void *data);
static Eina_Bool _edit_mode_item_moving_effect_cb(void *data);
static int _item_flip_effect_show(void *data);
static void _elm_genlist_pinch_zoom_execute(Evas_Object *obj, Eina_Bool emode);

static Evas_Smart_Class _pan_sc = EVAS_SMART_CLASS_INIT_VERSION;

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   if (wd->update_job) ecore_job_del(wd->update_job);
   free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_del(wd->pan_smart);
   wd->pan_smart = NULL;
   elm_genlist_clear(obj);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Item_Block *itb;
   Elm_Genlist_GroupItem *git;
   if (!wd) return;
   elm_smart_scroller_object_theme_set(obj, wd->scr, "genlist", "base", elm_widget_style_get(obj));
//   edje_object_scale_set(wd->scr, elm_widget_scale_get(obj) * _elm_config->scale);
   EINA_INLIST_FOREACH(wd->group_items, git)
    {
       _groupitem_unrealize(git);
    }
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
	Eina_List *l;
	Elm_Genlist_Item *it;
        
	if (itb->realized) _item_block_unrealize(itb);
	EINA_LIST_FOREACH(itb->items, l, it)
          it->mincalcd = EINA_FALSE;

	itb->changed = EINA_TRUE;
     }
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
   _sizing_eval(obj);
}

/*
static void
_show_region_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scr, x, y, w, h);
}
*/

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   evas_object_size_hint_min_get(wd->scr, &minw, &minh);
   evas_object_size_hint_max_get(wd->scr, &maxw, &maxh);
   minh = -1;
   if (wd->mode != ELM_LIST_LIMIT) minw = -1;
   else
     {
        Evas_Coord  vmw, vmh, vw, vh;
        
        minw = wd->minw;
        maxw = -1;
        elm_smart_scroller_child_viewport_size_get(wd->scr, &vw, &vh);
        if ((minw > 0) && (vw < minw)) vw = minw;
        else if ((maxw > 0) && (vw > maxw)) vw = maxw;
        minw = -1;
        edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr), &vmw, &vmh);
        minw = vmw + minw;
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_item_hilight(Elm_Genlist_Item *it)
{
   const char *selectraise;

   if ((it->wd->no_select) || (it->delete_me) || (it->hilighted) || (it->disabled) ||
         (it->wd->edit_mode != ELM_GENLIST_EDIT_MODE_NONE)) return;
   if( !it->menuopened )
   	edje_object_signal_emit(it->base, "elm,state,selected", "elm");
   selectraise = edje_object_data_get(it->base, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     {
	evas_object_raise(it->base);
	if( it->group_item && it->group_item->realized )
	  evas_object_raise(it->group_item->base);
     }
   it->hilighted = EINA_TRUE;
}

static void
_item_block_del(Elm_Genlist_Item *it)
{
   Eina_Inlist *il;
   Item_Block *itb = it->block;

   itb->items = eina_list_remove(itb->items, it);
   itb->count--;
   itb->changed = EINA_TRUE;
   if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
   it->wd->calc_job = ecore_job_add(_calc_job, it->wd);
   if (itb->count < 1)
     {
	il = EINA_INLIST_GET(itb);
	Item_Block *itbn = (Item_Block *)(il->next);
	if (it->parent)
	  it->parent->items = eina_list_remove(it->parent->items, it);
	else
	  it->wd->blocks = eina_inlist_remove(it->wd->blocks, il);
	free(itb);
	if (itbn) itbn->changed = EINA_TRUE;
     }
   else
     {
	if (itb->count < 16)
	  {
	     il = EINA_INLIST_GET(itb);
	     Item_Block *itbp = (Item_Block *)(il->prev);
	     Item_Block *itbn = (Item_Block *)(il->next);
	     if ((itbp) && ((itbp->count + itb->count) < 48))
	       {
		  Elm_Genlist_Item *it2;

		  EINA_LIST_FREE(itb->items, it2)
		    {
		       it2->block = itbp;
		       itbp->items = eina_list_append(itbp->items, it2);
		       itbp->count++;
		       itbp->changed = EINA_TRUE;
		    }
		  it->wd->blocks = eina_inlist_remove(it->wd->blocks, EINA_INLIST_GET(itb));
		  free(itb);
	       }
	     else if ((itbn) && ((itbn->count + itb->count) < 48))
	       {
		  while (itb->items)
		    {
		       Eina_List *last = eina_list_last(itb->items);
		       Elm_Genlist_Item *it2 = last->data;

		       it2->block = itbn;
		       itb->items = eina_list_remove_list(itb->items, last);
		       itbn->items = eina_list_prepend(itbn->items, it2);
		       itbn->count++;
		       itbn->changed = EINA_TRUE;
		    }
		  it->wd->blocks = 
                    eina_inlist_remove(it->wd->blocks, EINA_INLIST_GET(itb));
		  free(itb);
	       }
	  }
     }
}

static void
_item_del(Elm_Genlist_Item *it)
{
   elm_genlist_item_subitems_clear(it);
   it->wd->walking -= it->walking;
   if (it->wd->show_item == it) it->wd->show_item = NULL;
   if (it->selected) it->wd->selected = eina_list_remove(it->wd->selected, it);
   if (it->realized) _item_unrealize(it);
   if (it->block) _item_block_del(it);
   if ((!it->delete_me) && (it->itc->func.del))
     it->itc->func.del(it->data, it->wd->obj);
   it->delete_me = EINA_TRUE;
   if (it->queued)
     it->wd->queue = eina_list_remove(it->wd->queue, it);
   it->wd->items = eina_inlist_remove(it->wd->items, EINA_INLIST_GET(it));
   if (it->parent)
     it->parent->items = eina_list_remove(it->parent->items, it);
   if (it->long_timer) ecore_timer_del(it->long_timer);
   if( it->group_item )
     {
	it->group_item->items = eina_list_remove(it->group_item->items,it);
     }
   free(it);
}

static void
_item_select(Elm_Genlist_Item *it)
{
   if ((it->wd->no_select) || (it->delete_me) || (it->wd->edit_mode != ELM_GENLIST_EDIT_MODE_NONE)) return;
   if (it->selected)
     {
	if (it->wd->always_select) goto call;
	return;
     }
   it->selected = EINA_TRUE;
   it->wd->selected = eina_list_append(it->wd->selected, it);
   call:
   it->walking++;
   it->wd->walking++;
   if (it->func.func) it->func.func((void *)it->func.data, it->wd->obj, it);
   if (!it->delete_me)
     evas_object_smart_callback_call(it->wd->obj, "selected", it);
   it->walking--;
   it->wd->walking--;
   if ((it->wd->clear_me) && (!it->wd->walking))
      elm_genlist_clear(it->wd->obj);
   else
     {
        if ((!it->walking) && (it->delete_me))
          {
             if (!it->relcount) _item_del(it);
          }
     }
}

static void
_item_unselect(Elm_Genlist_Item *it)
{
   const char *stacking, *selectraise;

   if ((it->delete_me) || (!it->hilighted)) return;
   if( !it->menuopened )
   	edje_object_signal_emit(it->base, "elm,state,unselected", "elm");
   stacking = edje_object_data_get(it->base, "stacking");
   selectraise = edje_object_data_get(it->base, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     {
	if ((stacking) && (!strcmp(stacking, "below")))
	  evas_object_lower(it->base);
     }
   it->hilighted = EINA_FALSE;
   if (it->selected)
     {
	it->selected = EINA_FALSE;
	it->wd->selected = eina_list_remove(it->wd->selected, it);
	evas_object_smart_callback_call(it->wd->obj, "unselected", it);
     }
}

static void
_item_slide(Elm_Genlist_Item *it, Eina_Bool slide_to_right)
{
   const Eina_List *l;
   Elm_Genlist_Item *it2;
   const char *allow_slide;

   allow_slide = edje_object_data_get(it->base, "allow_slide");
   if (!allow_slide)
     return;

   if (atoi(allow_slide) != 1)
     return;

   if (slide_to_right)
     {
        if (!it->menuopened)
          edje_object_signal_emit(it->base, "elm,state,slide,right", "elm");
        it->wd->menuopened = eina_list_append(it->wd->menuopened, it);

        EINA_LIST_FOREACH(it->wd->menuopened, l, it2)
          {
             if (it2 != it)
               {
                  it2->menuopened = EINA_FALSE;
                  edje_object_signal_emit(it2->base, "elm,state,slide,left", "elm");
                  it2->wd->menuopened = eina_list_remove(it2->wd->menuopened, it2);
               }
          }
     }
   else
     {
        if (it->menuopened)
          edje_object_signal_emit(it->base, "elm,state,slide,left", "elm");
        it->wd->menuopened = eina_list_remove(it->wd->menuopened, it);
     }

   it->menuopened = slide_to_right;
}

static void
_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord minw = 0, minh = 0, x, y, dx, dy, adx, ady;
   Evas_Coord acc_x, acc_y;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
	if (!it->wd->on_hold)
	  {
	     it->wd->on_hold = EINA_TRUE;
             if (!it->wd->wasselected)
               _item_unselect(it);
	  }
     }
   if ((it->dragging) && (it->down))
     {
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        evas_object_smart_callback_call(it->wd->obj, "drag", it);
        return;
     }
   if ((!it->down)/* || (it->wd->on_hold)*/ || (it->wd->longpressed))
     {
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        return;
     }

   if (it->wd->multi_down)
     {
        acc_x = ev->prev.canvas.x - ev->cur.canvas.x;
        if (acc_x < 0)
          it->wd->acc_x1 = it->wd->acc_x1 - acc_x;
        else
          it->wd->acc_x1 = it->wd->acc_x1 + acc_x;

        acc_y = ev->prev.canvas.y - ev->cur.canvas.y;
        if (acc_y < 0)
          it->wd->acc_y1 = it->wd->acc_y1 - acc_y;
        else
          it->wd->acc_y1 = it->wd->acc_y1 + acc_y;

        return;
     }

   if (!it->display_only)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   x = ev->cur.canvas.x - x;
   y = ev->cur.canvas.y - y;
   dx = x - it->dx;
   adx = dx;
   if (adx < 0) adx = -dx;
   dy = y - it->dy;
   ady = dy;
   if (ady < 0) ady = -dy;
   minw /= 2;
   minh /= 2;
   if ((adx > minw) || (ady > minh))
     {
        it->dragging = 1;
        if (it->long_timer)
          {
             ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
          }
        if (!it->wd->wasselected)
          _item_unselect(it);
        if (dy < 0)
          {
             if (ady > adx)
               evas_object_smart_callback_call(it->wd->obj, "drag,start,up", it);
             else
               {
                  if (dx < 0)
                    {
                       evas_object_smart_callback_call(it->wd->obj,
                             "drag,start,left", it);
                       _item_slide( it, 0 );
                    }
                  else
                    {
                       evas_object_smart_callback_call(it->wd->obj,
                             "drag,start,right", it);
                       _item_slide( it, 1 );
                    }
               }
          }
        else
          {
             if (ady > adx)
               evas_object_smart_callback_call(it->wd->obj,
                     "drag,start,down", it);
             else
               {
                  if (dx < 0)
                    {
                       evas_object_smart_callback_call(it->wd->obj,
                             "drag,start,left", it);
                       _item_slide( it, 0 );
                    }
                  else
                    {
                       evas_object_smart_callback_call(it->wd->obj,
                             "drag,start,right", it);
                       _item_slide( it, 1 );
                    }
               }
          }
     }
}

static Eina_Bool
_long_press(void *data)
{
   Elm_Genlist_Item *it = data;

   it->long_timer = NULL;
   if ((it->disabled) || (it->dragging)) return ECORE_CALLBACK_CANCEL;
   it->wd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(it->wd->obj, "longpressed", it);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_edit_long_press(void *data)
{
  Elm_Genlist_Item *it = data; 
  Evas_Coord x, y;

  it->edit_long_timer = NULL;
  if ((it->disabled) || (it->dragging)) return 0;
  edje_object_signal_emit(it->edit_obj, "elm,action,item,reorder_start", "elm");

  evas_object_geometry_get(it->base, &x, &y, NULL, NULL);

  it->dx = it->reoder_cavas_x - x;
  it->dy = it->reoder_cavas_y - y;  

  evas_object_raise(it->base);
  evas_object_raise( it->edit_obj );

  it->wd->ed->reorder_item = it;
  it->wd->ed->reorder_item->reordering = 1;
  it->wd->ed->reorder_rel = NULL;
  elm_smart_scroller_hold_set(it->wd->scr, EINA_TRUE);

  return 0;
}
 
static void
_multi_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Multi_Down *ev = event_info;
   Evas_Coord dx, dy, adx, ady;

   if (it->long_timer)
     {
	ecore_timer_del(it->long_timer);
	it->long_timer = NULL;
     }

   dx = it->wd->td1_x - ev->canvas.x;
   adx = dx;
   if (adx < 0) adx = -dx;
   dy = it->wd->td1_y - ev->canvas.y;
   ady = dy;
   if (ady < 0) ady = -dy;

   if (adx < 60 && ady < 60)
     return;

   it->wd->multi_down = 1;
   it->wd->td2_x = ev->canvas.x;
   it->wd->td2_y = ev->canvas.y;
/*
	if( it->wd->effect_mode && it->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH ) 
	  {
		  evas_object_move(it->wd->point_rect, 2, (it->wd->td1_y + it->wd->td2_y) / 2);
		  evas_object_raise(it->wd->point_rect);
		  evas_object_show(it->wd->point_rect);
	  }
   fprintf(stderr, "\n MULTI_DOWN - BUTTON ID = %d, x= %d, y= %d\n", ev->device, it->wd->td2_x, it->wd->td2_y);
*/
}

static void
_multi_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Multi_Up *ev = event_info;
   Evas_Coord dy, uy, ady, auy;
   int multi_y_avg = 0;

   fprintf(stderr, "\n MULTI_UP - x= %d, y= %d down= %d multi_down= %d\n", ev->canvas.x, ev->canvas.y, it->down, it->wd->multi_down);

   if (!it->wd->multi_down)
     return;

   it->wd->multi_down = 0;
   it->wd->tu2_x = ev->canvas.x;
   it->wd->tu2_y = ev->canvas.y;
   it->wd->d2_x = ev->canvas.x - it->wd->td2_x;
   it->wd->d2_y = ev->canvas.y - it->wd->td2_y;

   evas_object_lower(it->wd->point_rect);
   evas_object_hide(it->wd->point_rect);

   if (it->down)
     {
	it->wd->multi_touch = EINA_TRUE;
     }
   else
     {
	fprintf(stderr, "CHECK d1_x= %d, d2_x= %d, d1_y= %d, d2_y= %d\n", it->wd->d1_x, it->wd->d2_x, it->wd->d1_y, it->wd->d2_y);

	if ( (it->wd->d1_x > 180) && (it->wd->d2_x > 180) )
	  {
	     // Two finger : Left -> Right
	     fprintf(stderr, "L->R acc_y1= %d, acc_y2= %d\n", it->wd->acc_y1, it->wd->acc_y2);
	     if (it->wd->acc_y1 < 200 && it->wd->acc_y2 < 200)
	       evas_object_smart_callback_call(it->wd->obj, "multi_touch,left,right", it);
	  }
	else if ( (it->wd->d1_y > 180) && (it->wd->d2_y > 180) )
	  {
	     // Two finger : Top -> Bottom
	     fprintf(stderr, "T->B acc_x1= %d, acc_x2= %d\n", it->wd->acc_x1, it->wd->acc_x2);
	     if (it->wd->acc_x1 < 200 && it->wd->acc_x2 < 200)
	       evas_object_smart_callback_call(it->wd->obj, "multi_touch,top,bottom", it);
	  }
	else
	  {
	     dy = it->wd->td1_y - it->wd->td2_y;
	     if (dy < 0)
	       ady = -dy;
	     else
	       ady = dy;

	     uy = it->wd->tu1_y - it->wd->tu2_y;
	     if (uy < 0)
	       auy = -uy;
	     else
	       auy = uy;

	     if (auy < ady)
	       {
		  if (auy < ady*0.8)
		    {
		       // Two finger : Pinch Out
		       evas_object_smart_callback_call(it->wd->obj, "multi_touch,pinch,out", it);

				 if(it->wd->effect_mode == EINA_TRUE)
			       _elm_genlist_pinch_zoom_execute(it->wd->obj, 1);
		    }
	       }
	     else
	       {
		  if (ady < auy*0.9)
		    {
		       // Two finger : Pinch In
		       evas_object_smart_callback_call(it->wd->obj, "multi_touch,pinch,in", it);
		       
		       if(it->wd->effect_mode == EINA_TRUE)
		       {
			       multi_y_avg= (it->wd->td1_y + it->wd->td2_y) / 2;
			       it->wd->pinch_it = (multi_y_avg / it->group_item->h + it->wd->contract_pan_y / it->group_item->h) - 2;
			       fprintf(stderr," pinch,in!! it ================ it->y = %d it->old_y = %d it = %d it->wd->td1_y = %d it->wd->td2_y = %d pinch_it = %d  it->wd->contract_pan_y = %d\n", it->y, it->old_scrl_y, it->old_scrl_y / 30,  it->wd->td1_y, it->wd->td2_y, it->wd->pinch_it, it->wd->contract_pan_y);
			       _elm_genlist_pinch_zoom_execute(it->wd->obj, 0);
		       }
		    }
	       }
	  }

	it->wd->acc_x1 = 0;
	it->wd->acc_y1 = 0;
	it->wd->acc_x2 = 0;
	it->wd->acc_y2 = 0;
	it->wd->prev_multi_x = 0;
	it->wd->prev_multi_y = 0;
     }
}

static void
_multi_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Multi_Move *ev = event_info;
   Evas_Coord acc_x, acc_y;

   if (!it->wd->prev_multi_x)
     {
	it->wd->prev_multi_x = ev->cur.canvas.x;
	it->wd->prev_multi_y = ev->cur.canvas.y;
	return;
     }

   acc_x = it->wd->prev_multi_x - ev->cur.canvas.x;
   if (acc_x < 0)
     it->wd->acc_x2 = it->wd->acc_x2 - acc_x;
   else
     it->wd->acc_x2 = it->wd->acc_x2 + acc_x;

   acc_y = it->wd->prev_multi_y - ev->cur.canvas.y;
   if (acc_y < 0)
     it->wd->acc_y2 = it->wd->acc_y2 - acc_y;
   else
     it->wd->acc_y2 = it->wd->acc_y2 + acc_y;

   it->wd->prev_multi_x = ev->cur.canvas.x;
   it->wd->prev_multi_y = ev->cur.canvas.y;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord x, y;

   it->wd->td1_x = ev->canvas.x;
   it->wd->td1_y = ev->canvas.y;
   if( it->wd->effect_mode && it->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH) 
   {
       elm_smart_scroller_bounce_allow_set(it->wd->scr, EINA_FALSE, EINA_TRUE);
       return;
   }
  
   if( it->wd->edit_mode != ELM_GENLIST_EDIT_MODE_NONE )
     (void)_edit_mode_reset( it->wd );
   if (ev->button != 1) return;

   if(it->wd->edit_field && !it->renamed)
     elm_genlist_item_rename_mode_set(it, 0);

   it->down = 1;
   it->dragging  = 0;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   it->dx = ev->canvas.x - x;
   it->dy = ev->canvas.y - y;
   it->wd->longpressed = EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) it->wd->on_hold = EINA_TRUE;
   else it->wd->on_hold = EINA_FALSE;
   it->wd->wasselected = it->selected;
   _item_hilight(it);
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(it->wd->obj, "clicked", it);
   if (it->long_timer) ecore_timer_del(it->long_timer);
   if (it->realized)
     it->long_timer = ecore_timer_add(it->wd->longpress_timeout, _long_press, it);
   else
     it->long_timer = NULL;
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Genlist_Item *it = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Eina_Bool dragged = EINA_FALSE;
   Evas_Coord dy, uy, ady, auy;
   int multi_y_avg;

   if (ev->button != 1) return;
   it->down = 0;
   it->wd->acc_x1 = 0;
   it->wd->acc_y1 = 0;

   it->wd->tu1_x = ev->canvas.x;
   it->wd->tu1_y = ev->canvas.y;
   it->wd->d1_x = ev->canvas.x - it->wd->td1_x;
   it->wd->d1_y = ev->canvas.y - it->wd->td1_y;

//   evas_object_lower(it->wd->point_rect);
//   evas_object_hide(it->wd->point_rect);
   if( it->wd->effect_mode && it->wd->pinchzoom_effect_mode != ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE)  return;

   if (!it->wd->multi_down && it->wd->multi_touch == EINA_TRUE)
     {
	if ( (it->wd->d1_x > 180) && (it->wd->d2_x > 180) )
	  {
	     // Two finger : Left -> Right
	     fprintf(stderr, "L->R acc_y1= %d, acc_y2= %d\n", it->wd->acc_y1, it->wd->acc_y2);
	     if (it->wd->acc_y1 < 200 && it->wd->acc_y2 < 200)
	       evas_object_smart_callback_call(it->wd->obj, "multi_touch,left,right", it);
	  }
	else if ( (it->wd->d1_y > 180) && (it->wd->d2_y > 180) )
	  {
	     // Two finger : Top -> Bottom
	     fprintf(stderr, "T->B acc_x1= %d, acc_x2= %d\n", it->wd->acc_x1, it->wd->acc_x2);
	     if (it->wd->acc_x1 < 200 && it->wd->acc_x2 < 200)
	       evas_object_smart_callback_call(it->wd->obj, "multi_touch,top,bottom", it);
	  }
	else
	  {
	     dy = it->wd->td1_y - it->wd->td2_y;
	     if (dy < 0) ady = -dy;
	     else ady = dy;

	     uy = it->wd->tu1_y - it->wd->tu2_y;
	     if (uy < 0) auy = -uy;
	     else auy = uy;

	     if (auy < ady)
	       {
		  if (auy < ady*0.8)
		    {
		       // Two finger : Pinch Out
		       evas_object_smart_callback_call(it->wd->obj, "multi_touch,pinch,out", it);
		       if(it->wd->effect_mode == EINA_TRUE)
			       _elm_genlist_pinch_zoom_execute(it->wd->obj, 1);
		    }
	       }
	     else
	       {
		  if (ady < auy*0.9)
		    {
		       // Two finger : Pinch In
		       evas_object_smart_callback_call(it->wd->obj, "multi_touch,pinch,in", it);
		       if(it->wd->effect_mode == EINA_TRUE)
		       {
			       multi_y_avg= (it->wd->td1_y + it->wd->td2_y) / 2;
			       it->wd->pinch_it = (multi_y_avg / it->group_item->h + it->wd->contract_pan_y / it->group_item->h) - 2;
                fprintf(stderr,"mouse up pinch,in!! it ================ it->y = %d it->old_y = %d it = %d it->wd->td1_y = %d it->wd->td2_y = %d pinch_it = %d  it->wd->contract_pan_y = %d \n", it->y, it->old_scrl_y, it->old_scrl_y / 30,  it->wd->td1_y, it->wd->td2_y, it->wd->pinch_it, it->wd->contract_pan_y);
                _elm_genlist_pinch_zoom_execute(it->wd->obj, 0);
		       }
		    }
	       }

	  }

	it->wd->acc_x1 = 0;
	it->wd->acc_y1 = 0;
	it->wd->acc_x2 = 0;
	it->wd->acc_y2 = 0;
	it->wd->prev_multi_x = 0;
	it->wd->prev_multi_y = 0;
	it->wd->multi_down = 0;
     }

   it->wd->multi_touch = EINA_FALSE;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) it->wd->on_hold = EINA_TRUE;
   else it->wd->on_hold = EINA_FALSE;
   if (it->long_timer)
     {
        ecore_timer_del(it->long_timer);
        it->long_timer = NULL;
     }
   if (it->dragging)
     {
	it->dragging = 0;
	evas_object_smart_callback_call(it->wd->obj, "drag,stop", it);
	dragged = 1;
     }
   if (it->wd->on_hold)
     {
        it->wd->longpressed = EINA_FALSE;
	it->wd->on_hold = EINA_FALSE;
	return;
     }
   if (it->wd->longpressed)
     {
        it->wd->longpressed = EINA_FALSE;
        if (!it->wd->wasselected)
          _item_unselect(it);
        it->wd->wasselected = 0;
        return;
     }
   if (dragged)
     {
        if (it->want_unrealize)
          {
             _item_unrealize(it);
             if (it->block->want_unrealize)
               _item_block_unrealize(it->block);
          }
     }
   if ((it->disabled) || (dragged)) return;
   if (it->wd->multi)
     {
	if (!it->selected && !it->menuopened)
	  {
	     _item_hilight(it);
	     _item_select(it);
	  }
	else _item_unselect(it);
     }
   else
     {
	if (!it->selected)
	  {
	     Widget_Data *wd = it->wd;
             if (wd)
               {
                  while (wd->selected) _item_unselect(wd->selected->data);
               }
	  }
	else
	  {
	     const Eina_List *l, *l_next;
	     Elm_Genlist_Item *it2;

	     EINA_LIST_FOREACH_SAFE(it->wd->selected, l, l_next, it2)
	       if (it2 != it) _item_unselect(it2);
//	     _item_hilight(it);
//	     _item_select(it);
	  }
	if (!it->menuopened)
	  {
	     _item_hilight(it);
	     _item_select(it);
	  }
     }
}

static void
_signal_expand_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Genlist_Item *it = data;

   if (it->expanded)
     evas_object_smart_callback_call(it->wd->obj, "contract,request", it);
   else
     evas_object_smart_callback_call(it->wd->obj, "expand,request", it);
}

static void
_signal_expand(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Genlist_Item *it = data;

   if (!it->expanded)
     evas_object_smart_callback_call(it->wd->obj, "expand,request", it);
}

static void
_signal_contract(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Genlist_Item *it = data;

   if (it->expanded)
     evas_object_smart_callback_call(it->wd->obj, "contract,request", it);
}

static void
_group_item_click_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Genlist_GroupItem *git = data;
   elm_smart_scroller_bounce_allow_set(git->wd->scr, EINA_FALSE, EINA_TRUE);
   if(git->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH)
   {
	   git->wd->pinch_it = git->num;
	   _elm_genlist_pinch_zoom_execute(git->wd->obj, 0);   
   }
  return;
}

static void
_groupitem_realize(Elm_Genlist_Item *it, int calc)
{
	char buf[1024];
	const char *align;
 	Elm_Genlist_GroupItem *git = it->group_item;
	Evas_Coord ox, oy, ow, oh;

	if (calc && git->realized)
        {
         git->wd->start_y_pos += it->h;
	}

	if ((git->realized) || (git->delete_me)) return;
	git->base = edje_object_add(evas_object_evas_get(git->wd->obj));
	edje_object_scale_set(git->base, elm_widget_scale_get(git->wd->obj) *
			_elm_config->scale);
	evas_object_smart_member_add(git->base, git->wd->pan_smart);
	elm_widget_sub_object_add(git->wd->obj, git->base);

	strncpy(buf, "item/", sizeof(buf));
	strncat(buf, git->itc->item_style, sizeof(buf) - strlen(buf));
	_elm_theme_object_set(git->wd->obj, git->base, "genlist", buf, elm_widget_style_get(git->wd->obj));

	align = edje_object_data_get(git->base, "orientation");
	git->align = GROUP_ALIGN_NORTH;
	if (align)
	  {
		  if (!strcmp(align, "top"))
			 git->align = GROUP_ALIGN_NORTH;
		  else if (!strcmp(align, "left"))
			 git->align = GROUP_ALIGN_WEST;
		  else git->align = GROUP_ALIGN_NORTH;
	  }

	if (git->itc->func.label_get)
	  {
		  const Eina_List *l;
		  const char *key;

		  git->labels = _elm_stringlist_get(edje_object_data_get(git->base, "labels"));
		  EINA_LIST_FOREACH(git->labels, l, key)
			 {
				 char *s = git->itc->func.label_get(git->data, git->wd->obj, l->data);

				 if (s)
					{
						edje_object_part_text_set(git->base, l->data, s);
						free(s);
					}
			 }
	  }

	if (git->itc->func.icon_get)
	  {
		  const Eina_List *l;
		  const char *key;

		  git->icons = _elm_stringlist_get(edje_object_data_get(git->base, "icons"));
		  EINA_LIST_FOREACH(git->icons, l, key)
			 {
				 Evas_Object *ic = git->itc->func.icon_get(git->data, git->wd->obj, l->data);

				 if (ic)
					{
						git->icon_objs = eina_list_append(git->icon_objs, ic);
						edje_object_part_swallow(git->base, key, ic);
						evas_object_show(ic);
						elm_widget_sub_object_add(git->wd->obj, ic);
					}
			 }

	  }
	if (git->itc->func.state_get)
	  {
		  const Eina_List *l;
		  const char *key;

		  git->states = _elm_stringlist_get(edje_object_data_get(git->base, "states"));
		  EINA_LIST_FOREACH(git->states, l, key)
			 {
				 Eina_Bool on = git->itc->func.state_get(git->data, git->wd->obj, l->data);

				 if (on)
					{
						snprintf(buf, sizeof(buf), "elm,state,%s,active", key);
						edje_object_signal_emit(git->base, buf, "elm");
					}
			 }
	  }
	if (!git->mincalcd)
	  {
		  Evas_Coord mw = -1, mh = -1;

		  edje_object_size_min_restricted_calc(git->base, &mw, &mh, mw, mh);

		  git->w = git->minw = mw;
		  git->h = git->minh = mh;
		  git->mincalcd = EINA_TRUE;
	  }
	evas_object_geometry_get(git->wd->pan_smart, &ox, &oy, &ow, &oh);      

	if(!oy)
	  git->update_finish_y = EINA_TRUE;

	git->y = git->finish_y = oy + git->wd->start_y_pos;
	git->wd->start_y_pos += it->h + git->h;

	git->num = ++git->wd->max_git_num;
	
	edje_object_signal_callback_add(git->base, "elm,title_action,item,click",
			"elm", _group_item_click_cb, git);
	evas_object_show(git->base);

	git->realized = EINA_TRUE;
}

static void
_groupitem_unrealize(Elm_Genlist_GroupItem *git)
{
	Evas_Object *icon;

	if (!git->realized) return;
	evas_object_del(git->base);
	git->base = NULL;
	_elm_stringlist_free(git->labels);
	git->labels = NULL;
	_elm_stringlist_free(git->icons);
	git->icons = NULL;
	_elm_stringlist_free(git->states);

	EINA_LIST_FREE(git->icon_objs, icon)
		evas_object_del(icon);

	if(git->wd->max_git_num)
		git->wd->max_git_num--;
	git->states = NULL;
	git->realized = EINA_FALSE;
}

static void
_groupitem_remove(Elm_Genlist_GroupItem *git, Eina_Bool update_items)
{
   Elm_Genlist_Item *it;
   const Eina_List *l;

   if (!git) return;

   if( git->realized )
      _groupitem_unrealize( git );

   git->wd->group_items = eina_inlist_remove(git->wd->group_items,EINA_INLIST_GET(git));

   if( update_items  )
   {
       EINA_LIST_FOREACH(git->items,l, it)
       {
            it->group_item = NULL;
            elm_genlist_item_update(it);
       }
   }

   if (git->itc->func.del) git->itc->func.del(git->data, git->wd->obj);
   free(git);
}

static void
_item_realize(Elm_Genlist_Item *it, int in, int calc)
{
   Elm_Genlist_Item *it2;
   const char *stacking;
   const char *treesize;
   char buf[1024];
   int depth, tsize = 20;

   if ((it->realized) || (it->delete_me)) return;
   it->base = edje_object_add(evas_object_evas_get(it->wd->obj));
   edje_object_scale_set(it->base, elm_widget_scale_get(it->wd->obj) *
                         _elm_config->scale);
   evas_object_smart_member_add(it->base, it->wd->pan_smart);
   elm_widget_sub_object_add(it->wd->obj, it->base);

   if (it->flags & ELM_GENLIST_ITEM_SUBITEMS) strncpy(buf, "tree", sizeof(buf));
   else strncpy(buf, "item", sizeof(buf));
   if (it->wd->compress) strncat(buf, "_compress", sizeof(buf) - strlen(buf));

   if (in & 0x1) strncat(buf, "_odd", sizeof(buf) - strlen(buf));
   strncat(buf, "/", sizeof(buf) - strlen(buf));
   strncat(buf, it->itc->item_style, sizeof(buf) - strlen(buf));

   _elm_theme_object_set(it->wd->obj, it->base, "genlist", buf, elm_widget_style_get(it->wd->obj));
   it->spacer = evas_object_rectangle_add(evas_object_evas_get(it->wd->obj));
   evas_object_color_set(it->spacer, 0, 0, 0, 0);
   elm_widget_sub_object_add(it->wd->obj, it->spacer);
   for (it2 = it, depth = 0; it2->parent; it2 = it2->parent) depth += 1;
   it->expanded_depth = depth;
   treesize = edje_object_data_get(it->base, "treesize");
   if (treesize) tsize = atoi(treesize);
   evas_object_size_hint_min_set(it->spacer,
                                 (depth * tsize) * _elm_config->scale, 1);
   edje_object_part_swallow(it->base, "elm.swallow.pad", it->spacer);
   if (!calc)
     {
	edje_object_signal_callback_add(it->base, "elm,action,expand,toggle",
                                        "elm", _signal_expand_toggle, it);
	edje_object_signal_callback_add(it->base, "elm,action,expand", "elm",
                                        _signal_expand, it);
	edje_object_signal_callback_add(it->base, "elm,action,contract",
                                        "elm", _signal_contract, it);
	stacking = edje_object_data_get(it->base, "stacking");
	if (stacking)
	  {
	     if (!strcmp(stacking, "below")) evas_object_lower(it->base);
	     else if (!strcmp(stacking, "above")) evas_object_raise(it->base);
	  }
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_DOWN,
				       _mouse_down, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_UP,
				       _mouse_up, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MOUSE_MOVE,
				       _mouse_move, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MULTI_DOWN,
				       _multi_down, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MULTI_UP,
				       _multi_up, it);
	evas_object_event_callback_add(it->base, EVAS_CALLBACK_MULTI_MOVE,
				       _multi_move, it);
	if (it->selected && !it->menuopened)
	  edje_object_signal_emit(it->base, "elm,state,selected", "elm");
	if (it->disabled)
	  edje_object_signal_emit(it->base, "elm,state,disabled", "elm");
	if (it->expanded)
	  edje_object_signal_emit(it->base, "elm,state,expanded", "elm");
     }

   if (calc && it->wd->homogeneous && it->wd->item_width)
     {
	/* homogenous genlist shortcut */
	if (!it->mincalcd)
	  {
	     it->w = it->minw = it->wd->item_width;
	     it->h = it->minh = it->wd->item_height;
	     it->mincalcd = EINA_TRUE;
	  }
     }
   else
     {
 	if(!strcmp(it->itc->item_style, "select_all")) {
	  const Eina_List *l;
	  const char *key;
			  
	  it->labels = _elm_stringlist_get(edje_object_data_get(it->base, "labels"));
	  EINA_LIST_FOREACH(it->labels, l, key) {
	     edje_object_part_text_set(it->base, l->data, "Select all");
	  }
	}

	if (it->itc->func.label_get)
	  {
	     const Eina_List *l;
	     const char *key;

	     it->labels = _elm_stringlist_get(edje_object_data_get(it->base, "labels"));
	     EINA_LIST_FOREACH(it->labels, l, key)
	       {
		  char *s = it->itc->func.label_get(it->data, it->wd->obj, l->data);

		  if (s)
		    {
		       edje_object_part_text_set(it->base, l->data, s);
		       free(s);
		    }
	       }
	  }
	if (it->itc->func.icon_get)
	  {
	     const Eina_List *l;
	     const char *key;

	     it->icons = _elm_stringlist_get(edje_object_data_get(it->base, "icons"));
	     EINA_LIST_FOREACH(it->icons, l, key)
	       {
		  Evas_Object *ic = it->itc->func.icon_get(it->data, it->wd->obj, l->data);

		  if (ic)
		    {
		       it->icon_objs = eina_list_append(it->icon_objs, ic);
		       edje_object_part_swallow(it->base, key, ic);
		       evas_object_show(ic);
		       elm_widget_sub_object_add(it->wd->obj, ic);
		    }
	       }
	  }
	if (it->itc->func.state_get)
	  {
	     const Eina_List *l;
	     const char *key;

	     it->states = _elm_stringlist_get(edje_object_data_get(it->base, "states"));
	     EINA_LIST_FOREACH(it->states, l, key)
	       {
		  Eina_Bool on = it->itc->func.state_get(it->data, it->wd->obj, l->data);

		  if (on)
		    {
		       snprintf(buf, sizeof(buf), "elm,state,%s,active", key);
		       edje_object_signal_emit(it->base, buf, "elm");
		    }
	       }
	  }
	if (!it->mincalcd)
	  {
	     Evas_Coord mw = -1, mh = -1;
             
             if (!it->display_only)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
	     edje_object_size_min_restricted_calc(it->base, &mw, &mh, mw, mh);
             if (!it->display_only)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
	     it->w = it->minw = mw;
	     it->h = it->minh = mh;
	     it->mincalcd = EINA_TRUE;

	     if (!in && it->wd->homogeneous)
	       {
		  it->wd->item_width = mw;
		  it->wd->item_height = mh;
	       }
	  }
	if (!calc) evas_object_show(it->base);
     }
   it->realized = EINA_TRUE;
   it->want_unrealize = EINA_FALSE;
   if(it->group_item)
     _groupitem_realize(it, calc);
   if( ELM_GENLIST_EDIT_MODE_NONE != it->wd->edit_mode ) 
   {
	Evas_Object *icon;
	EINA_LIST_FREE(it->edit_icon_objs, icon)
	evas_object_del(icon);
	_edit_controls_eval(it);
   }
}

static void
_item_unrealize(Elm_Genlist_Item *it)
{
   Evas_Object *icon;

   if (!it->realized) return;
   it->menuopened = EINA_FALSE;
   it->wd->menuopened = eina_list_remove(it->wd->menuopened, it);
   if (it->long_timer)
     {
        ecore_timer_del(it->long_timer);
        it->long_timer = NULL;
     }
   evas_object_del(it->base);
   it->base = NULL;
   evas_object_del(it->spacer);
   it->spacer = NULL;
   if(it->edit_obj)
     evas_object_del(it->edit_obj);
   it->edit_obj = NULL;
   _elm_stringlist_free(it->labels);
   it->labels = NULL;
   _elm_stringlist_free(it->icons);
   it->icons = NULL;
   _elm_stringlist_free(it->states);

   EINA_LIST_FREE(it->icon_objs, icon)
      evas_object_del(icon);

   EINA_LIST_FREE(it->edit_icon_objs, icon)
      evas_object_del(icon);

   it->states = NULL;
   it->realized = EINA_FALSE;
   it->want_unrealize = EINA_FALSE;
}

static int
_item_block_recalc(Item_Block *itb, int in, int qadd, int norender)
{
   const Eina_List *l;
   Elm_Genlist_Item *it;
   Elm_Genlist_GroupItem *git = NULL;
   Evas_Coord minw = 0, minh = 0;
   int showme = 0, changed = 0;
   Evas_Coord y = 0;
 
   itb->num = in;
   EINA_LIST_FOREACH(itb->items, l, it)
     {
	if (it->delete_me) continue;
	showme |= it->showme;
	if (!itb->realized)
	  {
             if (qadd)
               {
                  if (!it->mincalcd) changed = 1;
                  if (changed)
                    {
                       _item_realize(it, in, 1);
                       _item_unrealize(it);
                    }
               }
             else
               {
                  _item_realize(it, in, 1);
                  _item_unrealize(it);
               }
	  }
	else
          {
             Eina_Bool was_realized = it->realized;

             _item_realize(it, in, 0);
             if (!was_realized)
               evas_object_smart_callback_call(it->wd->obj, "realized", it);
          }
	minh += it->minh;
	if (minw < it->minw) minw = it->minw;
	in++;
	it->x = 0;
	it->y = y;
	y += it->h;
	if( git != it->group_item )
	{
            git = it->group_item;

            if( git && git->align == GROUP_ALIGN_NORTH && git->items->data == it) //Add Place holder for Group title
            {
                minh += git->minh;
                it->y += git->minh;
                y += git->minh;
            }
	}
     }
   itb->minw = minw;
   itb->minh = minh;
   itb->changed = EINA_FALSE;
   /* force an evas norender to garbage collect deleted objects */
   if (norender) evas_norender(evas_object_evas_get(itb->wd->obj));
   return showme;
}

static void
_item_block_realize(Item_Block *itb, int in, int full)
{
   const Eina_List *l;
   Elm_Genlist_Item *it;

   if (itb->realized) return;
   EINA_LIST_FOREACH(itb->items, l, it)
     {
	if (it->delete_me) continue;
	if (full)
          {
             Eina_Bool was_realized = it->realized;

             _item_realize(it, in, 0);
             if (!was_realized)
               evas_object_smart_callback_call(it->wd->obj, "realized", it);
          }
	in++;
     }
   itb->realized = EINA_TRUE;
   itb->want_unrealize = EINA_FALSE;
}

static void
_item_block_unrealize(Item_Block *itb)
{
   const Eina_List *l;
   Elm_Genlist_Item *it;
   int dragging = 0;

   if (!itb->realized) return;
   EINA_LIST_FOREACH(itb->items, l, it)
     {
        if (it->dragging || it->reordering)
          {
             dragging = 1;
             it->want_unrealize = EINA_TRUE;
          }
        else
          _item_unrealize(it);
     }
   if (!dragging)
     {
        itb->realized = EINA_FALSE;
        itb->want_unrealize = EINA_TRUE;
     }
   else
     itb->want_unrealize = EINA_FALSE;
}

static void
_delete_confirm_cb(void *data, Evas_Object *obj, void *event_info)
{
  Widget_Data *wd = data;
  evas_object_hide( wd->ed->del_confirm );
  if( wd->ed->ec && wd->ed->ec->remove  )
    wd->ed->ec->remove(wd->obj, wd->ed->del_item);
  wd->ed->del_item = NULL;
}

static void
_hide_delete_confirm_object (void *data, Evas_Object *obj, const char *emission, const char *source)
{
  const char *del_icon_part;
  Elm_Genlist_Item *it = data;
  del_icon_part = edje_object_data_get(it->edit_obj, "del_confirm");
  if (del_icon_part)
    edje_object_part_unswallow( it->edit_obj, it->wd->ed->del_confirm );

   evas_object_hide( it->wd->ed->del_confirm );
}

static void
_remove_item_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
  const char *del_conf_style;
  Elm_Genlist_Item *it = data;
  if(_edit_mode_reset( it->wd ))
     return;

  if (it->edit_long_timer) 
  {
	  ecore_timer_del(it->edit_long_timer);
	  it->edit_long_timer = NULL;
  }
 
  if( it->del_confirm_state )
  {
	  it->del_confirm_state = 0;
   	  it->delete_check = 0;
	  edje_object_signal_emit(it->edit_obj, "elm,state,del,animated,enable", "elm");
	  it->wd->selct_all = 0;
          if(it->wd->select_all_item)
	     edje_object_signal_emit(it->wd->select_all_item->base, "elm,state,del,animated,enable", "elm");

	   if (!it->wd->selct_all && it->wd->ed->ec->item_selected)
	  {
		  it->wd->ed->ec->item_selected(it->data, it, it->delete_check);
	  }
	  return;
  }

  it->del_confirm_state = 1;
  it->delete_check = 1;

  it->wd->ed->del_item = it;

  if (!it->wd->selct_all && it->wd->ed->ec->item_selected)
  {
	  it->wd->ed->ec->item_selected(it->data, it, it->delete_check);
  }

  del_conf_style = edje_object_data_get(it->edit_obj, "del_button_style");
  if(del_conf_style )
    elm_object_style_set( it->wd->ed->del_confirm, del_conf_style);

/*
   del_icon_part = edje_object_data_get(it->edit_obj, "del_confirm");
   if (del_icon_part)
     edje_object_part_swallow(it->edit_obj, del_icon_part, it->wd->ed->del_confirm);
   evas_object_show( it->wd->ed->del_confirm );
*/
   edje_object_signal_emit(it->edit_obj, "elm,state,del_confirm", "elm");
}

static void
_insert_new_item_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
  Elm_Genlist_Item *it = data;

  if(_edit_mode_reset( it->wd ))
     return;

  if( it->wd->ed->ec && it->wd->ed->ec->insert_new )
    it->wd->ed->ec->insert_new(it->wd->obj, it);
}

static Eina_Bool
_edit_mode_reset(Widget_Data *wd)
{
   /*
  if(wd->ed->del_confirm_state)
  {
       //edje_object_signal_emit(wd->ed->del_item->edit_obj, "elm,state,delete", "elm");
       //wd->ed->del_confirm_state = 0;
       //wd->ed->del_item = NULL;
       return EINA_TRUE;
  }
  */
  return EINA_FALSE;
}

static void
_reorder_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
  Elm_Genlist_Item *it = data;
  Evas_Event_Mouse_Down *ev = event_info;
  Evas_Coord x, y;

  if(!elm_genlist_item_next_get(it))
      return;
   
   if(it->wd->edit_field && !it->renamed)
     elm_genlist_item_rename_mode_set(it, 0);

   if(!(it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_REORDER))
     return;

  if(_edit_mode_reset( it->wd ) )
    return;

  it->dragging = 0;
  it->down = 1;
 
  it->reoder_cavas_x = ev->canvas.x;
  it->reoder_cavas_y = ev->canvas.y;

  evas_object_geometry_get(obj, &x, &y, NULL, NULL);
  it->dx = ev->canvas.x - x;
  it->dy = ev->canvas.y - y;

	if (it->edit_long_timer)
	  {
	     ecore_timer_del(it->edit_long_timer);
	     it->edit_long_timer = NULL;
	  }

  if (it->realized) {
     it->edit_long_timer = ecore_timer_add(0.3,_edit_long_press, it);
  }
  else
     it->edit_long_timer = NULL;
     
}

static void
_reorder_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
  Elm_Genlist_Item *it = data;
  
  Item_Block *itb;
   EINA_INLIST_FOREACH(it->wd->blocks, itb)
   {
	 	itb->reoder_y = 0;	 	
   }

  if (it->edit_long_timer)
  {
      ecore_timer_del(it->edit_long_timer);
      it->edit_long_timer = NULL;
  }
   
  it->down = 0;

  if (it->wd->ed->ec->selected)
  {
	  it->wd->ed->ec->selected(NULL, it, 1);
  }

  if( it->reordering && it->wd->ed->reorder_item )
     {
       it->wd->ed->reorder_item->reordering = 0;
       edje_object_signal_emit(it->wd->ed->reorder_item->edit_obj, "elm,action,item,reorder_end", "elm");
       elm_smart_scroller_hold_set(it->wd->scr, EINA_FALSE);

       if(  (!it->wd->ed->reorder_rel) || (!it->wd->ed->ec->move) ||
          (!it->wd->ed->ec->move(it->wd->obj, it->wd->ed->reorder_item, it->wd->ed->reorder_rel, EINA_TRUE ) ) )
        {
          evas_object_move(it->wd->ed->reorder_item->base, it->wd->ed->reorder_item->scrl_x+it->pad_left, it->wd->ed->reorder_item->scrl_y);
          _move_edit_controls( it,it->wd->ed->reorder_item->scrl_x, it->wd->ed->reorder_item->scrl_y );
       }
        it->wd->ed->reorder_item = NULL;
       it->wd->ed->reorder_rel = NULL;
       return;
     }
}

static void
_reorder_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
  Elm_Genlist_Item *it = data;
  Evas_Event_Mouse_Move *ev = event_info;

   if ((it->dragging) && (it->down))
     {
        if (it->edit_long_timer)
          {
             ecore_timer_del(it->edit_long_timer);
             it->edit_long_timer = NULL;
          }
          
        evas_object_smart_callback_call(it->wd->obj, "drag", it);
//      return;
     }


   Evas_Coord minw = 0, minh = 0, x, y, dx, dy, adx, ady;

     elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   x = ev->cur.canvas.x - x;
   y = ev->cur.canvas.y - y;
   dx = x - it->dx;
   adx = dx;
   if (adx < 0) adx = -dx;
   dy = y - it->dy;
   ady = dy;
   if (ady < 0) ady = -dy;
   minw /= 2;
   minh /= 2;
   if ((adx > minw) || (ady > minh))
     {
        it->dragging = 1;
        if (it->edit_long_timer)
          {
             ecore_timer_del(it->edit_long_timer);
             it->edit_long_timer = NULL;
          }
   }

  if( it->reordering && it->wd->ed->reorder_item )
    {
       int y = ev->cur.canvas.y - it->wd->ed->reorder_item->dy;
       evas_object_raise(it->wd->ed->reorder_item->base);
       evas_object_move(it->wd->ed->reorder_item->base, it->wd->ed->reorder_item->scrl_x+it->pad_left, y);
       evas_object_show(it->wd->ed->reorder_item->base);
       _move_edit_controls( it,it->wd->ed->reorder_item->scrl_x, y );

       it->block->updateme = EINA_TRUE;

       if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
       it->wd->calc_job = ecore_job_add(_calc_job, it->wd);

       return;
    }
}

static void
_select_all_down(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Item_Block *itb;
   Elm_Genlist_Item *select_all_it = data, *it;
   Eina_List *l;
   Widget_Data *wd = select_all_it->wd;
   if (!wd) return;

	if(!wd->selct_all) 
		edje_object_signal_emit(select_all_it->base, "elm,state,del_confirm", "elm");
	else
		 edje_object_signal_emit(select_all_it->base, "elm,state,del,animated,enable", "elm");
		

   EINA_INLIST_FOREACH(wd->blocks, itb)
     {

	    EINA_LIST_FOREACH(itb->items, l, it)
	     {

		if(!wd->selct_all) 
		  {
		     it->delete_check = 1;
		     it->del_confirm_state = 1;
		     edje_object_signal_emit(it->edit_obj, "elm,state,del_confirm", "elm");
		  }
		else 
		  {
		     it->delete_check = 0;
		     it->del_confirm_state = 0;
		     edje_object_signal_emit(it->edit_obj, "elm,state,del,animated,enable", "elm");
		  }
	     }
   }

   wd->selct_all ^= 0xFF;

  if (wd->ed->ec->item_selected)
  {
	  wd->ed->ec->item_selected(select_all_it->data, select_all_it, wd->selct_all);
  }

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);	
}

static void
_move_edit_controls( Elm_Genlist_Item *it, int itx, int ity )
{
   if(it->wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE )
     return;

   evas_object_resize(it->edit_obj,it->w, it->h );
   evas_object_move(it->edit_obj, itx, ity );
   evas_object_raise( it->edit_obj );
}

static void
_edit_controls_eval(Elm_Genlist_Item *it)
{
	int itmode = 0;
	const char *pad_str;
	int pad = 0;
	it->pad_left = 0;
	it->pad_right = 0;
	Evas_Object *icon;
	char buf[1024];

	if (it->wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE && !it->edit_obj)
	  return;


	if (it->itc->func.editmode_get)
	  itmode = it->itc->func.editmode_get(it->data, it->wd->obj, it->wd->edit_mode);
	itmode &= it->wd->edit_mode;

	if (itmode & ELM_GENLIST_EDIT_MODE_SELECTALL)
	  itmode |= ELM_GENLIST_EDIT_MODE_SELECT;

	if (!it->edit_obj)
	  {
		  it->edit_obj = edje_object_add(evas_object_evas_get(it->wd->obj));
		  edje_object_scale_set(it->edit_obj, elm_widget_scale_get(it->wd->obj) *
				  _elm_config->scale);
		  evas_object_smart_member_add(it->edit_obj, it->wd->pan_smart);
		  elm_widget_sub_object_add(it->wd->obj, it->edit_obj);
		  if (it->flags & ELM_GENLIST_ITEM_SUBITEMS) strncpy(buf, "tree", sizeof(buf));
		  else strncpy(buf, "item", sizeof(buf));
		  if (it->wd->compress) strncat(buf, "_compress", sizeof(buf) - strlen(buf));
		  // if (in & 0x1) strncat(buf, "_odd", sizeof(buf) - strlen(buf));
		  strncat(buf, "/", sizeof(buf) - strlen(buf));

		  if (it->wd->ed->ec->item_style && strcmp(it->wd->ed->ec->item_style, "default")) 
			 {
				 strncat(buf, it->wd->ed->ec->item_style, sizeof(buf) - strlen(buf));
				 _elm_theme_object_set(it->wd->obj, it->edit_obj, "genlist", buf, elm_widget_style_get(it->wd->obj));
			 }
		  else
			 {
				 _elm_theme_object_set(it->wd->obj, it->edit_obj, "genlist", "item/edit_control", elm_widget_style_get(it->wd->obj));
			 }

		//  edje_object_signal_callback_add(it->edit_obj, "elm,action,edit,reset",
			//	  "elm", _edit_mode_reset, it);
	  }

	pad_str = edje_object_data_get(it->edit_obj, "icon_width");
	if (pad_str) pad = atoi(pad_str);

	if ((itmode & ELM_GENLIST_EDIT_MODE_INSERT))
	  {
		  if (it->wd->animate_edit_controls)
			 edje_object_signal_emit(it->edit_obj, "elm,state,ins,animated,enable", "elm");
		  else
			 edje_object_signal_emit(it->edit_obj, "elm,state,ins,enable", "elm");

		  edje_object_signal_callback_add(it->edit_obj, "elm,action,item,insert",
				  "elm", _insert_new_item_cb, it);
		  it->pad_left += pad * _elm_config->scale;
	  }
	else
	  {
		  if (it->wd->animate_edit_controls)
			 edje_object_signal_emit(it->edit_obj, "elm,state,ins,animated,disable", "elm");
		  else
			 edje_object_signal_emit(it->edit_obj, "elm,state,ins,disable", "elm");

		  edje_object_signal_callback_del(it->edit_obj, "elm,action,item,insert",
				  "elm", _insert_new_item_cb );
	  }

	if ((itmode & ELM_GENLIST_EDIT_MODE_DELETE) || (itmode & ELM_GENLIST_EDIT_MODE_SELECT))
	  {
		  if (it->wd->animate_edit_controls)
			 edje_object_signal_emit(it->edit_obj, "elm,state,del,animated,enable", "elm");
		  else
			 edje_object_signal_emit(it->edit_obj, "elm,state,del,enable", "elm");

		  edje_object_signal_callback_del(it->edit_obj, "elm,action,item,delete",
				  "elm", _remove_item_cb );
		  edje_object_signal_callback_del(it->edit_obj, "elm,action,hide,del_confirm",
				  "elm", _hide_delete_confirm_object );

		  edje_object_signal_callback_add(it->edit_obj, "elm,action,item,delete",
				  "elm", _remove_item_cb, it);

		  edje_object_signal_callback_add(it->edit_obj, "elm,action,hide,del_confirm",
				  "elm", _hide_delete_confirm_object, it );
		  it->pad_left += pad * _elm_config->scale;
		  evas_object_event_callback_add(it->edit_obj, EVAS_CALLBACK_MOUSE_DOWN,
				  _reorder_mouse_down, it);
	  }
	else
	  {
		  if (it->wd->animate_edit_controls)
			 edje_object_signal_emit(it->edit_obj, "elm,state,del,animated,disable", "elm");
		  else
			 edje_object_signal_emit(it->edit_obj, "elm,state,del,disable", "elm");

		  edje_object_signal_callback_del(it->edit_obj, "elm,action,item,delete",
				  "elm", _remove_item_cb );
		  edje_object_signal_callback_del(it->edit_obj, "elm,action,hide,del_confirm",
				  "elm", _hide_delete_confirm_object );
                  evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_DOWN,
				  _reorder_mouse_down);
	  }

	if ((itmode & ELM_GENLIST_EDIT_MODE_REORDER))
	  {
		  const char* reorder_part;

		  reorder_part = edje_object_data_get(it->edit_obj, "reorder");
		  if (reorder_part && edje_object_part_exists(it->edit_obj, reorder_part))
			 {
				 edje_object_part_object_get(it->edit_obj, reorder_part);

				 evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_DOWN,
						 _reorder_mouse_down);
				 evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_UP,
						 _reorder_mouse_up);
				 evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_MOVE,
						 _reorder_mouse_move);

				 evas_object_event_callback_add(it->edit_obj, EVAS_CALLBACK_MOUSE_DOWN,
						 _reorder_mouse_down, it);
				 evas_object_event_callback_add(it->edit_obj, EVAS_CALLBACK_MOUSE_UP,
						 _reorder_mouse_up, it);
				 evas_object_event_callback_add(it->edit_obj, EVAS_CALLBACK_MOUSE_MOVE,
						 _reorder_mouse_move, it);
			 }
		  //  it->pad_right += pad * _elm_config->scale;
	  }
	else
	  {
		  const char* reorder_part;

		  reorder_part = edje_object_data_get(it->edit_obj, "reorder");
		  if (reorder_part && edje_object_part_exists(it->edit_obj, reorder_part))
			 {

				 if (itmode == ELM_GENLIST_EDIT_MODE_NONE) 
					{
						evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_DOWN,
								_reorder_mouse_down);
						evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_UP,
								_reorder_mouse_up);
						evas_object_event_callback_del(it->edit_obj, EVAS_CALLBACK_MOUSE_MOVE,
								_reorder_mouse_move);
					}
			 }
	  }

	if (it->wd->edit_mode != 0xF0) {
		  if (it->wd->edit_mode != ELM_GENLIST_EDIT_MODE_NONE) 
			 {
				 if (it->itc->func.icon_get)
					{
						edje_object_signal_emit(it->edit_obj, "elm,state,edit_end,enable", "elm");

						const Eina_List *l;
						const char *key;

						it->icons = _elm_stringlist_get(edje_object_data_get(it->edit_obj, "icons"));
						EINA_LIST_FOREACH(it->icons, l, key)
						  {
							  Evas_Object *ic = it->itc->func.icon_get(it->data, it->wd->obj, l->data);

							  if (ic)
								 {
									 it->edit_icon_objs = eina_list_append(it->edit_icon_objs, ic);
									 edje_object_part_swallow(it->edit_obj, key, ic);
									 evas_object_show(ic);
									 //  elm_widget_sub_object_add(it->wd->obj, ic);
								 }
						  }
					}		
			 }
		  else 
			 {
				 edje_object_signal_emit(it->edit_obj, "elm,state,edit_end,disable", "elm");
				 EINA_LIST_FREE(it->edit_icon_objs, icon)
					 evas_object_del(icon);

				 Evas_Object *editfield;
				 EINA_LIST_FREE(it->wd->edit_field, editfield)
					 evas_object_del(editfield);
			 }
	}

	if (it->wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE)//Unrealize
	  {
		  evas_object_del(it->edit_obj);
		  it->edit_obj = NULL;
		  return;
	  }
	_move_edit_controls(it,it->scrl_x, it->scrl_y );
	evas_object_show( it->edit_obj );
}

static void
_notify_item_position( Elm_Genlist_Item *it )
{
   const Eina_List *l;
   if( it->parent )
     {
	l = eina_list_last(it->parent->items);

	//Check if the Item is First Node or Last node of its Parent & raise signal.
	if( it->parent->items->data != it &&  l->data != it )
	  {
	     edje_object_signal_emit(it->base, "normal_item", "elm");
	  } 
	else 
	  {
	     if(it->parent->items->data == it )
	       edje_object_signal_emit(it->base, "first_item", "elm");

	     if(l->data == it )
	       edje_object_signal_emit(it->base, "last_item", "elm");
	  }
     }
}


static int
_get_space_for_reorder_item( Elm_Genlist_Item *it )
{
  int top=0;
  Evas_Coord rox, roy, row, roh;
  if(!it->wd->ed)
   return 0;

  if( !(it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_REORDER ) || !it->wd->ed->reorder_item )
    return 0;

  evas_object_geometry_get(it->wd->ed->reorder_item->base, &rox, &roy, &row, &roh);

  top = (ELM_RECTS_INTERSECT(it->scrl_x, it->scrl_y, it->w, it->h,
                             rox, roy+roh/2, row, 1));

  if( top )
    {
       it->wd->ed->reorder_rel = it;
       it->scrl_y+=it->wd->ed->reorder_item->h;
       return it->wd->ed->reorder_item->h;
    }

  return 0;
}

static void
_item_block_position(Item_Block *itb, int in)
{
	const Eina_List *l;
	Elm_Genlist_Item *it, *select_all_item;
	Elm_Genlist_GroupItem *git = NULL;
	Evas_Coord y = 0, ox, oy, ow, oh, cvx, cvy, cvw, cvh;
	int vis=0;
	int is_reorder = 0;

	evas_object_geometry_get(itb->wd->pan_smart, &ox, &oy, &ow, &oh);
	evas_output_viewport_get(evas_object_evas_get(itb->wd->obj), &cvx, &cvy, &cvw, &cvh);

	if (itb->wd->select_all_item && 
			(itb->wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECT || itb->wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECTALL) ) 
	  {

		  select_all_item = itb->wd->select_all_item;

		  evas_object_resize(select_all_item->base, itb->w, select_all_item->h);  
		  evas_object_move(select_all_item->base, ox, oy);
		  evas_object_raise(select_all_item->base);

		  if ( itb->wd->move_effect_mode != ELM_GENLIST_ITEM_MOVE_EFFECT_EDIT_MODE )         
			 evas_object_show(select_all_item->base);
		  else
			 evas_object_hide(select_all_item->base);
		  y = select_all_item->h;
	  }

	EINA_LIST_FOREACH(itb->items, l, it)
	  {
		  if (it->delete_me) continue;
		  it->x = 0;
		  it->y = y;
		  it->w = itb->w;
		  vis = (ELM_RECTS_INTERSECT(itb->x - it->wd->pan_x + ox,
					  itb->y - it->wd->pan_y + oy,
					  itb->w, itb->h,
					  cvx, cvy, cvw, cvh));
		  if ((itb->realized) && (!it->realized))
			 {
				 if (vis)
					{
						Eina_Bool was_realized = it->realized;

						_item_realize(it, in, 0);
						if (!was_realized)
						  evas_object_smart_callback_call(it->wd->obj,
								  "realized", it);
					}
			 }
		  if (it->realized)
			 {
				 _notify_item_position( it );
				 if (vis)
					{
						it->scrl_x = ox + itb->x + it->x - itb->wd->pan_x;
						it->scrl_y = oy + itb->y + it->y - itb->wd->pan_y + itb->reoder_y;;
						if( git != it->group_item )
						  {
							  git = it->group_item;
							  if (git)
								 {
									 git->visible = EINA_TRUE; //Mark Group Item to make it visible
									 if (git->items->data == it)
										git->y = it->scrl_y;
									 if (GROUP_ALIGN_NORTH == git->align)
										{
											git->w = itb->w;
											if (git->items->data == it)
											  {
												  it->scrl_y += git->minh;
												  y += git->minh;
											  }
										}
								 }
						  }
						if (git)
						  {
							  git->x = ox + itb->x - itb->wd->pan_x;

							  if (git->y < oy)
								 git->y = oy;

							  if (git->align == GROUP_ALIGN_WEST)
								 {
									 it->w -= git->w;
									 it->scrl_x += git->x + git->w;
									 git->h = (it->scrl_y + it->h)  -  git->y ;
									 if( git->h < it->h )
										{
											git->y = it->scrl_y;
											git->h = it->h;
										}
								 }
							  if (git->align == GROUP_ALIGN_NORTH)
								 {
									 git->h = git->minh;
									 if ((git->y + git->h) > (it->scrl_y + it->h))
										git->y = (it->scrl_y + it->h) - git->minh;
								 }
							  if(git->update_finish_y) 
								 {
									 git->finish_y += oy;
									 git->update_finish_y = EINA_FALSE;
								 }

						  }

						is_reorder = _get_space_for_reorder_item( it );

						if (is_reorder)
						  it->reorder_check = 1;
						else
						  it->reorder_check = 0;

						if (it->wd->ed)
						  {
							  if (it != it->wd->ed->reorder_item && is_reorder && in > 0 && !(in % it->wd->max_items_per_block) && !itb->reoder_y) 
								 {
									 itb->reoder_y -= it->h;
									 it->scrl_y = oy + itb->y + it->y - itb->wd->pan_y + itb->reoder_y;
								 }
							  else if (it != it->wd->ed->reorder_item && is_reorder && in > 0 && !(in % it->wd->max_items_per_block) && itb->reoder_y) 
								 {
									 itb->reoder_y = 0;
								 }
						  }
						y += is_reorder;

						if (!it->reordering )
						  {
							  if ((!it->wd->effect_mode || 
                        (it->wd->effect_mode && it->wd->move_effect_mode == ELM_GENLIST_ITEM_MOVE_EFFECT_NONE)) && !it->wd->pinch_zoom_reserve)
								 {
									 _move_edit_controls( it,it->scrl_x, it->scrl_y );
									 evas_object_resize(it->base, it->w-(it->pad_left+it->pad_right), it->h);

									 evas_object_move(it->base, it->scrl_x+it->pad_left, it->scrl_y);

									 if (it->delete_check)
										{
											edje_object_signal_emit(it->edit_obj, "elm,state,del_confirm", "elm");
											edje_object_signal_emit(it->base, "elm,state,del_confirm", "elm");
										}
									 evas_object_show(it->base);
									 it->old_pad_left = it->pad_left;
									 it->old_scrl_y = it->scrl_y;
								 }

						  }
					}
				 else
					{
						if (!it->dragging)
						  _item_unrealize(it);
					}
			 }
		  if (!it->reordering )
			 y += it->h;

		  in++;
	  }

	if (itb->wd->select_all_item && 
      	(itb->wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECT || itb->wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECTALL) ) 
	  evas_object_raise(select_all_item->base);

	if (vis)
	  {
		  itb->wd->animate_edit_controls = 0;
		  if (git)
			 git->visible = EINA_TRUE;
	  }
}

static void
_calc_job(void *data)
{
   Widget_Data *wd = data;
   Item_Block *itb;
   Evas_Coord minw = -1, minh = 0, y = 0, ow, oh;
   Item_Block *chb = NULL;
   int in = 0, minw_change = 0;
	if (!wd) return;
	EINA_INLIST_FOREACH(wd->blocks, itb)
	  {
		  int showme = 0;

		  itb->num = in;
		  showme = itb->showme;
		  itb->showme = 0;
		  if (chb)
			 {
				 if (itb->realized) _item_block_unrealize(itb);
			 }
		  if (itb->changed)
			 {
				 if (itb->realized) _item_block_unrealize(itb);
				 showme = _item_block_recalc(itb, in, 0, 1);
				 chb = itb;
			 }
		  itb->y = y;
		  itb->x = 0;
		  minh += itb->minh;
		  if (minw == -1) minw = itb->minw;
		  else if (minw < itb->minw)
			 {
				 minw = itb->minw;
				 minw_change = 1;
			 }
		  itb->w = minw;
		  itb->h = itb->minh;
		  y += itb->h;
		  in += itb->count;
		  if (showme)
			 {
				 if(wd->show_item) 
					{
						wd->show_item->showme = 0;
						if (wd->bring_in)
						  elm_smart_scroller_region_bring_in(wd->scr,
								  wd->show_item->x + wd->show_item->block->x,
								  wd->show_item->y + wd->show_item->block->y,
								  wd->show_item->block->w,
								  wd->show_item->h);
						else
						  elm_smart_scroller_child_region_show(wd->scr,
								  wd->show_item->x + wd->show_item->block->x,
								  wd->show_item->y + wd->show_item->block->y,
								  wd->show_item->block->w,
								  wd->show_item->h);
						wd->show_item = NULL;
					}
			 }
	  }

	if (minw_change)
	  {
		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 {
				 itb->minw = minw;
				 itb->w = itb->minw;
			 }
	  }
	if ((chb) && (EINA_INLIST_GET(chb)->next))
	  {
		  EINA_INLIST_FOREACH(EINA_INLIST_GET(chb)->next, itb)
			  if (itb->realized) _item_block_unrealize(itb);
	  }
	evas_object_geometry_get(wd->pan_smart, NULL, NULL, &ow, &oh);
	if (minw < ow) minw = ow;
	if ((minw != wd->minw) || (minh != wd->minh) || wd->select_all_item)
	  {
		  wd->minw = minw;
		  wd->minh = minh;
		  if(wd->select_all_item)
				 wd->minh += wd->select_all_item->h;
		  evas_object_smart_callback_call(wd->pan_smart, "changed", NULL);
		  _sizing_eval(wd->obj);
	  }
	wd->calc_job = NULL;
	evas_object_smart_changed(wd->pan_smart);

}

static void
_update_job(void *data)
{
   Widget_Data *wd = data;
   Eina_List *l2;
   Item_Block *itb;
   int num, num0, position = 0, recalc = 0;
   if (!wd) return;
   wd->update_job = NULL;
   num = 0;
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
        Evas_Coord itminw, itminh;
        Elm_Genlist_Item *it;

        if (!itb->updateme)
          {
             num += itb->count;
             if (position)
               _item_block_position(itb, num);
             continue;
          }
        num0 = num;
        recalc = 0;
        EINA_LIST_FOREACH(itb->items, l2, it)
          {
             if (it->updateme)
               {
                  itminw = it->w;
                  itminh = it->h;

                  it->updateme = 0;
                  if (it->realized)
                    {
                       _item_unrealize(it);
                       _item_realize(it, num, 0);
                       evas_object_smart_callback_call(it->wd->obj,
                                                       "realized", it);
                    }
                  else
                    {
                       _item_realize(it, num, 1);
                       _item_unrealize(it);
                    }
                  if ((it->minw != itminw) || (it->minh != itminh))
                    recalc = 1;
               }
             num++;
          }
        itb->updateme = 0;
        if (recalc)
          {
             position = 1;
             itb->changed = EINA_TRUE;
             _item_block_recalc(itb, num0, 0, 1);
             _item_block_position(itb, num0);
          }
     }
   if (position)
     {
	if (wd->calc_job) ecore_job_del(wd->calc_job);
	wd->calc_job = ecore_job_add(_calc_job, wd);
     }
}

static void
_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Pan *sd = evas_object_smart_data_get(obj);
//   Evas_Coord ow, oh;
//   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
//   ow = sd->wd->minw - ow;
//   if (ow < 0) ow = 0;
//   oh = sd->wd->minh - oh;
//   if (oh < 0) oh = 0;
//   if (x < 0) x = 0;
//   if (y < 0) y = 0;
//   if (x > ow) x = ow;
//   if (y > oh) y = oh;
   if ((x == sd->wd->pan_x) && (y == sd->wd->pan_y)) return;
   sd->wd->pan_x = x;
   sd->wd->pan_y = y;
   evas_object_smart_changed(obj);
}

static void
_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);

   if (x) *x = sd->wd->pan_x;
   if (y) *y = sd->wd->pan_y;

   if( sd->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH ) 
   {
       int git_cnt = 0, git_h = 0;
       Elm_Genlist_GroupItem *git;
    
       EINA_INLIST_FOREACH(sd->wd->group_items, git)
       {
            git_cnt++;
            git_h = git->h;
       }

	   if(  sd->wd->minh != (git_h+1) * git_cnt) 
	   {
		   sd->wd->minh = (git_h+1) * git_cnt;
	   }
   }

}

static void
_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ow = sd->wd->minw - ow;
   if (ow < 0) ow = 0;
   oh = sd->wd->minh - oh;
   if (oh < 0) oh = 0;
   if (x) *x = ow;
   if (y) *y = oh;
}

static void
_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Pan *sd = evas_object_smart_data_get(obj);

   if (w) *w = sd->wd->minw;
   if (h) *h = sd->wd->minh;
}

static void
_pan_add(Evas_Object *obj)
{
   Pan *sd;
   Evas_Object_Smart_Clipped_Data *cd;

   _pan_sc.add(obj);
   cd = evas_object_smart_data_get(obj);
   sd = ELM_NEW(Pan);
   if (!sd) return;
   sd->__clipped_data = *cd;
   free(cd);
   evas_object_smart_data_set(obj, sd);
}

static void
_pan_del(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   _pan_sc.del(obj);
}

static void
_pan_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
   sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_pan_calculate(Evas_Object *obj)
{
	Pan *sd = evas_object_smart_data_get(obj);
	Item_Block *itb;
	Elm_Genlist_GroupItem *git;
	Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;
	int in = 0;

	evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
	if (sd->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_EXPAND) return;


	if( sd->wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH )
	  {
		  sd->wd->contract_pan_y = sd->wd->pan_y;      
				EINA_INLIST_FOREACH(sd->wd->group_items, git)
				  {
					  git->visible = EINA_TRUE;
					  evas_object_raise(git->base);
					  evas_object_resize(git->base, sd->wd->minw, git->h);
					  evas_object_move(git->base, git->x, git->y + sd->wd->pan_y * -1);
					  evas_object_show(git->base);
				  }
	  }

	if (sd->wd->effect_mode && sd->wd->pinchzoom_effect_mode != ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE) return;

	if (sd->wd->edit_mode != ELM_GENLIST_EDIT_MODE_NONE)
	  (void)_edit_mode_reset(sd->wd);
	EINA_INLIST_FOREACH(sd->wd->group_items, git)
	  {
		  git->visible = EINA_FALSE;
	  }

	evas_output_viewport_get(evas_object_evas_get(obj), &cvx, &cvy, &cvw, &cvh);
	EINA_INLIST_FOREACH(sd->wd->blocks, itb)
	  {
		  itb->w = sd->wd->minw;
		  if (ELM_RECTS_INTERSECT(itb->x - sd->wd->pan_x + ox,
					  itb->y - sd->wd->pan_y + oy,
					  itb->w, itb->h,
					  cvx, cvy, cvw, cvh))
			 {
				 if ((!itb->realized) || (itb->changed))
					_item_block_realize(itb, in, 0);
				 _item_block_position(itb,  in);
			 }
		  else
			 {
				 if (itb->realized) _item_block_unrealize(itb);
			 }
		  in += itb->count;
	  }

	if (sd->wd->effect_mode && 
			(sd->wd->move_effect_mode == ELM_GENLIST_ITEM_MOVE_EFFECT_EXPAND || sd->wd->move_effect_mode == ELM_GENLIST_ITEM_MOVE_EFFECT_CONTRACT))
	  {
		  _item_flip_effect_show(sd->wd);
		  sd->wd->item_moving_effect_timer = ecore_animator_add(_item_moving_effect_timer_cb, sd->wd);
	  }

	if (sd->wd->effect_mode && sd->wd->edit_mode_effect_mode) 
	  sd->wd->item_moving_effect_timer = ecore_animator_add(_edit_mode_item_moving_effect_cb, sd->wd);

	EINA_INLIST_FOREACH(sd->wd->group_items, git)
	  {
		  if (git->visible)
			 {
				 evas_object_raise(git->base);
				 evas_object_resize(git->base, git->w, git->h - 1);
				 evas_object_move(git->base, git->x, git->y);
				 if(!sd->wd->pinch_zoom_reserve)
					evas_object_show(git->base);
				 else 
					evas_object_hide(git->base);
			 }
		  else
			 evas_object_hide(git->base);
	  }
	if ((sd->wd->edit_mode & ELM_GENLIST_EDIT_MODE_REORDER) && (sd->wd->ed->reorder_item))
	  {
		  evas_object_raise(sd->wd->ed->reorder_item->base);
		  evas_object_raise(sd->wd->ed->reorder_item->edit_obj);
	  }
	if(sd->wd->select_all_item) 
	  evas_object_raise(sd->wd->select_all_item->base);
}

static void
_pan_move(Evas_Object *obj, Evas_Coord x __UNUSED__, Evas_Coord y __UNUSED__)
{
	Pan *sd = evas_object_smart_data_get(obj);

	if (sd->wd->effect_mode && sd->wd->multi_down) return;
	if (sd->wd->effect_mode && sd->wd->pinchzoom_effect_mode != ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE) return;

	if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
	sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_hold_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 1);
}

static void
_hold_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 0);
}

static void
_freeze_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 1);
}

static void
_freeze_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 0);
}

/**
 * Add a new Genlist object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Genlist
 */
EAPI Evas_Object *
elm_genlist_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_Coord minw, minh;
   static Evas_Smart *smart = NULL;

	if (!parent) return NULL;
   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "genlist");
   elm_widget_type_set(obj, "genlist");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "genlist", "base", elm_widget_style_get(obj));
   elm_widget_resize_object_set(obj, wd->scr);

	elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, EINA_TRUE);

	wd->obj = obj;
	wd->mode = ELM_LIST_SCROLL;
	wd->max_items_per_block = 32;
	wd->longpress_timeout = 1.0;
	wd->max_git_num = 0;

	evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);

   if (!smart)
     {
	static Evas_Smart_Class sc;

	evas_object_smart_clipped_smart_set(&_pan_sc);
	sc = _pan_sc;
	sc.name = "elm_genlist_pan";
	sc.version = EVAS_SMART_CLASS_VERSION;
	sc.add = _pan_add;
	sc.del = _pan_del;
	sc.resize = _pan_resize;
	sc.move = _pan_move;
	sc.calculate = _pan_calculate;
	smart = evas_smart_class_new(&sc);
     }
   if (smart)
     {
	wd->pan_smart = evas_object_smart_add(e, smart);
	wd->pan = evas_object_smart_data_get(wd->pan_smart);
	wd->pan->wd = wd;
     }

   elm_smart_scroller_extern_pan_set(wd->scr, wd->pan_smart,
				     _pan_set, _pan_get,
				     _pan_max_get, _pan_child_size_get);

   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   _sizing_eval(obj);
   return obj;
}

static Elm_Genlist_Item *
_item_new(Widget_Data *wd, const Elm_Genlist_Item_Class *itc,
	  const void *data, Elm_Genlist_Item *parent,
	  Elm_Genlist_Item_Flags flags,
	  Evas_Smart_Cb func,
          const void *func_data)
{
   Elm_Genlist_Item *it;

   it = calloc(1, sizeof(Elm_Genlist_Item));
   if (!it) return NULL;
   it->wd = wd;
   it->itc = itc;
   it->data = data;
   it->parent = parent;
   it->flags = flags;
   it->func.func = func;
   it->func.data = func_data;
   it->expanded_depth = 0;
   return it;
}

static void
_item_block_add(Widget_Data *wd, Elm_Genlist_Item *it)
{
   Item_Block *itb = NULL;

   if (!it->rel)
     {
	newblock:
	if (it->rel)
	  {
	     itb = calloc(1, sizeof(Item_Block));
	     if (!itb) return;
	     itb->wd = wd;
	     if (!it->rel->block)
	       {
                  it->rel->block = itb;
		  wd->blocks =
                    eina_inlist_append(wd->blocks, EINA_INLIST_GET(itb));
		  itb->items = eina_list_append(itb->items, it);
	       }
	     else
	       {
		  if (it->before)
		    {
		       wd->blocks = 
                         eina_inlist_prepend_relative(wd->blocks, 
                                                      EINA_INLIST_GET(itb), 
                                                      EINA_INLIST_GET(it->rel->block));
		       itb->items = 
                         eina_list_prepend_relative(itb->items, it, it->rel);
		    }
		  else
		    {
		       wd->blocks = 
                         eina_inlist_append_relative(wd->blocks, 
                                                     EINA_INLIST_GET(itb), 
                                                     EINA_INLIST_GET(it->rel->block));
		       itb->items = 
                         eina_list_append_relative(itb->items, it, it->rel);
		    }
	       }
	  }
	else
	  {
	     if (it->before)
	       {
		  if (wd->blocks)
		    {
		       itb = (Item_Block *)(wd->blocks);
		       if (itb->count >= wd->max_items_per_block)
			 {
			    itb = calloc(1, sizeof(Item_Block));
			    if (!itb) return;
			    itb->wd = wd;
			    wd->blocks = 
                              eina_inlist_prepend(wd->blocks, 
                                                  EINA_INLIST_GET(itb));
			 }
		    }
		  else
		    {
		       itb = calloc(1, sizeof(Item_Block));
		       if (!itb) return;
		       itb->wd = wd;
		       wd->blocks = 
                         eina_inlist_prepend(wd->blocks, EINA_INLIST_GET(itb));
		    }
		  itb->items = eina_list_prepend(itb->items, it);
	       }
	     else
	       {
		  if (wd->blocks)
		    {
		       itb = (Item_Block *)(wd->blocks->last);
		       if (itb->count >= wd->max_items_per_block)
			 {
			    itb = calloc(1, sizeof(Item_Block));
			    if (!itb) return;
			    itb->wd = wd;
			    wd->blocks = 
                              eina_inlist_append(wd->blocks, 
                                                 EINA_INLIST_GET(itb));
			 }
		    }
		  else
		    {
		       itb = calloc(1, sizeof(Item_Block));
		       if (!itb) return;
		       itb->wd = wd;
		       wd->blocks = 
                         eina_inlist_append(wd->blocks, EINA_INLIST_GET(itb));
		    }
		  itb->items = eina_list_append(itb->items, it);
	       }
	  }
     }
   else
     {
	itb = it->rel->block;
	if (!itb) goto newblock;
	if (it->before)
	  itb->items = eina_list_prepend_relative(itb->items, it, it->rel);
	else
	  itb->items = eina_list_append_relative(itb->items, it, it->rel);
     }
   itb->count++;
   itb->changed = EINA_TRUE;
   it->block = itb;

   if(!itb->wd)
   	itb->wd = wd;

   if (itb->wd->calc_job) ecore_job_del(itb->wd->calc_job);
   itb->wd->calc_job = ecore_job_add(_calc_job, itb->wd);
   if (it->rel)
     {
	it->rel->relcount--;
	if ((it->rel->delete_me) && (!it->rel->relcount))
	  _item_del(it->rel);
	it->rel = NULL;
     }
   if (itb->count > itb->wd->max_items_per_block)
     {
        int newc;
        Item_Block *itb2;
        Elm_Genlist_Item *it2;

        newc = itb->count / 2;
        itb2 = calloc(1, sizeof(Item_Block));
        if (!itb2) return;
        itb2->wd = wd;
        wd->blocks = 
          eina_inlist_append_relative(wd->blocks, EINA_INLIST_GET(itb2), 
                                      EINA_INLIST_GET(itb));
        itb2->changed = EINA_TRUE;
        while ((itb->count > newc) && (itb->items))
          {
             Eina_List *l;

             l = eina_list_last(itb->items);
             it2 = l->data;
             itb->items = eina_list_remove_list(itb->items, l);
             itb->count--;

             itb2->items = eina_list_prepend(itb2->items, it2);
             it2->block = itb2;
             itb2->count++;
          }
     }
}

static int
_queue_proecess(Widget_Data *wd, int norender)
{
   int n, showme = 0;
   double t0, t;

   t0 = ecore_time_get();
   for (n = 0; (wd->queue) && (n < 128); n++)
     {
	Elm_Genlist_Item *it;

	it = wd->queue->data;
	wd->queue = eina_list_remove_list(wd->queue, wd->queue);
	it->queued = EINA_FALSE;
	_item_block_add(wd, it);
        t = ecore_time_get();
        if (it->block->changed)
          {
             showme = _item_block_recalc(it->block, it->block->num, 1, norender);
             it->block->changed = 0;
          }
        if (showme) it->block->showme = 1;
        if (eina_inlist_count(wd->blocks) > 1)
          {
             if ((t - t0) > (ecore_animator_frametime_get())) break;
          }
     }
   return n;
}

static Eina_Bool
_item_idler(void *data)
{
	Widget_Data *wd = data;

	if (_queue_proecess(wd, 1) > 0)
	  {
		  if (wd->calc_job) ecore_job_del(wd->calc_job);
		  wd->calc_job = ecore_job_add(_calc_job, wd);
	  }
	if (!wd->queue)
	  {
		  wd->queue_idler = NULL;
		  if(wd->pinch_zoom_reserve)
			 _elm_genlist_pinch_zoom_execute(wd->obj, 1);    
		  return ECORE_CALLBACK_CANCEL;
	  }
	return ECORE_CALLBACK_RENEW;
}

static void
_item_queue(Widget_Data *wd, Elm_Genlist_Item *it)
{
	if(!wd->queue_exception)
  {
   if (it->queued) return;
   wd->queue = eina_list_append(wd->queue, it);
   it->queued = EINA_TRUE;
   wd->item_count++;
  }
  
  while ((wd->queue) && ((!wd->blocks) || (!wd->blocks->next)))
     {
        if (wd->queue_idler)
          {
             ecore_idler_del(wd->queue_idler);
             wd->queue_idler = NULL;
          }
        _queue_proecess(wd, 0);
     }

   if (!wd->queue_idler) wd->queue_idler = ecore_idler_add(_item_idler, wd);

	if(wd->queue_exception)
   {
   wd->queue = eina_list_append(wd->queue, it);
	it->queued = EINA_TRUE;
   }
}

/**
 * Add Group Item to the genlist
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The group item data
 */
EAPI Elm_Genlist_GroupItem *
elm_genlist_groupitem_add(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
			const void *data)
{
	Elm_Genlist_GroupItem *git;
	Widget_Data *wd = elm_widget_data_get(obj);

	git = calloc(1, sizeof(Elm_Genlist_GroupItem));
	if (!git) return NULL;
	git->wd = wd;
	git->itc = itc;
	git->data = data;

	wd->group_items = eina_inlist_append(wd->group_items, EINA_INLIST_GET(git) );
	return git;
}

/**
 * Delete a given groupitem
 *
 * This deletes the group item from genlist and calls the genlist group item del class
 * callback defined in the item class, if it is set.
 *
 * @param git The group item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_groupitem_del(Elm_Genlist_GroupItem *git)
{
   _groupitem_remove( git, EINA_TRUE);
}

/**
 * Append item to the end of the genlist
 *
 * This appends the given item to the end of the list or the end of the
 * children if the parent is given.
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The item data
 * @param parent The parent item, or NULL if none
 * @param flags Item flags
 * @param func Convenience function called when item selected
 * @param func_data Data passed to @p func above.
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_append(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
                        const void *data, Elm_Genlist_Item *parent,
                        Elm_Genlist_Item_Flags flags,
                        Evas_Smart_Cb func, const void *func_data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Genlist_Item *it = _item_new(wd, itc, data, parent, flags, func, func_data);
   if (!wd) return NULL;
   if (!it) return NULL;
   if (!it->parent)
     {
        wd->items = eina_inlist_append(wd->items, EINA_INLIST_GET(it));
        it->rel = NULL;
        it->before = 0;
     }
   else
     {
        Elm_Genlist_Item *it2 = NULL;
        Eina_List *ll = eina_list_last(it->parent->items);
        if (ll) it2 = ll->data;
        it->parent->items = eina_list_append(it->parent->items, it);
        if (!it2) it2 = it->parent;
        wd->items = 
           eina_inlist_append_relative(wd->items, EINA_INLIST_GET(it), 
                                       EINA_INLIST_GET(it2));
        it->rel = it2;
        it->rel->relcount++;
        it->before = 0;
     }
   _item_queue(wd, it);
   return it;
}

/**
 * Append item to the end of the genlist with Group Item
 *
 * This appends the given item to the end of the list or the end of the
 * children if the parent is given.
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The item data
 * @param parent The parent item, or NULL if none
 * @param flags Item flags
 * @param git Group Item
 * @param func Convenience function called when item selected
 * @param func_data Data passed to @p func above.
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_append_with_group(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
			const void *data, Elm_Genlist_Item *parent,
			Elm_Genlist_Item_Flags flags, Elm_Genlist_GroupItem *git,
			Evas_Smart_Cb func, const void *func_data)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	Widget_Data *wd = elm_widget_data_get(obj);
	parent = NULL;
	Elm_Genlist_Item *it = _item_new(wd, itc, data, parent, flags, func, func_data);
   Elm_Genlist_GroupItem *pgit = NULL;
   Elm_Genlist_Item *it2 = NULL;
   Eina_List *ll = NULL;
	if (!wd) return NULL;
	if (!it) return NULL;
	if (!git) return NULL;

   pgit = git;
   while (pgit)
     {
        ll = eina_list_last(pgit->items);
        if (ll) 
          {
             it2 = ll->data;
             break;
          }
        if (!(EINA_INLIST_GET(pgit)->prev)) break;
        pgit = (Elm_Genlist_GroupItem *)(EINA_INLIST_GET(pgit)->prev);
     }
   if (it2)
     {
        wd->items =
           eina_inlist_append_relative(wd->items, EINA_INLIST_GET(it),
                                       EINA_INLIST_GET(it2));
        it->rel = it2;
        it->rel->relcount++;
     } 
   else 
     {
        wd->items = eina_inlist_append(wd->items, EINA_INLIST_GET(it));
        it->rel = NULL;
     }
   git->items = eina_list_append(git->items, it);
   it->before = 0;
   it->group_item = git;
   _item_queue(wd, it);
   return it;
}

/**
 * Prepend item at start of the genlist
 *
 * This adds an item to the beginning of the list or beginning of the children
 * of the parent if given.
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The item data
 * @param parent The parent item, or NULL if none
 * @param flags Item flags
 * @param func Convenience function called when item selected
 * @param func_data Data passed to @p func above.
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_prepend(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
			 const void *data, Elm_Genlist_Item *parent,
			 Elm_Genlist_Item_Flags flags,
			 Evas_Smart_Cb func, const void *func_data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Genlist_Item *it = _item_new(wd, itc, data, parent, flags, func, func_data);
   if (!wd) return NULL;
   if (!it) return NULL;
   if (!it->parent)
     wd->items = eina_inlist_prepend(wd->items, EINA_INLIST_GET(it));
   else
     {
	printf("FIXME: 12 tree not handled yet\n");
     }
   it->rel = NULL;
   it->before = 1;
   _item_queue(wd, it);
   return it;
}

/**
 * Insert item before another in the genlist
 *
 * This inserts an item before another in the list. It will be in the same tree
 * level as the item it is inseted before.
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The item data
 * @param before The item to insert before
 * @param flags Item flags
 * @param func Convenience function called when item selected
 * @param func_data Data passed to @p func above.
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_insert_before(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
			       const void *data, Elm_Genlist_Item *before,
			       Elm_Genlist_Item_Flags flags,
			       Evas_Smart_Cb func, const void *func_data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(before, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Genlist_Item *it = _item_new(wd, itc, data, NULL, flags, func, func_data);
   if (!wd) return NULL;
   if (!it) return NULL;
   if (!it->parent)
     wd->items = eina_inlist_prepend_relative(wd->items, EINA_INLIST_GET(it), 
                                              EINA_INLIST_GET(before));
   else
     {
	printf("FIXME: 13 tree not handled yet\n");
     }
   it->rel = before;
   it->rel->relcount++;
   it->before = 1;
   _item_queue(wd, it);
   return it;
}

/**
 * Insert and item after another in the genlst
 *
 * This inserts an item after another in the list. It will be in the same tree
 * level as the item it is inseted after.
 *
 * @param obj The genlist object
 * @param itc The item class for the item
 * @param data The item data
 * @param after The item to insert after
 * @param flags Item flags
 * @param func Convenience function called when item selected
 * @param func_data Data passed to @p func above.
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_insert_after(Evas_Object *obj, const Elm_Genlist_Item_Class *itc,
			      const void *data, Elm_Genlist_Item *after,
			      Elm_Genlist_Item_Flags flags,
			      Evas_Smart_Cb func, const void *func_data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(after, NULL);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Genlist_Item *it = _item_new(wd, itc, data, NULL, flags, func, func_data);
   if (!wd) return NULL;
   if (!it) return NULL;
   if (!it->parent)
     wd->items = eina_inlist_append_relative(wd->items, EINA_INLIST_GET(it), 
                                             EINA_INLIST_GET(after));
   else
     {
	printf("FIXME: 14 tree not handled yet\n");
     }
   it->rel = after;
   it->rel->relcount++;
   it->before = 0;
   _item_queue(wd, it);
   return it;
}

/**
 * Moves the Genlist Item
 */
EAPI void
elm_genlist_item_move_after(Elm_Genlist_Item *it, Elm_Genlist_Item *after )
{
  if (!it) return;

  Elm_Genlist_Item *next_item = elm_genlist_item_next_get(after);

  if(it->y == after->y  &&  after->reorder_check && it->reorder_check) {
  	if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
    it->wd->calc_job = ecore_job_add(_calc_job, it->wd);	
  	return;
  }

  it->wd->items = eina_inlist_remove(it->wd->items, EINA_INLIST_GET(it));
  _item_block_del(it);

  if((!next_item  && !after->reorder_check) || (next_item && !after->reorder_check) ) 
  {

	  if(next_item && !after->reorder_check  && it == after) {
	       it->wd->items = eina_inlist_append_relative(it->wd->items, EINA_INLIST_GET(it),
	                                 EINA_INLIST_GET(next_item));
		   it->rel = next_item;
	   }
	   else {
		   it->wd->items = eina_inlist_append_relative(it->wd->items, EINA_INLIST_GET(it),
	                                                 EINA_INLIST_GET(after));
		   it->rel = after;
	   }
	      
	   it->rel->relcount++;
	   it->before = 0;
	}
	else
	{
	  if( after)
	    {
	       it->wd->items = eina_inlist_prepend_relative(it->wd->items, EINA_INLIST_GET(it),
	                                                 EINA_INLIST_GET(after));
	       it->rel = after;
	       it->rel->relcount++;
	    }
	  else
	    {
	       it->wd->items = eina_inlist_prepend(it->wd->items, EINA_INLIST_GET(it));
	    }

	    it->before = 1;
	}
	
   _item_queue(it->wd, it);
}

/**
 * Clear the genlist
 *
 * This clears all items in the list, leaving it empty.
 *
 * @param obj The genlist object
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

	wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_NONE;
   wd->pinchzoom_effect_mode = ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE;
   elm_smart_scroller_hold_set(wd->scr, 0);
	elm_smart_scroller_freeze_set(wd->scr, 0);
   elm_smart_scroller_freeze_momentum_animator_set(wd->scr, 0);
   elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, EINA_TRUE);
   wd->max_git_num  = 0;
   wd->pinch_zoom_reserve = EINA_FALSE;

   if(wd->item_moving_effect_timer)
   {
  	//  ecore_timer_del(wd->item_moving_effect_timer);
	wd->item_moving_effect_timer = NULL;
   }

   while (wd->group_items)
     {
	_groupitem_remove((Elm_Genlist_GroupItem *)wd->group_items, EINA_FALSE);
     }

   if (wd->walking > 0)
     {
	Elm_Genlist_Item *it;
        
        wd->clear_me = 1;
        EINA_INLIST_FOREACH(wd->items, it)
          {
             it->delete_me = 1;
          }
	return;
     }
   wd->clear_me = 0;

   while (wd->items)
     {
	Elm_Genlist_Item *it = (Elm_Genlist_Item *)(wd->items);

	wd->items = eina_inlist_remove(wd->items, wd->items);
	if (it->realized) _item_unrealize(it);
	if (it->itc->func.del) it->itc->func.del(it->data, it->wd->obj);
	if (it->long_timer) ecore_timer_del(it->long_timer);
	free(it);
     }
   while (wd->blocks)
     {
	Item_Block *itb = (Item_Block *)(wd->blocks);

	wd->blocks = eina_inlist_remove(wd->blocks, wd->blocks);
	if (itb->items) eina_list_free(itb->items);
	free(itb);
     }
   if (wd->calc_job)
     {
	ecore_job_del(wd->calc_job);
	wd->calc_job = NULL;
     }
   if (wd->queue_idler)
     {
	ecore_idler_del(wd->queue_idler);
	wd->queue_idler = NULL;
     }
   if (wd->queue)
     {
	eina_list_free(wd->queue);
	wd->queue = NULL;
     }
   if (wd->selected)
     {
	eina_list_free(wd->selected);
	wd->selected = NULL;
     }
   wd->show_item = NULL;
   wd->pan_x = 0;
   wd->pan_y = 0;
   wd->minw = 0;
   wd->minh = 0;

	if(wd->alpha_bg)
	  evas_object_del(wd->alpha_bg);
	wd->alpha_bg = NULL;

   if (wd->pan_smart)
     {
        evas_object_size_hint_min_set(wd->pan_smart, wd->minw, wd->minh);
        evas_object_smart_callback_call(wd->pan_smart, "changed", NULL);
     }
   _sizing_eval(obj);
}

/**
 * Enable or disable multi-select in the genlist
 *
 * This enables (EINA_TRUE) or disableds (EINA_FALSE) multi-select in the list. This allows
 * more than 1 item to be selected.
 *
 * @param obj The genlist object
 * @param multi Multi-select enable/disable
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_multi_select_set(Evas_Object *obj, Eina_Bool multi)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->multi = multi;
}

/**
 * Gets if multi-select in genlist is enable or disable
 *
 * @param obj The genlist object
 * @return Multi-select enable/disable
 * (EINA_TRUE = enabled/EINA_FALSE = disabled)
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_multi_select_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->multi;
}


/**
 * Get the selectd item in the genlist
 *
 * This gets the selected item in the list (if multi-select is enabled only
 * the first item in the list is selected - which is not very useful, so see
 * elm_genlist_selected_items_get()for when multi-select is used).
 *
 * If no item is selected, NULL is returned.
 *
 * @param obj The genlist object
 * @return The selected item, or NULL if none.
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_selected_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->selected) return wd->selected->data;
   return NULL;
}

/**
 * Get a list of selected items in the genlist
 *
 * This retgurns a list of the selected items. This list pointer is only valid
 * so long as no items are selected or unselected (or unselected implicitly
 * by deletion). The list contains Elm_Genlist_Item pointers.
 *
 * @param obj The genlist object
 * @return The list of selected items, nor NULL if none are selected.
 *
 * @ingroup Genlist
 */
EAPI const Eina_List *
elm_genlist_selected_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->selected;
}

/**
 * Get a list of realized items in genlist
 *
 * This returns a list of the realized items in the genlist. The list
 * contains Elm_Genlist_Item pointers. The list must be freed by the
 * caller when done with eina_list_free(). The item pointers in the list
 * are only vallid so long as those items are not deleted or the genlist is
 * not deleted.
 *
 * @param obj The genlist object
 * @return The list of realized items, nor NULL if none are realized.
 *
 * @ingroup Genlist
 */
EAPI Eina_List *
elm_genlist_realized_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list = NULL;
   Item_Block *itb;
   Eina_Bool done = EINA_FALSE;
   if (!wd) return NULL;
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
	if (itb->realized)
          {
             Eina_List *l;
             Elm_Genlist_Item *it;
             
             done = 1;
             EINA_LIST_FOREACH(itb->items, l, it)
               {
                  if (it->realized) list = eina_list_append(list, it);
               }
          }
        else
          {
             if (done) break;
          }
     }
   return list;
}

/**
 * Get the item that is at the x, y canvas coords
 *
 * This returns the item at the given coordinates (which are canvas relative
 * not object-relative). If an item is at that coordinate, that item handle
 * is returned, and if @p posret is not NULL, the integer pointed to is set
 * to a value of -1, 0 or 1, depending if the coordinate is on the upper
 * portion of that item (-1), on the middle section (0) or on the lower part
 * (1). If NULL is returned as an item (no item found there), then posret
 * may indicate -1 or 1 based if the coordinate is above or below all items
 * respectively in the genlist.
 *
 * @param it The item
 * @param x The input x coordinate
 * @param y The input y coordinate
 * @param posret The position relative to the item returned here
 * @return The item at the coordinates or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_at_xy_item_get(const Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *posret)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ox, oy, ow, oh;
   Item_Block *itb;
   Evas_Coord lasty;
   if (!wd) return NULL;
   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   lasty = oy;
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
	Eina_List *l;
	Elm_Genlist_Item *it;

        if (!ELM_RECTS_INTERSECT(ox + itb->x - itb->wd->pan_x,
                                 oy + itb->y - itb->wd->pan_y,
                                 itb->w, itb->h, x, y, 1, 1))
          continue;
	EINA_LIST_FOREACH(itb->items, l, it)
	  {
             Evas_Coord itx, ity;

             itx = ox + itb->x + it->x - itb->wd->pan_x;
             ity = oy + itb->y + it->y - itb->wd->pan_y;
             if (ELM_RECTS_INTERSECT(itx, ity, it->w, it->h, x, y, 1, 1))
               {
                  if (posret)
                    {
                       if (y <= (ity + (it->h / 4))) *posret = -1;
                       else if (y >= (ity + it->h - (it->h / 4))) *posret = 1;
                       else *posret = 0;
                    }
                  return it;
               }
             lasty = ity + it->h;
	  }
     }
   if (posret)
     {
        if (y > lasty) *posret = 1;
        else *posret = -1;
     }
   return NULL;
}

/**
 * Get the first item in the genlist
 *
 * This returns the first item in the list.
 *
 * @param obj The genlist object
 * @return The first item, or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_first_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!wd->items) return NULL;
   Elm_Genlist_Item *it = (Elm_Genlist_Item *)(wd->items);
   while ((it) && (it->delete_me))
     it = (Elm_Genlist_Item *)(EINA_INLIST_GET(it)->next);
   return it;
}

/**
 * Get the last item in the genlist
 *
 * This returns the last item in the list.
 *
 * @return The last item, or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_last_item_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd->items) return NULL;
   Elm_Genlist_Item *it = (Elm_Genlist_Item *)(wd->items->last);
   if (!wd) return NULL;
   while ((it) && (it->delete_me))
     it = (Elm_Genlist_Item *)(EINA_INLIST_GET(it)->prev);
   return it;
}

/**
 * Get the next item in the genlist
 *
 * This returns the item after the item @p it.
 *
 * @param it The item
 * @return The item after @p it, or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_next_get(const Elm_Genlist_Item *it)
{
   while (it)
     {
	it = (Elm_Genlist_Item *)(EINA_INLIST_GET(it)->next);
	if ((it) && (!it->delete_me)) break;
     }
   return (Elm_Genlist_Item *)it;
}

/**
 * Get the previous item in the genlist
 *
 * This returns the item before the item @p it.
 *
 * @param it The item
 * @return The item before @p it, or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_prev_get(const Elm_Genlist_Item *it)
{
   while (it)
     {
	it = (Elm_Genlist_Item *)(EINA_INLIST_GET(it)->prev);
	if ((it) && (!it->delete_me)) break;
     }
   return (Elm_Genlist_Item *)it;
}

/**
 * Get the genlist object from an item
 *
 * This returns the genlist object itself that an item belongs to.
 *
 * @param it The item
 * @return The genlist object
 *
 * @ingroup Genlist
 */
EAPI Evas_Object *
elm_genlist_item_genlist_get(const Elm_Genlist_Item *it)
{
   if (!it) return NULL;
   return it->wd->obj;
}

/**
 * Get the parent item of the given item
 *
 * This returns the prent item of the item @p it given.
 *
 * @param it The item
 * @return The parent of the item or NULL if none
 *
 * @ingroup Genlist
 */
EAPI Elm_Genlist_Item *
elm_genlist_item_parent_get(const Elm_Genlist_Item *it)
{
   if (!it) return NULL;
   return it->parent;
}

/**
 * Clear all sub-items (children) of the given item
 *
 * This clears all items that are children (or their descendants) of the
 * given item @p it.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_subitems_clear(Elm_Genlist_Item *it)
{
   Eina_List *tl = NULL, *l;
   Elm_Genlist_Item *it2;

   if (!it) return;
   EINA_LIST_FOREACH(it->items, l, it2)
     tl = eina_list_append(tl, it2);
   EINA_LIST_FREE(tl, it2)
     elm_genlist_item_del(it2);
}

/**
 * Set the selected state of an item
 *
 * This sets the selected state (1 selected, 0 not selected) of the given
 * item @p it.
 *
 * @param it The item
 * @param selected The slected state
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_selected_set(Elm_Genlist_Item *it, Eina_Bool selected)
{
   Widget_Data *wd = elm_widget_data_get(it->wd->obj);
   if (!wd) return;
   if (!it) return;
   if (it->delete_me) return;
   selected = !!selected;
   if (it->selected == selected) return;

   if (selected)
     {
	if (!wd->multi)
	  {
	     while (wd->selected)
	       _item_unselect(wd->selected->data);
	  }
	_item_hilight(it);
	_item_select(it);
     }
   else
     _item_unselect(it);
}

/**
 * Get the selected state of an item
 *
 * This gets the selected state of an item (1 selected, 0 not selected).
 *
 * @param it The item
 * @return The selected state
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_item_selected_get(const Elm_Genlist_Item *it)
{
   if (!it) return EINA_FALSE;
   return it->selected;
}

/**
 * Sets the expanded state of an item (if it's a parent)
 *
 * This expands or contracts a parent iterm (thus showing or hiding the
 * children).
 *
 * @param it The item
 * @param expanded The expanded state (1 expanded, 0 not expanded).
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_expanded_set(Elm_Genlist_Item *it, Eina_Bool expanded)
{
   if (!it) return;
   if (it->expanded == expanded || it->disabled ) return;
   it->expanded = expanded;
   it->wd->expand_item = it;   
	it->effect_done = EINA_FALSE;   
   if (it->expanded)
     {
	it->wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_EXPAND;
	if (it->realized)
	  edje_object_signal_emit(it->base, "elm,state,expanded", "elm");
	evas_object_smart_callback_call(it->wd->obj, "expanded", it);

     }
   else
     {
  	it->wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_CONTRACT;
	if (it->realized)
	  edje_object_signal_emit(it->base, "elm,state,contracted", "elm");
	evas_object_smart_callback_call(it->wd->obj, "contracted", it);
	
     }
 }

/**
 * Get the expanded state of an item
 *
 * This gets the expanded state of an item
 *
 * @param it The item
 * @return Thre expanded state
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_item_expanded_get(const Elm_Genlist_Item *it)
{
   if (!it) return EINA_FALSE;
   return it->expanded;
}

/**
 * Get the depth of expanded item
 *
 * @param it The genlist item object
 * @return The depth of expanded item
 *
 * @ingroup Genlist
 */
EAPI int
elm_genlist_item_expanded_depth_get(const Elm_Genlist_Item *it)
{
   if (!it) return 0;
   return it->expanded_depth;
}

/**
 * Sets the disabled state of an item.
 *
 * A disabled item cannot be selected or unselected. It will also change
 * appearance to appear disabled. This sets the disabled state (1 disabled, 0
 * not disabled).
 *
 * @param it The item
 * @param disabled The disabled state
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_disabled_set(Elm_Genlist_Item *it, Eina_Bool disabled)
{
   if (!it) return;
   if (it->disabled == disabled) return;
   if (it->delete_me) return;
   it->disabled = disabled;
   if (it->realized)
     {
	if (it->disabled)
	  edje_object_signal_emit(it->base, "elm,state,disabled", "elm");
	else
	  edje_object_signal_emit(it->base, "elm,state,enabled", "elm");
     }
}

/**
 * Get the disabled state of an item
 *
 * This gets the disabld state of the given item.
 *
 * @param it The item
 * @return The disabled state
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_item_disabled_get(const Elm_Genlist_Item *it)
{
   if (!it) return EINA_FALSE;
   if (it->delete_me) return EINA_FALSE;
   return it->disabled;
}

/**
 * Sets the display only state of an item.
 *
 * A display only item cannot be selected or unselected. It is for display
 * only and not selecting or otherwise clicking, dragging etc. by the user,
 * thus finger size rules will not be applied to this item.
 *
 * @param it The item
 * @param display_only The display only state
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_display_only_set(Elm_Genlist_Item *it, Eina_Bool display_only)
{
   if (!it) return;
   if (!it->block) return;
   if (it->display_only == display_only) return;
   if (it->delete_me) return;
   it->display_only = display_only;
   it->mincalcd = EINA_FALSE;
   it->updateme = EINA_TRUE;
   it->block->updateme = EINA_TRUE;
   if (it->wd->update_job) ecore_job_del(it->wd->update_job);
   it->wd->update_job = ecore_job_add(_update_job, it->wd);
}

/**
 * Get the display only state of an item
 *
 * This gets the display only state of the given item.
 *
 * @param it The item
 * @return The display only state
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_item_display_only_get(const Elm_Genlist_Item *it)
{
   if (!it) return EINA_FALSE;
   if (it->delete_me) return EINA_FALSE;
   return it->display_only;
}

/**
 * Show the given item
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_show(Elm_Genlist_Item *it)
{
   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   elm_smart_scroller_child_region_show(it->wd->scr,
					it->x + it->block->x,
					it->y + it->block->y,
					it->block->w, it->h);
}

/**
 * Bring in the given item
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible. This may use animation to do so and take a
 * period of time
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_bring_in(Elm_Genlist_Item *it)
{
   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   elm_smart_scroller_region_bring_in(it->wd->scr,
                                      it->x + it->block->x,
                                      it->y + it->block->y,
                                      it->block->w, it->h);
}

/**
 * Show the given item at the top
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_top_show(Elm_Genlist_Item *it)
{
   Evas_Coord ow, oh;

   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &ow, &oh);
   elm_smart_scroller_child_region_show(it->wd->scr,
					it->x + it->block->x,
					it->y + it->block->y,
					it->block->w, oh);
}

/**
 * Bring in the given item at the top
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible. This may use animation to do so and take a
 * period of time
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_top_bring_in(Elm_Genlist_Item *it)
{
   Evas_Coord ow, oh;

   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &ow, &oh);
   elm_smart_scroller_region_bring_in(it->wd->scr,
                                      it->x + it->block->x,
                                      it->y + it->block->y,
                                      it->block->w, oh);
}

/**
 * Show the given item at the middle
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_middle_show(Elm_Genlist_Item *it)
{
   Evas_Coord ow, oh;

   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &ow, &oh);
   elm_smart_scroller_child_region_show(it->wd->scr,
					it->x + it->block->x,
		         it->y + it->block->y - oh/2 + it->h/2,
					it->block->w, oh);
}


/**
 * Bring in the given item at the middle
 *
 * This causes genlist to jump to the given item @p it and show it (by scrolling),
 * if it is not fully visible. This may use animation to do so and take a
 * period of time
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_middle_bring_in(Elm_Genlist_Item *it)
{
   Evas_Coord ow, oh;

   if (!it) return;
   if (it->delete_me) return;
   if ((it->queued) || (!it->mincalcd))
     {
	it->wd->show_item = it;
        it->wd->bring_in = 1;
	it->showme = EINA_TRUE;
	return;
     }
   if (it->wd->show_item)
     {
	it->wd->show_item->showme = EINA_FALSE;
	it->wd->show_item = NULL;
     }
   evas_object_geometry_get(it->wd->pan_smart, NULL, NULL, &ow, &oh);
   elm_smart_scroller_region_bring_in(it->wd->scr,
                                      it->x + it->block->x,
                                      it->y + it->block->y - oh/2 + it->h/2,
                                      it->block->w, oh);
}

/**
 * Delete a given item
 *
 * This deletes the item from genlist and calls the genlist item del class
 * callback defined in the item class, if it is set.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_del(Elm_Genlist_Item *it)
{
   if (!it) return;
   if ((it->relcount > 0) || (it->walking > 0))
     {
        elm_genlist_item_subitems_clear(it);
        it->delete_me = EINA_TRUE;
        if (it->wd->show_item == it) it->wd->show_item = NULL;
        if (it->selected) it->wd->selected = eina_list_remove(it->wd->selected, it);
        if (it->block)
          {
             if (it->realized) _item_unrealize(it);
             it->block->changed = EINA_TRUE;
             if (it->wd->calc_job) ecore_job_del(it->wd->calc_job);
             it->wd->calc_job = ecore_job_add(_calc_job, it->wd);
          }
        if (it->itc->func.del)
           it->itc->func.del((void *)it->data, it->wd->obj);
        return;
     }
   _item_del(it);
}

/**
 * Set the data item from the genlist item
 *
 * This set the data value passed on the elm_genlist_item_append() and
 * related item addition calls. This function will also call
 * elm_genlist_item_update() so the item will be updated to reflect the
 * new data.
 *
 * @param it The item
 * @param data The new data pointer to set
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_data_set(Elm_Genlist_Item *it, const void *data)
{
   if (!it) return;
   it->data = data;
   elm_genlist_item_update(it);
}

/**
 * Get the data item from the genlist item
 *
 * This returns the data value passed on the elm_genlist_item_append() and
 * related item addition calls.
 *
 * @param it The item
 * @return The data pointer provided when created
 *
 * @ingroup Genlist
 */
EAPI const void *
elm_genlist_item_data_get(const Elm_Genlist_Item *it)
{
   if (!it) return NULL;
   return it->data;
}

/**
 * Get the real evas object of the genlist item
 *
 * This returns the actual evas object used for the specified genlist item.
 * This may be NULL as it may not be created, and ma be deleted at any time
 * by genlist. Do not modify this object (move, resize, show, hide etc.) as
 * genlist is controlling it. This function is for querying, emitting
 * custom signals or hooking lower level callbacks for events. Do not
 * delete this object under any circumstances.
 *
 * @param it The item
 * @return The objct pointer
 *
 * @ingroup Genlist
 */
EAPI const Evas_Object *
elm_genlist_item_object_get(const Elm_Genlist_Item *it)
{
   if (!it) return NULL;
   return it->base;
}

/**
 * Update the contents of an item
 *
 * This updates an item by calling all the item class functions again to get
 * the icons, labels and states. Use this when he original item data has
 * changed and the changes are desired to be reflected.
 *
 * @param it The item
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_update(Elm_Genlist_Item *it)
{
   if (!it->block) return;
   if (it->delete_me) return;
   it->mincalcd = EINA_FALSE;
   it->updateme = EINA_TRUE;
   it->block->updateme = EINA_TRUE;
   if (it->wd->update_job) ecore_job_del(it->wd->update_job);
   it->wd->update_job = ecore_job_add(_update_job, it->wd);
}

/**
 * This sets the horizontal stretching mode
 *
 * This sets the mode used for sizing items horizontally. Valid modes are
 * ELM_LIST_LIMIT and ELM_LIST_SCROLL. The default is ELM_LIST_SCROLL. This
 * mode means that if items are too wide to fit, the scroller will scroll
 * horizontally. Otherwise items are expanded to fill the width of the
 * viewport of the scroller. If it is ELM_LIST_LIMIT, Items will be expanded
 * to the viewport width and limited to that size.
 *
 * @param obj The genlist object
 * @param mode The mode to use
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_horizontal_mode_set(Evas_Object *obj, Elm_List_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->mode == mode) return;
   wd->mode = mode;
   _sizing_eval(obj);
}

/**
 * Gets the horizontal stretching mode
 *
 * @param obj The genlist object
 * @return The mode to use
 * (ELM_LIST_LIMIT, ELM_LIST_SCROLL, ELM_LIST_LIMIT)
 *
 * @ingroup Genlist
 */
EAPI Elm_List_Mode
elm_genlist_horizontal_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_LIST_LAST;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_LIST_LAST;
   return wd->mode;
}

/**
 * Set the always select mode.
 *
 * Items will only call their selection func and callback when first becoming
 * selected. Any further clicks will do nothing, unless you enable always
 * select with elm_genlist_always_select_mode_set(). This means even if
 * selected, every click will make the selected callbacks be called.
 *
 * @param obj The genlist object
 * @param always_select The always select mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_always_select_mode_set(Evas_Object *obj, Eina_Bool always_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->always_select = always_select;
}

/**
 * Get the always select mode.
 *
 * @param obj The genlist object
 * @return The always select mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_always_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->always_select;
}

/**
 * Set no select mode
 *
 * This will turn off the ability to select items entirely and they will
 * neither appear selected nor call selected callback functions.
 *
 * @param obj The genlist object
 * @param no_select The no select mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_no_select_mode_set(Evas_Object *obj, Eina_Bool no_select)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->no_select = no_select;
}

/**
 * Gets no select mode
 *
 * @param obj The genlist object
 * @return The no select mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_no_select_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->no_select;
}

/**
 * Set compress mode
 *
 * This will enable the compress mode where items are "compressed" horizontally
 * to fit the genlist scrollable viewport width.
 *
 * @param obj The genlist object
 * @param no_select The compress mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_compress_mode_set(Evas_Object *obj, Eina_Bool compress)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->compress = compress;
}

/**
 * Get the compress mode
 *
 * @param obj The genlist object
 * @return The compress mode
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_compress_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->compress;
}

/**
 * Set bounce mode
 *
 * This will enable or disable the scroller bounce mode for the genlist. See 
 * elm_scroller_bounce_set() for details
 *
 * @param obj The genlist object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_set(wd->scr, h_bounce, v_bounce);
}

/**
 * Get the bounce mode
 *
 * @param obj The genlist object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_get(obj, h_bounce, v_bounce);
}

/**
 * Set homogenous mode
 *
 * This will enable the homogeneous mode where items are of the same height and width
 * so that genlist may do the lazy-loading at its maximum.  This implies 'compressed' mode
 *
 * @param obj The genlist object
 * @param homogeneous Assume the items within the genlist are of the same height and width
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_homogeneous_set(Evas_Object *obj, Eina_Bool homogeneous)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (homogeneous) elm_genlist_compress_mode_set(obj, EINA_TRUE);
   wd->homogeneous = homogeneous;
}

/**
 * Get the homogenous mode
 *
 * @param obj The genlist object
 * @return Assume the items within the genlist are of the same height and width
 * (EINA_TRUE = on, EINA_FALSE = off)
 *
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_homogeneous_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->homogeneous;
}

/**
 * Set the maximum number of items within an item block
 *
 * This will configure the block count to tune to the target with particular performance matrix.
 *
 * @param obj The genlist object
 * @param n   Maximum number of items within an item block
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_block_count_set(Evas_Object *obj, int n)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->max_items_per_block = n;
}

/**
 * Get the maximum number of items within an item block
 *
 * @param obj The genlist object
 * @return Maximum number of items within an item block
 *
 * @ingroup Genlist
 */
EAPI int
elm_genlist_block_count_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->max_items_per_block;
}

/**
 * Set the Genlist Internal scroller scrollbar policy
 *
 * This sets the Genlist Internal scrollbar visibility policy.
 * ELM_SMART_SCROLLER_POLICY_AUTO means the scrollbar is made visible if it
 * is needed, and otherwise kept hidden. ELM_SMART_SCROLLER_POLICY_ON turns
 * it on all the time, and ELM_SMART_SCROLLER_POLICY_OFF always keeps it off.
 * This applies respectively for the horizontal and vertical scrollbars.
 *
 * @param obj The Genlist object
 * @param policy_h Horizontal scrollbar policy
 * @param policy_v Vertical scrollbar policy
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_scroller_policy_set(Evas_Object *obj, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)  return;

   const Elm_Scroller_Policy map[3] =
       {
           ELM_SMART_SCROLLER_POLICY_AUTO,
            ELM_SMART_SCROLLER_POLICY_ON,
            ELM_SMART_SCROLLER_POLICY_OFF
       };
   if ((policy_h < 0) || (policy_h >= 3) || (policy_v < 0) || (policy_v >= 3))
     return;

   elm_smart_scroller_policy_set(wd->scr, map[policy_h], map[policy_v]);
}

EAPI void
elm_genlist_set_edit_mode(Evas_Object *obj, int emode, Elm_Genlist_Edit_Class *edit_class)
{
   fprintf(stderr, "=================> Caution!!! <========================\n");
   fprintf(stderr, "==> elm_genlist_set_edit_mode() is deprecated. <=======\n");
   fprintf(stderr, "==> Please use elm_genlist_edit_mode_set() instead. <==\n");
   fprintf(stderr, "=======================================================\n");

   elm_genlist_edit_mode_set(obj, emode, edit_class);
}

/**
 * Set Genlist edit mode
 *
 * This sets Genlist edit mode.
 *
 * @param obj The Genlist object
 * @param emode ELM_GENLIST_EDIT_MODE_{NONE & REORDER & INSERT & DELETE & SELECT & SELECT_ALL}
 * @param edit_class Genlist edit class (Elm_Genlist_Edit_Class structure)
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_edit_mode_set(Evas_Object *obj, int emode, Elm_Genlist_Edit_Class *edit_class)
{
	Elm_Genlist_Item *it;
	Eina_List *l;
	Item_Block *itb;

	static Elm_Genlist_Item_Class itc;

	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->edit_mode == emode) return;

	wd->edit_mode = emode;

	wd->animate_edit_controls = 1;

	if (wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECTALL)
	  wd->edit_mode |= ELM_GENLIST_EDIT_MODE_SELECT;

	if(wd->edit_mode_effect_mode) {
		  wd->effect_mode = EINA_TRUE;
		  wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_EDIT_MODE;
	}

	if (wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE)
	  {
		  if (wd->ed) free (wd->ed);
		  wd->ed = NULL;
	  }
	else
	  {
		  if (!wd->ed)
			 wd->ed = calloc(1, sizeof(Edit_Data));

		  wd->ed->ec = edit_class;

		  if (((wd->edit_mode & ELM_GENLIST_EDIT_MODE_DELETE) || (wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECT)) && !wd->ed->del_confirm)
			 {
				 wd->ed->del_confirm = elm_button_add(wd->obj);
				 elm_button_label_set(wd->ed->del_confirm, "Delete");
				 evas_object_smart_member_add(wd->ed->del_confirm, wd->pan_smart);
				 edje_object_scale_set( wd->ed->del_confirm, elm_widget_scale_get(wd->ed->del_confirm) *
						 _elm_config->scale);
				 evas_object_smart_callback_add(wd->ed->del_confirm, "clicked", _delete_confirm_cb, wd );
			 }
	  }

	EINA_INLIST_FOREACH(wd->blocks, itb)
	  {
		  EINA_LIST_FOREACH(itb->items, l, it)
			 {
				 it->delete_check = 0;
				 it->del_confirm_state = 0;
				 _item_unselect(it);
				 _edit_controls_eval(it);
			 }
	  }

	if (wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECTALL)
	  {
		  if(edit_class->select_all_item_style && strcmp(edit_class->select_all_item_style, "default"))
			 itc.item_style = edit_class->select_all_item_style;
		  else
			 itc.item_style = "select_all";
		  itc.func.label_get = NULL;
		  itc.func.icon_get = NULL;
		  itc.func.del = NULL;
		  itc.func.editmode_get = NULL;

		  wd->select_all_item = _item_new(wd, &itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

		  if (!wd) return;
		  if (!wd->select_all_item) return;

		  _item_realize(wd->select_all_item, 0, 0);
		  edje_object_signal_callback_add(wd->select_all_item->base, "elm,action,select,press", "elm", _select_all_down, wd->select_all_item);

		  wd->select_all_item->rel = NULL;
		  wd->select_all_item->selected = 0;
		  wd->select_all_item->before = 1;
		  wd->select_all_item->block = NULL;
		  wd->select_all_minh = wd->minh + wd->select_all_item->h;
		  }
	  else if(wd->move_effect_mode != ELM_GENLIST_ITEM_MOVE_EFFECT_EDIT_MODE)
	  {
		  if (wd->select_all_item)
			 {
				 if (wd->select_all_item->realized) _item_unrealize(wd->select_all_item);
				 free(wd->select_all_item);
			 }
		  wd->select_all_item = NULL;

	  }
	edje_object_signal_emit(wd->scr, "elm,state,edit,animated,enable", "elm");

	if (wd->calc_job) ecore_job_del(wd->calc_job);
	wd->calc_job = ecore_job_add(_calc_job, wd);
}

/**
 * Delete selected items in genlist edit mode.
 *
 * @param obj The genlist object
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_edit_selected_items_del(Evas_Object *obj)
{
  Elm_Genlist_Item *it;
   Eina_List *l;
   Item_Block *itb = NULL;
   Evas_Object *icon;     
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if(!wd->blocks) return;

   
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
	if(!wd->blocks) break;
	
	if (itb )
	  {
	     EINA_LIST_FOREACH(itb->items, l, it)
	       {
		  if (it->delete_check) 
		    {
				 it->wd->effect_mode = EINA_TRUE;
             
             if(!wd->selct_all) 
             {
		       	itb->wd->minh -= it->h;
		       	itb->wd->select_all_minh -= it->h;
             }

				if ((it->relcount > 0) || (it->walking > 0))
			     {
				elm_genlist_item_subitems_clear(it);
				it->delete_me = EINA_TRUE;
				if (it->wd->show_item == it) it->wd->show_item = NULL;
				if (it->selected) it->wd->selected = eina_list_remove(it->wd->selected, it);
				if (it->block)
				  {
				     if (it->realized) _item_unrealize(it);
				     it->block->changed = EINA_TRUE;
				  }
				if (it->itc->func.del) it->itc->func.del(it->data, it->wd->obj);
				return;
			     }

			   it->wd->walking -= it->walking;
			   if (it->wd->show_item == it) it->wd->show_item = NULL;
			   if (it->selected) it->wd->selected = eina_list_remove(it->wd->selected, it);
			   if (it->realized) _item_unrealize(it);

				EINA_LIST_FREE(it->edit_icon_objs, icon)
				   evas_object_del(icon);

				Evas_Object *editfield;
		   	EINA_LIST_FREE(it->wd->edit_field, editfield)
				   evas_object_del(editfield);

				evas_object_del(it->edit_obj);
				it->edit_obj = NULL;      

			   itb->items = eina_list_remove(itb->items, it);
			   itb->count--;
			   itb->changed = EINA_TRUE;
			   if ((!it->delete_me) && (it->itc->func.del))
			     it->itc->func.del(it->data, it->wd->obj);
			   it->delete_me = EINA_TRUE;
			   if (it->queued)
			     it->wd->queue = eina_list_remove(it->wd->queue, it);
			   it->wd->items = eina_inlist_remove(it->wd->items, EINA_INLIST_GET(it));
			   if (it->parent)
			     it->parent->items = eina_list_remove(it->parent->items, it);
			   if (it->long_timer) ecore_timer_del(it->long_timer);
			   if( it->group_item )
					it->group_item->items = eina_list_remove(it->group_item->items,it);

			   free(it);
		    }
       }
	  }
   }

   evas_render(evas_object_evas_get(wd->obj));
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);	
}


EAPI void
elm_genlist_selected_items_del(Evas_Object *obj)
{
   fprintf(stderr, "=================> Caution!!! <========================\n");
   fprintf(stderr, "==> elm_genlist_selected_items_del() is deprecated. <=======\n");
   fprintf(stderr, "==> Please use elm_genlist_edit_selected_items_del() instead. <==\n");
   fprintf(stderr, "=======================================================\n");
   elm_genlist_edit_selected_items_del(obj);
}

/**
 * Get a list of selected items in genlist
 *
 * This returns a list of the selected items in the genlist. The list
 * contains Elm_Genlist_Item pointers. The list must be freed by the
 * caller when done with eina_list_free(). The item pointers in the list
 * are only vallid so long as those items are not deleted or the genlist is
 * not deleted.
 *
 * @param obj The genlist object
 * @return The list of selected items, nor NULL if none are selected.
 *
 * @ingroup Genlist
 */
EAPI Eina_List *
elm_genlist_edit_selected_items_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list = NULL;
   Item_Block *itb;
   if (!wd) return NULL;
	
   EINA_INLIST_FOREACH(wd->blocks, itb)
     {
		  Eina_List *l;
	     Elm_Genlist_Item *it;

	     EINA_LIST_FOREACH(itb->items, l, it)
	       {
				 if (it->delete_check) list = eina_list_append(list, it);
	       }

     }
   return list;
}

/**
 * Set a given item's rename mode
 *
 * This renames the item's label from genlist 
 *
 * @param it The item
 * @param emode set if emode is EINA_TRUE, unset if emode is EINA_FALSE
 *
 * @ingroup Genlist
 */
EAPI void
elm_genlist_item_rename_mode_set(Elm_Genlist_Item *it, int emode)
{
   if (!it) return;

   const Eina_List *l, *list, *l2;
   const char *label, *rename_swallow_part;
   char *s;
   Eina_Bool done = EINA_FALSE;
   int label_cnt = 0 , swallow_part_cnt = 0;

   Item_Block *itb;
   Evas_Object *editfield;
   Evas_Object *entry;
   int edit_field_cnt = 0;
   Evas_Object *icon;   

   if (it->wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE)
     {
	it->wd->edit_mode = 0xF0;
	_edit_controls_eval(it);
     }

   EINA_INLIST_FOREACH(it->wd->blocks, itb)
     {
	if (itb->realized)
	  {
	     Eina_List *l;
	     Elm_Genlist_Item *it;

	     done = EINA_TRUE;
	     EINA_LIST_FOREACH(itb->items, l, it)
	       {
		  if (it->renamed)
		    {
		       it->renamed = EINA_FALSE;


		       EINA_LIST_FOREACH(it->wd->edit_field, l2, editfield)
			 {
			    entry = elm_editfield_entry_get(editfield);
			    const char *text = elm_entry_entry_get(entry);

			    if (it->itc->func.label_changed)
			      it->itc->func.label_changed( it->data, it, text, edit_field_cnt++);
			 }
		       Ecore_IMF_Context *imf = elm_entry_imf_context_get(entry);
		       ecore_imf_context_input_panel_hide(imf);

		       Evas_Object *editfield;
		       EINA_LIST_FREE(it->wd->edit_field, editfield)
			  evas_object_del(editfield);

		       if (it->wd->edit_mode)
			 {
			    if ((it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_INSERT))
			      edje_object_signal_emit(it->edit_obj, "elm,state,ins,enable", "elm");

			    if ((it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_DELETE) || (it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_SELECT))
			      edje_object_signal_emit(it->edit_obj, "elm,state,del,enable", "elm");

			    if ((it->wd->edit_mode & ELM_GENLIST_EDIT_MODE_REORDER))
			      edje_object_signal_emit(it->edit_obj, "elm,state,rename,disable", "elm");				  
			 }
		       if (it->edit_obj)
			 {
			    evas_object_del(it->edit_obj);
			    it->edit_obj = NULL;
			 }
	       	EINA_LIST_FREE(it->edit_icon_objs, icon)
				   evas_object_del(icon);
		    }
	       }
	  }
	else
	  {
	     if (done) break;
	  }
     }

   //		if (emode != ELM_GENLIST_RENAME_MODE_NONE) 
   if (emode != 0) 
     {
	it->renamed = EINA_TRUE;

	it->labels = _elm_stringlist_get(edje_object_data_get(it->base, "labels"));
	EINA_LIST_FOREACH(it->labels, list, label)
	  {
	     edje_object_signal_emit(it->edit_obj, "elm,state,rename,enable", "elm");
	     edje_object_signal_emit(it->edit_obj, "elm,state,ins,disable", "elm");
	     edje_object_signal_emit(it->edit_obj, "elm,state,del,disable", "elm");
	     edje_object_signal_emit(it->edit_obj, "elm,state,edit_end,disable", "elm");

	     if (it->itc->func.label_get)
	       {
		  it->renamed = EINA_TRUE;
		  swallow_part_cnt = 0;

	     EINA_LIST_FREE(it->edit_icon_objs, icon)
		evas_object_del(icon);
		  Eina_List *rename = _elm_stringlist_get(edje_object_data_get(it->edit_obj, "rename"));
		  EINA_LIST_FOREACH(rename, l, rename_swallow_part)
		    {
		       if (label_cnt == swallow_part_cnt)
			 {
			    editfield = elm_editfield_add(it->wd->obj);
			    it->wd->edit_field = eina_list_append(it->wd->edit_field, editfield);


			    elm_editfield_entry_single_line_set(editfield, EINA_TRUE);	
			    elm_editfield_eraser_set(editfield, EINA_TRUE);
			    edje_object_part_swallow(it->edit_obj, rename_swallow_part, editfield);

			    evas_object_show(editfield);

			    s = it->itc->func.label_get(it->data, it->wd->obj, list->data);

			    if (s)
			      {
				 Evas_Object *entry = elm_editfield_entry_get(editfield);
				 elm_entry_entry_set(entry,s);

				 free(s);
			      }
			    else
			      elm_editfield_guide_text_set(editfield, "Text Input");
			 }
		       swallow_part_cnt++;
		    }
		  label_cnt++;
	       }
	  }			
     }

   if (it->wd->edit_mode == 0xF0)
     {
	it->wd->edit_mode = 0;
     }
}

/**
 * Set the timeout in seconds for the longpress event
 * 
 * @param obj The genlist object
 * @param timeout timeout in seconds
 * 
 * @ingroup Genlist
 */
EAPI void
elm_genlist_longpress_timeout_set(Evas_Object *obj, double timeout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->longpress_timeout = timeout;
}

/**
 * Get the timeout in seconds for the longpress event
 * 
 * @param obj The genlist object
 * @return timeout in seconds
 * 
 * @ingroup Genlist
 */
EAPI double
elm_genlist_longpress_timeout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->longpress_timeout;
}

// added for item moving animation.
	static Eina_Bool
_group_item_contract_moving_effect_timer_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(obj);   

	Item_Block  *itb = NULL;
	Elm_Genlist_GroupItem *git;
	Elm_Genlist_Item *it;
	const Eina_List *l;
	int cnt = 0, git_count = 0;
	double added_gy = 1;

	int hide_git = 0, git_cnt = 0, list_start_y = 0;

	int *git_array = NULL;
	int base_git = 0, base_git_num = 0;
	int tmp_y = 0,  devide_size = 1;
	double t;

	Eina_Bool finish = EINA_FALSE;

	Evas_Coord ox, oy, ow, oh;
	if(!wd)
	  return ECORE_CALLBACK_CANCEL;

	if(wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE)
	  return ECORE_CALLBACK_CANCEL;

	git_array = (int*)malloc(sizeof(int) * wd->max_git_num);
	t = ecore_loop_time_get();

	if (t - wd->effect_start >= 5.0) 
	  finish = EINA_TRUE;

	if(wd->item_count < 100)
		devide_size = 8 * _elm_config->scale;
	else if(wd->item_count < 500)
		devide_size = (8 - (devide_size / 100 )) * _elm_config->scale;
	else
		devide_size = 2 * _elm_config->scale;

	evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);

	list_start_y = oy;

	EINA_INLIST_FOREACH(wd->group_items, git)
	  {
		  git_array[git_cnt++]  =  git->y;
		  if(git->y < list_start_y) 
			 hide_git++;
		  edje_object_signal_emit(git->base, "elm,state,alpha,disable", "elm");
	  }
	base_git_num = hide_git;

	EINA_INLIST_FOREACH(wd->group_items, git)
	  {
		  base_git = oy + git->h * (git->num-1) + git->num;
		  git->old_y = git->y;
		  added_gy = abs(base_git - git->y) / devide_size;
		  if(added_gy < 1.0)
			 added_gy = 1.0;

		  if (!git->down && git->old_y < list_start_y) 
			 {
				 git->finish_y = base_git;
				 git->down = 1;
				 git->y = list_start_y - (git->h+1) * hide_git--;

				 git->old_y = 0;
			 }
		  else if (!git->down && git->y < base_git )
			 git->down = 1;

		  if(wd->pinch_zoom_reserve)
			 git->y = base_git;
		  else 
	   {

		  if (git->down) 
			 {
				 if (git->y < base_git) 
					{
						git->y +=added_gy;
						if (git->y > base_git)
						  git->y = base_git;
					}
			 }
		  else 
			 {
				 if (git->y > base_git)  
					git->y -= added_gy;

				 if (git->y < base_git)
					git->y = base_git;
			 }
		  }
		  if (git->num - 1 == cnt && git->y == base_git)
			 git_count++;

		  evas_object_resize(git->base, wd->minw, git->h );
		  evas_object_move(git->base, git->x, git->y);
		  evas_object_raise(git->base);        
		  evas_object_show(git->base);

		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 {
				 EINA_LIST_FOREACH(itb->items, l, it)
					{
						if (it->group_item == git) 
						  {
							  if (it->group_item->old_y)
								 it->old_scrl_y -= (it->group_item->old_y - it->group_item->y);

							  if(git_array[it->group_item->num+1] <=  it->old_scrl_y || added_gy == 1.0)
								 evas_object_color_set(it->base, 0,0,0,0);

							  _move_edit_controls(it,it->scrl_x, it->scrl_y);
							  evas_object_resize(it->base, itb->wd->minw-(it->pad_left+it->pad_right), it->h);
							  evas_object_move(it->base, it->scrl_x+it->pad_left, it->old_scrl_y);
							  evas_object_raise(it->group_item->base);
							  evas_object_show(it->base);
						  }
					}
			 }
		  if (git_count == git_cnt ) 
			 {
				 finish = EINA_TRUE;
				 break;
			 }

		  cnt++;
	  }
	free(git_array);

	if(finish) // finish animation
	  {
		  added_gy = 1;
		  EINA_INLIST_FOREACH(wd->group_items, git)
			  git->down = 0;

		  wd->pan_y = 0;
		  wd->contract_pan_y = 0;
		  wd->pinch_zoom_reserve = EINA_FALSE;
		  wd->pinchzoom_effect_mode = ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH;
		  elm_smart_scroller_freeze_momentum_animator_set(wd->scr, 0);
		  elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, EINA_TRUE);

//		  fprintf(stderr,"ELM_GENLIST_ITEM_MOVE_EFFECT_CONTRACT  FINISH   \n");         

		  tmp_y  =0 ;
		  // added for event
		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 { 
				 if(itb->realized) {
						EINA_LIST_FOREACH(itb->items, l, it)
						  {
							  if(it->realized) {
									 it->scrl_y = oy + tmp_y;
									 tmp_y += it->h;
									 // need to handle edit mode
									 evas_object_move(it->base, it->scrl_x+it->pad_left, it->old_scrl_y);
									 evas_object_color_set(it->base, 0,0,0,0);
									 evas_object_show(it->base);
							  }
						  }
				 }

			 }
		  evas_object_lower(wd->alpha_bg);
		  evas_object_hide(wd->alpha_bg);

		  return ECORE_CALLBACK_CANCEL;      
	  }

	return ECORE_CALLBACK_RENEW;
}

// added for item moving animation.
	static Eina_Bool
_group_item_expand_moving_effect_timer_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Genlist_GroupItem *git, *tmp_git;
	Item_Block *itb = NULL;   
	Elm_Genlist_Item *it;
	const Eina_List *l;
	int cnt = 0, git_count = 0, git_cnt = 0, git_tmp_y = 0, in = 0, start_in = 0;
	int tmp = 0, show_git_cnt = 0, scroll_y = 0, top_git = 0 , git_h = 0, scroll_pan_y = 0, down = 0;
	int up_cnt = 1, down_cnt = 1, it_h = 0, devide_size = 1;
	static int last_git_y = 0;
   
	double t;

	Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;

	static double added_gy = 0; // temp value for animation speed
	static double added_gy2 = 0;
	Eina_Bool finish = EINA_FALSE;
	if (!wd)
	  return ECORE_CALLBACK_CANCEL;

	if(wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE)
	  return ECORE_CALLBACK_CANCEL;

	t = ecore_loop_time_get();

	top_git = wd->pinch_it;

	if (top_git< 1)
	  top_git = 1;
	else if (top_git >= wd->max_git_num)
	  top_git = wd->max_git_num - 1;    

	if(wd->item_count < 100)
		devide_size = 8 * _elm_config->scale;
	else if(wd->item_count < 500)
		devide_size = (8 - (devide_size / 100 )) * _elm_config->scale;
	else
		devide_size = 2 * _elm_config->scale;

	evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
	evas_output_viewport_get(evas_object_evas_get(wd->pan_smart), &cvx, &cvy, &cvw, &cvh);

	// calculate git count and srcroll move position
	EINA_INLIST_FOREACH(wd->group_items, git)
	  {
		  if (git_cnt == top_git - 1) 
			 scroll_y = tmp;

		  if (!scroll_y)
			 {
				 EINA_INLIST_FOREACH(wd->blocks, itb)
					{             
						EINA_LIST_FOREACH(itb->items, l, it)
						  {
							  if (it->group_item == git ) 
								 tmp += it->h;
							  it_h = it->h;
						  }
					}
			 }
		  git_h = git->h;
		  git_cnt++;

	  }
	scroll_pan_y = scroll_y + git_h * (top_git-1) ;

	if (t - wd->effect_start >= 5.0) 
	  finish = EINA_TRUE;

	// items realize
	EINA_INLIST_FOREACH(wd->blocks, itb)
	  {
		  if((itb->y + itb->h >= scroll_pan_y - oh && itb->y <= scroll_pan_y + oh) 
				  || (itb->y >= scroll_pan_y - oh && itb->y <= scroll_pan_y + oh))
			 {
				 if (!itb->realized)
					{
						start_in = in;
						EINA_LIST_FOREACH(itb->items, l, it)
						  {
							  _item_realize(it, start_in, 0);
							  it->realized = EINA_TRUE;
							  start_in++;
						  }
						itb->realized = EINA_TRUE;
						itb->want_unrealize = EINA_FALSE;         

					}
			 }
		  else
			 {
				 if (itb->realized) _item_block_unrealize(itb);
			 }
		  in += itb->count;
	  }

	// set group item finish y and items y position of group item and items. 
	tmp = 0;
	git_tmp_y = oy;   
	EINA_INLIST_FOREACH(wd->group_items, git)
	  {
		  if (git->num >= top_git)
			 {
				 git->finish_y = git_tmp_y; 
				 git_tmp_y += git->h;
				 tmp = git->y + git->h;

			 }
		  else 
			 git->finish_y =  -1 * oh;

		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 { 
				 EINA_LIST_FOREACH(itb->items, l, it)
					{
						if (it->group_item == git ) 
						  {
							  if(git->finish_y !=  -1 * oh)
								 {
									 it->old_scrl_y = tmp;
									 git_tmp_y += it->h;
									 added_gy2 = abs(git->finish_y - git->y) / devide_size;
									 tmp += it->h - added_gy2;
								 }
							  else
								 it->old_scrl_y = -1 * oh;
						  }
					}
			 }

		  if (git->finish_y  >= oy && git->finish_y < oy+oh)
			 show_git_cnt++;
	  }

	EINA_INLIST_FOREACH(wd->group_items, git)
	  {
		  down = 0;

		  evas_object_move(git->base, git->x, git->y);
		  evas_object_raise(git->base);
		  evas_object_show(git->base);

		  added_gy = abs(git->finish_y - git->y) / devide_size;
		  if(added_gy < 1.0)
			 added_gy = 1.0;
		  if(git->y > git->finish_y) {
				 if (git->y > oy)
					git->y -= added_gy; 


				 if(git->num >= top_git)
					up_cnt++;
				 down = 0;

		  }
		  else if(git->y < git->finish_y) {
				 git->y += added_gy;
				 down_cnt++;
				 down = 1;
		  }

		  if( (!down && git->y < git->finish_y) || (down && git->y > git->finish_y) )
			 git->y = git->finish_y; 

		  if (git_cnt-1 == cnt)
			 last_git_y = git->y;

		  if (git->num == top_git && git->y == oy)
			 {
				 evas_object_move(git->base, git->x, git->y);
				 evas_object_show(git->base);
				 git_count = 0;
				 EINA_INLIST_FOREACH(wd->group_items, tmp_git)
					{
						if (tmp_git->y == tmp_git->finish_y && tmp_git->y > oy && tmp_git->y < oy+oh   )
						  {
							  git_count++;        
						  }
					}
			 }

		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 { 
				 EINA_LIST_FOREACH(itb->items, l, it)
					{
						if (it->group_item == git)
						  {

							  if((itb->y + itb->h >= scroll_pan_y - oh && itb->y <= scroll_pan_y + oh) 
									  || (itb->y >= scroll_pan_y - oh && itb->y <= scroll_pan_y + oh))

								 {            
									 evas_object_resize(it->base, wd->minw-(it->pad_left+it->pad_right), it->h);
									 evas_object_move(it->base, it->scrl_x+it->pad_left, it->old_scrl_y);
									 evas_object_color_set(it->base, 255,255,255,255);
									 evas_object_raise(it->base);
									 evas_object_raise(it->group_item->base);

								 }
						  }
					}
			 }

		  cnt++;

		  if(git_count + 1  == show_git_cnt ) 
			 {
				 finish = EINA_TRUE;
			 }

	  }
	if(finish) 
	  {
		  added_gy = 0;
		  added_gy2 = 0;
		  last_git_y  = 0;

		  wd->pan_y = scroll_y + (git_h) * (top_git-1) ;

		  EINA_INLIST_FOREACH(wd->group_items, git)
			 {
				 edje_object_signal_emit(git->base, "elm,state,alpha,enable", "elm");			
			 }
		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 { 
				 EINA_LIST_FOREACH(itb->items, l, it)
					{
						_item_unselect(it);
					}
			 }


		  wd->pinchzoom_effect_mode = ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE;
		  fprintf(stderr,"ELM_GENLIST_ITEM_MOVE_EFFECT_EXPAND  FINISH   \n");
		  elm_smart_scroller_hold_set(wd->scr, 0);
		  elm_smart_scroller_freeze_set(wd->scr, 0);
		  elm_smart_scroller_freeze_momentum_animator_set(wd->scr, 0);
		  elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, EINA_TRUE);
//		  printf("_group_item_expand_moving_effect_timer_cb  FINISH \n");
		  evas_object_lower(wd->alpha_bg);
		  evas_object_hide(wd->alpha_bg);
		  //	 evas_object_smart_changed(wd->pan_smart);
		  if (wd->calc_job) ecore_job_del(wd->calc_job);
		  wd->calc_job = ecore_job_add(_calc_job, wd);

		  return ECORE_CALLBACK_CANCEL;
	  }      

	return ECORE_CALLBACK_RENEW;
}

	static int
_item_pinch_recalc(Evas_Object *obj, int emode)
{
	Item_Block *itb = NULL;

	Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;
	Widget_Data *wd = elm_widget_data_get(obj);

	if(wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_EXPAND)
	  return EINA_FALSE;

	evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
	evas_output_viewport_get(evas_object_evas_get(wd->pan_smart), &cvx, &cvy, &cvw, &cvh);

	if (emode)
	  {
		  if (wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_NONE)
			 {
				 wd->pinchzoom_effect_mode = ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT;

				 elm_smart_scroller_freeze_momentum_animator_set(wd->scr, 1);
				 elm_smart_scroller_bounce_allow_set(wd->scr, EINA_FALSE, EINA_FALSE);
				 evas_object_raise(wd->alpha_bg);
				 evas_object_show(wd->alpha_bg);

				 wd->effect_start =  ecore_loop_time_get();
				 wd->item_moving_effect_timer = ecore_animator_add(_group_item_contract_moving_effect_timer_cb, obj);
			 }
	  }
	else if (wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH)
	  {
		  elm_smart_scroller_freeze_momentum_animator_set(wd->scr, 1);

		  evas_object_raise(wd->alpha_bg);
		  evas_object_show(wd->alpha_bg);

		  wd->pinchzoom_effect_mode = ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_EXPAND;
		  EINA_INLIST_FOREACH(wd->blocks, itb)
			 {
				 if (ELM_RECTS_INTERSECT(itb->x - wd->pan_x + ox,
							 itb->y - wd->pan_y + oy,
							 itb->w, itb->h,
							 cvx, cvy, cvw, cvh))
					{
						if (itb->realized) _item_block_unrealize(itb);
						itb->realized = EINA_FALSE;
					}
			 }

		  wd->effect_start =  ecore_loop_time_get();
		  wd->item_moving_effect_timer = ecore_animator_add(_group_item_expand_moving_effect_timer_cb, obj);
	  }

	return EINA_TRUE;
}

static Evas_Object*
create_tray_alpha_bg(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   Evas_Object *bg = NULL;
   Evas_Coord ox, oy, ow, oh;
   
   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
	bg  =  evas_object_rectangle_add(evas_object_evas_get(wd->obj));
	evas_object_color_set(bg , 0,0,0,0);
	evas_object_resize(bg , ow, oh);
	evas_object_move(bg , ox, oy);
	evas_object_show(bg );
	evas_object_hide(bg );
	return bg ;
}

static void
_elm_genlist_pinch_zoom_execute(Evas_Object *obj, Eina_Bool emode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->pinch_zoom) return;   

	if(!wd->queue_idler)
	{

      if(!wd->alpha_bg)
	      wd->alpha_bg = create_tray_alpha_bg(obj);

      _item_pinch_recalc(obj, emode);
	}
}


/**
 * Set pinch zoom mode
 * 
 * @param obj The genlist object
 * @param emode 
 * (EINA_TRUE = pinch contract (zoom in), EINA_FALSE = pinch expand (zoom out)
 * 
 * @ingroup Genlist
 */
EAPI void
elm_genlist_pinch_zoom_mode_set(Evas_Object *obj, Eina_Bool emode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->pinch_zoom) return;   

	if(emode)
	  wd->pinch_zoom_reserve = EINA_TRUE;
	else
	  wd->pinch_zoom_reserve = EINA_FALSE;
}

/**
 * Get pinch zoom mode
 * 
 * @param obj The genlist object
 * @return The pinch mode
 * (EINA_TRUE = pinch contract (zoom in), EINA_FALSE = pinch expand (zoom out)
 * 
 * @ingroup Genlist
 */
EAPI Eina_Bool
elm_genlist_pinch_zoom_mode_get(const Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return EINA_FALSE;
	
   if(wd->pinchzoom_effect_mode == ELM_GENLIST_ITEM_PINCHZOOM_EFFECT_CONTRACT_FINISH)
      return EINA_TRUE;
   else 
      return EINA_FALSE;
}

EAPI void
elm_genlist_pinch_zoom_set(Evas_Object *obj, Eina_Bool emode)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return;
	
  	wd->pinch_zoom = emode;

}


// added for item moving animation.
static Eina_Bool
_item_moving_effect_timer_cb(void *data)
{
   Widget_Data *wd = data;
   Item_Block *itb;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;

   if (!wd) return EINA_FALSE;

	Elm_Genlist_Item *it;
	const Eina_List *l;
	int expanded_cnt = 0;
	int cnt = 0;
	static double added_gy =25;
	static int count = 0;

	if(added_gy < 1)
      added_gy = 1;
	
	count++;
   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(wd->pan_smart), &cvx, &cvy, &cvw, &cvh);
   
   EINA_INLIST_FOREACH(wd->blocks, itb)
   {
	itb->w = wd->minw;
	if (ELM_RECTS_INTERSECT(itb->x - wd->pan_x + ox,
				itb->y - wd->pan_y + oy,
				itb->w, itb->h,
				cvx, cvy, cvw, cvh))
	  {
	    EINA_LIST_FOREACH(itb->items, l, it)
	    {
		if (!it->old_scrl_y)
	      it->old_scrl_y  = it->scrl_y;
	      if(it->parent && it->parent->expanded && !it->showme)
	      {
	         evas_object_hide(it->base);
	      }
	      else evas_object_show(it->base);
	      
	      evas_object_show(it->base);

			if(itb && itb->wd->move_effect_mode == ELM_GENLIST_ITEM_MOVE_EFFECT_EXPAND)
			{

		      if(it->old_scrl_y && it->old_scrl_y < it->scrl_y) {

		         it->old_scrl_y += added_gy;
	                  
		      }

	         if(it->old_scrl_y >= it->scrl_y) {
			     it->list_expanded = 1;
			     it->old_scrl_y = it->scrl_y;
	         }

			}

			else if(itb->wd->move_effect_mode == ELM_GENLIST_ITEM_MOVE_EFFECT_CONTRACT)
			{
	         if(it->old_scrl_y && it->old_scrl_y > it->scrl_y) 
	         {

		         it->old_scrl_y -= added_gy;
	            }

	         if(it->old_scrl_y <= it->scrl_y) {
			     it->list_expanded = 1;
			     it->old_scrl_y = it->scrl_y;
	         }
	         
			}
			expanded_cnt += it->list_expanded;
			cnt++;

			_move_edit_controls( it,it->scrl_x, it->scrl_y );
			evas_object_resize(it->base, it->w-(it->pad_left+it->pad_right), it->h);

			evas_object_move(it->base, it->scrl_x+it->pad_left, it->old_scrl_y);

			evas_object_raise(it->base);
	    }
	}
   }
	if(expanded_cnt == cnt) {

		if(wd->item_moving_effect_timer) {
         added_gy = 25;
         count = 0;
         wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_NONE;
          EINA_INLIST_FOREACH(wd->blocks, itb)
			   {
			    EINA_LIST_FOREACH(itb->items, l, it)
			    {
					it->list_expanded = 0;
			    }
          }
			
    	}
       return ECORE_CALLBACK_CANCEL;
    }
 	return ECORE_CALLBACK_RENEW;
}

// added for edit mode item moving animation.
static Eina_Bool
_edit_mode_item_moving_effect_cb(void *data)
{
	Widget_Data *wd = data;
	Item_Block *itb;
	Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;

	if (!wd) return EINA_FALSE;

	Elm_Genlist_Item *it, *select_all_item;
	const Eina_List *l;
	int expanded_cnt = 0;
	static double added_gy = 9;
	static float count = 0;
	int finish = 0;

	static float select_all_y = 0;

	count++;

	if(!wd->select_all_item)
	  return ECORE_CALLBACK_CANCEL;


	static float dy = 9;
	dy -= 0.5;
	if(dy < 1.0)
	  dy = 1.0f;


	select_all_y += (dy);

	evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
	evas_output_viewport_get(evas_object_evas_get(wd->pan_smart), &cvx, &cvy, &cvw, &cvh);


	EINA_INLIST_FOREACH(wd->blocks, itb)
	  {
		  itb->w = wd->minw;
		  if (ELM_RECTS_INTERSECT(itb->x - wd->pan_x + ox,
					  itb->y - wd->pan_y + oy,
					  itb->w, itb->h,
					  cvx, cvy, cvw, cvh))

			 {
				 EINA_LIST_FOREACH(itb->items, l, it)
					{

						evas_object_show(it->base);
						if(it->old_pad_left < it->pad_left) {

							  it->old_pad_left += dy;

							  if(it->old_pad_left >= it->pad_left) {
									 it->list_expanded = 1;
									 it->old_pad_left = it->pad_left;
							  }

						}

						if(it->old_pad_left > it->pad_left) 
						  {
							  it->old_pad_left -= dy;
							  if(it->old_pad_left <= it->pad_left) {
									 it->list_expanded = 1;
									 it->old_pad_left = it->pad_left;
							  }

						  }


						if(it->old_scrl_y < it->scrl_y) {

							  it->old_scrl_y += (dy) ;
							  if(it->old_scrl_y >= it->scrl_y) {
									 it->list_expanded = 1;
									 it->old_scrl_y = it->scrl_y;
							  }

						}

						if(it->old_scrl_y > it->scrl_y) 
						  {
							  it->old_scrl_y -= (dy);               

							  if(it->old_scrl_y <= it->scrl_y) {
									 it->list_expanded = 1;
									 it->old_scrl_y = it->scrl_y;
							  }

						  }

						select_all_item = itb->wd->select_all_item;
						evas_object_resize(select_all_item->base, itb->w, select_all_item->h);  
						if(wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE)
						  evas_object_move(select_all_item->base, ox, oy - select_all_y);
						else if( select_all_y <=  select_all_item->h)
						  evas_object_move(select_all_item->base, ox, oy - select_all_item->h + select_all_y);
						evas_object_raise(select_all_item->base);

						evas_object_show(select_all_item->base);


						expanded_cnt += it->list_expanded;

						evas_object_move(it->base, it->scrl_x+it->old_pad_left, it->old_scrl_y);
						evas_object_move(it->edit_obj, it->scrl_x, it->old_scrl_y);
						evas_object_raise(it->edit_obj);
						evas_object_raise(it->base);

						evas_object_show(it->edit_obj);
						evas_object_show(it->base);         

						if(select_all_y >= wd->select_all_item->h && it->list_expanded == 1) 
						  {
							  finish = 1;         
							  evas_object_raise(it->edit_obj);            
							  break;
						  }

					}
			 }
	  }

	if(finish) {

		  if(wd->item_moving_effect_timer) {
				 added_gy = 9;
				 count = 0;
				 select_all_y = 0;
				 dy = 9;
				 wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_NONE;
				 EINA_INLIST_FOREACH(wd->blocks, itb)
					{
						EINA_LIST_FOREACH(itb->items, l, it)
						  {
							  it->list_expanded = 0;
						  }
					}
				 if(wd->edit_mode == ELM_GENLIST_EDIT_MODE_NONE)
					{
						if (wd->select_all_item)
						  {
							  if (wd->select_all_item->realized) _item_unrealize(wd->select_all_item);
							  free(wd->select_all_item);
						  }
						wd->select_all_item = NULL;         
					}

		  }
		  wd->move_effect_mode = ELM_GENLIST_ITEM_MOVE_EFFECT_NONE;
		  wd->effect_mode = 0;

		  if (wd->calc_job) ecore_job_del(wd->calc_job);
		  wd->calc_job = ecore_job_add(_calc_job, wd);      
		  return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;
}


// added for item moving animation.
static int
_item_flip_effect_show(void *data)
{
   Widget_Data *wd = data;
   Item_Block *itb;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;

   if (!wd) return EINA_FALSE;

	Elm_Genlist_Item *it;
	Evas_Object *base ;
	const Eina_List *l;
	int start = 0, end = 0;
	
   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(wd->pan_smart), &cvx, &cvy, &cvw, &cvh);


	EINA_INLIST_FOREACH(wd->blocks, itb)
	  {
		  itb->w = wd->minw;
		  if (ELM_RECTS_INTERSECT(itb->x - wd->pan_x + ox,
					  itb->y - wd->pan_y + oy,
					  itb->w, itb->h,
					  cvx, cvy, cvw, cvh))
			 {
				 EINA_LIST_FOREACH(itb->items, l, it)
					{
						if(it->parent && it->wd->expand_item == it->parent && !it->effect_done)
						  {
							  if(!start)
								 start = it->scrl_y;
							  base = (Evas_Object *) it->base;
							  edje_object_signal_emit(it->base, "flip_item", "");
							  end = it->scrl_y + it->h;
							  it->effect_done = EINA_TRUE;


						  }
					}
			 }
	  }

	wd->expand_item_cnt = end - start;
	return ECORE_CALLBACK_CANCEL;
}

EAPI void
elm_genlist_effect_set(const Evas_Object *obj, Eina_Bool emode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->effect_mode = emode;
   wd->point_rect = evas_object_rectangle_add(evas_object_evas_get(wd->obj));
	evas_object_resize(wd->point_rect, 10, 25);
	evas_object_color_set(wd->point_rect, 255, 0, 0, 130);   
   evas_object_show(wd->point_rect);
   evas_object_hide(wd->point_rect);
}

/*
EAPI void
elm_genlist_edit_mode_effect_set(const Evas_Object *obj, Eina_Bool emode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->edit_mode_effect_mode = emode;
}
*/

EAPI void
elm_genlist_queue_exception_set(const Evas_Object *obj, Eina_Bool emode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->queue_exception = emode;

   fprintf(stderr, "=================> Caution!!! <========================\n");
   fprintf(stderr, "==> elm_genlist_queue_exception_set() is for demo. <===\n");
   fprintf(stderr, "==> Do not use this API                             <==\n");
   fprintf(stderr, "=======================================================\n");
}
