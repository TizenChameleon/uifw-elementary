#ifndef ELM_MODULE_PRIV_H
#define ELM_MODULE_PRIV_H

typedef struct _Elm_Entry_Extension_data Elm_Entry_Extension_data;
typedef void (*cpfunc)(void *data, Evas_Object *obj, void *event_info);

struct _Elm_Entry_Extension_data
{
	Evas_Object *popup;
	Evas_Object *ent;
	Evas_Object *caller;
	Evas_Coord_Rectangle viewport_rect;
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
	Elm_Cnp_Mode cnp_mode : 2;
};

void elm_entry_extension_module_data_get(Evas_Object *obj,Elm_Entry_Extension_data *ext_mod);

#endif
