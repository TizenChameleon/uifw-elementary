#include <Elementary.h>
#include "elm_priv.h"
#include <stdlib.h>

/**
 * @defgroup Popup Popup
 * @ingroup Elementary
 *
 * This is a popup widget. it can be used to display information/ get information from user.
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *notify;
   Evas_Object *layout;
   const char *title_area;
   Evas_Object *title_icon;
   Evas_Object *content_area;
   Evas_Object *desc_label;
   Evas_Object *action_area;
   Eina_List *button_list;
   Elm_Popup_Mode mode;
   int no_of_buttons;
   Evas_Object *content;
   Elm_Notify_Orient notify_orient;
   Eina_Bool delete_me : 1;
#ifdef HAVE_ELEMENTARY_X
   Ecore_Event_Handler* wnd_map_handler;
#endif
};

typedef struct _Action_Area_Data Action_Area_Data;

struct _Action_Area_Data
{
   Evas_Object *obj;
   Evas_Object *btn;
   int response_id;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _elm_popup_buttons_add_valist(Evas_Object *obj, const char *first_button_text, va_list args);
static Evas_Object* _elm_popup_add_button(Evas_Object *obj, const char *text, int response_id);
static void _action_area_clicked(void *data, Evas_Object *obj, void *event_info);
static void _block_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _state_set_cb(void *data, Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

#ifdef HAVE_ELEMENTARY_X
   if(wd->wnd_map_handler)
     {
        ecore_event_handler_del(wd->wnd_map_handler);
        wd->wnd_map_handler = NULL;
     }
#endif

   free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Action_Area_Data *action_data = NULL;
   Eina_List *list = NULL;

   if (!wd) return;
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_SHOW, _show, NULL);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_HIDE, _hide, NULL);
   evas_object_smart_callback_del(wd->notify, "block,clicked", _block_clicked_cb);
   EINA_LIST_FOREACH(wd->button_list, list, action_data)
     {
        free(action_data);
        action_data = NULL;
     }
   eina_list_free(wd->button_list);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[4096];
   Eina_List *list = NULL;
   Action_Area_Data *action_data = NULL;
   int index =0;

   if (!wd) return;
   elm_layout_theme_set(wd->layout, "popup", "base", elm_widget_style_get(obj));
   elm_notify_orient_set(wd->notify, wd->notify_orient);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   edje_object_message_signal_process(elm_layout_edje_get(wd->layout));
   if (wd->action_area)
     {
        snprintf(buf, sizeof(buf), "buttons%d", wd->no_of_buttons);
        elm_layout_theme_set(wd->action_area, "popup", buf, elm_widget_style_get(obj));
        EINA_LIST_FOREACH(wd->button_list, list, action_data)
          {
             snprintf(buf, sizeof(buf), "popup_button/%s", elm_widget_style_get(obj));
             elm_object_style_set(action_data->btn, buf);
             ++index;
             snprintf(buf, sizeof(buf), "actionbtn%d", index);
             elm_layout_content_set(wd->action_area, buf, action_data->btn);
          }
        elm_layout_content_set(wd->layout, "elm.swallow.buttonArea", wd->action_area);
     }
   if (wd->content_area)
     {
        elm_layout_theme_set(wd->content_area, "popup", "content", elm_widget_style_get(obj));
        if (wd->desc_label)
          {
             snprintf(buf, sizeof(buf), "popup_description/%s", elm_widget_style_get(obj));
             elm_object_style_set(wd->desc_label, buf);
             elm_layout_content_set(wd->content_area, "elm.swallow.content", wd->desc_label);
          }
        else if (wd->content)
          {
             elm_layout_content_set(wd->content_area, "elm.swallow.content", wd->content);
          }
        elm_layout_content_set(wd->layout, "elm.swallow.content", wd->content_area);
     }
   if (wd->title_area)
     {
        edje_object_part_text_set(elm_layout_edje_get(wd->layout), "elm.swallow.title", wd->title_area);
     }
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   if (!wd) return;
   edje_object_size_min_calc(elm_layout_edje_get(wd->layout), &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(data);
}

static void
_block_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get((Evas_Object*)data);

   if (wd->mode == ELM_POPUP_TYPE_ALERT)
     {
        evas_object_hide((Evas_Object*)data);
        evas_object_smart_callback_call((Evas_Object *)data, "response", (void *)ELM_POPUP_RESPONSE_NONE);
     }
}

static Eina_Bool
_wnd_map_notify(void *data, int type, void *event)
{
   Evas* e = NULL;
   Evas_Object* obj = (Evas_Object*)data;
   Widget_Data *wd = elm_widget_data_get((Evas_Object*)data);

   if (obj && wd->wnd_map_handler)
     {
        e = evas_object_evas_get(obj);

        if (e)
          {
             /* Render given object again, previous frame was discarded. */
             evas_render(e);
             ecore_event_handler_del(wd->wnd_map_handler);
             wd->wnd_map_handler = NULL;
             return 1;
          }
     }

   return 0;
}

