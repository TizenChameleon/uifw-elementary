#include <Elementary.h>
#include "elm_module_priv.h"
#include "elm_priv.h"
#include <appsvc/appsvc.h>
#include "cbhm_helper.h"

#define MULTI_(id) dgettext("sys_string", #id)
#define S_SELECT MULTI_(IDS_COM_SK_SELECT)
#define S_SELECT_ALL MULTI_(IDS_COM_BODY_SELECT_ALL)
#define S_COPY MULTI_(IDS_COM_BODY_COPY)
#define S_CUT MULTI_(IDS_COM_BODY_CUT)
#define S_PASTE MULTI_(IDS_COM_BODY_PASTE)
#define S_CLIPBOARD MULTI_(IDS_COM_BODY_CLIPBOARD)


Elm_Entry_Extension_data *ext_mod;
static int _mod_hook_count = 0;

typedef struct _Elm_Entry_Context_Menu_Item Elm_Entry_Context_Menu_Item;
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

static void _ctxpopup_hide(Evas_Object *popup);
static void _ctxpopup_position(Evas_Object *obj);

static char *
_remove_tags(const char *str)
{
   char *ret;
   if (!str)
     return NULL;

   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf)
     return NULL;

   if (!eina_strbuf_append(buf, str))
     return NULL;

   eina_strbuf_replace_all(buf, "<br>", " ");
   eina_strbuf_replace_all(buf, "<br/>", " ");
   eina_strbuf_replace_all(buf, "<ps>", " ");
   eina_strbuf_replace_all(buf, "<ps/>", " ");

   while (EINA_TRUE)
     {
        const char *temp = eina_strbuf_string_get(buf);

        char *startTag = NULL;
        char *endTag = NULL;

        startTag = strstr(temp, "<");
        if (startTag)
          endTag = strstr(startTag, ">");
        else
          break;
        if (!endTag || startTag > endTag)
          break;

        size_t sindex = startTag - temp;
        size_t eindex = endTag - temp + 1;
        if (!eina_strbuf_remove(buf, sindex, eindex))
          break;
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static void
_entry_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_del(data);
}

static void
_entry_hide_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_hide(data);
}

static void
_entry_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (evas_pointer_button_down_mask_get(e))
     _ctxpopup_hide(data);
   else
     {
        /*update*/
        elm_entry_extension_module_data_get(obj, ext_mod);
        _ctxpopup_position(data);
     }
}

static void
_entry_resize_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _ctxpopup_hide(data);
}

static void
_ctxpopup_hide(Evas_Object *popup)
{
   evas_object_hide(popup);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_DEL, _entry_del_cb);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_HIDE, _entry_hide_cb);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOVE, _entry_move_cb);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_RESIZE, _entry_resize_cb);
}

static void
_ctxpopup_position(Evas_Object *obj)
{
   if(!ext_mod) return;

   Evas_Coord cx, cy, cw, ch, x, y, w, h;
   elm_ctxpopup_direction_priority_set(ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_RIGHT);
   if (!edje_object_part_text_selection_geometry_get(ext_mod->ent, "elm.text", &x, &y, &w, &h))
     {
        evas_object_geometry_get(ext_mod->ent, &x, &y, NULL, NULL);
        edje_object_part_text_cursor_geometry_get(ext_mod->ent, "elm.text",
                                                  &cx, &cy, &cw, &ch);
        evas_object_size_hint_min_get(ext_mod->popup, &w, &h);
        if (cw < w)
          {
             cx += (cw - w) / 2;
             cw = w;
          }
        if (ch < h)
          {
             cy += (ch - h) / 2;
             ch = h;
          }
        evas_object_move(ext_mod->popup, x + cx, y + cy);
        evas_object_resize(ext_mod->popup, cw, ch);
     }
   else
     {
        if (ext_mod->viewport_rect.x != -1 || ext_mod->viewport_rect.y != -1
            || ext_mod->viewport_rect.w != -1 || ext_mod->viewport_rect.h != -1)
          {
             Evas_Coord vx, vy, vw, vh, x2, y2;
             x2 = x + w;
             y2 = y + h;
             vx = ext_mod->viewport_rect.x;
             vy = ext_mod->viewport_rect.y;
             vw = ext_mod->viewport_rect.w;
             vh = ext_mod->viewport_rect.h;

             if (x < vx) x = vx;
             if (y < vy) y = vy;
             if (x2 > vx + vw) x2 = vx + vw;
             if (y2 > vy + vh) y2 = vy + vh;
             w = x2 - x;
             h = y2 - y;
          }
        else
          {
             Evas_Coord sw, sh, x2, y2;
             x2 = x + w;
             y2 = y + h;
             ecore_x_window_size_get(ecore_x_window_root_first_get(), &sw, &sh);

             if (x < 0) x = 0;
             if (y < 0) y = 0;
             if (x2 > sw) x2 = sw;
             if (y2 > sh) y2 = sh;
             w = x2 - x;
             h = y2 - y;
          }
        cx = x + (w / 2);
        cy = y + (h / 2);
        Elm_Ctxpopup_Direction dir = elm_ctxpopup_direction_get(ext_mod->popup);
        if (dir != ELM_CTXPOPUP_DIRECTION_UNKNOWN)
          {
             if (dir == ELM_CTXPOPUP_DIRECTION_UP)
               cy = y + (h / 5);
             else if (dir == ELM_CTXPOPUP_DIRECTION_DOWN)
               {
                  elm_ctxpopup_direction_priority_set(ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_RIGHT);
                  cy = y + h;
               }
          }
        evas_object_move(ext_mod->popup, cx, cy);
     }
}

