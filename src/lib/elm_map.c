#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"
#include "elm_priv.h"
#include "els_scroller.h"

#ifdef HAVE_ELEMENTARY_ECORE_CON

typedef struct _Widget_Data Widget_Data;
typedef struct _Pan Pan;
typedef struct _Grid Grid;
typedef struct _Grid_Item Grid_Item;
typedef struct _Marker_Group Marker_Group;
typedef struct _Event Event;
typedef struct _Path_Node Path_Node;
typedef struct _Path_Waypoint Path_Waypoint;
typedef struct _Url_Data Url_Data;
typedef struct _Route_Dump Route_Dump;
typedef struct _Name_Dump Name_Dump;
typedef struct _Track_Dump Track_Dump;

#define CACHE_ROOT_PATH   "/tmp/elm_map"
#define CACHE_PATH        CACHE_ROOT_PATH"/%d/%d/%d"
#define CACHE_FILE_PATH   "%s/%d.png"
#define DEST_ROUTE_XML_FILE "/tmp/elm_map-route-XXXXXX"
#define DEST_NAME_XML_FILE "/tmp/elm_map-name-XXXXXX"

#define ROUTE_YOURS_URL "http://www.yournavigation.org/api/dev/route.php"
#define ROUTE_TYPE_MOTORCAR "motocar"
#define ROUTE_TYPE_BICYCLE "bicycle"
#define ROUTE_TYPE_FOOT "foot"
#define YOURS_DISTANCE "distance"
#define YOURS_DESCRIPTION "description"
#define YOURS_COORDINATES "coordinates"

// TODO: fix monav & ors url
#define ROUTE_MONAV_URL "http://"
#define ROUTE_ORS_URL "http:///"

#define NAME_NOMINATIM_URL "http://nominatim.openstreetmap.org"
#define NOMINATIM_RESULT "result"
#define NOMINATIM_PLACE "place"
#define NOMINATIM_ATTR_LON "lon"
#define NOMINATIM_ATTR_LAT "lat"

#define PINCH_ZOOM_MIN 0.25
#define PINCH_ZOOM_MAX 4.0
#define MAX_CONCURRENT_DOWNLOAD 10

#define GPX_NAME "name>"
#define GPX_COORDINATES "trkpt "
#define GPX_LON "lon"
#define GPX_LAT "lat"
#define GPX_ELE "ele>"
#define GPX_TIME "time>"

// Map sources
// Currently the size of a tile must be 256*256
// and the size of the map must be pow(2.0, z)*tile_size
typedef struct _Map_Sources_Tab
{
   const char *name;
   int zoom_min;
   int zoom_max;
   ElmMapModuleUrlFunc url_cb;
   Elm_Map_Route_Sources route_source;
   ElmMapModuleRouteUrlFunc route_url_cb;
   ElmMapModuleNameUrlFunc name_url_cb;
   ElmMapModuleGeoIntoCoordFunc geo_into_coord;
   ElmMapModuleCoordIntoGeoFunc coord_into_geo;
} Map_Sources_Tab;

//Zemm min is supposed to be 0
static char *_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_mapquest_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_mapquest_aerial_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);

static char *_yours_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
/*
static char *_monav_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
static char *_ors_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
 */
static char *_nominatim_url_cb(Evas_Object *obj, int method, char *name, double lon, double lat);

static Map_Sources_Tab default_map_sources_tab[] =
{
     {"Mapnik", 0, 18, _mapnik_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb, _nominatim_url_cb, NULL, NULL},
     {"Osmarender", 0, 17, _osmarender_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb, _nominatim_url_cb, NULL, NULL},
     {"CycleMap", 0, 16, _cyclemap_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb, _nominatim_url_cb, NULL, NULL},
     {"MapQuest", 0, 18, _mapquest_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb, _nominatim_url_cb, NULL, NULL},
     {"MapQuest Open Aerial", 0, 11, _mapquest_aerial_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb, _nominatim_url_cb, NULL, NULL},
};

struct _Url_Data
{
   Ecore_Con_Url *con_url;

   FILE *fd;
   char *fname;
};

struct _Elm_Map_Marker_Class
{
   const char *style;
   int zoom_displayed;

   struct _Elm_Map_Marker_Class_Func {
        ElmMapMarkerGetFunc get;
        ElmMapMarkerDelFunc del; //if NULL the object will be destroyed with evas_object_del()
        ElmMapMarkerIconGetFunc icon_get;
   } func;

   struct { //this part is private, do not modify these values
        Eina_Bool set : 1;
        Evas_Coord edje_w, edje_h;
   } priv;
};

struct _Elm_Map_Marker
{
   Widget_Data *wd;
   Elm_Map_Marker_Class *clas;
   Elm_Map_Group_Class *clas_group;
   double longitude, latitude;

   Evas_Coord map_size;
   Evas_Coord *x, *y;
   void *data;
   Marker_Group **groups;
   Evas_Object *content;
};

struct _Elm_Map_Group_Class
{
   const char *style;
   void *data;
   int zoom_displayed; // display the group if the zoom is >= to zoom_display
   int zoom_grouped; // group the markers only if the zoom is <= to zoom_groups
   Eina_Bool hide : 1;

   struct {
        ElmMapGroupIconGetFunc icon_get;
   } func;

   struct { //this part is private, do not modify these values
        Eina_Bool set : 1;
        Evas_Coord edje_w, edje_h;
        Evas_Coord edje_max_w, edje_max_h;

        Eina_List *objs_used;
        Eina_List *objs_notused;
   } priv;
};

struct _Marker_Group
{
   Widget_Data *wd;
   Eina_Matrixsparse_Cell *cell;
   Elm_Map_Group_Class *clas;

   Eina_List *markers;
   long long sum_x, sum_y;
   Evas_Coord x, y;
   Evas_Coord w, h;

   Evas_Object *obj, *bubble, *sc, *bx, *rect;
   Eina_Bool open : 1;
   Eina_Bool bringin : 1;
   Eina_Bool update_nbelems : 1;
   Eina_Bool update_resize : 1;
   Eina_Bool update_raise : 1;
   Eina_Bool delete_object : 1;
};

struct _Elm_Map_Route
{
   Widget_Data *wd;

   Path_Node *n;
   Path_Waypoint *w;
   Ecore_Con_Url *con_url;

   int type;
   int method;
   int x, y;
   double flon, flat, tlon, tlat;

   Eina_List *nodes, *path;
   Eina_List *waypoint;

   struct {
      int node_count;
      int waypoint_count;
      const char *nodes;
      const char *waypoints;
      double distance; /* unit : km */
   } info;

   Eina_List *handlers;
   Url_Data ud;

   struct {
      int r;
      int g;
      int b;
      int a;
   } color;

   Eina_Bool inbound : 1;
};

struct _Path_Node
{
   Widget_Data *wd;

   int idx;
   struct {
      double lon, lat;
      char *address;
   } pos;
};

struct _Path_Waypoint
{
   Widget_Data *wd;

   const char *point;
};

struct _Elm_Map_Name
{
   Widget_Data *wd;

   Ecore_Con_Url *con_url;
   int method;
   char *address;
   double lon, lat;
   Url_Data ud;
   Ecore_Event_Handler *handler;
};

struct _Grid_Item
{
   Widget_Data *wd;
   Grid *g;
   int zoom;
   Evas_Object *img;
   //Evas_Object *txt;
   const char *file;
   const char *source;
   struct {
        int x, y, w, h;
   } src, out;

   Eina_Bool file_have : 1;
   Ecore_File_Download_Job *job;
   int try_num;
};

struct _Grid
{
   Widget_Data *wd;
   int tsize; // size of tile (tsize x tsize pixels)
   int zoom; // zoom level tiles want for optimal display (1, 2, 4, 8)
   int iw, ih; // size of image in pixels
   int w, h; // size of grid image in pixels (represented by grid)
   int gw, gh; // size of grid in tiles
   Eina_Matrixsparse *grid;
};

struct _Widget_Data
{
   Evas_Object *obj;
   Evas_Object *scr;
   Evas_Object *ges;
   Evas_Object *pan_smart;
   Evas_Object *rect;
   Evas_Object *sep_maps_markers; //map objects are below this object and marker objects are on top
   Pan *pan;
   Evas_Coord pan_x, pan_y, minw, minh;

   int id;
   int zoom;
   int zoom_method;
   Elm_Map_Zoom_Mode mode;

   Ecore_Job *calc_job;
   Ecore_Timer *scr_timer;
   Ecore_Timer *long_timer;
   Ecore_Animator *zoom_animator;
   double t;
   struct {
        int w, h;
        int ow, oh, nw, nh;
        struct {
             double x, y;
        } spos;
   } size;
   struct {
        Eina_Bool show : 1;
        Evas_Coord x, y ,w ,h;
   } show;
   int tsize;
   int nosmooth;
   Eina_List *grids;
   Eina_Bool resized : 1;
   Eina_Bool on_hold : 1;
   Eina_Bool paused : 1;
   Eina_Bool paused_markers : 1;

   struct {
        Eina_Bool enabled;
        double lon, lat;
   } center_on;

   Ecore_Job *markers_place_job;
   Eina_Matrixsparse **markers;
   Eina_List *cells_displayed; // list of Eina_Matrixsparse_Cell
   Evas_Coord markers_max_num;
   int marker_max_w, marker_max_h;
   int marker_zoom;
   Eina_List *opened_bubbles; //opened bubbles, list of Map_Group *

   Eina_List *groups_clas; // list of Elm_Map_Group_Class*
   Eina_List *markers_clas; // list of Elm_Map_Markers_Class*

   Elm_Map_Route_Sources route_source;
   Eina_List *s_event_list;
   int try_num;
   int finish_num;

   Eina_Hash *ua;
   const char *user_agent;
   Eina_List *route;
   Eina_List *track;
   Evas_Event_Mouse_Down ev;
   Eina_List *names;
   int multi_count;

   struct {
        Evas_Coord cx, cy;
        double level, diff;
   } pinch;

   struct {
        Evas_Coord cx, cy;
        double a, d;
   } rotate;

   int wheel_diff;
   Eina_Bool wheel_disabled : 1;
   Eina_Bool scr_started : 1;

   Eina_Array *modules;
   Eina_List *map_sources_tab;
   const char **source_names;
   Evas_Map *map;
   Ecore_Timer *zoom_timer;
   Map_Sources_Tab *src;
   const char *gpx_file;
   int zoom_min, zoom_max;
   Eina_List *download_list;
   int download_num;
};

struct _Pan
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Widget_Data *wd;
};

struct _Event
{
   int device;

   struct {
        Evas_Coord x, y;
   } start;

   struct {
        Evas_Coord x, y;
   } prev;

   Evas_Coord x, y, w, h;

   Evas_Object *object;

   int pinch_start_dis;
   int pinch_dis;
};

struct _Route_Dump
{
   int id;
   char *fname;
   double distance;
   char *description;
   char *coordinates;
};

enum _Route_Xml_Attribute
{
   ROUTE_XML_NONE,
   ROUTE_XML_DISTANCE,
   ROUTE_XML_DESCRIPTION,
   ROUTE_XML_COORDINATES,
   ROUTE_XML_LAST
} Route_Xml_Attibute;

struct _Name_Dump
{
   int id;
   char *address;
   double lon;
   double lat;
};

enum _Name_Xml_Attribute
{
   NAME_XML_NONE,
   NAME_XML_NAME,
   NAME_XML_LON,
   NAME_XML_LAT,
   NAME_XML_LAST
} Name_Xml_Attibute;

enum _Zoom_Method
{
   ZOOM_METHOD_NONE,
   ZOOM_METHOD_IN,
   ZOOM_METHOD_OUT,
   ZOOM_METHOD_LAST
} Zoom_Mode;

enum _Track_Xml_Attribute
{
   TRACK_XML_NONE,
   TRACK_XML_COORDINATES,
   TRACK_XML_LAST
} Track_Xml_Attibute;

static Evas_Smart_Class _pan_sc = EVAS_SMART_CLASS_INIT_NULL;
static const char *widtype = NULL;
static int idnum = 1;

static const char SIG_CHANGED[] = "changed";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_LOADED_DETAIL[] = "loaded,detail";
static const char SIG_LOAD_DETAIL[] = "load,detail";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_PRESS[] = "press";
static const char SIG_SCROLL[] = "scroll";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_ZOOM_CHANGE[] = "zoom,change";
static const char SIG_ZOOM_START[] = "zoom,start";
static const char SIG_ZOOM_STOP[] = "zoom,stop";
static const char SIG_DOWNLOADED[] = "downloaded";
static const char SIG_ROUTE_LOAD[] = "route,load";
static const char SIG_ROUTE_LOADED[] = "route,loaded";
static const char SIG_NAME_LOAD[] = "name,load";
static const char SIG_NAME_LOADED[] = "name,loaded";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CHANGED, ""},
       {SIG_CLICKED, ""},
       {SIG_CLICKED_DOUBLE, ""},
       {SIG_LOADED_DETAIL, ""},
       {SIG_LOAD_DETAIL, ""},
       {SIG_LONGPRESSED, ""},
       {SIG_PRESS, ""},
       {SIG_SCROLL, ""},
       {SIG_SCROLL_DRAG_START, ""},
       {SIG_SCROLL_DRAG_STOP, ""},
       {SIG_ZOOM_CHANGE, ""},
       {SIG_ZOOM_START, ""},
       {SIG_ZOOM_STOP, ""},
       {SIG_DOWNLOADED, ""},
       {SIG_ROUTE_LOAD, ""},
       {SIG_ROUTE_LOADED, ""},
       {SIG_NAME_LOAD, ""},
       {SIG_NAME_LOADED, ""},
       {NULL, NULL}
};

static void _pan_calculate(Evas_Object *obj);

static void _rect_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _calc_job(void *data);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);
static void grid_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);
static void grid_load(Evas_Object *obj, Grid *g);

static void _process_download_list(Evas_Object *obj);
static void _add_download_list(Evas_Object *obj, Grid_Item *gi);

static void _group_object_create(Marker_Group *group);
static void _group_object_free(Marker_Group *group);
static void _group_open_cb(void *data, Evas_Object *obj, const char *emission, const char *soure);
static void _group_bringin_cb(void *data, Evas_Object *obj, const char *emission, const char *soure);
static void _group_bubble_create(Marker_Group *group);
static void _group_bubble_free(Marker_Group *group);
static void _group_bubble_place(Marker_Group *group);

static int _group_bubble_content_update(Marker_Group *group);
static void _group_bubble_content_free(Marker_Group *group);
static void marker_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);
static void _bubble_sc_hints_changed_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void route_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);
static void track_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);

