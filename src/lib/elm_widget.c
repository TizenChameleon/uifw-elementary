#include <Elementary.h>
#include "elm_priv.h"

static const char SMART_NAME[] = "elm_widget";

#define API_ENTRY \
   Smart_Data *sd = evas_object_smart_data_get(obj); \
   if ((!obj) || (!sd) || (!_elm_widget_is(obj)))
#define INTERNAL_ENTRY \
   Smart_Data *sd = evas_object_smart_data_get(obj); \
   if (!sd) return;

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
   Evas_Object   *obj;
   const char    *type;
   Evas_Object   *parent_obj;
   Evas_Coord     x, y, w, h;
   Eina_List     *subobjs;
   Evas_Object   *resize_obj;
   Evas_Object   *hover_obj;
   void         (*del_func) (Evas_Object *obj);
   void         (*del_pre_func) (Evas_Object *obj);
   void         (*focus_func) (Evas_Object *obj);
   void         (*activate_func) (Evas_Object *obj);
   void         (*disable_func) (Evas_Object *obj);
   void         (*theme_func) (Evas_Object *obj);
   void         (*changed_func) (Evas_Object *obj);
   void         (*on_focus_func) (void *data, Evas_Object *obj);
   void          *on_focus_data;
   void         (*on_change_func) (void *data, Evas_Object *obj);
   void          *on_change_data;
   void         (*on_show_region_func) (void *data, Evas_Object *obj);
   void          *on_show_region_data;
   void          *data;
   Evas_Coord     rx, ry, rw, rh;
   int            scroll_hold;
   int            scroll_freeze;
   double         scale;
   Elm_Theme     *theme;
   const char    *style;
   
   int            child_drag_x_locked;
   int            child_drag_y_locked;
   Eina_Bool      drag_x_locked : 1;
   Eina_Bool      drag_y_locked : 1;
   
   Eina_Bool      can_focus : 1;
   Eina_Bool      child_can_focus : 1;
   Eina_Bool      focused : 1;
   Eina_Bool      disabled : 1;
};

/* local subsystem functions */
static void _smart_reconfigure(Smart_Data *sd);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_calculate(Evas_Object *obj);
static void _smart_init(void);
static inline Eina_Bool _elm_widget_is(const Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

static void
_sub_obj_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Smart_Data *sd = data;

   if (obj == sd->resize_obj)
     sd->resize_obj = NULL;
   else if (obj == sd->hover_obj)
     sd->hover_obj = NULL;
   else
     sd->subobjs = eina_list_remove(sd->subobjs, obj);
   evas_object_smart_callback_call(sd->obj, "sub-object-del", obj);
}

static void
_sub_obj_mouse_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *o = obj;
   do 
     {
        if (_elm_widget_is(o)) break;
        o = evas_object_smart_parent_get(o);
     }
   while (o);
   if (!o) return;
   if (!elm_widget_can_focus_get(o)) return;
   elm_widget_focus_steal(o);
}

EAPI Evas_Object *
elm_widget_add(Evas *evas)
{
   _smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
elm_widget_del_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->del_func = func;
}

EAPI void
elm_widget_del_pre_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->del_pre_func = func;
}

EAPI void
elm_widget_focus_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->focus_func = func;
}

EAPI void
elm_widget_activate_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->activate_func = func;
}

EAPI void
elm_widget_disable_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->disable_func = func;
}

EAPI void
elm_widget_theme_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->theme_func = func;
}

EAPI void
elm_widget_changed_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->changed_func = func;
}

EAPI void
elm_widget_theme(Evas_Object *obj)
{
   const Eina_List *l;
   Evas_Object *child;

   API_ENTRY return;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     elm_widget_theme(child);
   if (sd->resize_obj) elm_widget_theme(sd->resize_obj);
   if (sd->hover_obj) elm_widget_theme(sd->hover_obj);
   if (sd->theme_func) sd->theme_func(obj);
}

EAPI void
elm_widget_on_focus_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data)
{
   API_ENTRY return;
   sd->on_focus_func = func;
   sd->on_focus_data = data;
}

EAPI void
elm_widget_on_change_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data)
{
   API_ENTRY return;
   sd->on_change_func = func;
   sd->on_change_data = data;
}

EAPI void
elm_widget_on_show_region_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data)
{
   API_ENTRY return;
   sd->on_show_region_func = func;
   sd->on_show_region_data = data;
}

