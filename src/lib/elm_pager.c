#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Pager Pager
 *
 * The pager is an object that allows flipping (with animation) between 1 or
 * more “pages” of objects, much like a stack of windows within the window.
 *
 * Objects can be pushed or popped from the stack or deleted as normal.
 * Pushes and pops will animate (and a pop will delete the object once the
 * animation is finished). Any object in the pager can be promoted to the top
 * (from its current stacking position) as well. Objects are pushed to the
 * top with elm_pager_content_push() and when the top item is no longer
 * wanted, simply pop it with elm_pager_content_pop() and it will also be
 * deleted. Any object you wish to promote to the top that is already in the
 * pager, simply use elm_pager_content_promote(). If an object is no longer
 * needed and is not the top item, just delete it as normal. You can query
 * which objects are the top and bottom with elm_pager_content_bottom_get()
 * and elm_pager_content_top_get().
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _Item Item;

struct _Widget_Data
{
   Eina_List *stack;
   Item *top, *oldtop;
   Evas_Object *rect, *clip;
};

struct _Item
{
   Evas_Object *obj, *base, *content;
   Evas_Coord minw, minh;
   Eina_Bool popme : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
        edje_object_mirrored_set(it->base, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Item *it;
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
        _elm_theme_object_set(obj, it->base,  "pager", "base",
                              elm_widget_style_get(obj));
        edje_object_scale_set(it->base, elm_widget_scale_get(obj) *
                              _elm_config->scale);
     }
   _sizing_eval(obj);
}

static Eina_Bool
_elm_pager_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *cur;

   if ((!wd) || (!wd->top))
     return EINA_FALSE;

   cur = wd->top->content;

   /* Try Focus cycle in subitem */
   return elm_widget_focus_next_get(cur, dir, next);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Eina_List *l;
   Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
	if (it->minw > minw) minw = it->minw;
	if (it->minh > minh) minh = it->minh;
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Item *it = data;
   Evas_Coord minw = -1, minh = -1;
   evas_object_size_hint_min_get(it->content, &minw, &minh);
   // FIXME: why is this needed? how does edje get this unswallowed or
   // lose its callbacks to edje
   edje_object_part_swallow(it->base, "elm.swallow.content", it->content);
   edje_object_size_min_calc(it->base, &it->minw, &it->minh);
   _sizing_eval(it->obj);
}

static void
_eval_top(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Item *ittop;
   if (!wd) return;
   if (!wd->stack) return;
   ittop = eina_list_last(wd->stack)->data;
   if (ittop != wd->top)
     {
	Evas_Object *o;
	const char *onshow, *onhide;

	if (wd->top)
	  {
	     o = wd->top->base;
	     if (wd->top->popme)
		edje_object_signal_emit(o, "elm,action,pop", "elm");
	     else
		edje_object_signal_emit(o, "elm,action,hide", "elm");
	     onhide = edje_object_data_get(o, "onhide");
	     if (onhide)
	       {
		  if (!strcmp(onhide, "raise")) evas_object_raise(o);
		  else if (!strcmp(onhide, "lower")) evas_object_lower(o);
	       }
	  }
	wd->oldtop = wd->top;
	wd->top = ittop;
	o = wd->top->base;
	evas_object_show(o);
        if (wd->oldtop)
          {
             if (elm_object_focus_get(wd->oldtop->content))
               elm_object_focus(wd->top->content);
             if (wd->oldtop->popme)
               edje_object_signal_emit(o, "elm,action,show", "elm");
             else
               edje_object_signal_emit(o, "elm,action,push", "elm");
          }
        else
          edje_object_signal_emit(o, "elm,action,push", "elm");
	onshow = edje_object_data_get(o, "onshow");
	if (onshow)
	  {
	     if (!strcmp(onshow, "raise")) evas_object_raise(o);
	     else if (!strcmp(onshow, "lower")) evas_object_lower(o);
	  }
     }
}

static void
_move(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y;
   Eina_List *l;
   Item *it;
   if (!wd) return;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   EINA_LIST_FOREACH(wd->stack, l, it)
     evas_object_move(it->base, x, y);
}

static void
_sub_del(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Object *sub = event_info;
   Eina_List *l;
   Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
	if (it->content == sub)
	  {
	     wd->stack = eina_list_remove_list(wd->stack, l);
	     evas_object_event_callback_del_full
               (sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, it);
	     evas_object_del(it->base);
	     _eval_top(it->obj);
	     free(it);
	     return;
	  }
     }
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{   
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord w, h;
   Eina_List *l;
   Item *it;
   if (!wd) return;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   EINA_LIST_FOREACH(wd->stack, l, it) evas_object_resize(it->base, w, h);
}

