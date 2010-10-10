#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Navigationbar_ex Navigationbar_ex
 * @ingroup Elementary
 *
 * The NavigationBar_ex is an object that allows flipping (with animation) between 1 or
 * more pages of objects, much like a stack of windows within the window as well display
 * the title area for the page consisting of buttons, title, titleobjects etc:-.
 *
 * Objects can be pushed or popped from the stack or deleted as normal.
 * Pushes and pops will animate and a pop will delete the object once the
 * animation is finished if delete_on_pop is set else the content is unset and the 
 * content pointer is sent as event information in the hide,finished signal.  
 * Any object in the Navigationbar_ex can be promoted to the top
 * (from its current stacking position) as well. Objects are pushed to the
 * top with elm_navigationbar_ex_item_push() and when the top item is no longer
 * wanted, simply pop it with elm_navigationbar_ex_item_pop() and it will also be
 * deleted/unset depending on delete_on_pop variable. 
 * Any object you wish to promote to the top that is already in the
 * navigationbar, simply use elm_navigationbar_ex_item_promote(). If an object is no longer
 * needed and is not the top item, just delete it as normal. You can query
 * which objects are the top and bottom with elm_navigationbar_ex_item_bottom_get()
 * and elm_navigationbar_ex_item_top_get().
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _function_button fn_button;



struct _Widget_Data
{
   Eina_List *stack, *to_delete;
   Elm_Navigationbar_ex_Item *top;
   Evas_Object *rect, *clip;
   Eina_Bool del_on_pop : 1;
};

struct _Elm_Navigationbar_ex_Item
{
   Evas_Object *obj, *base, *content;
   Evas_Coord minw, minh;
   const char *title;
   const char *subtitle;
   const char *item_style;
   Eina_List *fnbtn_list;
   Evas_Object *title_obj;
   Evas_Object *icon;
   Eina_Bool popme : 1;
};

struct _function_button
{
	Evas_Object *btn;
	int btn_id;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
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


static Evas_Object*
_content_unset(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	Evas_Object *content = NULL;
	if(!item->content) return NULL; 
	content = item->content;
	elm_widget_sub_object_del(item->obj,item->content);
	edje_object_part_unswallow(item->base,item->content);	
	item->content = NULL;
	evas_object_hide(content);
	return content;
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   char buf_fn[1024];
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
   	{
   	Eina_List *bl;
	fn_button *btn;
     edje_object_scale_set(it->base, elm_widget_scale_get(obj) * 
                           _elm_config->scale);
	 EINA_LIST_FOREACH(it->fnbtn_list, bl, btn)
	 	{
	 		if(btn->btn_id == ELM_NAVIGATIONBAR_EX_BACK_BUTTON)
	 			{
	 				snprintf(buf_fn, sizeof(buf_fn), "navigationbar_backbutton/%s", elm_widget_style_get(obj));
					elm_object_style_set(btn->btn, buf_fn);
	 			}
			else
				{
	 				snprintf(buf_fn, sizeof(buf_fn), "navigationbar_functionbutton/%s", elm_widget_style_get(obj));
					elm_object_style_set(btn->btn, buf_fn);
				}
	 	}
   	}
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Eina_List *l;
   Elm_Navigationbar_ex_Item *it;
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
   Elm_Navigationbar_ex_Item *it = data;
   Evas_Coord minw = -1, minh = -1;
   evas_object_size_hint_min_get(it->content, &minw, &minh);
   // FIXME: why is this needed? how does edje get this unswallowed or
   // lose its callbacks to edje
   edje_object_part_swallow(it->base, "elm.swallow.content", it->content);
   edje_object_size_min_calc(it->base, &it->minw, &it->minh);
   _sizing_eval(it->obj);
}

static void
_eval_top(Evas_Object *obj, Eina_Bool push)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Bool animate=EINA_TRUE;
   Elm_Navigationbar_ex_Item *ittop;
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
	     edje_object_signal_emit(o, "elm,action,hide", "elm");
	     onhide = edje_object_data_get(o, "onhide");
	     if (onhide)
	       {
		  if (!strcmp(onhide, "raise")) evas_object_raise(o);
		  else if (!strcmp(onhide, "lower")) evas_object_lower(o);
	       }
	  }
	else
	  {
	     animate = EINA_FALSE;
	  }
	wd->top = ittop;
	o = wd->top->base;
	evas_object_show(o);
	if(animate)
		{
		if(push)
	  edje_object_signal_emit(o, "elm,action,show,push", "elm");
		else
	  edje_object_signal_emit(o, "elm,action,show,pop", "elm");
		}
	else
	  edje_object_signal_emit(o, "elm,action,show,noanimate", "elm");
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
   Elm_Navigationbar_ex_Item *it;
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
   Eina_List *l,*list;
   fn_button *btn_data;
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
	if (it->content == sub)
	  {
	    wd->stack = eina_list_remove_list(wd->stack, l);
	    evas_object_event_callback_del_full
               (sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, it);
		if (it->title) eina_stringshare_del(it->title);
		if (it->subtitle) eina_stringshare_del(it->subtitle);
		EINA_LIST_FOREACH(it->fnbtn_list, list, btn_data)
			{
				evas_object_del(btn_data->btn);
				free(btn_data);
				btn_data = NULL;
			}
		if(it->item_style) eina_stringshare_del(it->item_style);
		if(it->title_obj) evas_object_del(it->title_obj);
	     evas_object_del(it->base);
	     _eval_top(it->obj, EINA_FALSE);
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
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   EINA_LIST_FOREACH(wd->stack, l, it) evas_object_resize(it->base, w, h);
}

