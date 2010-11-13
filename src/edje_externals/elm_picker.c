#include "private.h"

static void
external_picker_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   ERR("no params");
}

static Eina_Bool
external_picker_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   ERR("no params");
   return EINA_FALSE;
}

static Eina_Bool
external_picker_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   ERR("no params");
   return EINA_FALSE;
}

static void *
external_picker_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   ERR("no params");
   return NULL;
}

static Evas_Object *external_picker_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
   ERR("so content");
   return NULL;
}

static void
external_picker_params_free(void *params)
{
}

static Edje_External_Param_Info external_picker_params[] = {
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(picker, "picker");
DEFINE_EXTERNAL_TYPE_SIMPLE(picker, "Picker");



