#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Ctxpopup Ctxpopup
 * @ingroup Elementary
 *
 * Contextual popup.
 *
 *Signals that you can add callbacks for are:
 *
 * ctxpopup,hide - This is called whenever the ctxpopup is hided.
 *
 */

typedef struct _Widget_Data Widget_Data;

typedef enum
{ BOTTOM_ARROW, RIGHT_ARROW, LEFT_ARROW, TOP_ARROW,
      NONE_ARROW } Arrow_Direction;

struct _Ctxpopup_Item
{
   Evas_Object *ctxpopup;
   Evas_Object *base;
   const char *label;
   Evas_Object *content;
   void (*func) (void *data, Evas_Object *obj, void *event_info);
   const void *data;
   Eina_Bool disabled:1;
   Eina_Bool separator:1;
};

struct _Widget_Data
{
   Evas_Object *parent;
   Evas_Object *base;
   Evas_Object *box;
   Evas_Object *arrow;
   Evas_Object *scroller;
   Evas_Object *bg;
   Evas_Object *btn_layout;
   Eina_List *items;
   double align_x, align_y;
   int btn_cnt;
   Eina_Bool scroller_disabled:1;
   Eina_Bool horizontal:1;
	Eina_Bool arrow_disabled:1;
	Eina_Bool visible:1;
	Eina_Bool screen_dimmed_disabled:1;

};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _bg_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _parent_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ctxpopup_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ctxpopup_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ctxpopup_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _item_obj_create(Elm_Ctxpopup_Item *item, char *group_name);
static void _item_sizing_eval(Elm_Ctxpopup_Item *item);
static void _ctxpopup_item_select(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _separator_obj_add(Evas_Object *obj); 
static void _separator_obj_del(Widget_Data *wd, Elm_Ctxpopup_Item *remove_item);
static Arrow_Direction _calc_base_geometry(Evas_Object *obj, Evas_Coord_Rectangle *rect);
static void _arrow_obj_add(Evas_Object *obj, const char *group_name);
static void _update_arrow_obj(Evas_Object *obj, Arrow_Direction arrow_dir);
static void _shift_base_by_arrow(Evas_Object *arrow, Arrow_Direction arrow_dir, Evas_Coord_Rectangle *rect);
static void _btn_layout_create(Evas_Object *obj);
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

   item = calloc(1, sizeof(Elm_Ctxpopup_Item));
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

#define WORLD_PARENT_W (parent_x+parent_w)
#define WORLD_PARENT_H (parent_y+parent_h)
#define BOX_HALF_W (box_w/2)
#define BOX_HALF_H (box_h/2)

#define ADJUST_POS_X(x) do {   \
		      x  = x - (box_w/2); \
                      if(x < x1) {   \
	   		      x = x1;  \
		      }else if(x + box_w > WORLD_PARENT_W) { \
		   	   x = WORLD_PARENT_W - box_w; \
   	              } \
	       }while(0)

#define ADJUST_POS_Y(y) do { \
		y = y - (box_h/2); \
		if(y < y1) {    \
			y = y1;   \
		}else if(y + box_h > WORLD_PARENT_H)     \
		{     \
			y = WORLD_PARENT_H - box_h;     \
		}    \
	}while(0)

