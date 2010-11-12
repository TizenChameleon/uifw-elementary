#include "private.h"

typedef struct _Elm_Params_Datefield
{
   const char *layout;
   const char *format;
   Eina_Bool mode:1;
   Eina_Bool mode_exists:1;
   int year;
   Eina_Bool year_exists:1;
   int mon;
   Eina_Bool mon_exists:1;
   int day;
   Eina_Bool day_exists:1;
   int hour;
   Eina_Bool hour_exists:1;
   int min;
   Eina_Bool min_exists:1;
} Elm_Params_Datefield;

static const char *datefield_layout_choices[] = {"time", "date", "dateandtime", NULL};
   
static Elm_Datefield_Layout
_datefield_layout_setting_get(const char *layout_str)
{
   unsigned int i;
   
   for (i = 0; i < sizeof(datefield_layout_choices)/sizeof(datefield_layout_choices[0]); i++)
     {
	    if (!strcmp(layout_str, datefield_layout_choices[i]))
	    return i;
     }
   return ELM_DATEFIELD_LAYOUT_DATEANDTIME;
}

static void
external_datefield_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Datefield *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->layout)
     {
	Elm_Datefield_Layout layout = _datefield_layout_setting_get(p->layout);
	elm_datefield_layout_set(obj, layout);
     }
   if (p->format)
     elm_datefield_date_format_set(obj, p->format);
   if (p->mode_exists)
     elm_datefield_time_mode_set(obj, p->mode);
   if ((p->year_exists) || (p->mon_exists) || (p->day_exists) || (p->hour_exists) || (p->min_exists))
     {
        int year, mon, day, hour, min;
	 elm_datefield_date_get(obj, &year, &mon, &day, &hour, &min);
	 
	 if (p->year_exists) year = p->year;
	 if (p->mon_exists) mon = p->mon;
	 if (p->day_exists) day = p->day;
	 if (p->hour_exists) hour = p->hour;
	 if (p->min_exists) min = p->min;
	 elm_datefield_date_set(obj, year, mon, day, hour, min);
     }
}

static Eina_Bool
external_datefield_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "layout"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
	  {
	     Elm_Datefield_Layout layout = _datefield_layout_setting_get(param->s);
	     elm_datefield_layout_set(obj, layout);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "date_format"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_datefield_date_format_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "time_mode"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_datefield_time_mode_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }   
   else if (!strcmp(param->name, "years"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     int mon, day, hour, min;
	     elm_datefield_date_get(obj, NULL, &mon, &day, &hour, &min);
	     elm_datefield_date_set(obj, param->i, mon, day, hour, min);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "months"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     int year, day, hour, min;
	     elm_datefield_date_get(obj, &year, NULL, &day, &hour, &min);
	     elm_datefield_date_set(obj, year, param->i, day, hour, min);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "days"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     int year, mon, hour, min;
	     elm_datefield_date_get(obj, &year, &mon, NULL, &hour, &min);
	     elm_datefield_date_set(obj, year, mon, param->i, hour, min);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "hours"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     int year, mon, day, min;
	     elm_datefield_date_get(obj, &year, &mon, &day, NULL, &min);
	     elm_datefield_date_set(obj, year, mon, day, param->i, min);
	     return EINA_TRUE;
	  }
     }	  
   else if (!strcmp(param->name, "days"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     int year, mon, day, hour;
	     elm_datefield_date_get(obj, &year, &mon, &day, &hour, NULL);
	     elm_datefield_date_set(obj, year, mon, day, hour, param->i);
	     return EINA_TRUE;
	  }
     }	  
   
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_datefield_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "layout"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
	  {
	     Elm_Datefield_Layout layout = elm_datefield_layout_get(obj);
	     if (layout > ELM_DATEFIELD_LAYOUT_DATEANDTIME)
	       return EINA_FALSE;

	     param->s = datefield_layout_choices[layout];
	     return EINA_TRUE;
	  }
     } 
   else if (!strcmp(param->name, "date_format"))
     {
       if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
         {
            param->s = elm_datefield_date_format_get(obj);
	     return EINA_TRUE;
         }
     }
   else if (!strcmp(param->name, "time_mode"))
     {
       if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
         {
            param->i = elm_datefield_time_mode_get(obj);
	     return EINA_TRUE;
         }
     }
   else if (!strcmp(param->name, "years"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_datefield_date_get(obj, &(param->i), NULL, NULL, NULL, NULL);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "months"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_datefield_date_get(obj, NULL, &(param->i), NULL, NULL, NULL);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "days"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_datefield_date_get(obj, NULL, NULL, &(param->i),NULL, NULL);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "hours"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_datefield_date_get(obj, NULL, NULL, NULL, &(param->i), NULL);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "minutes"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
	  {
	     elm_datefield_date_get(obj, NULL, NULL, NULL, NULL, &(param->i));
	     return EINA_TRUE;
	  }
     }   

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_datefield_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Datefield *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Datefield));
   if (!mem)
     return NULL;
   
   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "layout"))
	  mem->layout = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "date_format"))
	  mem->format = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "time_mode"))
	{
	  mem->mode = !!param->i;
	  mem->mode_exists = EINA_TRUE;
	}
	else if (!strcmp(param->name, "years"))
	{
	   mem->year = param->i;
	   mem->year_exists = EINA_TRUE;
	}
	else if (!strcmp(param->name, "months"))
	{
	   mem->mon = param->i;
	   mem->mon_exists = EINA_TRUE;
	}
	else if (!strcmp(param->name, "days"))
	{
	  mem->day = param->i;
	  mem->day_exists = EINA_TRUE;
	}
	else if (!strcmp(param->name, "hours"))
	{
	   mem->hour = param->i;
	   mem->hour_exists = EINA_TRUE;
	}
	else if (!strcmp(param->name, "minutes"))
	{
	   mem->min = param->i;
	   mem->min_exists = EINA_TRUE;
	}
     }

   return mem;
}

static Evas_Object *external_datefield_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_datefield_params_free(void *params)
{
   Elm_Params_Datefield *mem = params;
   
   if (mem->layout)
     eina_stringshare_del(mem->layout);  
   if (mem->format)
     eina_stringshare_del(mem->format);

   free(mem);
}

static Edje_External_Param_Info external_datefield_params[] = {
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("layout", "dateandtime", datefield_layout_choices),
   EDJE_EXTERNAL_PARAM_INFO_STRING("date_format"),  
   EDJE_EXTERNAL_PARAM_INFO_BOOL("time_mode"),
   EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("years", 1900),
   EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("months", 1),
   EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("days", 1),
   EDJE_EXTERNAL_PARAM_INFO_INT("hours"),
   EDJE_EXTERNAL_PARAM_INFO_INT("minutes"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(datefield, "datefield");
DEFINE_EXTERNAL_TYPE_SIMPLE(datefield, "Datefield");

