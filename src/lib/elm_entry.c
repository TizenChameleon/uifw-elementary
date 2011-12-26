#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"
#include "elm_module_priv.h"
#include "els_scroller.h"


/* Maximum chunk size to be inserted to the entry at once
 * FIXME: This size is arbitrary, should probably choose a better size.
 * Possibly also find a way to set it to a low value for weak computers,
 * and to a big value for better computers. */
#define _CHUNK_SIZE 10000

typedef struct _Mod_Api Mod_Api;

typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Entry_Context_Menu_Item Elm_Entry_Context_Menu_Item;
typedef struct _Elm_Entry_Item_Provider Elm_Entry_Item_Provider;
typedef struct _Elm_Entry_Text_Filter Elm_Entry_Text_Filter;

struct _Widget_Data
{
   Evas_Object *ent, *scroller, *end, *icon;
   Evas_Object *bg;
   Evas_Object *hoversel;
   Evas_Object *hover;
   Evas_Object *layout;
   Evas_Object *list;
   Evas_Object *mgf_proxy;
   Evas_Object *mgf_clip;
   Evas_Object *mgf_bg;
   Evas_Coord mgf_height;
   float mgf_scale;
   int mgf_type;
   Ecore_Job *deferred_recalc_job;
   Ecore_Job *region_get_job;
   Ecore_Event_Handler *sel_notify_handler;
   Ecore_Event_Handler *sel_clear_handler;
   Ecore_Timer *delay_write;
   /* for deferred appending */
   Ecore_Idler *append_text_idler;
   char *append_text_left;
   int append_text_position;
   int append_text_len;
   /* Only for clipboard */
   const char *cut_sel;
   const char *text;
   const char *password_text;
   const char *file;
   Elm_Text_Format format;
   Evas_Coord lastw, entmw, entmh;
   Evas_Coord downx, downy;
   Evas_Coord cx, cy, cw, ch;
   Eina_List *items;
   Eina_List *item_providers;
   Eina_List *text_filters;
   Eina_List *match_list;
   Ecore_Job *matchlist_job;
   int matchlist_threshold;
   Ecore_Job *hovdeljob;
   Mod_Api *api; // module api if supplied
   int cursor_pos;
   Elm_Scroller_Policy policy_h, policy_v;
   Elm_Wrap_Type linewrap;
   Elm_Input_Panel_Layout input_panel_layout;
   Elm_Autocapital_Type autocapital_type;
   Eina_Bool changed : 1;
   Eina_Bool single_line : 1;
   Eina_Bool password : 1;
   Eina_Bool editable : 1;
   Eina_Bool selection_asked : 1;
   Eina_Bool have_selection : 1;
   Eina_Bool selmode : 1;
   Eina_Bool deferred_cur : 1;
   Eina_Bool cur_changed : 1;
   Eina_Bool disabled : 1;
   Eina_Bool double_clicked : 1;
   Eina_Bool long_pressed : 1;
   Eina_Bool context_menu : 1;
   Eina_Bool magnifier_enabled : 1;
   Eina_Bool drag_selection_asked : 1;
   Eina_Bool bgcolor : 1;
   Eina_Bool can_write : 1;
   Eina_Bool autosave : 1;
   Eina_Bool usedown : 1;
   Eina_Bool scroll : 1;
   Eina_Bool h_bounce : 1;
   Eina_Bool v_bounce : 1;
   Eina_Bool input_panel_enable : 1;
   Eina_Bool autoreturnkey : 1;
   Eina_Bool autoperiod : 1;
   Eina_Bool matchlist_list_clicked : 1;
   Eina_Bool matchlist_case_sensitive : 1;
   Elm_CNP_Mode cnp_mode : 2;
};

struct _Elm_Entry_Context_Menu_Item
{
   Evas_Object *obj;
   const char *label;
   const char *icon_file;
   const char *icon_group;
   Elm_Icon_Type icon_type;
   Evas_Smart_Cb func;
   void *data;
};

struct _Elm_Entry_Item_Provider
{
   Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item);
   void *data;
};

struct _Elm_Entry_Text_Filter
{
   void (*func) (void *data, Evas_Object *entry, char **text);
   void *data;
};

typedef enum _Length_Unit
{
   LENGTH_UNIT_CHAR,
   LENGTH_UNIT_BYTE,
   LENGTH_UNIT_LAST
} Length_Unit;

