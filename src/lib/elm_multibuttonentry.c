#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Multibuttonentry Multibuttonentry
 * @ingroup Elementary
 *
 * This is a Multibuttonentry.
 */

#define MAX_STR 256
#define MIN_W_ENTRY 20

typedef enum _Multibuttonentry_Pos
{
   MULTIBUTONENTRY_POS_START,
   MULTIBUTONENTRY_POS_END,
   MULTIBUTONENTRY_POS_BEFORE,
   MULTIBUTONENTRY_POS_AFTER,
   MULTIBUTONENTRY_POS_NUM
} Multibuttonentry_Pos;

typedef enum _Multibuttonentry_Button_State
{
   MULTIBUTONENTRY_BUTTON_STATE_DEFAULT,
   MULTIBUTONENTRY_BUTTON_STATE_SELECTED,
   MULTIBUTONENTRY_BUTTON_STATE_NUM
} Multibuttonentry_Button_State;

typedef enum _MultiButtonEntry_Closed_Button_Type {
   MULTIBUTTONENTRY_CLOSED_IMAGE,
   MULTIBUTTONENTRY_CLOSED_LABEL
} MultiButtonEntry_Closed_Button_Type;

typedef enum _Multibuttonentry_View_State
{
   MULTIBUTTONENTRY_VIEW_NONE,
   MULTIBUTTONENTRY_VIEW_GUIDETEXT,
   MULTIBUTTONENTRY_VIEW_ENTRY,
   MULTIBUTTONENTRY_VIEW_CONTRACTED
} Multibuttonentry_View_State;

struct _Multibuttonentry_Item {
   Evas_Object *multibuttonentry;
   Evas_Object *button;
   void *data;
   Evas_Coord vw, rw; // vw: visual width, real width
   Eina_Bool  visible: 1;
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
   Evas_Object *base;
   Evas_Object *box;
   Evas_Object *entry;
   Evas_Object *label;
   Evas_Object *guidetext;
   Evas_Object *end;   // used to represent the total number of invisible buttons

   Evas_Object *rectForEnd;
   MultiButtonEntry_Closed_Button_Type end_type;

   Eina_List *items;
   Eina_List *current;
   int n_str;
   Multibuttonentry_View_State view_state;

   Evas_Coord w_box, h_box;
   int  contracted;
   Eina_Bool  focused: 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _on_focus_hook(void *data __UNUSED__, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hint_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event);
static void   _event_init(Evas_Object *obj);
static void _contracted_state_set(Evas_Object *obj, int contracted);
static void _view_update(Evas_Object *obj);
static void   _set_label(Evas_Object *obj, const char* str);
static void _change_current_button_state(Evas_Object *obj, Multibuttonentry_Button_State state);
static void   _change_current_button(Evas_Object *obj, Evas_Object *btn);
static void   _button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void   _del_button_obj(Evas_Object *obj, Evas_Object *btn);
static void   _del_button_item(Elm_Multibuttonentry_Item *item);
static void   _del_button(Evas_Object *obj);
static Elm_Multibuttonentry_Item* _add_button_item(Evas_Object *obj, const char *str, Multibuttonentry_Pos pos, const Elm_Multibuttonentry_Item *reference, void *data);
static void   _add_button(Evas_Object *obj, char *str);
static void   _evas_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info );
static void   _view_init(Evas_Object *obj);
static void _set_vis_guidetext(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->items)
     {
        Elm_Multibuttonentry_Item *item;
        EINA_LIST_FREE(wd->items, item)
          {
             _del_button_obj(obj, item->button);
             free(item);
          }
        wd->items = NULL;
     }
   wd->current = NULL;

   if (wd->entry) evas_object_del (wd->entry);
   if (wd->label) evas_object_del (wd->label);
   if (wd->guidetext) evas_object_del (wd->guidetext);
   if (wd->end) evas_object_del (wd->end);
   if (wd->rectForEnd) evas_object_del(wd->rectForEnd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Elm_Multibuttonentry_Item *item;
   if (!wd) return;

   _elm_theme_object_set(obj, wd->base, "multibuttonentry", "base", elm_widget_style_get(obj));
   if (wd->box)   edje_object_part_swallow (wd->base, "box.swallow", wd->box);
   edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);

   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->button)
          _elm_theme_object_set(obj, item->button, "multibuttonentry", "btn", elm_widget_style_get (obj));
        edje_object_scale_set(item->button, elm_widget_scale_get(obj) * _elm_config->scale);
     }

   _sizing_eval(obj);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (!elm_widget_focus_get(obj) && !(elm_widget_disabled_get(obj)))
     {
        wd->focused = EINA_FALSE;
        _view_update(obj);
        evas_object_smart_callback_call(obj, "unfocused", NULL);
        if (wd->n_str)
          {
             elm_scrolled_entry_entry_set(wd->entry, "");
             wd->n_str = 0;
          }
     }
}

