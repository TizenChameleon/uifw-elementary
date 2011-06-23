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

#define _ELM_NAVIBAR_PREV_BTN_DEFAULT_LABEL "Previous"

typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Navigationbar_Item Elm_Navigationbar_Item;
typedef struct _Transit_Cb_Data Transit_Cb_Data;

static const char _navigationbar_key[] = "_elm_navigationbar";

//TODO: Remove!
typedef enum
  {
     ELM_NAVIGATIONBAR_PREV_BUTTON = ELM_NAVIGATIONBAR_FUNCTION_BUTTON1,
     ELM_NAVIGATIONBAR_NEXT_BUTTON = ELM_NAVIGATIONBAR_FUNCTION_BUTTON2,
     ELM_NAVIGATIONBAR_TITLE_BTN_CNT = 2
  } Elm_Navigationbar_Button_Type;

struct _Widget_Data
{
   Eina_List *stack;
   Evas_Object *base;
   Evas_Object *pager;
   Eina_Bool title_visible : 1;
   Eina_Bool popping: 1;
 };

struct _Elm_Navigationbar_Item
{
   Elm_Widget_Item base;
   const char *title;
   const char *subtitle;
   Eina_List *title_obj_list;   //TODO: Remove!
   Evas_Object *title_obj;
   Evas_Object *title_btns[ELM_NAVIGATIONBAR_TITLE_BTN_CNT];
   Evas_Object *content;
   Evas_Object *icon;
   Eina_Bool titleobj_visible :1;
   Eina_Bool back_btn :1;
};

//TODO: Remove!
struct _Transit_Cb_Data
{
   Elm_Navigationbar_Item* prev_it;
   Elm_Navigationbar_Item* it;
   Evas_Object *navibar;
   Eina_Bool pop : 1;
   Eina_Bool first_page : 1;
};

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _item_sizing_eval(Elm_Navigationbar_Item *it);
static void _item_del(Elm_Navigationbar_Item *it);
static void _back_button_clicked(void *data, Evas_Object *obj, void *event_info);
static void _button_size_set(Evas_Object *obj);
static Eina_Bool _button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn);
static Elm_Navigationbar_Item *_check_item_is_added(Evas_Object *obj, Evas_Object *content);
static void _transition_complete_cb(void *data);
static void _elm_navigationbar_prev_btn_set(Evas_Object *obj,
                                            Evas_Object *content,
                                            Evas_Object *new_btn,
                                            Elm_Navigationbar_Item *it);
static void _elm_navigationbar_next_btn_set(Evas_Object *obj,
                                            Evas_Object *content,
                                            Evas_Object *new_btn,
                                            Elm_Navigationbar_Item *it);
static void _title_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _titleobj_switching(Evas_Object *obj, Elm_Navigationbar_Item *it);

static const char SIG_HIDE_FINISHED[] = "hide,finished";
static const char SIG_TITLE_OBJ_VISIBLE_CHANGED[] = "titleobj,visible,changed";
static const char SIG_TITLE_CLICKED[] = "title,clicked";

static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_HIDE_FINISHED, ""},
       {SIG_TITLE_OBJ_VISIBLE_CHANGED, ""},
       {SIG_TITLE_CLICKED, ""},
       {NULL, NULL}
};

static void
_content_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Navigationbar_Item *it = data;
   evas_object_data_del(obj, _navigationbar_key);
   it->content = NULL;
   //TODO: it will be better remove this page?
}

static void
_title_obj_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Navigationbar_Item *it = data;
   Eina_List *l = NULL;
   elm_navigationbar_title_object_list_unset(it->base.widget, it->content, &l);
   if (!l) return;
   evas_object_del(eina_list_data_get(l));
   eina_list_free(l);
}

static void
_title_icon_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Navigationbar_Item *it = data;
   it->icon = NULL;
}

static void
_title_btn_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Navigationbar_Item *it = data;

   if (it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON] == obj)
     it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON] = NULL;
   else if (it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON] == obj)
     it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON] = NULL;
}