EAPI void
elm_widget_data_set(Evas_Object *obj, void *data)
{
   API_ENTRY return;
   sd->data = data;
}

EAPI void *
elm_widget_data_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->data;
}

EAPI void
elm_widget_sub_object_add(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   double scale, pscale = elm_widget_scale_get(sobj);
   Elm_Theme *th, *pth = elm_widget_theme_get(sobj);

   sd->subobjs = eina_list_append(sd->subobjs, sobj);
   if (!sd->child_can_focus)
     {
	if (elm_widget_can_focus_get(sobj)) sd->child_can_focus = 1;
     }
   if (_elm_widget_is(sobj))
     {
	Smart_Data *sd2 = evas_object_smart_data_get(sobj);
	if (sd2)
	  {
	     if (sd2->parent_obj)
               elm_widget_sub_object_del(sd2->parent_obj, sobj);
	     sd2->parent_obj = obj;
	  }
     }
   evas_object_data_set(sobj, "elm-parent", obj);
   evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
   evas_object_smart_callback_call(obj, "sub-object-add", sobj);
   scale = elm_widget_scale_get(sobj);
   th = elm_widget_theme_get(sobj);
   if ((scale != pscale) || (th != pth)) elm_widget_theme(sobj);
}

EAPI void
elm_widget_sub_object_del(Evas_Object *obj, Evas_Object *sobj)
{
   Evas_Object *sobj_parent;
   API_ENTRY return;
   if (!sobj) return;

   sobj_parent = evas_object_data_del(sobj, "elm-parent");
   if (sobj_parent != obj)
     {
	static int abort_on_warn = -1;
	ERR("removing sub object %p from parent %p, "
	    "but elm-parent is different %p!",
	    sobj, obj, sobj_parent);
	if (EINA_UNLIKELY(abort_on_warn == -1))
	  {
	     if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
	     else abort_on_warn = 0;
	  }
	if (abort_on_warn == 1) abort();
     }
   sd->subobjs = eina_list_remove(sd->subobjs, sobj);
   if (!sd->child_can_focus)
     {
	if (elm_widget_can_focus_get(sobj)) sd->child_can_focus = 0;
     }
   if (_elm_widget_is(sobj))
     {
	Smart_Data *sd2 = evas_object_smart_data_get(sobj);
	if (sd2) sd2->parent_obj = NULL;
     }
   evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
   evas_object_smart_callback_call(obj, "sub-object-del", sobj);
}

EAPI void
elm_widget_resize_object_set(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   if (sd->resize_obj)
     {
	evas_object_data_del(sd->resize_obj, "elm-parent");
	if (_elm_widget_is(sd->resize_obj))
	  {
	     Smart_Data *sd2 = evas_object_smart_data_get(sd->resize_obj);
	     if (sd2) sd2->parent_obj = NULL;
	  }
	evas_object_event_callback_del_full(sd->resize_obj, EVAS_CALLBACK_DEL,
           _sub_obj_del, sd);
	evas_object_event_callback_del_full(sd->resize_obj, EVAS_CALLBACK_MOUSE_DOWN,
           _sub_obj_mouse_down, sd);
	evas_object_smart_member_del(sd->resize_obj);
     }
   sd->resize_obj = sobj;
   if (sd->resize_obj)
     {
	if (_elm_widget_is(sd->resize_obj))
	  {
	     Smart_Data *sd2 = evas_object_smart_data_get(sd->resize_obj);
	     if (sd2) sd2->parent_obj = obj;
	  }
	evas_object_clip_set(sobj, evas_object_clip_get(obj));
	evas_object_smart_member_add(sobj, obj);
	evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
	evas_object_event_callback_add(sobj, EVAS_CALLBACK_MOUSE_DOWN,
                                       _sub_obj_mouse_down, sd);
	_smart_reconfigure(sd);
	evas_object_data_set(sobj, "elm-parent", obj);
	evas_object_smart_callback_call(obj, "sub-object-add", sobj);
     }
}

EAPI void
elm_widget_hover_object_set(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   if (sd->hover_obj)
     {
	evas_object_event_callback_del_full(sd->hover_obj, EVAS_CALLBACK_DEL,
           _sub_obj_del, sd);
     }
   sd->hover_obj = sobj;
   if (sd->hover_obj)
     {
	evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
	_smart_reconfigure(sd);
     }
}

