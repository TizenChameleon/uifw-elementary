#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Mod_Api Mod_Api;
typedef struct _Widget_Data Widget_Data;
typedef struct _Elm_Entry_Context_Menu_Item Elm_Entry_Context_Menu_Item;
struct _Widget_Data
{
   Evas_Object *ent;
   Evas_Object *popup;/*copy paste UI - elm_popup*/
   Evas_Object *ctxpopup;/*copy paste UI - elm_ctxpopup*/
   Evas_Object *bg;   
   Evas_Object *hoversel;
   Ecore_Job *deferred_recalc_job;
   Ecore_Event_Handler *sel_notify_handler;
   Ecore_Event_Handler *sel_clear_handler;
   Ecore_Timer *longpress_timer;
   const char *cut_sel;
   const char *text;
   Evas_Coord wrap_w;
   Evas_Coord lastw;
   Evas_Coord downx, downy;
   Evas_Coord cx, cy, cw, ch;
   Eina_List *items;
   Eina_List *item_providers;
   Mod_Api *api; // module api if supplied
   int max_no_of_bytes;
   Eina_Bool changed : 1;
   Eina_Bool linewrap : 1;
   Eina_Bool char_linewrap : 1;
   Eina_Bool single_line : 1;
   Eina_Bool password : 1;
   Eina_Bool show_last_character : 1;
   Eina_Bool editable : 1;
   Eina_Bool selection_asked : 1;
   Eina_Bool have_selection : 1;
   Eina_Bool selmode : 1;
   Eina_Bool deferred_cur : 1;
   Eina_Bool disabled : 1;
   Eina_Bool context_menu : 1;
   Eina_Bool bgcolor : 1;
   Eina_Bool ellipsis : 1;
   Eina_Bool autoreturnkey : 1;
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


static void
_store_selection(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *sel = edje_object_part_text_selection_get(wd->ent, "elm.text");
   if (!wd) return;
   if (wd->cut_sel) eina_stringshare_del(wd->cut_sel);
   wd->cut_sel = eina_stringshare_add(sel);
}

static void
_select(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   wd->selmode = EINA_TRUE;
   edje_object_part_text_select_none(wd->ent, "elm.text");
   edje_object_part_text_select_allow_set(wd->ent, "elm.text", 1);
   if (!wd->password)
      edje_object_signal_emit(wd->ent, "elm,state,select,on", "elm");
   elm_widget_scroll_hold_push(data);
    evas_object_hide(wd->popup);
}

static void
_paste(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
    if (!wd) return;
   evas_object_smart_callback_call(data, "selection,paste", NULL);
   if (wd->sel_notify_handler)
     {
#ifdef HAVE_ELEMENTARY_X
	Evas_Object *top;

	top = elm_widget_top_get(data);
	if ((top) && (elm_win_xwindow_get(top)))
	  {
	     ecore_x_selection_primary_request
	       (elm_win_xwindow_get(top),
		ECORE_X_SELECTION_TARGET_UTF8_STRING);
	     wd->selection_asked = EINA_TRUE;
	  }
#endif
     }  
   evas_object_hide(wd->popup);
}

static void
_cut(void *data, Evas_Object *obj, void *event_info)
{	
   Widget_Data *wd = elm_widget_data_get(data);
   wd->selmode = EINA_FALSE;
   edje_object_part_text_select_allow_set(wd->ent, "elm.text", 0);
   edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
   elm_widget_scroll_hold_pop(data);
   _store_selection(data);
   edje_object_part_text_insert(wd->ent, "elm.text", "");
   edje_object_part_text_select_none(wd->ent, "elm.text");
   evas_object_hide(wd->popup);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_FALSE;
   edje_object_part_text_select_allow_set(wd->ent, "elm.text", 0);
   edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
   elm_widget_scroll_hold_pop(data);
   _store_selection(data);
   edje_object_part_text_select_none(wd->ent, "elm.text");
   evas_object_hide(wd->popup);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->selmode = EINA_FALSE;
   edje_object_part_text_select_allow_set(wd->ent, "elm.text", 0);
   edje_object_signal_emit(wd->ent, "elm,state,select,off", "elm");
   elm_widget_scroll_hold_pop(data);
   edje_object_part_text_select_none(wd->ent, "elm.text");
   evas_object_hide(wd->popup);
}

static void
_item_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

   if (it->func) it->func(it->data, obj2, NULL);
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

// module fucns for the specific module type
EAPI void
obj_hook(Evas_Object *obj)
{
  	
}

EAPI void
obj_unhook(Evas_Object *obj)
{
   
}

EAPI void
obj_longpress(Evas_Object *obj)
{
			Widget_Data *wd = elm_widget_data_get(obj);
			Evas_Object *top;
   			const Eina_List *l;
			Evas_Object *list;
   			const Elm_Entry_Context_Menu_Item *it;
   			
			if (wd->popup) evas_object_del(wd->popup);
		       else elm_widget_scroll_freeze_push(obj);
			top = elm_widget_top_get(obj);
			if(top)
				wd->popup = elm_popup_add(top);
			elm_object_style_set(wd->popup,"menustyle");
			elm_popup_set_mode(wd->popup, ELM_POPUP_TYPE_ALERT);
			elm_popup_title_label_set(wd->popup,"CopyPaste");
			list = elm_list_add(wd->popup);
			elm_object_style_set(list,"popup");
			elm_list_horizontal_mode_set(list, ELM_LIST_COMPRESS);
			elm_widget_sub_object_add(obj, wd->popup);
			if (!wd->selmode)
				{	
					if (!wd->password)
						elm_list_item_append(list, "Select", NULL, NULL,_select, obj);

					if (1) // need way to detect if someone has a selection
						{
							if (wd->editable)
								elm_list_item_append(list, "Paste", NULL, NULL,_paste, obj);

						}
						//elm_list_item_append(list, "Selectall", NULL, NULL,_select_all, data);
				}
			else
				{
					  if (!wd->password)
						{
							if (wd->have_selection)
								{
									elm_list_item_append(list, "Copy", NULL, NULL,_copy, obj);
									if (wd->editable)
										elm_list_item_append(list, "Cut", NULL, NULL,_cut, obj);
								}
							else
								{
									_cancel(obj,wd->popup,NULL);							
									if (1) // need way to detect if someone has a selection
										{
											if (wd->editable)
												elm_list_item_append(list, "Paste", NULL, NULL,_paste, obj);
										}
										elm_list_item_append(list, "Select", NULL, NULL,_select, obj);
								}
						}
				}
				EINA_LIST_FOREACH(wd->items, l, it)
				{
					elm_list_item_append(list, it->label, NULL, NULL,_item_clicked, it);
				}
			if (wd->popup)
				{
					elm_list_go(list);
					elm_popup_content_set(wd->popup, list);
					evas_object_show(wd->popup);	
					evas_render( evas_object_evas_get( wd->popup ) );
				}
			wd->longpress_timer = NULL;
	}

EAPI void
obj_mouseup(Evas_Object *obj)
{
  	Widget_Data *wd = elm_widget_data_get(obj);
   if (wd->longpress_timer)
     {    	
		ecore_timer_del(wd->longpress_timer);
		wd->longpress_timer = NULL;
		if (wd->have_selection )
			{				
				_cancel(obj,wd->popup,NULL);
			}
     }     
}