static void
_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_layout_theme_set(wd->layout, "popup", "base", elm_widget_style_get(obj));
   _sizing_eval(obj);
   evas_object_show(obj);
#ifdef HAVE_ELEMENTARY_X
   if (e && !wd->wnd_map_handler)
     {
        int curr_rmethod = 0;
        int gl_rmethod = 0;

        curr_rmethod = evas_output_method_get(e);
        gl_rmethod = evas_render_method_lookup("gl_x11");

        if (!curr_rmethod) return;
        if (!gl_rmethod) return;
        if (curr_rmethod == gl_rmethod)
           wd->wnd_map_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW, _wnd_map_notify, obj);
     }
#endif
}

static void
_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_hide(obj);
}

static void
_action_area_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Action_Area_Data *adata = data;

   if (!adata) return;
   evas_object_hide(adata->obj);
   evas_object_smart_callback_call(adata->obj, "response", (void *)adata->response_id);
}

static Evas_Object*
_elm_popup_add_button(Evas_Object *obj, const char *text, int response_id)
{
   char buf[4096];
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *btn;
   Action_Area_Data *adata;

   if (!wd) return NULL;
   adata = ELM_NEW(Action_Area_Data);
   btn = elm_button_add(obj);
   snprintf(buf, sizeof(buf), "popup_button/%s", elm_widget_style_get(obj));
   elm_object_style_set(btn, buf);
   elm_button_label_set(btn, text);
   adata->response_id = response_id;
   adata->obj = obj;
   adata->btn = btn;
   wd->button_list = eina_list_append(wd->button_list, adata);
   evas_object_smart_callback_add(btn, "clicked", _action_area_clicked, adata);
   return btn;
}

static void
_elm_popup_buttons_add_valist(Evas_Object *obj, const char *first_button_text, va_list args)
{
   const char *text = NULL;
   char buf[4096];
   int response = 0;
   int index = 0;
   Evas_Object *btn;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (first_button_text == NULL) return;
   text = first_button_text;
   response = va_arg(args, int);
   while (text != NULL)
     {
        btn = _elm_popup_add_button(obj, text, response);
        ++index;
        snprintf(buf, sizeof(buf), "actionbtn%d", index);
        elm_layout_content_set(wd->action_area, buf, btn);
        evas_object_event_callback_add(wd->action_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        text = va_arg(args, char*);
        if (text == NULL) break;
        response = va_arg(args, int);
     }
}

static void
_elm_popup_timeout(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_hide((Evas_Object*)data);
   evas_object_smart_callback_call((Evas_Object *)data, "response", (void *)ELM_POPUP_RESPONSE_TIMEOUT);
}

static Eina_Bool
_elm_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   int res_id  =  ELM_POPUP_RESPONSE_NONE;
   int *id = data;
   *id = res_id;
   ecore_main_loop_quit();
   return EINA_TRUE;
}

static void
response_cb(void *data, Evas_Object *obj, void *event_info)
{
   int res_id = event_info;
   int *id = data;

   *id = res_id;
   ecore_main_loop_quit();
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_object_mirrored_set(wd->notify, rtl);
}

static void
_state_set_cb(void *data, Evas_Object *obj __UNUSED__,
               const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get((Evas_Object*)data);
   if (!wd) return;
   if (wd->layout) elm_layout_sizing_eval(wd->layout);
}

/**
 * Add a new Popup object.
 *
 * @param[in] parent_app The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Popup
 */
