#include "private.h"

typedef struct _Elm_Params_Nocontents
{
   Elm_Params base;
}Elm_Params_Nocontents;

static void
external_nocontents_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Nocontents *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->base.label) elm_nocontents_label_set(obj, p->base.label);
}

static Eina_Bool
external_nocontents_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
		elm_nocontents_label_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_nocontents_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_nocontents_label_get(obj);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_nocontents_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Nocontents *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = external_common_params_parse(Elm_Params_Nocontents, data, obj, params);
   if (!mem)
     return NULL;

   return mem;
}

static Evas_Object *external_nocontents_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_nocontents_params_free(void *params)
{
Elm_Params_Nocontents *mem = params;
   external_common_params_free(params);
}

static Edje_External_Param_Info external_nocontents_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(nocontents, "nocontents");
DEFINE_EXTERNAL_TYPE_SIMPLE(nocontents, "Nocontents");
