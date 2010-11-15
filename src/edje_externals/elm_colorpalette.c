

#include "private.h"

typedef struct _Elm_Params_colorpalette
{
   unsigned int row, col;
   Elm_Colorpalette_Color *color;
   char *color_set ;
   int color_num ;

} Elm_Params_colorpalette;

static void
external_colorpalette_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_colorpalette *p;
   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;
   if((p->row > 0)&&( p->col > 0))
	   elm_colorpalette_row_column_set(obj,p->row,p->col) ;
   if(p->color_num)
   elm_colorpalette_color_set(obj,p->color_num, p->color) ;
}

static Eina_Bool
external_colorpalette_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "row"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
		//No EAPI for row set
	     return EINA_FALSE;
	  }
     }
   else if (!strcmp(param->name, "col"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
		//No EAPI for column set
		return EINA_FALSE;
	  }
     }
   else if (!strcmp(param->name,"color_num"))
     {
   	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
   	  {
		//No EAPI for colour_number set
		return EINA_FALSE;
   	  }
        }
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_colorpalette_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
	return EINA_FALSE;
   //FIX ME:getter functions not provided
}

static void *
external_colorpalette_params_parse(void *data __UNUSED__, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_colorpalette *mem;
   Edje_External_Param *param;
   const Eina_List *l;
   int k,m,ll ;
   k = m = ll = 0;
   char test[5] ;
   int d = 0;
   mem = calloc(1, sizeof(Elm_Params_colorpalette));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "row"))
          {
             mem->row = param->i;
          }
       else if (!strcmp(param->name, "col"))
	     {
	        mem->col = param->i;
	     }
	   else if (!strcmp(param->name, "color_num"))
	     {
	        mem->color_num = param->i ;
	     }

	   else if (!strcmp(param->name, "color_set"))
         {
           mem->color = (Elm_Colorpalette_Color*) calloc (mem->color_num, sizeof(Elm_Colorpalette_Color));
           char *s = NULL ;
           s = (char*)param->i;
           while(k <(mem->color_num) && (ll <= (char*)strlen(param->i)))
             {
                  if(d == 0)
                   {
            	      while(s[ll]!=':')
            		    test[m++] = s[ll++];
            	      test[m]= 0 ;
            	      ll++ ;
		      m = 0 ;
            	      mem->color[k].r = atoi(test) ;
            	      d++ ;
                   }
                  if(d == 1)
                   {
            	     while(s[ll]!=':')
            		   test[m++] = s[ll++];
            	     test[m]= 0 ;
		     ll++ ;
                     m = 0 ;
                     mem->color[k].g = atoi(test) ;
                     d++ ;
                   }
                  if(d == 2)
                   {
            	      while(s[ll]!='/' &&  m<3)
            		    test[m++] = s[ll++];
            	      test[m]= 0 ;
		      ll++ ;
                      m = 0 ;
                      mem->color[k].b = atoi(test) ;
                      d = 0 ;
                   }
                }
           k++;
         }
       }
    return mem;
}

static Evas_Object *external_colorpalette_content_get(void *data __UNUSED__,
		const Evas_Object *obj __UNUSED__, const char *content __UNUSED__)
{
	ERR("No content.");
	return NULL;
}

static void
external_colorpalette_params_free(void *params)
{
 	Elm_Params_colorpalette *mem = params;
    if(mem->color)free(mem->color);
 	if(mem->color_set)free(mem->color_set) ;
 	if(mem)free(mem);
}

static Edje_External_Param_Info external_colorpalette_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_INT("row"),
   EDJE_EXTERNAL_PARAM_INFO_INT("col"),
   EDJE_EXTERNAL_PARAM_INFO_INT("r"),
   EDJE_EXTERNAL_PARAM_INFO_INT("g"),
   EDJE_EXTERNAL_PARAM_INFO_INT("b"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("color_set"),
   EDJE_EXTERNAL_PARAM_INFO_INT("color_num"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(colorpalette, "colorpalette");
DEFINE_EXTERNAL_TYPE_SIMPLE(colorpalette, "colorpalette");
