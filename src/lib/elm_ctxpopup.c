#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Ctxpopup Ctxpopup
 * @ingroup Elementary
 *
 * Contextual popup.
 *
 * Signals that you can add callbacks for are:
 *
 * hide - This is called whenever the ctxpopup is hided.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Ctxpopup_Item
{
   Evas_Object *ctxpopup;
   Evas_Object *base;
   const char *label;
   Evas_Object *icon;
   void (*func) (void *data, Evas_Object * obj, void *event_info);
   void *data;
   Eina_Bool disabled:1;
   Eina_Bool separator:1;
};

struct _Widget_Data
{
   Evas_Object *parent;
   Evas_Object *base;
   Evas_Object *content;
   Evas_Object *box;
   Evas_Object *arrow;
   Evas_Object *scroller;
   Evas_Object *bg;
   Evas_Object *btn_layout;
   Eina_List *items;
   Elm_Ctxpopup_Arrow arrow_dir;
   int btn_cnt;
   Elm_Transit *transit;
   Elm_Ctxpopup_Arrow arrow_priority[4];
   Eina_Bool scroller_disabled:1;
   Eina_Bool horizontal:1;
   Eina_Bool visible:1;
   Eina_Bool screen_dimmed_disabled:1;
   Eina_Bool position_forced: 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _bg_clicked_cb(void *data, Evas_Object *obj, const char *emission,
			   const char *source);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj,
			   void *event_info);
static void _ctxpopup_show(void *data, Evas *e, Evas_Object *obj,
			   void *event_info);
static void _ctxpopup_hide(void *data, Evas *e, Evas_Object *obj,
			   void *event_info);
static void _ctxpopup_move(void *data, Evas *e, Evas_Object *obj,
			   void *event_info);
static void _ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object *obj,
				      void *event_info);
static void _item_obj_create(Elm_Ctxpopup_Item *item, char *group_name);
static void _item_sizing_eval(Elm_Ctxpopup_Item *item);
static void _ctxpopup_item_select(void *data, Evas_Object *obj,
				  const char *emission, const char *source);
static void _separator_obj_add(Evas_Object *obj);
static void _separator_obj_del(Widget_Data *wd,
			       Elm_Ctxpopup_Item *remove_item);
static Elm_Ctxpopup_Arrow _calc_base_geometry(Evas_Object *obj,
					Evas_Coord_Rectangle *rect);
static void _update_arrow_obj(Evas_Object *obj, Elm_Ctxpopup_Arrow arrow_dir);
static void _shift_base_by_arrow(Evas_Object *arrow,
				 Elm_Ctxpopup_Arrow arrow_dir,
				 Evas_Coord_Rectangle *rect);
static void _btn_layout_create(Evas_Object *obj);
static int _get_indicator_h(Evas_Object *parent);

static void
_show_effect_done(void *data, Elm_Transit *transit)
{
	//TODO: THIS FUNCTION IS TEMPORARY. It should be implemented on the edje.
	Widget_Data *wd = data;
	elm_transit_fx_clear(transit);

	if(wd->box)
		elm_transit_fx_insert(transit, elm_fx_color_add(wd->box, 0, 0, 0, 0, 255, 255, 255, 255 ));
	if(wd->content)
		elm_transit_fx_insert(transit, elm_fx_color_add(wd->content, 0, 0, 0, 0, 255, 255, 255, 255 ));
	if(wd->btn_layout)
	elm_transit_fx_insert(transit, elm_fx_color_add(wd->btn_layout, 0, 0, 0, 0, 255, 255, 255, 255 ));
	elm_transit_run(transit, 0.2);
	elm_transit_completion_callback_set(transit, NULL, NULL);
	elm_transit_del(transit);
	edje_object_signal_emit(wd->base, "elm,state,show", "elm");
	wd->transit = NULL;
}

static void
_show_effect(Widget_Data* wd)
{
	//TODO: THIS FUNCTION IS TEMPORARY. It should be implemented in the edc
	if(wd->transit) {
		elm_transit_stop(wd->transit);
		elm_transit_fx_clear(wd->transit);
	}else {
		wd->transit = elm_transit_add(wd->base);
		elm_transit_curve_style_set(wd->transit, ELM_ANIMATOR_CURVE_OUT);
		elm_transit_completion_callback_set(wd->transit, _show_effect_done, wd);
	}

	elm_transit_fx_insert(wd->transit, elm_fx_color_add( wd->base, 0, 0, 0, 0, 255, 255, 255, 255 ) );
	elm_transit_fx_insert(wd->transit, elm_fx_wipe_add( wd->base, ELM_FX_WIPE_TYPE_SHOW, wd->arrow_dir) );

	if(wd->box)
		evas_object_color_set(wd->box, 0, 0, 0, 0 );
	if(wd->content)
		evas_object_color_set(wd->content, 0, 0, 0, 0);
	if(wd->btn_layout)
		evas_object_color_set(wd->btn_layout, 0, 0, 0, 0);

	elm_transit_run(wd->transit, 0.3 );
}

