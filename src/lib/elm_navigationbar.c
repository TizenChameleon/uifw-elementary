#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup NavigationBar NavigationBar
 * @ingroup Elementary
 *
 * The Navigationbar is an object that allows flipping (with animation) between 1 or
 * more of objects, much like a stack of windows within the window. It also displays title
 * area above all the pages consisting of title,function buttons.
 *
 * Objects can be pushed or popped from the stack.
 * Pushes and pops will animate (and a pop will delete the object once the
 * animation is finished). Objects are pushed to the top with 
 * elm_navigationbar_push() and when the top item is no longer
 * wanted, simply pop it with elm_navigationbar_pop() and it will also be
 * deleted. You can query which objects are the top and bottom with 
 * elm_navigationbar_content_bottom_get() and elm_navigationbar_content_top_get().
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _Item Item;
typedef struct _Transit_Cb_Data Transit_Cb_Data;

struct _Widget_Data
{
   Eina_List *stack;
   Evas_Object *base;
   Evas_Object *pager;
   Eina_Bool hidden :1;
   Eina_Bool popping: 1;
 };

struct _Item
{
   Evas_Object *obj;
   const char *title;
   const char *subtitle;
   Evas_Object *title_obj;
   Eina_List *title_list;
   Evas_Object *fn_btn1;
   Evas_Object *fn_btn2;
   Evas_Object *fn_btn3;
   Evas_Object *back_btn;
   Evas_Object *content;
   int fn_btn1_w;
   int fn_btn2_w;
   int fn_btn3_w;
   int title_w;
};

struct _Transit_Cb_Data
{
   Item* prev_it;
   Item* it;
   Eina_Bool pop;
   Eina_Bool first_page;
};

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _item_sizing_eval(Item *it);
static void _delete_item(Item *it);
static void _back_button_clicked(void *data, Evas_Object *obj, void *event_info);
static int _set_button_width(Evas_Object *obj);
static Eina_Bool _button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn);
static Evas_Object *_multiple_object_set(Evas_Object *obj, Evas_Object *sub_obj, Eina_List *list, int width);
static Item *_check_item_is_added(Evas_Object *obj, Evas_Object *content);
static void _transition_complete_cb(void *data);
static void _elm_navigationbar_back_button_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button);
static Evas_Object *_elm_navigationbar_back_button_get(Evas_Object *obj, Evas_Object *content);
static void _elm_navigationbar_function_button1_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button);
static Evas_Object *_elm_navigationbar_function_button1_get(Evas_Object *obj, Evas_Object *content);
static void _elm_navigationbar_function_button2_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button);
static Evas_Object *_elm_navigationbar_function_button2_get(Evas_Object *obj, Evas_Object *content);
static void _elm_navigationbar_function_button3_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button);
static Evas_Object *_elm_navigationbar_function_button3_get(Evas_Object *obj, Evas_Object *content);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list;
   Item *it;
   
   EINA_LIST_FOREACH(wd->stack, list, it)
     _delete_item(it);
   eina_list_free(wd->stack);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list = NULL;
   Item *it = NULL;
   char buf_fn[4096];
   
   if (!wd) return;
   _elm_theme_object_set(obj, wd->base, "navigationbar", "base", elm_widget_style_get(obj));
   edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);
   EINA_LIST_FOREACH(wd->stack, list, it)
     {
        if (it->fn_btn1)
         {  
            snprintf(buf_fn, sizeof(buf_fn), "navigationbar_functionbutton/%s", elm_widget_style_get(obj));
            elm_object_style_set(it->fn_btn1, buf_fn);            
         }
        if (it->fn_btn2)
         {        
            snprintf(buf_fn, sizeof(buf_fn), "navigationbar_functionbutton/%s", elm_widget_style_get(obj));
            elm_object_style_set(it->fn_btn2, buf_fn);               
         }
        if (it->fn_btn3)
         {
            snprintf(buf_fn, sizeof(buf_fn), "navigationbar_functionbutton/%s", elm_widget_style_get(obj));
            elm_object_style_set(it->fn_btn3, buf_fn);            
         }
        if (it->back_btn)
          {
             snprintf(buf_fn, sizeof(buf_fn), "navigationbar_backbutton/%s", elm_widget_style_get(obj));
             elm_object_style_set(it->back_btn, buf_fn);
          }
     }
   edje_object_message_signal_process(wd->base);
   _sizing_eval(obj);
}

