#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *lbl;
<<<<<<< HEAD
   Evas_Object *bg;
=======
>>>>>>> remotes/origin/upstream
   const char *label;
   const char *format;
   Ecore_Job *deferred_recalc_job;
   double slide_duration;
   Evas_Coord lastw;
   Evas_Coord wrap_w;
<<<<<<< HEAD
   Evas_Coord wrap_h;
   Elm_Wrap_Type linewrap;
   Eina_Bool changed : 1;
   Eina_Bool bgcolor : 1;
=======
   Elm_Wrap_Type linewrap;
   Eina_Bool changed : 1;
>>>>>>> remotes/origin/upstream
   Eina_Bool ellipsis : 1;
   Eina_Bool slidingmode : 1;
   Eina_Bool slidingellipsis : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static int _get_value_in_key_string(const char *oldstring, const char *key, char **value);
static int _strbuf_key_value_replace(Eina_Strbuf *srcbuf, const char *key, const char *value, int deleteflag);
static int _stringshare_key_value_replace(const char **srcstring, const char *key, const char *value, int deleteflag);
<<<<<<< HEAD
static int _is_width_over(Evas_Object *obj);
static void _ellipsis_label_to_width(Evas_Object *obj);
static void _label_sliding_change(Evas_Object *obj);
=======
static void _label_sliding_change(Evas_Object *obj);
static void _label_format_set(Evas_Object *obj, const char *format);
>>>>>>> remotes/origin/upstream

static void
_elm_recalc_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord minw = -1, minh = -1;
<<<<<<< HEAD
   Evas_Coord resw, resh;
   if (!wd) return;
   evas_event_freeze(evas_object_evas_get(data));
   wd->deferred_recalc_job = NULL;
   evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
   if (wd->wrap_w > resw)
     resw = wd->wrap_w;
   if (wd->wrap_h > resh)
     resh = wd->wrap_h;

   if (wd->wrap_h == -1) /* open source routine  */
     {
        edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, resw, 0);
        /* This is a hack to workaround the way min size hints are treated.
         * If the minimum width is smaller than the restricted width, it means
         * the mininmum doesn't matter. */
        if ((minw <= resw) && (minw != wd->wrap_w))
          {
             Evas_Coord ominw = -1;
             evas_object_size_hint_min_get(data, &ominw, NULL);
             minw = ominw;
          }
     }
   else /* ellipsis && linewrap && wrap_height_set routine */
     {
        edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, 0, resh);
        if ((minh <= resh) && (minh != wd->wrap_h))
          {
             Evas_Coord ominh = -1;
             evas_object_size_hint_min_get(data, NULL, &ominh);
             minh = ominh;
          }
        evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
        minw = resw;
        if (minh > wd->wrap_h)
          minh = wd->wrap_h;

     }
   evas_object_size_hint_min_set(data, minw, minh);
   evas_object_size_hint_max_set(data, wd->wrap_w, wd->wrap_h);

   if ((wd->ellipsis) && (wd->linewrap) && (wd->wrap_h > 0) &&
       (_is_width_over(data) == 1))
     _ellipsis_label_to_width(data);
=======
   Evas_Coord resw;
   if (!wd) return;
   evas_event_freeze(evas_object_evas_get(data));
   wd->deferred_recalc_job = NULL;
   evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, NULL);
   if (wd->wrap_w > resw)
      resw = wd->wrap_w;

   edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, resw, 0);
   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it means
    * the mininmum doesn't matter. */
   if ((minw <= resw) && (minw != wd->wrap_w))
     {
        Evas_Coord ominw = -1;
        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }
   evas_object_size_hint_min_set(data, minw, minh);
>>>>>>> remotes/origin/upstream
   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_event_freeze(evas_object_evas_get(obj));
   if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
   if (wd->label) eina_stringshare_del(wd->label);
<<<<<<< HEAD
   if (wd->bg) evas_object_del(wd->bg);
=======
>>>>>>> remotes/origin/upstream
   free(wd);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static void
