#include <Elementary.h>
#include "elm_module_priv.h"
#include "elm_priv.h"

Elm_Entry_Extension_data *ext_mod;

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
_select_all(void *data, Evas_Object *obj, void *event_info)
{
   ext_mod->selectall(data,obj,event_info);
   evas_object_hide(obj);
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
	elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->copy(data,obj,event_info);
	evas_object_hide(obj);
	elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_share(void *data, Evas_Object *obj, void *event_info)
{
	char *str = NULL;
	Ecore_X_Atom atom;

	str = elm_entry_markup_to_utf8(elm_entry_selection_get(data));
        if (!str) return;

        atom = ecore_x_atom_get("_TEXT_SHARE_TEST");
        if (atom)
          {
             ecore_x_window_prop_string_set(ecore_x_window_root_first_get(), atom, str);
             ecore_x_client_message32_send(ecore_x_window_root_first_get(), 
                                           atom, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                           0, 0, 0, 0, 0);
          }
        free(str);

	evas_object_hide(obj);
	elm_ctxpopup_scroller_disabled_set(ext_mod->popup, EINA_FALSE);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
	ext_mod->cancel(data,obj,event_info);
	evas_object_hide(obj);
	elm_object_scroll_freeze_pop(ext_mod->popup);
}

static void
_clipboard_menu(void *data, Evas_Object *obj, void *event_info)
{
	// start for cbhm
	ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), "",1);
	ext_mod->cnpinit(data,obj,event_info);
	elm_cbhm_helper_init(obj);
	if (ext_mod->textonly)
		elm_cbhm_send_raw_data("show0");
	else
		elm_cbhm_send_raw_data("show1");
	evas_object_hide(obj);
	// end for cbhm
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
	const Eina_List *l;
	const Elm_Entry_Context_Menu_Item *it;
	const char *context_menu_orientation;
	char buf[255];
	Evas_Object* icon;

	/*update*/
	elm_entry_extension_module_data_get(obj,ext_mod);
	if (ext_mod->context_menu)
	{
		if (ext_mod->popup) evas_object_del(ext_mod->popup);
//      	else elm_widget_scroll_freeze_push(obj);
		top = elm_widget_top_get(obj);
		if(top)
			ext_mod->popup = elm_ctxpopup_add(top);
		/*currently below theme not used,when guideline comes a new theme can be created if required*/
		elm_object_style_set(ext_mod->popup,"extended/entry");
		elm_object_scroll_freeze_push(ext_mod->popup);
		context_menu_orientation = edje_object_data_get
		(ext_mod->ent, "context_menu_orientation");
		if ((context_menu_orientation) &&
		(!strcmp(context_menu_orientation, "horizontal")))
		elm_ctxpopup_horizontal_set(ext_mod->popup, EINA_TRUE);

		elm_widget_sub_object_add(obj, ext_mod->popup);
		if (!ext_mod->selmode)
		{	
			if (!ext_mod->password)
			{
				elm_ctxpopup_item_append(ext_mod->popup, "Select", NULL, _select, obj );
				elm_ctxpopup_item_append(ext_mod->popup, "Select All", NULL, _select_all, obj );
			}
			if (1) // need way to detect if someone has a selection
				{
					if (ext_mod->editable)
						elm_ctxpopup_item_append(ext_mod->popup, "Paste", NULL, _paste, obj );
				}
	//		elm_ctxpopup_item_append(wd->ctxpopup, NULL, "Selectall",_select_all, obj );
	// start for cbhm
			if (!ext_mod->password)
			{
				icon = elm_icon_add(ext_mod->popup);
				snprintf(buf, sizeof(buf), "%s/images/copypaste_icon_clipboard.png", PACKAGE_DATA_DIR);
				elm_icon_file_set(icon, buf, NULL);
				elm_ctxpopup_item_append(ext_mod->popup, NULL, icon, _clipboard_menu, obj);
				//elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
			}
	// end for cbhm
		}
		else
		{
			  if (!ext_mod->password)
				{
					if (ext_mod->have_selection)
						{
							elm_ctxpopup_item_append(ext_mod->popup, "Copy", NULL, _copy, obj );
							if (ext_mod->editable)
								elm_ctxpopup_item_append(ext_mod->popup, "Cut", NULL, _cut, obj );							
							if (ext_mod->editable)
								elm_ctxpopup_item_append(ext_mod->popup, "Paste", NULL, _paste, obj );
							icon = elm_icon_add(ext_mod->popup);
							snprintf(buf, sizeof(buf), "%s/images/copypaste_icon_share.png", PACKAGE_DATA_DIR);
							elm_icon_file_set(icon, buf, NULL);
							elm_ctxpopup_item_append(ext_mod->popup, NULL, icon, _clipboard_menu, obj);
							//elm_ctxpopup_item_append(ext_mod->popup, "Share", NULL, _share, obj);
						}
					else
						{
							_cancel(obj,ext_mod->popup,NULL);		
							elm_ctxpopup_item_append(ext_mod->popup, "Select", NULL, _select, obj );
							elm_ctxpopup_item_append(ext_mod->popup, "Select All", NULL, _select_all, obj );
							if (1) // need way to detect if someone has a selection
								{
									if (ext_mod->editable)
										elm_ctxpopup_item_append(ext_mod->popup, "Paste", NULL, _paste, obj );
								}
						}
	// start for cbhm
					icon = elm_icon_add(ext_mod->popup);
					snprintf(buf, sizeof(buf), "%s/images/copypaste_icon_clipboard.png", PACKAGE_DATA_DIR);
					elm_icon_file_set(icon, buf, NULL);
					elm_ctxpopup_item_append(ext_mod->popup, NULL, icon, _clipboard_menu, obj);
					//elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
	// end for cbhm
				}
		}
		EINA_LIST_FOREACH(ext_mod->items, l, it)
		{
			elm_ctxpopup_item_append(ext_mod->popup, it->label, NULL, _item_clicked, it );
		}
		if (ext_mod->popup)
			{
				_ctxpopup_position(obj);
				evas_object_show(ext_mod->popup);	          
			}
		}
	ext_mod->longpress_timer = NULL;
	}

EAPI void
obj_mouseup(Evas_Object *obj)
{
	if (!obj || !ext_mod) {
		return;
	}

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


EAPI void
obj_hidemenu(Evas_Object *obj)
{
	if (!obj || !ext_mod) {
		return;
	}

	evas_object_hide(ext_mod->popup);
//	if (ext_mod->popup) evas_object_del(ext_mod->popup);
}