static void
_separator_obj_del(Widget_Data *wd, Elm_Ctxpopup_Item *remove_item)
{
   Eina_List *elist, *cur_list, *prev_list;
   Elm_Ctxpopup_Item *separator;

   if ((!remove_item) || (!wd))
      return;
   elist = wd->items;
   cur_list = eina_list_data_find_list(elist, remove_item);
   if (!cur_list)
      return;
   prev_list = eina_list_prev(cur_list);
   if (!prev_list)
      return;
   separator = (Elm_Ctxpopup_Item *) eina_list_data_get(prev_list);
   if (!separator)
      return;
   wd->items = eina_list_remove(wd->items, separator);
   evas_object_del(separator->base);
   free(separator);
}

static void
_btn_layout_create(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   wd->btn_layout = edje_object_add(evas_object_evas_get(obj));
   elm_widget_sub_object_add(obj, wd->btn_layout);
   edje_object_signal_emit(wd->base, "elm,state,buttons,enable", "elm");
   edje_object_part_swallow(wd->base, "elm.swallow.btns", wd->btn_layout);
}

static void
_separator_obj_add(Evas_Object *obj)
{
   Elm_Ctxpopup_Item *item;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;
   if (eina_list_count(wd->items) == 0)
      return;

   item = ELM_NEW(Elm_Ctxpopup_Item);
   if (!item)
      return;

   item->base = edje_object_add(evas_object_evas_get(wd->base));
   if (!item->base)
     {
	free(item);
	return;
     }

   _elm_theme_object_set(obj, item->base, "ctxpopup", "separator",
			 elm_widget_style_get(obj));

   if (wd->horizontal)
      edje_object_signal_emit(item->base, "elm,state,horizontal", "elm");
   else
      edje_object_signal_emit(item->base, "elm,state,vertical", "elm");

   evas_object_size_hint_align_set(item->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(item->base);
   item->separator = EINA_TRUE;
   elm_box_pack_end(wd->box, item->base);
   wd->items = eina_list_append(wd->items, item);
}

static void
_item_sizing_eval(Elm_Ctxpopup_Item *item)
{
   Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;

   if (!item)
      return;

   if (!item->separator)
      elm_coords_finger_size_adjust(1, &min_w, 1, &min_h);

   edje_object_size_min_restricted_calc(item->base, &min_w, &min_h, min_w,
					min_h);

   if (!item->separator)
      elm_coords_finger_size_adjust(1, &min_w, 1, &min_h);

   evas_object_size_hint_min_set(item->base, min_w, min_h);
   evas_object_size_hint_max_set(item->base, max_w, max_h);
}

#define ADJUST_POS_X(x) do {   \
		      x  = x - (base_w/2); \
                      if(x < 0) {   \
	   		      x = 0;  \
		      }else if(x + base_w > parent_w) { \
		   	   x = parent_w - base_w; \
   	              } \
	       }while(0)

#define ADJUST_POS_Y(y) do { \
		y = y - (base_h/2); \
		if(y < 0 ) {    \
			y = 0;   \
		}else if(y + base_h > parent_h)     \
		{     \
			y = parent_h - base_h;     \
		}    \
	}while(0)