static Eina_Bool
module_list_cb(Eina_Module *m, void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(data);
   Map_Sources_Tab *s;
   ElmMapModuleSourceFunc source;
   ElmMapModuleZoomMinFunc zoom_min;
   ElmMapModuleZoomMaxFunc zoom_max;
   ElmMapModuleUrlFunc url;
   ElmMapModuleRouteSourceFunc route_source;
   ElmMapModuleRouteUrlFunc route_url;
   ElmMapModuleNameUrlFunc name_url;
   ElmMapModuleGeoIntoCoordFunc geo_into_coord;
   ElmMapModuleCoordIntoGeoFunc coord_into_geo;
   const char *file;

   if (!wd) return EINA_FALSE;

   file = eina_module_file_get(m);
   if (!eina_module_load(m))
     {
        ERR("could not load module \"%s\": %s", file, eina_error_msg_get(eina_error_get()));
        return EINA_FALSE;
     }

   source = eina_module_symbol_get(m, "map_module_source_get");
   zoom_min = eina_module_symbol_get(m, "map_module_zoom_min_get");
   zoom_max = eina_module_symbol_get(m, "map_module_zoom_max_get");
   url = eina_module_symbol_get(m, "map_module_url_get");
   route_source = eina_module_symbol_get(m, "map_module_route_source_get");
   route_url = eina_module_symbol_get(m, "map_module_route_url_get");
   name_url = eina_module_symbol_get(m, "map_module_name_url_get");
   geo_into_coord = eina_module_symbol_get(m, "map_module_geo_into_coord");
   coord_into_geo = eina_module_symbol_get(m, "map_module_coord_into_geo");
   if ((!source) || (!zoom_min) || (!zoom_max) || (!url) || (!route_source) || (!route_url) || (!name_url) || (!geo_into_coord) || (!coord_into_geo))
     {
        WRN("could not find map_module_source_get() in module \"%s\": %s", file, eina_error_msg_get(eina_error_get()));
        eina_module_unload(m);
        return EINA_FALSE;
     }
   s = calloc(1, sizeof(Map_Sources_Tab));
   EINA_SAFETY_ON_NULL_RETURN_VAL(s, EINA_FALSE);
   s->name = source();
   s->zoom_min = zoom_min();
   s->zoom_max = zoom_max();
   s->url_cb = url;
   s->route_source = route_source();
   s->route_url_cb = route_url;
   s->name_url_cb = name_url;
   s->geo_into_coord = geo_into_coord;
   s->coord_into_geo = coord_into_geo;
   wd->map_sources_tab = eina_list_append(wd->map_sources_tab, s);

   return EINA_TRUE;
}

static void
module_init(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   wd->modules = eina_module_list_get(wd->modules, MODULES_PATH, 1, &module_list_cb, data);
}

static void
source_init(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   Map_Sources_Tab *s;
   Eina_List *l;
   unsigned int idx;

   if (!wd) return;
   for (idx = 0; idx < sizeof(default_map_sources_tab)/sizeof(Map_Sources_Tab); idx++)
     {
        s = calloc(1, sizeof(Map_Sources_Tab));
        EINA_SAFETY_ON_NULL_RETURN(s);
        s->name = default_map_sources_tab[idx].name;
        s->zoom_min = default_map_sources_tab[idx].zoom_min;
        s->zoom_max = default_map_sources_tab[idx].zoom_max;
        s->url_cb = default_map_sources_tab[idx].url_cb;
        s->route_source = default_map_sources_tab[idx].route_source;
        s->route_url_cb = default_map_sources_tab[idx].route_url_cb;
        s->name_url_cb = default_map_sources_tab[idx].name_url_cb;
        s->geo_into_coord = default_map_sources_tab[idx].geo_into_coord;
        s->coord_into_geo = default_map_sources_tab[idx].coord_into_geo;
        wd->map_sources_tab = eina_list_append(wd->map_sources_tab, s);
        if (!idx) wd->src = s;
     }
   module_init(data);

   int n = eina_list_count(wd->map_sources_tab);
   wd->source_names = malloc(sizeof(char *) * (n + 1));
   if (!wd->source_names)
     {
        ERR("init source names failed.");
        return;
     }
   idx = 0;
   EINA_LIST_FOREACH(wd->map_sources_tab, l, s)
     {
        wd->source_names[idx] = strdup(s->name);
        INF("source : %s", wd->source_names[idx]);
        if (s->zoom_min < wd->zoom_min) wd->zoom_min = s->zoom_min;
        if (s->zoom_max > wd->zoom_max) wd->zoom_max = s->zoom_max;
        idx++;
     }
   wd->source_names[idx] = NULL;
}

static void
obj_rotate_zoom(void *data, Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   int ow, oh, iw, ih;
   if ((!wd->pinch.cx) && (!wd->pinch.cy))
     {
        wd->pinch.cx = wd->rotate.cx;
        wd->pinch.cy = wd->rotate.cy;
     }

   evas_map_util_points_populate_from_object_full(wd->map, obj, 0);
   evas_object_image_size_get(obj, &iw, &ih);
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow < iw) || (oh < ih))
     {
        ow *= (double)iw / ow;
        oh *= (double)ih / oh;
        evas_map_point_image_uv_set(wd->map, 1, ow, 0);
        evas_map_point_image_uv_set(wd->map, 2, ow, oh);
        evas_map_point_image_uv_set(wd->map, 3, 0, oh);
     }
   evas_map_util_zoom(wd->map, wd->pinch.level, wd->pinch.level, wd->pinch.cx, wd->pinch.cy);
   evas_map_util_rotate(wd->map, wd->rotate.d, wd->rotate.cx, wd->rotate.cy);
   evas_object_map_enable_set(obj, EINA_TRUE);
   evas_object_map_set(obj, wd->map);
}

static void
#ifdef ELM_EMAP
track_place(Evas_Object *obj, Grid *g __UNUSED__, Evas_Coord px, Evas_Coord py, Evas_Coord ox __UNUSED__, Evas_Coord oy __UNUSED__, Evas_Coord ow, Evas_Coord oh)
#else
track_place(Evas_Object *obj __UNUSED__, Grid *g __UNUSED__, Evas_Coord px __UNUSED__, Evas_Coord py __UNUSED__, Evas_Coord ox __UNUSED__, Evas_Coord oy __UNUSED__, Evas_Coord ow __UNUSED__, Evas_Coord oh __UNUSED__)
#endif
{
#ifdef ELM_EMAP
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Evas_Object *route;
   int xmin, xmax, ymin, ymax;

   if (!wd) return;
   Evas_Coord size = pow(2.0, wd->zoom)*wd->tsize;

   EINA_LIST_FOREACH(wd->track, l, route)
     {
        elm_map_utils_convert_geo_into_coord(wd->obj, elm_route_lon_min_get(route), elm_route_lat_max_get(route), size, &xmin, &ymin);
        elm_map_utils_convert_geo_into_coord(wd->obj, elm_route_lon_max_get(route), elm_route_lat_min_get(route), size, &xmax, &ymax);

        if( !(xmin < px && xmax < px) && !(xmin > px+ow && xmax > px+ow))
        {
           if( !(ymin < py && ymax < py) && !(ymin > py+oh && ymax > py+oh))
           {
              //display the route
              evas_object_move(route, xmin - px + ox, ymin - py + oy);
              evas_object_resize(route, xmax - xmin, ymax - ymin);

              evas_object_raise(route);
              obj_rotate_zoom(obj, route);
              evas_object_show(route);

              continue;
           }
        }
        //the route is not display
        evas_object_hide(route);
     }
#endif
}
static void
route_place(Evas_Object *obj, Grid *g __UNUSED__, Evas_Coord px, Evas_Coord py, Evas_Coord ox __UNUSED__, Evas_Coord oy __UNUSED__, Evas_Coord ow, Evas_Coord oh)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *lr, *lp, *ln;
   Path_Node *n;
   Evas_Object *p;
   Elm_Map_Route *r;
   int nodes;
   int x, y, rx, ry;
   double a;

   if (!wd) return;
   Evas_Coord size = pow(2.0, wd->zoom)*wd->tsize;

   EINA_LIST_FOREACH(wd->route, lr, r)
     {
        EINA_LIST_FOREACH(r->path, lp, p)
          {
             evas_object_polygon_points_clear(p);
          }

        evas_object_geometry_get(wd->rect, &rx, &ry, NULL, NULL);
        nodes = eina_list_count(r->nodes);

        EINA_LIST_FOREACH(r->nodes, ln, n)
          {
             if ((!wd->zoom) || ((n->idx) &&
                 ((n->idx % (int)ceil((double)nodes/(double)size*100.0))))) continue;
             if (r->inbound)
               {
                  elm_map_utils_convert_geo_into_coord(wd->obj, n->pos.lon, n->pos.lat, size, &x, &y);
                  if ((x >= px - ow) && (x <= (px + ow*2)) &&
                      (y >= py - oh) && (y <= (py + oh*2)))
                    {
                       x = x - px + rx;
                       y = y - py + ry;

                       p = eina_list_nth(r->path, n->idx);
                       a = (double)(y - r->y) / (double)(x - r->x);
                       if ((abs(a) >= 1) || (r->x == x))
                         {
                            evas_object_polygon_point_add(p, r->x - 3, r->y);
                            evas_object_polygon_point_add(p, r->x + 3, r->y);
                            evas_object_polygon_point_add(p, x + 3, y);
                            evas_object_polygon_point_add(p, x - 3, y);
                         }
                       else
                         {
                            evas_object_polygon_point_add(p, r->x, r->y - 3);
                            evas_object_polygon_point_add(p, r->x, r->y + 3);
                            evas_object_polygon_point_add(p, x, y + 3);
                            evas_object_polygon_point_add(p, x, y - 3);
                         }

                       evas_object_color_set(p, r->color.r, r->color.g, r->color.b, r->color.a);
                       evas_object_raise(p);
                       obj_rotate_zoom(obj, p);
                       evas_object_show(p);
                       r->x = x;
                       r->y = y;
                    }
                  else r->inbound = EINA_FALSE;
               }
             else
               {
                  elm_map_utils_convert_geo_into_coord(wd->obj, n->pos.lon, n->pos.lat, size, &x, &y);
                  if ((x >= px - ow) && (x <= (px + ow*2)) &&
                      (y >= py - oh) && (y <= (py + oh*2)))
                    {
                       r->x = x - px + rx;
                       r->y = y - py + ry;
                       r->inbound = EINA_TRUE;
                    }
                  else r->inbound = EINA_FALSE;
               }
          }
          r->inbound = EINA_FALSE;
     }
}

static void
rect_place(Evas_Object *obj, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ax, ay, gw, gh, hh, ww;

   if (!wd) return;
   evas_object_geometry_get(wd->rect, NULL, NULL, &ww, &hh);

   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;

   if ((ww == gw) && (hh == gh)) return;

   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;
   evas_object_move(wd->rect,
                    ox + 0 - px + ax,
                    oy + 0 - py + ay);
   evas_object_resize(wd->rect, gw, gh);

   if (wd->show.show)
     {
        wd->show.show = EINA_FALSE;
        elm_smart_scroller_child_region_show(wd->scr, wd->show.x, wd->show.y, wd->show.w, wd->show.h);
     }
}

static void
marker_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ax, ay, gw, gh, tx, ty;
   Eina_List *l, *markers;
   Eina_Matrixsparse_Cell *cell;
   Marker_Group *group;
   int xx, yy, ww, hh;
   char buf[PATH_MAX];
   int y, x;
   int g_xx, g_yy, g_hh, g_ww;

   if (!wd) return;

   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;
   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;

   if (wd->zoom != wd->marker_zoom)
     {
        EINA_LIST_FREE(wd->cells_displayed, cell)
          {
             EINA_LIST_FOREACH(eina_matrixsparse_cell_data_get(cell), l, group)
               {
                  if (group->obj) _group_object_free(group);
               }
          }
     }
   wd->marker_zoom = wd->zoom;

   if ((wd->paused_markers)
       && ((wd->size.nw != wd->size.w) || (wd->size.nh != wd->size.h)) )
     return;

   g_xx = wd->pan_x / wd->tsize;
   if (g_xx < 0) g_xx = 0;
   g_yy = wd->pan_y / wd->tsize;
   if (g_yy < 0) g_yy = 0;
   g_ww = (ow / wd->tsize) + 1;
   if (g_xx + g_ww >= g->gw) g_ww = g->gw - g_xx - 1;
   g_hh = (oh / wd->tsize) + 1;
   if (g_yy + g_hh >= g->gh) g_hh = g->gh - g_yy - 1;

   //hide groups no more displayed
   EINA_LIST_FREE(wd->cells_displayed, cell)
     {
        eina_matrixsparse_cell_position_get(cell, (unsigned long *)&y, (unsigned long *)&x);
        if ((y < g_yy) || (y > g_yy + g_hh) || (x < g_xx) || (x > g_xx + g_ww))
          {
             EINA_LIST_FOREACH(eina_matrixsparse_cell_data_get(cell), l, group)
               {
                  if (group->obj) _group_object_free(group);
               }
          }
     }

   if (!wd->marker_zoom)
     {
        g_ww = 0;
        g_hh = 0;
     }

   for (y = g_yy; y <= g_yy + g_hh; y++)
     {
        for (x = g_xx; x <= g_xx + g_ww; x++)
          {
             if (!wd->markers[wd->zoom]) continue;
             eina_matrixsparse_cell_idx_get(wd->markers[wd->zoom], y, x, &cell);
             if (!cell) continue;
             wd->cells_displayed = eina_list_append(wd->cells_displayed, cell);
             markers = eina_matrixsparse_cell_data_get(cell);
             EINA_LIST_FOREACH(markers, l, group)
               {
                  if (!group->markers) continue;
                  if (group->clas->zoom_displayed > wd->zoom) continue;

                  xx = group->x;
                  yy = group->y;
                  ww = group->w;
                  hh = group->h;

                  if (eina_list_count(group->markers) == 1)
                    {
                       Elm_Map_Marker *m = eina_list_data_get(group->markers);
                       ww = m->clas->priv.edje_w;
                       hh = m->clas->priv.edje_h;
                    }

                  if (ww <= 0) ww = 1;
                  if (hh <= 0) hh = 1;

                  if ((gw != g->w) && (g->w > 0))
                    {
                       tx = xx;
                       xx = ((long long )gw * xx) / g->w;
                       ww = (((long long)gw * (tx + ww)) / g->w) - xx;
                    }
                  if ((gh != g->h) && (g->h > 0))
                    {
                       ty = yy;
                       yy = ((long long)gh * yy) / g->h;
                       hh = (((long long)gh * (ty + hh)) / g->h) - yy;
                    }

                  if ((!group->clas->hide)
                      && (xx-px+ax+ox >= ox) && (xx-px+ax+ox<= ox+ow)
                      && (yy-py+ay+oy >= oy) && (yy-py+ay+oy<= oy+oh))
                    {
                       if (!group->obj) _group_object_create(group);

                       if (group->update_nbelems)
                         {
                            group->update_nbelems = EINA_FALSE;
                            if (eina_list_count(group->markers) > 1)
                              {
                                 snprintf(buf, sizeof(buf), "%d", eina_list_count(group->markers));
                                 edje_object_part_text_set(elm_layout_edje_get(group->obj), "elm.text", buf);
                              }
                            else
                              edje_object_part_text_set(elm_layout_edje_get(group->obj), "elm.text", "");
                         }
                       evas_object_move(group->obj,
                                        xx - px + ax + ox - ww/2,
                                        yy - py + ay + oy - hh/2);
                       if ((!wd->paused_markers) || (group->update_resize))
                         {
                            group->update_resize = EINA_FALSE;
                            evas_object_resize(group->obj, ww, hh);
                            obj_rotate_zoom(obj, group->obj);
                         }
                       if (group->update_raise)
                         {
                            group->update_raise = EINA_FALSE;
                            evas_object_raise(group->obj);
                            obj_rotate_zoom(obj, group->obj);
                            evas_object_show(group->obj);
                         }
                       if (group->bubble) _group_bubble_place(group);
                    }
                  else if (group->obj)
                    {
                       _group_object_free(group);
                    }
               }
          }
     }
}

static void
grid_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ax, ay, gw, gh, tx, ty;
   int xx, yy, ww, hh;

   if (!wd) return;
   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;
   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;

   Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
   Eina_Matrixsparse_Cell *cell;

   EINA_ITERATOR_FOREACH(it, cell)
     {
        Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);

        xx = gi->out.x;
        yy = gi->out.y;
        ww = gi->out.w;
        hh = gi->out.h;
        if ((gw != g->w) && (g->w > 0))
          {
             tx = xx;
             xx = ((long long )gw * xx) / g->w;
             ww = (((long long)gw * (tx + ww)) / g->w) - xx;
          }
        if ((gh != g->h) && (g->h > 0))
          {
             ty = yy;
             yy = ((long long)gh * yy) / g->h;
             hh = (((long long)gh * (ty + hh)) / g->h) - yy;
          }
        evas_object_move(gi->img,
                         xx - px + ax + ox,
                         yy - py + ay + oy);

        evas_object_resize(gi->img, ww, hh);

        obj_rotate_zoom(obj, gi->img);
        /*evas_object_move(gi->txt,
                           xx - px + ax + ox,
                           yy - py + ay + oy);

          evas_object_resize(gi->txt, ww, hh);
         */
     }
   eina_iterator_free(it);
}