static void
_delete_item(Item *it)
{
   Eina_List *ll;
   if (!it) return;
   Evas_Object *list_obj;
   
   if(it->back_btn)
     evas_object_del(it->back_btn);
   if(it->fn_btn1)
     {
	elm_object_unfocus(it->fn_btn1);
	evas_object_del(it->fn_btn1);
     }
   if(it->fn_btn2)
     {
	elm_object_unfocus(it->fn_btn2);
	evas_object_del(it->fn_btn2);
     }
   if(it->fn_btn3)
     {
	elm_object_unfocus(it->fn_btn3);
	evas_object_del(it->fn_btn3); 
     }
   if (it->title) 
     eina_stringshare_del(it->title);
   if (it->subtitle) 
     eina_stringshare_del(it->subtitle);
   EINA_LIST_FOREACH(it->title_list, ll, list_obj)
     evas_object_del(list_obj);
   eina_list_free(it->title_list);
   free(it);   
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord w = -1, h = -1;
   Eina_List *list;

   edje_object_size_min_calc(wd->base, &minw, &minh);
   evas_object_size_hint_min_get(obj, &w, &h);
   if (w > minw) minw = w;
   if (h > minw) minh = h;

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);

   list = eina_list_last(wd->stack);
   if (list)
     {
        Item *it = list->data;
        _item_sizing_eval(it);
     }
}

static void
_item_sizing_eval(Item *it)
{
   if (!it) return;
   Widget_Data *wd = elm_widget_data_get(it->obj);
   Evas_Coord pad, height, minw, w;
   int pad_count = 2;

   edje_object_size_min_calc(wd->base, &minw, NULL);
   evas_object_geometry_get(wd->base, NULL, NULL, &w, NULL);
   if (w < minw) w = minw;   
   edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
   edje_object_part_geometry_get(wd->base, "elm.swallow.title", NULL, NULL, NULL, &height);

   if (it->fn_btn1) 
     {
        it->fn_btn1_w = _set_button_width(it->fn_btn1);
        pad_count++;
     }
   else if (it->back_btn)
     {
        it->fn_btn1_w = _set_button_width(it->back_btn);
        pad_count++;
     }
   if (it->fn_btn2)
     {
        it->fn_btn2_w = _set_button_width(it->fn_btn2);
        pad_count++;
     }
   if (it->fn_btn3)
     {
        it->fn_btn3_w = _set_button_width(it->fn_btn3);
        pad_count++;
     }
   if (it->title_list)
     {  
        it->title_w = _set_button_width(it->title_obj);
        it->title_obj = _multiple_object_set(it->obj, it->title_obj, it->title_list, it->title_w);
        evas_object_resize(it->title_obj, it->title_w, height);
        evas_object_size_hint_min_set(it->title_obj, it->title_w, height);
     }
}

static void
_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(obj);
}