static int
_get_indicator_h(Evas_Object *parent)
{
	Ecore_X_Window zone, xwin, root;
	int w = 0, h = 0;
	int rotation = -1;
   int count;
   unsigned char *prop_data = NULL;
   int ret;

   if(elm_win_indicator_state_get(parent) != 1) {
	   return 0;
   }

   root = ecore_x_window_root_get(ecore_x_window_focus_get());
   ret  = ecore_x_window_prop_property_get(root, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
   						 ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if (ret && prop_data) memcpy(&rotation, prop_data, sizeof(int));
   if (prop_data) free(prop_data);

	xwin = elm_win_xwindow_get(parent);
	zone = ecore_x_e_illume_zone_get(xwin);
	ecore_x_e_illume_indicator_geometry_get(zone, NULL, NULL, &w, &h);

	if(w< 0) w = 0;
	if (h < 0) h = 0;

	if( (rotation == 0) || (rotation ==180)) {
		return h;
	}else {
		return w;
	}
}

static Elm_Ctxpopup_Arrow
_calc_base_geometry(Evas_Object *obj, Evas_Coord_Rectangle *rect)
{
   Widget_Data *wd;
   Evas_Coord x, y;
   Evas_Coord base_w = 0, base_h = 0;
   Elm_Ctxpopup_Arrow arrow;
   Evas_Coord x1, x2, y1, y2;
   Evas_Coord finger_size;
   Evas_Coord max_width_size, max_height_size;
   Evas_Coord arrow_w = 0, arrow_h = 0;
   Evas_Coord parent_w, parent_h;
   Evas_Coord indicator_h = 0;
   int available_direction[4] = { 1, 1, 1, 1 };
   int idx;

   wd = elm_widget_data_get(obj);

   if ((!wd) || (!rect))
     {
	return ELM_CTXPOPUP_ARROW_DOWN;
     }

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   edje_object_size_min_calc(wd->base,&base_w, &base_h);

   edje_object_size_max_get(wd->base, &max_width_size, &max_height_size);
   max_width_size *= elm_scale_get();
   max_height_size *= elm_scale_get();

   if (base_h > max_height_size)
      base_h = max_height_size;

   if (base_w > max_width_size)
      base_w = max_width_size;

   if(wd->position_forced)
   {
	   rect->x = x;
	   rect->y = y;
	   rect->w = base_w;
	   rect->h = base_h;
	   return ELM_CTXPOPUP_ARROW_DOWN;
   }

   indicator_h = _get_indicator_h(wd->parent);
   finger_size = elm_finger_size_get();

   	edje_object_part_geometry_get(wd->arrow, "ctxpopup_arrow", NULL, NULL,
				      &arrow_w, &arrow_h);
	evas_object_resize(wd->arrow, arrow_w, arrow_h);
	evas_object_geometry_get(wd->parent, NULL, NULL, &parent_w, &parent_h);

	//Define x, y Segments and find invalidated direction.
   for (idx = 0; idx < 4; ++idx)
     {
	switch (wd->arrow_priority[idx])
	  {
	  case ELM_CTXPOPUP_ARROW_DOWN:
	     y1 = y - base_h;
	     if ((y1 - arrow_h - finger_size) > indicator_h)
		continue;
	     y1 = 0;
	     available_direction[idx] = 0;
	     break;
	  case ELM_CTXPOPUP_ARROW_RIGHT:
	     x1 = (x - base_w);
	     if ((x1 - arrow_w - finger_size) > 0)
		continue;
	     x1 = 0;
	     available_direction[idx] = 0;
	     break;
	  case ELM_CTXPOPUP_ARROW_LEFT:
	     x2 = (x + base_w);
	     if ((x2 + arrow_w + finger_size) < parent_w)
		continue;
	     x2 = (parent_w - base_w);
	     available_direction[idx] = 0;
	     break;
	  case ELM_CTXPOPUP_ARROW_UP:
	     y2 = (y + base_h);
	     if ((y2 + arrow_h + finger_size) < parent_h)
		continue;
	     y2 = (parent_h - base_h);
	     available_direction[idx] = 0;
	     break;
	  default:
	     break;
	  }
     }

   //Adjust base geometry.
   for (idx = 0; idx < 4; ++idx)
     {
	if (available_direction[idx] == 0)
	   continue;

	//Find the Nearest point to center of box.
	switch (wd->arrow_priority[idx])
	  {
	  case ELM_CTXPOPUP_ARROW_DOWN:
	     ADJUST_POS_X(x);
	     y -= (base_h + finger_size);
	     arrow = ELM_CTXPOPUP_ARROW_DOWN;
	     break;
	  case ELM_CTXPOPUP_ARROW_RIGHT:
	     ADJUST_POS_Y(y);
	     x -= (base_w + finger_size);
	     arrow = ELM_CTXPOPUP_ARROW_RIGHT;
	     break;
	  case ELM_CTXPOPUP_ARROW_LEFT:
	     ADJUST_POS_Y(y);
	     x += finger_size;
	     arrow = ELM_CTXPOPUP_ARROW_LEFT;
	     break;
	  case ELM_CTXPOPUP_ARROW_UP:
	     ADJUST_POS_X(x);
	     y += finger_size;
	     arrow = ELM_CTXPOPUP_ARROW_UP;
	     break;
	  default:
	     break;
	  }
	break;
     }

   //Not enough space to locate. In this case, just show with down arrow.
   //And prevent to show the arrow.
   if( !(available_direction[0] | available_direction[1] | available_direction[2] | available_direction[3]) )
   {
	     ADJUST_POS_X(x);
	     arrow = -1;
	     y = indicator_h;
	     evas_object_hide(wd->arrow);
   }

   rect->x = x;
   rect->y = y;
   rect->w = base_w;
   rect->h = base_h;

   return arrow;
}

static void
_update_arrow_obj(Evas_Object *obj, Elm_Ctxpopup_Arrow arrow_dir)
{
   Evas_Coord x, y;
   Evas_Coord arrow_x, arrow_y, arrow_w, arrow_h;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
      return;

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_geometry_get(wd->arrow, NULL, NULL, &arrow_w, &arrow_h);

   switch (arrow_dir)
     {
     case ELM_CTXPOPUP_ARROW_LEFT:
	{
	   edje_object_signal_emit(wd->arrow, "elm,state,left", "elm");
	   arrow_y = (y - (arrow_h * 0.5));
	   arrow_x = (x + elm_finger_size_get());
	   break;
	}
     case ELM_CTXPOPUP_ARROW_RIGHT:
	{
	   edje_object_signal_emit(wd->arrow, "elm,state,right", "elm");
	   arrow_y = (y - (arrow_h * 0.5));
	   arrow_x = (x - elm_finger_size_get()-arrow_w);
	   break;
	}
     case ELM_CTXPOPUP_ARROW_UP:
	{
	   edje_object_signal_emit(wd->arrow, "elm,state,top", "elm");
	   arrow_x = (x - (arrow_w * 0.5));
	   arrow_y = (y + elm_finger_size_get());
	   break;
	}
     case ELM_CTXPOPUP_ARROW_DOWN:
	{
	   edje_object_signal_emit(wd->arrow, "elm,state,bottom", "elm");
	   arrow_x = (x - (arrow_w * 0.5));
	   arrow_y = (y - elm_finger_size_get()-arrow_h);
	   break;
	}
     default:
	break;
     }
   evas_object_move(wd->arrow, arrow_x, arrow_y);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd;
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   Evas_Coord_Rectangle rect = { 0, 0, 1, 1 };

   wd = (Widget_Data *) elm_widget_data_get(obj);

   if ((!wd) || (!wd->parent))
      return;

   EINA_LIST_FOREACH(wd->items, elist, item)
   {
      _item_sizing_eval(item);
   }

   //content
   if(wd->content) {
	   evas_object_geometry_get(wd->content, NULL, NULL, &rect.w, &rect.h);
	   evas_object_size_hint_min_set(wd->content, rect.w, rect.h);
   }

   //base
   wd->arrow_dir = _calc_base_geometry(obj, &rect);
   if ((!wd->position_forced) && (wd->arrow_dir != -1))
     {
	_update_arrow_obj(obj, wd->arrow_dir);
	_shift_base_by_arrow(wd->arrow, wd->arrow_dir, &rect);
     }

   evas_object_move(wd->base, rect.x, rect.y);
   evas_object_resize(wd->base, rect.w, rect.h);
}

static void
_shift_base_by_arrow(Evas_Object *arrow, Elm_Ctxpopup_Arrow arrow_dir,
		     Evas_Coord_Rectangle *rect)
{
   Evas_Coord arrow_w, arrow_h;

   evas_object_geometry_get(arrow, NULL, NULL, &arrow_w, &arrow_h);

   switch (arrow_dir)
     {
     case ELM_CTXPOPUP_ARROW_LEFT:
	rect->x += arrow_w;
	break;
     case ELM_CTXPOPUP_ARROW_RIGHT:
	rect->x -= arrow_w;
	break;
     case ELM_CTXPOPUP_ARROW_UP:
	rect->y += arrow_h;
	break;
     case ELM_CTXPOPUP_ARROW_DOWN:
	rect->y -= arrow_h;
	break;
     default:
	break;
     }
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;
   evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE,
				       _parent_resize, obj);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;
   elm_ctxpopup_clear(obj);
   evas_object_del(wd->arrow);
   evas_object_del(wd->base);

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;

   char buf[256];

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   //Items
   EINA_LIST_FOREACH(wd->items, elist, item)
   {
      if (item->separator)
	{
	   _elm_theme_object_set(obj, item->base, "ctxpopup", "separator",
				 elm_widget_style_get(obj));
	   if (wd->horizontal)
	      edje_object_signal_emit(item->base, "elm,state,horizontal",
				      "elm");
	}
      else
	{
	   if (item->label && item->icon)
	     {
		_elm_theme_object_set(obj, item->base, "ctxpopup",
				      "icon_text_style_item",
				      elm_widget_style_get(obj));
	     }
	   else if (item->label)
	     {
		_elm_theme_object_set(obj, item->base, "ctxpopup",
				      "text_style_item",
				      elm_widget_style_get(obj));
	     }
	   else if (item->icon)
	     {
		_elm_theme_object_set(obj, item->base, "ctxpopup",
				      "icon_style_item",
				      elm_widget_style_get(obj));
	     }
	   if (item->label)
	      edje_object_part_text_set(item->base, "elm.text", item->label);

	   if (item->disabled)
	      edje_object_signal_emit(item->base, "elm,state,disabled", "elm");
	}
      edje_object_message_signal_process(item->base);
   }

   //button layout
   if (wd->btn_layout)
     {
	sprintf(buf, "buttons%d", wd->btn_cnt);
	_elm_theme_object_set(obj, wd->btn_layout, "ctxpopup", buf,
			      elm_widget_style_get(obj));
     }

   _elm_theme_object_set(obj, wd->bg, "ctxpopup", "bg",
			 elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->base, "ctxpopup", "base",
			 elm_widget_style_get(obj));
   _elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow",
			 elm_widget_style_get(obj));

   //elm_object_style_set(wd->scroller, elm_widget_style_get(obj));

   _sizing_eval(obj);

}