EAPI Evas_Object *
elm_popup_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window_Type type;
#endif

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
   ELM_SET_WIDTYPE(widtype, "popup");
   elm_widget_type_set(obj, widtype);
   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->notify = elm_notify_add(parent);
   elm_widget_sub_object_add(obj, wd->notify);
   elm_widget_resize_object_set(obj, wd->notify);
   elm_notify_orient_set(wd->notify, ELM_NOTIFY_ORIENT_CENTER);
   wd->notify_orient = ELM_NOTIFY_ORIENT_CENTER;
   elm_notify_repeat_events_set(wd->notify, EINA_FALSE);
   evas_object_size_hint_weight_set(wd->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->notify, EVAS_HINT_FILL, EVAS_HINT_FILL);

   wd->layout = elm_layout_add(parent);
   evas_object_size_hint_weight_set(wd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_layout_theme_set(wd->layout, "popup", "base", elm_widget_style_get(obj));
   elm_notify_content_set(wd->notify, wd->layout);

   edje_object_signal_callback_add(elm_layout_edje_get(wd->layout), "elm,state,title,visible", "elm", _state_set_cb, obj);
   edje_object_signal_callback_add(elm_layout_edje_get(wd->layout), "elm,state,button,visible", "elm", _state_set_cb, obj);
   edje_object_signal_callback_add(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm", _state_set_cb, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, NULL);
   wd->mode = ELM_POPUP_TYPE_NONE;
   evas_object_smart_callback_add(wd->notify, "block,clicked", _block_clicked_cb, obj);
#ifdef HAVE_ELEMENTARY_X
   ecore_x_netwm_window_type_get(elm_win_xwindow_get(parent), &type);
   if (type == ECORE_X_WINDOW_TYPE_DIALOG)
     {
        elm_object_style_set(wd->notify, "popup");
     }
#endif
   _sizing_eval(obj);

   return obj;
}

/**
 * Add a new Popup object.
 *
 * @param[in] parent The parent object
 * @param[in] title text to be displayed in title area.
 * @param[in] desc_text text to be displayed in description area.
 * @param[in] no_of_buttons Number of buttons to be packed in action area.
 * @param[in] first_button_text button text
 * @param[in] Varargs response ID for first button, then additional buttons followed by response id's ending with NULL
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Popup
 */
EAPI Evas_Object *
elm_popup_with_buttons_add(Evas_Object *parent, const char *title, const char *desc_text,int no_of_buttons, const char *first_button_text, ...)
{
   Evas_Object *popup;
   char buf[4096];
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   popup = elm_popup_add(parent);
   wd = elm_widget_data_get(popup);
   if (desc_text)
     {
        elm_popup_desc_set(popup, desc_text);
     }
   if (title)
     {
        elm_popup_title_label_set(popup, title);
     }
   if (first_button_text)
     {
        va_list args;
        va_start(args, first_button_text);
        wd->action_area = elm_layout_add(popup);
        elm_layout_content_set(wd->layout, "elm.swallow.buttonArea", wd->action_area);
        snprintf(buf,sizeof(buf), "buttons%d", no_of_buttons);
                                wd->no_of_buttons = no_of_buttons;
        elm_layout_theme_set(wd->action_area, "popup", buf, elm_widget_style_get(popup));
        edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,visible", "elm");
        if (wd->title_area)
          {
             edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
          }
        _elm_popup_buttons_add_valist (popup, first_button_text, args);
        va_end(args);
     }
   edje_object_message_signal_process(wd->layout);
   _sizing_eval(popup);

   return popup;
}


/**
 * This Set's the description text in content area of Popup widget.
 *
 * @param[in] obj The Popup object
 * @param[in] text description text.
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_desc_set(Evas_Object *obj, const char *text)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[4096];

   if (!wd) return;
   if (wd->content_area)
     {
        evas_object_del(wd->content_area);
        wd->content_area = NULL;
     }
   wd->content_area = elm_layout_add(obj);
   elm_layout_theme_set(wd->content_area, "popup", "content", elm_widget_style_get(obj));
   wd->desc_label = elm_label_add(obj);
   snprintf(buf, sizeof(buf), "popup_description/%s", elm_widget_style_get(obj));
   elm_object_style_set(wd->desc_label, buf);
   elm_label_line_wrap_set(wd->desc_label, EINA_TRUE);
   elm_label_label_set(wd->desc_label, text);
   evas_object_size_hint_weight_set(wd->desc_label, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(wd->desc_label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(wd->desc_label);
   elm_layout_content_set(wd->content_area, "elm.swallow.content", wd->desc_label);
   elm_layout_content_set(wd->layout, "elm.swallow.content", wd->content_area);
   evas_object_event_callback_add(wd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, obj);
   _sizing_eval(obj);
}

/**
 * This Get's the description text packed in content area of popup object.
 *
 * @param[in] obj The Popup object
 * @return  description text.
 *
 * @ingroup Popup
 */
EAPI const char*
elm_popup_desc_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return elm_label_label_get(wd->desc_label);
}