static void 
_transition_complete_cb(void *data)
{
   Transit_Cb_Data *cb = data;
   if (!cb) return;
   Evas_Object *navi_bar = NULL;
   Evas_Object *content = NULL;
   Widget_Data *wd = NULL;
   Item *prev_it = cb->prev_it;
   Item *it = cb->it;

   if (prev_it)
     {
        navi_bar = prev_it->obj;
     }
   else if (it)
     {
        navi_bar = it->obj;
     }
   wd = elm_widget_data_get(navi_bar);
   if (cb->pop && prev_it)
     {
        Eina_List *ll;
        ll = eina_list_last(wd->stack);
        if (ll->data == prev_it)
          {
             _delete_item(prev_it);
             wd->stack = eina_list_remove_list(wd->stack, ll);
          }
     }  
   else if (prev_it)
     {
        evas_object_hide(prev_it->fn_btn1);
        evas_object_hide(prev_it->back_btn);
        evas_object_hide(prev_it->title_obj);
        evas_object_hide(prev_it->fn_btn2);
        evas_object_hide(prev_it->fn_btn3);
     }
   if ((it) && (!wd->hidden))
     {
        edje_object_part_text_set(wd->base, "elm.text", it->title);
        if (!cb->first_page)
          {
             if (cb->pop)
               {
                  edje_object_signal_emit(wd->base, "elm,action,pop", "elm");
               }
             else
               {
                  edje_object_signal_emit(wd->base, "elm,action,push", "elm");
               }
             edje_object_signal_emit(wd->base, "elm,state,rect,enabled", "elm");
          }
        if (it->title_obj) 
          {
             edje_object_part_swallow(wd->base, "elm.swallow.title", it->title_obj);
             edje_object_signal_emit(wd->base, "elm,state,retract,title", "elm");
          }
        if (it->title) 
          {
             edje_object_signal_emit(wd->base, "elm,state,retract,title", "elm");
          }
        if (it->subtitle) 
          {
             edje_object_part_text_set(wd->base, "elm.text.sub", it->subtitle);
          }
        else
          edje_object_part_text_set(wd->base, "elm.text.sub", NULL);
        if (it->fn_btn1) 
          {
             edje_object_signal_emit(wd->base, "elm,state,item,add,leftpad", "elm");
             edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->fn_btn1);
          }
        else if (it->back_btn) 
          {
             edje_object_signal_emit(wd->base, "elm,state,item,add,leftpad", "elm");
             edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->back_btn);
          }
        else
          edje_object_signal_emit(wd->base, "elm,state,item,reset,leftpad", "elm");
        if (it->fn_btn2) 
          {
             edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad", "elm");
             edje_object_part_swallow(wd->base, "elm.swallow.btn2", it->fn_btn2);
          }
        else
          edje_object_signal_emit(wd->base, "elm,state,item,reset,rightpad", "elm");
        if (it->fn_btn3) 
          {
             edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad2", "elm");
             edje_object_signal_emit(wd->base, "elm,state,item,fn_btn3_set", "elm");
             edje_object_part_swallow(wd->base, "elm.swallow.btn3", it->fn_btn3);
          }
        else
          edje_object_signal_emit(wd->base, "elm,state,item,reset,rightpad2", "elm");  
        if ((it->title_obj) && (it->title))
          {
             edje_object_signal_emit(wd->base, "elm,state,extend,title", "elm");
          }  
        content = it->content;
     }
   edje_object_message_signal_process(wd->base);
}

static void 
_back_button_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Item *it = data;
   
   elm_navigationbar_pop(it->obj);
}

static void 
_hide_finished(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *navi_bar = data; 
   Widget_Data *wd =  elm_widget_data_get(navi_bar);
   wd->popping = EINA_FALSE;
   evas_object_smart_callback_call(navi_bar, "hide,finished", event_info);
   edje_object_signal_emit(wd->base, "elm,state,rect,disabled", "elm");
}

static int
_set_button_width(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, maxw= -1, maxh = -1;
   Evas_Coord w = 0, h = 0;

   evas_object_size_hint_min_get(obj, &minw, &minh);
   evas_object_size_hint_max_get(obj, &maxw, &maxh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   if ((maxw >= 0) && (w > maxw)) w = maxw;
   if ((maxh >= 0) && (h > maxh)) h = maxh;
   evas_object_resize(obj, w, h);
   return w;
}

static Eina_Bool 
_button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn)
{
   char buf[4096];
   Eina_Bool changed = EINA_FALSE;
   
   if (prev_btn) 
     {
        evas_object_del(prev_btn);
        prev_btn = NULL;
     }
   if (new_btn) 
     {
        if (back_btn) 
          {
             snprintf(buf, sizeof(buf), "navigationbar_backbutton/%s", elm_widget_style_get(obj));
             elm_object_style_set(new_btn, buf);
          }
        else 
          {           
             snprintf(buf, sizeof(buf), "navigationbar_functionbutton/%s", elm_widget_style_get(obj));
             elm_object_style_set(new_btn, buf);             
          }
        elm_widget_sub_object_add(obj, new_btn);
        changed = EINA_TRUE;
     }
   return changed;
}