static const char *widtype = NULL;
// start for cbhm
static Evas_Object *cnpwidgetdata = NULL;
// end for cbhm

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool _drag_drop_cb(void *data, Evas_Object *obj, Elm_Selection_Data *);
#endif
static void _del_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _theme_hook(Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content);
static Evas_Object *_content_unset_hook(Evas_Object *obj, const char *part);
static Evas_Object *_content_get_hook(const Evas_Object *obj, const char *part);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static const char *_getbase(Evas_Object *obj);
static void _signal_entry_changed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_selection_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_selection_end(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_selection_changed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_selection_cleared(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_handler_move_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_handler_move_end(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_handler_moving(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_entry_paste_request(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_entry_copy_notify(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_entry_cut_notify(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _signal_cursor_changed(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _add_chars_till_limit(Evas_Object *obj, char **text, int can_add, Length_Unit unit);
static int _strbuf_key_value_replace(Eina_Strbuf *srcbuf, char *key, const char *value, int deleteflag);
static int _stringshare_key_value_replace(const char **srcstring, char *key, const char *value, int deleteflag);
static int _entry_length_get(Evas_Object *obj);
static void _magnifier_create(void *data);
static void _magnifier_show(void *data);
static void _magnifier_hide(void *data);
static void _magnifier_move(void *data);
static Evas_Coord_Rectangle _layout_region_get(Evas_Object *data);
static Evas_Coord_Rectangle _viewport_region_get(Evas_Object *data);
static void _elm_win_region_get_job(void *data);

static const char SIG_CHANGED[] = "changed";
static const char SIG_ACTIVATED[] = "activated";
static const char SIG_PRESS[] = "press";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_FOCUSED[] = "focused";
static const char SIG_UNFOCUSED[] = "unfocused";
static const char SIG_SELECTION_PASTE[] = "selection,paste";
static const char SIG_SELECTION_COPY[] = "selection,copy";
static const char SIG_SELECTION_CUT[] = "selection,cut";
static const char SIG_SELECTION_START[] = "selection,start";
static const char SIG_SELECTION_CHANGED[] = "selection,changed";
static const char SIG_SELECTION_CLEARED[] = "selection,cleared";
static const char SIG_CURSOR_CHANGED[] = "cursor,changed";
static const char SIG_ANCHOR_CLICKED[] = "anchor,clicked";
static const char SIG_MATCHLIST_CLICKED[] = "matchlist,clicked";
static const char SIG_PREEDIT_CHANGED[] = "preedit,changed";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CHANGED, ""},
       {SIG_ACTIVATED, ""},
       {SIG_PRESS, ""},
       {SIG_LONGPRESSED, ""},
       {SIG_CLICKED, ""},
       {SIG_CLICKED_DOUBLE, ""},
       {SIG_FOCUSED, ""},
       {SIG_UNFOCUSED, ""},
       {SIG_SELECTION_PASTE, ""},
       {SIG_SELECTION_COPY, ""},
       {SIG_SELECTION_CUT, ""},
       {SIG_SELECTION_START, ""},
       {SIG_SELECTION_CHANGED, ""},
       {SIG_SELECTION_CLEARED, ""},
       {SIG_CURSOR_CHANGED, ""},
       {SIG_ANCHOR_CLICKED, ""},
       {SIG_PREEDIT_CHANGED, ""},
       {SIG_MATCHLIST_CLICKED, ""},
       {NULL, NULL}
};

typedef enum _Elm_Entry_Magnifier_Type
{
   _ENTRY_MAGNIFIER_FIXEDSIZE = 0,
   _ENTRY_MAGNIFIER_FILLWIDTH,
   _ENTRY_MAGNIFIER_CIRCULAR,
} Elm_Entry_Magnifier_Type;


static Eina_List *entries = NULL;

struct _Mod_Api
{
   void (*obj_hook) (Evas_Object *obj);
   void (*obj_unhook) (Evas_Object *obj);
   void (*obj_longpress) (Evas_Object *obj);
   void (*obj_hidemenu) (Evas_Object *obj);
   void (*obj_mouseup) (Evas_Object *obj);
};

static Mod_Api *
_module(Evas_Object *obj __UNUSED__)
{
   static Elm_Module *m = NULL;
   if (m) goto ok; // already found - just use
   if (!(m = _elm_module_find_as("entry/api"))) return NULL;
   // get module api
   m->api = malloc(sizeof(Mod_Api));
   if (!m->api) return NULL;
   ((Mod_Api *)(m->api)      )->obj_hook = // called on creation
      _elm_module_symbol_get(m, "obj_hook");
   ((Mod_Api *)(m->api)      )->obj_unhook = // called on deletion
      _elm_module_symbol_get(m, "obj_unhook");
   ((Mod_Api *)(m->api)      )->obj_longpress = // called on long press menu
      _elm_module_symbol_get(m, "obj_longpress");
   ((Mod_Api *)(m->api)      )->obj_hidemenu = // called on hide menu
      _elm_module_symbol_get(m, "obj_hidemenu");
   ((Mod_Api *)(m->api)      )->obj_mouseup = // called on mouseup
      _elm_module_symbol_get(m, "obj_mouseup");
ok: // ok - return api
   return m->api;
}

static char *
_buf_append(char *buf, const char *str, int *len, int *alloc)
{
   int len2 = strlen(str);
   if ((*len + len2) >= *alloc)
     {
        char *buf2 = realloc(buf, *alloc + len2 + 512);
        if (!buf2) return NULL;
        buf = buf2;
        *alloc += (512 + len2);
     }
   strcpy(buf + *len, str);
   *len += len2;
   return buf;
}

static char *
_load_file(const char *file)
{
   FILE *f;
   size_t size;
   int alloc = 0, len = 0;
   char *text = NULL, buf[16384 + 1];

   f = fopen(file, "rb");
   if (!f) return NULL;
   while ((size = fread(buf, 1, sizeof(buf) - 1, f)))
     {
        char *tmp_text;
        buf[size] = 0;
        tmp_text = _buf_append(text, buf, &len, &alloc);
        if (!tmp_text) break;
        text = tmp_text;
     }
   fclose(f);
   return text;
}

static char *
_load_plain(const char *file)
{
   char *text;

   text = _load_file(file);
   if (text)
     {
        char *text2;

        text2 = elm_entry_utf8_to_markup(text);
        free(text);
        return text2;
     }
   return NULL;
}

static void
_load(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   char *text;
   if (!wd) return;
   if (!wd->file)
     {
        elm_entry_entry_set(obj, "");
        return;
     }
   switch (wd->format)
     {
      case ELM_TEXT_FORMAT_PLAIN_UTF8:
         text = _load_plain(wd->file);
         break;
      case ELM_TEXT_FORMAT_MARKUP_UTF8:
         text = _load_file(wd->file);
         break;
      default:
         text = NULL;
         break;
     }
   if (text)
     {
        elm_entry_entry_set(obj, text);
        free(text);
     }
   else
     elm_entry_entry_set(obj, "");
}

static void
_save_markup_utf8(const char *file, const char *text)
{
   FILE *f;

   if ((!text) || (!text[0]))
     {
        ecore_file_unlink(file);
        return;
     }
   f = fopen(file, "wb");
   if (!f)
     {
        // FIXME: report a write error
        return;
     }
   fputs(text, f); // FIXME: catch error
   fclose(f);
}

static void
_save_plain_utf8(const char *file, const char *text)
{
   char *text2;

   text2 = elm_entry_markup_to_utf8(text);
   if (!text2)
     return;
   _save_markup_utf8(file, text2);
   free(text2);
}

static void
_save(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!wd->file) return;
   switch (wd->format)
     {
      case ELM_TEXT_FORMAT_PLAIN_UTF8:
         _save_plain_utf8(wd->file, elm_entry_entry_get(obj));
         break;
      case ELM_TEXT_FORMAT_MARKUP_UTF8:
         _save_markup_utf8(wd->file, elm_entry_entry_get(obj));
         break;
      default:
         break;
     }
}

static Eina_Bool
_delay_write(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_CANCEL;
   _save(data);
   wd->delay_write = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Elm_Entry_Text_Filter *
_filter_new(void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{
   Elm_Entry_Text_Filter *tf = ELM_NEW(Elm_Entry_Text_Filter);
   if (!tf) return NULL;

   tf->func = func;
   if (func == elm_entry_filter_limit_size)
     {
        Elm_Entry_Filter_Limit_Size *lim = data, *lim2;

        if (!data)
          {
             free(tf);
             return NULL;
          }
        lim2 = malloc(sizeof(Elm_Entry_Filter_Limit_Size));
        if (!lim2)
          {
             free(tf);
             return NULL;
          }
        memcpy(lim2, lim, sizeof(Elm_Entry_Filter_Limit_Size));
        tf->data = lim2;
     }
   else if (func == elm_entry_filter_accept_set)
     {
        Elm_Entry_Filter_Accept_Set *as = data, *as2;

        if (!data)
          {
             free(tf);
             return NULL;
          }
        as2 = malloc(sizeof(Elm_Entry_Filter_Accept_Set));
        if (!as2)
          {
             free(tf);
             return NULL;
          }
        if (as->accepted)
          as2->accepted = eina_stringshare_add(as->accepted);
        else
          as2->accepted = NULL;
        if (as->rejected)
          as2->rejected = eina_stringshare_add(as->rejected);
        else
          as2->rejected = NULL;
        tf->data = as2;
     }
   else
     tf->data = data;
   return tf;
}

static void
_filter_free(Elm_Entry_Text_Filter *tf)
{
   if (tf->func == elm_entry_filter_limit_size)
     {
        Elm_Entry_Filter_Limit_Size *lim = tf->data;
        if (lim) free(lim);
     }
   else if (tf->func == elm_entry_filter_accept_set)
     {
        Elm_Entry_Filter_Accept_Set *as = tf->data;
        if (as)
          {
             if (as->accepted) eina_stringshare_del(as->accepted);
             if (as->rejected) eina_stringshare_del(as->rejected);
             free(as);
          }
     }
   free(tf);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->delay_write)
     {
        ecore_timer_del(wd->delay_write);
        wd->delay_write = NULL;
        if (wd->autosave) _save(obj);
     }
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Entry_Context_Menu_Item *it;
   Elm_Entry_Item_Provider *ip;
   Elm_Entry_Text_Filter *tf;

   if (wd->file) eina_stringshare_del(wd->file);

   if (wd->hovdeljob) ecore_job_del(wd->hovdeljob);
   if ((wd->api) && (wd->api->obj_unhook)) wd->api->obj_unhook(obj); // module - unhook

   entries = eina_list_remove(entries, obj);
#ifdef HAVE_ELEMENTARY_X
   if (wd->sel_notify_handler)
     ecore_event_handler_del(wd->sel_notify_handler);
   if (wd->sel_clear_handler)
     ecore_event_handler_del(wd->sel_clear_handler);
#endif
   if (wd->cut_sel) eina_stringshare_del(wd->cut_sel);
   if (wd->text) eina_stringshare_del(wd->text);
   if (wd->password_text) eina_stringshare_del(wd->password_text);
   if (wd->bg) evas_object_del(wd->bg);
   if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
   if (wd->region_get_job) ecore_job_del(wd->region_get_job);
   if (wd->append_text_idler)
     {
        ecore_idler_del(wd->append_text_idler);
        free(wd->append_text_left);
        wd->append_text_left = NULL;
        wd->append_text_idler = NULL;
     }
   if (wd->matchlist_job) ecore_job_del(wd->matchlist_job);
   if (wd->mgf_proxy) evas_object_del(wd->mgf_proxy);
   if (wd->mgf_bg) evas_object_del(wd->mgf_bg);
   if (wd->mgf_clip) evas_object_del(wd->mgf_clip);

   EINA_LIST_FREE(wd->items, it)
     {
        eina_stringshare_del(it->label);
        eina_stringshare_del(it->icon_file);
        eina_stringshare_del(it->icon_group);
        free(it);
     }
   EINA_LIST_FREE(wd->item_providers, ip)
     {
        free(ip);
     }
   EINA_LIST_FREE(wd->text_filters, tf)
     {
        _filter_free(tf);
     }
   if (wd->delay_write) ecore_timer_del(wd->delay_write);
   free(wd);
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_mirrored_set(wd->ent, rtl);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *t;
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   t = eina_stringshare_add(elm_entry_entry_get(obj));
   _elm_theme_object_set(obj, wd->ent, "entry", _getbase(obj), elm_widget_style_get(obj));
   if (_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_TRUE);
   elm_entry_entry_set(obj, t);
   eina_stringshare_del(t);
   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->ent, "elm,state,disabled", "elm");
   edje_object_part_text_input_panel_layout_set(wd->ent, "elm.text", wd->input_panel_layout);
   edje_object_part_text_autocapital_type_set(wd->ent, "elm.text", wd->autocapital_type);
   edje_object_part_text_input_panel_enabled_set(wd->ent, "elm.text", wd->input_panel_enable);
   elm_entry_cursor_pos_set(obj, wd->cursor_pos);
   if (elm_widget_focus_get(obj))
     edje_object_signal_emit(wd->ent, "elm,action,focus", "elm");
   edje_object_message_signal_process(wd->ent);
   edje_object_scale_set(wd->ent, elm_widget_scale_get(obj) * _elm_config->scale);
   if (wd->scroll)
     {
        const char *str;
        Evas_Object *edj;

        elm_smart_scroller_mirrored_set(wd->scroller, elm_widget_mirrored_get(obj));
        elm_smart_scroller_object_theme_set(obj, wd->scroller, "scroller", "entry",
                                       elm_widget_style_get(obj));
        edj = elm_smart_scroller_edje_object_get(wd->scroller);
        str = edje_object_data_get(edj, "focus_highlight");
        if ((str) && (!strcmp(str, "on")))
          elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
        else
          elm_widget_highlight_in_theme_set(obj, EINA_FALSE);
     }
   _sizing_eval(obj);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (elm_widget_disabled_get(obj))
     {
        edje_object_signal_emit(wd->ent, "elm,state,disabled", "elm");
        wd->disabled = EINA_TRUE;
     }
   else
     {
        edje_object_signal_emit(wd->ent, "elm,state,enabled", "elm");
        wd->disabled = EINA_FALSE;
     }
}

static void
_recalc_cursor_geometry(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_smart_callback_call(obj, SIG_CURSOR_CHANGED, NULL);
   if (!wd->deferred_recalc_job)
     {
        Evas_Coord cx, cy, cw, ch;
        edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text",
                                                  &cx, &cy, &cw, &ch);
        if (wd->cur_changed)
          {
             elm_widget_show_region_set(obj, cx, cy, cw, ch, EINA_FALSE);
             wd->cur_changed = EINA_FALSE;
          }
     }
   else
     wd->deferred_cur = EINA_TRUE;
}

static void
_elm_win_recalc_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord minh = -1, resw = -1, minw = -1, fw = 0, fh = 0;
   if (!wd) return;
   wd->deferred_recalc_job = NULL;

   evas_object_geometry_get(wd->ent, NULL, NULL, &resw, NULL);
   /* This is a hack to workaround to avoid the cursor geometry bug
    * in a char-wrap or a mixed-mode textblock in a scroller.
    * The order of calling next 2 functions affects to the size of the textblock cursor.
    * when the textblock core issue is fixed these comments will be removed. */
   edje_object_size_min_restricted_calc(wd->ent, &fw, &fh, 0, 0);
   edje_object_size_min_restricted_calc(wd->ent, &minw, &minh, resw, 0);
   elm_coords_finger_size_adjust(1, &fw, 1, &minh);
   wd->entmw = fw;
   wd->entmh = minh;
   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it means
    * the mininmum doesn't matter. */
   if (minw <= resw)
     {
        Evas_Coord ominw = -1;
        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }

   elm_coords_finger_size_adjust(1, &fw, 1, &fh);
   if (wd->scroll)
     {
        Evas_Coord vmw = 0, vmh = 0;

        edje_object_size_min_calc
           (elm_smart_scroller_edje_object_get(wd->scroller),
               &vmw, &vmh);
        if (wd->single_line)
          {
             evas_object_size_hint_min_set(data, vmw, minh + vmh);
             evas_object_size_hint_max_set(data, -1, minh + vmh);
          }
        else
          {
             evas_object_size_hint_min_set(data, vmw, vmh);
             evas_object_size_hint_max_set(data, -1, -1);
          }
     }
   else
     {
        if (wd->single_line)
          {
             evas_object_size_hint_min_set(data, minw, minh);
             evas_object_size_hint_max_set(data, -1, minh);
          }
        else
          {
             evas_object_size_hint_min_set(data, fw, minh);
             evas_object_size_hint_max_set(data, -1, -1);
          }
     }

   if (wd->deferred_cur)
     {
        Evas_Coord cx, cy, cw, ch;
        edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text",
                                                  &cx, &cy, &cw, &ch);
        if (wd->cur_changed)
          {
             elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
             wd->cur_changed = EINA_FALSE;
          }
     }
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;
   if (!wd) return;

   evas_object_geometry_get(obj, NULL, NULL, &resw, &resh);
   if (wd->linewrap)
     {
        if ((resw == wd->lastw) && (!wd->changed)) return;
        wd->changed = EINA_FALSE;
        wd->lastw = resw;
        if (wd->scroll)
          {
             Evas_Coord vw = 0, vh = 0, vmw = 0, vmh = 0, w = -1, h = -1;

             evas_object_resize(wd->scroller, resw, resh);
             edje_object_size_min_calc
                (elm_smart_scroller_edje_object_get(wd->scroller),
                 &vmw, &vmh);
             elm_smart_scroller_child_viewport_size_get(wd->scroller, &vw, &vh);
             edje_object_size_min_restricted_calc(wd->ent, &minw, &minh, vw, 0);
             wd->entmw = minw;
             wd->entmh = minh;
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);

             if ((minw > 0) && (vw < minw)) vw = minw;
             if (minh > vh) vh = minh;

             if (wd->single_line) h = vmh + minh;
             else h = vmh;
             evas_object_resize(wd->ent, vw, vh);
             evas_object_size_hint_min_get(obj, &w, NULL);
             evas_object_size_hint_min_set(obj, w, h);
             if (wd->single_line)
               evas_object_size_hint_max_set(obj, -1, h);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
        else
          {
             if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
             wd->deferred_recalc_job = ecore_job_add(_elm_win_recalc_job, obj);
          }
     }
   else
     {
        if (!wd->changed) return;
        wd->changed = EINA_FALSE;
        wd->lastw = resw;
        if (wd->scroll)
          {
             Evas_Coord vw = 0, vh = 0, vmw = 0, vmh = 0, w = -1, h = -1;

             edje_object_size_min_calc(wd->ent, &minw, &minh);
             wd->entmw = minw;
             wd->entmh = minh;
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);

             elm_smart_scroller_child_viewport_size_get(wd->scroller, &vw, &vh);

             if ((minw > 0) && (vw < minw)) vw = minw;
             if (minh > 0) vh = minh;

             evas_object_resize(wd->ent, vw, vh);
             edje_object_size_min_calc
                (elm_smart_scroller_edje_object_get(wd->scroller),
                 &vmw, &vmh);
             if (wd->single_line) h = vmh + minh;
             else h = vmh;
             evas_object_size_hint_min_get(obj, &w, NULL);
             evas_object_size_hint_min_set(obj, w, h);
             if (wd->single_line)
               evas_object_size_hint_max_set(obj, -1, h);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
        else
          {
             edje_object_size_min_calc(wd->ent, &minw, &minh);
             wd->entmw = minw;
             wd->entmh = minh;
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);
             evas_object_size_hint_min_set(obj, minw, minh);
             if (wd->single_line)
               evas_object_size_hint_max_set(obj, -1, minh);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
     }

   _recalc_cursor_geometry(obj);
}

static void
_check_enable_returnkey(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Ecore_IMF_Context *ic = elm_entry_imf_context_get(obj);
   if (!ic) return;

   if (!wd->autoreturnkey) return;

   if (_entry_length_get(obj) == 0)
     {
        ecore_imf_context_input_panel_key_disabled_set(ic, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL, ECORE_IMF_INPUT_PANEL_KEY_ENTER, EINA_TRUE);
     }
   else
     {
        ecore_imf_context_input_panel_key_disabled_set(ic, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL, ECORE_IMF_INPUT_PANEL_KEY_ENTER, EINA_FALSE);
     }
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *top = elm_widget_top_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        printf("[Elm_entry::Focused] obj : %p\n", obj);
        evas_object_focus_set(wd->ent, EINA_TRUE);
        edje_object_signal_emit(wd->ent, "elm,action,focus", "elm");
        if (top && wd->input_panel_enable)
          elm_win_keyboard_mode_set(top, ELM_WIN_KEYBOARD_ON);
        evas_object_smart_callback_call(obj, SIG_FOCUSED, NULL);
        _check_enable_returnkey(obj);
        wd->mgf_type = _ENTRY_MAGNIFIER_FIXEDSIZE;
     }
   else
     {
        printf("[Elm_entry::Unfocused] obj : %p\n", obj);
        edje_object_signal_emit(wd->ent, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->ent, EINA_FALSE);
        if (top && wd->input_panel_enable)
          elm_win_keyboard_mode_set(top, ELM_WIN_KEYBOARD_OFF);
        evas_object_smart_callback_call(obj, SIG_UNFOCUSED, NULL);

        if ((wd->api) && (wd->api->obj_hidemenu))
          {
             wd->api->obj_hidemenu(obj);
          }
     }
}


static void
_content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *edje;
   if ((!wd) || (!content)) return;

   if (wd->scroll)
     {
        edje = elm_smart_scroller_edje_object_get(wd->scroller);
        if (!strcmp(part, "elm.swallow.icon"))
          {
             if (wd->icon)
               evas_object_del(wd->icon);
             wd->icon = content;
             edje_object_signal_emit(edje, "elm,action,show,icon", "elm");
          }
        else if (!strcmp(part, "elm.swallow.end"))
          {
             if (wd->end)
               evas_object_del(wd->end);
             wd->end = content;
             edje_object_signal_emit(edje, "elm,action,show,end", "elm");
          }
     }
   else
     edje = wd->ent;
   evas_event_freeze(evas_object_evas_get(obj));
   elm_widget_sub_object_add(obj, content);
   edje_object_part_swallow(edje, part, content);
   _sizing_eval(obj);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static Evas_Object *
_content_unset_hook(Evas_Object *obj, const char *part)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *content, *edje;
   if (!wd) return NULL;

   if (wd->scroll)
     {
        edje = elm_smart_scroller_edje_object_get(wd->scroller);
        if (!strcmp(part, "elm.swallow.icon"))
          {
             wd->icon = NULL;
             edje_object_signal_emit(edje, "elm,action,hide,icon", "elm");
          }
        else if (!strcmp(part, "elm.swallow.end"))
          {
             wd->end = NULL;
             edje_object_signal_emit(edje, "elm,action,hide,end", "elm");
          }
     }
   else
     edje = wd->ent;

   content = edje_object_part_swallow_get(edje, part);
   edje_object_part_swallow(edje, part, NULL);
   if (!content) return NULL;
   evas_event_freeze(evas_object_evas_get(obj));
   elm_widget_sub_object_del(obj, content);
   edje_object_part_unswallow(wd->ent, content);
   _sizing_eval(obj);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   return content;
}

static Evas_Object *
_content_get_hook(const Evas_Object *obj, const char *part)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *content = NULL, *edje;
   if (!wd) return NULL;

   if (wd->scroll)
     {
        if (!strcmp(part, "elm.swallow.icon"))
          return wd->icon;
        if (!strcmp(part, "elm.swallow.end"))
          return wd->end;

        edje = elm_smart_scroller_edje_object_get(wd->scroller);
     }
   else
      edje = wd->ent;

   if (edje)
     content = edje_object_part_swallow_get(edje, part);
   return content;
}

static void
_translate_hook(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, "language,changed", NULL);
}

static void
_signal_emit_hook(Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_emit(wd->ent, emission, source);
   if (wd->scroller)
      edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scroller),
                              emission, source);
}

static void
_signal_callback_add_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_callback_add(wd->ent, emission, source, func_cb, data);
   if (wd->scroller)
     edje_object_signal_callback_add(elm_smart_scroller_edje_object_get(wd->scroller),
                                     emission, source, func_cb, data);
}

static void
_signal_callback_del_hook(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func_cb, void *data)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_signal_callback_del_full(wd->ent, emission, source, func_cb,
                                        data);
   if (wd->scroller)
     edje_object_signal_callback_del_full(elm_smart_scroller_edje_object_get(wd->scroller),
                                          emission, source, func_cb, data);
}

