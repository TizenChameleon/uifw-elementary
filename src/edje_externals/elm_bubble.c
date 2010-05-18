#include "private.h"

typedef struct _Elm_Params_Bubble
{
   Elm_Params base;
   Evas_Object *icon;
   const char *info;
   Evas_Object *content; /* part name whose obj is to be set as content */
} Elm_Params_Bubble;

static void
external_bubble_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Bubble *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->base.label) elm_bubble_label_set(obj, p->base.label);
   if (p->icon) elm_bubble_icon_set(obj, p->icon);
   if (p->info) elm_bubble_info_set(obj, p->info);
   if (p->content) elm_bubble_content_set(obj, p->content);
}

static Eina_Bool
external_bubble_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_bubble_label_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if (icon)
	       {
		  elm_bubble_icon_set(obj, icon);
		  return EINA_TRUE;
	       }
	  }
     }
   else if (!strcmp(param->name, "info"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_bubble_info_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "content"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *content = \
	       external_common_param_edje_object_get(obj, param);
	     if (content)
	       {
		  elm_bubble_content_set(obj, content);
		  return EINA_TRUE;
	       }
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_bubble_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_bubble_label_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "info"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_bubble_info_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "content"))
     {
	/* not easy to get content name back from live object */
	return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_bubble_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Bubble *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = external_common_params_parse(Elm_Params_Bubble, data, obj, params);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "info"))
	  mem->info = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "content"))
	  mem->content = external_common_param_edje_object_get(obj, param);
     }

   return mem;
}

 static void
external_bubble_params_free(void *params)
{
   Elm_Params_Bubble *mem = params;

   if (mem->icon)
     evas_object_del(mem->icon);
   if (mem->content)
     evas_object_del(mem->content);
   if (mem->info)
     eina_stringshare_del(mem->info);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_bubble_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("info"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("content"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(bubble, "bubble");
DEFINE_EXTERNAL_TYPE_SIMPLE(bubble, "Bubble");