static Item *
_check_item_is_added(Evas_Object *obj, Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             return it;
          }
     }
   return NULL;
}

static Evas_Object *
_multiple_object_set(Evas_Object *obj, Evas_Object *sub_obj, Eina_List *list, int width)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Evas_Object *new_obj, *list_obj;
   Evas_Coord pad, height;
   char buf[32];
   int num = 1;
   int count;
   
   edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
   edje_object_part_geometry_get(wd->base, "elm.swallow.title", NULL, NULL, NULL, &height);
   if (!sub_obj)
     {
        new_obj = elm_layout_add(obj);
        elm_widget_sub_object_add(obj, new_obj);
        elm_layout_theme_set(new_obj, "navigationbar", "title", elm_widget_style_get(obj));
     }
   else 
     new_obj = sub_obj;
   count = eina_list_count(list);
   EINA_LIST_FOREACH(list, ll, list_obj)
     {  
        evas_object_resize(list_obj, (width-(count-1)*pad)/count, height);
        evas_object_size_hint_min_set(list_obj, (width-(count-1)*pad)/count, height);    
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "elm,state,item,add,%d", num);
        edje_object_signal_emit(elm_layout_edje_get(new_obj), buf, "elm");
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "elm.swallow.title%d", num++);
        elm_layout_content_set(new_obj, buf, list_obj);
     }
   return new_obj;
}

static void
_multiple_object_unset(Item *it, Eina_List **list)
{
   Evas_Object *list_obj = NULL;
   Eina_List *l = NULL;
   Evas_Object *temp_obj;
   char buf[1024];
   int num = 1;
   if (it->title_obj)
     {
        EINA_LIST_FOREACH (it->title_list, l, list_obj)
          {
             memset(buf, 0, sizeof(buf));
             sprintf(buf, "elm.swallow.title%d", num++);
             temp_obj = elm_layout_content_unset(it->title_obj, buf);
             *list = eina_list_append(*list, temp_obj);
             evas_object_hide(temp_obj);
          }
        eina_list_free(it->title_list);
        it->title_list = NULL;
     }
}