_theme_change(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   _elm_theme_object_set(obj, wd->lbl, "label", "base",
         elm_widget_style_get(obj));
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_mirrored_set(wd->lbl, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_event_freeze(evas_object_evas_get(obj));
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   _theme_change(obj);
<<<<<<< HEAD
   edje_object_part_text_style_user_set(wd->lbl, "elm.text", wd->format);
=======
   _label_format_set(wd->lbl, wd->format);
>>>>>>> remotes/origin/upstream
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   edje_object_scale_set(wd->lbl, elm_widget_scale_get(obj) *
                         _elm_config->scale);
   _label_sliding_change(obj);
   _sizing_eval(obj);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;
   if (!wd) return;

   if (wd->linewrap)
     {
        evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
        if ((resw == wd->lastw) && (!wd->changed)) return;
        wd->changed = EINA_FALSE;
        wd->lastw = resw;
        _elm_recalc_job(obj);
        // FIXME: works ok. but NOT for genlist. what should genlist do?
        //        if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
        //        wd->deferred_recalc_job = ecore_job_add(_elm_recalc_job, obj);
     }
   else
     {
        evas_event_freeze(evas_object_evas_get(obj));
        evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
        edje_object_size_min_calc(wd->lbl, &minw, &minh);
        if (wd->wrap_w > 0 && minw > wd->wrap_w) minw = wd->wrap_w;
<<<<<<< HEAD
        if (wd->wrap_h > 0 && minh > wd->wrap_h) minh = wd->wrap_h;
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_object_size_hint_max_set(obj, wd->wrap_w, wd->wrap_h);
        if ((wd->ellipsis) && (_is_width_over(obj) == 1))
          _ellipsis_label_to_width(obj);
=======
        evas_object_size_hint_min_set(obj, minw, minh);
>>>>>>> remotes/origin/upstream
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }
}

static void
_lbl_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (wd->linewrap) _sizing_eval(data);
}