static void
_signal_emit_hook(Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   edje_object_signal_emit(wd->base, emission, source);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord left, right, top, bottom;
   if (!wd) return;

   evas_object_size_hint_min_get(wd->box, &minw, &minh);
   edje_object_part_geometry_get(wd->base, "top.left.pad", NULL, NULL, &left, &top);
   edje_object_part_geometry_get(wd->base, "bottom.right.pad", NULL, NULL, &right, &bottom);

   minw += (left + right);
   minh += (top + bottom);

   evas_object_size_hint_min_set(obj, minw, minh);
}

static void
_signal_mouse_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if(!wd || !wd->base) return;

   wd->focused = EINA_TRUE;
   _view_update(data);
   evas_object_smart_callback_call(data, "clicked", NULL);
}

static void
_changed_size_hint_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *eo = (Evas_Object *)data;
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   _sizing_eval(eo);
}

static void
_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord w, h;
   if (!wd) return;

   evas_object_geometry_get(wd->box, NULL, NULL, &w, &h);

   if (wd->h_box < h) evas_object_smart_callback_call (data, "expanded", NULL);
   else if (wd->h_box > h) evas_object_smart_callback_call (data, "contracted", NULL);
   else ;

   wd->w_box = w;
   wd->h_box = h;

   _view_update(data);
}

static void
_event_init(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->base)   return;

   if (wd->base)
     {
        edje_object_signal_callback_add(wd->base, "mouse,clicked,1", "*", _signal_mouse_clicked, obj);
        edje_object_signal_callback_add(wd->base, "clicked", "*", _signal_mouse_clicked, obj);
     }

   if (wd->box)
     {
        evas_object_event_callback_add(wd->box, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
        evas_object_event_callback_add(wd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hint_cb, obj);
     }
}

static void
_set_vis_guidetext(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   elm_box_unpack(wd->box, wd->guidetext);
   elm_box_unpack(wd->box, wd->entry);
   if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED) return;

   if (wd && !eina_list_count(wd->items) && wd->guidetext
       && !elm_widget_focus_get(obj) && !wd->focused)
     {
        evas_object_hide(wd->entry);
        elm_box_pack_end(wd->box, wd->guidetext);
        evas_object_show(wd->guidetext);
        wd->view_state = MULTIBUTTONENTRY_VIEW_GUIDETEXT;
     }
   else
     {
        evas_object_hide(wd->guidetext);
        elm_box_pack_end(wd->box, wd->entry);
        evas_object_show(wd->entry);
        if (elm_widget_focus_get(obj) || wd->focused)
          elm_object_focus(wd->entry);
        wd->view_state = MULTIBUTTONENTRY_VIEW_ENTRY;
     }
   return;
}