/**
 * Add a new navigationbar to the parent
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "navigationbar");
   elm_widget_type_set(obj, "navigationbar");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "navigationbar", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   wd->pager = elm_pager_add(obj);
   elm_object_style_set(wd->pager, "navigationbar");
   elm_widget_sub_object_add(obj, wd->pager);
   edje_object_part_swallow(wd->base, "elm.swallow.content", wd->pager);
   evas_object_smart_callback_add(wd->pager, "hide,finished", _hide_finished, obj); 
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, NULL);  

   _sizing_eval(obj);
   return obj;
}

/**
 * Push an object to the top of the NavigationBar stack (and show it)
 * The object pushed becomes a child of the navigationbar and will be controlled
 * it is deleted when the navigationbar is deleted or when the content is popped.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] title The title string
 * @param[in] fn_btn1 The left button
 * @param[in] fn_btn2 The right button
 * @param[in] fn_btn3 The button placed before right most button.
 * @param[in] content The object to push
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_push(Evas_Object *obj, const char *title, Evas_Object *fn_btn1, Evas_Object *fn_btn2, Evas_Object *fn_btn3, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Item *prev_it = NULL;

   if (!wd) return;
   it = _check_item_is_added(obj, content);
   if (it) return;
   if (!it) it = ELM_NEW(Item); 
   if (!it) return;
   
   // add and set new items      
   _button_set(obj, NULL, fn_btn1, EINA_FALSE);
   _button_set(obj, NULL, fn_btn2, EINA_FALSE);
   _button_set(obj, NULL, fn_btn3, EINA_FALSE);
   
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        prev_it = ll->data;
     }
   it->obj = obj;
   it->fn_btn1 = fn_btn1;
   it->fn_btn2 = fn_btn2;
   it->fn_btn3 = fn_btn3;
   it->content = content;

   if (!fn_btn1 && prev_it)
   {
      char *prev_title = NULL;

      it->back_btn = elm_button_add(obj);
      prev_title = (char *)prev_it->title;
      if (prev_title)
         {
            elm_button_label_set(it->back_btn, prev_title); 
         }
      else
         {
            elm_button_label_set(it->back_btn, "Previous");
         }
      evas_object_smart_callback_add(it->back_btn, "clicked", _back_button_clicked, it); 
      _button_set(obj, NULL, it->back_btn, EINA_TRUE);
      elm_object_focus_allow_set(it->back_btn, EINA_FALSE);
   }
   
   eina_stringshare_replace(&it->title, title);
   edje_object_part_text_set(wd->base, "elm.text", title);
   _item_sizing_eval(it);

   Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);    
   // unswallow items and start transition
   if (prev_it)
     {
        cb->prev_it = prev_it;
        cb->it = it;
        cb->pop = EINA_FALSE;
        cb->first_page = EINA_FALSE;
        if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
        if (prev_it->fn_btn1) edje_object_part_unswallow(wd->base, prev_it->fn_btn1);
        else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
        if (prev_it->fn_btn2) edje_object_part_unswallow(wd->base, prev_it->fn_btn2);
        if (prev_it->fn_btn3) edje_object_part_unswallow(wd->base, prev_it->fn_btn3);
     }
   else  
     {
        cb->prev_it = NULL;
        cb->it = it;
        cb->pop = EINA_FALSE;   
        cb->first_page = EINA_TRUE;
     }  
   _transition_complete_cb(cb);
   free(cb);
   elm_pager_content_push(wd->pager, it->content); 
   //push item into the stack. it should be always the tail
   if (!_check_item_is_added(obj, content))
     wd->stack = eina_list_append(wd->stack, it); 
   else
     {
        EINA_LIST_FOREACH(wd->stack, ll, it)
          {
             if (it->content == content) 
               {
                  wd->stack = eina_list_demote_list(wd->stack, ll);
                  break;
               }
          }
     }
   _sizing_eval(obj);
}

/**
 * This pops the object that is on top (visible) in the navigationbar, makes it disappear, then deletes the object. 
 * The object that was underneath it, on the stack will become visible.
 *
 * @param[in] obj The NavigationBar object
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_pop(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it = NULL;
   Item *prev_it = NULL;
   
   if(wd->popping) return;
   if (!wd->stack) return;

   //find item to be popped and to be shown
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        prev_it = ll->data;
        ll = ll->prev;
        if (ll)
          {
             it = ll->data;
          }
     }
   //unswallow items and start trasition
   Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);
   if (prev_it && it) 
     {
        cb->prev_it = prev_it;
        cb->it = it;
        cb->pop = EINA_TRUE; 
        if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
        if (prev_it->fn_btn1) edje_object_part_unswallow(wd->base, prev_it->fn_btn1);
        else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
        if (prev_it->fn_btn2) edje_object_part_unswallow(wd->base, prev_it->fn_btn2);
        if (prev_it->fn_btn3) edje_object_part_unswallow(wd->base, prev_it->fn_btn3);
        _item_sizing_eval(it);
     }
   else if (prev_it)
     {
        cb->prev_it = prev_it;
        cb->it = NULL;
        cb->pop = EINA_TRUE;
        cb->first_page = EINA_FALSE;
     }
   _transition_complete_cb(cb);
   wd->popping = EINA_TRUE;
   //pop content from pager
   elm_pager_content_pop(wd->pager);
   if (prev_it && !it)
      {
         edje_object_part_text_set(wd->base, "elm.text", NULL);
      }
   free(cb);
}
   
/**
 * This Pops to the given content object (and update it) by deleting rest of the objects in between.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content the object to show
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_to_content_pop(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!content) return;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it = NULL;
   Item *prev_it = NULL;

   if (!wd->stack) return;
   //find item to be popped and to be shown
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        prev_it = ll->data;
        ll =  ll->prev;   
        while (ll) 
          {
             it = ll->data;    
             if (it->obj && (it->content != content)) 
               { 
                  _delete_item(ll->data);
                  wd->stack = eina_list_remove_list(wd->stack, ll);
                  it = NULL;
               }
             else
               break;
             ll =  ll->prev;   
          }
     }
   if (prev_it && it) 
     {
        //unswallow items and start trasition
        Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);
        cb->prev_it = prev_it;
        cb->it = it;
        cb->pop = EINA_TRUE;
        cb->first_page = EINA_FALSE;
        if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
        if (prev_it->fn_btn1) edje_object_part_unswallow(wd->base, prev_it->fn_btn1);
        else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
        if (prev_it->fn_btn2) edje_object_part_unswallow(wd->base, prev_it->fn_btn2);
        if (prev_it->fn_btn3) edje_object_part_unswallow(wd->base, prev_it->fn_btn3);
        _item_sizing_eval(it);
        _transition_complete_cb(cb);
        //pop content from pager
        elm_pager_to_content_pop(wd->pager, content);
     }
}

/**
 * Set the title string for the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] title The title string
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_label_set(Evas_Object *obj, Evas_Object *content, const char *title)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             eina_stringshare_replace(&it->title, title);
             edje_object_part_text_set(wd->base, "elm.text", title);
             if (!wd->hidden)
               {
                  if ((it->title_obj) && (it->title))
                    { 
                       edje_object_signal_emit(wd->base, "elm,state,extend,title", "elm");
                    }
                  else
                    edje_object_signal_emit(wd->base, "elm,state,retract,title", "elm");
               }
             _item_sizing_eval(it);
             break;
          }
     }
}

/**
 * Return the title string of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The title string or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI const char *
elm_navigationbar_title_label_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype)NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          return it->title;
     }
   return NULL;
}

/**
 * Add a title object for the content.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object pushed
 * @param[in] title_obj a title object (normally button or segment_control)
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_object_add(Evas_Object *obj, Evas_Object *content, Evas_Object *title_obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Item *last_it;

   if (!title_obj) return;
   if (!content) return;
   if (!wd) return;
   it = _check_item_is_added(obj, content);
   if (!it) 
     {
        ERR("[ERROR]Push the Item first, later add the title object");
        return;
     } 
   it->title_list = eina_list_append(it->title_list, title_obj);  
   if (it->obj) _item_sizing_eval(it);  
   //update if the content is the top item
   ll = eina_list_last(wd->stack);
   if (ll) 
     {
        last_it = ll->data;
        if (last_it->content == content) 
          {
             Evas_Object *swallow;
             swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.title");
             if (swallow) {
                edje_object_part_unswallow(wd->base, swallow);
                evas_object_hide(swallow);
             }
             edje_object_part_swallow(wd->base, "elm.swallow.title", it->title_obj);
             if (!wd->hidden)
               {
                  if (it->fn_btn3)
                    {
                       edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad2", "elm");
                       edje_object_signal_emit(wd->base, "elm,state,item,fn_btn3_set", "elm");
                    }        
                  if ((it->title_obj) && (it->title))
                    { 
                       edje_object_signal_emit(wd->base, "elm,state,extend,title", "elm");
                    }
               }
             _item_sizing_eval(it);  
          }
     }
}

/**
 * Unset the list of title objects corresponding to given content and returns it to  
 * the application.
 * @param[in] obj The NavigationBar object
 * @param[in] content The content object pushed
 * @param[out] list updates the list with title objects list, this list has to be freed and the
 * objects have to be deleted by application.
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_object_list_unset(Evas_Object *obj, Evas_Object *content, Eina_List **list)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   Eina_List *ll;
   Item *it;
   Item *last_it = NULL;
   ll = eina_list_last(wd->stack);

   if (ll) 
     {
        last_it = ll->data;
     }
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content)   
          {
             if (last_it->content == it->content) 
               {
                  Evas_Object *swallow;
                  swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.title");
                  if (swallow) 
                    {
                       edje_object_part_unswallow(wd->base, swallow);
                       evas_object_hide(swallow);
                    }
               } 
             _multiple_object_unset(it, list);  
	     if (it->title_obj)
               { 
                  evas_object_del(it->title_obj);
                  it->title_obj = NULL;      
	       }
             if (!wd->hidden)
               {
                  edje_object_signal_emit(wd->base, "elm,state,retract,title", "elm");
                  if(it->fn_btn3)
                     {
                        edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad2", "elm");
                        edje_object_signal_emit(wd->base, "elm,state,item,fn_btn3_set", "elm");
                     }
               }
             _item_sizing_eval(it);
         }
     }
}


/**
 * Return the list of title objects of the pushed content.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The list of title objects
 *
 * @ingroup NavigationBar
 */
