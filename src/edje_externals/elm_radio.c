#include "private.h"

typedef struct _Elm_Params_Radio
{
   Elm_Params base;
   Evas_Object *icon;
   const char* group_name;
   int value;
   Eina_Bool value_exists:1;
} Elm_Params_Radio;

static void
external_radio_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Radio *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->base.label)
     elm_radio_label_set(obj, p->base.label);
   if (p->icon)
     elm_radio_icon_set(obj, p->icon);
   if (p->value_exists)
     elm_radio_state_value_set(obj, p->value);
   if (p->group_name)
     {
	Evas_Object *ed = evas_object_smart_parent_get(obj);
	Evas_Object *group = edje_object_part_swallow_get(ed, p->group_name);
	elm_radio_group_add(obj, group);
     }
}

static Eina_Bool
external_radio_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_radio_label_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if ((strcmp(param->s, "")) && (!icon)) return EINA_FALSE;
	     elm_radio_icon_set(obj, icon);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "value"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_radio_value_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "group"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *ed = evas_object_smart_parent_get(obj);
	     Evas_Object *group = edje_object_part_swallow_get(ed, param->s);
	     elm_radio_group_add(obj, group);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_radio_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_radio_label_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "value"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     param->i = elm_radio_value_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "group"))
     {
	/* not easy to get group name back from live object */
	return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_radio_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Radio *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = external_common_params_parse(Elm_Params_Radio, data, obj, params);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "group"))
	  mem->group_name = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "value"))
	  {
	     mem->value = param->i;
	     mem->value_exists = EINA_TRUE;
	  }
     }

   return mem;
}

static Evas_Object *external_radio_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_radio_params_free(void *params)
{
   Elm_Params_Radio *mem = params;

   if (mem->group_name)
     eina_stringshare_del(mem->group_name);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_radio_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("group"),
   EDJE_EXTERNAL_PARAM_INFO_INT("value"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(radio, "radio")
DEFINE_EXTERNAL_TYPE_SIMPLE(radio, "Radio")