static void
_on_focus_region_hook(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text", x, y, w, h);
}

static void
_focus_region_hook(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->scroll)
     elm_smart_scroller_child_region_show(wd->scroller, x, y, w, h);
}

static void
_show_region_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   if (wd->scroll)
     elm_smart_scroller_child_region_show(wd->scroller, x, y, w, h);
}

static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = data;
   Evas_Object *sub = event_info;
   Evas_Object *edje;

   edje = elm_smart_scroller_edje_object_get(wd->scroller);
   if (sub == wd->icon)
     {
        wd->icon = NULL;
        if (edje)
          edje_object_signal_emit(edje, "elm,action,hide,icon", "elm");
     }
   else if (sub == wd->end)
     {
        wd->end = NULL;
        if (edje)
          edje_object_signal_emit(edje, "elm,action,hide,end", "elm");
     }
   _sizing_eval(obj);
}

static void
_hoversel_position(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord cx, cy, cw, ch, x, y, mw, mh;
   if (!wd) return;

   cx = cy = 0;
   cw = ch = 1;
   evas_object_geometry_get(wd->ent, &x, &y, NULL, NULL);
   if (wd->usedown)
     {
        cx = wd->downx - x;
        cy = wd->downy - y;
        cw = 1;
        ch = 1;
     }
   else
     edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text",
                                               &cx, &cy, &cw, &ch);
   evas_object_size_hint_min_get(wd->hoversel, &mw, &mh);
   if (cw < mw)
     {
        cx += (cw - mw) / 2;
        cw = mw;
     }
   if (ch < mh)
     {
        cy += (ch - mh) / 2;
        ch = mh;
     }
   evas_object_move(wd->hoversel, x + cx, y + cy);
   evas_object_resize(wd->hoversel, cw, ch);
}

static void
_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (wd->hoversel) _hoversel_position(data);

   if (!_elm_config->desktop_entry)
     {
        if (wd->region_get_job) ecore_job_del(wd->region_get_job);
        wd->region_get_job = ecore_job_add(_elm_win_region_get_job, data);
     }
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->linewrap)
     {
        _sizing_eval(data);
     }
   else if (wd->scroll)
     {
        Evas_Coord vw = 0, vh = 0;

        elm_smart_scroller_child_viewport_size_get(wd->scroller, &vw, &vh);
        if (vw < wd->entmw) vw = wd->entmw;
        if (vh < wd->entmh) vh = wd->entmh;
        evas_object_resize(wd->ent, vw, vh);
     }
   if (wd->hoversel) _hoversel_position(data);

   if (!_elm_config->desktop_entry)
     {
        if (wd->region_get_job) ecore_job_del(wd->region_get_job);
        wd->region_get_job = ecore_job_add(_elm_win_region_get_job, data);
     }
}

static void
_hover_del(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->hoversel)
     {
        evas_object_del(wd->hoversel);
        wd->hoversel = NULL;
     }
   wd->hovdeljob = NULL;
}

static void
_dismissed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->usedown = 0;
   if (wd->hoversel) evas_object_hide(wd->hoversel);
   if (wd->selmode)
     {
        if (!_elm_config->desktop_entry)
          {
             if (!wd->password)
               edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_TRUE);
          }
     }
   elm_widget_scroll_freeze_pop(data);
   if (wd->hovdeljob) ecore_job_del(wd->hovdeljob);
   wd->hovdeljob = ecore_job_add(_hover_del, data);
}

static void
_selectall(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_TRUE;
   edje_object_part_text_select_none(wd->ent, "elm.text");
   edje_object_signal_emit(wd->ent, "elm,state,select,on", "elm");
   edje_object_part_text_select_all(wd->ent, "elm.text");
   elm_object_scroll_freeze_pop(data);
}

static void
_select(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_TRUE;
   edje_object_part_text_select_none(wd->ent, "elm.text");
   if (!_elm_config->desktop_entry)
     {
        if (!wd->password)
          edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_TRUE);
     }
   edje_object_signal_emit(wd->ent, "elm,state,select,on", "elm");
   if (!_elm_config->desktop_entry)
      elm_object_scroll_freeze_pop(data);
      //elm_widget_scroll_hold_push(data);
}

static void
_paste(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
   if (wd->sel_notify_handler)
     {
#ifdef HAVE_ELEMENTARY_X
        Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;
        wd->selection_asked = EINA_TRUE;
        if (wd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
          formats = ELM_SEL_FORMAT_TEXT;
        else if (wd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
          formats |= ELM_SEL_FORMAT_IMAGE;
        elm_selection_get(ELM_SEL_CLIPBOARD, formats, data, NULL, NULL);
#endif
     }
}

static void
_store_selection(Elm_Sel_Type seltype, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *sel;
   char *sel_str = NULL;
   Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;

   if (!wd) return;
   sel = edje_object_part_text_selection_get(wd->ent, "elm.text");
   if (wd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
     {
        sel_str = elm_entry_markup_to_utf8(sel);
        if (!sel_str)
          return;
        formats = ELM_SEL_FORMAT_TEXT;
     }
   else
     {
        sel_str = strdup(sel);
        if (!sel_str)
          return;
        if (wd->cnp_mode == ELM_CNP_MODE_NO_IMAGE)
          {
             while (EINA_TRUE)
               {
                  char *startTag = NULL;
                  char *endTag = NULL;

                  startTag = strstr(sel_str, "<item");
                  if (!startTag)
                    startTag = strstr(sel_str, "</item");
                  if (startTag)
                    endTag = strstr(startTag, ">");
                  else
                    break;
                  if (!endTag || startTag > endTag)
                    break;

                  size_t sindex = startTag - sel_str;
                  size_t eindex = endTag - sel_str + 1;

                  Eina_Strbuf *buf = eina_strbuf_new();
                  if (buf)
                    {
                       eina_strbuf_append(buf, sel_str);
                       eina_strbuf_remove(buf, sindex, eindex);
                       sel_str = eina_strbuf_string_steal(buf);
                       eina_strbuf_free(buf);
                    }
               }
          }
     }
   elm_selection_set(seltype, obj, formats, sel_str);
   if (seltype == ELM_SEL_CLIPBOARD)
     eina_stringshare_replace(&wd->cut_sel, sel);
   free(sel_str);
}

static void
_cut(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   /* Store it */
   wd->selmode = EINA_FALSE;
   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_FALSE);
   edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
   if (!_elm_config->desktop_entry)
     elm_widget_scroll_hold_pop(data);
   _store_selection(ELM_SEL_CLIPBOARD, data);
   edje_object_part_text_insert(wd->ent, "elm.text", "");
   edje_object_part_text_select_none(wd->ent, "elm.text");
}

static void
_copy(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_TRUE;
   if (!_elm_config->desktop_entry)
     {
        edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
        elm_widget_scroll_hold_pop(data);
     }
   _store_selection(ELM_SEL_CLIPBOARD, data);
}

static void
_cancel(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_FALSE;
   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_FALSE);
   edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
   if (!_elm_config->desktop_entry)
     elm_widget_scroll_hold_pop(data);
   edje_object_part_text_select_none(wd->ent, "elm.text");
}

static void
_clipboard_menu(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   // start for cbhm
#ifdef HAVE_ELEMENTARY_X
   ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), "",1);
#endif
   cnpwidgetdata = data;
   elm_cbhm_helper_init(obj);
   if (elm_entry_cnp_mode_get(obj) != ELM_CNP_MODE_MARKUP)
     elm_cbhm_send_raw_data("show0");
   else
     elm_cbhm_send_raw_data("show1");
   // end for cbhm
}

// start for cbhm
static void
_cnpinit(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   cnpwidgetdata = data;
}
// end for cbhm


static void
_item_clicked(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;
   if (it->func) it->func(it->data, obj2, NULL);
}

static void
_menu_press(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *top;
   const Eina_List *l;
   const Elm_Entry_Context_Menu_Item *it;
   if (!wd) return;
   if ((wd->api) && (wd->api->obj_longpress))
     {
        wd->api->obj_longpress(obj);
     }
   else if (wd->context_menu)
     {
        const char *context_menu_orientation;

        if (wd->hoversel) evas_object_del(wd->hoversel);
        else elm_widget_scroll_freeze_push(obj);
        wd->hoversel = elm_hoversel_add(obj);
        context_menu_orientation = edje_object_data_get
           (wd->ent, "context_menu_orientation");
        if ((context_menu_orientation) &&
            (!strcmp(context_menu_orientation, "horizontal")))
          elm_hoversel_horizontal_set(wd->hoversel, EINA_TRUE);
        elm_object_style_set(wd->hoversel, "entry");
        elm_widget_sub_object_add(obj, wd->hoversel);
        elm_object_text_set(wd->hoversel, "Text");
        top = elm_widget_top_get(obj);
        if (top) elm_hoversel_hover_parent_set(wd->hoversel, top);
        evas_object_smart_callback_add(wd->hoversel, "dismissed", _dismissed, obj);
        if (!wd->selmode)
          {
             if (!wd->password)
               elm_hoversel_item_add(wd->hoversel, E_("Select"), NULL, ELM_ICON_NONE,
                                     _select, obj);
             if (1) // need way to detect if someone has a selection
               {
                  if (wd->editable)
                    elm_hoversel_item_add(wd->hoversel, E_("Paste"), NULL, ELM_ICON_NONE,
                                          _paste, obj);
               }
	// start for cbhm
             if ((!wd->password) && (wd->editable))
               elm_hoversel_item_add(wd->hoversel, E_("More"), NULL, ELM_ICON_NONE,
                                     _clipboard_menu, obj);
	// end for cbhm
          }
        else
          {
             if (!wd->password)
               {
                  if (wd->have_selection)
                    {
                       elm_hoversel_item_add(wd->hoversel, E_("Copy"), NULL, ELM_ICON_NONE,
                                             _copy, obj);
                       if (wd->editable)
                         elm_hoversel_item_add(wd->hoversel, E_("Cut"), NULL, ELM_ICON_NONE,
                                               _cut, obj);
                    }
                  elm_hoversel_item_add(wd->hoversel, E_("Cancel"), NULL, ELM_ICON_NONE,
                                        _cancel, obj);
	// start for cbhm
                  if (wd->editable)
                    elm_hoversel_item_add(wd->hoversel, E_("More"), NULL, ELM_ICON_NONE,
                                          _clipboard_menu, obj);
	// end for cbhm
               }
          }
        EINA_LIST_FOREACH(wd->items, l, it)
          {
             elm_hoversel_item_add(wd->hoversel, it->label, it->icon_file,
                                   it->icon_type, _item_clicked, it);
          }
        if (wd->hoversel)
          {
             _hoversel_position(obj);
             evas_object_show(wd->hoversel);
             elm_hoversel_hover_begin(wd->hoversel);
          }
        if (!_elm_config->desktop_entry)
          {
             edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_FALSE);
             edje_object_part_text_select_abort(wd->ent, "elm.text");
          }
     }
}

static void
_magnifier_hide(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   evas_object_hide(wd->mgf_bg);
   evas_object_hide(wd->mgf_clip);

   if (wd->scroll)
     elm_smart_scroller_freeze_set(wd->scroller, EINA_FALSE);
}

static void
_magnifier_show(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   evas_object_show(wd->mgf_bg);
   evas_object_show(wd->mgf_clip);
}

static void
_magnifier_move(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   Evas_Coord x, y, w, h;
   Evas_Coord cx, cy, cw, ch, ox, oy;

   edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text", &cx, &cy, &cw, &ch);

   if (wd->scroll)
     {
        evas_object_geometry_get(wd->scroller, &x, &y, &w, &h);
        elm_smart_scroller_child_pos_get(wd->scroller, &ox, &oy);
        cx -= ox;
        cy -= oy;
     }
   else
     evas_object_geometry_get(data, &x, &y, &w, &h);

   ox = oy = 0;

   if ((cy + y) - wd->mgf_height < 0)
     oy = -1 * ((cy + y) - wd->mgf_height);

   if (wd->mgf_type == _ENTRY_MAGNIFIER_FIXEDSIZE)
     evas_object_move(wd->mgf_bg, (cx + x + cw/2) + ox, (cy + y) - wd->mgf_height + oy);
   else if (wd->mgf_type == _ENTRY_MAGNIFIER_FILLWIDTH)
     evas_object_move(wd->mgf_bg, x, (cy + y) - wd->mgf_height + oy);
   else
     return;

   evas_object_move(wd->mgf_proxy, (1 - wd->mgf_scale) * cx + x + ox, (1 - wd->mgf_scale) * cy + y - wd->mgf_height/2 - ch/2 + oy);
}

static void
_magnifier_create(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y, w, h, mw, mh;
   const char* key_data = NULL;
   double elm_scale;

   if (!wd) return;

   if (wd->mgf_proxy)
     {
        evas_object_image_source_unset(wd->mgf_proxy);
        evas_object_color_set(wd->mgf_proxy, 255, 255, 255, 0);
        evas_object_hide(wd->mgf_proxy);
        evas_object_clip_unset(wd->mgf_proxy);
        evas_object_del(wd->mgf_proxy);
     }
   if (wd->mgf_bg) evas_object_del(wd->mgf_bg);
   if (wd->mgf_clip) evas_object_del(wd->mgf_clip);

   if (wd->scroll)
     evas_object_geometry_get(wd->scroller, &x, &y, &w, &h);
   else
     evas_object_geometry_get(data, &x, &y, &w, &h);

   if ((w <= 0) || (h <= 0))
     return;

   wd->mgf_bg = edje_object_add(evas_object_evas_get(data));

   if (wd->mgf_type == _ENTRY_MAGNIFIER_FIXEDSIZE)
     _elm_theme_object_set(data, wd->mgf_bg, "entry", "magnifier", "fixed-size");
   else if (wd->mgf_type == _ENTRY_MAGNIFIER_FILLWIDTH)
     _elm_theme_object_set(data, wd->mgf_bg, "entry", "magnifier", "fill-width");
   else
     return;

   wd->mgf_clip = evas_object_rectangle_add(evas_object_evas_get(data));
   evas_object_color_set(wd->mgf_clip, 255, 255, 255, 255);
   edje_object_part_swallow(wd->mgf_bg, "swallow", wd->mgf_clip);

   key_data = edje_object_data_get(wd->mgf_bg, "height");
   if (key_data) wd->mgf_height = atoi(key_data);
   key_data = edje_object_data_get(wd->mgf_bg, "scale");
   if (key_data) wd->mgf_scale = atof(key_data);

   elm_scale = elm_scale_get();
   wd->mgf_height = (int)((float)wd->mgf_height * elm_scale);

   if (wd->mgf_type == _ENTRY_MAGNIFIER_FILLWIDTH)
     evas_object_resize(wd->mgf_bg, w, wd->mgf_height);

   if (wd->scroll)
     {
        elm_smart_scroller_freeze_set(wd->scroller, EINA_TRUE);
        wd->mgf_proxy = evas_object_image_add(evas_object_evas_get(wd->scroller));
        evas_object_image_source_set(wd->mgf_proxy, wd->scroller);
     }
   else
     {
        wd->mgf_proxy = evas_object_image_add(evas_object_evas_get(data));
        evas_object_image_source_set(wd->mgf_proxy, data);
     }

   mw = (Evas_Coord)((float)w * wd->mgf_scale);
   mh = (Evas_Coord)((float)h * wd->mgf_scale);
   if ((mw <= 0) || (mh <= 0))
     return;

   evas_object_resize(wd->mgf_proxy, mw, mh);
   evas_object_image_fill_set(wd->mgf_proxy, 0, 0, mw, mh);
   evas_object_color_set(wd->mgf_proxy, 255, 255, 255, 255);
   evas_object_pass_events_set(wd->mgf_proxy, EINA_TRUE);
   evas_object_show(wd->mgf_proxy);
   evas_object_clip_set(wd->mgf_proxy, wd->mgf_clip);

   evas_object_layer_set(wd->mgf_bg, EVAS_LAYER_MAX);
   evas_object_layer_set(wd->mgf_proxy, EVAS_LAYER_MAX);
}