static void
_contracted_state_set(Evas_Object *obj, int contracted)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Elm_Multibuttonentry_Item *item;
   if (!wd || !wd->box) return;

   elm_scrolled_entry_entry_set(wd->entry, "");

   if (wd->view_state == MULTIBUTTONENTRY_VIEW_ENTRY)
     evas_object_hide(wd->entry);
   else if (wd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
     evas_object_hide(wd->guidetext);
   else if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
     {
        evas_object_hide(wd->rectForEnd);
        evas_object_hide(wd->end);
        wd->view_state = MULTIBUTTONENTRY_VIEW_NONE;
     }

   if (contracted == 1)
     {
        Evas_Coord w=0, w_tmp=0;
        Evas_Coord box_inner_item_width_padding = 0;

        elm_box_padding_get(wd->box, &box_inner_item_width_padding, NULL);
        // unpack all items and entry
        elm_box_unpack_all(wd->box);
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item)
               {
                  evas_object_hide(item->button);
                  item->visible = EINA_FALSE;
               }
          }

        // pack buttons only 1line
        w = wd->w_box;

        if (wd->label)
          {
             elm_box_pack_end(wd->box, wd->label);
             evas_object_size_hint_min_get(wd->label, &w_tmp, NULL);
             w -= w_tmp;
             w -= box_inner_item_width_padding;
          }

        item = NULL;
        int count = eina_list_count(wd->items);
        Evas_Coord button_min_width = 0;
        /* Evas_Coord button_min_height = 0; */
        if (wd->end_type == MULTIBUTTONENTRY_CLOSED_IMAGE)
          {
             const char *size_str;
             size_str = edje_object_data_get(wd->end, "closed_button_width");
             if(size_str) button_min_width = (Evas_Coord)atoi(size_str);
             /* it use for later
             size_str = edje_object_data_get(wd->end, "closed_button_height");
             if(size_str) button_min_width = (Evas_Coord)atoi(size_str);
              */
          }

        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item)
               {
                  int w_label_count = 0;
                  char buf[MAX_STR] = {0,};

                  elm_box_pack_end(wd->box, item->button);
                  evas_object_show(item->button);
                  item->visible = EINA_TRUE;

                  w -= item->vw;
                  w -= box_inner_item_width_padding;
                  count--;

                  if (wd->end_type == MULTIBUTTONENTRY_CLOSED_LABEL)
                    {
                       if (count > 0)
                         {
                            snprintf(buf, sizeof(buf), "... + %d", count);
                            elm_label_label_set(wd->end, buf);
                            evas_object_size_hint_min_get(wd->end, &w_label_count, NULL);
                         }

                       if (w < 0 || w < w_label_count)
                         {
                            elm_box_unpack(wd->box, item->button);
                            evas_object_hide(item->button);
                            item->visible = EINA_FALSE;

                            count++;
                            snprintf(buf, sizeof(buf), "... + %d", count);
                            elm_label_label_set(wd->end, buf);
                            evas_object_size_hint_min_get(wd->end, &w_label_count, NULL);

                            elm_box_pack_end(wd->box, wd->end);
                            evas_object_show(wd->end);

                            wd->view_state = MULTIBUTTONENTRY_VIEW_CONTRACTED;
                            evas_object_smart_callback_call(obj, "contracted,state,changed", (void *)1);
                            break;
                         }
                    }
                  else if (wd->end_type == MULTIBUTTONENTRY_CLOSED_IMAGE)
                    {
                       if (w < button_min_width)
                         {
                            Evas_Coord rectSize;
                            Evas_Coord closed_height = 0;
                            const char *height_str = edje_object_data_get(wd->base, "closed_height");
                            if(height_str) closed_height = (Evas_Coord)atoi(height_str);
                            elm_box_unpack(wd->box, item->button);
                            evas_object_hide(item->button);
                            item->visible = EINA_FALSE;

                            w += item->vw;
                            rectSize = w - button_min_width;
                            if (!wd->rectForEnd)
                              {
                                 Evas *e = evas_object_evas_get(obj);
                                 wd->rectForEnd = evas_object_rectangle_add(e);
                                 evas_object_color_set(wd->rectForEnd, 0, 0, 0, 0);
                              }
                            evas_object_size_hint_min_set(wd->rectForEnd, rectSize, closed_height);
                            elm_box_pack_end(wd->box, wd->rectForEnd);
                            evas_object_show(wd->rectForEnd);

                            elm_box_pack_end(wd->box, wd->end);
                            evas_object_show(wd->end);

                            wd->view_state = MULTIBUTTONENTRY_VIEW_CONTRACTED;
                            evas_object_smart_callback_call(obj, "contracted,state,changed", (void *)0);
                            break;
                         }
                    }
               }
          }
     }
   else
     {
        // unpack all items and entry
        elm_box_unpack_all(wd->box);
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item)
               {
                  evas_object_hide(item->button);
                  item->visible = EINA_FALSE;
               }
          }
        evas_object_hide(wd->end);

        if (wd->rectForEnd) evas_object_hide(wd->rectForEnd);

        // pack buttons only 1line

        if (wd->label) elm_box_pack_end(wd->box, wd->label);

        // pack remain btns
        item = NULL;
        EINA_LIST_FOREACH(wd->items, l, item)
          {
             if (item)
               {
                  elm_box_pack_end(wd->box, item->button);
                  evas_object_show(item->button);
                  item->visible = EINA_TRUE;
               }
          }

        wd->view_state = MULTIBUTTONENTRY_VIEW_NONE;
        evas_object_smart_callback_call(obj, "contracted,state,changed", (void *)wd->contracted);
     }
   if (wd->view_state != MULTIBUTTONENTRY_VIEW_CONTRACTED)
     {
        _set_vis_guidetext(obj);
     }
}

static void
_view_update(Evas_Object *obj)
{
   Evas_Coord width = 1, height = 1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->box || !wd->entry || !(wd->w_box > 0)) return;

   // update label
   if (wd->label)
     {
        elm_box_unpack(wd->box, wd->label);
        elm_box_pack_start(wd->box, wd->label);
        evas_object_size_hint_min_get(wd->label, &width, &height);
     }

   if (wd->guidetext)
     {
        Evas_Coord guide_text_width = wd->w_box - width;
        evas_object_size_hint_min_set(wd->guidetext, guide_text_width, height);
     }

   // update buttons in contracted mode
   if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
     _contracted_state_set(obj, 1);

   // update guidetext
   _set_vis_guidetext(obj);
}