EAPI void
elm_widget_can_focus_set(Evas_Object *obj, int can_focus)
{
   API_ENTRY return;
   sd->can_focus = can_focus;
}

EAPI int
elm_widget_can_focus_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   if (sd->can_focus) return 1;
   if (sd->child_can_focus) return 1;
   return 0;
}

EAPI int
elm_widget_focus_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->focused;
}

EAPI Evas_Object *
elm_widget_focused_object_get(const Evas_Object *obj)
{
   const Evas_Object *subobj;
   const Eina_List *l;
   API_ENTRY return NULL;

   if (!sd->focused) return NULL;
   EINA_LIST_FOREACH(sd->subobjs, l, subobj)
     {
	Evas_Object *fobj = elm_widget_focused_object_get(subobj);
	if (fobj) return fobj;
     }
   return (Evas_Object *)obj;
}

EAPI Evas_Object *
elm_widget_top_get(const Evas_Object *obj)
{
#if 1 // strict way  
   API_ENTRY return NULL;
   if (sd->parent_obj) return elm_widget_top_get(sd->parent_obj);
   return (Evas_Object *)obj;
#else // loose way
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Evas_Object *par;
   
   if (!obj) return NULL;
   if ((sd) && _elm_widget_is(obj))
     {
        if ((sd->type) && (!strcmp(sd->type, "win"))) 
          return (Evas_Object *)obj;
        if (sd->parent_obj)
          return elm_widget_top_get(sd->parent_obj);
     }
   par = evas_object_smart_parent_get(obj);
   if (!par) return (Evas_Object *)obj;
   return elm_widget_top_get(par);
#endif   
}

EAPI Eina_Bool
elm_widget_is(const Evas_Object *obj)
{
   return _elm_widget_is(obj);
}

EAPI Evas_Object *
elm_widget_parent_widget_get(const Evas_Object *obj)
{
   Evas_Object *parent;

   if (_elm_widget_is(obj))
     {
	Smart_Data *sd = evas_object_smart_data_get(obj);
	if (!sd) return NULL;
	parent = sd->parent_obj;
     }
   else
     {
	parent = evas_object_data_get(obj, "elm-parent");
	if (!parent)
	  parent = evas_object_smart_data_get(obj);
     }

   while (parent)
     {
	Evas_Object *elm_parent;
	if (_elm_widget_is(parent)) break;
	elm_parent = evas_object_data_get(parent, "elm-parent");
	if (elm_parent)
	  parent = elm_parent;
	else
	  parent = evas_object_smart_parent_get(parent);
     }
   return parent;
}

