#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *chk, *icon;
   Eina_Bool state;
   Eina_Bool *statep;
   const char *label;
   const char *ontext, *offtext;
};

static const char *widtype = NULL;
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj,
                                void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _signal_check_off(void *data, Evas_Object *obj,
                              const char *emission, const char *source);
static void _signal_check_on(void *data, Evas_Object *obj,
                             const char *emission, const char *source);
static void _signal_check_toggle(void *data, Evas_Object *obj,
                                 const char *emission, const char *source);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _activate_hook(Evas_Object *obj);
static void _content_set_hook(Evas_Object *obj, const char *part,
                              Evas_Object *content);
static Evas_Object *_content_get_hook(const Evas_Object *obj, const char *part);
static Evas_Object *_content_unset_hook(Evas_Object *obj, const char *part);
static void _activate(Evas_Object *obj);
static const char SIG_CHANGED[] = "changed";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CHANGED, ""},
       {NULL, NULL}
};

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__,
            Evas_Callback_Type type, void *event_info)
{
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if ((strcmp(ev->keyname, "Return")) &&
       (strcmp(ev->keyname, "KP_Enter")) &&
       (strcmp(ev->keyname, "space")))
     return EINA_FALSE;
   _activate(obj);
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->label) eina_stringshare_del(wd->label);
   if (wd->ontext) eina_stringshare_del(wd->ontext);
   if (wd->offtext) eina_stringshare_del(wd->offtext);
   free(wd);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->chk, "elm,action,focus", "elm");
        evas_object_focus_set(wd->chk, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(wd->chk, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->chk, EINA_FALSE);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_mirrored_set(wd->chk, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   unsigned int counter = 0;
   unsigned int i = 1;
   unsigned int length = 0;
   const char *str = NULL;
   char labels[128] ;
   char buffer[PATH_MAX]={'\0',};
   char s1[PATH_MAX] = {'\0',};
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _elm_theme_object_set(obj, wd->chk, "check", "base", elm_widget_style_get(obj));
   if (wd->icon)
     edje_object_signal_emit(wd->chk, "elm,state,icon,visible", "elm");
   else
     edje_object_signal_emit(wd->chk, "elm,state,icon,hidden", "elm");
   if (wd->state)
     edje_object_signal_emit(wd->chk, "elm,state,check,on", "elm");
   else
     edje_object_signal_emit(wd->chk, "elm,state,check,off", "elm");
   if (wd->label)
     edje_object_signal_emit(wd->chk, "elm,state,text,visible", "elm");
   else
     edje_object_signal_emit(wd->chk, "elm,state,text,hidden", "elm");
   edje_object_part_text_escaped_set(wd->chk, "elm.text", wd->label);
   edje_object_part_text_escaped_set(wd->chk, "elm.ontext", wd->ontext);
   edje_object_part_text_escaped_set(wd->chk, "elm.offtext", wd->offtext);
   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->chk, "elm,state,disabled", "elm");
   edje_object_message_signal_process(wd->chk);
   edje_object_scale_set(wd->chk, elm_widget_scale_get(obj) * _elm_config->scale);
   //introduced internationalization of additional text parts used in style
   while (1)
     {
        // s1 is  used  to store part name while buffer is used to store the part's value string
        snprintf(labels,sizeof(labels),"label_%d",i++);
        str = edje_object_data_get(wd->chk,labels);
        if (!str) break;
        length = strlen(str);
        while ((str[counter]!= ' ') && (counter < length))
          counter++;
        if (counter == length)
          continue;
        strncpy(s1, str, counter);
        s1[counter] = '\0';
        strncpy(buffer, str + counter, sizeof(buffer));
        edje_object_part_text_set(wd->chk, s1, E_(buffer));
     }
   _sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->chk, "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(wd->chk, "elm,state,enabled", "elm");
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->chk, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (obj != wd->icon) return;
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (!wd) return;
   if (sub == wd->icon)
     {
        edje_object_signal_emit(wd->chk, "elm,state,icon,hidden", "elm");
        evas_object_event_callback_del_full(sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _changed_size_hints, obj);
        wd->icon = NULL;
        _sizing_eval(obj);
        edje_object_message_signal_process(wd->chk);
     }
}