/**
 * This Set's the title text in title area of popup object.
 *
 * @param[in] obj The popup object
 * @param[in] text The title text
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_title_label_set(Evas_Object *obj, const char *text)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   eina_stringshare_replace(&wd->title_area, text);
   edje_object_part_text_set(elm_layout_edje_get(wd->layout), "elm.swallow.title", text);
   edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,visible", "elm");
   if (wd->action_area)
      edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
   if (wd->title_icon)
      edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,icon,visible", "elm");
   edje_object_message_signal_process(wd->layout);
   _sizing_eval(obj);
}

/**
 * This Get's the title text packed in title area of popup object.
 *
 * @param[in] obj The Popup object
 * @return title text
 *
 * @ingroup Popup
 */
EAPI const char*
elm_popup_title_label_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->title_area;
}

/**
 * This Set's the icon in the title area of Popup object.
 *
 * @param[in] obj The popup object
 * @param[in] icon The title icon
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_title_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->title_icon)
     {
        evas_object_del(wd->title_icon);
        wd->title_icon = NULL;
     }
   wd->title_icon = icon;
   elm_layout_content_set(wd->layout, "elm.swallow.title.icon", wd->title_icon);
   edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,title,icon,visible", "elm");
   edje_object_message_signal_process(wd->layout);
   _sizing_eval(obj);
}

/**
 * This Get's the icon packed in title area of Popup object.
 *
 * @param[in] obj The Popup object
 * @return title icon
 *
 * @ingroup Popup
 */
EAPI Evas_Object*
elm_popup_title_icon_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->title_icon;
}

/**
 * This Set's the content widget in content area of Popup object.
 *
 * @param[in] obj The popup object
 * @param[in] content The content widget
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->content == content) return;
   if (wd->content_area)
     {
        evas_object_del(wd->content_area);
        wd->content_area = NULL;
     }
   wd->content = content;
   if (content)
     {
        wd->content_area = elm_layout_add(obj);
        elm_layout_theme_set(wd->content_area, "popup","content", elm_widget_style_get(obj));
        elm_layout_content_set(wd->content_area, "elm.swallow.content", content);
        elm_layout_content_set(wd->layout, "elm.swallow.content", wd->content_area);
        evas_object_event_callback_add(wd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
     }
   _sizing_eval(obj);
}

/**
 * This Get's the content widget packed in content area of Popup object.
 *
 * @param[in] obj The Popup object
 * @return content packed in popup widget
 *
 * @ingroup Popup
 */
EAPI Evas_Object*
elm_popup_content_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->content;
}

/**
 * Adds the buttons in to the action area of popup object.
 *
 * @param[in] obj The popup object
 * @param[in] no_of_buttons Number of buttons that has to be packed in action area.
 * @param[in] first_button_text   Label of first button
 * @param[in] Varargs  Response ID(Elm_Popup_Response/ any integer value) for first button, then additional buttons along with their response id ending with NULL.
 * @ingroup Popup
 */
EAPI void
elm_popup_buttons_add(Evas_Object *obj,int no_of_buttons, const char *first_button_text,  ...)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[4096];
   va_list args;

   if (!wd) return;
   va_start(args, first_button_text);
   if (wd->action_area)
     {
        evas_object_del(wd->action_area);
        wd->action_area = NULL;
     }
   wd->action_area = elm_layout_add(obj);
   elm_layout_content_set(wd->layout, "elm.swallow.buttonArea", wd->action_area);
   evas_object_size_hint_weight_set(wd->action_area, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->action_area, EVAS_HINT_FILL, EVAS_HINT_FILL);
   snprintf(buf, sizeof(buf), "buttons%d", no_of_buttons);
   elm_layout_theme_set(wd->action_area, "popup", buf, elm_widget_style_get(obj));
   wd->no_of_buttons = no_of_buttons;
   edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,visible", "elm");
   if (wd->title_area)
     edje_object_signal_emit(elm_layout_edje_get(wd->layout), "elm,state,button,title,visible", "elm");
   _elm_popup_buttons_add_valist (obj, first_button_text, args);
   va_end(args);
   edje_object_message_signal_process(wd->layout);
   _sizing_eval(obj);
}