EAPI int
elm_widget_focus_jump(Evas_Object *obj, int forward)
{
   API_ENTRY return 0;
   if (!elm_widget_can_focus_get(obj)) return 0;

   /* if it has a focus func its an end-point widget like a button */
   if (sd->focus_func)
     {
	if (!sd->focused) sd->focused = 1;
	else sd->focused = 0;
	if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
	sd->focus_func(obj);
	return sd->focused;
     }
   /* its some container */
   else
     {
	int focus_next;
	int noloop = 0;

	focus_next = 0;
	if (!sd->focused)
	  {
	     elm_widget_focus_set(obj, forward);
	     sd->focused = 1;
	     if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
	     return 1;
	  }
	else
	  {
	     if (forward)
	       {
		  if (elm_widget_can_focus_get(sd->resize_obj))
		    {
		       if ((focus_next) &&
			   (!elm_widget_disabled_get(sd->resize_obj)))
			 {
			    /* the previous focused item was unfocused - so focus
			     * the next one (that can be focused) */
			    if (elm_widget_focus_jump(sd->resize_obj, forward))
                              return 1;
			    else noloop = 1;
			 }
		       else
			 {
			    if (elm_widget_focus_get(sd->resize_obj))
			      {
				 /* jump to the next focused item or focus this item */
				 if (elm_widget_focus_jump(sd->resize_obj, forward))
                                   return 1;
				 /* it returned 0 - it got to the last item and is past it */
				 focus_next = 1;
			      }
			 }
		    }
		  if (!noloop)
		    {
		       const Eina_List *l;
		       Evas_Object *child;
		       EINA_LIST_FOREACH(sd->subobjs, l, child)
			 {
			    if (elm_widget_can_focus_get(child))
			      {
				 if ((focus_next) &&
				     (!elm_widget_disabled_get(child)))
				   {
				      /* the previous focused item was unfocused - so focus
				       * the next one (that can be focused) */
				      if (elm_widget_focus_jump(child, forward))
                                        return 1;
				      else break;
				   }
				 else
				   {
				      if (elm_widget_focus_get(child))
					{
					   /* jump to the next focused item or focus this item */
					   if (elm_widget_focus_jump(child, forward))
                                             return 1;
					   /* it returned 0 - it got to the last item and is past it */
					   focus_next = 1;
					}
				   }
			      }
			 }
		    }
	       }
	     else
	       {
		  const Eina_List *l;
		  Evas_Object *child;

		  EINA_LIST_REVERSE_FOREACH(sd->subobjs, l, child)
		    {
		       if (elm_widget_can_focus_get(child))
			 {
			    if ((focus_next) &&
				(!elm_widget_disabled_get(child)))
			      {
				 /* the previous focused item was unfocused - so focus
				  * the next one (that can be focused) */
				 if (elm_widget_focus_jump(child, forward))
                                   return 1;
				 else break;
			      }
			    else
			      {
				 if (elm_widget_focus_get(child))
				   {
				      /* jump to the next focused item or focus this item */
				      if (elm_widget_focus_jump(child, forward))
                                        return 1;
				      /* it returned 0 - it got to the last item and is past it */
				      focus_next = 1;
				   }
			      }
			 }
		    }
		  if (!l)
		    {
		       if (elm_widget_can_focus_get(sd->resize_obj))
			 {
			    if ((focus_next) &&
				(!elm_widget_disabled_get(sd->resize_obj)))
			      {
				 /* the previous focused item was unfocused - so focus
				  * the next one (that can be focused) */
				 if (elm_widget_focus_jump(sd->resize_obj, forward))
                                   return 1;
			      }
			    else
			      {
				 if (elm_widget_focus_get(sd->resize_obj))
				   {
				      /* jump to the next focused item or focus this item */
				      if (elm_widget_focus_jump(sd->resize_obj, forward))
                                        return 1;
				      /* it returned 0 - it got to the last item and is past it */
				      focus_next = 1;
				   }
			      }
			 }
		    }
	       }
	  }
     }
   /* no next item can be focused */
   if (sd->focused)
     {
	sd->focused = 0;
	if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
     }
   return 0;
}

EAPI void
elm_widget_focus_set(Evas_Object *obj, int first)
{
   API_ENTRY return;
   if (!sd->focused)
     {
	sd->focused = 1;
	if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
     }
   if (sd->focus_func)
     {
	sd->focus_func(obj);
	return;
     }
   else
     {
	if (first)
	  {
	     if ((elm_widget_can_focus_get(sd->resize_obj)) &&
		 (!elm_widget_disabled_get(sd->resize_obj)))
	       {
		  elm_widget_focus_set(sd->resize_obj, first);
	       }
	     else
	       {
		  const Eina_List *l;
		  Evas_Object *child;
		  EINA_LIST_FOREACH(sd->subobjs, l, child)
		    {
		       if ((elm_widget_can_focus_get(child)) &&
			   (!elm_widget_disabled_get(child)))
			 {
			    elm_widget_focus_set(child, first);
			    break;
			 }
		    }
	       }
	  }
	else
	  {
	     const Eina_List *l;
	     Evas_Object *child;
	     EINA_LIST_REVERSE_FOREACH(sd->subobjs, l, child)
	       {
		  if ((elm_widget_can_focus_get(child)) &&
		      (!elm_widget_disabled_get(child)))
		    {
		       elm_widget_focus_set(child, first);
		       break;
		    }
	       }
	     if (!l)
	       {
		  if ((elm_widget_can_focus_get(sd->resize_obj)) &&
		      (!elm_widget_disabled_get(sd->resize_obj)))
		    {
		       elm_widget_focus_set(sd->resize_obj, first);
		    }
	       }
	  }
     }
}

EAPI Evas_Object *
elm_widget_parent_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->parent_obj;
}

