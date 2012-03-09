#include "Elementary.h"
#include <Eina.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

EAPI char *
<<<<<<< HEAD
map_module_source_get(void)
=======
map_module_source_name_get(void)
>>>>>>> remotes/origin/upstream
{
   return strdup("test_map");
}

EAPI int
<<<<<<< HEAD
map_module_zoom_min_get(void)
=======
map_module_tile_zoom_min_get(void)
>>>>>>> remotes/origin/upstream
{
   return 0;
}

EAPI int
<<<<<<< HEAD
map_module_zoom_max_get(void)
=======
map_module_tile_zoom_max_get(void)
>>>>>>> remotes/origin/upstream
{
   return 18;
}

EAPI char *
<<<<<<< HEAD
map_module_url_get(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
=======
map_module_tile_url_get(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
>>>>>>> remotes/origin/upstream
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://tile.openstreetmap.org/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

<<<<<<< HEAD
EAPI int
map_module_route_source_get(void)
{
   return 0;
}

EAPI char *
map_module_route_url_get(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
=======
EAPI char *
map_module_route_source_get(void)
{
   return NULL;
}

EAPI char *
map_module_route_url_get(Evas_Object *obj __UNUSED__, const char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
>>>>>>> remotes/origin/upstream
{
   return strdup("");
}

EAPI char *
<<<<<<< HEAD
map_module_name_url_get(Evas_Object *obj __UNUSED__, int method __UNUSED__, char *name __UNUSED__, double lon __UNUSED__, double lat __UNUSED__)
=======
map_module_name_url_get(Evas_Object *obj __UNUSED__, int method __UNUSED__, const char *name __UNUSED__, double lon __UNUSED__, double lat __UNUSED__)
>>>>>>> remotes/origin/upstream
{
   return strdup("");
}

EAPI Eina_Bool
<<<<<<< HEAD
map_module_geo_into_coord(const Evas_Object *obj __UNUSED__, int zoom __UNUSED__, double lon __UNUSED__, double lat __UNUSED__, int size __UNUSED__, int *x __UNUSED__, int *y __UNUSED__)
=======
map_module_tile_geo_to_coord(const Evas_Object *obj __UNUSED__, int zoom __UNUSED__, double lon __UNUSED__, double lat __UNUSED__, int size __UNUSED__, int *x __UNUSED__, int *y __UNUSED__)
>>>>>>> remotes/origin/upstream
{
   return EINA_FALSE;
}

EAPI Eina_Bool
<<<<<<< HEAD
map_module_coord_into_geo(const Evas_Object *obj __UNUSED__, int zoom __UNUSED__, int x __UNUSED__, int y __UNUSED__, int size __UNUSED__, double *lon __UNUSED__, double *lat __UNUSED__)
=======
map_module_tile_coord_to_geo(const Evas_Object *obj __UNUSED__, int zoom __UNUSED__, int x __UNUSED__, int y __UNUSED__, int size __UNUSED__, double *lon __UNUSED__, double *lat __UNUSED__)
>>>>>>> remotes/origin/upstream
{
   return EINA_FALSE;
}

static Eina_Bool
_module_init(void)
{
   return EINA_TRUE;
}

static void
_module_shutdown(void)
{
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);

