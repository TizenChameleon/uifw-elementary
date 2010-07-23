/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#ifdef ELM_EWEBKIT
#include <EWebKit.h>
#include <glib-object.h>

static Ewk_View_Smart_Class _parent_sc = EWK_VIEW_SMART_CLASS_INIT_NULL;
#endif

Evas_Object*
_els_webview_add(Evas_Object *parent, Eina_Bool tiled)
{
#ifdef ELM_EWEBKIT
   static Evas_Smart* smart = NULL;
   Evas_Object *obj;
   Evas *e;
   int (*ewk_init)(void);
   Eina_Bool (*ewk_view_single_smart_set)(Ewk_View_Smart_Class *);
   Eina_Bool (*ewk_view_tiled_smart_set)(Ewk_View_Smart_Class *);

   if (!smart)
     {
	g_type_init();
	if (!g_thread_get_initialized())
	     g_thread_init(NULL);

	void *ewk_handle = dlopen("/usr/lib/libewebkit.so", RTLD_LAZY);//FIXME
        ewk_init = (int (*)())dlsym(ewk_handle, "ewk_init");
	ewk_init();
	static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION("ELM_WEBVIEW");

	if (tiled)
	  {
	     ewk_view_tiled_smart_set = (Eina_Bool (*)(Ewk_View_Smart_Class *))dlsym(ewk_handle, "ewk_view_tiled_smart_set");
	     ewk_view_tiled_smart_set(&api);
	     if (EINA_UNLIKELY(!_parent_sc.sc.add))
	       ewk_view_tiled_smart_set(&_parent_sc);
	  }
	else
	  {
	     ewk_view_single_smart_set = (Eina_Bool (*)(Ewk_View_Smart_Class *))dlsym(ewk_handle, "ewk_view_single_smart_set");
	     ewk_view_single_smart_set(&api);
	     if (EINA_UNLIKELY(!_parent_sc.sc.add))
	       ewk_view_single_smart_set(&_parent_sc);
	  }
	//TODO: add apis
	dlclose(ewk_handle);
     }

   if (!smart)
     return NULL;

   obj = evas_object_smart_add(e, smart);
   if (!obj)
     return NULL;

   /*TODO:
   View_Smart_Data *sd = evas_object_smart_data_get(obj);
   if (sd)
     sd->tiled = tiled;
   */
   /*
      ewk_init = (int (*)())dlsym(ewk_handle, "ewk_init");
      ewk_view_add = (Evas_Object *(*)(Evas *))dlsym(ewk_handle, "ewk_view_tiled_add");
      ewk_view_uri_set = (Eina_Bool (*)(Evas_Object *, const char *))dlsym(ewk_handle, "ewk_view_uri_set");

      e = evas_object_evas_get(parent);
      ewk_init();
      obj = ewk_view_add(e);
      ewk_view_uri_set(obj, "file:///a.html");
      evas_object_show(obj);
      */
   return obj;
#else
   return NULL;
#endif
}