static Eina_Bool
_title_btn_set(Elm_Navigationbar_Item *it, Evas_Object *btn, int title_btn_idx, Eina_Bool back_btn)
{
   Eina_Bool changed;

   if(it->title_btns[title_btn_idx] == btn) return EINA_FALSE;

   changed = _button_set(it->base.widget, it->title_btns[title_btn_idx], btn, back_btn);
   it->title_btns[title_btn_idx] = btn;

   if ((!changed) || (!btn)) return EINA_FALSE;

   it->back_btn = back_btn;

   evas_object_event_callback_add(btn, EVAS_CALLBACK_DEL, _title_btn_del, it);

   return EINA_TRUE;
}

static Evas_Object *
_create_back_btn(Evas_Object *parent, const char *title, void *data)
{
   Evas_Object *btn = elm_button_add(parent);
   if (!btn) return NULL;
   elm_button_label_set(btn, title);
   evas_object_smart_callback_add(btn, "clicked", _back_button_clicked, data);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   return btn;
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list;
   Elm_Navigationbar_Item *it;

   EINA_LIST_FOREACH(wd->stack, list, it)
     _item_del(it);
   eina_list_free(wd->stack);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *list = NULL;
   Elm_Navigationbar_Item *it = NULL;
   char buf_fn[4096];

   if (!wd) return;
   _elm_theme_object_set(obj, wd->base, "navigationbar", "base", elm_widget_style_get(obj));
   EINA_LIST_FOREACH(wd->stack, list, it)
     {
        if (it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
         {
            if (it->back_btn)
              snprintf(buf_fn, sizeof(buf_fn), "navigationbar_prev_btn/%s", elm_widget_style_get(obj));
            else
              snprintf(buf_fn, sizeof(buf_fn), "navigationbar_next_btn/%s", elm_widget_style_get(obj));
            elm_object_style_set(it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON], buf_fn);
         }
        if (it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
         {
            snprintf(buf_fn, sizeof(buf_fn), "navigationbar_next_btn/%s", elm_widget_style_get(obj));
            elm_object_style_set(it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON], buf_fn);
         }
     }
   edje_object_message_signal_process(wd->base);
   _sizing_eval(obj);
}

static void
_item_del(Elm_Navigationbar_Item *it)
{
   //TODO: So hard to manage.
   //TODO: Just prepare one layout for title objects.
   //TODO: then remove the layout only.
   Widget_Data *wd;
   int idx;

   if (!it) return;

   wd = elm_widget_data_get(it->base.widget);
   if (!wd) return;

   //Remove Function Buttons
   for (idx = 0; idx < ELM_NAVIGATIONBAR_TITLE_BTN_CNT; idx++)
     {
        if (!it->title_btns[idx]) continue;
        if (!it->back_btn) elm_object_unfocus(it->title_btns[idx]);
        evas_object_event_callback_del(it->title_btns[idx], EVAS_CALLBACK_DEL, _title_btn_del);
        evas_object_del(it->title_btns[idx]);
     }
   if (it->icon)
     {
        evas_object_event_callback_del(it->icon, EVAS_CALLBACK_DEL, _title_icon_del);
        evas_object_del(it->icon);
     }
   if (it->title_obj)
     {
        evas_object_event_callback_del(it->title_obj, EVAS_CALLBACK_DEL, _title_obj_del);
        evas_object_del(it->title_obj);
        eina_list_free(it->title_obj_list);
     }
   if (it->title) eina_stringshare_del(it->title);
   if (it->subtitle) eina_stringshare_del(it->subtitle);

   if (it->content)
     {
        evas_object_data_del(it->content, _navigationbar_key);
        evas_object_event_callback_del(it->content, EVAS_CALLBACK_DEL, _content_del);
     }

   free(it);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd;
   Eina_List *list;

   wd  = elm_widget_data_get(obj);
   if (!wd) return;

   list = eina_list_last(wd->stack);
   if (!list) return;

   _item_sizing_eval(list->data);
}

static void
_item_sizing_eval(Elm_Navigationbar_Item *it)
{
   if (!it) return;
   Widget_Data *wd = elm_widget_data_get(it->base.widget);
   Evas_Coord minw;

   if (!wd) return;

   edje_object_size_min_calc(wd->base, &minw, NULL);

   //TODO: Even the below code for size calculation is redundant and should be removed.
   //TODO: Item_sizing_eval function has to be totally refactored/removed.
   _button_size_set(it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
   _button_size_set(it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);
}

static void
_resize(void *data __UNUSED__, Evas *e  __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   _sizing_eval(obj);
}

static void
_titleobj_switching(Evas_Object *obj, Elm_Navigationbar_Item *it)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (!it->title_obj) return;

   if (elm_navigationbar_content_top_get(it->base.widget) != it->content)
     return;

   if (it->titleobj_visible)
     edje_object_signal_emit(wd->base, "elm,state,show,title", "elm"); //elm,state,title,show
   else
     edje_object_signal_emit(wd->base, "elm,state,hide,title", "elm"); //elm,state,title,hide

   _item_sizing_eval(it);
}

