#include <Elementary.h>
#include "elm_priv.h"

Evas_Object*
_els_webview_add(Evas_Object *parent, Eina_Bool tiled)
{
#ifdef ELM_EWEBKIT
   Evas_Object *obj;
   Evas *e;
   int (*ewk_init)(void);
   Evas_Object *(*ewk_view_add)(Evas *);
   Eina_Bool (*ewk_view_uri_set)(Evas_Object *, const char *);

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