static void
_signal_hide_finished(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Item *it = data;
   Evas_Object *obj2 = it->obj;
   evas_object_hide(it->base);
   edje_object_signal_emit(it->base, "elm,action,reset", "elm");
   evas_object_smart_callback_call(obj2, "hide,finished", it->content);
   edje_object_message_signal_process(it->base);
   evas_object_hide(it->content);
   if (it->popme) evas_object_del(it->content);
   _sizing_eval(obj2);
}

/**
 * Add a new pager to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Pager
 */
EAPI Evas_Object *
elm_pager_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
   
   ELM_SET_WIDTYPE(widtype, "pager");
   elm_widget_type_set(obj, "pager");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_focus_next_hook_set(obj, _elm_pager_focus_next_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   wd->clip = evas_object_rectangle_add(e);
   elm_widget_resize_object_set(obj, wd->clip);
   elm_widget_sub_object_add(obj, wd->clip);

   wd->rect = evas_object_rectangle_add(e);
   elm_widget_sub_object_add(obj, wd->rect);
   evas_object_color_set(wd->rect, 255, 255, 255, 0); 
   evas_object_clip_set(wd->rect, wd->clip);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);
   return obj;
}

/**
 * Push an object to the top of the pager stack (and show it)
 *
 * The object pushed becomes a child of the pager and will be controlled
 * it and deleted when the pager is deleted.
 *
 * @param obj The pager object
 * @param content The object to push
 *
 * @ingroup Pager
 */
EAPI void
elm_pager_content_push(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Item *it = ELM_NEW(Item);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   if (!it) return;
   it->obj = obj;
   it->content = content;
   it->base = edje_object_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(it->base, obj);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(it->base, x, y);
   evas_object_resize(it->base, w, h);
   evas_object_clip_set(it->base, wd->clip);
   elm_widget_sub_object_add(obj, it->base);
   elm_widget_sub_object_add(obj, it->content);
   _elm_theme_object_set(obj, it->base,  "pager", "base", elm_widget_style_get(obj));
   edje_object_signal_callback_add(it->base, "elm,action,hide,finished", "", 
                                   _signal_hide_finished, it);
   evas_object_event_callback_add(it->content,
                                  EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, it);
   edje_object_part_swallow(it->base, "elm.swallow.content", it->content);
   edje_object_size_min_calc(it->base, &it->minw, &it->minh);
   evas_object_data_set(it->base, "_elm_leaveme", obj);
   evas_object_show(it->content);
   wd->stack = eina_list_append(wd->stack, it);
   _eval_top(obj);
   _sizing_eval(obj);
}

/**
 * Pop the object that is on top of the stack
 *
 * This pops the object that is on top (visible) in the pager, makes it
 * disappear, then deletes the object. The object that was underneath it
 * on the stack will become visible.
 *
 * @param obj The pager object
 *
 * @ingroup Pager
 */
EAPI void
elm_pager_content_pop(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   if (!wd) return;
   if (!wd->stack) return;
   it = eina_list_last(wd->stack)->data;
   it->popme = EINA_TRUE;
   ll = eina_list_last(wd->stack);
   if (ll)
     {
	ll = ll->prev;
	if (!ll)
	  {
	     Evas_Object *o;
	     const char *onhide;

	     wd->top = it;
	     o = wd->top->base;
	     edje_object_signal_emit(o, "elm,action,pop", "elm");
	     onhide = edje_object_data_get(o, "onhide");
	     if (onhide)
	       {
		  if (!strcmp(onhide, "raise")) evas_object_raise(o);
		  else if (!strcmp(onhide, "lower")) evas_object_lower(o);
	       }
	     wd->top = NULL;
	  }
	else
	  {
	     it = ll->data;
	     elm_pager_content_promote(obj, it->content);
	  }
     }
}

/**
 * Promote an object already in the pager stack to the top of the stack
 *
 * This will take the indicated object and promote it to the top of the stack
 * as if it had been pushed there. The object must already be inside the
 * pager stack to work.
 *
 * @param obj The pager object
 * @param content The object to promote
 *
 * @ingroup Pager
 */
EAPI void
elm_pager_content_promote(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
	if (it->content == content)
	  {
	     wd->stack = eina_list_remove_list(wd->stack, l);
	     wd->stack = eina_list_append(wd->stack, it);
	     _eval_top(obj);
	     return;
	  }
     }
}

/**
 * Return the object at the bottom of the pager stack
 *
 * @param obj The pager object
 * @return The bottom object or NULL if none
 *
 * @ingroup Pager
 */
EAPI Evas_Object *
elm_pager_content_bottom_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Item *it;
   if (!wd) return NULL;
   if (!wd->stack) return NULL;
   it = wd->stack->data;
   return it->content;
}

/**
 * Return the object at the top of the pager stack
 *
 * @param obj The pager object
 * @return The top object or NULL if none
 *
 * @ingroup Pager
 */
EAPI Evas_Object *
elm_pager_content_top_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!wd->top) return NULL;
   return wd->top->content;
}