EAPI void
elm_widget_focused_object_clear(Evas_Object *obj)
{
   API_ENTRY return;
   if (!sd->focused) return;
   if (elm_widget_focus_get(sd->resize_obj))
     elm_widget_focused_object_clear(sd->resize_obj);
   else
     {
	const Eina_List *l;
	Evas_Object *child;
	EINA_LIST_FOREACH(sd->subobjs, l, child)
	  {
	     if (elm_widget_focus_get(child))
	       {
		  elm_widget_focused_object_clear(child);
		  break;
	       }
	  }
     }
   sd->focused = 0;
   if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
   if (sd->focus_func) sd->focus_func(obj);
}

static void
_elm_widget_parent_focus(Evas_Object *obj)
{
   API_ENTRY return;
   Evas_Object *o = elm_widget_parent_get(obj);

   if (sd->focused) return;
   if (o) _elm_widget_parent_focus(o);
   sd->focused = 1;
   if (sd->on_focus_func) sd->on_focus_func(sd->on_focus_data, obj);
   if (sd->focus_func) sd->focus_func(obj);
}

EAPI void
elm_widget_focus_steal(Evas_Object *obj)
{
   Evas_Object *parent, *o;
   API_ENTRY return;

   if (sd->focused) return;
   if (sd->disabled) return;
   parent = obj;
   for (;;)
     {
	o = elm_widget_parent_get(parent);
	if (!o) break;
	sd = evas_object_smart_data_get(o);
	if (sd->focused) break;
	parent = o;
     }
   if (!elm_widget_parent_get(parent))
     elm_widget_focused_object_clear(parent);
   else
     {
	parent = elm_widget_parent_get(parent);
	sd = evas_object_smart_data_get(parent);
	if (elm_widget_focus_get(sd->resize_obj))
          {
             elm_widget_focused_object_clear(sd->resize_obj);
          }
	else
	  {
	     const Eina_List *l;
	     Evas_Object *child;
	     EINA_LIST_FOREACH(sd->subobjs, l, child)
	       {
		  if (elm_widget_focus_get(child))
		    {
		       elm_widget_focused_object_clear(child);
		       break;
		    }
	       }
	  }
     }
   _elm_widget_parent_focus(obj);
   return;
}

EAPI void
elm_widget_activate(Evas_Object *obj)
{
   API_ENTRY return;
   elm_widget_change(obj);
   if (sd->activate_func) sd->activate_func(obj);
}

EAPI void
elm_widget_change(Evas_Object *obj)
{
   API_ENTRY return;
   elm_widget_change(elm_widget_parent_get(obj));
   if (sd->on_change_func) sd->on_change_func(sd->on_change_data, obj);
}

EAPI void
elm_widget_disabled_set(Evas_Object *obj, int disabled)
{
   API_ENTRY return;

   if (sd->disabled == disabled) return;
   sd->disabled = disabled;
   if (sd->focused)
     {
	Evas_Object *o, *parent;

	parent = obj;
	for (;;)
	  {
	     o = elm_widget_parent_get(parent);
	     if (!o) break;
	     parent = o;
	  }
	elm_widget_focus_jump(parent, 1);
     }
   if (sd->disable_func) sd->disable_func(obj);
}

EAPI int
elm_widget_disabled_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->disabled;
}

EAPI void
elm_widget_show_region_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   API_ENTRY return;
   if ((x == sd->rx) && (y == sd->ry) && (w == sd->rw) && (h == sd->rh)) return;
   sd->rx = x;
   sd->ry = y;
   sd->rw = w;
   sd->rh = h;
   if (sd->on_show_region_func)
     sd->on_show_region_func(sd->on_show_region_data, obj);
}

EAPI void
elm_widget_show_region_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   if (x) *x = sd->rx;
   if (y) *y = sd->ry;
   if (w) *w = sd->rw;
   if (h) *h = sd->rh;
}

EAPI void
elm_widget_scroll_hold_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold++;
   if (sd->scroll_hold == 1)
     evas_object_smart_callback_call(obj, "scroll-hold-on", obj);
   if (sd->parent_obj) elm_widget_scroll_hold_push(sd->parent_obj);
   // FIXME: on delete/reparent hold pop
}

EAPI void
elm_widget_scroll_hold_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold--;
   if (sd->scroll_hold < 0) sd->scroll_hold = 0;
   if (sd->scroll_hold == 0)
     evas_object_smart_callback_call(obj, "scroll-hold-off", obj);
   if (sd->parent_obj) elm_widget_scroll_hold_pop(sd->parent_obj);
}

EAPI int
elm_widget_scroll_hold_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_hold;
}