static Eina_Bool
_signal_long_pressed(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_CANCEL;

   wd->long_pressed = EINA_TRUE;

   _cancel(data, NULL, NULL);

   if (wd->magnifier_enabled)
     {
        _magnifier_create(data);
        _magnifier_move(data);
        _magnifier_show(data);
        elm_object_scroll_freeze_push(data);
     }

   evas_object_smart_callback_call(data, SIG_LONGPRESSED, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Down *ev = event_info;
   if (!wd) return;
   if (wd->disabled) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   wd->downx = ev->canvas.x;
   wd->downy = ev->canvas.y;
   wd->long_pressed = EINA_FALSE;
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Up *ev = event_info;
   if (!wd) return;
   if (wd->disabled) return;
   if (ev->button == 1)
     {
        if (!wd->double_clicked)
          {
             if ((wd->api) && (wd->api->obj_mouseup))
               wd->api->obj_mouseup(data);
          }
        if (wd->magnifier_enabled)
          {
             _magnifier_hide(data);
             elm_object_scroll_freeze_pop(data);
          }

        if (wd->long_pressed)
          _menu_press(data);
     }
   else if (ev->button == 3)
     {
        wd->usedown = 1;
        _menu_press(data);
     }
}

static void
_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Move *ev = event_info;
   if (!wd) return;
   if (wd->disabled) return;

   if (ev->buttons == 1)
     {
        if ((wd->long_pressed) && (wd->magnifier_enabled))
          {
             _magnifier_show(data);
             _magnifier_move(data);
          }
     }
}

static const char *
_getbase(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return "base";
   if (wd->editable)
     {
        if (wd->password) return "base-password";
        else
          {
             if (wd->single_line) return "base-single";
             else
               {
                  switch (wd->linewrap)
                    {
                     case ELM_WRAP_CHAR:
                        return "base-charwrap";
                     case ELM_WRAP_WORD:
                        return "base";
                     case ELM_WRAP_MIXED:
                        return "base-mixedwrap";
                     case ELM_WRAP_NONE:
                     default:
                        return "base-nowrap";
                    }
               }
          }
     }
   else
     {
        if (wd->password) return "base-password";
        else
          {
             if (wd->single_line) return "base-single-noedit";
             else
               {
                  switch (wd->linewrap)
                    {
                     case ELM_WRAP_CHAR:
                        return "base-noedit-charwrap";
                     case ELM_WRAP_WORD:
                        return "base-noedit";
                     case ELM_WRAP_MIXED:
                        return "base-noedit-mixedwrap";
                     case ELM_WRAP_NONE:
                     default:
                        return "base-nowrap-noedit";
                    }
               }
          }
     }
   return "base";
}


static int
_entry_length_get(Evas_Object *obj)
{
   int len;
   const char *str = elm_entry_entry_get(obj);
   if (!str) return 0;

   char *plain_str = _elm_util_mkup_to_text(str);
   if (!plain_str) return 0;

   len = strlen(plain_str);
   free(plain_str);

   return len;
}

#ifndef HAVE_STRCASESTR
char* _strcasestr(const char *s, const char *find)
{
   char c, sc;
   size_t len;

   if ((c = *find++) != 0) {
      c = tolower((unsigned char) c);
      len = strlen(find);
      do {
         do {
            if( (sc = *s++) == 0)
               return NULL;
         } while ((char)tolower((unsigned char)sc) != c);
      } while (strncasecmp(s, find, len) != 0);
      s--;
   }
   return ((char*) s);
}
#endif

static void
_matchlist_show(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   const char *text = NULL;
   int textlen = 0;
   char *str_list = NULL, *str_result = NULL;
   char *str_mkup = NULL, *str_front = NULL, *str_mid = NULL;

   Eina_List *l;
   Eina_Bool textfound = EINA_FALSE;

   if (!wd) return;
   if (elm_widget_disabled_get(data)) return;

   wd->matchlist_job = NULL;

   if (wd->matchlist_list_clicked)
     {
        evas_object_hide(wd->hover);
        wd->matchlist_list_clicked = EINA_FALSE;
        return;
     }
   text = elm_entry_entry_get(data);
   if (text == NULL)
     return;
   textlen = strlen(text);

   if (textlen < wd->matchlist_threshold)
     {
        evas_object_hide(wd->hover);
        return;
     }

   evas_object_hide(wd->hover);

   if (wd->match_list)
     {
        elm_list_clear(wd->list);
        EINA_LIST_FOREACH(wd->match_list, l, str_list)
          {
             if (wd->matchlist_case_sensitive)
               str_result = strstr(str_list, text);
             else
#ifdef HAVE_STRCASESTR
                str_result = strcasestr(str_list, text);
#else
                str_result = _strcasestr(str_list, text);
#endif

             if (str_result)
               {
                  str_mkup = malloc(strlen(str_list) + 16);
                  if (str_mkup == NULL) return;

                  textlen = strlen(str_list) - strlen(str_result);
                  str_front = malloc(textlen + 1);
                  if (str_front == NULL)
                    {
                       free(str_mkup);
                       return;
                    }

                  memset(str_front, 0, textlen + 1);
                  strncpy(str_front, str_list, textlen);

                  textlen = strlen(text);
                  str_mid = malloc(textlen + 1);
                  if (str_mid == NULL)
                    {
                       free(str_mkup);
                       free(str_front);
                       return;
                    }

                  memset(str_mid, 0, textlen + 1);
                  strncpy(str_mid, str_list + strlen(str_front), textlen);

                  sprintf(str_mkup, "%s<match>%s</match>%s", str_front, str_mid, str_result + strlen(text));

                  elm_list_item_append(wd->list, str_mkup, NULL, NULL, NULL, NULL);

                  if (str_mkup) free(str_mkup);
                  if (str_front) free(str_front);
                  if (str_mid) free(str_mid);

                  textfound=EINA_TRUE;
               }
          }
     }
   else
     return;

   if (textfound)
     {
        elm_list_go(wd->list);
        evas_object_show(wd->hover);
        evas_object_raise(wd->hover);
     }
}

static void _matchlist_list_clicked( void *data, Evas_Object *obj, void *event_info )
{
   Elm_List_Item *it = (Elm_List_Item *) elm_list_selected_item_get(obj);
   Widget_Data *wd = elm_widget_data_get(data);
   if ((it == NULL) || (wd == NULL))
     return;

   const char *str = NULL;
   const char *text = elm_list_item_label_get(it);
   evas_object_smart_callback_call((Evas_Object *)data, "selected", (void *)text);
   if (wd->match_list)
     {
        if (text != NULL)
          {
             str = elm_entry_markup_to_utf8(text);
             elm_entry_entry_set(data, str);
             elm_entry_cursor_end_set(data);
             wd->matchlist_list_clicked = EINA_TRUE;

             evas_object_smart_callback_call(data, SIG_MATCHLIST_CLICKED, elm_entry_markup_to_utf8(text));
             free(str);
          }
     }
   elm_widget_focus_set(data, EINA_TRUE);
}

static void
_entry_changed_common_handling(void *data, const char *event)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord minw, minh;
   if (!wd) return;
   wd->changed = EINA_TRUE;
   /* Reset the size hints which are no more relevant.
    * Keep the height, this is a hack, but doesn't really matter
    * cause we'll re-eval in a moment. */
   if (wd->scroll)
     {
        evas_object_size_hint_min_get(data, &minw, &minh);
        evas_object_size_hint_min_set(data, minw, minh);
     }
   else
     {
        evas_object_size_hint_min_get(data, NULL, &minh);
        evas_object_size_hint_min_set(data, -1, minh);
     }

   _sizing_eval(data);
   if (wd->text) eina_stringshare_del(wd->text);
   wd->text = NULL;
   if (wd->password_text) eina_stringshare_del(wd->password_text);
   wd->password_text = NULL;
   _check_enable_returnkey(data);
   if (wd->delay_write)
     {
        ecore_timer_del(wd->delay_write);
        wd->delay_write = NULL;
     }

   if ((wd->single_line) && (wd->match_list))
     {
        if (wd->matchlist_job) ecore_job_del(wd->matchlist_job);
        wd->matchlist_job = ecore_job_add(_matchlist_show, data);
     }

   if ((wd->api) && (wd->api->obj_hidemenu))
     wd->api->obj_hidemenu(data);

   if ((wd->autosave) && (wd->file))
     wd->delay_write = ecore_timer_add(2.0, _delay_write, data);
   /* callback - this could call callbacks that delete the entry... thus...
    * any access to wd after this could be invalid */
   evas_object_smart_callback_call(data, event, NULL);
}

static void
_signal_entry_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   _entry_changed_common_handling(data, SIG_CHANGED);
}

static void
_signal_preedit_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   _entry_changed_common_handling(data, SIG_PREEDIT_CHANGED);
}

static void
_signal_handler_move_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   elm_object_scroll_freeze_push(data);

   if ((wd->api) && (wd->api->obj_hidemenu))
     wd->api->obj_hidemenu(data);

   if (wd->magnifier_enabled)
     {
        _magnifier_create(data);
        _magnifier_move(data);
        _magnifier_show(data);
     }
}

static void
_signal_handler_move_end(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   elm_object_scroll_freeze_pop(data);

   if (wd->have_selection)
     {
        _magnifier_hide(data);
        _menu_press(data);
     }
}

static void
_signal_handler_moving(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->magnifier_enabled)
     {
        _magnifier_move(data);
        _magnifier_show(data);
     }
}

static Evas_Coord_Rectangle
_intersection_region_get(Evas_Coord_Rectangle rect1, Evas_Coord_Rectangle rect2)
{
   Evas_Coord_Rectangle ret_rect;
   Evas_Coord_Point l1, l2, r1, r2, p1, p2;

   l1.x = rect1.x;
   l1.y = rect1.y;
   r1.x = rect1.x + rect1.w;
   r1.y = rect1.y + rect1.h;

   l2.x = rect2.x;
   l2.y = rect2.y;
   r2.x = rect2.x + rect2.w;
   r2.y = rect2.y + rect2.h;

   p1.x = (l1.x > l2.x) ? l1.x : l2.x;
   p1.y = (l1.y > l2.y) ? l1.y : l2.y;
   p2.x = (r1.x < r2.x) ? r1.x : r2.x;
   p2.y = (r1.y < r2.y) ? r1.y : r2.y;

   ret_rect.x = p1.x;
   ret_rect.y = p1.y;
   ret_rect.w = (p2.x > p1.x) ? p2.x - p1.x : -1;
   ret_rect.h = (p2.y > p1.y) ? p2.y - p1.y : -1;

   return ret_rect;
}

static Evas_Coord_Rectangle
_viewport_region_get(Evas_Object *data)
{
   Evas_Coord_Rectangle geometry, ret_rect;
   geometry.x = geometry.y = geometry.w = geometry.h = -1;
   ret_rect = geometry;

   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return geometry;
   if (!data || !strlen(elm_widget_type_get(data))) return geometry;

   if (wd->scroll)
     {
        evas_object_geometry_get(wd->scroller, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
        ret_rect = geometry;
     }

   Evas_Object *parent_obj = data;

   while ((parent_obj = elm_widget_parent_get(parent_obj)))
     {
        if (!strcmp(elm_widget_type_get(parent_obj), "scroller") ||
            !strcmp(elm_widget_type_get(parent_obj), "genlist"))
          {
             evas_object_geometry_get(parent_obj, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
             if ((ret_rect.w == -1) && (ret_rect.h == -1)) ret_rect = geometry;
             ret_rect = _intersection_region_get(geometry, ret_rect);
          }
     }

   return ret_rect;
}

static Evas_Coord_Rectangle
_layout_region_get(Evas_Object *data)
{
   Evas_Coord_Rectangle geometry;
   geometry.x = geometry.y = geometry.w = geometry.h = -1;

   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return geometry;
   if (!data || !strlen(elm_widget_type_get(data))) return geometry;

   Evas_Object *child_obj = data;
   Evas_Object *parent_obj;

   while ((parent_obj = elm_widget_parent_get(child_obj)))
     {
        if (!strcmp(elm_widget_type_get(parent_obj), "conformant"))
          {
             evas_object_geometry_get(child_obj, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
             return geometry;
          }
        child_obj = parent_obj;
     }

   return geometry;
}

static void
_elm_win_region_get_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord_Rectangle ret_rect;
   if (!wd) return;
   wd->region_get_job = NULL;

   if (!_elm_config->desktop_entry)
     {
        evas_smart_objects_calculate(evas_object_evas_get(data));

        ret_rect = _viewport_region_get(data);
        edje_object_part_text_viewport_region_set(wd->ent, "elm.text", ret_rect.x, ret_rect.y, ret_rect.w, ret_rect.h);
        ret_rect = _layout_region_get(data);
        edje_object_part_text_layout_region_set(wd->ent, "elm.text", ret_rect.x, ret_rect.y, ret_rect.w, ret_rect.h);
     }
}


static void
_signal_selection_end(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   if (wd->magnifier_enabled)
     _magnifier_hide(data);
   _menu_press(data);
}

static void
_signal_selection_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   const Eina_List *l;
   Evas_Object *entry;
   if (!wd) return;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        if (entry != data) elm_entry_select_none(entry);
     }
   wd->have_selection = EINA_TRUE;
   wd->selmode = EINA_TRUE;
   evas_object_smart_callback_call(data, SIG_SELECTION_START, NULL);
#ifdef HAVE_ELEMENTARY_X
   if (wd->sel_notify_handler)
     {
        const char *txt = elm_entry_selection_get(data);
        Evas_Object *top;

        top = elm_widget_top_get(data);
        if ((top) && (elm_win_xwindow_get(top)))
          elm_selection_set(ELM_SEL_PRIMARY, data, ELM_SEL_FORMAT_MARKUP, txt);
     }
#endif

}