static void
_signal_hide_finished(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Navigationbar_ex_Item *it = data;
   Evas_Object *obj2 = it->obj;
   Widget_Data *wd = elm_widget_data_get(it->obj);
   evas_object_hide(it->base);
   edje_object_signal_emit(it->base, "elm,action,reset", "elm");
   evas_object_smart_callback_call(obj2, "hide,finished", it->content);
   edje_object_message_signal_process(it->base);
   if(it->popme)
   	{
   		if(wd->del_on_pop)
   			{
   				evas_object_del(it->content);
   			}
		else
			{
				_content_unset(it);
			}
   	}
   _sizing_eval(obj2);
}

static void _item_promote(Elm_Navigationbar_ex_Item* item)
{
   if(!item) return;
   Widget_Data *wd = elm_widget_data_get(item->obj);
   Eina_List *l;
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, l, it)
     {
	if (it == item)
	  {
	     wd->stack = eina_list_remove_list(wd->stack, l);
	     wd->stack = eina_list_append(wd->stack, it);
	     _eval_top(it->obj, EINA_FALSE);
	     return;
	  }
     }
}

static void
_process_deletions(Widget_Data *wd)
{
	if (!wd) return;	
	Elm_Navigationbar_ex_Item *it;	
	fn_button *btn_data;
	Eina_List *list;
	EINA_LIST_FREE(wd->to_delete, it)
		{
			evas_object_event_callback_del_full
						  (it->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, it);
			if (it->title) eina_stringshare_del(it->title);
			if (it->subtitle) eina_stringshare_del(it->subtitle);
			if(it->item_style) eina_stringshare_del(it->item_style);
			EINA_LIST_FOREACH(it->fnbtn_list, list, btn_data)
				{
					evas_object_del(btn_data->btn);
					free(btn_data);
					btn_data = NULL;
				}
			if(it->title_obj) evas_object_del(it->title_obj);		
			if(it->content)  evas_object_del(it->content);
			evas_object_del(it->base);
			_eval_top(it->obj, EINA_FALSE);
			free(it);
			it = NULL;
		}	
}

/**
 * Add a new navigationbar_ex to the parent
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object *
elm_navigationbar_ex_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "navigationbar_ex");
   elm_widget_type_set(obj, "navigationbar_ex");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

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
   wd->del_on_pop = EINA_TRUE;
   _sizing_eval(obj);
   return obj;
}

/**
 * Push an object along with its style to the top of the Navigationbar_ex stack (and show it)
 *
 * The object pushed becomes a child of the Navigationbar_ex and will be controlled
 * it will be deleted when the Navigationbar_ex is deleted or when content is popped(depending on del_
 * on_pop variable).
 *
 * @param[in] obj The Navigationbar_ex object
 * @param[in] content The object to push
 * @param[in] item_style The style of the page
 * @return The Navigationbar_ex Item or NULL
 * @ingroup Navigationbar_ex
 */