<<<<<<< HEAD
static int
_get_value_in_key_string(const char *oldstring, const char *key, char **value)
{
   char *curlocater, *starttag, *endtag;
=======
static void
_label_format_set(Evas_Object *obj, const char *format)
{
   if (format)
      edje_object_part_text_style_user_push(obj, "elm.text", format);
   else
      edje_object_part_text_style_user_pop(obj, "elm.text");
}

static int
_get_value_in_key_string(const char *oldstring, const char *key, char **value)
{
   char *curlocater, *endtag;
>>>>>>> remotes/origin/upstream
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        int key_len = strlen(key);
<<<<<<< HEAD
        starttag = curlocater;
=======
>>>>>>> remotes/origin/upstream
        endtag = curlocater + key_len;
        if ((!endtag) || (*endtag != '='))
          {
             foundflag = 0;
             return -1;
          }
        firstindex = abs(oldstring - curlocater);
        firstindex += key_len + 1; // strlen("key") + strlen("=")
        *value = (char *)oldstring + firstindex;

<<<<<<< HEAD
        while (oldstring != starttag)
          {
             if (*starttag == '>')
               {
                  foundflag = 0;
                  break;
               }
             if (*starttag == '<')
               break;
             else
               starttag--;
             if (!starttag) break;
          }

        while (endtag)
          {
             if (*endtag == '<')
               {
                  foundflag = 0;
                  break;
               }
             if (*endtag == '>')
               break;
             else
               endtag++;
             if (!endtag) break;
          }

        if ((foundflag) && (*starttag == '<') && (*endtag == '>'))
          foundflag = 1;
        else
          foundflag = 0;
=======
        foundflag = 1;
>>>>>>> remotes/origin/upstream
     }
   else
     {
        foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

<<<<<<< HEAD
=======

>>>>>>> remotes/origin/upstream
static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf, const char *key, const char *value, int deleteflag)
{
   char *kvalue;
   const char *srcstring = NULL;

   srcstring = eina_strbuf_string_get(srcbuf);

   if (_get_value_in_key_string(srcstring, key, &kvalue) == 0)
     {
        const char *val_end;
        int val_end_idx = 0;
        int key_start_idx = 0;
        val_end = strchr(kvalue, ' ');

        if (val_end)
           val_end_idx = val_end - srcstring;
        else
           val_end_idx = kvalue - srcstring + strlen(kvalue) - 1;

        /* -1 is because of the '=' */
        key_start_idx = kvalue - srcstring - 1 - strlen(key);
        eina_strbuf_remove(srcbuf, key_start_idx, val_end_idx);
        if (!deleteflag)
          {
             eina_strbuf_insert_printf(srcbuf, "%s=%s", key_start_idx, key,
                   value);
          }
     }
   else if (!deleteflag)
     {
        if (*srcstring)
          {
             /* -1 because we want it before the ' */
             eina_strbuf_insert_printf(srcbuf, " %s=%s",
                   eina_strbuf_length_get(srcbuf) - 1, key, value);
          }
        else
          {
             eina_strbuf_append_printf(srcbuf, "DEFAULT='%s=%s'", key, value);
          }
     }
   return 0;
}

static int
_stringshare_key_value_replace(const char **srcstring, const char *key, const char *value, int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;

   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}

<<<<<<< HEAD
static int
_is_width_over(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;

   evas_event_freeze(evas_object_evas_get(obj));
   edje_object_part_geometry_get(wd->lbl, "elm.text", &x, &y, NULL, NULL);
   /* Calc the formatted size with ellipsis turned off */
   if (wd->ellipsis)
     {
        const Evas_Object *tb;
        char *_kvalue;
        double ellipsis = 0.0;
        Eina_Bool found_key = EINA_FALSE;
        if (_get_value_in_key_string(wd->format, "ellipsis", &_kvalue) == 0)
          {
             ellipsis = atof(_kvalue);
             found_key = EINA_TRUE;
          }

        if (_stringshare_key_value_replace(&wd->format,
                 "ellipsis", NULL, 1) == 0)
          {
             edje_object_part_text_style_user_set(wd->lbl, "elm.text",
                   wd->format);
             edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
          }

        tb = edje_object_part_object_get(wd->lbl, "elm.text");
        evas_object_textblock_size_formatted_get(tb, &w, &h);

        if (found_key)
          {
             Eina_Strbuf *elpbuf;
             elpbuf = eina_strbuf_new();
             eina_strbuf_append_printf(elpbuf, "%f", ellipsis);
             if (_stringshare_key_value_replace(&wd->format, "ellipsis",
                      eina_strbuf_string_get(elpbuf), 0) == 0)
               {
                  edje_object_part_text_style_user_set(wd->lbl, "elm.text",
                        wd->format);
                  edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
               }
             eina_strbuf_free(elpbuf);
          }
     }
   else
     {
        const Evas_Object *tb;
        tb = edje_object_part_object_get(wd->lbl, "elm.text");
        evas_object_textblock_size_formatted_get(tb, &w, &h);
     }
   evas_object_geometry_get(obj, &vx, &vy, &vw, &vh);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   if (w > wd->wrap_w || h > wd->wrap_h)
      return 1;

   return 0;
}

static void
_ellipsis_fontsize_set(Evas_Object *obj, int fontsize)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *fontbuf = NULL;
   int removeflag = 0;
   if (!wd) return;

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%d", fontsize);
   if (fontsize == 0) removeflag = 1;  // remove fontsize tag

   if (_stringshare_key_value_replace(&wd->format, "font_size", eina_strbuf_string_get(fontbuf), removeflag) == 0)
     {
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
     }
   eina_strbuf_free(fontbuf);
}

