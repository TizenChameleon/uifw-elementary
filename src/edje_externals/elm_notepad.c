#include "private.h"

typedef struct _Elm_Params_Notepad
{
} Elm_Params_Notepad;

static void
external_notepad_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Notepad *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   /* TODO: to be expanded */
}

static Eina_Bool
external_notepad_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   /* TODO: to be expanded */

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_notepad_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   /* TODO: to be expanded */

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_notepad_params_parse(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const Eina_List *params)
{
   /* TODO: to be expanded */

   return NULL;
}

static void
external_notepad_params_free(void *params)
{
}

static Edje_External_Param_Info external_notepad_params[] = {
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(notepad, "notepad")
DEFINE_EXTERNAL_TYPE_SIMPLE(notepad, "Notepad");
