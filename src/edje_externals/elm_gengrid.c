#include <assert.h>

#include "private.h"

typedef struct _Elm_Params_Gengrid
{
   Eina_Bool item_size_x_exists;
   int item_size_x;
   Eina_Bool item_size_y_exists;
   int item_size_y;
   Eina_Bool align_x_exists;
   double align_x;
   Eina_Bool align_y_exists;
   double align_y;
   Eina_Bool always_select_exists;
   Eina_Bool always_select : 1;
   Eina_Bool no_select_exists;
   Eina_Bool no_select;
   Eina_Bool multi_select_exists;
   Eina_Bool multi_select : 1;
   Eina_Bool h_bounce_exists;
   Eina_Bool h_bounce : 1;
   Eina_Bool v_bounce_exists;
   Eina_Bool v_bounce : 1;
   Eina_Bool horizontal_exists;
   Eina_Bool horizontal;
} Elm_Params_Gengrid;

static Eina_Bool horizontal_bk;

static void
external_gengrid_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Gengrid *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->item_size_x_exists && p->item_size_y_exists)
     elm_gengrid_item_size_set(obj, p->item_size_x, p->item_size_y);
   else if (p->item_size_x_exists || p->item_size_y_exists)
     {
	Evas_Coord w, h;
	elm_gengrid_item_size_get(obj, &w, &h);
	if (p->item_size_x_exists)
	  elm_gengrid_item_size_set(obj, p->item_size_x, h);
	else
	  elm_gengrid_item_size_set(obj, w, p->item_size_y);
     }
   if (p->align_x_exists && p->align_y_exists)
     elm_gengrid_align_set(obj, p->align_x, p->align_y);
   else if (p->align_x_exists || p->align_y_exists)
     {
        double x, y;
	elm_gengrid_align_get(obj, &x, &y);
	if (p->align_x_exists)
	  elm_gengrid_align_set(obj, p->align_x, y);
	else
	  elm_gengrid_align_set(obj, x, p->align_y);
     }
   if (p->always_select_exists)
     elm_gengrid_always_select_mode_set(obj, p->always_select);
   if (p->no_select_exists)
     elm_gengrid_no_select_mode_set(obj, p->no_select);
   if (p->multi_select_exists)
     elm_gengrid_multi_select_set(obj, p->multi_select);
   if (p->h_bounce_exists && p->v_bounce_exists)
     elm_gengrid_bounce_set(obj, p->h_bounce, p->v_bounce);
   else if (p->h_bounce_exists || p->v_bounce_exists)
     {
        Eina_Bool h, v;
	elm_gengrid_bounce_get(obj, &h, &v);
	if (p->h_bounce_exists)
	  elm_gengrid_bounce_set(obj, p->h_bounce, v);
	else
	  elm_gengrid_bounce_set(obj, h, p->v_bounce);
     }
   if (p->horizontal_exists)
     {
        elm_gengrid_horizontal_set(obj, p->horizontal);
	horizontal_bk = p->horizontal;
     }
}

static Eina_Bool
external_gengrid_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "item size x") 
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	Evas_Coord w, h;
	elm_gengrid_item_size_get(obj, &w, &h);
	elm_gengrid_item_size_set(obj, param->i, h);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "item size y")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
        Evas_Coord w, h;
	elm_gengrid_item_size_get(obj, &w, &h);
	elm_gengrid_item_size_set(obj, w, param->i);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align x")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
	double x, y;
	elm_gengrid_align_get(obj, &x, &y);
	elm_gengrid_align_set(obj, param->d, y);	   
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align y")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
	double x, y;
	elm_gengrid_align_get(obj, &x, &y);
	elm_gengrid_align_set(obj, x, param->d);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "always select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	elm_gengrid_always_select_mode_set(obj, param->i);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "no select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	elm_gengrid_no_select_mode_set(obj, param->i);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "multi select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	elm_gengrid_multi_select_set(obj, param->i);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "h bounce")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	Eina_Bool h, v;
	elm_gengrid_bounce_get(obj, &h, &v);
	elm_gengrid_item_size_set(obj, param->i, v);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "v bounce")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	Eina_Bool h, v;
	elm_gengrid_bounce_get(obj, &h, &v);
	elm_gengrid_item_size_set(obj, h, param->i);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "horizontal")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	elm_gengrid_horizontal_set(obj, param->i);
	horizontal_bk = param->i;
	return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_gengrid_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "item size x")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	Evas_Coord x, y;
	elm_gengrid_item_size_get(obj, &x, &y);
        param->i = x;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "item size y")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
	Evas_Coord x, y;
	elm_gengrid_item_size_get(obj, &x, &y);
	param->i = y;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align x")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
	double x, y;
	elm_gengrid_align_get(obj, &x, &y);
	param->d = x;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align y")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
	double x, y;
	elm_gengrid_align_get(obj, &x, &y);
	param->d = y;
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "always select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	param->i = elm_gengrid_always_select_mode_get(obj);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "no select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	param->i = elm_gengrid_no_select_mode_get(obj);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "multi select")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	param->i = elm_gengrid_multi_select_get(obj);
	return EINA_TRUE;
     }
   else if (!strcmp(param->name, "horizontal")
		   && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
	param->i = horizontal_bk;
	return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
		   param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_gengrid_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Gengrid *mem;
   Edje_External_Param *param;
   const Eina_List *l;
   
   mem = calloc(1, sizeof(Elm_Params_Gengrid));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "item size x"))
	  {
	     mem->item_size_x = param->i;
	     mem->item_size_x_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "item size y"))
	  {
	     mem->item_size_y = param->i;
	     mem->item_size_y_exists = EINA_TRUE;
  	  }
	else if (!strcmp(param->name, "align x"))
	  {
	     mem->align_x = param->d;
	     mem->align_x_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "align y"))
	  {
	     mem->align_y = param->d;
	     mem->align_y_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "always select"))
	  {
	     mem->always_select = param->i;
	     mem->always_select_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "no_select"))
	  {
	     mem->no_select = param->i;
	     mem->no_select_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "multi select"))
	  {
	     mem->multi_select = param->i;
	     mem->multi_select_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "h bounce"))
	  {
	     mem->h_bounce = param->i;
	     mem->h_bounce_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "v bounce"))
	  {
	     mem->v_bounce = param->i;
	     mem->v_bounce_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "horizontal"))
	{
	     mem->horizontal = param->i;
	     horizontal_bk = param->i;
	     mem->horizontal_exists = EINA_TRUE;
	}
     }
   
   return mem;
}

static Evas_Object *external_gengrid_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
   ERR("so content");
   return NULL;
}

static void
external_gengrid_params_free(void *params)
{
   Elm_Params_Gengrid *mem = params;

   free(mem);
}

static Edje_External_Param_Info external_gengrid_params[] = {
    EDJE_EXTERNAL_PARAM_INFO_INT("item size x"),
    EDJE_EXTERNAL_PARAM_INFO_INT("item size y"),
    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align x"),
    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align y"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("always select"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("no select"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("multi select"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("h bounce"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("v bounce"),
    EDJE_EXTERNAL_PARAM_INFO_BOOL("hirizontal"),
    EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(gengrid, "gengrid");
DEFINE_EXTERNAL_TYPE_SIMPLE(gengrid, "Gengrid");