static void
_signal_magnifier_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Coord cx, cy, cw, ch;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;

   edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text", &cx, &cy, &cw, &ch);
   if (!wd->deferred_recalc_job)
     elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
   else
     {
        wd->deferred_cur = EINA_TRUE;
        wd->cx = cx;
        wd->cy = cy;
        wd->cw = cw;
        wd->ch = ch;
     }
}

static void
_signal_selection_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Coord cx, cy, cw, ch;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->have_selection = EINA_TRUE;
   wd->selmode = EINA_TRUE;
   evas_object_smart_callback_call(data, SIG_SELECTION_CHANGED, NULL);
   elm_selection_set(ELM_SEL_PRIMARY, obj, ELM_SEL_FORMAT_MARKUP,
                     elm_entry_selection_get(data));

   edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text", &cx, &cy, &cw, &ch);
   if (!wd->deferred_recalc_job)
     elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
   else
     {
        wd->deferred_cur = EINA_TRUE;
        wd->cx = cx;
        wd->cy = cy;
        wd->cw = cw;
        wd->ch = ch;
     }
}

static void
_signal_selection_cleared(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (!wd->have_selection) return;
   wd->have_selection = EINA_FALSE;
   wd->selmode = EINA_FALSE;
   evas_object_smart_callback_call(data, SIG_SELECTION_CLEARED, NULL);
   if (wd->sel_notify_handler)
     {
        if (wd->cut_sel)
          {
#ifdef HAVE_ELEMENTARY_X
             Evas_Object *top;

             top = elm_widget_top_get(data);
             if ((top) && (elm_win_xwindow_get(top)))
               elm_selection_set(ELM_SEL_PRIMARY, data, ELM_SEL_FORMAT_MARKUP,
                                 wd->cut_sel);
#endif
             eina_stringshare_del(wd->cut_sel);
             wd->cut_sel = NULL;
          }
        else
          {
#ifdef HAVE_ELEMENTARY_X
             Evas_Object *top;

             top = elm_widget_top_get(data);
             if ((top) && (elm_win_xwindow_get(top)))
               elm_selection_clear(ELM_SEL_PRIMARY, data);
#endif
          }
     }

   if ((wd->api) && (wd->api->obj_hidemenu))
     {
        wd->api->obj_hidemenu(data);
     }
}

static void
_signal_entry_paste_request(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
   if (wd->sel_notify_handler)
     {
#ifdef HAVE_ELEMENTARY_X
        Evas_Object *top;

        top = elm_widget_top_get(data);
        if ((top) && (elm_win_xwindow_get(top)))
          {
             wd->selection_asked = EINA_TRUE;
             elm_selection_get(ELM_SEL_CLIPBOARD, ELM_SEL_FORMAT_MARKUP, data,
                               NULL, NULL);
          }
#endif
     }
}

static void
_signal_entry_copy_notify(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_SELECTION_COPY, NULL);
   elm_selection_set(ELM_SEL_CLIPBOARD, obj, ELM_SEL_FORMAT_MARKUP,
                     elm_entry_selection_get(data));
}

static void
_signal_entry_cut_notify(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_SELECTION_CUT, NULL);
   elm_selection_set(ELM_SEL_CLIPBOARD, obj, ELM_SEL_FORMAT_MARKUP,
                     elm_entry_selection_get(data));
   edje_object_part_text_insert(wd->ent, "elm.text", "");
   wd->changed = EINA_TRUE;
   _sizing_eval(data);
}

static void
_signal_cursor_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->cursor_pos = edje_object_part_text_cursor_pos_get(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
   wd->cur_changed = EINA_TRUE;
   _recalc_cursor_geometry(data);
}

static void
_signal_anchor_down(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
}

static void
_signal_anchor_up(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
}

static void
_signal_anchor_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Elm_Entry_Anchor_Info ei;
   char *buf2, *p, *p2, *n;
   if (!wd) return;
   p = strrchr(emission, ',');
   if (p)
     {
        const Eina_List *geoms;

        n = p + 1;
        p2 = p -1;
        while (p2 >= emission)
          {
             if (*p2 == ',') break;
             p2--;
          }
        p2++;
        buf2 = alloca(5 + p - p2);
        strncpy(buf2, p2, p - p2);
        buf2[p - p2] = 0;
        ei.name = n;
        ei.button = atoi(buf2);
        ei.x = ei.y = ei.w = ei.h = 0;
        geoms =
           edje_object_part_text_anchor_geometry_get(wd->ent, "elm.text", ei.name);
        if (geoms)
          {
             Evas_Textblock_Rectangle *r;
             const Eina_List *l;
             Evas_Coord px, py, x, y;

             evas_object_geometry_get(wd->ent, &x, &y, NULL, NULL);
             evas_pointer_canvas_xy_get(evas_object_evas_get(wd->ent), &px, &py);
             EINA_LIST_FOREACH(geoms, l, r)
               {
                  if (((r->x + x) <= px) && ((r->y + y) <= py) &&
                      ((r->x + x + r->w) > px) && ((r->y + y + r->h) > py))
                    {
                       ei.x = r->x + x;
                       ei.y = r->y + y;
                       ei.w = r->w;
                       ei.h = r->h;
                       break;
                    }
               }
          }
        if (!wd->disabled)
          evas_object_smart_callback_call(data, SIG_ANCHOR_CLICKED, &ei);
     }
}

static void
_signal_anchor_move(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
}

static void
_signal_anchor_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
}

static void
_signal_anchor_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
}

static void
_signal_key_enter(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_ACTIVATED, NULL);
}

static void
_signal_mouse_down(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->double_clicked = EINA_FALSE;
   evas_object_smart_callback_call(data, SIG_PRESS, NULL);

   if ((wd->api) && (wd->api->obj_hidemenu))
     wd->api->obj_hidemenu(data);
}

static void
_signal_mouse_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);

   if (!_elm_config->desktop_entry && !wd->double_clicked)
     _cancel(data, NULL, NULL);
}

static void
_signal_mouse_double(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->double_clicked = EINA_TRUE;
   evas_object_smart_callback_call(data, SIG_CLICKED_DOUBLE, NULL);
}

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool
_event_selection_notify(void *data, int type __UNUSED__, void *event)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Ecore_X_Event_Selection_Notify *ev = event;
   if (!wd) return ECORE_CALLBACK_PASS_ON;
   if ((!wd->selection_asked) && (!wd->drag_selection_asked))
     return ECORE_CALLBACK_PASS_ON;

   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
        Ecore_X_Selection_Data_Text *text_data;

        text_data = ev->data;
        if (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT)
          {
             if (text_data->text)
               {
                  char *txt = _elm_util_text_to_mkup(text_data->text);

                  if (txt)
                    {
                       elm_entry_entry_insert(data, txt);
                       free(txt);
                    }
               }
          }
        wd->selection_asked = EINA_FALSE;
     }
   else if (ev->selection == ECORE_X_SELECTION_XDND)
     {
        Ecore_X_Selection_Data_Text *text_data;

        text_data = ev->data;
        if (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT)
          {
             if (text_data->text)
               {
                  char *txt = _elm_util_text_to_mkup(text_data->text);

                  if (txt)
                    {
                       /* Massive FIXME: this should be at the drag point */
                       elm_entry_entry_insert(data, txt);
                       free(txt);
                    }
               }
          }
        wd->drag_selection_asked = EINA_FALSE;

        ecore_x_dnd_send_finished();

     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_event_selection_clear(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
#if 0
   Widget_Data *wd = elm_widget_data_get(data);
   Ecore_X_Event_Selection_Clear *ev = event;
   if (!wd) return ECORE_CALLBACK_PASS_ON;
   if (!wd->have_selection) return ECORE_CALLBACK_PASS_ON;
   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
        elm_entry_select_none(data);
     }
#else

   // start for cbhm
   Evas_Object *top = elm_widget_top_get(data);
   Ecore_X_Event_Selection_Clear *ev = event;

   if (!top)
      return ECORE_CALLBACK_PASS_ON;

   if (ev->selection != ECORE_X_SELECTION_SECONDARY)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   if (cnpwidgetdata == data)
     {
        Widget_Data *wd = elm_widget_data_get(data);
        Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;
        evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
        if (wd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
          formats = ELM_SEL_FORMAT_TEXT;
        else if (wd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
          formats |= ELM_SEL_FORMAT_IMAGE;
        elm_selection_get(ELM_SEL_SECONDARY, formats ,data, NULL, NULL);
     }

   // end for cbhm
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_drag_drop_cb(void *data __UNUSED__, Evas_Object *obj, Elm_Selection_Data *drop)
{
   Widget_Data *wd;
   Eina_Bool rv;

   wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   printf("Inserting at (%d,%d) %s\n",drop->x,drop->y,(char*)drop->data);

   edje_object_part_text_cursor_copy(wd->ent, "elm.text",
                                     EDJE_CURSOR_MAIN,/*->*/EDJE_CURSOR_USER);
   rv = edje_object_part_text_cursor_coord_set(wd->ent,"elm.text",
                                               EDJE_CURSOR_MAIN,drop->x,drop->y);
   if (!rv) printf("Warning: Failed to position cursor: paste anyway\n");
   elm_entry_entry_insert(obj, drop->data);
   edje_object_part_text_cursor_copy(wd->ent, "elm.text",
                                     EDJE_CURSOR_USER,/*->*/EDJE_CURSOR_MAIN);

   return EINA_TRUE;
}
#endif

static Evas_Object *
_get_item(void *data, Evas_Object *edje __UNUSED__, const char *part __UNUSED__, const char *item)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Object *o;
   Eina_List *l;
   Elm_Entry_Item_Provider *ip;

   EINA_LIST_FOREACH(wd->item_providers, l, ip)
     {
        o = ip->func(ip->data, data, item);
        if (o) return o;
     }
   if (!strncmp(item, "file://", 7))
     {
        const char *fname = item + 7;

        o = evas_object_image_filled_add(evas_object_evas_get(data));
        evas_object_image_file_set(o, fname, NULL);
        if (evas_object_image_load_error_get(o) == EVAS_LOAD_ERROR_NONE)
          {
             evas_object_show(o);
          }
        else
          {
             evas_object_del(o);
             o = edje_object_add(evas_object_evas_get(data));
             _elm_theme_object_set(data, o, "entry/emoticon", "wtf", elm_widget_style_get(data));
          }
        return o;
     }
   o = edje_object_add(evas_object_evas_get(data));
   if (!_elm_theme_object_set(data, o, "entry", item, elm_widget_style_get(data)))
     _elm_theme_object_set(data, o, "entry/emoticon", "wtf", elm_widget_style_get(data));
   return o;
}

static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf, char *key, const char *value, int deleteflag)
{
   const char *srcstring = NULL;
   Eina_Strbuf *repbuf = NULL, *diffbuf = NULL;
   char *curlocater = NULL, *replocater;
   char *starttag, *endtag;
   int tagtxtlen = 0, insertflag = 0;

   srcstring = eina_strbuf_string_get(srcbuf);
   if (srcstring)
     curlocater = strstr(srcstring, key);
   if (!curlocater || !srcstring)
     {
        insertflag = 1;
     }
   else
     {
       do
         {
           starttag = strchr(srcstring, '<');
           endtag = strchr(srcstring, '>');
           tagtxtlen = endtag - starttag;
           if (tagtxtlen <= 0) tagtxtlen = 0;
           if (starttag < curlocater && curlocater < endtag) break;
           if (endtag != NULL && endtag+1 != NULL)
             srcstring = endtag+1;
           else
             break;
         } while (strlen(srcstring) > 1);

       if (starttag && endtag && tagtxtlen > strlen(key))
         {
           repbuf = eina_strbuf_new();
           diffbuf = eina_strbuf_new();
           eina_strbuf_append_n(repbuf, starttag, tagtxtlen);
           srcstring = eina_strbuf_string_get(repbuf);
           curlocater = strstr(srcstring, key);

           if (curlocater != NULL)
             {
               replocater = curlocater + strlen(key) + 1;

               while ((*replocater) && (*replocater != ' ') && (*replocater != '>'))
                 replocater++;

               if (replocater-curlocater > strlen(key)+1)
                 {
                   eina_strbuf_append_n(diffbuf, curlocater, replocater-curlocater+1);
                 }
               else
                 insertflag = 1;
             }
           else
             {
               insertflag = 1;
             }
           eina_strbuf_reset(repbuf);
         }
       else
         {
           insertflag = 1;
         }
     }

   if (repbuf == NULL) repbuf = eina_strbuf_new();
   if (diffbuf == NULL) diffbuf = eina_strbuf_new();

   if (insertflag)
     {
       eina_strbuf_append_printf(repbuf, "<%s=%s>", key, value);
       eina_strbuf_prepend(srcbuf, eina_strbuf_string_get(repbuf));
     }
   else
     {
        if (deleteflag)
          {
            eina_strbuf_prepend(diffbuf, "<");
            eina_strbuf_append(diffbuf, ">");
            eina_strbuf_replace_first(srcbuf, eina_strbuf_string_get(diffbuf), "");
          }
        else
          {
            eina_strbuf_append_printf(repbuf, "%s=%s", key, value);
            eina_strbuf_replace_first(srcbuf, eina_strbuf_string_get(diffbuf), eina_strbuf_string_get(repbuf));
          }
     }

   if (repbuf) eina_strbuf_free(repbuf);
   if (diffbuf) eina_strbuf_free(diffbuf);

   return 0;
}

static int
_stringshare_key_value_replace(const char **srcstring, char *key, const char *value, int deleteflag)
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

static void
_text_filter(void *data, Evas_Object *edje __UNUSED__, const char *part __UNUSED__, Edje_Text_Filter_Type type, char **text)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Eina_List *l;
   Elm_Entry_Text_Filter *tf;

   if (type == EDJE_TEXT_FILTER_FORMAT)
     return;

   EINA_LIST_FOREACH(wd->text_filters, l, tf)
     {
        tf->func(tf->data, data, text);
        if (!*text)
          break;
     }
}