static void
_tile_update(Grid_Item *gi)
{
   evas_object_image_file_set(gi->img, gi->file, NULL);
   Evas_Load_Error err = evas_object_image_load_error_get(gi->img);
   if (err != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Image loading error (%s): %s", gi->file, evas_load_error_str(err));
        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
     }
   else
     {
        obj_rotate_zoom(gi->wd->obj, gi->img);
        evas_object_show(gi->img);
        gi->file_have = EINA_TRUE;
        //evas_object_text_text_set(gi->txt, gi->file);
        //evas_object_show(gi->txt);
     }
}

static void
_tile_downloaded(void *data, const char *file __UNUSED__, int status)
{
   Grid_Item *gi = data;

   gi->job = NULL;

   if (status == 200)
     {
        DBG("Download success from %s to %s", gi->source, gi->file);
        _tile_update(gi);
        gi->wd->finish_num++;
     }
   else
     {
        WRN("Download failed from %s to %s (%d) ", gi->source, gi->file, status);
        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
     }

   gi->wd->download_num--;
   evas_object_smart_callback_call(gi->wd->obj, SIG_DOWNLOADED, NULL);
   if (!gi->wd->download_num)
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(gi->wd->scr), "elm,state,busy,stop", "elm");
        evas_object_smart_callback_call(gi->wd->obj, SIG_LOADED_DETAIL, NULL);
     }
   _process_download_list(gi->wd->obj);
}

static void
_process_download_list(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l, *ll;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh, tx, ty, gw, gh, xx, yy, ww, hh;
   Grid_Item *gi;

   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(wd->obj), &cvx, &cvy, &cvw, &cvh);

   gw = wd->size.w;
   gh = wd->size.h;

   EINA_LIST_REVERSE_FOREACH_SAFE(wd->download_list, l, ll, gi)
     {
        xx = gi->out.x;
        yy = gi->out.y;
        ww = gi->out.w;
        hh = gi->out.h;

        if ((gw != gi->g->w) && (gi->g->w > 0))
          {
             tx = xx;
             xx = ((long long )gw * xx) / gi->g->w;
             ww = (((long long)gw * (tx + ww)) / gi->g->w) - xx;
          }
        if ((gh != gi->g->h) && (gi->g->h > 0))
          {
             ty = yy;
             yy = ((long long)gh * yy) / gi->g->h;
             hh = (((long long)gh * (ty + hh)) / gi->g->h) - yy;
          }
        if (!ELM_RECTS_INTERSECT(xx - wd->pan_x + ox,
                                 yy  - wd->pan_y + oy,
                                 ww, hh,
                                 cvx, cvy, cvw, cvh) ||
           (gi->zoom != wd->zoom))
          {
             wd->download_list = eina_list_remove(wd->download_list, gi);
             continue;
          }

        if (gi->wd->download_num >= MAX_CONCURRENT_DOWNLOAD) break;

        Eina_Bool ret = ecore_file_download_full(gi->source, gi->file, _tile_downloaded, NULL, gi, &(gi->job), wd->ua);
        if (!ret || !gi->job) ERR("Can't start to download from %s to %s", gi->source, gi->file);
        else
          {
             gi->wd->download_num++;
             wd->try_num++;
             wd->download_list = eina_list_remove(wd->download_list, gi);
             if (wd->download_num == 1)
               edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr), "elm,state,busy,start", "elm");
          }
     }
}

static void
_add_download_list(Evas_Object *obj, Grid_Item *gi)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   wd->download_list = eina_list_remove(wd->download_list, gi);
   wd->download_list = eina_list_append(wd->download_list, gi);
   _process_download_list(obj);
}

static void
grid_create_all(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;
   int zoom = 0;

   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src);

   for (zoom = wd->src->zoom_min; zoom <= wd->src->zoom_max; zoom++)
     {
        g = calloc(1, sizeof(Grid));
        EINA_SAFETY_ON_NULL_RETURN(g);
        g->zoom = zoom;
        g->tsize = wd->tsize;
        g->wd = wd;
        int size =  pow(2.0, g->zoom);
        g->gw = size;
        g->gh = size;
        g->w = g->tsize * g->gw;
        g->h = g->tsize * g->gh;

        g->grid = eina_matrixsparse_new(g->gh, g->gw, NULL, NULL);
        wd->grids = eina_list_append(wd->grids, g);
     }
}

static void
grid_clear_all(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;
   Grid_Item *gi;

   EINA_SAFETY_ON_NULL_RETURN(wd);

   EINA_LIST_FREE(wd->grids, g)
     {
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
        Eina_Matrixsparse_Cell *cell;

        EINA_ITERATOR_FOREACH(it, cell)
          {
             gi = eina_matrixsparse_cell_data_get(cell);
             evas_object_del(gi->img);
             //evas_object_del(gi->txt);

             if (gi->job)
               {
                  DBG("DOWNLOAD abort %s", gi->file);
                  ecore_file_download_abort(gi->job);
                  ecore_file_remove(gi->file);
                  gi->file_have = EINA_FALSE;
                  gi->job = NULL;
                  wd->try_num--;
               }
             if (gi->file)   eina_stringshare_del(gi->file);
             if (gi->source) eina_stringshare_del(gi->source);
             free(gi);
          }
        eina_matrixsparse_free(g->grid);
        eina_iterator_free(it);
        free(g);
     }

   EINA_LIST_FREE(wd->download_list, gi);
   if (!ecore_file_recursive_rm("/tmp/elm_map/")) WRN("Deletion of /tmp/elm_map/ failed");

}

static void
grid_unload(Evas_Object *obj, Grid *g)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   Eina_Iterator *it;
   Eina_Matrixsparse_Cell *cell;
   Grid_Item *gi;

   EINA_SAFETY_ON_NULL_RETURN(wd);

   it = eina_matrixsparse_iterator_new(g->grid);
   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);

        if (gi->file_have)
          {
             evas_object_hide(gi->img);
             //evas_object_hide(gi->txt);
             evas_object_image_file_set(gi->img, NULL, NULL);
          }
        else if (gi->job)
          {
             DBG("DOWNLOAD abort %s", gi->file);
             ecore_file_download_abort(gi->job);
             ecore_file_remove(gi->file);
             gi->job = NULL;
             wd->try_num--;
          }
     }
   eina_iterator_free(it);
}


static void
grid_load(Evas_Object *obj, Grid *g)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int x, y;
   int size;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh, tx, ty, gw, gh, xx, yy, ww, hh;
   Eina_Iterator *it;
   Eina_Matrixsparse_Cell *cell;
   Grid_Item *gi;

   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src);

   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(wd->obj), &cvx, &cvy, &cvw, &cvh);

   gw = wd->size.w;
   gh = wd->size.h;

   if ((gw <= 0) || (gh <= 0)) return;

   size = g->tsize;
   if ((gw != g->w) && (g->w > 0))
     size = ((long long)gw * size) / g->w;
   if (size < (g->tsize / 2)) return; // else we will load to much tiles

   it = eina_matrixsparse_iterator_new(g->grid);

   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);

        xx = gi->out.x;
        yy = gi->out.y;
        ww = gi->out.w;
        hh = gi->out.h;

        if ((gw != g->w) && (g->w > 0))
          {
             tx = xx;
             xx = ((long long )gw * xx) / g->w;
             ww = (((long long)gw * (tx + ww)) / g->w) - xx;
          }
        if ((gh != g->h) && (g->h > 0))
          {
             ty = yy;
             yy = ((long long)gh * yy) / g->h;
             hh = (((long long)gh * (ty + hh)) / g->h) - yy;
          }

        if (!ELM_RECTS_INTERSECT(xx - wd->pan_x + ox,
                                 yy  - wd->pan_y + oy,
                                 ww, hh,
                                 cvx, cvy, cvw, cvh))
          {
             if (gi->file_have)
               {
                  evas_object_hide(gi->img);
                  //evas_object_hide(gi->txt);
                  evas_object_image_file_set(gi->img, NULL, NULL);
               }
             else if (gi->job)
               {
                  DBG("Download abort %s", gi->file);
                  ecore_file_download_abort(gi->job);
                  ecore_file_remove(gi->file);
                  gi->job = NULL;
                  wd->try_num--;
               }
          }
     }
   eina_iterator_free(it);

   xx = wd->pan_x / size - 1;
   if (xx < 0) xx = 0;

   yy = wd->pan_y / size - 1;
   if (yy < 0) yy = 0;

   ww = ow / size + 2;
   if (xx + ww >= g->gw) ww = g->gw - xx - 1;

   hh = oh / size + 2;
   if (yy + hh >= g->gh) hh = g->gh - yy - 1;

   for (y = yy; y <= yy + hh; y++)
     {
        for (x = xx; x <= xx + ww; x++)
          {
             gi = eina_matrixsparse_data_idx_get(g->grid, y, x);

             if (!gi)
               {
                  char buf[PATH_MAX];
                  char buf2[PATH_MAX];
                  char *source;

                  gi = calloc(1, sizeof(Grid_Item));
                  EINA_SAFETY_ON_NULL_RETURN(gi);

                  gi->wd = wd;
                  gi->g = g;
                  gi->zoom = g->zoom;
                  gi->file_have = EINA_FALSE;
                  gi->job = NULL;

                  gi->src.x = x * g->tsize;
                  gi->src.y = y * g->tsize;
                  gi->src.w = g->tsize;
                  gi->src.h = g->tsize;

                  gi->out.x = gi->src.x;
                  gi->out.y = gi->src.y;
                  gi->out.w = gi->src.w;
                  gi->out.h = gi->src.h;

                  gi->img = evas_object_image_add(evas_object_evas_get(obj));
                  evas_object_image_scale_hint_set(gi->img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
                  evas_object_image_filled_set(gi->img, 1);

                  evas_object_smart_member_add(gi->img, wd->pan_smart);
                  elm_widget_sub_object_add(obj, gi->img);
                  evas_object_pass_events_set(gi->img, EINA_TRUE);
                  evas_object_stack_below(gi->img, wd->sep_maps_markers);

/*                gi->txt = evas_object_text_add(evas_object_evas_get(obj));
                  evas_object_text_font_set(gi->txt, "Vera", 12);
                  evas_object_color_set(gi->txt, 100, 100, 100, 255);
                  evas_object_smart_member_add(gi->txt,
                                               wd->pan_smart);
                  elm_widget_sub_object_add(obj, gi->txt);
                  evas_object_pass_events_set(gi->txt, EINA_TRUE);
*/
                  snprintf(buf, sizeof(buf), CACHE_PATH, wd->id, g->zoom, x);
                  snprintf(buf2, sizeof(buf2), CACHE_FILE_PATH, buf, y);
                  if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);

                  eina_stringshare_replace(&gi->file, buf2);
                  source = wd->src->url_cb(obj, x, y, g->zoom);
                  if ((!source) || (strlen(source)==0))
                    {
                       eina_stringshare_replace(&gi->source, NULL);
                       WRN("Getting source url failed: %s", gi->file);
                    }
                  else eina_stringshare_replace(&gi->source, source);
                  if (source) free(source);

                  eina_matrixsparse_data_idx_set(g->grid, y, x, gi);
               }

               if (gi->file_have)
                 {
                    DBG("File exists: %s", gi->file);
                    _tile_update(gi);
                 }
               else if (!gi->job)
                 {
                    DBG("Added to download list: %s", gi->file);
                    _add_download_list(obj, gi);
                 }
          }
     }
}

static void
_smooth_update(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Grid *g;

   if (!wd) return;
   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
        Eina_Matrixsparse_Cell *cell;

        EINA_ITERATOR_FOREACH(it, cell)
          {
             Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);
             evas_object_image_smooth_scale_set(gi->img, (!wd->nosmooth));
          }
        eina_iterator_free(it);
     }
}

static Eina_Bool
_scr_timeout(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype) ECORE_CALLBACK_CANCEL;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->nosmooth--;
   if (!wd->nosmooth) _smooth_update(data);
   wd->scr_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_scr(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;
   if (!wd->scr_timer)
     {
        wd->nosmooth++;
        if (wd->nosmooth == 1) _smooth_update(data);
     }
   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   wd->scr_timer = ecore_timer_add(0.5, _scr_timeout, data);
}

static void
zoom_do(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord xx, yy, ow, oh;

   if (!wd) return;
   wd->size.w = wd->size.nw;
   wd->size.h = wd->size.nh;

   elm_smart_scroller_child_viewport_size_get(wd->scr, &ow, &oh);

   if (wd->center_on.enabled)
     {
        elm_map_utils_convert_geo_into_coord(obj, wd->center_on.lon, wd->center_on.lat, wd->size.w, &xx, &yy);
        xx -= ow / 2;
        yy -= oh / 2;
     }
   else
     {
        xx = (wd->size.spos.x * wd->size.w) - (ow / 2);
        yy = (wd->size.spos.y * wd->size.h) - (oh / 2);
     }


   if (xx < 0) xx = 0;
   else if (xx > (wd->size.w - ow)) xx = wd->size.w - ow;
   if (yy < 0) yy = 0;
   else if (yy > (wd->size.h - oh)) yy = wd->size.h - oh;

   wd->show.show = EINA_TRUE;
   wd->show.x = xx;
   wd->show.y = yy;
   wd->show.w = ow;
   wd->show.h = oh;

   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
}