static Arrow_Direction
_calc_base_geometry(Evas_Object *obj, Evas_Coord_Rectangle *rect)
{
	Widget_Data *wd;
	Evas_Coord x, y;
   Evas_Coord parent_x, parent_y, parent_w, parent_h;
   Evas_Coord box_w, box_h;
   Evas_Coord base_w = 0, base_h = 0;
   Arrow_Direction arrow_dir;
   Evas_Coord x1, x2, y1, y2;
   int idx;
   Evas_Coord finger_size;
   Evas_Coord max_width_size, max_height_size;
   Evas_Coord arrow_w=0, arrow_h=0;
   int available_direction[4] = {1, 1, 1, 1};

	wd = elm_widget_data_get(obj);

   if ((!wd) || (!rect))
      return NONE_ARROW;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	evas_object_geometry_get(wd->parent, &parent_x, &parent_y, &parent_w,
			    &parent_h);
   evas_object_geometry_get(wd->box, NULL, NULL, &box_w, &box_h);
   edje_object_part_geometry_get( wd->base, "ctxpopup_btns_frame", NULL, NULL, &base_w, &base_h );

	if(box_w > base_w) base_w = box_w;
	base_h += box_h ;

	edje_object_size_max_get(wd->base, &max_width_size, &max_height_size);

   if (base_h > max_height_size)
      base_h = max_height_size;

   if (base_w > max_width_size)
      base_w = max_width_size;

   finger_size = elm_finger_size_get();

	if((base_h == 0) || (base_w == 0))
		return NONE_ARROW;

	if (!wd->arrow_disabled)
		{
				edje_object_part_geometry_get( wd->arrow, "ctxpopup_arrow", NULL, NULL, &arrow_w, &arrow_h);
				evas_object_resize( wd->arrow, arrow_w, arrow_h );
		}

   //Phase 1: Define x, y Segments and find invalidated direction.
   //Left
   x1 = x - base_w;
   if ((x1 - arrow_w - finger_size) < parent_x)
     {
	x1 = parent_x;
	available_direction[RIGHT_ARROW] = 0;
     }

   //Right
   x2 = x + base_w;
   if (x2 + arrow_w + finger_size > WORLD_PARENT_W)
     {
	x2 = WORLD_PARENT_W - base_w;
	available_direction[LEFT_ARROW] = 0;
     }

   //Top
   y1 = y - base_h;
   if (y1 - arrow_h - finger_size < parent_y)
     {
	y1 = parent_y;
	available_direction[BOTTOM_ARROW] = 0;
     }
   //Bottom
   y2 = y + base_h;
   if (y2 + arrow_h + finger_size > WORLD_PARENT_H)
     {
	y2 = WORLD_PARENT_H - base_h;
	available_direction[TOP_ARROW] = 0;
     }

//ADDITIONAL OPTION: Phase 2: Determine Direction Priority ?

//Phase 3: adjust base geometry.
   for (idx = 0; idx < 4; ++idx)
     {
	if (available_direction[idx] == 0)
	   continue;

	//Find the Nearest point to center of box.
	switch (idx)
	  {
	  case BOTTOM_ARROW:
	     ADJUST_POS_X(x);
	     y -= (base_h + finger_size);
	     arrow_dir = BOTTOM_ARROW;
	     break;
	  case RIGHT_ARROW:
	     ADJUST_POS_Y(y);
	     x -= (base_w + finger_size);
	     arrow_dir = RIGHT_ARROW;
	     break;
	  case LEFT_ARROW:
	     ADJUST_POS_Y(y);
	     x += finger_size;
	     arrow_dir = LEFT_ARROW;
	     break;
	  case TOP_ARROW:
	     ADJUST_POS_X(x);
	     y += finger_size;
	     arrow_dir = TOP_ARROW;
	     break;
	  default:
	     fprintf(stderr, "Not Enough space to show contextual popup!! \n");
	  }
	break;
     }
   rect->x = x;
   rect->y = y;
   rect->w = base_w;
   rect->h = base_h;

   return arrow_dir;
}