/* This function is used to insert text by chunks in jobs */
static Eina_Bool
_text_append_idler(void *data)
{
   int start;
   char backup;
   Evas_Object *obj = (Evas_Object *) data;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->text) eina_stringshare_del(wd->text);
   wd->text = NULL;
   if (wd->password_text) eina_stringshare_del(wd->password_text);
   wd->password_text = NULL;
   wd->changed = EINA_TRUE;

   start = wd->append_text_position;
   if (start + _CHUNK_SIZE < wd->append_text_len)
     {
        int pos = start;
        int tag_start, esc_start;

        tag_start = esc_start = -1;
        /* Find proper markup cut place */
        while (pos - start < _CHUNK_SIZE)
          {
             int prev_pos = pos;
             Eina_Unicode tmp =
                eina_unicode_utf8_get_next(wd->append_text_left, &pos);
             if (esc_start == -1)
               {
                  if (tmp == '<')
                     tag_start = prev_pos;
                  else if (tmp == '>')
                     tag_start = -1;
               }
             else if (tag_start == -1)
               {
                  if (tmp == '&')
                     esc_start = prev_pos;
                  else if (tmp == ';')
                     esc_start = -1;
               }
          }

        if (tag_start >= 0)
          {
             wd->append_text_position = tag_start;
          }
        else if (esc_start >= 0)
          {
             wd->append_text_position = esc_start;
          }
        else
          {
             wd->append_text_position = pos;
          }
     }
   else
     {
        wd->append_text_position = wd->append_text_len;
     }

   backup = wd->append_text_left[wd->append_text_position];
   wd->append_text_left[wd->append_text_position] = '\0';

   edje_object_part_text_append(wd->ent, "elm.text",
                                wd->append_text_left + start);

   wd->append_text_left[wd->append_text_position] = backup;

   /* If there's still more to go, renew the idler, else, cleanup */
   if (wd->append_text_position < wd->append_text_len)
     {
        return ECORE_CALLBACK_RENEW;
     }
   else
     {
        free(wd->append_text_left);
        wd->append_text_left = NULL;
        wd->append_text_idler = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
}

static void
_add_chars_till_limit(Evas_Object *obj, char **text, int can_add, Length_Unit unit)
{
   int i = 0, current_len = 0;
   char *new_text;

   if (!*text) return;
   if (unit >= LENGTH_UNIT_LAST) return;
   new_text = *text;
   current_len = strlen(*text);
   while (*new_text)
     {
        int idx = 0, unit_size = 0;
        char *markup, *utfstr;
        if (*new_text == '<')
          {
             while (*(new_text + idx) != '>')
               {
                  idx++;
                  if (!*(new_text + idx)) break;
               }
          }
        else if (*new_text == '&')
          {
             while (*(new_text + idx) != ';')
               {
                  idx++;
                  if (!*(new_text + idx)) break;
               }
          }
        idx = evas_string_char_next_get(new_text, idx, NULL);
        markup = malloc(idx + 1);
        if (markup)
          {
             strncpy(markup, new_text, idx);
             markup[idx] = 0;
             utfstr = elm_entry_markup_to_utf8(markup);
             if (utfstr)
               {
                  if (unit == LENGTH_UNIT_BYTE)
                    unit_size = strlen(utfstr);
                  else if (unit == LENGTH_UNIT_CHAR)
                    unit_size = evas_string_char_len_get(utfstr);
                  free(utfstr);
                  utfstr = NULL;
               }
             free(markup);
             markup = NULL;
          }
        if (can_add < unit_size)
          {
             if (!i)
               {
                  evas_object_smart_callback_call(obj, "maxlength,reached", NULL);
                  free(*text);
                  *text = NULL;
                  return;
               }
             can_add = 0;
             strncpy(new_text, new_text + idx, current_len - ((new_text + idx) - *text));
             current_len -= idx;
             (*text)[current_len] = 0;
          }
        else
          {
             new_text += idx;
             can_add -= unit_size;
          }
        i++;
     }
   evas_object_smart_callback_call(obj, "maxlength,reached", NULL);
}

static void
_elm_entry_text_set(Evas_Object *obj, const char *item, const char *entry)
{
   int len = 0;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!entry) entry = "";
   if (item && strcmp(item, "default"))
     {
        edje_object_part_text_set(wd->ent, item, entry);
        return;
     }

   if (wd->text) eina_stringshare_del(wd->text);
   wd->text = NULL;
   if (wd->password_text) eina_stringshare_del(wd->password_text);
   wd->password_text = NULL;
   wd->changed = EINA_TRUE;

   /* Clear currently pending job if there is one */
   if (wd->append_text_idler)
     {
        ecore_idler_del(wd->append_text_idler);
        free(wd->append_text_left);
        wd->append_text_left = NULL;
        wd->append_text_idler = NULL;
     }

   len = strlen(entry);
   /* Split to ~_CHUNK_SIZE chunks */
   if (len > _CHUNK_SIZE)
     {
        wd->append_text_left = (char *) malloc(len + 1);
     }

   /* If we decided to use the idler */
   if (wd->append_text_left)
     {
        /* Need to clear the entry first */
        edje_object_part_text_set(wd->ent, "elm.text", "");
        memcpy(wd->append_text_left, entry, len + 1);
        wd->append_text_position = 0;
        wd->append_text_len = len;
        wd->append_text_idler = ecore_idler_add(_text_append_idler, obj);
     }
   else
     {
        edje_object_part_text_set(wd->ent, "elm.text", entry);
     }
}

static const char *
_elm_entry_text_get(const Evas_Object *obj, const char *item)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (item && strcmp(item, "default")) return NULL;
   const char *text;
   if (!wd) return NULL;
   if (wd->password)
     {
        if(wd->password_text) return wd->password_text;
     }
   else if (wd->text) 
     {
        return wd->text;
     }
   text = edje_object_part_text_get(wd->ent, "elm.text");
   if (!text)
     {
        ERR("text=NULL for edje %p, part 'elm.text'", wd->ent);
        return NULL;
     }
   eina_stringshare_replace(&wd->text, text);
   if (wd->password)
     {
        const char *pw_text;
        pw_text = elm_entry_markup_to_utf8(wd->text);
        if (pw_text)
          {
             eina_stringshare_replace(&wd->password_text, pw_text);
             free(pw_text);
             return wd->password_text;
          }
     }
   return wd->text;
}

EAPI Evas_Object *
elm_entry_add(Evas_Object *parent)
{
   Evas_Object *obj, *top;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "entry");
   elm_widget_type_set(obj, "entry");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_disable_hook_set(obj, _disable_hook);
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);
   elm_widget_focus_region_hook_set(obj, _focus_region_hook);
   elm_widget_on_focus_region_hook_set(obj, _on_focus_region_hook);
   elm_widget_signal_callback_add_hook_set(obj, _signal_callback_add_hook);
   elm_widget_signal_callback_del_hook_set(obj, _signal_callback_del_hook);
   elm_object_cursor_set(obj, ELM_CURSOR_XTERM);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_highlight_ignore_set(obj, EINA_TRUE);
   elm_widget_text_set_hook_set(obj, _elm_entry_text_set);
   elm_widget_text_get_hook_set(obj, _elm_entry_text_get);
   elm_widget_content_set_hook_set(obj, _content_set_hook);
   elm_widget_content_unset_hook_set(obj, _content_unset_hook);
   elm_widget_content_get_hook_set(obj, _content_get_hook);
   elm_widget_translate_hook_set(obj, _translate_hook);

   evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, wd);

   wd->linewrap     = ELM_WRAP_WORD;
   wd->editable     = EINA_TRUE;
   wd->disabled     = EINA_FALSE;
   wd->context_menu = EINA_TRUE;
   wd->magnifier_enabled = EINA_TRUE;
   wd->autosave     = EINA_TRUE;
   wd->scroll       = EINA_FALSE;
   wd->cnp_mode     = ELM_CNP_MODE_MARKUP;
   wd->autoperiod   = EINA_TRUE;

   wd->ent = edje_object_add(e);
   elm_widget_sub_object_add(obj, wd->ent);
   edje_object_item_provider_set(wd->ent, _get_item, obj);
   edje_object_text_insert_filter_callback_add(wd->ent,"elm.text", _text_filter, obj);
   evas_object_event_callback_add(wd->ent, EVAS_CALLBACK_MOVE, _move, obj);
   evas_object_event_callback_add(wd->ent, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, obj);
   evas_object_event_callback_add(wd->ent, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, obj);
   evas_object_event_callback_add(wd->ent, EVAS_CALLBACK_MOUSE_MOVE,
                                  _mouse_move, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);

   _elm_theme_object_set(obj, wd->ent, "entry", "base", "default");
   edje_object_signal_callback_add(wd->ent, "entry,changed", "elm.text",
                                   _signal_entry_changed, obj);
   edje_object_signal_callback_add(wd->ent, "preedit,changed", "elm.text",
                                   _signal_preedit_changed, obj);
   edje_object_signal_callback_add(wd->ent, "handler,move,start", "elm.text",
                                   _signal_handler_move_start, obj);
   edje_object_signal_callback_add(wd->ent, "handler,move,end", "elm.text",
                                   _signal_handler_move_end, obj);
   edje_object_signal_callback_add(wd->ent, "handler,moving", "elm.text",
                                   _signal_handler_moving, obj);
   edje_object_signal_callback_add(wd->ent, "selection,start", "elm.text",
                                   _signal_selection_start, obj);
   edje_object_signal_callback_add(wd->ent, "selection,end", "elm.text",
                                   _signal_selection_end, obj);
   edje_object_signal_callback_add(wd->ent, "long,pressed", "elm.text",
                                   _signal_long_pressed, obj);
   edje_object_signal_callback_add(wd->ent, "magnifier,changed", "elm.text",
                                   _signal_magnifier_changed, obj);
   edje_object_signal_callback_add(wd->ent, "selection,changed", "elm.text",
                                   _signal_selection_changed, obj);
   edje_object_signal_callback_add(wd->ent, "selection,cleared", "elm.text",
                                   _signal_selection_cleared, obj);
   edje_object_signal_callback_add(wd->ent, "entry,paste,request", "elm.text",
                                   _signal_entry_paste_request, obj);
   edje_object_signal_callback_add(wd->ent, "entry,copy,notify", "elm.text",
                                   _signal_entry_copy_notify, obj);
   edje_object_signal_callback_add(wd->ent, "entry,cut,notify", "elm.text",
                                   _signal_entry_cut_notify, obj);
   edje_object_signal_callback_add(wd->ent, "cursor,changed", "elm.text",
                                   _signal_cursor_changed, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,down,*", "elm.text",
                                   _signal_anchor_down, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,up,*", "elm.text",
                                   _signal_anchor_up, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,clicked,*", "elm.text",
                                   _signal_anchor_clicked, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,move,*", "elm.text",
                                   _signal_anchor_move, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,in,*", "elm.text",
                                   _signal_anchor_in, obj);
   edje_object_signal_callback_add(wd->ent, "anchor,mouse,out,*", "elm.text",
                                   _signal_anchor_out, obj);
   edje_object_signal_callback_add(wd->ent, "entry,key,enter", "elm.text",
                                   _signal_key_enter, obj);
   edje_object_signal_callback_add(wd->ent, "mouse,down,1", "elm.text",
                                   _signal_mouse_down, obj);
   edje_object_signal_callback_add(wd->ent, "mouse,clicked,1", "elm.text",
                                   _signal_mouse_clicked, obj);
   edje_object_signal_callback_add(wd->ent, "mouse,down,1,double", "elm.text",
                                   _signal_mouse_double, obj);
   edje_object_part_text_set(wd->ent, "elm.text", "");
   if (_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_TRUE);
   else
     {
        edje_object_part_text_copy_paste_disabled_set(wd->ent, "elm.text", EINA_FALSE);
        edje_object_part_text_viewport_region_set(wd->ent, "elm.text", -1, -1, -1, -1);
        edje_object_part_text_layout_region_set(wd->ent, "elm.text", -1, -1, -1, -1);
     }
   elm_widget_resize_object_set(obj, wd->ent);
   _sizing_eval(obj);

   elm_entry_input_panel_layout_set(obj, ELM_INPUT_PANEL_LAYOUT_NORMAL);

   wd->input_panel_enable = edje_object_part_text_input_panel_enabled_get(wd->ent, "elm.text");
   wd->autocapital_type = edje_object_part_text_autocapital_type_get(wd->ent, "elm.text");

#ifdef HAVE_ELEMENTARY_X
   top = elm_widget_top_get(obj);
   if ((top) && (elm_win_xwindow_get(top)))
     {
        wd->sel_notify_handler =
           ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY,
                                   _event_selection_notify, obj);
        wd->sel_clear_handler =
           ecore_event_handler_add(ECORE_X_EVENT_SELECTION_CLEAR,
                                   _event_selection_clear, obj);
     }

   elm_drop_target_add(obj, ELM_SEL_FORMAT_MARKUP | ELM_SEL_FORMAT_IMAGE,
                       _drag_drop_cb, NULL);
#endif

   entries = eina_list_prepend(entries, obj);

   // module - find module for entry
   wd->api = _module(obj);
   // if found - hook in
   if ((wd->api) && (wd->api->obj_hook)) wd->api->obj_hook(obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);
   return obj;
}

EAPI void elm_entry_extension_module_data_get(Evas_Object *obj,Elm_Entry_Extension_data *ext_mod)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   ext_mod->cancel = _cancel;
   ext_mod->copy = _copy;
   ext_mod->cut = _cut;
   ext_mod->paste = _paste;
   ext_mod->select = _select;
   ext_mod->selectall = _selectall;
   ext_mod->ent = wd->ent;
   ext_mod->items = wd->items;
   ext_mod->editable = wd->editable;
   ext_mod->have_selection = wd->have_selection;
   ext_mod->password = wd->password;
   ext_mod->selmode = wd->selmode;
   ext_mod->cnpinit = _cnpinit;
   ext_mod->context_menu = wd->context_menu;
   ext_mod->cnp_mode = wd->cnp_mode;
   ext_mod->viewport_rect = _viewport_region_get(obj);
}