static Eina_Bool
_zoom_timeout(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype) ECORE_CALLBACK_CANCEL;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->zoom_timer = NULL;
   wd->pinch.level = 1.0;
   zoom_do(data);
   evas_object_smart_callback_call(data, SIG_ZOOM_STOP, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_zoom_anim(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype) ECORE_CALLBACK_CANCEL;
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ECORE_CALLBACK_CANCEL;
   if (wd->zoom_method == ZOOM_METHOD_IN) wd->t += 0.1 ;
   else if (wd->zoom_method == ZOOM_METHOD_OUT) wd->t -= 0.05;
   else
     {
        wd->zoom_animator = NULL;
        zoom_do(obj);
        evas_object_smart_callback_call(data, SIG_ZOOM_STOP, NULL);
        return ECORE_CALLBACK_CANCEL;
     }

   if (wd->t >= 2.0)
     {
        wd->zoom_animator = NULL;
        wd->pinch.level = 2.0;
        if (wd->zoom_timer) ecore_timer_del(wd->zoom_timer);
        wd->zoom_timer = ecore_timer_add(0.35, _zoom_timeout, obj);
        return ECORE_CALLBACK_CANCEL;
     }
   else if (wd->t <= 0.5)
     {
        wd->zoom_animator = NULL;
        wd->pinch.level = 0.5;
        if (wd->zoom_timer) ecore_timer_del(wd->zoom_timer);
        wd->zoom_timer = ecore_timer_add(1.35, _zoom_timeout, obj);
        return ECORE_CALLBACK_CANCEL;
     }
   else if (wd->t != 1.0)
     {
        Evas_Coord x, y, w, h;
        float half_w, half_h;
        evas_object_geometry_get(data, &x, &y, &w, &h);
        half_w = (float)w * 0.5;
        half_h = (float)h * 0.5;
        wd->pinch.cx = x + half_w;
        wd->pinch.cy = y + half_h;
        wd->pinch.level = wd->t;
        if (wd->calc_job) ecore_job_del(wd->calc_job);
        wd->calc_job = ecore_job_add(_calc_job, wd);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_long_press(void *data)
{
   ELM_CHECK_WIDTYPE(data, widtype) ECORE_CALLBACK_CANCEL;
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->long_timer = NULL;
   evas_object_smart_callback_call(data, SIG_LONGPRESSED, &wd->ev);
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(data, SIG_CLICKED_DOUBLE, ev);
   else
     evas_object_smart_callback_call(data, SIG_PRESS, ev);
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   wd->ev = *ev;
   wd->long_timer = ecore_timer_add(_elm_config->longpress_timeout, _long_press, data);
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Evas_Event_Mouse_Up *ev = event_info;
   EINA_SAFETY_ON_NULL_RETURN(ev);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (wd->long_timer)
     {
        ecore_timer_del(wd->long_timer);
        wd->long_timer = NULL;
     }
   if (!wd->on_hold) evas_object_smart_callback_call(data, SIG_CLICKED, ev);
   wd->on_hold = EINA_FALSE;
}

static void
_mouse_wheel_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if (!wd->paused)
     {
        int zoom_diff = 0;
        Evas_Event_Mouse_Wheel *ev = (Evas_Event_Mouse_Wheel*) event_info;
        Evas_Coord x, y, w, h;

        evas_object_geometry_get(data, &x, &y, &w, &h);

        if (wd->calc_job) ecore_job_del(wd->calc_job);
        wd->calc_job = ecore_job_add(_calc_job, wd);

        wd->wheel_diff -= ev->z;
        wd->pinch.level = wd->pinch.diff * pow(2.0, (double)wd->wheel_diff/10);
        wd->pinch.cx = x + ((double)w * 0.5);
        wd->pinch.cy = y + ((double)h * 0.5);

        if (wd->pinch.level > 2.0 || wd->pinch.level < 1.0)
          {
             wd->wheel_diff = 0;
             if (wd->pinch.level > 2.0)
               {
                  zoom_diff = 1;
                  wd->pinch.diff = 1.0;
                  wd->pinch.level = 1.0;
               }
             else if (wd->pinch.level < 1.0)
               {
                  zoom_diff = -1;
                  wd->pinch.diff = 2.0;
                  wd->pinch.level = 2.0;
               }

             Elm_Map_Zoom_Mode temp;
             temp = wd->mode;
             wd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
             wd->paused = EINA_TRUE;
             elm_map_zoom_set(data, wd->zoom + zoom_diff);
             wd->paused = EINA_FALSE;
             wd->mode = temp;
          }
        else
          {
             if (wd->calc_job) ecore_job_del(wd->calc_job);
             wd->calc_job = ecore_job_add(_calc_job, wd);
          }
     }
}

static void
_rect_resize_cb(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(data, widtype);
   Widget_Data *wd = elm_widget_data_get(data);
   int x, y, w, h;

   evas_object_geometry_get(wd->rect, &x, &y, &w, &h);
   evas_object_geometry_get(wd->pan_smart, &x, &y, &w, &h);
   evas_object_resize(wd->rect, w, h);
   evas_object_move(wd->rect, x, y);
}

static void
_del_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Elm_Map_Group_Class *group_clas;
   Elm_Map_Marker_Class *marker_clas;
   Eina_List *l;
   Evas_Object *p;
   Path_Node *n;
   Path_Waypoint *w;
   Ecore_Event_Handler *h;
   Elm_Map_Route *r;
   Elm_Map_Name *na;
   Evas_Object *route;

   if (!wd) return;
   EINA_LIST_FREE(wd->groups_clas, group_clas)
     {
        if (group_clas->style)
          eina_stringshare_del(group_clas->style);
        free(group_clas);
     }

   EINA_LIST_FREE(wd->markers_clas, marker_clas)
     {
        if (marker_clas->style)
          eina_stringshare_del(marker_clas->style);
        free(marker_clas);
     }

   EINA_LIST_FOREACH(wd->route, l, r)
     {
        EINA_LIST_FREE(r->path, p)
          {
             evas_object_del(p);
          }

        EINA_LIST_FREE(r->waypoint, w)
          {
             if (w->point) eina_stringshare_del(w->point);
             free(w);
          }

        EINA_LIST_FREE(r->nodes, n)
          {
             if (n->pos.address) eina_stringshare_del(n->pos.address);
             free(n);
          }

        EINA_LIST_FREE(r->handlers, h)
          {
             ecore_event_handler_del(h);
          }

        if (r->con_url) ecore_con_url_free(r->con_url);
        if (r->info.nodes) eina_stringshare_del(r->info.nodes);
        if (r->info.waypoints) eina_stringshare_del(r->info.waypoints);
     }

   EINA_LIST_FREE(wd->names, na)
     {
        if (na->address) free(na->address);
        if (na->handler) ecore_event_handler_del(na->handler);
        if (na->ud.fname)
          {
             ecore_file_remove(na->ud.fname);
             free(na->ud.fname);
             na->ud.fname = NULL;
          }
     }

   EINA_LIST_FREE(wd->track, route)
     {
        evas_object_del(route);
     }

   if (wd->map) evas_map_free(wd->map);
   if (wd->source_names) free(wd->source_names);
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   if (wd->user_agent) eina_stringshare_del(wd->user_agent);
   if (wd->ua) eina_hash_free(wd->ua);
   if (wd->markers) free(wd->markers);

   free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Marker_Group *group;
   Elm_Map_Marker *marker;
   int i;
   Eina_Bool free_marker = EINA_TRUE;
   Eina_List *l;

   if (!wd) return;
   grid_clear_all(obj);
   for (i = 0; i <= wd->zoom_max; i++)
     {
        if (!wd->markers[i]) continue;
        Eina_Iterator *it = eina_matrixsparse_iterator_new(wd->markers[i]);
        Eina_Matrixsparse_Cell *cell;

        EINA_ITERATOR_FOREACH(it, cell)
          {
             l =  eina_matrixsparse_cell_data_get(cell);
             EINA_LIST_FREE(l, group)
               {
                  EINA_LIST_FREE(group->markers, marker)
                    {
                       evas_object_event_callback_del_full(group->sc, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                                           _bubble_sc_hints_changed_cb, group);
                       if (free_marker) free(marker);
                    }
                  free(group);
               }
             free_marker = EINA_FALSE;
          }
        eina_iterator_free(it);
        eina_matrixsparse_free(wd->markers[i]);
     }

   evas_object_del(wd->sep_maps_markers);
   evas_object_del(wd->pan_smart);
   wd->pan_smart = NULL;
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr), "elm,action,focus", "elm");
        evas_object_focus_set(wd->obj, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr), "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->obj, EINA_FALSE);
     }
}

static void
_theme_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_smart_scroller_object_theme_set(obj, wd->scr, "map", "base", elm_widget_style_get(obj));
   //   edje_object_scale_set(wd->scr, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   if (!wd) return;
   evas_object_size_hint_max_get(wd->scr, &maxw, &maxh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_calc_job(void *data)
{
   Widget_Data *wd = data;
   Evas_Coord minw, minh;

   if (!wd) return;
   minw = wd->size.w;
   minh = wd->size.h;
   if (wd->resized)
     {
        wd->resized = EINA_FALSE;
        if (wd->mode != ELM_MAP_ZOOM_MODE_MANUAL)
          {
             double tz = wd->zoom;
             wd->zoom = 0.0;
             elm_map_zoom_set(wd->obj, tz);
          }
     }
   if ((minw != wd->minw) || (minh != wd->minh))
     {
        wd->minw = minw;
        wd->minh = minh;
        evas_object_smart_callback_call(wd->pan_smart, SIG_CHANGED, NULL);
        _sizing_eval(wd->obj);
     }
   wd->calc_job = NULL;
   evas_object_smart_changed(wd->pan_smart);
}

static void
_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((x == sd->wd->pan_x) && (y == sd->wd->pan_y)) return;
   sd->wd->pan_x = x;
   sd->wd->pan_y = y;
   evas_object_smart_changed(obj);
}

static void
_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (x) *x = sd->wd->pan_x;
   if (y) *y = sd->wd->pan_y;
}

static void
_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;
   if (!sd) return;
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ow = sd->wd->minw - ow;
   if (ow < 0) ow = 0;
   oh = sd->wd->minh - oh;
   if (oh < 0) oh = 0;
   if (x) *x = ow;
   if (y) *y = oh;
}

static void
_pan_min_get(Evas_Object *obj __UNUSED__, Evas_Coord *x, Evas_Coord *y)
{
   if (x) *x = 0;
   if (y) *y = 0;
}

static void
_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (w) *w = sd->wd->minw;
   if (h) *h = sd->wd->minh;
}

static void
_pan_add(Evas_Object *obj)
{
   Pan *sd;
   Evas_Object_Smart_Clipped_Data *cd;
   _pan_sc.add(obj);
   cd = evas_object_smart_data_get(obj);
   if (!cd) return;
   sd = calloc(1, sizeof(Pan));
   if (!sd) return;
   sd->__clipped_data = *cd;
   free(cd);
   evas_object_smart_data_set(obj, sd);
}

static void
_pan_del(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   _pan_sc.del(obj);
}

static void
_pan_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ow, oh;
   if (!sd) return;
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   sd->wd->resized = EINA_TRUE;
   if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
   sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_pan_calculate(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;
   Eina_List *l;
   Grid *g;

   EINA_SAFETY_ON_NULL_RETURN(sd);
   EINA_SAFETY_ON_NULL_RETURN(sd->wd);

   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   rect_place(sd->wd->obj, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
   EINA_LIST_FOREACH(sd->wd->grids, l, g)
     {
        if (sd->wd->zoom == g->zoom) grid_load(sd->wd->obj, g);
        else if (sd->wd->zoom-1 != g->zoom && sd->wd->zoom+1 != g->zoom) grid_unload(sd->wd->obj, g); // remain only adjacent grids
        grid_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
        if (sd->wd->zoom == g->zoom) marker_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
        if (!sd->wd->zoom_animator) route_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
        if (!sd->wd->zoom_animator) track_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
     }
}

static void
_pan_move(Evas_Object *obj, Evas_Coord x __UNUSED__, Evas_Coord y __UNUSED__)
{
   Pan *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->wd->calc_job) ecore_job_del(sd->wd->calc_job);
   sd->wd->calc_job = ecore_job_add(_calc_job, sd->wd);
}

static void
_hold_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 1);
}

static void
_hold_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 0);
}

static void
_freeze_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 1);
}

static void
_freeze_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 0);
}

static void
_scr_anim_start(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, "scroll,anim,start", NULL);
}

static void
_scr_anim_stop(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, "scroll,anim,stop", NULL);
}

static void
_scr_drag_start(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   wd->center_on.enabled = EINA_FALSE;

   // FIXME: els_scoller sometimes give start event again & again... it confuses app. (els_scr bug?)
   if (!wd->scr_started)
     {
        wd->scr_started = EINA_TRUE;
        evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_START, NULL);
     }
}

static void
_scr_drag_stop(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   wd->center_on.enabled = EINA_FALSE;

   // FIXME: els_scoller sometimes give start event again & again... it confuses app. (els_scr bug?)
   if (wd->scr_started)
     {
        wd->scr_started = EINA_FALSE;
        evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_STOP, NULL);
     }
}

static void
_scr_scroll(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL, NULL);
}


static void
_group_object_create(Marker_Group *group)
{
   const char *style = "radio";
   Evas_Object *icon = NULL;

   if (group->obj) return;
   if ((!group->clas->priv.objs_notused) || (eina_list_count(group->markers) == 1))
     {
        //set icon and style
        if (eina_list_count(group->markers) == 1)
          {
             Elm_Map_Marker *m = eina_list_data_get(group->markers);
             if (m->clas->style)
               style = m->clas->style;

             if (m->clas->func.icon_get)
               icon = m->clas->func.icon_get(group->wd->obj, m, m->data);

             group->delete_object = EINA_TRUE;
          }
        else
          {
             if (group->clas->style)
               style = group->clas->style;

             if (group->clas->func.icon_get)
               icon = group->clas->func.icon_get(group->wd->obj, group->clas->data);

             group->delete_object = EINA_FALSE;
          }

        group->obj = elm_layout_add(group->wd->obj);
        elm_layout_theme_set(group->obj, "map/marker", style, elm_widget_style_get(group->wd->obj));

        if (icon) elm_object_part_content_set(group->obj, "elm.icon", icon);

        evas_object_smart_member_add(group->obj, group->wd->pan_smart);
        elm_widget_sub_object_add(group->wd->obj, group->obj);
        evas_object_stack_above(group->obj, group->wd->sep_maps_markers);

        if (!group->delete_object)
          group->clas->priv.objs_used = eina_list_append(group->clas->priv.objs_used, group->obj);
     }
   else
     {
        group->delete_object = EINA_FALSE;

        group->obj = eina_list_data_get(group->clas->priv.objs_notused);
        group->clas->priv.objs_used = eina_list_append(group->clas->priv.objs_used, group->obj);
        group->clas->priv.objs_notused = eina_list_remove(group->clas->priv.objs_notused, group->obj);
        evas_object_show(group->obj);
     }

   edje_object_signal_callback_add(elm_layout_edje_get(group->obj), "open", "elm", _group_open_cb, group);
   edje_object_signal_callback_add(elm_layout_edje_get(group->obj), "bringin", "elm", _group_bringin_cb, group);

   group->update_nbelems = EINA_TRUE;
   group->update_resize = EINA_TRUE;
   group->update_raise = EINA_TRUE;

   if (group->open) _group_bubble_create(group);
}

static void
_group_object_free(Marker_Group *group)
{
   if (!group->obj) return;
   if (!group->delete_object)
     {
        group->clas->priv.objs_notused = eina_list_append(group->clas->priv.objs_notused, group->obj);
        group->clas->priv.objs_used = eina_list_remove(group->clas->priv.objs_used, group->obj);
        evas_object_hide(group->obj);

        edje_object_signal_callback_del(elm_layout_edje_get(group->obj), "open", "elm", _group_open_cb);
        edje_object_signal_callback_del(elm_layout_edje_get(group->obj), "bringin", "elm", _group_bringin_cb);
     }
   else
     evas_object_del(group->obj);

   group->obj = NULL;
   _group_bubble_free(group);
}

static void
_group_bubble_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Group *group = data;

   if (!evas_object_above_get(group->rect)) return;
   evas_object_raise(group->bubble);
   evas_object_raise(group->sc);
   evas_object_raise(group->rect);
}

static void
_group_bubble_create(Marker_Group *group)
{
   if (group->bubble) return;

   group->wd->opened_bubbles = eina_list_append(group->wd->opened_bubbles, group);
   group->bubble = edje_object_add(evas_object_evas_get(group->obj));
   _elm_theme_object_set(group->wd->obj, group->bubble, "map", "marker_bubble",
                         elm_widget_style_get(group->wd->obj));
   evas_object_smart_member_add(group->bubble,
                                group->wd->obj);
   elm_widget_sub_object_add(group->wd->obj, group->bubble);

   _group_bubble_content_free(group);
   if (!_group_bubble_content_update(group))
     {
        //no content, we can delete the bubble
        _group_bubble_free(group);
        return;
     }

   group->rect = evas_object_rectangle_add(evas_object_evas_get(group->obj));
   evas_object_color_set(group->rect, 0, 0, 0, 0);
   evas_object_repeat_events_set(group->rect, EINA_TRUE);
   evas_object_smart_member_add(group->rect, group->wd->obj);
   elm_widget_sub_object_add(group->wd->obj, group->rect);

   evas_object_event_callback_add(group->rect, EVAS_CALLBACK_MOUSE_UP, _group_bubble_mouse_up_cb, group);

   _group_bubble_place(group);
}

static void _bubble_sc_hints_changed_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _group_bubble_place(data);
}

