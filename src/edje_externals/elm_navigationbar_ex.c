#include <assert.h>

#include "private.h"

typedef struct _Elm_Params_Navigationbar_ex
{
   Eina_Bool del_on_pop_exists:1;
   Eina_Bool del_on_pop:1;
} Elm_Params_Navigationbar_ex;


static void
external_navigationbar_ex_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Navigationbar_ex *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

  if(p->del_on_pop_exists) elm_navigationbar_ex_delete_on_pop_set(obj, p->del_on_pop);
}

static Eina_Bool
external_navigationbar_ex_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "delete on pop"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_navigationbar_ex_delete_on_pop_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_navigationbar_ex_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_navigationbar_ex_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Navigationbar_ex *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Navigationbar_ex));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "delete on pop"))
		{
	     mem->del_on_pop = !!param->i;
		  mem->del_on_pop_exists = EINA_TRUE;
		}
     }

   return mem;
}

static Evas_Object *external_navigationbar_ex_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_navigationbar_ex_params_free(void *params)
{
   Elm_Params_Navigationbar_ex* mem = params;
   free(mem);
}

static Edje_External_Param_Info external_navigationbar_ex_params[] = {
   EDJE_EXTERNAL_PARAM_INFO_BOOL("delete on pop"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(navigationbar_ex, "navigationbar_ex");
DEFINE_EXTERNAL_TYPE_SIMPLE(navigationbar_ex, "Navigationbar_ex");