EAPI void
elm_widget_scroll_freeze_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze++;
   if (sd->scroll_freeze == 1)
     evas_object_smart_callback_call(obj, "scroll-freeze-on", obj);
   if (sd->parent_obj) elm_widget_scroll_freeze_push(sd->parent_obj);
   // FIXME: on delete/reparent freeze pop
}

EAPI void
elm_widget_scroll_freeze_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze--;
   if (sd->scroll_freeze < 0) sd->scroll_freeze = 0;
   if (sd->scroll_freeze == 0)
     evas_object_smart_callback_call(obj, "scroll-freeze-off", obj);
   if (sd->parent_obj) elm_widget_scroll_freeze_pop(sd->parent_obj);
}

EAPI int
elm_widget_scroll_freeze_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_freeze;
}

EAPI void
elm_widget_scale_set(Evas_Object *obj, double scale)
{
   API_ENTRY return;
   if (scale <= 0.0) scale = 0.0;
   if (sd->scale != scale)
     {
	sd->scale = scale;
	elm_widget_theme(obj);
     }
}

EAPI double
elm_widget_scale_get(const Evas_Object *obj)
{
   API_ENTRY return 1.0;
   // FIXME: save walking up the tree by storingcaching parent scale
   if (sd->scale == 0.0)
     {
	if (sd->parent_obj)
	  return elm_widget_scale_get(sd->parent_obj);
	else
	  return 1.0;
     }
   return sd->scale;
}

EAPI void
elm_widget_theme_set(Evas_Object *obj, Elm_Theme *th)
{
   API_ENTRY return;
   if (sd->theme != th)
     {
        if (sd->theme) elm_theme_free(sd->theme);
        sd->theme = th;
        if (th) th->ref++;
        elm_widget_theme(obj);
     }
}

EAPI Elm_Theme *
elm_widget_theme_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (!sd->theme)
     {
        if (sd->parent_obj)
          return elm_widget_theme_get(sd->parent_obj);
        else
          return NULL;
     }
   return sd->theme;
}

EAPI void
elm_widget_style_set(Evas_Object *obj, const char *style)
{
   API_ENTRY return;

   if (eina_stringshare_replace(&sd->style, style))
     elm_widget_theme(obj);
}

EAPI const char *
elm_widget_style_get(const Evas_Object *obj)
{
   API_ENTRY return "";
   if (sd->style) return sd->style;
   return "default";
}

EAPI void
elm_widget_type_set(Evas_Object *obj, const char *type)
{
   API_ENTRY return;
   eina_stringshare_replace(&sd->type, type);
}

EAPI const char *
elm_widget_type_get(const Evas_Object *obj)
{
   API_ENTRY return "";
   if (sd->type) return sd->type;
   return "";
}









static void
_propagate_x_drag_lock(Evas_Object *obj, int dir)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (sd->parent_obj)
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_x_locked += dir;
             _propagate_x_drag_lock(sd->parent_obj, dir);
          }
     }
}

static void
_propagate_y_drag_lock(Evas_Object *obj, int dir)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (sd->parent_obj)
     {
        Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_y_locked += dir;
             _propagate_y_drag_lock(sd->parent_obj, dir);
          }
     }
}

EAPI void
elm_widget_drag_lock_x_set(Evas_Object *obj, Eina_Bool lock)
{
   API_ENTRY return;
   if (sd->drag_x_locked == lock) return;
   sd->drag_x_locked = lock;
   if (sd->drag_x_locked) _propagate_x_drag_lock(obj, 1);
   else _propagate_x_drag_lock(obj, -1);
}

EAPI void
elm_widget_drag_lock_y_set(Evas_Object *obj, Eina_Bool lock)
{
   API_ENTRY return;
   if (sd->drag_y_locked == lock) return;
   sd->drag_y_locked = lock;
   if (sd->drag_y_locked) _propagate_y_drag_lock(obj, 1);
   else _propagate_y_drag_lock(obj, -1);
}

EAPI Eina_Bool
elm_widget_drag_lock_x_get(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   printf("check %p x lock %i\n", obj, sd->drag_x_locked);
   return sd->drag_x_locked;
}

EAPI Eina_Bool
elm_widget_drag_lock_y_get(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   printf("check %p y lock %i\n", obj, sd->drag_y_locked);
   return sd->drag_y_locked;
}