static int
_group_bubble_content_update(Marker_Group *group)
{
   Eina_List *l;
   Elm_Map_Marker *marker;
   int i = 0;

   if (!group->bubble) return 1;

   if (!group->sc)
     {
        group->sc = elm_scroller_add(group->bubble);
        elm_widget_style_set(group->sc, "map_bubble");
        elm_scroller_content_min_limit(group->sc, EINA_FALSE, EINA_TRUE);
        elm_scroller_policy_set(group->sc, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        elm_scroller_bounce_set(group->sc, _elm_config->thumbscroll_bounce_enable, EINA_FALSE);
        edje_object_part_swallow(group->bubble, "elm.swallow.content", group->sc);
        evas_object_show(group->sc);
        evas_object_smart_member_add(group->sc,
                                     group->wd->obj);
        elm_widget_sub_object_add(group->wd->obj, group->sc);

        group->bx = elm_box_add(group->bubble);
        evas_object_size_hint_align_set(group->bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(group->bx, 0.5, 0.5);
        elm_box_horizontal_set(group->bx, EINA_TRUE);
        evas_object_show(group->bx);

        elm_object_content_set(group->sc, group->bx);

        evas_object_event_callback_add(group->sc, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _bubble_sc_hints_changed_cb, group);
     }

   EINA_LIST_FOREACH(group->markers, l, marker)
     {
        if (i >= group->wd->markers_max_num) break;
        if ((!marker->content) && (marker->clas->func.get))
          marker->content = marker->clas->func.get(group->wd->obj, marker, marker->data);
        else if (marker->content)
          elm_box_unpack(group->bx, marker->content);
        if (marker->content)
          {
             elm_box_pack_end(group->bx, marker->content);
             i++;
          }
     }
   return i;
}

static void
_group_bubble_content_free(Marker_Group *group)
{
   Eina_List *l;
   Elm_Map_Marker *marker;

   if (!group->sc) return;
   EINA_LIST_FOREACH(group->markers, l, marker)
     {
        if ((marker->content) && (marker->clas->func.del))
          marker->clas->func.del(group->wd->obj, marker, marker->data, marker->content);
        else if (marker->content)
          evas_object_del(marker->content);
        marker->content = NULL;
     }
   evas_object_del(group->sc);
   group->sc = NULL;
}

static void
_group_bubble_free(Marker_Group *group)
{
   if (!group->bubble) return;
   group->wd->opened_bubbles = eina_list_remove(group->wd->opened_bubbles, group);
   evas_object_event_callback_del_full(group->sc, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _bubble_sc_hints_changed_cb, group);
   evas_object_del(group->bubble);
   evas_object_del(group->rect);
   group->bubble = NULL;
   _group_bubble_content_free(group);
}

static void
_group_bubble_place(Marker_Group *group)
{
   Evas_Coord x, y, w;
   Evas_Coord xx, yy, ww, hh;
   const char *s;

   if ((!group->bubble) || (!group->obj)) return;

   evas_object_geometry_get(group->obj, &x, &y, &w, NULL);
   edje_object_size_min_calc(group->bubble, NULL, &hh);

   s = edje_object_data_get(group->bubble, "size_w");
   if (s) ww = atoi(s);
   else ww = 0;
   xx = x + w / 2 - ww / 2;
   yy = y-hh;

   evas_object_move(group->bubble, xx, yy);
   evas_object_resize(group->bubble, ww, hh);
   obj_rotate_zoom(group->wd, group->bubble);
   evas_object_show(group->bubble);

   evas_object_move(group->rect, xx, yy);
   evas_object_resize(group->rect, ww, hh);
   obj_rotate_zoom(group->wd, group->rect);
   evas_object_show(group->rect);
}

static void
_group_bringin_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   Marker_Group *group = data;
   Elm_Map_Marker *marker = eina_list_data_get(group->markers);
   if (!marker) return;
   group->bringin = EINA_TRUE;
   elm_map_geo_region_bring_in(group->wd->obj, marker->longitude, marker->latitude);
}

static void
_group_open_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   Marker_Group *group = data;

   if (group->bringin)
     {
        group->bringin = EINA_FALSE;
        return;
     }

   if (group->bubble)
     {
        group->open = EINA_FALSE;
        _group_bubble_free(group);
        return;
     }
   group->open = EINA_TRUE;
   _group_bubble_create(group);
}

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   int zoom;
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;

   if (!wd) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;

   elm_smart_scroller_child_pos_get(wd->scr, &x, &y);
   elm_smart_scroller_step_size_get(wd->scr, &step_x, &step_y);
   elm_smart_scroller_page_size_get(wd->scr, &page_x, &page_y);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &v_w, &v_h);

   if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")))
     {
        x -= step_x;
     }
   else if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")))
     {
        x += step_x;
     }
   else if ((!strcmp(ev->keyname, "Up"))  || (!strcmp(ev->keyname, "KP_Up")))
     {
        y -= step_y;
     }
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
        y += step_y;
     }
   else if ((!strcmp(ev->keyname, "Prior")) || (!strcmp(ev->keyname, "KP_Prior")))
     {
        if (page_y < 0)
          y -= -(page_y * v_h) / 100;
        else
          y -= page_y;
     }
   else if ((!strcmp(ev->keyname, "Next")) || (!strcmp(ev->keyname, "KP_Next")))
     {
        if (page_y < 0)
          y += -(page_y * v_h) / 100;
        else
          y += page_y;
     }
   else if (!strcmp(ev->keyname, "KP_Add"))
     {
        zoom = elm_map_zoom_get(obj) + 1;
        elm_map_zoom_mode_set(obj, ELM_MAP_ZOOM_MODE_MANUAL);
        elm_map_zoom_set(obj, zoom);
        return EINA_TRUE;
     }
   else if (!strcmp(ev->keyname, "KP_Subtract"))
     {
        zoom = elm_map_zoom_get(obj) - 1;
        elm_map_zoom_mode_set(obj, ELM_MAP_ZOOM_MODE_MANUAL);
        elm_map_zoom_set(obj, zoom);
        return EINA_TRUE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   elm_smart_scroller_child_pos_set(wd->scr, x, y);

   return EINA_TRUE;
}

static Eina_Bool
cb_dump_name_attrs(void *data, const char *key, const char *value)
{
   Name_Dump *dump = (Name_Dump*)data;
   if (!dump) return EINA_FALSE;

   if (!strncmp(key, NOMINATIM_ATTR_LON, sizeof(NOMINATIM_ATTR_LON))) dump->lon = atof(value);
   else if (!strncmp(key, NOMINATIM_ATTR_LAT, sizeof(NOMINATIM_ATTR_LAT))) dump->lat = atof(value);

   return EINA_TRUE;
}


static Eina_Bool
cb_route_dump(void *data, Eina_Simple_XML_Type type, const char *value, unsigned offset __UNUSED__, unsigned length)
{
   Route_Dump *dump = data;
   if (!dump) return EINA_FALSE;

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
      case EINA_SIMPLE_XML_OPEN_EMPTY:
        {
           const char *attrs;

           attrs = eina_simple_xml_tag_attributes_find(value, length);
           if (!attrs)
             {
                if (!strncmp(value, YOURS_DISTANCE, length)) dump->id = ROUTE_XML_DISTANCE;
                else if (!strncmp(value, YOURS_DESCRIPTION, length)) dump->id = ROUTE_XML_DESCRIPTION;
                else if (!strncmp(value, YOURS_COORDINATES, length)) dump->id = ROUTE_XML_COORDINATES;
                else dump->id = ROUTE_XML_NONE;
             }
         }
        break;
      case EINA_SIMPLE_XML_DATA:
        {
           char *buf = malloc(length);
           if (!buf) return EINA_FALSE;
           snprintf(buf, length, "%s", value);
           if (dump->id == ROUTE_XML_DISTANCE) dump->distance = atof(buf);
           else if (!(dump->description) && (dump->id == ROUTE_XML_DESCRIPTION)) dump->description = strdup(buf);
           else if (dump->id == ROUTE_XML_COORDINATES) dump->coordinates = strdup(buf);
           free(buf);
        }
        break;
      default:
        break;
     }

   return EINA_TRUE;
}

static Eina_Bool
cb_name_dump(void *data, Eina_Simple_XML_Type type, const char *value, unsigned offset __UNUSED__, unsigned length)
{
   Name_Dump *dump = data;
   if (!dump) return EINA_FALSE;

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
      case EINA_SIMPLE_XML_OPEN_EMPTY:
        {
           const char *attrs;
           attrs = eina_simple_xml_tag_attributes_find(value, length);
           if (attrs)
             {
                if (!strncmp(value, NOMINATIM_RESULT, sizeof(NOMINATIM_RESULT) - 1)) dump->id = NAME_XML_NAME;
                else dump->id = NAME_XML_NONE;

                eina_simple_xml_attributes_parse
                  (attrs, length - (attrs - value), cb_dump_name_attrs, dump);
             }
        }
        break;
      case EINA_SIMPLE_XML_DATA:
        {
           char *buf = malloc(length + 1);
           if (!buf) return EINA_FALSE;
           snprintf(buf, length + 1, "%s", value);
           if (dump->id == NAME_XML_NAME) dump->address = strdup(buf);
           free(buf);
        }
        break;
      default:
        break;
     }

   return EINA_TRUE;
}

static void
_parse_kml(void *data)
{
   Elm_Map_Route *r = (Elm_Map_Route*)data;
   if (!r || !r->ud.fname) return;

   FILE *f;
   char **str;
   unsigned int ele, idx;
   double lon, lat;
   Evas_Object *path;

   Route_Dump dump = {0, r->ud.fname, 0.0, NULL, NULL};

   f = fopen(r->ud.fname, "rb");
   if (f)
     {
        long sz;

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > 0)
          {
             char *buf;

             fseek(f, 0, SEEK_SET);
             buf = malloc(sz);
             if (buf)
               {
                  if (fread(buf, 1, sz, f))
                    {
                       eina_simple_xml_parse(buf, sz, EINA_TRUE, cb_route_dump, &dump);
                       free(buf);
                    }
               }
          }
        fclose(f);

        if (dump.distance) r->info.distance = dump.distance;
        if (dump.description)
          {
             eina_stringshare_replace(&r->info.waypoints, dump.description);
             str = eina_str_split_full(dump.description, "\n", 0, &ele);
             r->info.waypoint_count = ele;
             for (idx = 0 ; idx < ele ; idx++)
               {
                  Path_Waypoint *wp = ELM_NEW(Path_Waypoint);
                  if (wp)
                    {
                       wp->wd = r->wd;
                       wp->point = eina_stringshare_add(str[idx]);
                       DBG("%s", str[idx]);
                       r->waypoint = eina_list_append(r->waypoint, wp);
                    }
               }
             if (str && str[0])
               {
                  free(str[0]);
                  free(str);
               }
          }
        else WRN("description is not found !");

        if (dump.coordinates)
          {
             eina_stringshare_replace(&r->info.nodes, dump.coordinates);
             str = eina_str_split_full(dump.coordinates, "\n", 0, &ele);
             r->info.node_count = ele;
             for (idx = 0 ; idx < ele ; idx++)
               {
                  sscanf(str[idx], "%lf,%lf", &lon, &lat);
                  Path_Node *n = ELM_NEW(Path_Node);
                  if (n)
                    {
                       n->wd = r->wd;
                       n->pos.lon = lon;
                       n->pos.lat = lat;
                       n->idx = idx;
                       DBG("%lf:%lf", lon, lat);
                       n->pos.address = NULL;
                       r->nodes = eina_list_append(r->nodes, n);

                       path = evas_object_polygon_add(evas_object_evas_get(r->wd->obj));
                       evas_object_smart_member_add(path, r->wd->pan_smart);
                       r->path = eina_list_append(r->path, path);
                    }
               }
             if (str && str[0])
               {
                  free(str[0]);
                  free(str);
               }
          }
     }
}

static void
_parse_name(void *data)
{
   Elm_Map_Name *n = (Elm_Map_Name*)data;
   if (!n || !n->ud.fname) return;

   FILE *f;

   Name_Dump dump = {0, NULL, 0.0, 0.0};

   f = fopen(n->ud.fname, "rb");
   if (f)
     {
        long sz;

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > 0)
          {
             char *buf;

             fseek(f, 0, SEEK_SET);
             buf = malloc(sz);
             if (buf)
               {
                  if (fread(buf, 1, sz, f))
                    {
                       eina_simple_xml_parse(buf, sz, EINA_TRUE, cb_name_dump, &dump);
                       free(buf);
                    }
               }
          }
        fclose(f);

        if (dump.address)
          {
             INF("[%lf : %lf] ADDRESS : %s", n->lon, n->lat, dump.address);
             n->address = strdup(dump.address);
          }
        n->lon = dump.lon;
        n->lat = dump.lat;
     }
}

Grid *_get_current_grid(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   Eina_List *l;
   Grid *g = NULL, *ret = NULL;
   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        if (wd->zoom == g->zoom)
          {
             ret = g;
             break;
          }
     }
   return ret;
}

static Eina_Bool
_route_complete_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   Elm_Map_Route *r = (Elm_Map_Route*)data;
   Widget_Data *wd = r->wd;

   if ((!r) || (!ev)) return EINA_TRUE;
   Elm_Map_Route *rr = ecore_con_url_data_get(r->con_url);
   ecore_con_url_data_set(r->con_url, NULL);
   if (r!=rr) return EINA_TRUE;

   if (r->ud.fd) fclose(r->ud.fd);
   _parse_kml(r);

   if (wd->grids)
     {
        Grid *g;
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(wd->obj, &ox, &oy, &ow, &oh);
        g = _get_current_grid(wd);
        route_place(wd->obj, g, wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,stop", "elm");
   evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOADED, NULL);
   return EINA_TRUE;
}

static Eina_Bool
_name_complete_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   Elm_Map_Name *n = (Elm_Map_Name*)data;
   Widget_Data *wd = n->wd;

   if ((!n) || (!ev)) return EINA_TRUE;
   Elm_Map_Name *nn = ecore_con_url_data_get(n->con_url);
   ecore_con_url_data_set(n->con_url, NULL);
   if (n!=nn) return EINA_TRUE;

   if (n->ud.fd) fclose(n->ud.fd);
   _parse_name(n);

   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,stop", "elm");
   evas_object_smart_callback_call(wd->obj, SIG_NAME_LOADED, NULL);
   return EINA_TRUE;
}

static Elm_Map_Name *
_utils_convert_name(const Evas_Object *obj, int method, char *address, double lon, double lat)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[PATH_MAX];
   char *source;
   int fd;

   if ((!wd) || (!wd->src)) return NULL;
   Elm_Map_Name *name = ELM_NEW(Elm_Map_Name);
   if (!name) return NULL;

   snprintf(buf, sizeof(buf), DEST_NAME_XML_FILE);
   fd = mkstemp(buf);
   if (fd < 0)
     {
        free(name);
        return NULL;
     }

   name->con_url = ecore_con_url_new(NULL);
   name->ud.fname = strdup(buf);
   INF("xml file : %s", name->ud.fname);

   name->ud.fd = fdopen(fd, "w+");
   if ((!name->con_url) || (!name->ud.fd))
     {
        ecore_con_url_free(name->con_url);
        free(name);
        return NULL;
     }

   name->wd = wd;
   name->handler = ecore_event_handler_add (ECORE_CON_EVENT_URL_COMPLETE, _name_complete_cb, name);
   name->method = method;
   if (method == ELM_MAP_NAME_METHOD_SEARCH) name->address = strdup(address);
   else if (method == ELM_MAP_NAME_METHOD_REVERSE) name->address = NULL;
   name->lon = lon;
   name->lat = lat;

   source = wd->src->name_url_cb(wd->obj, method, address, lon, lat);
   INF("name url = %s", source);

   wd->names = eina_list_append(wd->names, name);
   ecore_con_url_url_set(name->con_url, source);
   ecore_con_url_fd_set(name->con_url, fileno(name->ud.fd));
   ecore_con_url_data_set(name->con_url, name);

   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,start", "elm");
   evas_object_smart_callback_call(wd->obj, SIG_NAME_LOAD, NULL);
   ecore_con_url_get(name->con_url);
   if (source) free(source);

   return name;

}