static void
_set_label(Evas_Object *obj, const char* str)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !str) return;

   if (wd->label)
   {
      Evas_Coord width, height, sum_width = 0;
      evas_object_size_hint_min_set(wd->label, 0, 0);
      evas_object_resize(wd->label, 0, 0);
      edje_object_part_text_set(wd->label, "mbe.label", str);

      if (!strcmp(str, ""))
        {
           /* FIXME: not work yet */
           edje_object_signal_emit(wd->label, "elm,mbe,clear_text", "");
           edje_object_part_geometry_get(wd->label, "mbe.label", NULL, NULL, &width, &height);
           sum_width += width;
        }
      else
        {
           edje_object_signal_emit(wd->label, "elm,mbe,set_text", "");
           edje_object_part_geometry_get(wd->label, "mbe.label", NULL, NULL, &width, &height);
           sum_width += width;

           edje_object_part_geometry_get(wd->label, "mbe.label.left.padding", NULL, NULL, &width, NULL);
           sum_width += width;

           edje_object_part_geometry_get(wd->label, "mbe.label.right.padding", NULL, NULL, &width, NULL);
           sum_width += width;
        }
      evas_object_size_hint_min_set(wd->label, sum_width, height);
   }
   evas_object_show(wd->label);
   _view_update(obj);
}

static void
_set_guidetext(Evas_Object *obj, const char* str)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !str)   return;

   if (!wd->guidetext)
     {
        if (! (wd->guidetext = edje_object_add (evas_object_evas_get (obj)))) return;
        _elm_theme_object_set(obj, wd->guidetext, "multibuttonentry", "guidetext", elm_widget_style_get(obj));
        evas_object_size_hint_weight_set(wd->guidetext, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(wd->guidetext, EVAS_HINT_FILL, EVAS_HINT_FILL);
     }

   if (wd->guidetext) edje_object_part_text_set (wd->guidetext, "elm.text", str);
   _view_update(obj);
}

static void
_change_current_button_state(Evas_Object *obj, Multibuttonentry_Button_State state)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item = NULL;
   if (!wd) return;

   if (wd->current)
     item = eina_list_data_get(wd->current);

   if (item && item->button)
     {
        switch(state)
          {
             case MULTIBUTONENTRY_BUTTON_STATE_DEFAULT:
                edje_object_signal_emit(item->button, "default", "");
                wd->current = NULL;
                break;
             case MULTIBUTONENTRY_BUTTON_STATE_SELECTED:
                edje_object_signal_emit(item->button, "focused", "");
                evas_object_smart_callback_call(obj, "item,selected", item);
                break;
             default:
                edje_object_signal_emit(item->button, "default", "");
                wd->current = NULL;
                break;
          }
     }
}

static void
_change_current_button(Evas_Object *obj, Evas_Object *btn)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Elm_Multibuttonentry_Item *item;
   if (!wd) return;

   // change the state of previous button to "default"
   _change_current_button_state(obj, MULTIBUTONENTRY_BUTTON_STATE_DEFAULT);

   // change the current
   EINA_LIST_FOREACH(wd->items, l, item)
     {
        if (item->button == btn)
          {
             wd->current = l;
             break;
          }
     }

   // chagne the state of current button to "focused"
   _change_current_button_state(obj, MULTIBUTONENTRY_BUTTON_STATE_SELECTED);

}

static void
_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   static char str[MAX_STR];
   Elm_Multibuttonentry_Item *item = NULL;
   if (!wd || wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED) return;

   strncpy(str, elm_scrolled_entry_entry_get(wd->entry), MAX_STR);
   str[MAX_STR - 1]= 0;

   if (strlen (str))
      _add_button(data, str);

   _change_current_button(data, obj);

   if (wd->current)
      if((item = eina_list_data_get(wd->current)) != NULL)
         evas_object_smart_callback_call(data, "item,clicked", item);
}

static void
_del_button_obj(Evas_Object *obj, Evas_Object *btn)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !btn)   return;

   if (btn)
     // del button
     evas_object_del(btn);
}

static void
_del_button_item(Elm_Multibuttonentry_Item *item)
{
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item) return;
   Widget_Data *wd;

   Evas_Object *obj = item->multibuttonentry;
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   EINA_LIST_FOREACH(wd->items, l, _item)
     {
        if (_item == item)
          {
             wd->items = eina_list_remove(wd->items, _item);
             elm_box_unpack(wd->box, _item->button);

             evas_object_smart_callback_call(obj, "item,deleted", _item);

             _del_button_obj(obj, _item->button);

             free(_item);
             if (wd->current == l)
               wd->current = NULL;
             break;
          }
     }
   if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
     _contracted_state_set(obj, 1);

   evas_object_smart_callback_call(obj, "item,added", item);
}