static void
_ellipsis_label_to_width(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   int cur_fontsize = 0;
   char *kvalue;
   const char *minfont, *deffont, *maxfont;
   int minfontsize, maxfontsize;

   evas_event_freeze(evas_object_evas_get(obj));
   minfont = edje_object_data_get(wd->lbl, "min_font_size");
   if (minfont) minfontsize = atoi(minfont);
   else minfontsize = 1;
   maxfont = edje_object_data_get(wd->lbl, "max_font_size");
   if (maxfont) maxfontsize = atoi(maxfont);
   else maxfontsize = 1;
   deffont = edje_object_data_get(wd->lbl, "default_font_size");
   if (deffont) cur_fontsize = atoi(deffont);
   else cur_fontsize = 1;
   if (minfontsize > maxfontsize || cur_fontsize == 1) return;  // theme is not ready for ellipsis
   if (eina_stringshare_strlen(wd->label) <= 0) return;

   if (_get_value_in_key_string(wd->format, "font_size", &kvalue) == 0)
     {
        if (kvalue != NULL) cur_fontsize = atoi(kvalue);
     }

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

=======
>>>>>>> remotes/origin/upstream
static void
_label_sliding_change(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   char *plaintxt;
   int plainlen = 0;

   // dosen't support multiline sliding effect
   if (wd->linewrap)
     {
        wd->slidingmode = EINA_FALSE;
        return;
     }

   plaintxt = _elm_util_mkup_to_text(edje_object_part_text_get(wd->lbl, "elm.text"));
   if (plaintxt != NULL)
     {
        plainlen = strlen(plaintxt);
        free(plaintxt);
     }
   // too short to slide label
   if (plainlen < 1)
     {
        wd->slidingmode = EINA_TRUE;
        return;
     }

   if (wd->slidingmode)
     {
        Edje_Message_Float_Set *msg = alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

        if (wd->ellipsis)
          {
             wd->slidingellipsis = EINA_TRUE;
             elm_label_ellipsis_set(obj, EINA_FALSE);
          }

        msg->count = 1;
        msg->val[0] = wd->slide_duration;

        edje_object_message_send(wd->lbl, EDJE_MESSAGE_FLOAT_SET, 0, msg);
        edje_object_signal_emit(wd->lbl, "elm,state,slide,start", "elm");
     }
   else
     {
        edje_object_signal_emit(wd->lbl, "elm,state,slide,stop", "elm");
        if (wd->slidingellipsis)
          {
             wd->slidingellipsis = EINA_FALSE;
             elm_label_ellipsis_set(obj, EINA_TRUE);
          }
     }
}

static void
_elm_label_label_set(Evas_Object *obj, const char *item, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (item && strcmp(item, "default")) return;
   if (!label) label = "";
   eina_stringshare_replace(&wd->label, label);
<<<<<<< HEAD
   edje_object_part_text_style_user_set(wd->lbl, "elm.text",
         wd->format);
=======
   _label_format_set(wd->lbl, wd->format);
>>>>>>> remotes/origin/upstream
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   wd->changed = 1;
   _sizing_eval(obj);
}

static const char *
_elm_label_label_get(const Evas_Object *obj, const char *item)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (item && strcmp(item, "default")) return NULL;
   if (!wd) return NULL;
   return wd->label;
}

static void
_translate_hook(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, "language,changed", NULL);
}

EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "label");
   elm_widget_type_set(obj, "label");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_text_set_hook_set(obj, _elm_label_label_set);
   elm_widget_text_get_hook_set(obj, _elm_label_label_get);
   elm_widget_translate_hook_set(obj, _translate_hook);

<<<<<<< HEAD
   wd->bgcolor = EINA_FALSE;
   wd->bg = evas_object_rectangle_add(e);
   evas_object_color_set(wd->bg, 0, 0, 0, 0);

=======
>>>>>>> remotes/origin/upstream
   wd->linewrap = ELM_WRAP_NONE;
   wd->ellipsis = EINA_FALSE;
   wd->slidingmode = EINA_FALSE;
   wd->slidingellipsis = EINA_FALSE;
   wd->wrap_w = -1;
<<<<<<< HEAD
   wd->wrap_h = -1;
=======
>>>>>>> remotes/origin/upstream
   wd->slide_duration = 10;

   wd->lbl = edje_object_add(e);
   _elm_theme_object_set(obj, wd->lbl, "label", "base", "default");
   wd->format = eina_stringshare_add("");
   wd->label = eina_stringshare_add("<br>");
<<<<<<< HEAD

   edje_object_part_text_style_user_set(wd->lbl, "elm.text",
         wd->format);
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   elm_widget_resize_object_set(obj, wd->lbl);
=======
   _label_format_set(wd->lbl, wd->format);
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);

   elm_widget_resize_object_set(obj, wd->lbl);