static void
_update_arrow_obj(Evas_Object *obj, Arrow_Direction arrow_dir)
{
  	Evas_Coord x, y;
	Evas_Coord arrow_x, arrow_y, arrow_w, arrow_h;
	Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
      return;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	evas_object_geometry_get(wd->arrow, NULL, NULL, &arrow_w, &arrow_h );

	switch (arrow_dir)
     {
     case LEFT_ARROW:
	{
		edje_object_signal_emit(wd->arrow, "elm,state,left", "elm");
	   arrow_y = (y - (arrow_h*0.5));
	   arrow_x = (x + elm_finger_size_get());
	   break;
	}
     case RIGHT_ARROW:
	{
		edje_object_signal_emit(wd->arrow, "elm,state,right", "elm");
	   arrow_y = (y - (arrow_h*0.5));
		arrow_x = (x - elm_finger_size_get() - arrow_w);
	   break;
	}
     case TOP_ARROW:
	{
		edje_object_signal_emit(wd->arrow, "elm,state,top", "elm");
	   arrow_x = (x - (arrow_w*0.5));
	   arrow_y = (y + elm_finger_size_get());
	   break;
	}
     case BOTTOM_ARROW:
	{
		edje_object_signal_emit(wd->arrow, "elm,state,bottom", "elm");
		arrow_x = (x - (arrow_w*0.5));
	    arrow_y = (y - elm_finger_size_get() - arrow_h);
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
   Evas_Coord_Rectangle rect = {0,0,1,1};
   Evas_Coord x, y, w, h;
   wd = (Widget_Data *) elm_widget_data_get(obj);

   if ((!wd) || (!wd->parent))
      return;

   EINA_LIST_FOREACH(wd->items, elist, item)
   {
      _item_sizing_eval(item);
   }

   Arrow_Direction arrow_dir = _calc_base_geometry(obj, &rect);

	if((!wd->arrow_disabled) && (arrow_dir!=NONE_ARROW)) 
	{
		_update_arrow_obj(obj, arrow_dir);
		_shift_base_by_arrow(wd->arrow, arrow_dir, &rect);
	}
/*
	if(wd->btn_layout) {
		Evas_Coord temp;
		edje_object_part_geometry_get(wd->base, "ctxpopup_list", &x, &y, &w, &h);
		fprintf( stderr, "%d %d %d %d\n", x, y, w, h );
		evas_object_resize(wd->scroller, w, h+(y*2));
	}else{ */
		evas_object_resize(wd->scroller, rect.w, rect.h);
	//}

	evas_object_move(wd->scroller, rect.x, rect.y);
	evas_object_resize(wd->base, rect.w, rect.h);
	evas_object_move(wd->base, rect.x, rect.y);
}

static void
_shift_base_by_arrow(Evas_Object *arrow, Arrow_Direction arrow_dir,
			 Evas_Coord_Rectangle *rect)
{
   Evas_Coord arrow_w, arrow_h;
   evas_object_geometry_get( arrow, NULL, NULL, &arrow_w, &arrow_h );

	switch (arrow_dir)
     {
     case LEFT_ARROW:
	rect->x += arrow_w;
	break;
     case RIGHT_ARROW:
	rect->x -= arrow_w;
	break;
     case TOP_ARROW:
	rect->y += arrow_h;
	break;
     case BOTTOM_ARROW:
	rect->y -= arrow_h;
	break;
     case NONE_ARROW:
	break;
     }
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd) return;
   evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE,
				       _parent_resize, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_MOVE, _parent_move, obj);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd) return;
   elm_ctxpopup_clear(obj);
	evas_object_del(wd->arrow);
	evas_object_del(wd->base);

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Eina_List *elist, *elist_child, *elist_temp;
   Elm_Ctxpopup_Item *item;

   int item_count;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   item_count = eina_list_count(wd->items);
   item_count -= (item_count / 2);
   elist = eina_list_append(elist, wd->items);

   EINA_LIST_FOREACH(elist, elist, elist_child)
   {
      EINA_LIST_FOREACH(elist_child, elist_temp, item)
      {
	 if (item->separator)
	   {
	      _elm_theme_object_set(obj, item->base, "ctxpopup", "separator",
				    elm_widget_style_get(obj));
	      if (wd->horizontal)
		 edje_object_signal_emit(item->base, "elm,state,horizontal",
					 "elm");
	      else
		 edje_object_signal_emit(item->base, "elm,state,vertical",
					 "elm");
	   }
	 else
	   {
	      _elm_theme_object_set(obj, item->base, "ctxpopup", "item",
				    elm_widget_style_get(obj));
	      edje_object_part_text_set(item->base, "elm.text", item->label);

	      if (item->label)
		 edje_object_part_text_set(item->base, "elm.text", item->label);
	      else if (item->content)
		 edje_object_signal_emit(item->base, "elm,state,enable_icon",
					 "elm");

	      if (item->disabled)
		{
		      edje_object_signal_emit(item->base, "elm,state,disabled",
					      "elm");
		}
	      else
		{
		      edje_object_signal_emit(item->base, "elm,state,enabled",
					      "elm");
		}
	   }
	 edje_object_message_signal_process(item->base);
      }
   }

   if (wd->horizontal)
      elm_object_style_set(wd->scroller, "ctxpopup_hbar");
   else
      elm_object_style_set(wd->scroller, "ctxpopup_vbar");

   _elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow",
			 elm_widget_style_get(obj));
   _sizing_eval(obj);
}