static void
_del_button(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item;
   if (!wd || !wd->items) return;

   if (!wd->current)
     {
        // let the last button focus
        item = eina_list_data_get(eina_list_last(wd->items));
        if (item->button)   _change_current_button (obj, item->button);
     }
   else
     {
        item = eina_list_data_get(wd->current);
        if (item)
          _del_button_item(item);
     }
}

static void
_resize_button(Evas_Object *btn, Evas_Coord *realw, Evas_Coord *vieww)
{
   Evas_Coord rw, vw;
   Evas_Coord w_text, h_btn, padding_outer, padding_inner;
   Evas_Coord w_btn = 0, button_max_width = 0;
   const char *size_str;

   size_str = edje_object_data_get(btn, "button_max_size");
   if (size_str) button_max_width = (Evas_Coord)atoi(size_str);

   // decide the size of button
   edje_object_part_geometry_get(btn, "elm.base", NULL, NULL, NULL, &h_btn);
   edje_object_part_geometry_get(btn, "elm.btn.text", NULL, NULL, &w_text, NULL);
   edje_object_part_geometry_get(btn, "left.padding", NULL, NULL, &padding_outer, NULL);
   edje_object_part_geometry_get(btn, "left.inner.padding", NULL, NULL, &padding_inner, NULL);
   w_btn = w_text + 2*padding_outer + 2*padding_inner;

   rw = w_btn;
   vw = (button_max_width < w_btn) ? button_max_width : w_btn;

   //resize btn
   evas_object_resize(btn, vw, h_btn);
   evas_object_size_hint_min_set(btn, vw, h_btn);

   if(realw) *realw = rw;
   if(vieww) *vieww = vw;
}

static Elm_Multibuttonentry_Item*
_add_button_item(Evas_Object *obj, const char *str, Multibuttonentry_Pos pos, const Elm_Multibuttonentry_Item *reference, void *data)
{
   Elm_Multibuttonentry_Item *item;
   Evas_Object *btn;
   Evas_Coord width, height;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->box || !wd->entry) return NULL;

   // add button
   btn = edje_object_add(evas_object_evas_get(obj));

   _elm_theme_object_set(obj, btn, "multibuttonentry", "btn", elm_widget_style_get(obj));
   edje_object_part_text_set(btn, "elm.btn.text", str);
   edje_object_part_geometry_get(btn, "elm.btn.text", NULL, NULL, &width, &height);
   evas_object_size_hint_min_set(btn, width, height);
   edje_object_signal_callback_add(btn, "mouse,clicked,1", "*", _button_clicked, obj);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   // append item list
   item = ELM_NEW(Elm_Multibuttonentry_Item);
   if (item)
     {
        Evas_Coord rw, vw;
        _resize_button(btn, &rw, &vw);
        item->multibuttonentry = obj;
        item->button = btn;
        item->data = data;
        item->rw = rw;
        item->vw = vw;
        item->visible = EINA_TRUE;

        switch(pos)
          {
             case MULTIBUTONENTRY_POS_START:
                wd->items = eina_list_prepend(wd->items, item);
                if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
                  {
                     elm_widget_sub_object_add(obj, btn);
                     _contracted_state_set(obj, 1);
                  }
                else
                  {
                     if (wd->label)
                       elm_box_pack_after(wd->box, btn, wd->label);
                     else
                       elm_box_pack_start(wd->box, btn);
                     if (wd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                       _set_vis_guidetext(obj);
                  }
                break;
             case MULTIBUTONENTRY_POS_END:
                wd->items = eina_list_append(wd->items, item);
                if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
                  {
                     elm_widget_sub_object_add(obj, btn);
                     evas_object_hide(btn);
                  }
                else
                  {
                     if (wd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                       _set_vis_guidetext(obj);
                     if (wd->entry)
                       elm_box_pack_before(wd->box, btn, wd->entry);
                     else
                       elm_box_pack_end(wd->box, btn);
                  }
                break;
             case MULTIBUTONENTRY_POS_BEFORE:
                if (reference)
                     wd->items = eina_list_prepend_relative(wd->items, item, reference);
                else
                     wd->items = eina_list_append(wd->items, item);
                if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
                  {
                     elm_widget_sub_object_add(obj, btn);
                     evas_object_hide(btn);
                     _contracted_state_set(obj, 1);
                  }
                else
                  {
                     if (reference)
                       elm_box_pack_before(wd->box, btn, reference->button);
                     else
                       {
                          if (wd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                            _set_vis_guidetext(obj);
                          if (wd->entry)
                            elm_box_pack_before(wd->box, btn, wd->entry);
                          else
                            elm_box_pack_end(wd->box, btn);
                       }
                  }
                break;
             case MULTIBUTONENTRY_POS_AFTER:
                if (reference)
                     wd->items = eina_list_append_relative(wd->items, item, reference);
                else
                     wd->items = eina_list_append(wd->items, item);
                if (wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED)
                  {
                     elm_widget_sub_object_add(obj, btn);
                     _contracted_state_set(obj, 1);
                  }
                else
                  {
                     if (reference)
                       elm_box_pack_after(wd->box, btn, reference->button);
                     else
                       {
                          if (wd->view_state == MULTIBUTTONENTRY_VIEW_GUIDETEXT)
                            _set_vis_guidetext(obj);
                          if (wd->entry)
                            elm_box_pack_before(wd->box, btn, wd->entry);
                          else
                            elm_box_pack_end(wd->box, btn);
                       }
                  }
                break;
             default:
                break;
          }
     }
   evas_object_smart_callback_call(obj, "item,added", item);

   return item;
}

static void
_add_button(Evas_Object *obj, char *str)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   //remove entry text
   elm_scrolled_entry_entry_set(wd->entry, "");

   // add button
   _add_button_item(obj, str, MULTIBUTONENTRY_POS_END, NULL, NULL);
}

static void
_evas_key_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info )
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;
   static char str[MAX_STR];
   if (!wd || !wd->base || !wd->box) return;

   strncpy(str, elm_scrolled_entry_entry_get(wd->entry), MAX_STR);
   str[MAX_STR - 1]= 0;

   if ( (wd->n_str == 0) &&  (strcmp (str, "") == 0) &&  ( (strcmp (ev->keyname, "BackSpace") == 0)|| (strcmp (ev->keyname, "BackSpace (") == 0)))
     {
        _del_button(data);
     }
   else if ( (strcmp (str, "") != 0) &&  (strcmp (ev->keyname, "KP_Enter") == 0 ||strcmp (ev->keyname, "Return") == 0 ))
     {
        _add_button(data, str);
        wd->n_str = 0;
        return;
     }
   else
     {
        //
     }

   wd->n_str = strlen(str);
}

