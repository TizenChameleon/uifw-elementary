#include "private.h"

typedef struct _Elm_Params_Scroller
{
   Elm_Params base;
   const char *policy;
   Eina_Bool bounce_exist: 1;
   Eina_Bool h_bounce:1;
   Eina_Bool h_bounce_exists:1;
   Eina_Bool v_bounce:1;
   Eina_Bool v_bounce_exists:1;
}Elm_Params_Scroller;

static const char* list_policy_choices[] = {"auto", "on", "off", NULL};

static Elm_Scroller_Policy
_scroller_horizontal_policy_setting_get(const char *horizontal_policy_str)
{
   unsigned int i;

   assert(sizeof(list_policy_choices)/sizeof(list_policy_choices[0]) == ELM_SCROLLER_POLICY_LAST + 1);

   for (i = 0; i < sizeof(list_policy_choices); i++)
     {
	    if (!strcmp(horizontal_policy_str, list_policy_choices[i]))
	     return i;
     }
   return ELM_SCROLLER_POLICY_LAST;
}

static Elm_Scroller_Policy
_scroller_vertical_policy_setting_get(const char *vertical_policy_str)
{
   unsigned int i;

   assert(sizeof(list_policy_choices)/sizeof(list_policy_choices[0]) == ELM_SCROLLER_POLICY_LAST + 1);

   for (i = 0; i < sizeof(list_policy_choices); i++)
     {
	   if (!strcmp(vertical_policy_str, list_policy_choices[i]))
	   return i;
     }
   return ELM_SCROLLER_POLICY_LAST;
}

static void
external_scroller_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Scroller *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->policy)
     {
	   Elm_Scroller_Policy h_policy;
	   Elm_Scroller_Policy v_policy;

   	   h_policy = _scroller_horizontal_policy_setting_get(p->policy);
   	   v_policy = _scroller_vertical_policy_setting_get(p->policy);

   	   if (h_policy != ELM_LIST_LAST && v_policy != ELM_LIST_LAST)
   		  elm_scroller_policy_set(obj, h_policy, v_policy);
   	   else
   	    {
   		   Elm_Scroller_Policy /*unsigned int*/ policy_h, policy_v;

   		   elm_scroller_policy_get(obj, &policy_h, &policy_v);

   		   if (h_policy != ELM_LIST_LAST && v_policy == ELM_LIST_LAST)
   		      elm_scroller_policy_set(obj, h_policy, policy_v);
   		   else if(v_policy != ELM_LIST_LAST && h_policy == ELM_LIST_LAST)
   		      elm_scroller_policy_set(obj, policy_h, v_policy);
   		   else
   	          elm_scroller_policy_set(obj, policy_h, policy_v);
   	   }
     }
   if ((p->h_bounce_exists) && (p->v_bounce_exists))
	   elm_scroller_bounce_set(obj, p->h_bounce, p->v_bounce);
      else if ((p->h_bounce_exists) || (p->v_bounce_exists))
        {
   	       Eina_Bool h_bounce, v_bounce;

   	       elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
   	       if (p->h_bounce_exists)
   		      elm_scroller_bounce_set(obj, p->h_bounce, v_bounce);
      	   else
   		      elm_scroller_bounce_set(obj, h_bounce, p->v_bounce);
        }
}

static Eina_Bool
external_scroller_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
	{
	if (!strcmp(param->name, "horizontal policy"))
	  {
	     if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
	       {
	  	      Elm_Scroller_Policy h_policy;
			  Elm_Scroller_Policy policy_h, policy_v;

			  elm_scroller_policy_get(obj, &policy_h, &policy_v);

			  h_policy = _scroller_horizontal_policy_setting_get(param->s);

		      if (h_policy == ELM_LIST_LAST) return EINA_FALSE;
		      elm_scroller_policy_set(obj, h_policy, policy_v);
		      return EINA_TRUE;
	       }
	  }
	else if (!strcmp(param->name, "vertical policy"))
	  {
	     if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
	       {
	  	      Elm_Scroller_Policy v_policy;
			  Elm_Scroller_Policy policy_h, policy_v;

			  elm_scroller_policy_get(obj, &policy_h, &policy_v);

			  v_policy = _scroller_vertical_policy_setting_get(param->s);

		      if (v_policy == ELM_LIST_LAST) return EINA_FALSE;
		      elm_scroller_policy_set(obj, policy_h, v_policy);
		      return EINA_TRUE;
	       }
	  }
	else if (!strcmp(param->name, "horizontal bounce"))
	  {
		 if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
		   {
		      Eina_Bool h_bounce, v_bounce;
		      elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
		      elm_scroller_bounce_set(obj, param->i, v_bounce);
		      return EINA_TRUE;
		   }
	  }
    else if (!strcmp(param->name, "vertical bounce"))
	  {
	 	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	       {
		 	  Eina_Bool h_bounce, v_bounce;
			  elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
			  elm_scroller_bounce_set(obj, h_bounce, param->i);
			  return EINA_TRUE;
		   }
	  }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_scroller_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "horizontal policy"))
	 {
		if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
		  {
			 Elm_Scroller_Policy h_policy, v_policy;
			 elm_scroller_policy_get(obj, &h_policy, &v_policy);

		     if (h_policy == ELM_LIST_LAST)
		       return EINA_FALSE;

		     param->s = list_policy_choices[h_policy];
		     return EINA_TRUE;
		  }
	 }
   else if (!strcmp(param->name, "vertical policy"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
			 Elm_Scroller_Policy h_policy, v_policy;
			 elm_scroller_policy_get(obj, &h_policy, &v_policy);

		     if (v_policy == ELM_LIST_LAST)
		       return EINA_FALSE;

		     param->s = list_policy_choices[v_policy];
		     return EINA_TRUE;
		  }
   	 }
   else if (!strcmp(param->name, "horizontal bounce"))
     {
	    if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	      {
	         Eina_Bool h_bounce, v_bounce;
	         elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
	         param->i = h_bounce;
	         return EINA_TRUE;
	      }
     }
   else if (!strcmp(param->name, "vertical bounce"))
     {
	    if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	      {
	         Eina_Bool h_bounce, v_bounce;
	         elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
	         param->i = v_bounce;
	         return EINA_TRUE;
	      }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_scroller_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Scroller *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Scroller));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
	    if (!strcmp(param->name, "horizontal policy"))
	        mem->policy = eina_stringshare_add(param->s);
	    else if (!strcmp(param->name, "vertical policy"))
	       mem->policy = eina_stringshare_add(param->s);
	    else if (!strcmp(param->name, "horizontal bounce"))
		  {
			 mem->h_bounce = param->i;
			 mem->h_bounce_exists = EINA_TRUE;
		  }
		else if (!strcmp(param->name, "vertical bounce"))
		  {
			 mem->v_bounce = param->i;
			 mem->v_bounce_exists = EINA_TRUE;
		  }
     }

   return mem;
}

static Evas_Object *external_scroller_content_get(void *data __UNUSED__,
		const Evas_Object *obj, const char *content)
{
	ERR("so content");
	return NULL;
}

static void
external_scroller_params_free(void *params)
{
   Elm_Params_Scroller *mem = params;

   if (mem->policy)
      eina_stringshare_del(mem->policy);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_scroller_params[] = {
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("horizontal policy", "auto", list_policy_choices),
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("vertical policy", "auto", list_policy_choices),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal bounce"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("vertical bounce"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(scroller, "scroller");
DEFINE_EXTERNAL_TYPE_SIMPLE(scroller, "Scroller");