EAPI Eina_List *
elm_navigationbar_title_object_list_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content)   
          return it->title_list;
     }  
   return NULL;
}

static void
_elm_navigationbar_back_button_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Eina_Bool changed;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             if(it->back_btn == button)
               return;
             changed = _button_set(obj, it->back_btn, button, EINA_TRUE);
             it->back_btn = button;
             _item_sizing_eval(it);
             break;
          }
     }
   
   //update if the content is the top item
   ll = eina_list_last(wd->stack);
   if (ll) 
     {
        it = ll->data;
        if (it->back_btn && changed && (it->content == content) && (!it->fn_btn1)) 
          {
             edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->back_btn);   
             evas_object_smart_callback_add(it->back_btn, "clicked", _back_button_clicked, it); 
          }
     }
}

static Evas_Object *
_elm_navigationbar_back_button_get(Evas_Object *obj, Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   
   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content)
          return it->back_btn;
     }  
   return NULL;
}

static void 
_elm_navigationbar_function_button1_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Eina_Bool changed;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             if(it->fn_btn1 == button)
               return;
             changed = _button_set(obj, it->fn_btn1, button, EINA_FALSE);
             it->fn_btn1 = button;
             _item_sizing_eval(it);
             break;
          }
     }

   //update if the content is the top item
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        it = ll->data;
        if (it->fn_btn1 && changed && (it->content == content)) 
          {
             if (edje_object_part_swallow_get(wd->base, "elm.swallow.btn1") == it->back_btn)
               {
                  edje_object_part_unswallow(wd->base, it->back_btn);
                  evas_object_hide(it->back_btn);
               }
             edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->fn_btn1);
          }
     }
}