static void
_view_init(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (!wd->box)
     {
        if (! (wd->box = elm_box_add (obj))) return;
        elm_widget_sub_object_add(obj, wd->box);
        elm_box_extended_mode_set(wd->box, EINA_TRUE);
        elm_box_homogenous_set(wd->box, EINA_FALSE);
        edje_object_part_swallow(wd->base, "box.swallow", wd->box);
     }

   if (!wd->label)
     {
        if (!(wd->label = edje_object_add(evas_object_evas_get(obj)))) return;
        _elm_theme_object_set(obj, wd->label, "multibuttonentry", "label", elm_widget_style_get(obj));
        _set_label(obj, "");
        elm_widget_sub_object_add(obj, wd->label);
     }

   if (!wd->entry)
     {
        if (! (wd->entry = elm_scrolled_entry_add (obj))) return;
        elm_scrolled_entry_single_line_set(wd->entry, EINA_TRUE);
        elm_scrolled_entry_entry_set(wd->entry, "");
        elm_scrolled_entry_cursor_end_set(wd->entry);
        evas_object_size_hint_min_set(wd->entry, MIN_W_ENTRY, 0);
        evas_object_event_callback_add(wd->entry, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, obj);
        evas_object_size_hint_weight_set(wd->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(wd->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
        if (wd->box)   elm_box_pack_end (wd->box, wd->entry);
        evas_object_show(wd->entry);
        wd->view_state = MULTIBUTTONENTRY_VIEW_ENTRY;
     }

   if (!wd->end)
     {
        const char *end_type = edje_object_data_get(wd->base, "closed_button_type");
        if (!end_type || !strcmp(end_type, "label"))
          {
             if (! (wd->end = elm_label_add (obj))) return;
             elm_object_style_set(wd->end, "extended/multibuttonentry_default");
             wd->end_type = MULTIBUTTONENTRY_CLOSED_LABEL;
          }
        else
          {
             const char *size_str;
             if (!(wd->end = edje_object_add(evas_object_evas_get(obj)))) return;
             _elm_theme_object_set(obj, wd->end, "multibuttonentry", "closedbutton", elm_widget_style_get(obj));
             Evas_Coord button_min_width = 0;
             Evas_Coord button_min_height = 0;

             size_str = edje_object_data_get(wd->end, "closed_button_width");
             if(size_str) button_min_width = (Evas_Coord)atoi(size_str);
             size_str = edje_object_data_get(wd->end, "closed_button_height");
             if(size_str) button_min_height = (Evas_Coord)atoi(size_str);

             wd->end_type = MULTIBUTTONENTRY_CLOSED_IMAGE;
             evas_object_size_hint_min_set(wd->end, button_min_width, button_min_height);
             elm_widget_sub_object_add(obj, wd->end);
          }
     }
}

/**
 * Add a new multibuttonentry to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Multibuttonentry
 */
EAPI Evas_Object *
elm_multibuttonentry_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   if (!parent) return NULL;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "multibuttonentry");
   elm_widget_type_set(obj, "multibuttonentry");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);

   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_signal_emit_hook_set(obj, _signal_emit_hook);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "multibuttonentry", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   wd->view_state = MULTIBUTTONENTRY_VIEW_NONE;
   wd->focused = EINA_FALSE;
   wd->n_str = 0;
   wd->rectForEnd = NULL;

   _view_init(obj);
   _event_init(obj);

   return obj;
}