>>>>>>> remotes/origin/upstream
   evas_object_event_callback_add(wd->lbl, EVAS_CALLBACK_RESIZE, _lbl_resize, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   wd->changed = 1;
   _sizing_eval(obj);
   return obj;
}

EAPI void
elm_label_label_set(Evas_Object *obj, const char *label)
{
  _elm_label_label_set(obj, NULL, label);
}

EAPI const char *
elm_label_label_get(const Evas_Object *obj)
{
  return _elm_label_label_get(obj, NULL);
}

EAPI void
elm_label_line_wrap_set(Evas_Object *obj, Elm_Wrap_Type wrap)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *wrap_str;
   int len;

   if (!wd) return;
   if (wd->linewrap == wrap) return;
   wd->linewrap = wrap;
   len = strlen(wd->label);
   if (len <= 0) return;

   switch (wrap)
     {
      case ELM_WRAP_CHAR:
         wrap_str = "char";
         break;
      case ELM_WRAP_WORD:
         wrap_str = "word";
         break;
      case ELM_WRAP_MIXED:
         wrap_str = "mixed";
         break;
      default:
         wrap_str = "none";
         break;
     }

   if (_stringshare_key_value_replace(&wd->format,
            "wrap", wrap_str, 0) == 0)
     {
<<<<<<< HEAD
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
=======
        _label_format_set(wd->lbl, wd->format);
>>>>>>> remotes/origin/upstream
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
        wd->changed = 1;
        _sizing_eval(obj);
     }
}

EAPI Elm_Wrap_Type
elm_label_line_wrap_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->linewrap;
}

EAPI void
elm_label_wrap_width_set(Evas_Object *obj, Evas_Coord w)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (w < 0) w = 0;
   if (wd->wrap_w == w) return;
   if (wd->ellipsis)
     {
<<<<<<< HEAD
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
=======
        _label_format_set(wd->lbl, wd->format);
>>>>>>> remotes/origin/upstream
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
     }
   wd->wrap_w = w;
   _sizing_eval(obj);
}

EAPI Evas_Coord
elm_label_wrap_width_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->wrap_w;
}

<<<<<<< HEAD
EAPI void
elm_label_wrap_height_set(Evas_Object *obj,
                          Evas_Coord   h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (h < 0) h = 0;
   if (wd->wrap_h == h) return;
   if (wd->ellipsis)
     {
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
     }
   wd->wrap_h = h;
   _sizing_eval(obj);
}

EAPI Evas_Coord
elm_label_wrap_height_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->wrap_h;
}