static void
_signal_check_off(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->state = EINA_FALSE;
   if (wd->statep) *wd->statep = wd->state;
   edje_object_signal_emit(wd->chk, "elm,state,check,off", "elm");
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
}

static void
_signal_check_on(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->state = EINA_TRUE;
   if (wd->statep) *wd->statep = wd->state;
   edje_object_signal_emit(wd->chk, "elm,state,check,on", "elm");
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
}

static void
_signal_check_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _activate(data);
}

static void
_activate_hook(Evas_Object *obj)
{
   _activate(obj);
}

static void
_signal_emit_hook(Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   edje_object_signal_emit(wd->chk, emission, source);
}

static void
_signal_callback_add_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_callback_add(wd->chk, emission, source, func_cb, data);
}

static void
_signal_callback_del_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_signal_callback_del_full(wd->chk, emission, source, func_cb,
                                        data);
}

static void
_content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;

   if (part && strcmp(part, "icon")) return;
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->icon == content) return;
   if (wd->icon) evas_object_del(wd->icon);
   wd->icon = content;
   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_part_swallow(wd->chk, "elm.swallow.content", content);
        edje_object_signal_emit(wd->chk, "elm,state,icon,visible", "elm");
        edje_object_message_signal_process(wd->chk);
     }
   _sizing_eval(obj);
}

static Evas_Object *
_content_get_hook(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;

   if (part && strcmp(part, "icon")) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon;
}

static Evas_Object *
_content_unset_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;

   if (part && strcmp(part, "icon")) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!wd->icon) return NULL;
   Evas_Object *icon = wd->icon;
   elm_widget_sub_object_del(obj, wd->icon);
   edje_object_part_unswallow(wd->chk, icon);
   return icon;
}

static void
_activate(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((_elm_config->access_mode == ELM_ACCESS_MODE_OFF) ||
       (_elm_access_2nd_click_timeout(obj)))
     {
        wd->state = !wd->state;
        if (wd->statep) *wd->statep = wd->state;
        if (wd->state)
          {
             edje_object_signal_emit(wd->chk, "elm,state,check,on", "elm");
             if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
               {
                  if (!wd->ontext)
                    {
                       _elm_access_say(E_("State: On"));
                    }
                  else
                     _elm_access_say(E_("State: On"));
               }
          }
        else
          {
             edje_object_signal_emit(wd->chk, "elm,state,check,off", "elm");
             if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
               {
                  if (!wd->offtext)
                    {
                       _elm_access_say(E_("State: Off"));
                    }
                  else
                     _elm_access_say(E_("State: Off"));
               }
          }
        evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
     }
}

static void
_elm_check_label_set(Evas_Object *obj, const char *item, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if ((!item) || (!strcmp(item, "default")))
     {
        eina_stringshare_replace(&wd->label, label);
        if (label)
           edje_object_signal_emit(wd->chk, "elm,state,text,visible", "elm");
        else
           edje_object_signal_emit(wd->chk, "elm,state,text,hidden", "elm");
        edje_object_message_signal_process(wd->chk);
        edje_object_part_text_escaped_set(wd->chk, "elm.text", label);
     }
   else if ((item) && (!strcmp(item, "on")))
     {
        eina_stringshare_replace(&wd->ontext, label);
        edje_object_part_text_escaped_set(wd->chk, "elm.ontext", wd->ontext);
     }
   else if ((item) && (!strcmp(item, "off")))
     {
        eina_stringshare_replace(&wd->offtext, label);
        edje_object_part_text_escaped_set(wd->chk, "elm.offtext", wd->offtext);
     }
   _sizing_eval(obj);
}

static const char *
_elm_check_label_get(const Evas_Object *obj, const char *item)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if ((!item) || (!strcmp(item, "default")))
      return wd->label;
   else if ((item) && (!strcmp(item, "on")))
      return wd->ontext;
   else if ((item) && (!strcmp(item, "off")))
      return wd->offtext;
   return NULL;
}