static void
_select_all(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->selectall(data,obj,event_info);
   _ctxpopup_hide(obj);
}

static void
_select(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->select(data,obj,event_info);
   _ctxpopup_hide(obj);
}

static void
_paste(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->paste(data,obj,event_info);
   _ctxpopup_hide(obj);
}

static void
_cut(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->cut(data,obj,event_info);
   _ctxpopup_hide(obj);
   //elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->copy(data,obj,event_info);
   _ctxpopup_hide(obj);
   //elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   ext_mod->cancel(data,obj,event_info);
   _ctxpopup_hide(obj);
   //elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_search_menu(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   int ret;
   bundle *b = bundle_create();
   if (!b)
     {
        //printf("bundle_create() failed\n");
        return;
     }

   appsvc_set_operation(b, APPSVC_OPERATION_SEARCH);
   if (ext_mod->selmode)
     {
        const char *selection = elm_entry_selection_get(ext_mod->caller);
        if (selection)
          {
             char *str = _remove_tags(selection);
             if (str)
               {
                  appsvc_add_data(b, APPSVC_DATA_KEYWORD, str);
                  free(str);
               }
             else
               appsvc_add_data(b, APPSVC_DATA_KEYWORD, selection);
          }
     }
   appsvc_run_service(b, 0, NULL, NULL);
   bundle_free(b);
   _ctxpopup_hide(obj);
}

static void
_clipboard_menu(void *data, Evas_Object *obj, void *event_info)
{
   if(!ext_mod) return;

   // start for cbhm
#ifdef HAVE_ELEMENTARY_X
   ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), "",1);
#endif
   ext_mod->cnpinit(data,obj,event_info);
   if (ext_mod->cnp_mode != ELM_CNP_MODE_MARKUP)
     _cbhm_msg_send(obj, "show0");
   else
     _cbhm_msg_send(obj, "show1");
   _ctxpopup_hide(obj);
   // end for cbhm
}

static void
_item_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

   if (it->func) it->func(it->data, obj2, NULL);
   _ctxpopup_hide(obj);
}

static void
_ctxpopup_dismissed_cb(void *data, Evas_Object *o __UNUSED__, void *event_info __UNUSED__)
{
   if (!ext_mod) return;

   //elm_object_scroll_freeze_pop(ext_mod->popup);
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m)
{
   return 1; // succeed always
}

// module funcs for the specific module type
EAPI void
obj_hook(Evas_Object *obj)
{
   _mod_hook_count++;
   //if(_mod_hook_count > 1) return;

   if(!ext_mod)
     {
        ext_mod = ELM_NEW(Elm_Entry_Extension_data);
        elm_entry_extension_module_data_get(obj,ext_mod);
     }
}

EAPI void
obj_unhook(Evas_Object *obj)
{
   _mod_hook_count--;
   if(_mod_hook_count > 0) return;

   if(ext_mod)
     {
        free(ext_mod);
        ext_mod = NULL;
     }
}

EAPI void
obj_longpress(Evas_Object *obj)
{
   if(!ext_mod) return;

   Evas_Object *top;
   const Eina_List *l;
   const Elm_Entry_Context_Menu_Item *it;
   const char *context_menu_orientation;
   char buf[255];
   Evas_Object* icon;
   Elm_Object_Item *added_item = NULL;

   /*update*/
   elm_entry_extension_module_data_get(obj,ext_mod);
   if (ext_mod->context_menu)
     {
#ifdef HAVE_ELEMENTARY_X
        int cbhm_count = _cbhm_item_count_get(obj);
#endif
        if (ext_mod->popup) evas_object_del(ext_mod->popup);
        //else elm_widget_scroll_freeze_push(obj);
        top = elm_widget_top_get(obj);
        if(top)
          {
             ext_mod->popup = elm_ctxpopup_add(top);
             elm_object_tree_focus_allow_set(ext_mod->popup, EINA_FALSE);
             evas_object_smart_callback_add(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb, NULL);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _entry_del_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _entry_hide_cb, ext_mod->popup);
          }
        /*currently below theme not used,when guideline comes a new theme can be created if required*/
        //elm_object_style_set(ext_mod->popup,"extended/entry");
        context_menu_orientation = edje_object_data_get
           (ext_mod->ent, "context_menu_orientation");
        if ((context_menu_orientation) &&
            (!strcmp(context_menu_orientation, "horizontal")))
          elm_ctxpopup_horizontal_set(ext_mod->popup, EINA_TRUE);

        if (!ext_mod->selmode)
          {
             if (!ext_mod->password)
               {
                  if (!elm_entry_is_empty(obj))
                    {
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, S_SELECT, NULL, _select, obj );
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, S_SELECT_ALL, NULL, _select_all, obj );
                    }
               }