static void
_bg_clicked_cb(void *data, Evas_Object *obj, const char *emission,
	       const char *source)
{
   evas_object_smart_callback_call(data, "hide", NULL);
   evas_object_hide(data);
}

static void
_parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord w, h;
    Widget_Data *wd = (Widget_Data *) elm_widget_data_get(data);

   if (!wd)
      return;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(wd->bg, w, h);

   evas_object_hide(data);
}

static void
_ctxpopup_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   if ((eina_list_count(wd->items) < 1) && (!wd->content) && (wd->btn_cnt < 1))
      return;

   _sizing_eval(obj);

   if (!wd->screen_dimmed_disabled) {
      evas_object_show(wd->bg);
      edje_object_signal_emit(wd->bg, "elm,state,show", "elm");
   }
   evas_object_show(wd->base);

   if (!wd->position_forced)
      evas_object_show(wd->arrow);

	_show_effect(wd);
   
   wd->visible = EINA_TRUE;

}

static void
_ctxpopup_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   evas_object_hide(wd->arrow);

   if (!wd->screen_dimmed_disabled)
      evas_object_hide(wd->bg);

   evas_object_hide(wd->base);

   wd->visible = EINA_FALSE;
}


static void
_ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object * obj,
			  void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (wd->visible) {
  	_sizing_eval(data);
	_show_effect(wd);
   }
}