static Evas_Event_Flags
zoom_start_cb(void *data, void *event_info __UNUSED__)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   // FIXME: scroller can be jumping strangely when resizing & scrolling at the sametime (els_scr bug?)
   elm_smart_scroller_hold_set(wd->scr, 1);
   elm_smart_scroller_freeze_set(wd->scr, 1);
   _scr_drag_start(wd->obj, NULL, NULL);

   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
zoom_end_cb(void *data, void *event_info __UNUSED__)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   // FIXME: scroller can be jumping strangely when resizing & scrolling at the sametime (els_scr bug?)
   elm_smart_scroller_hold_set(wd->scr, 0);
   elm_smart_scroller_freeze_set(wd->scr, 0);
   _scr_drag_stop(wd->obj, NULL, NULL);

   wd->pinch.diff = wd->pinch.level;

   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
zoom_cb(void *data, void *event_info)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   if (!wd->paused)
     {
        int zoom_diff = 0;
        int x, y, w, h;
        Elm_Gesture_Zoom_Info *ei = event_info;
        evas_object_geometry_get(wd->obj, &x, &y, &w, &h);

        wd->pinch.level = wd->pinch.diff * ei->zoom;
        wd->pinch.cx = x + ((double)w * 0.5);
        wd->pinch.cy = y + ((double)h * 0.5);

        if (wd->pinch.level > 2.0 || wd->pinch.level < 1.0)
          {
             if (wd->pinch.level > 2.0)
               {
                  zoom_diff = 1;
                  wd->pinch.diff = 1.0;
                  wd->pinch.level = 1.0;
               }
             else if (wd->pinch.level < 1.0)
               {
                  zoom_diff = -1;
                  wd->pinch.diff = 2.0;
                  wd->pinch.level = 2.0;
               }
             Elm_Map_Zoom_Mode temp;
             elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE, NULL, wd);  // ei->zoom is refreshed
             temp = wd->mode;
             wd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
             wd->paused = EINA_TRUE;
             elm_map_zoom_set(wd->obj, wd->zoom + zoom_diff);
             wd->paused = EINA_FALSE;
             wd->mode = temp;
	     elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE, zoom_cb, wd); // ei->zoom is refreshed
          }
        else
          {
             if (wd->calc_job) ecore_job_del(wd->calc_job);
             wd->calc_job = ecore_job_add(_calc_job, wd);
          }
        evas_object_smart_callback_call(wd->obj, SIG_ZOOM_CHANGE, NULL);
     }

   // FIXME: scroller can be jumping strangely when resizing & scrolling at the sametime (els_scr bug?)
   _scr_scroll(wd->obj, NULL, NULL);

   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
rotate_cb(void *data, void *event_info)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   if (!wd->paused)
     {
        int x, y, w, h;
        Elm_Gesture_Rotate_Info *ei = event_info;
        evas_object_geometry_get(wd->obj, &x, &y, &w, &h);

        wd->rotate.d = wd->rotate.a + (ei->base_angle-ei->angle)*50;
        wd->rotate.cx = x + ((double)w * 0.5);
        wd->rotate.cy = y + ((double)h * 0.5);

        if (wd->calc_job) ecore_job_del(wd->calc_job);
        wd->calc_job = ecore_job_add(_calc_job, wd);
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
rotate_end_cb(void *data, void *event_info __UNUSED__)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   wd->rotate.a = wd->rotate.d;

   return EVAS_EVENT_FLAG_NONE;
}

#endif

EAPI Evas_Object *
elm_map_add(Evas_Object *parent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Evas *e;
   Widget_Data *wd;
   Evas_Coord minw, minh;
   Evas_Object *obj;
   static Evas_Smart *smart = NULL;
   Eina_Bool bounce = _elm_config->thumbscroll_bounce_enable;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "map");
   elm_widget_type_set(obj, "map");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_event_hook_set(obj, _event_hook);

   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "map", "base", "default");
   evas_object_smart_callback_add(wd->scr, "scroll", _scr, obj);
   evas_object_smart_callback_add(wd->scr, "drag", _scr, obj);
   elm_widget_resize_object_set(obj, wd->scr);
   elm_smart_scroller_wheel_disabled_set(wd->scr, EINA_TRUE);

   evas_object_smart_callback_add(wd->scr, "animate,start", _scr_anim_start, obj);
   evas_object_smart_callback_add(wd->scr, "animate,stop", _scr_anim_stop, obj);
   evas_object_smart_callback_add(wd->scr, "drag,start", _scr_drag_start, obj);
   evas_object_smart_callback_add(wd->scr, "drag,stop", _scr_drag_stop, obj);
   evas_object_smart_callback_add(wd->scr, "scroll", _scr_scroll, obj);

   elm_smart_scroller_bounce_allow_set(wd->scr, bounce, bounce);

   wd->zoom_min = 0xFF;
   wd->zoom_max = 0X00;
   source_init(obj);

   wd->obj = obj;
   wd->map = evas_map_new(4);
   if (!wd->map) return NULL;

   wd->markers_max_num = 30;
   wd->pinch.level = 1.0;
   wd->pinch.diff = 1.0;
   wd->markers = calloc(wd->zoom_max + 1, sizeof(void*));

   evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);

   if (!smart)
     {
        static Evas_Smart_Class sc;

        evas_object_smart_clipped_smart_set(&_pan_sc);
        sc = _pan_sc;
        sc.name = "elm_map_pan";
        sc.version = EVAS_SMART_CLASS_VERSION;
        sc.add = _pan_add;
        sc.del = _pan_del;
        sc.resize = _pan_resize;
        sc.move = _pan_move;
        sc.calculate = _pan_calculate;
        smart = evas_smart_class_new(&sc);
     }
   if (smart)
     {
        wd->pan_smart = evas_object_smart_add(e, smart);
        wd->pan = evas_object_smart_data_get(wd->pan_smart);
        wd->pan->wd = wd;
     }

   elm_smart_scroller_extern_pan_set(wd->scr, wd->pan_smart,
                                     _pan_set, _pan_get, _pan_max_get,
                                     _pan_min_get, _pan_child_size_get);

   wd->rect = evas_object_rectangle_add(e);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_RESIZE,
                                  _rect_resize_cb, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _mouse_wheel_cb, obj);

   evas_object_smart_member_add(wd->rect, wd->pan_smart);
   elm_widget_sub_object_add(obj, wd->rect);
   evas_object_show(wd->rect);
   evas_object_color_set(wd->rect, 0, 0, 0, 0);

   wd->ges = elm_gesture_layer_add(obj);
   if (!wd->ges) ERR("elm_gesture_layer_add() failed");
   elm_gesture_layer_attach(wd->ges, wd->rect);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START,
                            zoom_start_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE,
                            zoom_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END,
                            zoom_end_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT,
                            zoom_end_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,
                            rotate_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_END,
                            rotate_end_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_ABORT,
                            rotate_end_cb, wd);

   wd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
   wd->id = ((int)getpid() << 16) | idnum;
   idnum++;

   wd->tsize = 256;
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   wd->sep_maps_markers = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(wd->sep_maps_markers, wd->pan_smart);

   grid_create_all(obj);

   wd->paused = EINA_TRUE;
   elm_map_zoom_set(obj, 0);
   wd->paused = EINA_FALSE;
   _sizing_eval(obj);

   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   if (!ecore_file_download_protocol_available("http://"))
     {
        ERR("Ecore must be built with curl support for the map widget!");
     }

   return obj;
#else
   (void) parent;
   return NULL;
#endif
}

EAPI void
elm_map_zoom_set(Evas_Object *obj, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Evas_Coord rx, ry, rw, rh;
   Evas_Object *p;
   Elm_Map_Route *r;
   Evas_Object *route;
   int z = 0;

   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src);
   if (wd->zoom_animator) return;

   if (zoom < 0) zoom = 0;
   if (zoom > wd->src->zoom_max) zoom = wd->src->zoom_max;
   if (zoom < wd->src->zoom_min) zoom = wd->src->zoom_min;

   if ((wd->zoom - zoom) > 0) wd->zoom_method = ZOOM_METHOD_OUT;
   else if ((wd->zoom - zoom) < 0) wd->zoom_method = ZOOM_METHOD_IN;
   else wd->zoom_method = ZOOM_METHOD_NONE;

   wd->zoom = zoom;
   wd->size.ow = wd->size.w;
   wd->size.oh = wd->size.h;
   elm_smart_scroller_child_pos_get(wd->scr, &rx, &ry);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);

   EINA_LIST_FOREACH(wd->route, l, r)
     {
        if (r)
          {
             EINA_LIST_FOREACH(r->path, l, p)
               {
                  evas_object_polygon_points_clear(p);
               }
          }
     }

   EINA_LIST_FOREACH(wd->track, l, route)
     {
       evas_object_hide(route);
     }

   if (wd->mode != ELM_MAP_ZOOM_MODE_MANUAL)
     {
        int p2w, p2h;
        int cumulw, cumulh;

        cumulw = wd->tsize;
        p2w = 0;
        while (cumulw <= rw)
          {
             p2w++;
             cumulw *= 2;
          }
        p2w--;

        cumulh = wd->tsize;
        p2h = 0;
        while (cumulh <= rh)
          {
             p2h++;
             cumulh *= 2;
          }
        p2h--;

        if (wd->mode == ELM_MAP_ZOOM_MODE_AUTO_FIT)
          {
             if (p2w < p2h) z = p2w;
             else z = p2h;
          }
        else if (wd->mode == ELM_MAP_ZOOM_MODE_AUTO_FILL)
          {
             if (p2w > p2h) z = p2w;
             else z = p2h;
          }
        wd->zoom = z;
     }

   wd->size.nw = pow(2.0, wd->zoom) * wd->tsize;
   wd->size.nh = pow(2.0, wd->zoom) * wd->tsize;
   wd->t = 1.0;

   if ((wd->size.w > 0) && (wd->size.h > 0))
     {
        wd->size.spos.x = (double)(rx + (rw / 2)) / (double)wd->size.ow;
        wd->size.spos.y = (double)(ry + (rh / 2)) / (double)wd->size.oh;
     }
   else
     {
        wd->size.spos.x = 0.5;
        wd->size.spos.y = 0.5;
     }

   if (rw > wd->size.ow) wd->size.spos.x = 0.5;
   if (rh > wd->size.oh) wd->size.spos.y = 0.5;
   if (wd->size.spos.x > 1.0) wd->size.spos.x = 1.0;
   if (wd->size.spos.y > 1.0) wd->size.spos.y = 1.0;

   if (wd->paused)
     {
        zoom_do(obj);
     }
   else
     {
        if (!wd->zoom_animator)
          {
             wd->zoom_animator = ecore_animator_add(_zoom_anim, obj);
             wd->nosmooth++;
             if (wd->nosmooth == 1) _smooth_update(obj);
             evas_object_smart_callback_call(obj, SIG_ZOOM_START, NULL);
          }
     }

   if (wd->zoom_method != ZOOM_METHOD_NONE) evas_object_smart_callback_call(obj, SIG_ZOOM_CHANGE, NULL);
#else
   (void) obj;
   (void) zoom;
#endif
}

EAPI int
elm_map_zoom_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0;
   return wd->zoom;
#else
   (void) obj;
   return 0;
#endif
}

EAPI void
elm_map_zoom_mode_set(Evas_Object *obj, Elm_Map_Zoom_Mode mode)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->mode == mode) return;
   wd->mode = mode;

   if (wd->mode != ELM_MAP_ZOOM_MODE_MANUAL)
     {
        int tz = wd->zoom;
        wd->zoom = 0;
        elm_map_zoom_set(wd->obj, tz);
     }
#else
   (void) obj;
   (void) mode;
#endif
}

EAPI Elm_Map_Zoom_Mode
elm_map_zoom_mode_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_MAP_ZOOM_MODE_MANUAL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ELM_MAP_ZOOM_MODE_MANUAL;
   return wd->mode;
#else
   (void) obj;
   return ELM_MAP_ZOOM_MODE_MANUAL;
#endif
}

EAPI void
elm_map_geo_region_bring_in(Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int rx, ry, rw, rh;

   if (!wd) return;
   elm_map_utils_convert_geo_into_coord(obj, lon, lat, wd->size.w, &rx, &ry);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);

   rx = rx - rw / 2;
   ry = ry - rh / 2;

   if (wd->zoom_animator)
     {
        wd->nosmooth--;
        if (!wd->nosmooth) _smooth_update(obj);
        ecore_animator_del(wd->zoom_animator);
        wd->zoom_animator = NULL;
        zoom_do(obj);
        evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }
   elm_smart_scroller_region_bring_in(wd->scr, rx, ry, rw, rh);

   wd->center_on.enabled = EINA_TRUE;
   wd->center_on.lon = lon;
   wd->center_on.lat = lat;
#else
   (void) obj;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_geo_region_show(Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int rx, ry, rw, rh;

   if (!wd) return;
   elm_map_utils_convert_geo_into_coord(obj, lon, lat, wd->size.w, &rx, &ry);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);

   rx = rx - rw / 2;
   ry = ry - rh / 2;

   if (wd->zoom_animator)
     {
        wd->nosmooth--;
        ecore_animator_del(wd->zoom_animator);
        wd->zoom_animator = NULL;
        zoom_do(obj);
        evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }
   elm_smart_scroller_child_region_show(wd->scr, rx, ry, rw, rh);

   wd->center_on.enabled = EINA_TRUE;
   wd->center_on.lon = lon;
   wd->center_on.lat = lat;
#else
   (void) obj;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_geo_region_get(const Evas_Object *obj, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord sx, sy, sw, sh;

   if (!wd) return;
   elm_smart_scroller_child_pos_get(wd->scr, &sx, &sy);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &sw, &sh);
   if (wd->size.w < sw) sw = wd->size.w;
   if (wd->size.h < sh) sh = wd->size.h;
   sx += sw / 2;
   sy += sh / 2;

   elm_map_utils_convert_coord_into_geo(obj, sx, sy, wd->size.w, lon, lat);
#else
   (void) obj;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_paused_set(Evas_Object *obj, Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->paused == !!paused) return;
   wd->paused = !!paused;
   if (wd->paused)
     {
        if (wd->zoom_animator)
          {
             if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
             wd->zoom_animator = NULL;
             zoom_do(obj);
             evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
          }
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                "elm,state,busy,stop", "elm");
     }
   else
     {
        if (wd->download_num >= 1)
           edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                   "elm,state,busy,start", "elm");
     }
#else
   (void) obj;
   (void) paused;
#endif
}

EAPI void
elm_map_paused_markers_set(Evas_Object *obj, Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (wd->paused_markers == !!paused) return;
   wd->paused_markers = paused;
#else
   (void) obj;
   (void) paused;
#endif
}

EAPI Eina_Bool
elm_map_paused_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;
   return wd->paused;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI Eina_Bool
elm_map_paused_markers_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;
   return wd->paused_markers;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_utils_downloading_status_get(const Evas_Object *obj, int *try_num, int *finish_num)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (try_num)
     {
        *try_num = wd->try_num;
     }

   if (finish_num)
     {
        *finish_num = wd->finish_num;
     }
#else
   (void) obj;
   (void) try_num;
   (void) finish_num;
#endif
}

