#include <string.h>

#include "private.h"

typedef struct _Elm_Params_Toggle
{
   Elm_Params base;
   const char *label;
   Evas_Object *icon;
   const char *on, *off;
   Eina_Bool state:1;
   Eina_Bool state_exists:1;
} Elm_Params_Toggle;

static void
external_toggle_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Toggle *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_toggle_icon_set(obj, p->icon);

   if ((p->on) && (p->off))
     elm_toggle_states_labels_set(obj, p->on, p->off);
   else if ((p->on) || (p->off))
     {
	const char *on, *off;
	elm_toggle_states_labels_get(obj, &on, &off);
	if (p->on)
	  elm_toggle_states_labels_set(obj, p->on, off);
	else
	  elm_toggle_states_labels_set(obj, on, p->off);
     }

   if (p->state_exists)
     elm_toggle_state_set(obj, p->state);
}

static Eina_Bool
external_toggle_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_object_text_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if ((strcmp(param->s, "")) && (!icon)) return EINA_FALSE;
	     elm_toggle_icon_set(obj, icon);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "label on"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     const char *on, *off;
	     elm_toggle_states_labels_get(obj, &on, &off);
	     elm_toggle_states_labels_set(obj, param->s, off);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "label off"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     const char *on, *off;
	     elm_toggle_states_labels_get(obj, &on, &off);
	     elm_toggle_states_labels_set(obj, on, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "state"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_toggle_state_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_toggle_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_object_text_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "label on"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     const char *on, *off;
	     elm_toggle_states_labels_get(obj, &on, &off);
	     param->s = on;
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "label off"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     const char *on, *off;
	     elm_toggle_states_labels_get(obj, &on, &off);
	     param->s = off;
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "state"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_toggle_state_get(obj);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_toggle_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Toggle *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Toggle));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "state")) {
	   mem->state = param->i;
	   mem->state_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "label on"))
	  mem->on = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "label off"))
	  mem->off = eina_stringshare_add(param->s);
     }

   return mem;
}

static Evas_Object *external_toggle_content_get(void *data __UNUSED__,
		const Evas_Object *obj __UNUSED__, const char *content __UNUSED__)
{
	ERR("No content.");
	return NULL;
}

static void
external_toggle_params_free(void *params)
{
   Elm_Params_Toggle *mem = params;

   if (mem->on)
     eina_stringshare_del(mem->on);
   if (mem->off)
     eina_stringshare_del(mem->off);
   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

static Edje_External_Param_Info external_toggle_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("label on", "ON"),
   EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("label off", "OFF"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("state"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(toggle, "toggle")
DEFINE_EXTERNAL_TYPE_SIMPLE(toggle, "Toggle")