EAPI void
elm_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->single_line == single_line) return;
   wd->single_line = single_line;
   wd->linewrap = ELM_WRAP_NONE;
   elm_entry_cnp_mode_set(obj, ELM_CNP_MODE_NO_IMAGE);
   _theme_hook(obj);
   if (wd->scroller)
     {
        if (wd->single_line)
          {
             elm_smart_scroller_policy_set(wd->scroller,
                                           ELM_SMART_SCROLLER_POLICY_OFF,
                                           ELM_SMART_SCROLLER_POLICY_OFF);
             elm_smart_scroller_bounce_allow_set(wd->scroller, EINA_FALSE, EINA_FALSE);
          }
        else
          {
             const Elm_Scroller_Policy map[3] =
               {
                  ELM_SMART_SCROLLER_POLICY_AUTO,
                  ELM_SMART_SCROLLER_POLICY_ON,
                  ELM_SMART_SCROLLER_POLICY_OFF
               };
             elm_smart_scroller_policy_set(wd->scroller,
                                           map[wd->policy_h],
                                           map[wd->policy_v]);
             elm_smart_scroller_bounce_allow_set(wd->scroller, EINA_FALSE, EINA_FALSE);
          }
        _sizing_eval(obj);
     }
}

EAPI Eina_Bool
elm_entry_single_line_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->single_line;
}

EAPI void
elm_entry_password_set(Evas_Object *obj, Eina_Bool password)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->password == password) return;
   wd->password = password;
   wd->single_line = EINA_TRUE;
   wd->linewrap = ELM_WRAP_NONE;
   _theme_hook(obj);
}

EAPI Eina_Bool
elm_entry_password_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->password;
}

EAPI void
elm_entry_entry_set(Evas_Object *obj, const char *entry)
{
   _elm_entry_text_set(obj, NULL, entry);
}

EAPI void
elm_entry_entry_append(Evas_Object *obj, const char *entry)
{
   int len = 0;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!entry) entry = "";
   wd->changed = EINA_TRUE;

   len = strlen(entry);
   if (wd->append_text_left)
     {
        char *tmpbuf;
        tmpbuf = realloc(wd->append_text_left, wd->append_text_len + len + 1);
        if (!tmpbuf)
          {
             /* Do something */
             return;
          }
        wd->append_text_left = tmpbuf;
        memcpy(wd->append_text_left + wd->append_text_len, entry, len + 1);
        wd->append_text_len += len;
     }
   else
     {
        /* FIXME: Add chunked appending here (like in entry_set) */
        edje_object_part_text_append(wd->ent, "elm.text", entry);
     }
}

EAPI const char *
elm_entry_entry_get(const Evas_Object *obj)
{
   return _elm_entry_text_get(obj, NULL);
}

EAPI Eina_Bool
elm_entry_is_empty(const Evas_Object *obj)
{
   /* FIXME: until there's support for that in textblock, we just check
    * to see if the there is text or not. */
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_TRUE;
   Widget_Data *wd = elm_widget_data_get(obj);
   const Evas_Object *tb;
   Evas_Textblock_Cursor *cur;
   Eina_Bool ret;
   if (!wd) return EINA_TRUE;

   if (0) {
   /* It's a hack until we get the support suggested above.
    * We just create a cursor, point it to the begining, and then
    * try to advance it, if it can advance, the tb is not empty,
    * otherwise it is. */
   tb = edje_object_part_object_get(wd->ent, "elm.text");
   cur = evas_object_textblock_cursor_new((Evas_Object *) tb); /* This is
                                                                  actually, ok for the time being, thsese hackish stuff will be removed
                                                                  once evas 1.0 is out*/
   evas_textblock_cursor_pos_set(cur, 0);
   ret = evas_textblock_cursor_char_next(cur);
   evas_textblock_cursor_free(cur);

   return !ret;
   }

   char *str = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
   if (!str) return EINA_TRUE;

   ret = (strlen(str) == 0);

   free(str);
   return ret;
}

EAPI const Evas_Object *
elm_entry_textblock_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;

   return edje_object_part_object_get(wd->ent, "elm.text");
}

EAPI void
elm_entry_calc_force(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   edje_object_calc_force(wd->ent);
}


EAPI const char *
elm_entry_selection_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return edje_object_part_text_selection_get(wd->ent, "elm.text");
}

EAPI void
elm_entry_entry_insert(Evas_Object *obj, const char *entry)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_insert(wd->ent, "elm.text", entry);
   // start for cbhm
#ifdef HAVE_ELEMENTARY_X
   if (cnpwidgetdata == obj)
      ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), "",1);
#endif
   // end for cbhm
   wd->changed = EINA_TRUE;
   _sizing_eval(obj);
}

EAPI void
elm_entry_line_wrap_set(Evas_Object *obj, Elm_Wrap_Type wrap)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->linewrap == wrap) return;
   wd->lastw = -1;
   wd->linewrap = wrap;
   _theme_hook(obj);
}

EAPI Elm_Wrap_Type
elm_entry_line_wrap_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->linewrap;
}

EAPI void
elm_entry_editable_set(Evas_Object *obj, Eina_Bool editable)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->editable == editable) return;
   wd->editable = editable;
   _theme_hook(obj);

#ifdef HAVE_ELEMENTARY_X
   if (editable)
     elm_drop_target_add(obj, ELM_SEL_FORMAT_MARKUP, _drag_drop_cb, NULL);
   else
     elm_drop_target_del(obj);
#endif
}

EAPI Eina_Bool
elm_entry_editable_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->editable;
}

EAPI void
elm_entry_select_none(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->selmode)
     {
        wd->selmode = EINA_FALSE;
        if (!_elm_config->desktop_entry)
          edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_FALSE);
        edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
     }
   wd->have_selection = EINA_FALSE;
   edje_object_part_text_select_none(wd->ent, "elm.text");
}

EAPI void
elm_entry_select_all(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->selmode)
     {
        wd->selmode = EINA_FALSE;
        if (!_elm_config->desktop_entry)
          edje_object_part_text_select_allow_set(wd->ent, "elm.text", EINA_FALSE);
        edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
     }
   wd->have_selection = EINA_TRUE;
   edje_object_part_text_select_all(wd->ent, "elm.text");
}

EAPI Eina_Bool
elm_entry_cursor_geometry_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   edje_object_part_text_cursor_geometry_get(wd->ent, "elm.text", x, y, w, h);
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_entry_cursor_next(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_next(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_prev(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_prev(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_up(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_up(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_down(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_down(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_begin_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_cursor_begin_set(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_end_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_cursor_end_set(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
   edje_object_message_signal_process(wd->ent);
}

EAPI void
elm_entry_cursor_line_begin_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_cursor_line_begin_set(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_line_end_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_cursor_line_end_set(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_selection_begin(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_select_begin(wd->ent, "elm.text");
}

EAPI void
elm_entry_cursor_selection_end(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_select_extend(wd->ent, "elm.text");
}

EAPI Eina_Bool
elm_entry_cursor_is_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_is_format_get(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_is_visible_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return edje_object_part_text_cursor_is_visible_format_get(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI const char *
elm_entry_cursor_content_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return edje_object_part_text_cursor_content_get(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_pos_set(Evas_Object *obj, int pos)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_part_text_cursor_pos_set(wd->ent, "elm.text", EDJE_CURSOR_MAIN, pos);
   edje_object_message_signal_process(wd->ent);
}

EAPI int
elm_entry_cursor_pos_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return edje_object_part_text_cursor_pos_get(wd->ent, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_selection_cut(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _cut(obj, NULL, NULL);
}

EAPI void
elm_entry_selection_copy(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _copy(obj, NULL, NULL);
}

EAPI void
elm_entry_selection_paste(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _paste(obj, NULL, NULL);
}

EAPI void
elm_entry_context_menu_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Entry_Context_Menu_Item *it;
   if (!wd) return;
   EINA_LIST_FREE(wd->items, it)
     {
        eina_stringshare_del(it->label);
        eina_stringshare_del(it->icon_file);
        eina_stringshare_del(it->icon_group);
        free(it);
     }
}

EAPI void
elm_entry_context_menu_item_add(Evas_Object *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Entry_Context_Menu_Item *it;
   if (!wd) return;
   it = calloc(1, sizeof(Elm_Entry_Context_Menu_Item));
   if (!it) return;
   wd->items = eina_list_append(wd->items, it);
   it->obj = obj;
   it->label = eina_stringshare_add(label);
   it->icon_file = eina_stringshare_add(icon_file);
   it->icon_type = icon_type;
   it->func = func;
   it->data = (void *)data;
}

EAPI void
elm_entry_context_menu_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->context_menu == !disabled) return;
   wd->context_menu = !disabled;

   if (!_elm_config->desktop_entry)
     edje_object_part_text_copy_paste_disabled_set(wd->ent, "elm.text", disabled);
}

EAPI Eina_Bool
elm_entry_context_menu_disabled_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return !wd->context_menu;
}

EAPI void
elm_entry_magnifier_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->magnifier_enabled == !disabled) return;
   wd->magnifier_enabled = !disabled;
}

EAPI Eina_Bool
elm_entry_magnifier_disabled_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return !wd->magnifier_enabled;
}

EAPI void
elm_entry_item_provider_append(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(func);
   Elm_Entry_Item_Provider *ip = calloc(1, sizeof(Elm_Entry_Item_Provider));
   if (!ip) return;
   ip->func = func;
   ip->data = data;
   wd->item_providers = eina_list_append(wd->item_providers, ip);
}

EAPI void
elm_entry_item_provider_prepend(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(func);
   Elm_Entry_Item_Provider *ip = calloc(1, sizeof(Elm_Entry_Item_Provider));
   if (!ip) return;
   ip->func = func;
   ip->data = data;
   wd->item_providers = eina_list_prepend(wd->item_providers, ip);
}

EAPI void
elm_entry_item_provider_remove(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Elm_Entry_Item_Provider *ip;
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(func);
   EINA_LIST_FOREACH(wd->item_providers, l, ip)
     {
        if ((ip->func == func) && ((!data) || (ip->data == data)))
          {
             wd->item_providers = eina_list_remove_list(wd->item_providers, l);
             free(ip);
             return;
          }
     }
}

EAPI void
elm_entry_text_filter_append(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{
   Widget_Data *wd;
   Elm_Entry_Text_Filter *tf;
   ELM_CHECK_WIDTYPE(obj, widtype);

   wd = elm_widget_data_get(obj);

   EINA_SAFETY_ON_NULL_RETURN(func);

   tf = _filter_new(func, data);
   if (!tf) return;

   wd->text_filters = eina_list_append(wd->text_filters, tf);
}

EAPI void
elm_entry_text_filter_prepend(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{
   Widget_Data *wd;
   Elm_Entry_Text_Filter *tf;
   ELM_CHECK_WIDTYPE(obj, widtype);

   wd = elm_widget_data_get(obj);

   EINA_SAFETY_ON_NULL_RETURN(func);

   tf = _filter_new(func, data);
   if (!tf) return;

   wd->text_filters = eina_list_prepend(wd->text_filters, tf);
}

EAPI void
elm_entry_text_filter_remove(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Entry_Text_Filter *tf;
   ELM_CHECK_WIDTYPE(obj, widtype);

   wd = elm_widget_data_get(obj);

   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(wd->text_filters, l, tf)
     {
        if ((tf->func == func) && ((!data) || (tf->data == data)))
          {
             wd->text_filters = eina_list_remove_list(wd->text_filters, l);
             _filter_free(tf);
             return;
          }
     }
}

EAPI char *
elm_entry_markup_to_utf8(const char *s)
{
   char *ss = _elm_util_mkup_to_text(s);
   if (!ss) ss = strdup("");
   return ss;
}

EAPI char *
elm_entry_utf8_to_markup(const char *s)
{
   char *ss = _elm_util_text_to_mkup(s);
   if (!ss) ss = strdup("");
   return ss;
}

EAPI void
elm_entry_filter_limit_size(void *data, Evas_Object *entry, char **text)
{
   Elm_Entry_Filter_Limit_Size *lim = data;
   char *current;
   int len, newlen;
   const char *(*text_get)(const Evas_Object *);
   const char *widget_type;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(entry);
   EINA_SAFETY_ON_NULL_RETURN(text);

   /* hack. I don't want to copy the entire function to work with
    * scrolled_entry */
   widget_type = elm_widget_type_get(entry);
   if (!strcmp(widget_type, "entry"))
     text_get = elm_entry_entry_get;
   else /* huh? */
     return;

   current = elm_entry_markup_to_utf8(text_get(entry));

   if (lim->max_char_count > 0)
     {
        len = evas_string_char_len_get(current);
        if (len >= lim->max_char_count)
          {
             evas_object_smart_callback_call(entry, "maxlength,reached", NULL);
             free(*text);
             free(current);
             *text = NULL;
             return;
          }
        newlen = evas_string_char_len_get(elm_entry_markup_to_utf8(*text));
        if ((len + newlen) > lim->max_char_count)
          _add_chars_till_limit(entry, text, (lim->max_char_count - len), LENGTH_UNIT_CHAR);
     }
   else if (lim->max_byte_count > 0)
     {
        len = strlen(current);
        if (len >= lim->max_byte_count)
          {
             evas_object_smart_callback_call(entry, "maxlength,reached", NULL);
             free(*text);
             free(current);
             *text = NULL;
             return;
          }
        newlen = strlen(elm_entry_markup_to_utf8(*text));
        if ((len + newlen) > lim->max_byte_count)
          _add_chars_till_limit(entry, text, (lim->max_byte_count - len), LENGTH_UNIT_BYTE);
     }
   free(current);
}

EAPI void
elm_entry_filter_accept_set(void *data, Evas_Object *entry __UNUSED__, char **text)
{
   Elm_Entry_Filter_Accept_Set *as = data;
   const char *set;
   char *insert;
   Eina_Bool goes_in;
   int read_idx, last_read_idx = 0, read_char;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(text);

   if ((!as->accepted) && (!as->rejected))
     return;

   if (as->accepted)
     {
        set = as->accepted;
        goes_in = EINA_TRUE;
     }
   else
     {
        set = as->rejected;
        goes_in = EINA_FALSE;
     }

   insert = *text;
   read_idx = evas_string_char_next_get(*text, 0, &read_char);
   while (read_char)
     {
        int cmp_idx, cmp_char;
        Eina_Bool in_set = EINA_FALSE;

        cmp_idx = evas_string_char_next_get(set, 0, &cmp_char);
        while (cmp_char)
          {
             if (read_char == cmp_char)
               {
                  in_set = EINA_TRUE;
                  break;
               }
             cmp_idx = evas_string_char_next_get(set, cmp_idx, &cmp_char);
          }
        if (in_set == goes_in)
          {
             int size = read_idx - last_read_idx;
             const char *src = (*text) + last_read_idx;
             if (src != insert)
               memcpy(insert, *text + last_read_idx, size);
             insert += size;
          }
        last_read_idx = read_idx;
        read_idx = evas_string_char_next_get(*text, read_idx, &read_char);
     }
   *insert = 0;
}

EAPI void
elm_entry_file_set(Evas_Object *obj, const char *file, Elm_Text_Format format)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->delay_write)
     {
        ecore_timer_del(wd->delay_write);
        wd->delay_write = NULL;
     }
   if (wd->autosave) _save(obj);
   eina_stringshare_replace(&wd->file, file);
   wd->format = format;
   _load(obj);
}

EAPI void
elm_entry_file_get(const Evas_Object *obj, const char **file, Elm_Text_Format *format)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (file) *file = wd->file;
   if (format) *format = wd->format;
}

EAPI void
elm_entry_file_save(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->delay_write)
     {
        ecore_timer_del(wd->delay_write);
        wd->delay_write = NULL;
     }
   _save(obj);
   wd->delay_write = ecore_timer_add(2.0, _delay_write, obj);
}

EAPI void
elm_entry_autosave_set(Evas_Object *obj, Eina_Bool autosave)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->autosave = !!autosave;
}

EAPI Eina_Bool
elm_entry_autosave_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->autosave;
}

EAPI void
elm_entry_cnp_textonly_set(Evas_Object *obj, Eina_Bool textonly)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   elm_entry_cnp_mode_set(obj, textonly ? ELM_CNP_MODE_NO_IMAGE : ELM_CNP_MODE_MARKUP);
}

EAPI Eina_Bool
elm_entry_cnp_textonly_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   return elm_entry_cnp_mode_get(obj) != ELM_CNP_MODE_MARKUP;
}

EAPI void
elm_entry_cnp_mode_set(Evas_Object *obj, Elm_CNP_Mode cnp_mode)
{
   Elm_Sel_Format format = ELM_SEL_FORMAT_MARKUP;
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->cnp_mode == cnp_mode) return;
   wd->cnp_mode = cnp_mode;
   if (wd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
     format = ELM_SEL_FORMAT_TEXT;
   else if (cnp_mode == ELM_CNP_MODE_MARKUP) format |= ELM_SEL_FORMAT_IMAGE;
#ifdef HAVE_ELEMENTARY_X
   elm_drop_target_add(obj, format, _drag_drop_cb, NULL);
#endif
}

EAPI Elm_CNP_Mode
elm_entry_cnp_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->cnp_mode;
}