EAPI void
elm_map_utils_convert_coord_into_geo(const Evas_Object *obj, int x, int y, int size, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   int zoom = floor(log(size / 256) / log(2));
   if ((wd->src) && (wd->src->coord_into_geo))
     {
        if (wd->src->coord_into_geo(obj, zoom, x, y, size, lon, lat)) return;
     }

   if (lon)
     {
        *lon = x / (double)size * 360.0 - 180;
     }
   if (lat)
     {
        double n = ELM_PI - 2.0 * ELM_PI * y / size;
        *lat = 180.0 / ELM_PI * atan(0.5 * (exp(n) - exp(-n)));
     }
#else
   (void) obj;
   (void) x;
   (void) y;
   (void) size;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_utils_convert_geo_into_coord(const Evas_Object *obj, double lon, double lat, int size, int *x, int *y)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   int zoom = floor(log(size / 256) / log(2));
   if ((wd->src) && (wd->src->geo_into_coord))
     {
        if (wd->src->geo_into_coord(obj, zoom, lon, lat, size, x, y)) return;
     }

   if (x)
     *x = floor((lon + 180.0) / 360.0 * size);
   if (y)
     *y = floor((1.0 - log( tan(lat * ELM_PI / 180.0) + 1.0 / cos(lat * ELM_PI / 180.0)) / ELM_PI) / 2.0 * size);
#else
   (void) obj;
   (void) lon;
   (void) lat;
   (void) size;
   (void) x;
   (void) y;
#endif
}

EAPI Elm_Map_Name *
elm_map_utils_convert_coord_into_name(const Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   return _utils_convert_name(obj, ELM_MAP_NAME_METHOD_REVERSE, NULL, lon, lat);
#else
   (void) obj;
   (void) lon;
   (void) lat;
   return NULL;
#endif
}

EAPI Elm_Map_Name *
elm_map_utils_convert_name_into_coord(const Evas_Object *obj, char *address)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   if (!address) return NULL;
   return _utils_convert_name(obj, ELM_MAP_NAME_METHOD_SEARCH, address, 0.0, 0.0);
#else
   (void) obj;
   (void) address;
   return NULL;
#endif
}

EAPI void
elm_map_utils_rotate_coord(const Evas_Object *obj __UNUSED__, const Evas_Coord x, const Evas_Coord y, const Evas_Coord cx, const Evas_Coord cy, const double degree, Evas_Coord *xx, Evas_Coord *yy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   if ((!xx) || (!yy)) return;

   double r = (degree * M_PI) / 180.0;
   double tx, ty, ttx, tty;

   tx = x - cx;
   ty = y - cy;

   ttx = tx * cos(r);
   tty = tx * sin(r);
   tx = ttx + (ty * cos(r + M_PI_2));
   ty = tty + (ty * sin(r + M_PI_2));

   *xx = tx + cx;
   *yy = ty + cy;
#else
   (void) x;
   (void) y;
   (void) cx;
   (void) cy;
   (void) degree;
   (void) xx;
   (void) yy;
#endif
}

EAPI Elm_Map_Marker *
elm_map_marker_add(Evas_Object *obj, double lon, double lat, Elm_Map_Marker_Class *clas, Elm_Map_Group_Class *clas_group, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   int i, j;
   Eina_List *l;
   Marker_Group *group;
   int mpi, mpj;
   int tabi[9];
   int tabj[9];
   const char *s;
   const char *style;
   Evas_Object *o;

   if (!wd) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(clas_group, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(clas, NULL);

   Elm_Map_Marker *marker = ELM_NEW(Elm_Map_Marker);

   marker->wd = wd;
   marker->clas = clas;
   marker->clas_group = clas_group;
   marker->longitude = lon;
   marker->latitude = lat;
   marker->data = data;
   marker->x = calloc(wd->zoom_max + 1, sizeof(Evas_Coord));
   marker->y = calloc(wd->zoom_max + 1, sizeof(Evas_Coord));
   marker->groups = calloc(wd->zoom_max + 1, sizeof(Marker_Group*));

   tabi[1] = tabi[4] = tabi[6] = -1;
   tabi[2] = tabi[0] = tabi[7] = 0;
   tabi[3] = tabi[5] = tabi[8] = 1;

   tabj[1] = tabj[2] = tabj[3] = -1;
   tabj[4] = tabj[0] = tabj[5] = 0;
   tabj[6] = tabj[7] = tabj[8] = 1;

   if (!clas_group->priv.set)
     {
        style = "radio";
        if (marker->clas_group && marker->clas_group->style)
          style = marker->clas_group->style;

        o = edje_object_add(evas_object_evas_get(obj));
        _elm_theme_object_set(obj, o, "map/marker", style, elm_widget_style_get(obj));
        s = edje_object_data_get(o, "size_w");
        if (s) clas_group->priv.edje_w = atoi(s);
        else clas_group->priv.edje_w = 0;
        s = edje_object_data_get(o, "size_h");
        if (s) clas_group->priv.edje_h = atoi(s);
        else clas_group->priv.edje_h = 0;
        s = edje_object_data_get(o, "size_max_w");
        if (s) clas_group->priv.edje_max_w = atoi(s);
        else clas_group->priv.edje_max_w = 0;
        s = edje_object_data_get(o, "size_max_h");
        if (s) clas_group->priv.edje_max_h = atoi(s);
        else clas_group->priv.edje_max_h = 0;
        evas_object_del(o);

        clas_group->priv.set = EINA_TRUE;
     }

   if (!clas->priv.set)
     {
        style = "radio";
        if (marker->clas && marker->clas->style)
          style = marker->clas->style;

        o = edje_object_add(evas_object_evas_get(obj));
        _elm_theme_object_set(obj, o, "map/marker", style, elm_widget_style_get(obj));
        s = edje_object_data_get(o, "size_w");
        if (s) clas->priv.edje_w = atoi(s);
        else clas->priv.edje_w = 0;
        s = edje_object_data_get(o, "size_h");
        if (s) clas->priv.edje_h = atoi(s);
        else clas->priv.edje_h = 0;
        evas_object_del(o);

        clas->priv.set = EINA_TRUE;
     }

   for (i = clas_group->zoom_displayed; i <= wd->zoom_max; i++)
     {
        elm_map_utils_convert_geo_into_coord(obj, lon, lat, pow(2.0, i)*wd->tsize,
                                             &(marker->x[i]), &(marker->y[i]));

        //search in the matrixsparse the region where the marker will be
        mpi = marker->x[i] / wd->tsize;
        mpj = marker->y[i] / wd->tsize;

        if (!wd->markers[i])
          {
             int size =  pow(2.0, i);
             wd->markers[i] = eina_matrixsparse_new(size, size, NULL, NULL);
          }

        group = NULL;
        if (i <= clas_group->zoom_grouped)
          {
             for (j = 0, group = NULL; j < 9 && !group; j++)
               {
                  EINA_LIST_FOREACH(eina_matrixsparse_data_idx_get(wd->markers[i], mpj + tabj[j], mpi + tabi[j]),
                                    l, group)
                    {
                       if (group->clas == marker->clas_group
                           && ELM_RECTS_INTERSECT(marker->x[i]-clas->priv.edje_w/4,
                                                  marker->y[i]-clas->priv.edje_h/4, clas->priv.edje_w, clas->priv.edje_h,
                                                  group->x-group->w/4, group->y-group->h/4, group->w, group->h))
                         {
                            group->markers = eina_list_append(group->markers, marker);
                            group->update_nbelems = EINA_TRUE;
                            group->update_resize = EINA_TRUE;

                            group->sum_x += marker->x[i];
                            group->sum_y += marker->y[i];
                            group->x = group->sum_x / eina_list_count(group->markers);
                            group->y = group->sum_y / eina_list_count(group->markers);

                            group->w = group->clas->priv.edje_w + group->clas->priv.edje_w/8.
                               * eina_list_count(group->markers);
                            group->h = group->clas->priv.edje_h + group->clas->priv.edje_h/8.
                               * eina_list_count(group->markers);
                            if (group->w > group->clas->priv.edje_max_w) group->w = group->clas->priv.edje_max_w;
                            if (group->h > group->clas->priv.edje_max_h) group->h = group->clas->priv.edje_max_h;

                            if (group->obj && eina_list_count(group->markers) == 2)
                              {
                                 _group_object_free(group);
                                 _group_object_create(group);
                              }
                            if (group->bubble)
                              _group_bubble_content_update(group);

                            break;
                         }
                    }
               }
          }
        if (!group)
          {
             group = calloc(1, sizeof(Marker_Group));
             group->wd = wd;
             group->sum_x = marker->x[i];
             group->sum_y = marker->y[i];
             group->x = marker->x[i];
             group->y = marker->y[i];
             group->w = clas_group->priv.edje_w;
             group->h = clas_group->priv.edje_h;
             group->clas = clas_group;

             group->markers = eina_list_append(group->markers, marker);
             group->update_nbelems = EINA_TRUE;
             group->update_resize = EINA_TRUE;

             eina_matrixsparse_cell_idx_get(wd->markers[i], mpj, mpi, &(group->cell));

             if (!group->cell)
               {
                  l = eina_list_append(NULL, group);
                  eina_matrixsparse_data_idx_set(wd->markers[i], mpj, mpi, l);
                  eina_matrixsparse_cell_idx_get(wd->markers[i], mpj, mpi, &(group->cell));
               }
             else
               {
                  l = eina_matrixsparse_cell_data_get(group->cell);
                  l = eina_list_append(l, group);
                  eina_matrixsparse_cell_data_set(group->cell, l);
               }
          }
        marker->groups[i] = group;
     }

   if (wd->grids)
     {
        Grid *g;
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
        g = _get_current_grid(wd);
        marker_place(obj, g, wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }

   return marker;
#else
   (void) obj;
   (void) lon;
   (void) lat;
   (void) clas;
   (void) clas_group;
   (void) data;
   return NULL;
#endif
}

EAPI void
elm_map_marker_remove(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   int i;
   Eina_List *groups;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN(marker);
   wd = marker->wd;
   if (!wd) return;
   for (i = marker->clas_group->zoom_displayed; i <= wd->zoom_max; i++)
     {
        marker->groups[i]->markers = eina_list_remove(marker->groups[i]->markers, marker);
        if (!eina_list_count(marker->groups[i]->markers))
          {
             groups = eina_matrixsparse_cell_data_get(marker->groups[i]->cell);
             groups = eina_list_remove(groups, marker->groups[i]);
             eina_matrixsparse_cell_data_set(marker->groups[i]->cell, groups);

             _group_object_free(marker->groups[i]);
             _group_bubble_free(marker->groups[i]);
             free(marker->groups[i]);
          }
        else
          {
             marker->groups[i]->sum_x -= marker->x[i];
             marker->groups[i]->sum_y -= marker->y[i];

             marker->groups[i]->x = marker->groups[i]->sum_x / eina_list_count(marker->groups[i]->markers);
             marker->groups[i]->y = marker->groups[i]->sum_y / eina_list_count(marker->groups[i]->markers);

             marker->groups[i]->w = marker->groups[i]->clas->priv.edje_w
                + marker->groups[i]->clas->priv.edje_w/8. * eina_list_count(marker->groups[i]->markers);
             marker->groups[i]->h = marker->groups[i]->clas->priv.edje_h
                + marker->groups[i]->clas->priv.edje_h/8. * eina_list_count(marker->groups[i]->markers);
             if (marker->groups[i]->w > marker->groups[i]->clas->priv.edje_max_w)
               marker->groups[i]->w = marker->groups[i]->clas->priv.edje_max_w;
             if (marker->groups[i]->h > marker->groups[i]->clas->priv.edje_max_h)
               marker->groups[i]->h = marker->groups[i]->clas->priv.edje_max_h;

             if ((marker->groups[i]->obj) && (eina_list_count(marker->groups[i]->markers) == 1))
               {
                  _group_object_free(marker->groups[i]);
                  _group_object_create(marker->groups[i]);
               }
          }
     }

   if ((marker->content) && (marker->clas->func.del))
     marker->clas->func.del(marker->wd->obj, marker, marker->data, marker->content);
   else if (marker->content)
     evas_object_del(marker->content);

   if (marker->x) free(marker->x);
   if (marker->y) free(marker->y);
   if (marker->groups) free(marker->groups);

   free(marker);

   if (wd->grids)
     {
        Grid *g;
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(wd->obj, &ox, &oy, &ow, &oh);
        g = _get_current_grid(wd);
        marker_place(wd->obj, g, wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
#else
   (void) marker;
#endif
}

EAPI void
elm_map_marker_region_get(const Elm_Map_Marker *marker, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   if (lon) *lon = marker->longitude;
   if (lat) *lat = marker->latitude;
#else
   (void) marker;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_marker_bring_in(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_geo_region_bring_in(marker->wd->obj, marker->longitude, marker->latitude);
#else
   (void) marker;
#endif
}

EAPI void
elm_map_marker_show(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_geo_region_show(marker->wd->obj, marker->longitude, marker->latitude);
#else
   (void) marker;
#endif
}

EAPI void
elm_map_markers_list_show(Eina_List *markers)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   int zoom;
   double lon, lat;
   Eina_List *l;
   Elm_Map_Marker *marker, *m_max_lon = NULL, *m_max_lat = NULL, *m_min_lon = NULL, *m_min_lat = NULL;
   Evas_Coord rw, rh, xc, yc;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN(markers);
   EINA_LIST_FOREACH(markers, l, marker)
     {
        wd = marker->wd;

        if ((!m_min_lon) || (marker->longitude < m_min_lon->longitude))
          m_min_lon = marker;

        if ((!m_max_lon) || (marker->longitude > m_max_lon->longitude))
          m_max_lon = marker;

        if ((!m_min_lat) || (marker->latitude > m_min_lat->latitude))
          m_min_lat = marker;

        if ((!m_max_lat) || (marker->latitude < m_max_lat->latitude))
          m_max_lat = marker;
     }

   lon = (m_max_lon->longitude - m_min_lon->longitude) / 2. + m_min_lon->longitude;
   lat = (m_max_lat->latitude - m_min_lat->latitude) / 2. + m_min_lat->latitude;

   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);
   for (zoom = wd->src->zoom_max; zoom > wd->src->zoom_min; zoom--)
     {
        Evas_Coord size = pow(2.0, zoom)*wd->tsize;
        elm_map_utils_convert_geo_into_coord(wd->obj, lon, lat, size, &xc, &yc);

        if ((m_min_lon->x[zoom] - wd->marker_max_w >= xc-rw/2)
            && (m_min_lat->y[zoom] - wd->marker_max_h >= yc-rh/2)
            && (m_max_lon->x[zoom] + wd->marker_max_w <= xc+rw/2)
            && (m_max_lat->y[zoom] + wd->marker_max_h <= yc+rh/2))
          break;
     }

   elm_map_geo_region_show(wd->obj, lon, lat);
   elm_map_zoom_set(wd->obj, zoom);
#else
   (void) markers;
#endif
}

EAPI void
elm_map_max_marker_per_group_set(Evas_Object *obj, int max)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->markers_max_num = max;
#else
   (void) obj;
   (void) max;
#endif
}

EAPI Evas_Object *
elm_map_marker_object_get(const Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(marker, NULL);
   return marker->content;
#else
   (void) marker;
   return NULL;
#endif
}

EAPI void
elm_map_marker_update(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   if (marker->content)
     {
        if (marker->clas->func.del)
          marker->clas->func.del(marker->wd->obj, marker, marker->data, marker->content);
        else
          evas_object_del(marker->content);
        marker->content = NULL;
        _group_bubble_content_update(marker->groups[marker->wd->zoom]);
     }
#else
   (void) marker;
#endif
}

EAPI void
elm_map_bubbles_close(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Marker_Group *group;
   Eina_List *l, *l_next;

   if (!wd) return;
   EINA_LIST_FOREACH_SAFE(wd->opened_bubbles, l, l_next, group)
      _group_bubble_free(group);
#else
   (void) obj;
#endif
}

EAPI Elm_Map_Group_Class *
elm_map_group_class_new(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   Elm_Map_Group_Class *clas = calloc(1, sizeof(Elm_Map_Group_Class));
   clas->zoom_grouped = wd->zoom_max;
   wd->groups_clas = eina_list_append(wd->groups_clas, clas);
   return clas;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_group_class_style_set(Elm_Map_Group_Class *clas, const char *style)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   eina_stringshare_replace(&clas->style, style);
#else
   (void) clas;
   (void) style;
#endif
}

EAPI void
elm_map_group_class_icon_cb_set(Elm_Map_Group_Class *clas, ElmMapGroupIconGetFunc icon_get)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.icon_get = icon_get;
#else
   (void) clas;
   (void) icon_get;
#endif
}

EAPI void
elm_map_group_class_data_set(Elm_Map_Group_Class *clas, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->data = data;
#else
   (void) clas;
   (void) data;
#endif
}

EAPI void
elm_map_group_class_zoom_displayed_set(Elm_Map_Group_Class *clas, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->zoom_displayed = zoom;
#else
   (void) clas;
   (void) zoom;
#endif
}

EAPI void
elm_map_group_class_zoom_grouped_set(Elm_Map_Group_Class *clas, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->zoom_grouped = zoom;
#else
   (void) clas;
   (void) zoom;
#endif
}

EAPI void
elm_map_group_class_hide_set(Evas_Object *obj, Elm_Map_Group_Class *clas, Eina_Bool hide)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(clas);
   if (clas->hide == hide) return;
   clas->hide = hide;
   if (wd->grids)
     {
        Grid *g;
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
        g = _get_current_grid(wd);
        marker_place(obj, g, wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
#else
   (void) obj;
   (void) clas;
   (void) hide;
#endif
}

EAPI Elm_Map_Marker_Class *
elm_map_marker_class_new(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   Elm_Map_Marker_Class *clas = calloc(1, sizeof(Elm_Map_Marker_Class));
   wd->markers_clas = eina_list_append(wd->markers_clas, clas);
   return clas;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_marker_class_style_set(Elm_Map_Marker_Class *clas, const char *style)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   eina_stringshare_replace(&clas->style, style);
#else
   (void) clas;
   (void) style;
#endif
}

EAPI void
elm_map_marker_class_icon_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerIconGetFunc icon_get)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.icon_get = icon_get;
#else
   (void) clas;
   (void) icon_get;
#endif
}

