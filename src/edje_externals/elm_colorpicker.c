/*
 * elm_colorpicker.c
 *
 *  Created on: Nov 12, 2010
 *      Author: Amit
 */


#include "private.h"

typedef struct _Elm_Params_colorpicker
{
   unsigned int r, g, b;
} Elm_Params_colorpicker ;

static void
external_colorpicker_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_colorpicker *p;
   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if((p->r >= 0)&&( p->g >= 0)&& (p->b >= 0))
   elm_colorpicker_color_set(obj, p->r, p->g, p->b) ;
}

static Eina_Bool
external_colorpicker_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
	//Cannot set particular parameter
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));
   return EINA_FALSE;
}

static Eina_Bool
external_colorpicker_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "r"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_colorpicker_color_get( obj,&(param->i),NULL, NULL) ;
	     return EINA_TRUE;
	  }
     }

   if (!strcmp(param->name, "g"))
       {
  	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
  	  {
  	     elm_colorpicker_color_get( obj,NULL,&(param->i), NULL) ;
  	     return EINA_TRUE;
  	  }
       }
   if (!strcmp(param->name, "b"))
         {
    	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
    	  {
    	     elm_colorpicker_color_get( obj,NULL, NULL,&(param->i)) ;
    	     return EINA_TRUE;
    	  }
         }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_colorpicker_params_parse(void *data __UNUSED__, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_colorpicker *mem;
   Edje_External_Param *param;
   const Eina_List *l;
   mem = calloc(1, sizeof(Elm_Params_colorpicker));
	   if (!mem)
	     return NULL;

	   EINA_LIST_FOREACH(params, l, param)
	     {
		if (!strcmp(param->name, "r"))
		  {
		     mem->r = param->i;
		  }
		else if (!strcmp(param->name, "g"))
		  {
		     mem->g = param->i;
		  }
		else if (!strcmp(param->name, "b"))
		  {
		     mem->b = param->i;
		  }

	     }

	   return mem;
	}

static Evas_Object *external_colorpicker_content_get(void *data __UNUSED__,
		const Evas_Object *obj __UNUSED__, const char *content __UNUSED__)
{
	ERR("No content.");
	return NULL;
}

static void
external_colorpicker_params_free(void *params)
{
	Elm_Params_colorpicker *mem = params;

   free(mem);
}

static Edje_External_Param_Info external_colorpicker_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_INT("r"),
   EDJE_EXTERNAL_PARAM_INFO_INT("g"),
   EDJE_EXTERNAL_PARAM_INFO_INT("b"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(colorpicker, "colorpicker");
DEFINE_EXTERNAL_TYPE_SIMPLE(colorpicker, "colorpicker");