static Evas_Object *
_elm_navigationbar_function_button1_get(Evas_Object *obj, Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   
   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          return it->fn_btn1;
     }  
   return NULL;
}

static void 
_elm_navigationbar_function_button2_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Eina_Bool changed;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             if(it->fn_btn2 == button)
               return;
             changed = _button_set(obj, it->fn_btn2, button, EINA_FALSE);
             it->fn_btn2 = button;
             _item_sizing_eval(it);
             break;
          }
     } 
    
   //update if the content is the top item
   ll = eina_list_last(wd->stack);
   if (ll) 
      {
         it = ll->data;
         if (it->fn_btn2 && changed && (it->content == content)) 
            {
               edje_object_part_swallow(wd->base, "elm.swallow.btn2", it->fn_btn2);
            }
      }
}

static Evas_Object *
_elm_navigationbar_function_button2_get(Evas_Object *obj, 
                              Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          return it->fn_btn2;
     }
   return NULL;
}

static void 
_elm_navigationbar_function_button3_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;
   Eina_Bool changed;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             if(it->fn_btn3 == button)
               return;
             changed = _button_set(obj, it->fn_btn3, button, EINA_FALSE);
             it->fn_btn3 = button;
             _item_sizing_eval(it);
             break;
          }
     } 

   //update if the content is the top item
   ll = eina_list_last(wd->stack);
   if (ll) 
     {
        it = ll->data;
        if (it->fn_btn3 && changed && (it->content == content)) 
          {
             edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad2", "elm");
             edje_object_signal_emit(wd->base, "elm,state,item,fn_btn3_set", "elm");
             edje_object_part_swallow(wd->base, "elm.swallow.btn3", it->fn_btn3);
          }
        else
          edje_object_signal_emit(wd->base, "elm,state,retract,title", "elm");
     }
}

