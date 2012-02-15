#include "private.h"

#define ITEM_COUNT   6

char *item_value_param[]  = { "year", "month", "date", "hour", "minute", "ampm"};
char *item_max_param[]    = { "year_max", "month_max", "date_max",
                              "hour_max", "minute_max", "ampm_max" };
char *item_min_param[]    = { "year_min", "month_min", "date_min",
                              "hour_min", "minute_min", "ampm_min" };
char *item_enable_param[] = { "year_enable", "month_enable", "date_enable",
                              "hour_enable", "minute_enable", "ampm_enable" };
char *item_max_absolute_param[] = { "year_max_abs", "month_max_abs", "date_max_abs",
                                    "hour_max_abs", "minute_max_abs", "ampm_max_abs" };
char *item_min_absolute_param[] = { "year_min_abs", "month_min_abs", "date_min_abs",
                                    "hour_min_abs", "minute_min_abs", "ampm_min_abs" };

typedef struct _Elm_Params_Datefield
{
   Elm_Params base;
   const char *format;
   int item_value[ITEM_COUNT];
   int item_max[ITEM_COUNT];
   int item_min[ITEM_COUNT];
   Eina_Bool item_enable[ITEM_COUNT];
   Eina_Bool item_max_absolute[ITEM_COUNT];
   Eina_Bool item_min_absolute[ITEM_COUNT];
   Eina_Bool item_value_set[ITEM_COUNT];
   Eina_Bool item_max_set[ITEM_COUNT];
   Eina_Bool item_min_set[ITEM_COUNT];
   Eina_Bool item_enable_set[ITEM_COUNT];
   Eina_Bool item_max_absolute_set[ITEM_COUNT];
   Eina_Bool item_min_absolute_set[ITEM_COUNT];
} Elm_Params_Datefield;

static void
external_datefield_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Datefield *p;
   int i;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->format)
     elm_datefield_format_set(obj, p->format);
   for (i = 0; i < ITEM_COUNT; i++)
     {
        if (p->item_value_set[i])
           elm_datefield_item_value_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_value[i]);
     }
   for (i = 0; i < ITEM_COUNT; i++)
     {
        if (p->item_enable_set[i])
          elm_datefield_item_enabled_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_enable[i]);
     }
   for (i = 0; i < ITEM_COUNT; i++)
     {
        if ((p->item_max_set[i]) && (p->item_max_absolute_set[i]))
          elm_datefield_item_max_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_max[i], p->item_max_absolute[i]);
        else if ((p->item_max_set[i]) && !(p->item_max_absolute_set[i]))
          elm_datefield_item_max_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_max[i],
                        elm_datefield_item_max_is_absolute(obj,(ELM_DATEFIELD_YEAR+i)));
        else if (!(p->item_max_set[i]) && (p->item_max_absolute_set[i]))
          elm_datefield_item_max_set(obj, (ELM_DATEFIELD_YEAR+i), elm_datefield_item_max_get
                        (obj, (ELM_DATEFIELD_YEAR+i)), p->item_max_absolute[i]);
     }
   for (i = 0; i < ITEM_COUNT; i++)
     {
        if ((p->item_min_set[i]) && (p->item_min_absolute_set[i]))
          elm_datefield_item_min_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_min[i], p->item_min_absolute[i]);
        else if ((p->item_min_set[i]) && !(p->item_min_absolute_set[i]))
          elm_datefield_item_min_set(obj, (ELM_DATEFIELD_YEAR+i), p->item_min[i],
                        elm_datefield_item_min_is_absolute(obj,(ELM_DATEFIELD_YEAR+i)));
        else if (!(p->item_min_set[i]) && (p->item_min_absolute_set[i]))
          elm_datefield_item_min_set(obj, (ELM_DATEFIELD_YEAR+i), elm_datefield_item_min_get
                        (obj, (ELM_DATEFIELD_YEAR+i)), p->item_min_absolute[i]);
     }
}