/**
 * This Set's the time before the popup window is hidden,
 * and ELM_POPUP_RESPONSE_TIMEOUT is sent along with response signal.
 *
 * @param[in] obj The popup object
 * @param[in] timeout The timeout value in seconds.
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_timeout_set(Evas_Object *obj, double timeout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_notify_timeout_set(wd->notify, timeout);
   evas_object_smart_callback_add(wd->notify, "timeout", _elm_popup_timeout, obj);
}

/**
 * This Set's the mode of popup, by default ELM_POPUP_TYPE_NONE is set i.e, popup
 * will not close when clicked outside. if ELM_POPUP_TYPE_ALERT is set, popup will close
 * when clicked outside, and ELM_POPUP_RESPONSE_NONE is sent along with response signal.
 *
 * @param[in] obj The popup object
 * @param[in] mode  Elm_Popup_Mode
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_mode_set(Evas_Object *obj, Elm_Popup_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (mode == wd->mode) return;
   wd->mode = mode;
}

/**
 * This Hides the popup by emitting response signal.
 *
 * @param[in] obj The popup object
 * @param[in] response_id  response ID of the signal to be emitted along with response signal
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_response(Evas_Object *obj, int  response_id)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_hide(obj);
   evas_object_smart_callback_call((Evas_Object *)obj, "response", (void *)response_id);
}

/**
 * This API controls the direction from which popup will appear and location of popup.
 * @param[in] obj The popup object
 * @param[in] orient  the orientation of the popup
 *
 * @ingroup Popup
 */
EAPI void
elm_popup_orient_set(Evas_Object *obj, Elm_Popup_Orient orient)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Notify_Orient notify_orient = -1;

   if (!wd) return;
   switch (orient)
     {
      case ELM_POPUP_ORIENT_TOP:
         notify_orient = ELM_NOTIFY_ORIENT_TOP;
         break;
      case ELM_POPUP_ORIENT_CENTER:
         notify_orient = ELM_NOTIFY_ORIENT_CENTER;
         break;
      case ELM_POPUP_ORIENT_BOTTOM:
         notify_orient = ELM_NOTIFY_ORIENT_BOTTOM;
         break;
      case ELM_POPUP_ORIENT_LEFT:
         notify_orient = ELM_NOTIFY_ORIENT_LEFT;
         break;
      case ELM_POPUP_ORIENT_RIGHT:
         notify_orient = ELM_NOTIFY_ORIENT_RIGHT;
         break;
      case ELM_POPUP_ORIENT_TOP_LEFT:
         notify_orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
         break;
      case ELM_POPUP_ORIENT_TOP_RIGHT:
         notify_orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
         break;
      case ELM_POPUP_ORIENT_BOTTOM_LEFT:
         notify_orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
         break;
      case ELM_POPUP_ORIENT_BOTTOM_RIGHT:
         notify_orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
         break;
     }
   wd->notify_orient = notify_orient;
   elm_notify_orient_set(wd->notify, notify_orient);
}

/**
 * Blocks in a main loop until popup either emits response signal or is exited due
 * to exit signal, when exit signal is received dialog responds with ELM_POPUP_RESPONSE_NONE
 * response ID else returns the response ID from response signal emission.
 * before entering the main loop popup calls evas_object_show on the popup for you.
 * you can force popup to return at any time by calling elm_popup_responsec to emit the
 * response signal. destroying the popup during elm_popup_run is a very bad idea.
 * typical usage of this function may be
 * int result = elm_popup_run(popup);
 * switch(result)
 * {
 * case ELM_POPUP_RESPONSE_OK:
 * do_something_specific_to_app();
 * evas_object_del(popup);
 * break;
 * case ELM_POPUP_RESPONSE_CANCEL:
 * do_nothing_popup_was_cancelled();
 * evas_object_del(popup);
 * break;
 * case ELM_POPUP_RESPONSE_NONE:
 * default:
 * evas_object_del(popup);
 * elm_exit();
 * }
 * do not run elm_popup_run in a timer/idler callback.
 * when popup returns with signal ELM_POPUP_RESPONSE_NONE, then exit the application using elm_exit
 * by calling any post exit application code.
 *
 * @param[in] obj The popup object
 * @ingroup Popup
 */
EAPI int
elm_popup_run(Evas_Object *obj)
{
   int response_id=0;
   Ecore_Event_Handler *_elm_exit_handler = NULL;

   /*Finger waggle warning*/
   _elm_dangerous_call_check(__FUNCTION__);
   evas_object_show(obj);
   evas_object_smart_callback_add(obj, "response", response_cb, &response_id);
   _elm_exit_handler = ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _elm_signal_exit, &response_id);
   ecore_main_loop_begin();
   if (_elm_exit_handler)
     {
        ecore_event_handler_del(_elm_exit_handler);
        _elm_exit_handler = NULL;
     }
   return response_id;
}