EAPI Elm_Navigationbar_ex_Item*
elm_navigationbar_ex_item_push(Evas_Object *obj, Evas_Object *content, const char* item_style)
{
   ELM_CHECK_WIDTYPE(obj, widtype)NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Navigationbar_ex_Item *it = ELM_NEW(Elm_Navigationbar_ex_Item);
   Evas_Coord x, y, w, h;
   char buf[1024];
   if (!wd) return NULL;
   if (!content) return NULL;
   if (!item_style) return NULL;
   if (!it) return NULL;
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
   strncpy(buf, "item/", sizeof(buf));
   strncat(buf, item_style, sizeof(buf) - strlen(buf));
   if (!eina_stringshare_replace(&it->item_style, item_style)) return NULL;
   
   _elm_theme_object_set(obj, it->base,  "navigationbar_ex", buf, elm_widget_style_get(obj));
   edje_object_signal_callback_add(it->base, "elm,action,hide,finished", "", 
                                   _signal_hide_finished, it);
   evas_object_event_callback_add(it->content,
                                  EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				  _changed_size_hints, it);
   edje_object_part_swallow(it->base, "elm.swallow.content", it->content);
   edje_object_size_min_calc(it->base, &it->minw, &it->minh);
   evas_object_show(it->content);
   wd->stack = eina_list_append(wd->stack, it);
   _eval_top(obj, EINA_TRUE);
   _sizing_eval(obj);
   return it;
}

/**
 * Set the title string for the pushed Item.
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] title The title string
 *
 * @ingroup Navigationbar_ex
 */
EAPI void
elm_navigationbar_ex_item_title_label_set( Elm_Navigationbar_ex_Item* item, 
							const char *title)
{
	if(!item) return;
	if (!eina_stringshare_replace(&item->title, title)) return;
   	if (item->base)
     edje_object_part_text_set(item->base, "elm.text", item->title);	
}

/**
 * Return the title string of the pushed item.
 *
 * @param[in]  item The Navigationbar_ex Item
 * @return The title string or NULL if none
 *
 * @ingroup Navigationbar_ex
 */
EAPI const char *
elm_navigationbar_ex_item_title_label_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	return item->title;
}

/**
 * Set the sub title string for the pushed content
 *
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] subtitle The subtitle string
 *
 * @ingroup Navigationbar_ex
 */
EAPI void 
elm_navigationbar_ex_item_subtitle_label_set( Elm_Navigationbar_ex_Item* item, 
							const char *subtitle)
{
	if(!item) return;
	if (!eina_stringshare_replace(&item->subtitle, subtitle)) return;
   	if (item->base)
     edje_object_part_text_set(item->base, "elm.text.sub", item->subtitle);	
}

/**
 * Return the subtitle string of the pushed content
 *
 * @param[in] item The Navigationbar_ex Item
 * @return The subtitle string or NULL if none
 *
 * @ingroup Navigationbar_ex
 */
EAPI const char *
elm_navigationbar_ex_item_subtitle_label_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	return item->subtitle;
}


EAPI void
elm_navigationbar_ex_item_icon_set(Elm_Navigationbar_ex_Item* item, Evas_Object *icon)
{
	if(!item) return; 
	edje_object_part_swallow(item->base, "elm.swallow.icon", icon);
	elm_widget_sub_object_add(item->obj, icon);
	item->icon = icon;
}

EAPI Evas_Object *
elm_navigationbar_ex_item_icon_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL; 
	return item->icon;
}


/**
 * Set the button object of the pushed content
 *
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] btn_label The button label
 * @param[in] icon The button icon
 * @param[in] button_type Indicates the position[use macros of type Elm_Navi_ex_Button_Type
 * if more function buttons are required you can use values ELM_NAVIGATIONBAR_EX_MAX and more]
 * @param[in] func Callback function called when button is clicked.
 * @param[in] data Callback data that would be sent when button is clicked.
 * @ingroup Navigationbar_ex
 */ 
 EAPI void
elm_navigationbar_ex_item_title_button_set(Elm_Navigationbar_ex_Item* item, char *btn_label, Evas_Object *icon, int button_type, Evas_Smart_Cb func, const void *data)
{
	if(!item) return;
	Evas_Object *btn;
	char buf[1024],theme[1024];
	fn_button *btn_det;
	btn = elm_button_add(item->obj);
	btn_det = ELM_NEW(btn_det);
	if(button_type == ELM_NAVIGATIONBAR_EX_BACK_BUTTON)
		{
			snprintf(theme, sizeof(theme), "navigationbar_backbutton/%s", elm_widget_style_get(item->obj));
			elm_object_style_set(btn, theme);
			snprintf(buf, sizeof(buf), "elm.swallow.back");
		}
	else
		{
			snprintf(theme, sizeof(theme), "navigationbar_functionbutton/%s", elm_widget_style_get(item->obj));
			elm_object_style_set(btn, theme);					
			snprintf(buf, sizeof(buf), "elm.swallow.btn%d", button_type);
		}
	if(btn_label)
		elm_button_label_set(btn, btn_label);
	if(icon)
		elm_button_icon_set(btn, icon);
	elm_object_focus_allow_set(btn, EINA_FALSE); 
	evas_object_smart_callback_add(btn, "clicked", func, data);
	edje_object_part_swallow(item->base, buf, btn);
	elm_widget_sub_object_add(item->obj, btn);
	btn_det->btn = btn;
	btn_det->btn_id = button_type;
	item->fnbtn_list = eina_list_append(item->fnbtn_list, btn_det);
}