static void
_title_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *navibar = data;
   Widget_Data *wd;
   Eina_List *last;
   Elm_Navigationbar_Item *it;

   wd = elm_widget_data_get(navibar);
   if (!wd) return;

   last = eina_list_last(wd->stack);
   if (!last) return;

   it = eina_list_data_get(last);
   if ((!it) || (!it->title_obj)) return;

   if (!it->titleobj_visible)
     {
        it->titleobj_visible = EINA_TRUE;
        evas_object_smart_callback_call(it->base.widget, SIG_TITLE_OBJ_VISIBLE_CHANGED, (void *) EINA_TRUE);
     }
   else
     {
        it->titleobj_visible = EINA_FALSE;
        evas_object_smart_callback_call(it->base.widget, SIG_TITLE_OBJ_VISIBLE_CHANGED, (void *) EINA_FALSE);
     }

   evas_object_smart_callback_call(navibar, SIG_TITLE_CLICKED, NULL);

   _titleobj_switching(navibar, it);
}

//TODO: should be renamed.
static void
_transition_complete_cb(void *data)
{
   Evas_Object *navi_bar;
   Widget_Data *wd;
   Elm_Navigationbar_Item *prev_it;
   Elm_Navigationbar_Item *it;
   Eina_List *ll;

   Transit_Cb_Data *cb = data;
   if (!cb) return;

   navi_bar = cb->navibar;
   if (!navi_bar) return;

   wd = elm_widget_data_get(navi_bar);
   if (!wd) return;

   prev_it = cb->prev_it;
   it = cb->it;

   if (cb->pop && prev_it)
     {
        ll = eina_list_last(wd->stack);
        if (ll->data == prev_it)
          {
             _item_del(prev_it);
             wd->stack = eina_list_remove_list(wd->stack, ll);
          }
     }
   else if (prev_it)
     {
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
           evas_object_hide(prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
          evas_object_hide(prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);
        if (prev_it->title_obj)
          evas_object_hide(prev_it->title_obj);
        if (prev_it->icon)
          evas_object_hide(prev_it->icon);
     }
   if ((it) && (wd->title_visible))
     {
        edje_object_part_text_set(wd->base, "elm.text", it->title);

        if (!cb->first_page)
          {
             if (cb->pop)
               edje_object_signal_emit(wd->base, "elm,action,pop", "elm");
             else
               edje_object_signal_emit(wd->base, "elm,action,push", "elm");
             evas_object_pass_events_set(wd->base, EINA_TRUE);
          }
        if (it->title_obj)
          {
             edje_object_part_swallow(wd->base, "elm.swallow.title", it->title_obj);
          }
        if (it->subtitle)
          edje_object_part_text_set(wd->base, "elm.text.sub", it->subtitle);
        else
          edje_object_part_text_set(wd->base, "elm.text.sub", NULL);

        if (it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
          edje_object_part_swallow(wd->base, "elm.swallow.prev_btn", it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
        if (it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
          edje_object_part_swallow(wd->base, "elm.swallow.next_btn", it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);

        if(it->icon)
          {
             edje_object_part_swallow(wd->base, "elm.swallow.icon", it->icon);
             edje_object_signal_emit(wd->base, "elm,state,icon,visible", "elm");
          }
        else
          edje_object_signal_emit(wd->base, "elm,state,icon,hidden", "elm");

        if ((it->title_obj) && (it->title))
          {
             edje_object_signal_emit(wd->base, "elm,state,show,extended", "elm");
             if(it->titleobj_visible)
               {
                 //TODO: remove the dependency on these signals as related to nbeat, try to make it totally theme dependent
                 edje_object_signal_emit(wd->base, "elm,state,show,noanimate,title", "elm");
               }
             else
               //TODO: remove the dependency on these signals as related to nbeat, try to make it totally theme dependent
               edje_object_signal_emit(wd->base, "elm,state,hide,noanimate,title", "elm");
          }
        else
          {
             edje_object_signal_emit(wd->base, "elm,state,hide,extended", "elm");
             //TODO: remove the dependency on these signals as related to nbeat, try to make it totally theme dependent
             edje_object_signal_emit(wd->base, "elm,state,hide,noanimate,title", "elm");
             it->titleobj_visible = EINA_FALSE;
          }
     }
}

static void
_back_button_clicked(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Navigationbar_Item *it = data;
   elm_navigationbar_pop(it->base.widget);
}

static void
_hide_finished(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Object *navi_bar = data;
   Widget_Data *wd =  elm_widget_data_get(navi_bar);
   wd->popping = EINA_FALSE;
   evas_object_smart_callback_call(navi_bar, SIG_HIDE_FINISHED, event_info);
   evas_object_pass_events_set(wd->base, EINA_FALSE);
}

static void
_button_size_set(Evas_Object *obj)
{
   if (!obj) return;
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
}

static void
_elm_navigationbar_prev_btn_set(Evas_Object *obj, Evas_Object *content, Evas_Object *new_btn, Elm_Navigationbar_Item *it)
{
   Widget_Data *wd;
   Evas_Object *prev_btn;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (!_title_btn_set(it, new_btn, ELM_NAVIGATIONBAR_PREV_BUTTON, EINA_FALSE))
     return;

   //update if the content is the top item
   if (elm_navigationbar_content_top_get(obj) != content)
     return;

   prev_btn = edje_object_part_swallow_get(wd->base, "elm.swallow.prev_btn");
   if (prev_btn) evas_object_del(prev_btn);
   edje_object_part_swallow(wd->base, "elm.swallow.prev_btn", new_btn);
}

//TODO: looks make this  _elm_navigationbar_function_button1_set  same.
static void
_elm_navigationbar_next_btn_set(Evas_Object *obj, Evas_Object *content, Evas_Object *new_btn, Elm_Navigationbar_Item *it)
{
   Widget_Data *wd;
   Evas_Object *prev_btn;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (!_title_btn_set(it, new_btn, ELM_NAVIGATIONBAR_NEXT_BUTTON, EINA_FALSE))
     return;

   //update if the content is the top item
   if (elm_navigationbar_content_top_get(obj) != content)
     return;

   prev_btn = edje_object_part_swallow_get(wd->base, "elm.swallow.next_btn");
   if (prev_btn) evas_object_del(prev_btn);
   edje_object_part_swallow(wd->base, "elm.swallow.next_btn", new_btn);
}

static Eina_Bool
_button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn)
{
   char buf[4096];   //TODO: How to guarantee this buffer size?
   Eina_Bool changed = EINA_FALSE;

   if (prev_btn)
     {
        changed = EINA_TRUE;
        evas_object_del(prev_btn);
     }
   if (!new_btn) return changed;

   if (back_btn)
     {
        snprintf(buf, sizeof(buf), "navigationbar_prev_btn/%s", elm_widget_style_get(obj));
        elm_object_style_set(new_btn, buf);
     }
   else
     {
        snprintf(buf, sizeof(buf), "navigationbar_next_btn/%s", elm_widget_style_get(obj));
        elm_object_style_set(new_btn, buf);
     }

   elm_widget_sub_object_add(obj, new_btn);
   changed = EINA_TRUE;

   return changed;
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
   //TODO: elm,action,title,clicked
   edje_object_signal_callback_add(wd->base, "elm,action,clicked", "elm",
                                            _title_clicked, obj);

   //TODO: How about making the pager as a base?
   //TODO: Swallow title and content as one content into the pager.
   wd->pager = elm_pager_add(obj);
   elm_object_style_set(wd->pager, "navigationbar");
   elm_widget_sub_object_add(obj, wd->pager);
   edje_object_part_swallow(wd->base, "elm.swallow.content", wd->pager);
   evas_object_smart_callback_add(wd->pager, "hide,finished", _hide_finished, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, NULL);

   wd->title_visible = EINA_TRUE;

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   //TODO: apply elm_object_disabled_set

   return obj;
}

/**
 * Push an object to the top of the NavigationBar stack (and show it)
 * The object pushed becomes a child of the navigationbar and will be controlled
 * it is deleted when the navigationbar is deleted or when the content is popped.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] title The title string
 * @param[in] prev_btn The previous button
 * @param[in] next_btn The next button
 * @param[in] unused Unused.
 * @param[in] content The object to push
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_push(Evas_Object *obj, const char *title, Evas_Object *prev_btn, Evas_Object *next_btn, Evas_Object *unused __UNUSED__, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd;
   Elm_Navigationbar_Item *it;
   Elm_Navigationbar_Item *top_it;

   if (!content) return;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (evas_object_data_get(content, _navigationbar_key)) return;

   it = elm_widget_item_new(obj, Elm_Navigationbar_Item);
   if (!it) return;

   top_it = eina_list_data_get(eina_list_last(wd->stack));

   _title_btn_set(it, prev_btn, ELM_NAVIGATIONBAR_PREV_BUTTON, EINA_FALSE);
   _title_btn_set(it, next_btn, ELM_NAVIGATIONBAR_NEXT_BUTTON, EINA_FALSE);

   it->content = content;
   evas_object_event_callback_add(content, EVAS_CALLBACK_DEL, _content_del, it);
   evas_object_data_set(content, _navigationbar_key, it);

   //Add a prev-button automatically.
   if ((!prev_btn) && (top_it))
   {
      if (top_it->title)
        _title_btn_set(it, _create_back_btn(obj, top_it->title, it), ELM_NAVIGATIONBAR_PREV_BUTTON, EINA_TRUE);
      else
        _title_btn_set(it, _create_back_btn(obj, _ELM_NAVIBAR_PREV_BTN_DEFAULT_LABEL, it), ELM_NAVIGATIONBAR_PREV_BUTTON, EINA_TRUE);
   }

   eina_stringshare_replace(&it->title, title);
   edje_object_part_text_set(wd->base, "elm.text", title);
   _item_sizing_eval(it);

   Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);
   // unswallow items and start transition
   // TODO: For what? why does it need to unswallow?
   if (top_it)
     {
        cb->prev_it = top_it;
        cb->first_page = EINA_FALSE;
        if (top_it->title_obj) edje_object_part_unswallow(wd->base, top_it->title_obj);
        if (top_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
          edje_object_part_unswallow(wd->base, top_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
        if (top_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
          edje_object_part_unswallow(wd->base, top_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);
        if (top_it->icon)
          edje_object_part_unswallow(wd->base, top_it->icon);
     }
   //If page is the first, then do not run the transition... but if user want.. ?
   else
     {
        cb->prev_it = NULL;
        cb->first_page = EINA_TRUE;
     }
   cb->navibar = obj;
   cb->it = it;
   cb->pop = EINA_FALSE;

   _transition_complete_cb(cb);
   free(cb);
   elm_pager_content_push(wd->pager, it->content);

   wd->stack = eina_list_append(wd->stack, it);
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
   Transit_Cb_Data *cb;
   Elm_Navigationbar_Item *it = NULL;
   Elm_Navigationbar_Item *prev_it = NULL;

   //TODO: It's impossible to pop while popping?
   if (wd->popping) return;
   if (!wd->stack) return;

   //find item to be popped and to be shown
   //TODO: hmm.. i think it's hard to manager separated list from elm_pager internal list. but how about use evas_object_data_set to each content??
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        prev_it = ll->data;
        ll = ll->prev;
        if (ll)
          it = ll->data;
     }
   //unswallow items and start trasition
   cb = ELM_NEW(Transit_Cb_Data);

   //Previous page is exist.
   if (prev_it && it)
     {
        cb->prev_it = prev_it;
        cb->it = it;
        cb->pop = EINA_TRUE;
        if (prev_it->title_obj)
          edje_object_part_unswallow(wd->base, prev_it->title_obj);
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
          edje_object_part_unswallow(wd->base, prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
          edje_object_part_unswallow(wd->base, prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);
        if (prev_it->icon)
          edje_object_part_unswallow(wd->base, prev_it->icon);
        _item_sizing_eval(it);
     }
   //This case, it's the last page.
   else if (prev_it)
     {
        cb->prev_it = prev_it;
        cb->it = NULL;
        cb->pop = EINA_TRUE;
        //TODO: seems that flag is inverted.
        cb->first_page = EINA_FALSE;
     }
   cb->navibar = obj;
   _transition_complete_cb(cb);
   wd->popping = EINA_TRUE;

   elm_pager_content_pop(wd->pager);

   if ((prev_it) && (!it))
     edje_object_part_text_set(wd->base, "elm.text", NULL);

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

   Widget_Data *wd;
   Eina_List *ll;
   Elm_Navigationbar_Item *it;
   Elm_Navigationbar_Item *prev_it;
   Transit_Cb_Data *cb;

   wd = elm_widget_data_get(obj);
   if ((!wd) || (!content) || (!wd->stack)) return;

   //find item to be popped and to be shown
   it = prev_it = NULL;
   ll = eina_list_last(wd->stack);
   if (ll)
     {
        prev_it = ll->data;
        ll =  ll->prev;
     }
   while (ll)
     {
        it = ll->data;
        if ((it->base.widget) && (it->content != content))
          {
             _item_del(ll->data);
             wd->stack = eina_list_remove_list(wd->stack, ll);
             it = NULL;
           }
         else
           break;
         ll =  ll->prev;
      }
   if (prev_it && it)
     {
        //unswallow items and start trasition
        cb = ELM_NEW(Transit_Cb_Data);
        cb->prev_it = prev_it;
        cb->it = it;
        cb->pop = EINA_TRUE;
        cb->first_page = EINA_FALSE;
        cb->navibar = obj;

        //TODO: make one call.
        if (prev_it->title_obj)
          edje_object_part_unswallow(wd->base, prev_it->title_obj);
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON])
          edje_object_part_unswallow(wd->base, prev_it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON]);
        if (prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON])
          edje_object_part_unswallow(wd->base, prev_it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON]);

        _item_sizing_eval(it);
        _transition_complete_cb(cb);

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
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   eina_stringshare_replace(&it->title, title);
   edje_object_part_text_set(wd->base, "elm.text", title);
   _item_sizing_eval(it);
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
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return NULL;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;
   return it->title;
}

/**
 * Set the title icon for the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] icon The icon object
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_icon_set(Evas_Object *obj, Evas_Object *content, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;
   Evas_Object *swallow;

   if (!content) return;

    wd = elm_widget_data_get(obj);
    if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   if (it->icon == icon) return;
   if (it->icon) evas_object_del(it->icon);
   it->icon = icon;

   if (!icon) return;

   elm_widget_sub_object_add(obj, icon);
   evas_object_event_callback_add(icon, EVAS_CALLBACK_DEL, _title_icon_del, it);
   _item_sizing_eval(it);
   //update if the content is the top item
   if (elm_navigationbar_content_top_get(obj) != content)
     return;

   swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.icon");
   if (swallow)
     {
        edje_object_signal_emit(wd->base, "elm,state,icon,hidden", "elm");
        edje_object_part_unswallow(wd->base, swallow);
        evas_object_hide(swallow);
     }
   if (wd->title_visible)
     {
        edje_object_part_swallow(wd->base, "elm.swallow.icon", icon);
        edje_object_signal_emit(wd->base, "elm,state,icon,visible", "elm");
        edje_object_message_signal_process(wd->base);
     }
   else
      edje_object_signal_emit(wd->base, "elm,state,icon,hidden", "elm");
}

/**
 * Get the title icon for the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The icon object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_title_icon_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype)NULL;
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return NULL;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;

   return it->icon;
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
//TODO: elm_navigationbar_title_object_set ( .... )
EAPI void
elm_navigationbar_title_object_add(Evas_Object *obj, Evas_Object *content, Evas_Object *title_obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if ((!title_obj) || (!content)) return;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   if (it->title_obj)
     {
       evas_object_event_callback_del(it->title_obj, EVAS_CALLBACK_DEL, _title_obj_del);
       evas_object_del(it->title_obj);
     }

   it->title_obj = title_obj;
   elm_widget_sub_object_add(obj, title_obj);
   evas_object_event_callback_add(title_obj, EVAS_CALLBACK_DEL, _title_obj_del, it);

   //TODO: Conserve this line for a while the object list get API is alive.
   eina_list_free(it->title_obj_list);
   it->title_obj_list = eina_list_append(NULL, title_obj);

   if (elm_navigationbar_content_top_get(obj) != content)
     return;

   edje_object_part_swallow(wd->base, "elm.swallow.title", title_obj);

   //TODO: Looks something incorrect. 
   if (wd->title_visible)
     {
       if (it->title)
         {
            edje_object_signal_emit(wd->base, "elm,state,show,extended", "elm");
            //TODO: for before nbeat?
            edje_object_signal_emit(wd->base, "elm,state,show,noanimate,title", "elm");
            it->titleobj_visible = EINA_TRUE;
         }
     }
   _item_sizing_eval(it);
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
//TODO: remove!
EAPI void
elm_navigationbar_title_object_list_unset(Evas_Object *obj, Evas_Object *content, Eina_List **list)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;
   Evas_Object *swallow;

   if ((!list) || (!content)) return;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   *list = eina_list_append(*list, it->title_obj);
   it->title_obj = NULL;

   if (elm_navigationbar_content_top_get(obj) != content)
     return;

   //In this case, the content is in the last of the stack
   swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.title");
   if (!swallow) return;

   if (wd->title_visible)
    {
      if(it->titleobj_visible)
        {
          //TODO: remove the dependency on these signals as related to nbeat?
          edje_object_signal_emit(wd->base, "elm,state,hide,noanimate,title", "elm");
          it->titleobj_visible = EINA_FALSE;
        }
       edje_object_signal_emit(wd->base, "elm,state,hide,extended", "elm");
   }
   _item_sizing_eval(it);
}

EAPI Evas_Object *
elm_navigationbar_title_object_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return NULL;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;

   return it->title_obj;
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
//TODO: Remove!!
EAPI Eina_List *
elm_navigationbar_title_object_list_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return NULL;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;

   return it->title_obj_list;
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
//TODO: does not provide hidden get ?
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

   wd->title_visible = !hidden;
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

   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   switch(button_type)
     {
      case ELM_NAVIGATIONBAR_PREV_BUTTON:
        _elm_navigationbar_prev_btn_set(obj, content, button, it);
        break;
      case ELM_NAVIGATIONBAR_NEXT_BUTTON:
        _elm_navigationbar_next_btn_set(obj, content, button, it);
        break;
      default:
        _elm_navigationbar_prev_btn_set(obj, content, button, it);
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
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   if (!content) return NULL;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;

   switch(button_type)
     {
        case ELM_NAVIGATIONBAR_PREV_BUTTON:
           return it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON];
        case ELM_NAVIGATIONBAR_NEXT_BUTTON:
           return it->title_btns[ELM_NAVIGATIONBAR_NEXT_BUTTON];
        default:
           return it->title_btns[ELM_NAVIGATIONBAR_PREV_BUTTON];
     }
   return NULL;
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
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return;

   eina_stringshare_replace(&it->subtitle, subtitle);
   edje_object_part_text_set(wd->base, "elm.text.sub", subtitle);
   _item_sizing_eval(it);
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
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Elm_Navigationbar_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return NULL;
   return it->subtitle;
}

/**
 * This disables content area animation on push/pop.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] disable  if EINA_TRUE animation is disabled.
 *
 * @ingroup NavigationBar
 */
//TODO: Let's check to remove this API.
EAPI void
elm_navigationbar_animation_disabled_set(Evas_Object *obj, Eina_Bool disable)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_pager_animation_disabled_set(wd->pager, disable);
}

/**
 * This shows/hides title object area.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] show  if EINA_TRUE title object is shown else its hidden.
 * @param[in] content  The content object packed in navigationbar.
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_object_visible_set(Evas_Object *obj, Evas_Object *content, Eina_Bool visible)
{
    ELM_CHECK_WIDTYPE(obj, widtype);
    Elm_Navigationbar_Item *it;
    Widget_Data *wd;

    if (!content) return;

    wd = elm_widget_data_get(obj);
    if (!wd) return;

    it = evas_object_data_get(content, _navigationbar_key);
    if (!it) return;
    if (it->titleobj_visible == visible) return;
    _titleobj_switching(obj, it);
}

/**
 * This gets the status whether title object is shown/hidden.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content  The content object packed in navigationbar.
 * @return The status whether title object is shown/hidden.
 * @ingroup NavigationBar
 */
Eina_Bool
elm_navigationbar_title_object_visible_get(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Elm_Navigationbar_Item *it;
   Widget_Data *wd;

   if (!content) return EINA_FALSE;

   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   it = evas_object_data_get(content, _navigationbar_key);
   if (!it) return EINA_FALSE;

   return it->titleobj_visible;
}