static void
_ctxpopup_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   if( wd->visible && !wd->position_forced)
	   evas_object_show(wd->arrow);

   _sizing_eval(obj);
}

static void
_ctxpopup_item_select(void *data, Evas_Object *obj, const char *emission,
		      const char *source)
{
   Elm_Ctxpopup_Item *item = (Elm_Ctxpopup_Item *) data;

   if (!item)
      return;
   if (item->disabled)
      return;
   if (item->func)
     {
	item->func(item->data, item->ctxpopup, item);
     }
}

static void
_item_obj_create(Elm_Ctxpopup_Item *item, char *group_name)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);

   if (!wd)
      return;
   item->base = edje_object_add(evas_object_evas_get(wd->base));
   _elm_theme_object_set(item->ctxpopup, item->base, "ctxpopup", group_name,
			 elm_widget_style_get(item->ctxpopup));
   edje_object_signal_callback_add(item->base, "elm,action,click", "",
				   _ctxpopup_item_select, item);
   evas_object_size_hint_align_set(item->base, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(item->base, EVAS_HINT_EXPAND,
				    EVAS_HINT_EXPAND);
   evas_object_show(item->base);
}

/**
 * Get the icon object for the given item.
 *
 * @param item 	Ctxpopup item
 * @return 		Icon object or NULL if the item does not have icon
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_item_icon_get(Elm_Ctxpopup_Item *item)
{
   if (!item)
      return NULL;
   return item->icon;
}

/**
 * Disable or Enable the scroller for contextual popup.
 *
 * @param obj 		Ctxpopup object
 * @param disabled  disable or enable
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_scroller_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;
   if (wd->scroller_disabled == disabled)
      return;

   if (disabled)
      elm_object_scroll_freeze_push(wd->scroller);
   else
      elm_object_scroll_freeze_pop(wd->scroller);

   wd->scroller_disabled = disabled;
}

/**
 * Get the label for the given item.
 *
 * @param item 	 	Ctxpopup item
 * @return 		Label or NULL if the item does not have label
 *
 * @ingroup Ctxpopup
 *
 */
EAPI const char *
elm_ctxpopup_item_label_get(Elm_Ctxpopup_Item *item)
{
   if (!item)
      return NULL;
   return item->label;
}