/**
 * Get the entry of the multibuttonentry object
 *
 * @param obj The multibuttonentry object
 * @return The entry object, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Evas_Object *
elm_multibuttonentry_entry_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)   return NULL;

   return wd->entry;
}

/**
 * Get the label
 *
 * @param obj The multibuttonentry object
 * @return The label, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const char *
elm_multibuttonentry_label_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (wd->label) return edje_object_part_text_get(wd->guidetext, "elm.text");
   return NULL;
}

/**
 * Set the label
 *
 * @param obj The multibuttonentry object
 * @param label The text label string
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_label_set(Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (label)
     _set_label(obj, label);
   else
     _set_label(obj, "");
}

/**
 * Get the guide text
 *
 * @param obj The multibuttonentry object
 * @return The guide text, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const char *
elm_multibuttonentry_guide_text_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd)   return NULL;
   if (wd->guidetext) return edje_object_part_text_get(wd->guidetext, "elm.text");
   return NULL;
}

/**
 * Set the guide text
 *
 * @param obj The multibuttonentry object
 * @param label The guide text string
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_guide_text_set(Evas_Object *obj, const char *guidetext)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (guidetext)
      _set_guidetext(obj, guidetext);
   else
      _set_guidetext(obj, "");

}

/**
 * Get the value of contracted state.
 *
 * @param obj The multibuttonentry object
 * @param the value of contracted state.
 *
 * @ingroup Multibuttonentry
 */
EAPI int
elm_multibuttonentry_contracted_state_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return -1;
   return wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED ? 1 : 0;
}

/**
 * Set/Unset the multibuttonentry to contracted state of single line
 *
 * @param obj The multibuttonentry object
 * @param the value of contracted state. set this to 1 to set the multibuttonentry to contracted state of single line. set this to 0 to unset the contracted state.
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_contracted_state_set(Evas_Object *obj, int contracted)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->box ||
       ((wd->view_state == MULTIBUTTONENTRY_VIEW_CONTRACTED) ? 1 : 0) == contracted) return;
   _contracted_state_set(obj, contracted);
}

/**
 * Prepend a new item to the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_start(Evas_Object *obj, const char *label, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item;
   if (!wd || !label) return NULL;
   item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_START, NULL, data);
   return item;
}

/**
 * Append a new item to the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_end(Evas_Object *obj, const char *label, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item;
   if (!wd || !label) return NULL;
   item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_END, NULL, data);
   return item;
}

/**
 * Add a new item to the multibuttonentry before the indicated object
 *
 * reference.
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param before The item before which to add it
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_before(Evas_Object *obj, const char *label, Elm_Multibuttonentry_Item *before, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item;
   if (!wd || !label) return NULL;
   item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_BEFORE, before, data);
   return item;
}

/**
 * Add a new item to the multibuttonentry after the indicated object
 *
 * @param obj The multibuttonentry object
 * @param label The label of new item
 * @param after The item after which to add it
 * @param data The ponter to the data to be attached
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_add_after(Evas_Object *obj, const char *label, Elm_Multibuttonentry_Item *after, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Multibuttonentry_Item *item;
   if (!wd || !label) return NULL;
   item = _add_button_item(obj, label, MULTIBUTONENTRY_POS_AFTER, after, data);
   return item;
}

/**
 * Get a list of items in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The list of items, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const Eina_List *
elm_multibuttonentry_items_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->items;
}

/**
 * Get the first item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The first item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_first_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(wd->items);
}

/**
 * Get the last item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The last item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_last_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->items) return NULL;
   return eina_list_data_get(eina_list_last(wd->items));
}

/**
 * Get the selected item in the multibuttonentry
 *
 * @param obj The multibuttonentry object
 * @return The selected item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_selected_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd || !wd->current) return NULL;
   return eina_list_data_get(wd->current);
}

/**
 * Set the selected state of an item
 *
 * @param item The item
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_selected_set(Elm_Multibuttonentry_Item *item)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item) return;
   ELM_CHECK_WIDTYPE(item->multibuttonentry, widtype);
   wd = elm_widget_data_get(item->multibuttonentry);
   if (!wd) return;

   EINA_LIST_FOREACH(wd->items, l, _item)
     {
        if (_item == item)
          {
             _change_current_button(item->multibuttonentry, item->button);
          }
     }
}

/**
 * unselect all of items.
 *
 * @param obj The multibuttonentry object
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_unselect_all(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _change_current_button_state(obj, MULTIBUTONENTRY_BUTTON_STATE_DEFAULT);
}

/**
 * Remove all items in the multibuttonentry.
 *
 * @param obj The multibuttonentry object
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_items_del(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->items)
     {
        Elm_Multibuttonentry_Item *item;
        EINA_LIST_FREE(wd->items, item)
          {
             elm_box_unpack(wd->box, item->button);
             _del_button_obj(obj, item->button);
             free(item);
          }
        wd->items = NULL;
     }
   wd->current = NULL;
   _view_update(obj);
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_del(Elm_Multibuttonentry_Item *item)
{
   if (!item) return;
   _del_button_item(item);
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Multibuttonentry
 */