EAPI void
elm_map_marker_class_get_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerGetFunc get)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.get = get;
#else
   (void) clas;
   (void) get;
#endif
}

EAPI void
elm_map_marker_class_del_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerDelFunc del)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.del = del;
#else
   (void) clas;
   (void) del;
#endif
}

EAPI const char **
elm_map_source_names_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->source_names;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_source_name_set(Evas_Object *obj, const char *source_name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Map_Sources_Tab *s;
   Eina_List *l;
   int zoom;

   if (!wd) return;
   if (wd->src)
     {
        if (!strcmp(wd->src->name, source_name)) return;
        if (!wd->src->url_cb) return;
     }

   grid_clear_all(obj);
   EINA_LIST_FOREACH(wd->map_sources_tab, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             wd->src = s;
             break;
          }
     }
   zoom = wd->zoom;
   wd->zoom = -1;

   if (wd->src)
     {
        if (wd->src->zoom_max < zoom)
          zoom = wd->src->zoom_max;
        if (wd->src->zoom_min > zoom)
          zoom = wd->src->zoom_min;
     }
   grid_create_all(obj);
   elm_map_zoom_set(obj, zoom);
#else
   (void) obj;
   (void) source_name;
#endif
}

EAPI const char *
elm_map_source_name_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->src)) return NULL;
   return wd->src->name;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_route_source_set(Evas_Object *obj, Elm_Map_Route_Sources source)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->route_source = source;
#else
   (void) obj;
   (void) source;
#endif
}

EAPI Elm_Map_Route_Sources
elm_map_route_source_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_MAP_ROUTE_SOURCE_YOURS;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ELM_MAP_ROUTE_SOURCE_YOURS;
   return wd->route_source;
#else
   (void) obj;
   return ELM_MAP_ROUTE_SOURCE_YOURS;
#endif
}

EAPI void
elm_map_source_zoom_max_set(Evas_Object *obj, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->src)) return;
   if ((zoom > wd->zoom_max) || (zoom < wd->zoom_min)) return;
   wd->src->zoom_max = zoom;
#else
   (void) obj;
   (void) zoom;
#endif
}

EAPI int
elm_map_source_zoom_max_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) 18;
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->src)) return 18;
   return wd->src->zoom_max;
#else
   (void) obj;
   return 18;
#endif
}

EAPI void
elm_map_source_zoom_min_set(Evas_Object *obj, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->src)) return;
   if ((zoom > wd->zoom_max) || (zoom < wd->zoom_min)) return;
   wd->src->zoom_min = zoom;
#else
   (void) obj;
   (void) zoom;
#endif
}

EAPI int
elm_map_source_zoom_min_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);

   if ((!wd) || (!wd->src)) return 0;
   return wd->src->zoom_min;
#else
   (void) obj;
   return 0;
#endif
}

EAPI void
elm_map_user_agent_set(Evas_Object *obj, const char *user_agent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (!wd->user_agent) wd->user_agent = eina_stringshare_add(user_agent);
   else eina_stringshare_replace(&wd->user_agent, user_agent);

   if (!wd->ua) wd->ua = eina_hash_string_small_new(NULL);
   eina_hash_set(wd->ua, "User-Agent", wd->user_agent);
#else
   (void) obj;
   (void) user_agent;
#endif
}

EAPI const char *
elm_map_user_agent_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->user_agent;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI Elm_Map_Route *
elm_map_route_add(Evas_Object *obj,
                  Elm_Map_Route_Type type,
                  Elm_Map_Route_Method method,
                  double flon,
                  double flat,
                  double tlon,
                  double tlat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[PATH_MAX];
   char *source;
   char *type_name = NULL;
   int fd;

   if ((!wd) || (!wd->src)) return NULL;

   Elm_Map_Route *route = ELM_NEW(Elm_Map_Route);
   if (!route) return NULL;

   snprintf(buf, sizeof(buf), DEST_ROUTE_XML_FILE);
   fd = mkstemp(buf);
   if (fd < 0)
     {
        free(route);
        return NULL;
     }

   route->con_url = ecore_con_url_new(NULL);
   route->ud.fname = strdup(buf);
   INF("xml file : %s", route->ud.fname);

   route->ud.fd = fdopen(fd, "w+");
   if ((!route->con_url) || (!route->ud.fd))
     {
        ecore_con_url_free(route->con_url);
        free(route);
        return NULL;
     }

   route->wd = wd;
   route->color.r = 255;
   route->color.g = 0;
   route->color.b = 0;
   route->color.a = 255;
   route->handlers = eina_list_append
     (route->handlers, (void *)ecore_event_handler_add
         (ECORE_CON_EVENT_URL_COMPLETE, _route_complete_cb, route));

   route->inbound = EINA_FALSE;
   route->type = type;
   route->method = method;
   route->flon = flon;
   route->flat = flat;
   route->tlon = tlon;
   route->tlat = tlat;

   switch (type)
     {
      case ELM_MAP_ROUTE_TYPE_MOTOCAR:
        type_name = strdup(ROUTE_TYPE_MOTORCAR);
        break;
      case ELM_MAP_ROUTE_TYPE_BICYCLE:
        type_name = strdup(ROUTE_TYPE_BICYCLE);
        break;
      case ELM_MAP_ROUTE_TYPE_FOOT:
        type_name = strdup(ROUTE_TYPE_FOOT);
        break;
      default:
        break;
     }

   source = wd->src->route_url_cb(obj, type_name, method, flon, flat, tlon, tlat);
   INF("route url = %s", source);

   wd->route = eina_list_append(wd->route, route);

   ecore_con_url_url_set(route->con_url, source);
   ecore_con_url_fd_set(route->con_url, fileno(route->ud.fd));
   ecore_con_url_data_set(route->con_url, route);
   ecore_con_url_get(route->con_url);
   if (type_name) free(type_name);
   if (source) free(source);

   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,start", "elm");
   evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOAD, NULL);
   return route;
#else
   (void) obj;
   (void) type;
   (void) method;
   (void) flon;
   (void) flat;
   (void) tlon;
   (void) tlat;
   return NULL;
#endif
}

EAPI void
elm_map_route_remove(Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(route);

   Path_Waypoint *w;
   Path_Node *n;
   Evas_Object *p;
   Ecore_Event_Handler *h;

   EINA_LIST_FREE(route->path, p)
     {
        evas_object_del(p);
     }

   EINA_LIST_FREE(route->waypoint, w)
     {
        if (w->point) eina_stringshare_del(w->point);
        free(w);
     }

   EINA_LIST_FREE(route->nodes, n)
     {
        if (n->pos.address) eina_stringshare_del(n->pos.address);
        free(n);
     }

   EINA_LIST_FREE(route->handlers, h)
     {
        ecore_event_handler_del(h);
     }

   if (route->ud.fname)
     {
        ecore_file_remove(route->ud.fname);
        free(route->ud.fname);
        route->ud.fname = NULL;
     }
#else
   (void) route;
#endif
}

EAPI void
elm_map_route_color_set(Elm_Map_Route *route, int r, int g , int b, int a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(route);
   route->color.r = r;
   route->color.g = g;
   route->color.b = b;
   route->color.a = a;
#else
   (void) route;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
#endif
}

EAPI void
elm_map_route_color_get(const Elm_Map_Route *route, int *r, int *g , int *b, int *a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(route);
   if (r) *r = route->color.r;
   if (g) *g = route->color.g;
   if (b) *b = route->color.b;
   if (a) *a = route->color.a;
#else
   (void) route;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
#endif
}

EAPI double
elm_map_route_distance_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, 0.0);
   return route->info.distance;
#else
   (void) route;
   return 0.0;
#endif
}

EAPI const char*
elm_map_route_node_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);
   return route->info.nodes;
#else
   (void) route;
   return NULL;
#endif
}

EAPI const char*
elm_map_route_waypoint_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);
   return route->info.waypoints;
#else
   (void) route;
   return NULL;
#endif
}

EAPI const char *
elm_map_name_address_get(const Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   return name->address;
#else
   (void) name;
   return NULL;
#endif
}

EAPI void
elm_map_name_region_get(const Elm_Map_Name *name, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(name);
   if (lon) *lon = name->lon;
   if (lat) *lat = name->lat;
#else
   (void) name;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_name_remove(Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(name);
   if (name->address)
     {
        free(name->address);
        name->address = NULL;
     }
   if (name->handler)
     {
        ecore_event_handler_del(name->handler);
        name->handler = NULL;
     }
   if (name->ud.fname)
     {
        ecore_file_remove(name->ud.fname);
        free(name->ud.fname);
        name->ud.fname = NULL;
     }
#else
   (void) name;
#endif
}

EAPI void
elm_map_rotate_set(Evas_Object *obj, double degree, Evas_Coord cx, Evas_Coord cy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   wd->rotate.d = degree;
   wd->rotate.cx = cx;
   wd->rotate.cy = cy;
   wd->calc_job = ecore_job_add(_calc_job, wd);
#else
   (void) obj;
   (void) degree;
   (void) cx;
   (void) cy;
#endif
}

EAPI void
elm_map_rotate_get(const Evas_Object *obj, double *degree, Evas_Coord *cx, Evas_Coord *cy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (degree) *degree = wd->rotate.d;
   if (cx) *cx = wd->rotate.cx;
   if (cy) *cy = wd->rotate.cy;
#else
   (void) obj;
   (void) degree;
   (void) cx;
   (void) cy;
#endif
}

EAPI void
elm_map_wheel_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if ((!wd->wheel_disabled) && (disabled))
     evas_object_event_callback_del_full(wd->rect, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, obj);
   else if ((wd->wheel_disabled) && (!disabled))
     evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, obj);
   wd->wheel_disabled = !!disabled;
#else
   (void) obj;
   (void) disabled;
#endif
}

EAPI Eina_Bool
elm_map_wheel_disabled_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;
   return wd->wheel_disabled;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

#ifdef ELM_EMAP
EAPI Evas_Object *
elm_map_track_add(Evas_Object *obj, EMap_Route *emap)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;

   Evas_Object *route = elm_route_add(obj);
   elm_route_emap_set(route, emap);
   wd->track = eina_list_append(wd->track, route);

   return route;
#else
   (void) obj;
   (void) emap;
   return NULL;
#endif
}
#endif

EAPI void
elm_map_track_remove(Evas_Object *obj, Evas_Object *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) ;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ;

   wd->track = eina_list_remove(wd->track, route);
   evas_object_del(route);
#else
   (void) obj;
   (void) route;
#endif
}

#ifdef HAVE_ELEMENTARY_ECORE_CON

static char *
_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   // ((x+y+zoom)%3)+'a' is requesting map images from distributed tile servers (eg., a, b, c)
   snprintf(buf, sizeof(buf), "http://%c.tile.openstreetmap.org/%d/%d/%d.png", ((x+y+zoom)%3)+'a', zoom, x, y);
   return strdup(buf);
}

static char *
_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://%c.tah.openstreetmap.org/Tiles/tile/%d/%d/%d.png", ((x+y+zoom)%3)+'a', zoom, x, y);
   return strdup(buf);
}

static char *
_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://%c.tile.opencyclemap.org/cycle/%d/%d/%d.png", ((x+y+zoom)%3)+'a', zoom, x, y);
   return strdup(buf);
}

static char *
_mapquest_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://otile%d.mqcdn.com/tiles/1.0.0/osm/%d/%d/%d.png", ((x+y+zoom)%4)+1, zoom, x, y);
   return strdup(buf);
}

static char *
_mapquest_aerial_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://oatile%d.mqcdn.com/naip/%d/%d/%d.png", ((x+y+zoom)%4)+1, zoom, x, y);
   return strdup(buf);
}

static char *_yours_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "%s?flat=%lf&flon=%lf&tlat=%lf&tlon=%lf&v=%s&fast=%d&instructions=1",
            ROUTE_YOURS_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
}

// TODO: fix monav api
/*
static char *_monav_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "%s?flat=%f&flon=%f&tlat=%f&tlon=%f&v=%s&fast=%d&instructions=1",
            ROUTE_MONAV_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
}
*/

// TODO: fix ors api
/*
static char *_ors_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "%s?flat=%f&flon=%f&tlat=%f&tlon=%f&v=%s&fast=%d&instructions=1",
            ROUTE_ORS_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
}
*/

static char *
_nominatim_url_cb(Evas_Object *obj, int method, char *name, double lon, double lat)
{
   ELM_CHECK_WIDTYPE(obj, widtype) strdup("");
   Widget_Data *wd = elm_widget_data_get(obj);
   char **str;
   unsigned int ele, idx;
   char search_url[PATH_MAX];
   char buf[PATH_MAX];

   if (!wd) return strdup("");
   if (method == ELM_MAP_NAME_METHOD_SEARCH)
     {
        search_url[0] = '\0';
        str = eina_str_split_full(name, " ", 0, &ele);
        for (idx = 0 ; idx < ele ; idx++)
          {
             eina_strlcat(search_url, str[idx], sizeof(search_url));
             if (!(idx == (ele-1))) eina_strlcat(search_url, "+", sizeof(search_url));
          }
        snprintf(buf, sizeof(buf), "%s/search?q=%s&format=xml&polygon=0&addressdetails=0", NAME_NOMINATIM_URL, search_url);

        if (str && str[0])
          {
             free(str[0]);
             free(str);
          }
     }
   else if (method == ELM_MAP_NAME_METHOD_REVERSE) snprintf(buf, sizeof(buf), "%s/reverse?format=xml&lat=%lf&lon=%lf&zoom=%d&addressdetails=0", NAME_NOMINATIM_URL, lat, lon, wd->zoom);
   else strcpy(buf, "");

   return strdup(buf);
}

#endif
