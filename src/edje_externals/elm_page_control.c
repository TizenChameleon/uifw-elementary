#include <assert.h>

#include "private.h"

typedef struct _Elm_Params_Icon
{
   Eina_Bool page_count_exists;
   unsigned int page_count;
   Eina_Bool page_id_exists;
   unsigned int page_id;
} Elm_Params_Icon;

static unsigned int page_count_bk;

static void
external_page_control_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Icon *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->page_count_exists)
     {
        elm_page_control_page_count_set(obj, p->page_count);
	page_count_bk = p->page_count;
     }
   if (p->page_id_exists)
     elm_page_control_page_id_set(obj, p->page_id-1);
}

static Eina_Bool
external_page_control_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "page count")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	elm_page_control_page_count_set(obj, param->i);
	page_count_bk = param->i;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "page id")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	elm_page_control_page_id_set(obj, param->i-1);
	return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_page_control_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "page count")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
        param->i = page_count_bk;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "page id")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	param->i = elm_page_control_page_id_get((Evas_Object *)obj);
	return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
		   param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_page_control_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Icon *mem;
   Edje_External_Param *param;
   const Eina_List *l;
  
   mem = calloc(1, sizeof(Elm_Params_Icon));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "page count"))
	  {
	     mem->page_count = param->i;
	     mem->page_count_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "page id"))
	  {
	     mem->page_id = param->i;
	     mem->page_id_exists = EINA_TRUE;
	  }
     }
   
   return mem;
}

static Evas_Object *external_page_control_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
   ERR("so content");
   return NULL;
}

static void
external_page_control_params_free(void *params)
{
   Elm_Params_Icon *mem = params;

   free(mem);
}

static Edje_External_Param_Info external_page_control_params[] = {
    EDJE_EXTERNAL_PARAM_INFO_INT("page count"),
    EDJE_EXTERNAL_PARAM_INFO_INT("page id"),
    EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(page_control, "page_control");
DEFINE_EXTERNAL_TYPE_SIMPLE(page_control, "Page_control");