static Eina_Bool
external_datefield_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   int i;
   if (!strcmp(param->name, "format"))
     {
       if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
         {
            elm_datefield_format_set(obj, param->s);
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_value_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
              {
                 elm_datefield_item_value_set(obj, (ELM_DATEFIELD_YEAR+i), param->i);
                 return EINA_TRUE;
              }
        }
    }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_max_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
              {
                Eina_Bool max_abs;
                max_abs = elm_datefield_item_max_is_absolute(obj, (ELM_DATEFIELD_YEAR+i));
                 elm_datefield_item_max_set(obj, (ELM_DATEFIELD_YEAR+i), param->i, max_abs);
                 return EINA_TRUE;
              }
        }
    }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_min_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
              {
                Eina_Bool min_abs;
                min_abs = elm_datefield_item_min_is_absolute(obj, (ELM_DATEFIELD_YEAR+i));
                 elm_datefield_item_min_set(obj, (ELM_DATEFIELD_YEAR+i), param->i, min_abs);
                 return EINA_TRUE;
              }
        }
    }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_enable_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
              {
                 elm_datefield_item_enabled_set(obj, (ELM_DATEFIELD_YEAR+i), param->i);
                 return EINA_TRUE;
              }
        }
    }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_max_absolute_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
              {
                int max;
                max = elm_datefield_item_max_get(obj, (ELM_DATEFIELD_YEAR+i));
                 elm_datefield_item_max_set(obj, (ELM_DATEFIELD_YEAR+i), max, param->i);
                 return EINA_TRUE;
              }
        }
    }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
        if (!strcmp(param->name, item_min_absolute_param[i]))
        {
            if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
              {
                int min;
                min = elm_datefield_item_min_get(obj, (ELM_DATEFIELD_YEAR+i));
                 elm_datefield_item_min_set(obj, (ELM_DATEFIELD_YEAR+i), min, param->i);
                 return EINA_TRUE;
              }
        }
    }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_datefield_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   int i;

   if (!strcmp(param->name, "format"))
     {
       if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
         {
            param->s = elm_datefield_format_get(obj);
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_value_param[i]))
         {
            param->i = elm_datefield_item_value_get(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_max_param[i]))
         {
            param->i = elm_datefield_item_max_get(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_min_param[i]))
         {
            param->i = elm_datefield_item_min_get(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_enable_param[i]))
         {
            param->i = elm_datefield_item_enabled_get(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_max_absolute_param[i]))
         {
            param->i = elm_datefield_item_max_is_absolute(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }
   for (i =0 ; i < ITEM_COUNT; i++)
     {
       if (!strcmp(param->name, item_min_absolute_param[i]))
         {
            param->i = elm_datefield_item_min_is_absolute(obj, (ELM_DATEFIELD_YEAR+i));
            return EINA_TRUE;
         }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_datefield_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   Elm_Params_Datefield *mem;
   Edje_External_Param *param;
   const Eina_List *l;
   int i;

   mem = calloc(1, sizeof(Elm_Params_Datefield));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "format"))
          mem->format = eina_stringshare_add(param->s);
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_value_param[i]))
               {
                  mem->item_value[i] = param->i;
                  mem->item_value_set[i] = EINA_TRUE;
               }
          }
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_max_param[i]))
               {
                  mem->item_max[i] = param->i;
                  mem->item_max_set[i] = EINA_TRUE;
               }
          }
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_min_param[i]))
               {
                  mem->item_min[i] = param->i;
                  mem->item_min_set[i] = EINA_TRUE;
               }
          }
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_enable_param[i]))
               {
                  mem->item_enable[i] = param->i;
                  mem->item_enable_set[i] = EINA_TRUE;
               }
          }
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_max_absolute_param[i]))
               {
                  mem->item_max_absolute[i] = param->i;
                  mem->item_max_absolute_set[i] = EINA_TRUE;
               }
          }
        for (i = 0; i < ITEM_COUNT; i ++)
          {
             if (!strcmp(param->name, item_min_absolute_param[i]))
               {
                  mem->item_min_absolute[i] = param->i;
                  mem->item_min_absolute_set[i] = EINA_TRUE;
               }
          }
     }

   return mem;
}

static Evas_Object *external_datefield_content_get(void *data __UNUSED__,
      const Evas_Object *obj __UNUSED__, const char *content __UNUSED__)
{
   ERR("so content");
   return NULL;
}

static void
external_datefield_params_free(void *params)
{
   Elm_Params_Datefield *mem = params;

   if (mem->format)
     eina_stringshare_del(mem->format);

   free(mem);
}

static Edje_External_Param_Info external_datefield_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("format"),
   EDJE_EXTERNAL_PARAM_INFO_INT("year"),
   EDJE_EXTERNAL_PARAM_INFO_INT("month"),
   EDJE_EXTERNAL_PARAM_INFO_INT("date"),
   EDJE_EXTERNAL_PARAM_INFO_INT("hour"),
   EDJE_EXTERNAL_PARAM_INFO_INT("minute"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm"),
   EDJE_EXTERNAL_PARAM_INFO_INT("year_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("year_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("month_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("month_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("date_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("date_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("hour_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("hour_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("minute_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("minute_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm_max"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm_max_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("year_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("year_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("month_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("month_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("date_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("date_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("hour_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("hour_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_INT("minute_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("minute_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm_min"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm_min_abs"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("year_enable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("month_enable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("date_enable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("hour_enable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("minute_enable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("ampm_enable"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(datefield, "datefield");
DEFINE_EXTERNAL_TYPE_SIMPLE(datefield, "Datefield");