static char *
_access_info_cb(void *data __UNUSED__, Evas_Object *obj, Elm_Widget_Item *item __UNUSED__)
{
   const char *txt = elm_widget_access_info_get(obj);
   if (!txt) txt = _elm_check_label_get(obj, NULL);
   if (txt) return strdup(txt);
   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj, Elm_Widget_Item *item __UNUSED__)
{
   Evas_Object *o = data;
   Widget_Data *wd = elm_widget_data_get(o);
   if (!wd) return NULL;
   if (elm_widget_disabled_get(obj))
     return strdup(E_("State: Disabled"));
   if (wd->state)
     {
        if (wd->ontext)
          {
             char buf[1024];

             snprintf(buf, sizeof(buf), "%s: %s", E_("State"), wd->ontext);
             return strdup(buf);
          }
        else
           return strdup(E_("State: On"));
     }
   if (wd->offtext)
     {
        char buf[1024];

        snprintf(buf, sizeof(buf), "%s: %s", E_("State"), wd->offtext);
        return strdup(buf);
     }
   return strdup(E_("State: Off"));
}

EAPI Evas_Object *
elm_check_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "check");
   elm_widget_type_set(obj, "check");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_activate_hook_set(obj, _activate_hook);
   elm_widget_event_hook_set(obj, _event_hook);
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);
   elm_widget_signal_callback_add_hook_set(obj, _signal_callback_add_hook);
   elm_widget_signal_callback_del_hook_set(obj, _signal_callback_del_hook);
   elm_widget_text_set_hook_set(obj, _elm_check_label_set);
   elm_widget_text_get_hook_set(obj, _elm_check_label_get);
   elm_widget_content_set_hook_set(obj, _content_set_hook);
   elm_widget_content_get_hook_set(obj, _content_get_hook);
   elm_widget_content_unset_hook_set(obj, _content_unset_hook);

   wd->chk = edje_object_add(e);
   _elm_theme_object_set(obj, wd->chk, "check", "base", "default");
   edje_object_signal_callback_add(wd->chk, "elm,action,check,on", "",
                                   _signal_check_on, obj);
   edje_object_signal_callback_add(wd->chk, "elm,action,check,off", "",
                                   _signal_check_off, obj);
   edje_object_signal_callback_add(wd->chk, "elm,action,check,toggle", "",
                                   _signal_check_toggle, obj);
   elm_widget_resize_object_set(obj, wd->chk);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _sizing_eval(obj);

   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   _elm_access_object_register(obj, wd->chk);
   _elm_access_text_set(_elm_access_object_get(obj),
                        ELM_ACCESS_TYPE, E_("Check"));
   _elm_access_callback_set(_elm_access_object_get(obj),
                            ELM_ACCESS_INFO, _access_info_cb, obj);
   _elm_access_callback_set(_elm_access_object_get(obj),
                            ELM_ACCESS_STATE, _access_state_cb, obj);
   return obj;
}

EAPI void
elm_check_state_set(Evas_Object *obj, Eina_Bool state)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (state != wd->state)
     {
        wd->state = state;
        if (wd->statep) *wd->statep = wd->state;
        if (wd->state)
          edje_object_signal_emit(wd->chk, "elm,state,check,on", "elm");
        else
          edje_object_signal_emit(wd->chk, "elm,state,check,off", "elm");
     }
   edje_object_message_signal_process(wd->chk);
}

EAPI Eina_Bool
elm_check_state_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->state;
}

EAPI void
elm_check_state_pointer_set(Evas_Object *obj, Eina_Bool *statep)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (statep)
     {
        wd->statep = statep;
        if (*wd->statep != wd->state)
          {
             wd->state = *wd->statep;
             if (wd->state)
               edje_object_signal_emit(wd->chk, "elm,state,check,on", "elm");
             else
               edje_object_signal_emit(wd->chk, "elm,state,check,off", "elm");
          }
     }
   else
     wd->statep = NULL;
}