/**
 * Return the button object of the pushed content
 *
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] button_type Indicates the position
 * @return The button object or NULL if none
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object *
elm_navigationbar_ex_item_title_button_get(Elm_Navigationbar_ex_Item* item, int button_type)
 	{
 		fn_button *btn_det;
		Eina_List *bl;
		EINA_LIST_FOREACH(item->fnbtn_list, bl, btn_det)
	 	{
	 		if(btn_det->btn_id == button_type)
	 			return btn_det->btn;
	 	}
		return NULL;
 		
 	}

/**
 * Sets a title object for the Item 
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] title_obj Title object (normally segment_control/searchbar)
 *
 * @ingroup Navigationbar_ex
 */
EAPI void
elm_navigationbar_ex_item_title_object_set(Elm_Navigationbar_ex_Item* item,
									Evas_Object *title_obj)
{
	if(!item) return;
	if(item->title_obj) evas_object_del(item->title_obj);
	item->title_obj = title_obj;
	if(title_obj)
		{
			elm_widget_sub_object_add(item->obj,title_obj);
			edje_object_part_swallow(item->base, "elm.swallow.title", title_obj);
		}
	_sizing_eval(item->obj);
}

/**
 * Hides the title area of the item.
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] hidden if EINA_TRUE the title area is hidden else its shown.
 *
 * @ingroup Navigationbar_ex
 */

EAPI void
elm_navigationbar_ex_item_title_hidden_set(Elm_Navigationbar_ex_Item* item,
									Eina_Bool hidden)
{
	if(!item) return;

	if (hidden) edje_object_signal_emit(item->base, "elm,state,item,moveup", "elm");
	else edje_object_signal_emit(item->base, "elm,state,item,movedown", "elm");
	_sizing_eval(item->obj);
}

/**
 * Unsets a title object for the item, the return object has to be deleted
 * by application if not added again in to navigationbar.
 *
 * @param[in] item The Navigationbar_ex Item 
 * @return The title object or NULL if none is set
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object*
elm_navigationbar_ex_item_title_object_unset(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	Evas_Object *title_obj=NULL;
	if(!item->title_obj) return NULL; 
	title_obj = item->title_obj;
	elm_widget_sub_object_del(item->obj,item->title_obj);
	edje_object_part_unswallow(item->base,item->title_obj);	
	item->title_obj = NULL;
	return title_obj;
}

/**
 * Returns the title object of the pushed content.
 *
 * @param[in] item The Navigationbar_ex Item 
 * @return The title object or NULL if none is set
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object*
elm_navigationbar_ex_item_title_object_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	return item->title_obj;
}


/**
 * Unsets the content of the item, the return object has to be deleted
 * by application if not added again in to navigationbar, when the content 
 * is unset the corresponding item would be deleted, when this content is pushed again 
 * a new item would be created again.
 *
 * @param[in] item The Navigationbar_ex Item 
 * @return The content object or NULL if none is set
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object *elm_navigationbar_ex_item_content_unset(Elm_Navigationbar_ex_Item* item)
{
	Evas_Object *content = _content_unset(item);
	return content;
}


/**
 * Returns the content of the item.
 *
 * @param[in] item The Navigationbar_ex Item 
 * @return The content object or NULL if none is set
 *
 * @ingroup Navigationbar_ex
 */
EAPI Evas_Object *elm_navigationbar_ex_item_content_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	return item->content;
}

/**
 * Set whether the content pushed has to be deleted on pop.
 * if false the item is not deleted but only removed from the stack
 * the pointer of the content is sent along with hide,finished signal.
 *
 * @param[in] obj The Navigationbar_ex object. 
 * @param[in] del_on_pop if set the content is deleted on pop else unset, by default the value is EINA_TRUE.
 *
 * @ingroup Navigationbar_ex
 */
EAPI void elm_navigationbar_ex_delete_on_pop_set(Evas_Object *obj, Eina_Bool del_on_pop)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->del_on_pop = del_on_pop;	
}

