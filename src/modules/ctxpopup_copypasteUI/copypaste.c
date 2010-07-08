#include <Elementary.h>
#include "elm_module_priv.h"
#include "elm_priv.h"

Elm_Entry_Extension_data *ext_mod;

static void
_ctxpopup_position(Evas_Object *obj)
{
   Evas_Coord cx, cy, cw, ch, x, y, mw, mh;
   evas_object_geometry_get(ext_mod->ent, &x, &y, NULL, NULL);
   edje_object_part_text_cursor_geometry_get(ext_mod->ent, "elm.text",
					     &cx, &cy, &cw, &ch);
   evas_object_size_hint_min_get(ext_mod->popup, &mw, &mh);
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
   evas_object_move(ext_mod->popup, x + cx, y + cy);
   evas_object_resize(ext_mod->popup, cw, ch);
}

static void
_select(void *data, Evas_Object *obj, void *event_info)
{
   ext_mod->select(data,obj,event_info);
   evas_object_hide(obj);
}

static void
_paste(void *data, Evas_Object *obj, void *event_info)
{
 	ext_mod->paste(data,obj,event_info);
   evas_object_hide(obj);
}

static void
_cut(void *data, Evas_Object *obj, void *event_info)
{
   ext_mod->cut(data,obj,event_info);
	evas_object_hide(obj);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->copy(data,obj,event_info);
	evas_object_hide(obj);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->cancel(data,obj,event_info);
   evas_object_hide(obj);
}

static void
_item_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

	if (it->func) it->func(it->data, obj2, NULL);
	evas_object_hide(obj);
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
	ext_mod = ELM_NEW(Elm_Entry_Extension_data);
  	elm_entry_extension_module_data_get(obj,ext_mod);
}

EAPI void
obj_unhook(Evas_Object *obj)
{
   if(ext_mod)
	 	free(ext_mod);
}

EAPI void
obj_longpress(Evas_Object *obj)
{		
	if(!ext_mod) return;
	Evas_Object *top;
	const Eina_List *l;
	const Elm_Entry_Context_Menu_Item *it;
	const char *context_menu_orientation;

	/*update*/
	elm_entry_extension_module_data_get(obj,ext_mod);

	if (ext_mod->popup) evas_object_del(ext_mod->popup);
       else elm_widget_scroll_freeze_push(obj);
	top = elm_widget_top_get(obj);
	if(top)
		ext_mod->popup = elm_ctxpopup_add(top);
	elm_object_style_set(ext_mod->popup,"entry");
	elm_ctxpopup_scroller_disabled_set(ext_mod->popup, EINA_TRUE);
	context_menu_orientation = edje_object_data_get
	(ext_mod->ent, "context_menu_orientation");
	if ((context_menu_orientation) &&
	(!strcmp(context_menu_orientation, "horizontal")))
		elm_ctxpopup_horizontal_set(ext_mod->popup, EINA_TRUE);

	elm_widget_sub_object_add(obj, ext_mod->popup);
	if (!ext_mod->selmode)
		{	
			if (!ext_mod->password)
				elm_ctxpopup_label_add(ext_mod->popup, "Select",_select, obj );
			if (1) // need way to detect if someone has a selection
				{
					if (ext_mod->editable)
						elm_ctxpopup_label_add(ext_mod->popup, "Paste",	_paste, obj );
				}
	//		elm_ctxpopup_label_add(wd->ctxpopup, "Selectall",_select_all, obj );
		}
	else
		{
			  if (!ext_mod->password)
				{
					if (ext_mod->have_selection)
						{
							elm_ctxpopup_label_add(ext_mod->popup, "Copy",_copy, obj );
							if (ext_mod->editable)
								elm_ctxpopup_label_add(ext_mod->popup, "Cut",_cut, obj );							
						}
					else
						{
							_cancel(obj,ext_mod->popup,NULL);		
							elm_ctxpopup_label_add(ext_mod->popup, "Select",_select, obj );
							if (1) // need way to detect if someone has a selection
								{
									if (ext_mod->editable)
										elm_ctxpopup_label_add(ext_mod->popup, "Paste",	_paste, obj );
								}
						}
				}
		}
		EINA_LIST_FOREACH(ext_mod->items, l, it)
		{
			elm_ctxpopup_label_add(ext_mod->popup, it->label,_item_clicked, it );
		}
	if (ext_mod->popup)
		{
			_ctxpopup_position(obj);
			evas_object_show(ext_mod->popup);	          
		}
		ext_mod->longpress_timer = NULL;
	}

EAPI void
obj_mouseup(Evas_Object *obj)
{
/*update*/
	elm_entry_extension_module_data_get(obj,ext_mod);
   if (ext_mod->longpress_timer)
     {    
		if (ext_mod->have_selection )
			{				
				_cancel(obj,ext_mod->popup,NULL);
			}
     }     
  else
   {
   		if (ext_mod->have_selection )
   			{
				obj_longpress( obj );
   			}
   }
}