static void
_bg_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	evas_object_smart_callback_call(data, "ctxpopup,hide", NULL);
	evas_object_hide(data);
}

static void 
_parent_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;

	Evas_Coord x, y;
	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	evas_object_move(wd->bg, x, y);
	_sizing_eval(data);
}

static void
_parent_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;

	Evas_Coord w, h;
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	evas_object_resize(wd->bg, w, h);
	_sizing_eval(data);
}

static void
_ctxpopup_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   if (!wd) return;
   if (eina_list_count(wd->items) < 1) return;

   _sizing_eval(obj);

	if(!wd->screen_dimmed_disabled) 
		evas_object_show(wd->bg);
	evas_object_show(wd->base);
	
	if(!wd->arrow_disabled) 
		evas_object_show(wd->arrow);
	
	wd->visible = EINA_TRUE;
}

static void
_ctxpopup_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   if (!wd) return;
   evas_object_hide(wd->arrow);

	if(!wd->screen_dimmed_disabled)
		evas_object_hide(wd->bg);

	evas_object_hide(wd->base);

	wd->visible = EINA_FALSE;
}

static void
_ctxpopup_scroller_resize(void *data, Evas *e, Evas_Object *obj,
			  void *event_info)
{
	Widget_Data* wd = elm_widget_data_get(data);
	if(!wd) return;

	if(wd->visible)
		_sizing_eval(data);
}

static void
_ctxpopup_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;

   if (!wd)
      return;
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
      item->func((void *)(item->data), item->ctxpopup, item);
}

static void
_arrow_obj_add(Evas_Object *obj, const char *group_name)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return;

   if (wd->arrow)
     {
	elm_widget_sub_object_del(obj, wd->arrow);
	evas_object_del(wd->arrow);
	wd->arrow = NULL;
     }
   wd->arrow = edje_object_add(evas_object_evas_get(wd->base));
   elm_widget_sub_object_add(obj, wd->arrow);
   
   if (evas_object_visible_get(obj))
      evas_object_show(wd->arrow);
}