EAPI void
elm_entry_scrollable_set(Evas_Object *obj, Eina_Bool scroll)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   const Elm_Scroller_Policy map[3] =
     {
        ELM_SMART_SCROLLER_POLICY_AUTO,
        ELM_SMART_SCROLLER_POLICY_ON,
        ELM_SMART_SCROLLER_POLICY_OFF
     };
   if (!wd) return;
   scroll = !!scroll;
   if (wd->scroll == scroll) return;
   wd->scroll = scroll;
   if (wd->scroll)
     {
        if (!wd->scroller)
          {
             wd->scroller = elm_smart_scroller_add(evas_object_evas_get(obj));
             elm_widget_resize_object_set(obj, wd->scroller);
             elm_smart_scroller_widget_set(wd->scroller, obj);
             elm_smart_scroller_object_theme_set(obj, wd->scroller, "scroller", "entry",
                                                 elm_widget_style_get(obj));
             evas_object_size_hint_weight_set(wd->scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(wd->scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_propagate_events_set(wd->scroller, EINA_TRUE);
             evas_object_propagate_events_set(elm_smart_scroller_edje_object_get(wd->scroller),
                                              EINA_TRUE);
          }
        elm_smart_scroller_bounce_allow_set(wd->scroller, wd->h_bounce, wd->v_bounce);
        if (wd->single_line)
          elm_smart_scroller_policy_set(wd->scroller, ELM_SMART_SCROLLER_POLICY_OFF,
                                        ELM_SMART_SCROLLER_POLICY_OFF);
        else
          elm_smart_scroller_policy_set(wd->scroller, map[wd->policy_h], map[wd->policy_v]);
        elm_widget_sub_object_add(obj, wd->ent);
        elm_smart_scroller_child_set(wd->scroller, wd->ent);
        evas_object_show(wd->scroller);
        elm_widget_on_show_region_hook_set(obj, _show_region_hook, obj);
     }
   else
     {
        if (wd->scroller)
          {
             elm_smart_scroller_child_set(wd->scroller, NULL);
             evas_object_smart_member_add(wd->scroller, obj);
             elm_widget_sub_object_add(obj, wd->scroller);
             evas_object_hide(wd->scroller);
          }
        elm_widget_sub_object_del(obj, wd->ent);
        elm_widget_resize_object_set(obj, wd->ent);
        elm_widget_on_show_region_hook_set(obj, NULL, NULL);
     }
   wd->lastw = -1;
   _theme_hook(obj);
}

EAPI Eina_Bool
elm_entry_scrollable_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->scroll;
}

EAPI void
elm_entry_icon_set(Evas_Object *obj, Evas_Object *icon)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *edje;
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(icon);
   if (wd->icon == icon) return;
   if (wd->icon) evas_object_del(wd->icon);
   wd->icon = icon;
   edje = elm_smart_scroller_edje_object_get(wd->scroller);
   if (!edje) return;
   edje_object_part_swallow(edje, "elm.swallow.icon", wd->icon);
   edje_object_signal_emit(edje, "elm,action,show,icon", "elm");
   _sizing_eval(obj);
}

EAPI Evas_Object *
elm_entry_icon_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->icon;
}

EAPI Evas_Object *
elm_entry_icon_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *ret = NULL;
   if (!wd) return NULL;
   if (wd->icon)
     {
        Evas_Object *edje = elm_smart_scroller_edje_object_get(wd->scroller);
        if (!edje) return NULL;
        ret = wd->icon;
        edje_object_part_unswallow(edje, wd->icon);
        edje_object_signal_emit(edje, "elm,action,hide,icon", "elm");
        wd->icon = NULL;
        _sizing_eval(obj);
     }
   return ret;
}

EAPI void
elm_entry_icon_visible_set(Evas_Object *obj, Eina_Bool setting)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->icon)) return;
   if (setting)
      evas_object_hide(wd->icon);
   else
      evas_object_show(wd->icon);
   _sizing_eval(obj);
}

EAPI void
elm_entry_end_set(Evas_Object *obj, Evas_Object *end)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *edje;
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(end);
   if (wd->end == end) return;
   if (wd->end) evas_object_del(wd->end);
   wd->end = end;
   edje = elm_smart_scroller_edje_object_get(wd->scroller);
   if (!edje) return;
   edje_object_part_swallow(edje, "elm.swallow.end", wd->end);
   edje_object_signal_emit(edje, "elm,action,show,end", "elm");
   _sizing_eval(obj);
}

EAPI Evas_Object *
elm_entry_end_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->end;
}

EAPI Evas_Object *
elm_entry_end_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *ret = NULL;
   if (!wd) return NULL;
   if (wd->end)
     {
        Evas_Object *edje = elm_smart_scroller_edje_object_get(wd->scroller);
        if (!edje) return NULL;
        ret = wd->end;
        edje_object_part_unswallow(edje, wd->end);
        edje_object_signal_emit(edje, "elm,action,hide,end", "elm");
        wd->end = NULL;
        _sizing_eval(obj);
     }
   return ret;
}

EAPI void
elm_entry_end_visible_set(Evas_Object *obj, Eina_Bool setting)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if ((!wd) || (!wd->end)) return;
   if (setting)
      evas_object_hide(wd->end);
   else
      evas_object_show(wd->end);
   _sizing_eval(obj);
}

EAPI void
elm_entry_scrollbar_policy_set(Evas_Object *obj, Elm_Scroller_Policy h, Elm_Scroller_Policy v)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   const Elm_Scroller_Policy map[3] =
     {
        ELM_SMART_SCROLLER_POLICY_AUTO,
        ELM_SMART_SCROLLER_POLICY_ON,
        ELM_SMART_SCROLLER_POLICY_OFF
     };
   if (!wd) return;
   wd->policy_h = h;
   wd->policy_v = v;
   elm_smart_scroller_policy_set(wd->scroller,
                                 map[wd->policy_h],
                                 map[wd->policy_v]);
}

EAPI void
elm_entry_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->h_bounce = h_bounce;
   wd->v_bounce = v_bounce;
   elm_smart_scroller_bounce_allow_set(wd->scroller, h_bounce, v_bounce);
}

EAPI void
elm_entry_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_bounce_allow_get(wd->scroller, h_bounce, v_bounce);
}

EAPI void
elm_entry_input_panel_layout_set(Evas_Object *obj, Elm_Input_Panel_Layout layout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->input_panel_layout = layout;

   edje_object_part_text_input_panel_layout_set(wd->ent, "elm.text", layout);
}

EAPI Elm_Input_Panel_Layout
elm_entry_input_panel_layout_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_INPUT_PANEL_LAYOUT_INVALID;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_INPUT_PANEL_LAYOUT_INVALID;

   return wd->input_panel_layout;
}

EAPI void
elm_entry_autocapital_type_set(Evas_Object *obj, Elm_Autocapital_Type autocapital_type)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->autocapital_type = autocapital_type;
   edje_object_part_text_autocapital_type_set(wd->ent, "elm.text", autocapital_type);
}

EAPI Elm_Autocapital_Type
elm_entry_autocapital_type_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_AUTOCAPITAL_TYPE_NONE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_AUTOCAPITAL_TYPE_NONE;

   return wd->autocapital_type;
}

EAPI void
elm_entry_input_panel_enabled_set(Evas_Object *obj, Eina_Bool enabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->input_panel_enable = enabled;
   edje_object_part_text_input_panel_enabled_set(wd->ent, "elm.text", enabled);
}

EINA_DEPRECATED EAPI void
elm_entry_line_char_wrap_set(Evas_Object *obj, Eina_Bool wrap)
{
   if (wrap) elm_entry_line_wrap_set(obj, ELM_WRAP_CHAR);
}

EINA_DEPRECATED EAPI void
elm_entry_background_color_set(Evas_Object *obj, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   evas_object_color_set(wd->bg, r, g, b, a);

   if (wd->bgcolor == EINA_FALSE)
     {
       wd->bgcolor = 1;
       edje_object_part_swallow(wd->ent, "entry.swallow.background", wd->bg);
     }
}

EAPI void
elm_entry_autocapitalization_set(Evas_Object *obj, Eina_Bool autocap)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (autocap)
     wd->autocapital_type = ELM_AUTOCAPITAL_TYPE_SENTENCE;
   else
     wd->autocapital_type = ELM_AUTOCAPITAL_TYPE_NONE;

   if (wd->input_panel_layout == ELM_INPUT_PANEL_LAYOUT_URL ||
       wd->input_panel_layout == ELM_INPUT_PANEL_LAYOUT_EMAIL)
     wd->autocapital_type = ELM_AUTOCAPITAL_TYPE_NONE;

   edje_object_part_text_autocapital_type_set(wd->ent, "elm.text", wd->autocapital_type);
}

EAPI void
elm_entry_autoperiod_set(Evas_Object *obj, Eina_Bool autoperiod)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->password)
     wd->autoperiod = EINA_FALSE;
   else
     wd->autoperiod = autoperiod;

   if (wd->input_panel_layout == ELM_INPUT_PANEL_LAYOUT_URL ||
       wd->input_panel_layout == ELM_INPUT_PANEL_LAYOUT_EMAIL)
     wd->autoperiod = EINA_FALSE;

   edje_object_part_text_autoperiod_set(wd->ent, "elm.text", wd->autoperiod);
}

EAPI void
elm_entry_autoenable_returnkey_set(Evas_Object *obj, Eina_Bool on)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->autoreturnkey = on;
   _check_enable_returnkey(obj);
}

EAPI Ecore_IMF_Context *elm_entry_imf_context_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->ent) return NULL;

   return edje_object_part_text_imf_context_get(wd->ent, "elm.text");
}

EAPI void
elm_entry_matchlist_set(Evas_Object *obj, Eina_List *match_list, Eina_Bool case_sensitive)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (match_list)
     {
        Evas_Coord max_w = 9999, max_h = 9999;
        const char* key_data = NULL;

        wd->matchlist_threshold = 1;
        wd->hover = elm_hover_add(elm_widget_parent_get(obj));
        elm_hover_parent_set(wd->hover, elm_widget_parent_get(obj));
        elm_hover_target_set(wd->hover, obj);
        elm_object_style_set(wd->hover, "matchlist");

        wd->layout = elm_layout_add(wd->hover);
        elm_layout_theme_set(wd->layout, "entry", "matchlist", "default");
        wd->list = elm_list_add(wd->layout);
        evas_object_size_hint_weight_set(wd->list, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(wd->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_list_mode_set(wd->list, ELM_LIST_EXPAND);
        elm_object_style_set(wd->list, "matchlist");

        key_data = edje_object_data_get(elm_layout_edje_get(wd->layout), "max_width");
        if (key_data) max_w = atoi(key_data);
        key_data = edje_object_data_get(elm_layout_edje_get(wd->layout), "max_height");
        if (key_data) max_h = atoi(key_data);

        elm_list_go(wd->list);
        evas_object_size_hint_max_set(wd->list, max_w, max_h);
        evas_object_smart_callback_add(wd->list, "selected", _matchlist_list_clicked, obj);
        elm_layout_content_set(wd->layout, "elm.swallow.content", wd->list);
        elm_hover_content_set(wd->hover, "bottom", wd->layout);

        wd->match_list = match_list;
     }
   else
     {
        if (wd->hover)
          evas_object_del(wd->hover);

        wd->match_list = NULL;
     }

   wd->matchlist_case_sensitive = case_sensitive;
}

EAPI void
elm_entry_magnifier_type_set(Evas_Object *obj, int type)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->mgf_type = type;
   _magnifier_create(obj);
}
