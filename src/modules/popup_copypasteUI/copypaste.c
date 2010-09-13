#include <Elementary.h>
#include "elm_module_priv.h"
#include "elm_priv.h"

Elm_Entry_Extension_data *ext_mod;

static void
_select(void *data, Evas_Object *obj, void *event_info)
{
   ext_mod->select(data,obj,event_info);
   evas_object_hide(ext_mod->popup);
}

static void
_paste(void *data, Evas_Object *obj, void *event_info)
{
 	ext_mod->paste(data,obj,event_info);
   evas_object_hide(ext_mod->popup);
}

static void
_cut(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->cut(data,obj,event_info);
	evas_object_hide(ext_mod->popup);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->copy(data,obj,event_info);
	evas_object_hide(ext_mod->popup);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->cancel(data,obj,event_info);
   evas_object_hide(ext_mod->popup);
}

static void
_item_clicked(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

	if (it->func) it->func(it->data, obj2, NULL);
	evas_object_hide(ext_mod->popup);
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
	if(!ext_mod)
		{
			ext_mod = ELM_NEW(Elm_Entry_Extension_data);
  		elm_entry_extension_module_data_get(obj,ext_mod);
		}
}

EAPI void
obj_unhook(Evas_Object *obj)
{
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
	Evas_Object *list;
	
	const Eina_List *l;
	const Elm_Entry_Context_Menu_Item *it;
	/*update*/
	elm_entry_extension_module_data_get(obj,ext_mod);
	if (ext_mod->context_menu)
	{
		if (ext_mod->popup) evas_object_del(ext_mod->popup);
        else elm_widget_scroll_freeze_push(obj);
		top = elm_widget_top_get(obj);
		if(top)
		ext_mod->popup = elm_popup_add(top);
		elm_object_style_set(ext_mod->popup,"menustyle");
		elm_popup_set_mode(ext_mod->popup, ELM_POPUP_TYPE_ALERT);
		elm_popup_title_label_set(ext_mod->popup,"CopyPaste");
		list = elm_list_add(ext_mod->popup);
		elm_object_style_set(list,"popup");
		elm_list_horizontal_mode_set(list, ELM_LIST_COMPRESS);
		elm_widget_sub_object_add(obj, ext_mod->popup);
		if (!ext_mod->selmode)
		{	
			if (!ext_mod->password)
				elm_list_item_append(list, "Select", NULL, NULL,_select, obj);
			if (1) // need way to detect if someone has a selection
				{
					if (ext_mod->editable)
						elm_list_item_append(list, "Paste", NULL, NULL,_paste, obj);
				}
	//		elm_ctxpopup_item_add(wd->ctxpopup, NULL, "Selectall",_select_all, obj );
		}
		else
		{
			  if (!ext_mod->password)
				{
					if (ext_mod->have_selection)
						{
							elm_list_item_append(list, "Copy", NULL, NULL,_copy, obj);
							if (ext_mod->editable)
								elm_list_item_append(list, "Cut", NULL, NULL,_cut, obj);
						}
					else
						{
							_cancel(obj,ext_mod->popup,NULL);		
							elm_list_item_append(list, "Select", NULL, NULL,_select, obj);
							if (1) // need way to detect if someone has a selection
								{
									if (ext_mod->editable)
										elm_list_item_append(list, "Paste", NULL, NULL,_paste, obj);
								}
						}
				}
		}
		EINA_LIST_FOREACH(ext_mod->items, l, it)
		{
			elm_list_item_append(list, it->label,NULL,NULL, _item_clicked, it);
		}
		if (ext_mod->popup)
		{
			elm_list_go(list);
			elm_popup_content_set(ext_mod->popup, list);
			evas_object_show(ext_mod->popup);	       
			evas_render( evas_object_evas_get( ext_mod->popup ) );
		}
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
}