/**
 * Sets the style of the navigationbar item. 
 * @param[in] item The Navigationbar_ex Item 
 * @param[in] item_style Navigationbar Item style, this can be used when the style of the item has to be dynamically set.
 *
 * @ingroup Navigationbar_ex
 */
EAPI void
elm_navigationbar_ex_item_style_set(Elm_Navigationbar_ex_Item* item, const char* item_style)
{
	if(!item) return;
	char buf[1024];
	strncpy(buf, "item/", sizeof(buf));
	strncat(buf, item_style, sizeof(buf) - strlen(buf));
	if (!eina_stringshare_replace(&item->item_style, item_style)) return;
	_elm_theme_object_set(item->obj, item->base,  "navigationbar2", buf, elm_widget_style_get(item->obj));
	if(item->title)
		edje_object_part_text_set(item->base, "elm.text", item->title);
	if(item->subtitle)
		edje_object_part_text_set(item->base, "elm.text.sub", item->subtitle);
}

/**
 * Returns the style of the item.
 *
 * @param[in] item The Navigationbar_ex Item 
 * @return The item style.
 *
 * @ingroup Navigationbar_ex
 */
EAPI const char* elm_navigationbar_ex_item_style_get(Elm_Navigationbar_ex_Item* item)
{
	if(!item) return NULL;
	return item->item_style;
}


/**
 * Promote an object already in the stack to the top of the stack
 *
 * This will take the indicated object and promote it to the top of the stack
 * as if it had been pushed there. The object must already be inside the
 * Navigationbar_ex stack to work.
 *
 * @param[in] item The Navigationbar_ex item to promote.
 * @ingroup Navigationbar_ex
 */
EAPI void
elm_navigationbar_ex_item_promote(Elm_Navigationbar_ex_Item* item)
{
	_item_promote(item);  
}

/**
 * Pop to the inputted Navigationbar_ex item
 * the rest of the items are deleted.
 *
 * @param[in] item The Navigationbar_ex item
 *
 * @ingroup Navigationbar_ex
 */
EAPI void elm_navigationbar_ex_to_item_pop(Elm_Navigationbar_ex_Item* item)
{
   if(!item) return;
   Widget_Data *wd = elm_widget_data_get(item->obj);
   Elm_Navigationbar_ex_Item *it = NULL;
   Eina_List *list;
   if (!wd) return;
   if (!wd->stack) return;
   list = eina_list_last(wd->stack);
   if(list)
   	{
   		while(list)
   			{
   				it = list->data;
				if(it != item)
					{
						wd->to_delete = eina_list_append(wd->to_delete, it);
						wd->stack = eina_list_remove_list(wd->stack, list);
					}
				else
					break;
				
				list = list->prev;
   			}
   	}  
    _eval_top(it->obj, EINA_FALSE);
	if(wd->to_delete)
		_process_deletions(wd);
}

/**
 * Pop the object that is on top of the Navigationbar_ex stack 
 * This pops the object that is on top (visible) in the navigationbar, makes it disappear, then deletes/unsets the object
 * based on del_on_pop variable. 
 * The object that was underneath it on the stack will become visible.
 *
 * @param[in] obj The Navigationbar_ex object
 *
 * @ingroup Navigationbar_ex
 */
EAPI void
elm_navigationbar_ex_item_pop(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Elm_Navigationbar_ex_Item *it;
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
		 
	     edje_object_signal_emit(o, "elm,action,hide", "elm");
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
	     _item_promote(it);
	  }
     }
}


/**
 * Return the item at the bottom of the Navigationbar_ex stack
 *
 * @param[in] obj The Navigationbar_ex object
 * @return The bottom item or NULL if none
 *
 * @ingroup Navigationbar_ex
 */
EAPI Elm_Navigationbar_ex_Item*
elm_navigationbar_ex_item_bottom_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return NULL;
   if (!wd->stack) return NULL;
   it = wd->stack->data;
   return it;
}

/**
 * Return the item at the top of the Navigationbar_ex stack
 *
 * @param[in] obj The Navigationbar_ex object
 * @return The top object or NULL if none
 *
 * @ingroup Navigationbar_ex
 */
EAPI Elm_Navigationbar_ex_Item*
elm_navigationbar_ex_item_top_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Navigationbar_ex_Item *it;
   if (!wd) return NULL;
   if (!wd->stack) return NULL;
   it = eina_list_last(wd->stack)->data;
   return it;
}


