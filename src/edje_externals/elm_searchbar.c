#include "private.h"

typedef struct _Elm_Params_Searchbar
{
   Elm_Params base;
   Eina_Bool cancel_button_exists:1;
   Eina_Bool cancel_button_visible:1;
   Eina_Bool cancel_button_animation:1;
   Eina_Bool cancel_button_animation_exists:1;
   Eina_Bool boundary_rect_exists:1;
   Eina_Bool boundary_rect:1;
   const char *text;
} Elm_Params_Searchbar;

static void
external_searchbar_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Searchbar *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->cancel_button_exists)
      elm_searchbar_cancel_button_set(obj, p->cancel_button_visible);
   if (p->cancel_button_animation_exists)
      elm_searchbar_cancel_button_animation_set(obj, p->cancel_button_animation);
   if (p->boundary_rect_exists)
      elm_searchbar_boundary_rect_set(obj, p->boundary_rect);
   if (p->text)
     elm_object_text_set(obj, p->text);
}

static Eina_Bool
external_searchbar_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "cancel_button_visible"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_searchbar_cancel_button_set(obj, param->i);
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
   else if (!strcmp(param->name, "boundary_rect"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_searchbar_boundary_rect_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "text"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_searchbar_param_get(void *data __UNUSED__, const Evas_Object *obj __UNUSED__, Edje_External_Param *param)
{
   if (!strcmp(param->name, "cancel_button_visible"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             return param->i;
          }
     }
   else if (!strcmp(param->name, "cancel_button_animation"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             return param->i;
          }
     }
   else if (!strcmp(param->name, "boundary_rect"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             return param->i;
          }
     }
   else if (!strcmp(param->name, "text"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_text_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_searchbar_params_parse(void *data __UNUSED__,
                                Evas_Object *obj __UNUSED__,
                                const Eina_List *params)
{
   Elm_Params_Searchbar *mem = NULL;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Searchbar));
   if (!mem)
      return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "cancel_button_visible"))
         {
            mem->cancel_button_visible = !!param->i;
            mem->cancel_button_exists = EINA_TRUE;
         }
        if (!strcmp(param->name, "cancel_button_animation"))
          {
             mem->cancel_button_animation = !!param->i;
             mem->cancel_button_animation_exists = EINA_TRUE;
          }
        if (!strcmp(param->name, "boundary_rect"))
          {
             mem->boundary_rect = !!param->i;
             mem->boundary_rect_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "text"))
          {
             mem->text = eina_stringshare_add(param->s);
          }
     }

   return mem;
}

static Evas_Object *external_searchbar_content_get(void *data __UNUSED__,
		const Evas_Object *obj __UNUSED__,
		const char *content __UNUSED__)
{
	ERR("so content");
	return NULL;
}

static void
external_searchbar_params_free(void *params)
{
   Elm_Params_Searchbar *mem = params;
   if (mem->text)
     eina_stringshare_del(mem->text);
   free(params);
}

static Edje_External_Param_Info external_searchbar_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_BOOL("cancel_button_visible"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("cancel_button_animation"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("boundary_rect"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("text"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(searchbar, "searchbar");
DEFINE_EXTERNAL_TYPE_SIMPLE(searchbar, "Searchbar");
