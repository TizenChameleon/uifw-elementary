#ifndef ELM_MODULE_PRIV_H
#define ELM_MODULE_PRIV_H

typedef struct _Elm_Entry_Extension_data Elm_Entry_Extension_data;
typedef struct _Elm_Entry_Context_Menu_Item Elm_Entry_Context_Menu_Item;
typedef void (*cpfunc)(void *data, Evas_Object *obj, void *event_info);

struct _Elm_Entry_Extension_data
{
	Evas_Object *popup;
	Evas_Object *ent;
	Ecore_Timer *longpress_timer;
	Eina_List *items;
	cpfunc select;
	cpfunc copy;
	cpfunc cut;
	cpfunc paste;
	cpfunc cancel;
	cpfunc selectall;
	cpfunc cnpinit;
	Eina_Bool password :1;
	Eina_Bool editable :1;
	Eina_Bool have_selection: 1;
	Eina_Bool selmode :1;
	Eina_Bool context_menu : 1;
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

EAPI void elm_entry_extension_module_data_get(Evas_Object *obj,Elm_Entry_Extension_data *ext_mod);

#endif