static void
_item_obj_create(Elm_Ctxpopup_Item *item,  char *group_name)
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
   return item->content;
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
	wd->align_x = 0.5;
	wd->align_y = 0.5;

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
      elm_object_style_set(wd->scroller, "ctxpopup_vbar");
   elm_scroller_content_min_limit(wd->scroller, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_OFF,
			   ELM_SCROLLER_POLICY_ON);
   elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
	edje_object_part_swallow(wd->base, "elm.swallow.content", wd->scroller);
   evas_object_event_callback_add(wd->scroller, EVAS_CALLBACK_RESIZE,
				  _ctxpopup_scroller_resize, obj);

   //Box
   wd->box = elm_box_add(obj);
   elm_scroller_content_set(wd->scroller, wd->box);
   evas_object_size_hint_align_set(wd->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(wd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	//Arrow
	wd->arrow = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->arrow);
	_elm_theme_object_set(obj, wd->arrow, "ctxpopup", "arrow",
				      elm_widget_style_get(obj));

	evas_object_event_callback_add(parent, EVAS_CALLBACK_MOVE, _parent_move, obj);
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
      if (item->content)
	 evas_object_del(item->content);
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
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   ELM_CHECK_WIDTYPE(obj, widtype);
   if (!wd)
      return;

   if (wd->horizontal == horizontal)
      return;
   wd->horizontal = horizontal;
   if (!horizontal)
     {
	elm_object_style_set(wd->scroller, "ctxpopup_vbar");
	elm_box_horizontal_set(wd->box, EINA_FALSE);
	elm_scroller_bounce_set(wd->scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(wd->scroller, ELM_SCROLLER_POLICY_ON,
				ELM_SCROLLER_POLICY_OFF);
	EINA_LIST_FOREACH(wd->items, elist, item)
		edje_object_signal_emit(item->base, "elm,state,vertical", "elm");
     }
   else
     {
	elm_object_style_set(wd->scroller, "ctxpopup_hbar");
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

   if (!item) return;
   wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
   if (!wd)
      return;
   if (item->content == icon)
      return;
   if (item->content)
     {
	elm_widget_sub_object_del(item->base, item->content);
	evas_object_del( item->content );
     }
   item->content = icon;
   edje_object_part_swallow(item->base, "elm.swallow.icon", item->content);
   edje_object_message_signal_process(item->base);

	if(wd->visible) 
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
	Widget_Data* wd; 
		  
	if (!item)
      return;

   if (item->label) {
	   eina_stringshare_del(item->label);
	   item->label = NULL;
   }

   item->label = eina_stringshare_add(label);
   edje_object_message_signal_process(item->base);
   edje_object_part_text_set(item->base, "elm.text", label);

	wd = elm_widget_data_get(item->ctxpopup);
	if(!wd) return;
	if(wd->visible)
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
elm_ctxpopup_item_add(Evas_Object *obj, Evas_Object *icon, const char* label, void (*func) (void *data, Evas_Object *obj, void *event_info), void* data)
{
	ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Ctxpopup_Item *item;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return NULL;

   _separator_obj_add(obj);

   item = calloc(1, sizeof(Elm_Ctxpopup_Item));
   if (!item)
      return NULL;

   item->func = func;
   item->data = data;
   item->ctxpopup = obj;
   item->separator = EINA_FALSE;

   _item_obj_create(item, "icon_text_style_item");
   wd->items = eina_list_append(wd->items, item);
   elm_box_pack_end(wd->box, item->base);
   elm_ctxpopup_item_icon_set(item, icon);
   elm_ctxpopup_item_label_set(item, label);

	return item;
}

/**
 * Add a new item as an icon in given ctxpopup object.
 *
 * @param obj 	 	Ctxpopup object
 * @param icon		Icon to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data          User data for callback function
 * @return 		Added ctxpopup item
 * 
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_icon_add(Evas_Object *obj, Evas_Object *icon,
		      void (*func) (void *data, Evas_Object *obj,
				    void *event_info), void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Ctxpopup_Item *item;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return NULL;

   _separator_obj_add(obj);

   item = calloc(1, sizeof(Elm_Ctxpopup_Item));
   if (!item)
      return NULL;

   item->func = func;
   item->data = data;
   item->ctxpopup = obj;
   item->separator = EINA_FALSE;
   _item_obj_create(item, "icon_style_item");
   wd->items = eina_list_append(wd->items, item);
   elm_box_pack_end(wd->box, item->base);
   elm_ctxpopup_item_icon_set(item, icon);
	return item;
}

/**
 * Add a new item as an label in given ctxpopup object.
 *
 * @param obj 	 	Ctxpopup object
 * @param icon		label to be set
 * @param func		Callback function to call when this item click is clicked
 * @param data    User data for callback function
 * @return 		Added ctxpopup item
 *
 * @ingroup Ctxpopup
 */
EAPI Elm_Ctxpopup_Item *
elm_ctxpopup_label_add(Evas_Object *obj, const char *label,
		       void (*func) (void *data, Evas_Object *obj,
				     void *event_info), void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Elm_Ctxpopup_Item *item;

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);

   if (!wd)
      return NULL;

   _separator_obj_add(obj);
   item = calloc(1, sizeof(Elm_Ctxpopup_Item));
   if (!item)
      return NULL;

   item->func = func;
   item->data = data;
   item->ctxpopup = obj;
   item->separator = EINA_FALSE;
   _item_obj_create(item, "text_style_item");
      wd->items = eina_list_append(wd->items, item);
   elm_box_pack_end(wd->box, item->base);
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
   if (item->content)
      evas_object_del(item->content);
   if (item->base)
      evas_object_del(item->base);

   wd = (Widget_Data *) elm_widget_data_get(item->ctxpopup);
   if (wd)
     {
	_separator_obj_del(wd, item);
	wd->items = eina_list_remove(wd->items, item);
     }
   free(item);
   if (eina_list_count(wd->items) < 1)
     {
	evas_object_hide(wd->arrow);
	evas_object_hide(wd->base);
	if (!wd->screen_dimmed_disabled) evas_object_hide(wd->bg);
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
 * Disable or Enable arrow. 
 * @param obj		Ctxpopup object
 * @param disabled 	EINA_TRUE - disable, EINA_FALSE - enable
 *
 * @ingroup Ctxpopup
 */
EAPI void
elm_ctxpopup_arrow_disabled_set(Evas_Object* obj, Eina_Bool disabled)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(!wd) return;
	
	if(wd->arrow_disabled == disabled) return;

	wd->arrow_disabled = disabled;

	if(disabled) evas_object_hide(wd->arrow);		
	else evas_object_show(wd->arrow);

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
	if(!wd) return;

	wd->screen_dimmed_disabled = disabled;
}

EAPI void
elm_ctxpopup_button_append(Evas_Object *obj, const char *label)
{
	ELM_CHECK_WIDTYPE(obj, widtype);

	char buf[ 256 ];
	int idx;
	Evas_Object *btn;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(!wd) return;

	if(!wd->btn_layout) {
		_btn_layout_create(obj);
		edje_object_signal_emit(wd->base, "elm,state,buttons,enable", "elm");
	}else {
		//TODO: Change Theme,
	}

	++wd->btn_cnt;
	sprintf( buf, "buttons%d", wd->btn_cnt );
	_elm_theme_object_set(obj, wd->btn_layout, "ctxpopup", buf,   elm_widget_style_get(obj));

	for(idx=0;idx<wd->btn_cnt;++idx) {
		btn = elm_button_add(obj);
		elm_object_style_set(btn, "text_only/style1");
		elm_button_label_set(btn, label);
		sprintf(buf, "actionbtn%d", wd->btn_cnt);
		edje_object_part_swallow(wd->btn_layout,  buf, btn);
		Evas_Coord w, h;
		edje_object_part_geometry_get( wd->btn_layout, "actionbtn1", 0, 0, &w, &h );
		evas_object_size_hint_min_set( wd->btn_layout, w, h );
	}

	edje_object_part_swallow(wd->base, "elm.swallow.btns", wd->btn_layout);
	_sizing_eval(obj);



}


/*
EAPI void
elm_ctxpopup_align_set(Evas_Object *obj, double align_x, double align_y)
{
	Eina_List *elist;
	Elm_Ctxpopup_Item *item;
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(!wd) return;

	EINA_LIST_FOREACH(wd->items, elist, item)
	   {
			if(!item->separator)
				evas_object_size_hint_align_set( item->base, align_x, align_y );
	   }

	wd->align_x = align_x;
	wd->align_y = align_y;
}


EAPI void
elm_ctxpopup_align_get(Evas_Object *obj, double *align_x, double *align_y)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
	if(!wd) return;
	if( align_x ) *align_x = wd->align_x;
	if( align_y ) *align_y = wd->align_y;
}

*/