#ifdef HAVE_ELEMENTARY_X
             if (cbhm_count)
#else
             if (1) // need way to detect if someone has a selection
#endif
               {
                  if (ext_mod->editable)
                    added_item = elm_ctxpopup_item_append(ext_mod->popup, S_PASTE, NULL, _paste, obj );
               }
             //elm_ctxpopup_item_append(wd->ctxpopup, NULL, "Selectall",_select_all, obj );
             // start for cbhm
#ifdef HAVE_ELEMENTARY_X
             if ((!ext_mod->password) && (ext_mod->editable) && (cbhm_count))
#else
             if ((!ext_mod->password) && (ext_mod->editable))
#endif
               {
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, S_CLIPBOARD, NULL, _clipboard_menu, obj);  // Clipboard
                  //elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
               }
             // end for cbhm

             const char *entry_str;
             char *str;
             entry_str = edje_object_part_text_get(ext_mod->ent, "elm.text");
             str = _remove_tags(entry_str);
             if (strcmp(str, "") != 0)
               {
                  icon = elm_icon_add(ext_mod->popup);
                  snprintf(buf, sizeof(buf), "%s/images/copy&paste_icon_search.png", PACKAGE_DATA_DIR);
                  elm_icon_file_set(icon, buf, NULL);
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL, icon, _search_menu, obj);  // Search
               }
          }
        else
          {
             if (!ext_mod->password)
               {
                  if (ext_mod->have_selection)
                    {
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, S_COPY, NULL, _copy, obj );
                       if (ext_mod->editable)
                         added_item = elm_ctxpopup_item_append(ext_mod->popup, S_CUT, NULL, _cut, obj );
#ifdef HAVE_ELEMENTARY_X
                       if (ext_mod->editable && cbhm_count)
#else
                       if (ext_mod->editable)
#endif
                         added_item = elm_ctxpopup_item_append(ext_mod->popup, S_PASTE, NULL, _paste, obj );
                    }
                  else
                    {
                       _cancel(obj,ext_mod->popup,NULL);
                       if (!elm_entry_is_empty(obj))
                         {
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, S_SELECT, NULL, _select, obj );
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, S_SELECT_ALL, NULL, _select_all, obj );
                         }
#ifdef HAVE_ELEMENTARY_X
                       if (cbhm_count)
#else
                       if (1) // need way to detect if someone has a selection
#endif
                         {
                            if (ext_mod->editable)
                              added_item = elm_ctxpopup_item_append(ext_mod->popup, S_PASTE, NULL, _paste, obj );
                         }
                    }
                  // start for cbhm
#ifdef HAVE_ELEMENTARY_X
                  if (ext_mod->editable && cbhm_count)
#else
                  if (ext_mod->editable)
#endif
                    {
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, S_CLIPBOARD, NULL, _clipboard_menu, obj);  // Clipboard
                       //elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
                    }
                  // end for cbhm

                  const char *entry_str;
                  char *str;
                  entry_str = edje_object_part_text_get(ext_mod->ent, "elm.text");
                  str = _remove_tags(entry_str);
                  if (strcmp(str, "") != 0)
                    {
                       icon = elm_icon_add(ext_mod->popup);
                       snprintf(buf, sizeof(buf), "%s/images/copy&paste_icon_search.png", PACKAGE_DATA_DIR);
                       elm_icon_file_set(icon, buf, NULL);
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL, icon, _search_menu, obj);  // Search
                    }
              }
          }
        EINA_LIST_FOREACH(ext_mod->items, l, it)
          {
             added_item = elm_ctxpopup_item_append(ext_mod->popup, it->label, NULL, _item_clicked, it );
          }
        if (ext_mod->popup && added_item)
          {
             //elm_object_scroll_freeze_push(ext_mod->popup);
             _ctxpopup_position(obj);
             evas_object_show(ext_mod->popup);
             _ctxpopup_position(obj);
             ext_mod->caller = obj;
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _entry_move_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _entry_resize_cb, ext_mod->popup);
          }
        else
          ext_mod->caller = NULL;
     }
}

EAPI void
obj_mouseup(Evas_Object *obj)
{
   if (!obj || !ext_mod)
     return;
}


EAPI void
obj_hidemenu(Evas_Object *obj)
{
   if (!obj || !ext_mod || obj != ext_mod->caller)
     return;

   _ctxpopup_hide(ext_mod->popup);
   // if (ext_mod->popup) evas_object_del(ext_mod->popup);
}