/**
 * Add a new ctxpopup object to the parent.
 *
 * @param parent 	Parent object
 * @return 		New object or NULL if it cannot be created
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_Coord x, y, w, h;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "ctxpopup");
   elm_widget_type_set(obj, "ctxpopup");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->parent = parent;

   //Background
   wd->bg = edje_object_add(e);
   elm_widget_sub_object_add(obj, wd->bg);
   _elm_theme_object_set(obj, wd->bg, "ctxpopup", "bg",
			 elm_widget_style_get(obj));
   evas_object_geometry_get(parent, &x, &y, &w, &h);
   evas_object_move(wd->bg, x, y);
   evas_object_resize(wd->bg, w, h);
   edje_object_signal_callback_add(wd->bg, "elm,action,click", "",
				   _bg_clicked_cb, obj);

   //Base
   wd->base = edje_object_add(e);
   elm_widget_sub_object_add(obj, wd->base);
   _elm_theme_object_set(obj, wd->base, "ctxpopup", "base",
			 elm_widget_style_get(obj));

   //Scroller
   wd->scroller = elm_scroller_add(obj);
   elm_object_style_set(wd->scroller, "vertical");
   elm_scroller_content_min_limit(wd->scroller, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_OFF,
			   ELM_SCROLLER_POLICY_ON);
   elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
   edje_object_part_swallow(wd->base, "elm.swallow.scroller", wd->scroller);
   evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE,
				  _ctxpopup_scroller_resize, obj);

   //Box
   wd->box = elm_box_add(obj);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_scroller_content_set(wd->scroller, wd->box);

   //Arrow
   wd->arrow = edje_object_add(e);
   elm_widget_sub_object_add(obj, wd->arrow);
   _elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow",
			 elm_widget_style_get(obj));

   wd->arrow_priority[0] = ELM_CTXPOPUP_ARROW_DOWN;
   wd->arrow_priority[1] = ELM_CTXPOPUP_ARROW_RIGHT;
   wd->arrow_priority[2] = ELM_CTXPOPUP_ARROW_LEFT;
   wd->arrow_priority[3] = ELM_CTXPOPUP_ARROW_UP;

   evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_resize,
				  obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _ctxpopup_show, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _ctxpopup_hide, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _ctxpopup_move, wd);
   
   return obj;
}

/**
 * Clear all items in given ctxpopup object.
 *
 * @param obj 		Ctxpopup object
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_clear(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   EINA_LIST_FOREACH(wd->items, elist, item)
   {
      if (item->label)
	 eina_stringshare_del(item->label);
      if (item->icon)
	 evas_object_del(item->icon);
      wd->items = eina_list_remove(wd->items, item);
      free(item);
   }
   evas_object_hide(wd->arrow);
   evas_object_hide(wd->base);
}

/**
 * Change the mode to horizontal or vertical.
 *
 * @param obj   	Ctxpopup object
 * @param horizontal 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
	Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   if (wd->horizontal == horizontal)
      return;
   wd->horizontal = horizontal;
   if (!horizontal)
     {
	elm_object_style_set(wd->scroller, "vertical");
	elm_box_horizontal_set(wd->box, EINA_FALSE);
	elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_ON,
				ELM_SCROLLER_POLICY_OFF);
	EINA_LIST_FOREACH(wd->items, elist, item)
	   edje_object_signal_emit(item->base, "elm,state,vertical", "elm");
     }
   else
     {
	elm_object_style_set(wd->scroller, "horizontal");
	elm_box_horizontal_set(wd->box, EINA_TRUE);
	elm_scroller_bounce_set(wd->scroller, EINA_TRUE, EINA_FALSE);
	elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_ON,
				ELM_SCROLLER_POLICY_OFF);
	EINA_LIST_FOREACH(wd->items, elist, item)
	   edje_object_signal_emit(item->base, "elm,state,horizontal", "elm");
     }
}

/**
 * Get the value of current horizontal mode.
 *
 * @param obj 	 	Ctxpopup object
 * @return 	 	EINA_TRUE - horizontal mode, EINA_FALSE - vertical mode.
 *
 * @ingroup Ctxpopup
 */
EAPI Eina_Bool
elm_ctxpopup_horizontal_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return EINA_FALSE;

   return wd->horizontal;
}