static Evas_Object *
_elm_navigationbar_function_button3_get(Evas_Object *obj, 
                              Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          return it->fn_btn3;
     }
   return NULL;
}

/**
 * Return the content object at the top of the NavigationBar stack
 *
 * @param[in] obj The NavigationBar object
 * @return The top content object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_content_top_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return NULL;
   return elm_pager_content_top_get(wd->pager);
}

/**
 * Return the content object at the bottom of the NavigationBar stack
 *
 * @param[in] obj The NavigationBar object
 * @return The bottom content object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_content_bottom_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype)NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return NULL;
   return elm_pager_content_bottom_get(wd->pager);
}

/**
 * This hides the title area of navigationbar.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] hidden if EINA_TRUE the title area is hidden.
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_hidden_set(Evas_Object *obj, 
                        Eina_Bool hidden)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   
   if (!wd) return;
   if (hidden) 
     edje_object_signal_emit(wd->base, "elm,state,item,moveup", "elm");
   else 
     edje_object_signal_emit(wd->base, "elm,state,item,movedown", "elm");
   wd->hidden = hidden;
}

/**
 * Set the button object of the pushed content.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] button The button
 * @param[in] button_type Indicates the position
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_title_button_set(Evas_Object *obj, Evas_Object *content, Evas_Object *button, Elm_Navi_Button_Type button_type)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!content) return;
   
   switch(button_type)
     {
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON1:
        _elm_navigationbar_function_button1_set(obj, content, button);
        break;
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON2:
        _elm_navigationbar_function_button2_set(obj, content, button);
        break;
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON3:
        _elm_navigationbar_function_button3_set(obj, content, button);
        break;
      case ELM_NAVIGATIONBAR_BACK_BUTTON:
        _elm_navigationbar_back_button_set(obj, content, button);
        break;
      default: 
        break;
     }        
   _sizing_eval(obj);
}

/**
 * Return the button object of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] button_type Indicates the position
 * @return The button object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_title_button_get(Evas_Object *obj, Evas_Object *content, Elm_Navi_Button_Type button_type)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Evas_Object *button=NULL;
   
   if (!content || !obj) return NULL;   
   switch(button_type)
     {
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON1:
        button  = _elm_navigationbar_function_button1_get(obj, content);
        break;
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON2:
        button = _elm_navigationbar_function_button2_get(obj, content);
        break;
      case ELM_NAVIGATIONBAR_FUNCTION_BUTTON3:
        button  = _elm_navigationbar_function_button3_get(obj, content);
        break;
      case ELM_NAVIGATIONBAR_BACK_BUTTON:
        button = _elm_navigationbar_back_button_get(obj, content);
        break;
      default: 
        break;
     }  
   return button;
}

/**
 * Set the sub title string for the pushed content.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] subtitle The subtitle string
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_subtitle_label_set(Evas_Object *obj, Evas_Object *content, const char *subtitle)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          {
             eina_stringshare_replace(&it->subtitle, subtitle);
             edje_object_part_text_set(wd->base, "elm.text.sub", subtitle);
             _item_sizing_eval(it);
             break;
          }
     }
}

/**
 * Return the subtitle string of the pushed content.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The subtitle string or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI const char *
elm_navigationbar_subtitle_label_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype)NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *ll;
   Item *it;

   if (!wd) return NULL;
   EINA_LIST_FOREACH(wd->stack, ll, it)
     {
        if (it->content == content) 
          return it->subtitle;
     }
   return NULL;
}

/**
 * deprecate this This disables content area animation on push/pop.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] disable  if EINA_TRUE animation is disabled.
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_animation_disable_set(Evas_Object *obj, Eina_Bool disable)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   
   elm_pager_animation_disabled_set(wd->pager, disable);
}

/**
 * This disables content area animation on push/pop.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] disable  if EINA_TRUE animation is disabled.
 *
 * @ingroup NavigationBar
 */
EAPI void 
elm_navigationbar_animation_disabled_set(Evas_Object *obj, Eina_Bool disable)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   
   elm_pager_animation_disabled_set(wd->pager, disable);
}

