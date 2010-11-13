#include "private.h"

typedef struct _Elm_Params_Searchbar
{
   Elm_Params base;
   Eina_Bool cancel_button_animation:1;
   Eina_Bool cancel_button_animation_exists:1;
} Elm_Params_Searchbar;

static void
external_searchbar_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Searchbar *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->base.label)
      elm_searchbar_text_set(obj, p->base.label);
   if (p->cancel_button_animation_exists)
      elm_searchbar_cancel_button_animation_set(obj, p->cancel_button_animation);
}

static Eina_Bool
external_searchbar_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_searchbar_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "cancel_button_animation"))
     {
     	 if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     	   {
     	      elm_searchbar_cancel_button_animation_set(obj, param->i);
            return EINA_TRUE;
         }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_searchbar_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_searchbar_text_get((Evas_Object *)obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "cancel_button_animation"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             return param->i;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_searchbar_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Searchbar *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = external_common_params_parse(Elm_Params_Searchbar, data, obj, params);
   if (!mem)
      return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "cancel_button_animation"))
          {
             mem->cancel_button_animation = param->i;
             mem->cancel_button_animation_exists = EINA_TRUE;
          }
     }

   return mem;
}

static Evas_Object *external_searchbar_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_searchbar_params_free(void *params)
{
   external_common_params_free(params);
}

static Edje_External_Param_Info external_searchbar_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_BOOL("cancel_button_animation"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(searchbar, "searchbar");
DEFINE_EXTERNAL_TYPE_SIMPLE(searchbar, "Searchbar");