/**
 * reset the icon on the given item. This function is only for icon item.
 *
 * @param obj 	 	Ctxpopup item
 * @param icon		Icon object to be set
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_item_icon_set(Elm_Ctxpopup_Item *item, Evas_Object *icon)
{
   Widget_Data *wd;

   if (!item)
      return;
   wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
   if (!wd)
      return;
   if (item->icon == icon)
      return;
   if (item->icon)
     {
	elm_widget_sub_object_del(item->base, item->icon);
	evas_object_del(item->icon);
     }
   item->icon = icon;
   edje_object_part_swallow(item->base, "elm.swallow.icon", item->icon);
   edje_object_message_signal_process(item->base);

   if (wd->visible)
      _sizing_eval(item->ctxpopup);
}

/**
 * reset the label on the given item. This function is only for label item.
 *
 * @param obj 	 	Ctxpopup item
 * @param label		Label to be set
 * 
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_item_label_set(Elm_Ctxpopup_Item *item, const char *label)
{
   Widget_Data *wd;

   if (!item)
      return;

   if (item->label)
     {
	eina_stringshare_del(item->label);
	item->label = NULL;
     }

   item->label = eina_stringshare_add(label);
   edje_object_message_signal_process(item->base);
   edje_object_part_text_set(item->base, "elm.text", label);

   wd = elm_widget_data_get(item->ctxpopup);
   if (!wd)
      return;
   if (wd->visible)
      _sizing_eval(item->ctxpopup);
}

/**
 * Add a new item in given ctxpopup object.
 *
 * @param obj 	 	Ctxpopup object
 * @param icon		Icon to be set
 * @param label   Label to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data    User data for callback function
 * @return 		Added ctxpopup item
 * 
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_item_add(Evas_Object *obj, Evas_Object *icon, const char *label,
		      Evas_Smart_Cb func, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Ctxpopup_Item *item;
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return NULL;

   _separator_obj_add(obj);

   item = ELM_NEW(Elm_Ctxpopup_Item);
   if (!item)
      return NULL;

   item->func = func;
   item->data = data;
   item->ctxpopup = obj;
   item->separator = EINA_FALSE;

   if (icon && label)
      _item_obj_create(item, "icon_text_style_item");
   else if (icon)
      _item_obj_create(item, "icon_style_item");
   else
      _item_obj_create(item, "text_style_item");

   if(eina_list_count(wd->items)==0)
	   edje_object_signal_emit(wd->base, "elm,state,scroller,enable", "elm");

   wd->items = eina_list_append(wd->items, item);
   elm_box_pack_end(wd->box, item->base);
   elm_ctxpopup_item_icon_set(item, icon);
   elm_ctxpopup_item_label_set(item, label);

   return item;
}

/**
 * Delete the given item in ctxpopup object.
 *
 * @param item 	 	Ctxpopup item to be deleted
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_item_del(Elm_Ctxpopup_Item *item)
{
   Widget_Data *wd;

   if (!item)
      return;
   if (item->label)
      eina_stringshare_del(item->label);
   if (item->icon)
      evas_object_del(item->icon);
   if (item->base)
      evas_object_del(item->base);

   wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
   if (wd)
     {
	_separator_obj_del(wd, item);
	wd->items = eina_list_remove(wd->items, item);
     }
   free(item);
   if (eina_list_count(wd->items) == 0)
     {
	evas_object_hide(wd->arrow);
	evas_object_hide(wd->base);
	if (!wd->screen_dimmed_disabled)
	   evas_object_hide(wd->bg);
	edje_object_signal_emit(wd->base, "elm,state,scroller,disable", "elm");
     }
}

/**
 * Disable or Enable the given item. Once an item is disabled, the click event will be never happend for the item.
 *
 * @param item		Ctxpopup item to be disabled
 * @param disabled 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_item_disabled_set(Elm_Ctxpopup_Item *item, Eina_Bool disabled)
{
   Widget_Data *wd;

   if (!item)
      return;
   if (disabled == item->disabled)
      return;
   wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);

   if (disabled)
      edje_object_signal_emit(item->base, "elm,state,disabled", "elm");
   else
      edje_object_signal_emit(item->base, "elm,state,enabled", "elm");

   edje_object_message_signal_process(item->base);
   item->disabled = disabled;
}

/**
 * Disable or Enable background dimmed function 
 * @param obj		Ctxpopup object
 * @param dimmed 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_screen_dimmed_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   wd->screen_dimmed_disabled = disabled;

   if(wd->visible) {
	   if (!disabled) {
	      evas_object_show(wd->bg);
	   }
 	}	
}

/**
 * Append additional button in ctxpoppup bottom layout.
 * @param obj		Ctxpopup object
 * @param label  Button label
 * @param func   Button clicked event callback function
 * @param data   Button clicked event callback function data
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_button_append(Evas_Object *obj, const char *label,
			   Evas_Smart_Cb func, const void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   char buf[256];
   Evas_Object *btn;
   Evas_Coord w, h;
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   if (!wd->btn_layout)
     {
	_btn_layout_create(obj);
     }

   ++wd->btn_cnt;
   sprintf(buf, "buttons%d", wd->btn_cnt);
   _elm_theme_object_set(obj, wd->btn_layout, "ctxpopup", buf,
			 elm_widget_style_get(obj));

	btn = elm_button_add(obj);
	elm_object_style_set(btn, "text_only/style1");
	elm_button_label_set(btn, label);
	evas_object_smart_callback_add(btn, "clicked", func, data);
	sprintf(buf, "actionbtn%d", wd->btn_cnt);
	edje_object_part_swallow(wd->btn_layout, buf, btn);

	edje_object_part_geometry_get(wd->btn_layout, buf, NULL, NULL, &w, &h);
   	evas_object_size_hint_min_set(wd->btn_layout, w, h);
   	evas_object_size_hint_max_set(wd->btn_layout, -1, h);

   if (wd->visible)
      _sizing_eval(obj);

}

/**
 * Set the priority of arrow direction
 *
 *  This functions gives user to set the priority of ctxpopup box showing position.
 *
 * @param obj		Ctxpopup object
 * @param first    1st priority of arrow direction
 * @param second 2nd priority of arrow direction
 * @param third   3th priority of arrow direction
 * @param fourth 4th priority of arrow direction
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_arrow_priority_set(Evas_Object *obj, Elm_Ctxpopup_Arrow first,
				Elm_Ctxpopup_Arrow second,
				Elm_Ctxpopup_Arrow third,
				Elm_Ctxpopup_Arrow fourth)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;
   wd->arrow_priority[0] = first;
   wd->arrow_priority[1] = second;
   wd->arrow_priority[2] = third;
   wd->arrow_priority[3] = fourth;
}

/**
 * Swallow the user contents
 *
 * @param obj		Ctxpopup object
 * @param content 		Contents to be swallowed
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_content_set(Evas_Object *obj, Evas_Object *content)
{
	   ELM_CHECK_WIDTYPE(obj, widtype);
	   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	   Evas_Coord w, h;

	   edje_object_part_swallow(wd->base,  "elm.swallow.content", content);
	   elm_widget_sub_object_add(obj, content);
	   wd->content = content;
	   edje_object_signal_emit(wd->base, "elm,state,content,enable", "elm");

	   evas_object_size_hint_min_get(content, &w, &h);

	   if( (w == 0) && (h == 0)) {
		   evas_object_geometry_get(content, NULL, NULL, &w, &h);
		   evas_object_size_hint_min_set(content, w, h);
	   }

	   elm_ctxpopup_scroller_disabled_set(obj, EINA_TRUE);

	   if(wd->visible)
		   _sizing_eval(obj);
}

/**
 * Unswallow the user contents
 *
 * @param obj		Ctxpopup object
 * @return 			The unswallowed contents
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object *
elm_ctxpopup_content_unset(Evas_Object *obj)
{
	   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
	   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	   Evas_Object *content;

	   content = wd->content;
	   wd->content = NULL;

	   if(content)
		   edje_object_part_unswallow(wd->base,  content);

	   elm_widget_sub_object_del(obj, content);
	   edje_object_signal_emit(wd->base, "elm,state,content,disable", "elm");

	   elm_ctxpopup_scroller_disabled_set(obj, EINA_FALSE);

	   if(wd->visible)
		   _sizing_eval(obj);

	   return content;
}

/**
 * Change the origin of the ctxpopup position.
 *
 * Basically, ctxpopup position is computed internally. When user call evas_object_move,
 * Ctxpopup will be showed up with that position which is indicates the arrow point.
 *
 * @param obj		Ctxpopup object
 * @param forced		EINA_TRUE is left-top. EINA_FALSE is indicates arrow point.
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_position_forced_set(Evas_Object *obj, Eina_Bool forced)
{
	   ELM_CHECK_WIDTYPE(obj, widtype);
	   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	   wd->position_forced = forced;

	   if(forced)
		   evas_object_hide(wd->arrow);

	   if(wd->visible)
		   _sizing_eval(obj);
}

/**
 * Get the status of the position forced
 *
 * @param obj		Ctxpopup objet
 * @return			value of position forced
 *
 * @ingroup Ctxpopup
 */
EAPI Eina_Bool
elm_ctxpopup_position_forced_get(Evas_Object *obj)
{
	   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
	   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

	   return wd->position_forced;
}

EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_icon_add(Evas_Object *obj, Evas_Object *icon,
		      Evas_Smart_Cb func, void *data)
{
	fprintf( stderr, "elm_ctxpopup_icon_add is deprecated!! Pleaes use \"elm_ctxpopup_item_add.\"");
	return elm_ctxpopup_item_add(obj, icon, NULL, func, data);
}

EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_label_add(Evas_Object *obj, const char *label,
		       Evas_Smart_Cb func, void *data)
{
	fprintf( stderr, "elm_ctxpopup_label_add is deprecated!! Pleaes use \"elm_ctxpopup_item_add.\"");
	return elm_ctxpopup_item_add(obj, NULL, label, func, data);
}