EAPI void
elm_label_fontsize_set(Evas_Object *obj, int fontsize)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;

   if (!wd) return;
   _elm_dangerous_call_check(__FUNCTION__);
   len = strlen(wd->label);
   if (len <= 0) return;
   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%d", fontsize);

   if (fontsize == 0) removeflag = 1;  // remove fontsize tag

   if (_stringshare_key_value_replace(&wd->format, "font_size", eina_strbuf_string_get(fontbuf), removeflag) == 0)
     {
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
        wd->changed = 1;
        _sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

EAPI void
elm_label_text_align_set(Evas_Object *obj, const char *alignmode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int len;

   if (!wd) return;
   _elm_dangerous_call_check(__FUNCTION__);
   len = strlen(wd->label);
   if (len <= 0) return;

   if (_stringshare_key_value_replace(&wd->format, "align", alignmode, 0) == 0)
     {
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
     }

   wd->changed = 1;
   _sizing_eval(obj);
}

EAPI void
elm_label_text_color_set(Evas_Object *obj,
                         unsigned int r,
                         unsigned int g,
                         unsigned int b,
                         unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *colorbuf = NULL;
   int len;

   if (!wd) return;
   _elm_dangerous_call_check(__FUNCTION__);
   len = strlen(wd->label);
   if (len <= 0) return;
   colorbuf = eina_strbuf_new();
   eina_strbuf_append_printf(colorbuf, "#%02X%02X%02X%02X", r, g, b, a);

   if (_stringshare_key_value_replace(&wd->format, "color", eina_strbuf_string_get(colorbuf), 0) == 0)
     {
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
        wd->changed = 1;
        _sizing_eval(obj);
     }
   eina_strbuf_free(colorbuf);
}

EAPI void
elm_label_background_color_set(Evas_Object *obj,
                               unsigned int r,
                               unsigned int g,
                               unsigned int b,
                               unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_color_set(wd->bg, r, g, b, a);

   if (!wd) return;
   _elm_dangerous_call_check(__FUNCTION__);
   if (wd->bgcolor == EINA_FALSE)
     {
        wd->bgcolor = 1;
        edje_object_part_swallow(wd->lbl, "label.swallow.background", wd->bg);
     }
=======
EINA_DEPRECATED EAPI void
elm_label_wrap_height_set(Evas_Object *obj __UNUSED__,
                          Evas_Coord   h __UNUSED__)
{
   return;
}

EINA_DEPRECATED EAPI Evas_Coord
elm_label_wrap_height_get(const Evas_Object *obj __UNUSED__)
{
   return 0;
}

EINA_DEPRECATED EAPI void
elm_label_fontsize_set(Evas_Object *obj __UNUSED__,
                       int fontsize __UNUSED__)
{
   return;
}

EINA_DEPRECATED EAPI void
elm_label_text_align_set(Evas_Object *obj __UNUSED__,
                         const char *alignmode __UNUSED__)
{
   return;
}

EINA_DEPRECATED EAPI void
elm_label_text_color_set(Evas_Object *obj __UNUSED__,
                         unsigned int r __UNUSED__,
                         unsigned int g __UNUSED__,
                         unsigned int b __UNUSED__,
                         unsigned int a __UNUSED__)
{
   return;
}

EINA_DEPRECATED EAPI void
elm_label_background_color_set(Evas_Object *obj __UNUSED__,
                               unsigned int r __UNUSED__,
                               unsigned int g __UNUSED__,
                               unsigned int b __UNUSED__,
                               unsigned int a __UNUSED__)
{
   return;
>>>>>>> remotes/origin/upstream
}

EAPI void
elm_label_ellipsis_set(Evas_Object *obj, Eina_Bool ellipsis)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;

   if (!wd) return;
   if (wd->ellipsis == ellipsis) return;
   wd->ellipsis = ellipsis;
   len = strlen(wd->label);
   if (len <= 0) return;

   if (ellipsis == EINA_FALSE) removeflag = 1;  // remove fontsize tag

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%f", 1.0);

   if (_stringshare_key_value_replace(&wd->format,
            "ellipsis", eina_strbuf_string_get(fontbuf), removeflag) == 0)
     {
<<<<<<< HEAD
        edje_object_part_text_style_user_set(wd->lbl, "elm.text",
              wd->format);
=======
        _label_format_set(wd->lbl, wd->format);
>>>>>>> remotes/origin/upstream
        edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
        wd->changed = 1;
        _sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);

}

<<<<<<< HEAD
=======
EAPI Eina_Bool
elm_label_ellipsis_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->ellipsis;
}

>>>>>>> remotes/origin/upstream
EAPI void
elm_label_slide_set(Evas_Object *obj,
                    Eina_Bool    slide)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->slidingmode == slide) return;
   wd->slidingmode = slide;
   _label_sliding_change(obj);
   wd->changed = 1;
   _sizing_eval(obj);
}

EAPI Eina_Bool
<<<<<<< HEAD
elm_label_slide_get(Evas_Object *obj)
=======
elm_label_slide_get(const Evas_Object *obj)
>>>>>>> remotes/origin/upstream
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->slidingmode;
}

EAPI void
elm_label_slide_duration_set(Evas_Object *obj, double duration)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Edje_Message_Float_Set *msg = alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

   if (!wd) return;
   wd->slide_duration = duration;
   msg->count = 1;
   msg->val[0] = wd->slide_duration;
   edje_object_message_send(wd->lbl, EDJE_MESSAGE_FLOAT_SET, 0, msg);
}

EAPI double
<<<<<<< HEAD
elm_label_slide_duration_get(Evas_Object *obj)
=======
elm_label_slide_duration_get(const Evas_Object *obj)
>>>>>>> remotes/origin/upstream
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0.0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->slide_duration;
}