EAPI const char *
elm_multibuttonentry_item_label_get(Elm_Multibuttonentry_Item *item)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item) return NULL;
   ELM_CHECK_WIDTYPE(item->multibuttonentry, widtype) NULL;
   wd = elm_widget_data_get(item->multibuttonentry);
   if (!wd || !wd->items) return NULL;

   EINA_LIST_FOREACH(wd->items, l, _item)
     {
        if (_item == item)
          return edje_object_part_text_get(_item->button, "elm.btn.text");
     }

   return NULL;
}

/**
 * Set the label of a given item
 *
 * @param item The item
 * @param label The text label string
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_label_set(Elm_Multibuttonentry_Item *item, const char *str)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item || !str) return;
   ELM_CHECK_WIDTYPE(item->multibuttonentry, widtype);
   wd = elm_widget_data_get(item->multibuttonentry);
   if (!wd || !wd->items) return;

   EINA_LIST_FOREACH(wd->items, l, _item)
      if (_item == item)
        {
           edje_object_part_text_set(_item->button, "elm.btn.text", str);
           _resize_button(_item->button, &_item->rw, &_item->vw);
           break;
        }
}

/**
 * Get the previous item in the multibuttonentry
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_prev(Elm_Multibuttonentry_Item *item)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item) return NULL;
   ELM_CHECK_WIDTYPE(item->multibuttonentry, widtype) NULL;
   wd = elm_widget_data_get(item->multibuttonentry);
   if (!wd || !wd->items) return NULL;

   EINA_LIST_FOREACH(wd->items, l, _item)
      if (_item == item)
        {
           l = eina_list_prev(l);
           if (!l) return NULL;
           return eina_list_data_get(l);
        }
   return NULL;
}

/**
 * Get the next item in the multibuttonentry
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Multibuttonentry
 */
EAPI Elm_Multibuttonentry_Item *
elm_multibuttonentry_item_next(Elm_Multibuttonentry_Item *item)
{
   Widget_Data *wd;
   Eina_List *l;
   Elm_Multibuttonentry_Item *_item;
   if (!item) return NULL;
   ELM_CHECK_WIDTYPE(item->multibuttonentry, widtype) NULL;
   wd = elm_widget_data_get(item->multibuttonentry);
   if (!wd || !wd->items) return NULL;

   EINA_LIST_FOREACH(wd->items, l, _item)
      if (_item == item)
        {
           l = eina_list_next(l);
           if (!l) return NULL;
           return eina_list_data_get(l);
        }
   return NULL;
}

/**
 * Get private data of item
 *
 * @param item The item
 * @return The data pointer stored, or NULL if none was stored
 *
 * @ingroup Multibuttonentry
 */
EAPI void *
elm_multibuttonentry_item_data_get(Elm_Multibuttonentry_Item *item)
{
   if (!item) return NULL;
   return item->data;
}

/**
 * Set private data of item
 *
 * @param item The item
 * @param data The ponter to the data to be attached
 *
 * @ingroup Multibuttonentry
 */
EAPI void
elm_multibuttonentry_item_data_set(Elm_Multibuttonentry_Item *item, void *data)
{
   if (!item) return;
   item->data = data;
}