EAPI int
elm_widget_drag_child_locked_x_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_x_locked;
}

EAPI int
elm_widget_drag_child_locked_y_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_y_locked;
}











/* local subsystem functions */
static void
_smart_reconfigure(Smart_Data *sd)
{
   if (sd->resize_obj)
     {
	evas_object_move(sd->resize_obj, sd->x, sd->y);
	evas_object_resize(sd->resize_obj, sd->w, sd->h);
     }
   if (sd->hover_obj)
     {
	evas_object_move(sd->hover_obj, sd->x, sd->y);
	evas_object_resize(sd->hover_obj, sd->w, sd->h);
     }
}

static void
_smart_add(Evas_Object *obj)
{
   Smart_Data *sd;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->can_focus = 1;
   evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object *obj)
{
   Evas_Object *sobj;

   INTERNAL_ENTRY;
   if (sd->del_pre_func) sd->del_pre_func(obj);
   if (sd->resize_obj)
     {
	sobj = sd->resize_obj;
	sd->resize_obj = NULL;
	evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
	evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
	evas_object_del(sobj);
     }
   if (sd->hover_obj)
     {
	sobj = sd->hover_obj;
	sd->hover_obj = NULL;
	evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
	evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
	evas_object_del(sobj);
     }
   EINA_LIST_FREE(sd->subobjs, sobj)
     {
	evas_object_event_callback_del_full(sobj, EVAS_CALLBACK_DEL, _sub_obj_del, sd);
	evas_object_smart_callback_call(sd->obj, "sub-object-del", sobj);
	evas_object_del(sobj);
     }
   if (sd->del_func) sd->del_func(obj);
   if (sd->style) eina_stringshare_del(sd->style);
   if (sd->type) eina_stringshare_del(sd->type);
   if (sd->theme) elm_theme_free(sd->theme);
   free(sd);
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   _smart_reconfigure(sd);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   _smart_reconfigure(sd);
}

static void
_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->resize_obj);
}

static void
_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->resize_obj);
}

static void
_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->resize_obj, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->resize_obj, clip);
}

static void
_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->resize_obj);
}

static void
_smart_calculate(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   if (sd->changed_func) sd->changed_func(obj);
}

/* never need to touch this */

static void
_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _smart_add,
	       _smart_del,
	       _smart_move,
	       _smart_resize,
	       _smart_show,
	       _smart_hide,
	       _smart_color_set,
	       _smart_clip_set,
	       _smart_clip_unset,
	       _smart_calculate,
	       NULL,
	       NULL,
	       NULL,
               NULL,
               NULL,
               NULL
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

/* utilities */

Eina_List *
_elm_stringlist_get(const char *str)
{
   Eina_List *list = NULL;
   const char *s, *b;
   if (!str) return NULL;
   for (b = s = str; 1; s++)
     {
	if ((*s == ' ') || (*s == 0))
	  {
	     char *t = malloc(s - b + 1);
	     if (t)
	       {
		  strncpy(t, b, s - b);
		  t[s - b] = 0;
		  list = eina_list_append(list, eina_stringshare_add(t));
		  free(t);
	       }
	     b = s + 1;
	  }
	if (*s == 0) break;
     }
   return list;
}

void
_elm_stringlist_free(Eina_List *list)
{
   const char *s;
   EINA_LIST_FREE(list, s) eina_stringshare_del(s);
}

Eina_Bool
_elm_widget_type_check(const Evas_Object *obj, const char *type)
{
   const char *provided, *expected = "(unknown)";
   static int abort_on_warn = -1;
   provided = elm_widget_type_get(obj);
   if (EINA_LIKELY(provided == type)) return EINA_TRUE;
   if (type) expected = type;
   if ((!provided) || (provided[0] == 0))
     {
        provided = evas_object_type_get(obj);
        if ((!provided) || (provided[0] == 0))
          provided = "(unknown)";
     }
   ERR("Passing Object: %p, of type: '%s' when expecting type: '%s'", obj, provided, expected);
   if (abort_on_warn == -1)
     {
        if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
        else abort_on_warn = 0;
     }
   if (abort_on_warn == 1) abort();
   return EINA_FALSE;
}

static inline Eina_Bool
_elm_widget_is(const Evas_Object *obj)
{
   const char *type = evas_object_type_get(obj);
   return type == SMART_NAME;
}
