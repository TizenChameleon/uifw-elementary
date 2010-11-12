#include <assert.h>

#include "private.h"

typedef struct _Elm_Params_Timepicker
{
   Eina_Bool show_am_pm:1;
   Eina_Bool show_am_pm_exists:1;
   Eina_Bool show_seconds:1;
   Eina_Bool show_seconds_exists:1;
} Elm_Params_Timepicker;

static unsigned int page_count_bk;

static void
external_timepicker_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Timepicker *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->show_am_pm_exists)
     elm_timepicker_show_am_pm_set(obj, p->show_am_pm);
   if (p->show_seconds_exists)
     elm_timepicker_show_seconds_set(obj, p->show_seconds);
   
}

static Eina_Bool
external_timepicker_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "show am_pm"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_timepicker_show_am_pm_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "show seconds"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_timepicker_show_seconds_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_timepicker_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{

   ERR("unknown parameter '%s' of type '%s'",
		   param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_timepicker_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Timepicker *mem;
   Edje_External_Param *param;
   const Eina_List *l;
  
   mem = calloc(1, sizeof(Elm_Params_Timepicker));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "show am_pm"))
	  {
	     mem->show_am_pm = !!param->i;
	     mem->show_am_pm_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "show seconds"))
	  {
	     mem->show_seconds = !!param->i;
	     mem->show_seconds_exists = EINA_TRUE;
	  }
     }
   
   return mem;
}

static Evas_Object *external_timepicker_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
   ERR("so content");
   return NULL;
}

static void
external_timepicker_params_free(void *params)
{
   Elm_Params_Timepicker *mem = params;

   free(mem);
}

static Edje_External_Param_Info external_timepicker_params[] = {
    EDJE_EXTERNAL_PARAM_INFO_BOOL("show am_pm"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("show seconds"),
    EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(timepicker, "timepicker");
DEFINE_EXTERNAL_TYPE_SIMPLE(timepicker, "Picker");

