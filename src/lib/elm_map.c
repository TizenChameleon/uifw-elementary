#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"
#include "elm_priv.h"
#include "els_scroller.h"

#define OVERLAY_CLASS_ZOOM_MAX 255

#ifdef HAVE_ELEMENTARY_ECORE_CON

typedef struct _Widget_Data Widget_Data;
typedef struct _Pan Pan;
typedef struct _Grid Grid;
typedef struct _Grid_Item Grid_Item;
typedef struct _Overlay_Default Overlay_Default;
typedef struct _Overlay_Group Overlay_Group;
typedef struct _Overlay_Class Overlay_Class;
typedef struct _Overlay_Bubble Overlay_Bubble;
typedef struct _Overlay_Route Overlay_Route;
typedef struct _Marker_Group Marker_Group;
typedef struct _Marker_Bubble Marker_Bubble;
typedef struct _Path_Node Path_Node;
typedef struct _Path_Waypoint Path_Waypoint;
typedef struct _Url_Data Url_Data;
typedef struct _Route_Dump Route_Dump;
typedef struct _Name_Dump Name_Dump;
typedef struct _Delayed_Data Delayed_Data;
typedef struct _Source_Tile Source_Tile;
typedef struct _Source_Route Source_Route;
typedef struct _Source_Name Source_Name;

typedef char                      *(*Elm_Map_Module_Source_Name_Func)(void);
typedef int                        (*Elm_Map_Module_Tile_Zoom_Min_Func)(void);
typedef int                        (*Elm_Map_Module_Tile_Zoom_Max_Func)(void);
typedef char                      *(*Elm_Map_Module_Tile_Url_Func)(Evas_Object *obj, int x, int y, int zoom);
typedef Eina_Bool                  (*Elm_Map_Module_Tile_Geo_to_Coord_Func)(const Evas_Object *obj, int zoom, double lon, double lat, int size, int *x, int *y);
typedef Eina_Bool                  (*Elm_Map_Module_Tile_Coord_to_Geo_Func)(const Evas_Object *obj, int zoom, int x, int y, int size, double *lon, double *lat);
typedef char                      *(*Elm_Map_Module_Route_Url_Func)(Evas_Object *obj, const char *type_name, int method, double flon, double flat, double tlon, double tlat);
typedef char                      *(*Elm_Map_Module_Name_Url_Func)(Evas_Object *obj, int method, const char *name, double lon, double lat);

#define ROUND(z)                (((z) < 0) ? (int)ceil((z) - 0.005) : (int)floor((z) + 0.005))
#define EVAS_MAP_POINT          4
#define DEFAULT_TILE_SIZE       256
#define MAX_CONCURRENT_DOWNLOAD 10
#define MARER_MAX_NUMBER        30
#define OVERLAY_GROUPING_SCALE 2

#define CACHE_ROOT          "/tmp/elm_map"
#define CACHE_TILE_ROOT     CACHE_ROOT"/%d/%d/%d"
#define CACHE_TILE_PATH     "%s/%d.png"
#define CACHE_ROUTE_ROOT    CACHE_ROOT"/route"
#define CACHE_NAME_ROOT     CACHE_ROOT"/name"

#define ROUTE_YOURS_URL     "http://www.yournavigation.org/api/dev/route.php"
#define ROUTE_TYPE_MOTORCAR "motocar"
#define ROUTE_TYPE_BICYCLE  "bicycle"
#define ROUTE_TYPE_FOOT     "foot"
#define YOURS_DISTANCE      "distance"
#define YOURS_DESCRIPTION   "description"
#define YOURS_COORDINATES   "coordinates"

#define NAME_NOMINATIM_URL  "http://nominatim.openstreetmap.org"
#define NOMINATIM_RESULT    "result"
#define NOMINATIM_PLACE     "place"
#define NOMINATIM_ATTR_LON  "lon"
#define NOMINATIM_ATTR_LAT  "lat"

enum _Route_Xml_Attribute
{
   ROUTE_XML_NONE,
   ROUTE_XML_DISTANCE,
   ROUTE_XML_DESCRIPTION,
   ROUTE_XML_COORDINATES,
   ROUTE_XML_LAST
} Route_Xml_Attibute;

enum _Name_Xml_Attribute
{
   NAME_XML_NONE,
   NAME_XML_NAME,
   NAME_XML_LON,
   NAME_XML_LAT,
   NAME_XML_LAST
} Name_Xml_Attibute;

enum _Track_Xml_Attribute
{
   TRACK_XML_NONE,
   TRACK_XML_COORDINATES,
   TRACK_XML_LAST
} Track_Xml_Attibute;

struct _Delayed_Data
{
   void (*func)(void *data);
   Widget_Data *wd;
   Elm_Map_Zoom_Mode mode;
   int zoom;
   double lon, lat;
   Eina_List *markers;
   Eina_List *overlays;
};

// Map Tile source
// FIXME: Currently tile size must be 256*256
// and the map size is pow(2.0, z) * (tile size)
struct _Source_Tile
{
   const char *name;
   int zoom_min;
   int zoom_max;
   Elm_Map_Module_Tile_Url_Func url_cb;
   Elm_Map_Module_Tile_Geo_to_Coord_Func geo_to_coord;
   Elm_Map_Module_Tile_Coord_to_Geo_Func coord_to_geo;
};

// Map Route Source
struct _Source_Route
{
   const char *name;
   Elm_Map_Module_Route_Url_Func url_cb;
};

// Map Name Source
struct _Source_Name
{
   const char *name;
   Elm_Map_Module_Name_Url_Func url_cb;
};

struct _Url_Data
{
   Ecore_Con_Url *con_url;

   FILE *fd;
   char *fname;
};

struct _Overlay_Default
{
   Evas_Coord w, h;

   // Display priority is obj > icon > clas_obj > clas_icon > layout
   Evas_Object *obj;
   Evas_Object *icon;

   // if obj or icon exists, do not inherit from class
   Evas_Object *clas_obj;      // Duplicated from class icon
   Evas_Object *clas_obj_ref;  // Checking fro class icon is changed
   Evas_Object *clas_icon;     // Duplicated from class icon
   Evas_Object *clas_icon_ref; // Checking for class icon is changed

   char *style;
   Evas_Object *layout;
   double lon, lat;
   Evas_Coord x, y;
};

struct _Overlay_Group
{
   Overlay_Default *ovl;
   Evas_Object *clas_icon;
   Elm_Map_Overlay *clas;
   Eina_List *members;
};

struct _Overlay_Class
{
   Elm_Map_Overlay *clas;
   Evas_Object *obj;
   char *style;
   Evas_Object *icon;
   Eina_List *members;
   int zoom_max;
};

struct _Overlay_Bubble
{
   Widget_Data *wd;
   Evas_Object *pobj;
   Evas_Object *obj, *sc, *bx;
   double lon, lat;
   Evas_Coord x, y, w, h;
};

struct _Overlay_Route
{
   Widget_Data *wd;

   Eina_Bool inbound : 1;
   struct
     {
        int r;
        int g;
        int b;
        int a;
     } color;

   Eina_List *paths;
   Eina_List *nodes;
   int x, y;
};

struct _Elm_Map_Overlay
{
   Widget_Data *wd;

   Eina_Bool paused : 1;
   Eina_Bool hide : 1;
   Evas_Coord zoom_min;

    void *data;               // user set data

   Elm_Map_Overlay_Type type;
   void *ovl;                 // Overlay Data for each type

   // These are not used if overlay type is class
   Eina_Bool grp_in : 1;
   Eina_Bool grp_boss : 1;
   Overlay_Group *grp;

   Elm_Map_Overlay_Get_Cb cb;
   void *cb_data;
};

struct _Elm_Map_Marker_Class
{
   const char *style;
   struct _Elm_Map_Marker_Class_Func
     {
        Elm_Map_Marker_Get_Func get;
        Elm_Map_Marker_Del_Func del; //if NULL the object will be destroyed with evas_object_del()
        Elm_Map_Marker_Icon_Get_Func icon_get;
     } func;
};

struct _Elm_Map_Group_Class
{
   Widget_Data *wd;

   Eina_List *markers;
   int zoom_displayed; // display the group if the zoom is >= to zoom_display
   int zoom_grouped;   // group the markers only if the zoom is <= to zoom_groups
   const char *style;
   void *data;
   struct
     {
        Elm_Map_Group_Icon_Get_Func icon_get;
     } func;

   Eina_Bool hide : 1;
};

struct _Marker_Bubble
{
   Widget_Data *wd;
   Evas_Object *pobj;
   Evas_Object *obj, *sc, *bx;
};

struct _Elm_Map_Marker
{
   Widget_Data *wd;
   Elm_Map_Marker_Class *clas;
   Elm_Map_Group_Class *group_clas;
   double longitude, latitude;
   Evas_Coord w, h;
   Evas_Object *obj;

   Evas_Coord x, y;
   Eina_Bool grouped : 1;
   Eina_Bool leader : 1; // if marker is group leader
   Marker_Group *group;

   Marker_Bubble *bubble;
   Evas_Object *content;
   void *data;
};

struct _Marker_Group
{
   Widget_Data *wd;
   Elm_Map_Group_Class *clas;
   Evas_Coord w, h;
   Evas_Object *obj;

   Evas_Coord x, y;
   Eina_List *markers;

   Marker_Bubble *bubble;
};

struct _Elm_Map_Route
{
   Widget_Data *wd;

   char *fname;
   Elm_Map_Route_Type type;
   Elm_Map_Route_Method method;
   double flon, flat, tlon, tlat;
   Elm_Map_Route_Cb cb;
   void *data;
   Ecore_File_Download_Job *job;


   Eina_List *nodes;
   Eina_List *waypoint;
   struct
     {
        int node_count;
        int waypoint_count;
        const char *nodes;
        const char *waypoints;
        double distance; /* unit : km */
     } info;

   Path_Node *n;
   Path_Waypoint *w;

};

struct _Path_Node
{
   Widget_Data *wd;

   int idx;
   struct
     {
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

   int method;
   char *address;
   double lon, lat;

   char *fname;
   Ecore_File_Download_Job *job;
   Elm_Map_Name_Cb cb;
   void *data;
};

struct _Route_Dump
{
   int id;
   char *fname;
   double distance;
   char *description;
   char *coordinates;
};

struct _Name_Dump
{
   int id;
   char *address;
   double lon;
   double lat;
};

struct _Grid_Item
{
   Grid *g;

   Widget_Data *wd;
   Evas_Object *img;
   const char *file;
   const char *url;
   int x, y;  // Tile coordinate
   Eina_Bool file_have : 1;

   Ecore_File_Download_Job *job;
};

struct _Grid
{
   Widget_Data *wd;
   int zoom; // zoom level tiles want for optimal display (1, 2, 4, 8)
   int tw, th; // size of grid in tiles
   Eina_Matrixsparse *grid;
};

struct _Pan
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Widget_Data *wd;
};

struct _Widget_Data
{
   Evas_Object *obj;
   Evas_Object *scr;
   Evas_Object *ges;
   Evas_Object *pan_smart;
   Evas_Object *sep_maps_markers; // Tiles are below this and overlays are on top
   Evas_Map *map;

   Eina_Array *src_tile_mods;
   Source_Tile *src_tile;
   Eina_List *src_tiles;
   const char **src_tile_names;

   Eina_Array *src_route_mods;
   Source_Route *src_route;
   Eina_List *src_routes;
   const char **src_route_names;

   Eina_Array *src_name_mods;
   Source_Name *src_name;
   Eina_List *src_names;
   const char **src_name_names;

   int zoom_min, zoom_max;
   int tsize;

   int id;
   Eina_List *grids;

   int zoom;
   double zoom_detail;
   double prev_lon, prev_lat;
   Evas_Coord ox, oy;
   struct
     {
        int w, h;  // Current pixel width, heigth of a grid
        int tile;  // Current pixel size of a grid item
     } size;
   Elm_Map_Zoom_Mode mode;
   struct
     {
        double zoom;
        double diff;
        int cnt;
     } ani;
   Ecore_Timer *zoom_timer;
   Ecore_Animator *zoom_animator;

   int try_num;
   int finish_num;
   int download_num;
   Eina_List *download_list;
   Ecore_Idler *download_idler;
   Eina_Hash *ua;
   const char *user_agent;

   Evas_Coord pan_x, pan_y;
   Eina_List *delayed_jobs;

   Ecore_Timer *scr_timer;
   Ecore_Timer *long_timer;
   Evas_Event_Mouse_Down ev;
   Eina_Bool on_hold : 1;
   Eina_Bool paused : 1;

   double pinch_zoom;
   struct
     {
        Evas_Coord cx, cy;
        double a, d;
     } rotate;

   Eina_Bool wheel_disabled : 1;

   unsigned int markers_max_num;
   Eina_Bool paused_markers : 1;
   Eina_List *group_classes;
   Eina_List *marker_classes;
   Eina_List *markers;

   Eina_List *routes;
   Eina_List *track;
   Eina_List *names;

   Eina_List *overlays;
};

static char *_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_mapquest_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_mapquest_aerial_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_yours_url_cb(Evas_Object *obj __UNUSED__, const char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_nominatim_url_cb(Evas_Object *obj, int method, const char *name, double lon, double lat);
/*
static char *_monav_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
static char *_ors_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
*/

const Source_Tile src_tiles[] =
{
   {"Mapnik", 0, 18, _mapnik_url_cb, NULL, NULL},
   {"Osmarender", 0, 17, _osmarender_url_cb, NULL, NULL},
   {"CycleMap", 0, 16, _cyclemap_url_cb, NULL, NULL},
   {"MapQuest", 0, 18, _mapquest_url_cb, NULL, NULL},
   {"MapQuest Open Aerial", 0, 11, _mapquest_aerial_url_cb, NULL, NULL},
};

// FIXME: Fix more open sources
const Source_Route src_routes[] =
{
   {"Yours", _yours_url_cb}    // http://www.yournavigation.org/
   //{"Monav", _monav_url_cb},
   //{"ORS", _ors_url_cb},     // http://www.openrouteservice.org
};

// FIXME: Add more open sources
const Source_Name src_names[] =
{
   {"Nominatim", _nominatim_url_cb}
};

static const char *widtype = NULL;
static Evas_Smart_Class parent_sc = EVAS_SMART_CLASS_INIT_NULL;
static Evas_Smart_Class sc;
static Evas_Smart *smart;
static int idnum = 1;

static const char SIG_CLICKED[] =            "clicked";
static const char SIG_CLICKED_DOUBLE[] =     "clicked,double";
static const char SIG_PRESS[] =              "press";
static const char SIG_LONGPRESSED[] =        "longpressed";
static const char SIG_SCROLL[] =             "scroll";
static const char SIG_SCROLL_DRAG_START[] =  "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] =   "scroll,drag,stop";
static const char SIG_SCROLL_ANIM_START[] =  "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] =   "scroll,anim,stop";
static const char SIG_ZOOM_START[] =         "zoom,start";
static const char SIG_ZOOM_STOP[] =          "zoom,stop";
static const char SIG_ZOOM_CHANGE[] =        "zoom,change";
static const char SIG_TILE_LOAD[] =          "tile,load";
static const char SIG_TILE_LOADED[] =        "tile,loaded";
static const char SIG_TILE_LOADED_FAIL[] =   "tile,loaded,fail";
static const char SIG_ROUTE_LOAD[] =         "route,load";
static const char SIG_ROUTE_LOADED[] =       "route,loaded";
static const char SIG_ROUTE_LOADED_FAIL[] =  "route,loaded,fail";
static const char SIG_NAME_LOAD[] =          "name,load";
static const char SIG_NAME_LOADED[] =        "name,loaded";
static const char SIG_NAME_LOADED_FAIL[] =   "name,loaded,fail";
static const char SIG_OVERLAY_CLICKED[] =    "overlay,clicked";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CLICKED, ""},
       {SIG_CLICKED_DOUBLE, ""},
       {SIG_PRESS, ""},
       {SIG_LONGPRESSED, ""},
       {SIG_SCROLL, ""},
       {SIG_SCROLL_DRAG_START, ""},
       {SIG_SCROLL_DRAG_STOP, ""},
       {SIG_SCROLL_ANIM_START, ""},
       {SIG_SCROLL_ANIM_STOP, ""},
       {SIG_ZOOM_START, ""},
       {SIG_ZOOM_STOP, ""},
       {SIG_ZOOM_CHANGE, ""},
       {SIG_TILE_LOAD, ""},
       {SIG_TILE_LOADED, ""},
       {SIG_TILE_LOADED_FAIL, ""},
       {SIG_ROUTE_LOAD, ""},
       {SIG_ROUTE_LOADED, ""},
       {SIG_ROUTE_LOADED_FAIL, ""},
       {SIG_NAME_LOAD, ""},
       {SIG_NAME_LOADED, ""},
       {SIG_NAME_LOADED_FAIL, ""},
       {SIG_OVERLAY_CLICKED, ""},
       {NULL, NULL}
};

static void
_edj_marker_size_get(Widget_Data *wd, Evas_Coord *w, Evas_Coord *h)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(w);
   EINA_SAFETY_ON_NULL_RETURN(h);

   Evas_Object *edj;
   const char *s;

   edj = edje_object_add(evas_object_evas_get(wd->obj));
   _elm_theme_object_set(wd->obj, edj, "map/marker", "radio",
                         elm_widget_style_get(wd->obj));
   s = edje_object_data_get(edj, "size_w");
   if (s) *w = atoi(s);
   else   *w = 0;
   s = edje_object_data_get(edj, "size_h");
   if (s) *h = atoi(s);
   else   *h = 0;
   evas_object_del(edj);
}

static void
_coord_rotate(Evas_Coord x, Evas_Coord y, Evas_Coord cx, Evas_Coord cy, double degree, Evas_Coord *xx, Evas_Coord *yy)
{
   EINA_SAFETY_ON_NULL_RETURN(xx);
   EINA_SAFETY_ON_NULL_RETURN(yy);

   double r = (degree * M_PI) / 180.0;

   if (xx) *xx = ((x - cx) * cos(r)) + ((y - cy) * cos(r + M_PI_2)) + cx;
   if (yy) *yy = ((x - cx) * sin(r)) + ((y - cy) * sin(r + M_PI_2)) + cy;
}

static void
_obj_rotate(Widget_Data *wd, Evas_Object *obj)
{
   Evas_Coord w, h, ow, oh;
   evas_map_util_points_populate_from_object(wd->map, obj);

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   evas_object_image_size_get(obj, &w, &h);
   if ((w > ow) || (h > oh))
     {
        evas_map_point_image_uv_set(wd->map, 0, 0, 0);
        evas_map_point_image_uv_set(wd->map, 1, w, 0);
        evas_map_point_image_uv_set(wd->map, 2, w, h);
        evas_map_point_image_uv_set(wd->map, 3, 0, h);
     }
   evas_map_util_rotate(wd->map, wd->rotate.d, wd->rotate.cx, wd->rotate.cy);

   evas_object_map_set(obj, wd->map);
   evas_object_map_enable_set(obj, EINA_TRUE);
}

static void
_obj_place(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);

   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_show(obj);
}

static void
_coord_to_region_convert(Widget_Data *wd, Evas_Coord x, Evas_Coord y, Evas_Coord size, double *lon, double *lat)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   int zoom = floor(log(size / wd->size.tile) / log(2));
   if ((wd->src_tile) && (wd->src_tile->coord_to_geo))
     {
        if (wd->src_tile->coord_to_geo(wd->obj, zoom, x, y, size, lon, lat))
           return;
     }

   if (lon) *lon = (x / (double)size * 360.0) - 180;
   if (lat)
     {
        double n = ELM_PI - (2.0 * ELM_PI * y / size);
        *lat = 180.0 / ELM_PI * atan(0.5 * (exp(n) - exp(-n)));
     }
}

static void
_region_to_coord_convert(Widget_Data *wd, double lon, double lat, Evas_Coord size, Evas_Coord *x, Evas_Coord *y)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   int zoom = floor(log(size / 256) / log(2));
   if ((wd->src_tile) && (wd->src_tile->geo_to_coord))
     {
        if (wd->src_tile->geo_to_coord(wd->obj, zoom, lon, lat, size, x, y)) return;
     }

   if (x) *x = floor((lon + 180.0) / 360.0 * size);
   if (y)
      *y = floor((1.0 - log(tan(lat * ELM_PI / 180.0) + (1.0 / cos(lat * ELM_PI / 180.0)))
                 / ELM_PI) / 2.0 * size);
}

static void
_viewport_size_get(Widget_Data *wd, Evas_Coord *vw, Evas_Coord *vh)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(wd->pan_smart, &x, &y, &w, &h);
   if (vw) *vw = (x * 2) + w;
   if (vh) *vh = (y * 2) + h;
}

static void
_pan_geometry_get(Widget_Data *wd, Evas_Coord *px, Evas_Coord *py)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Evas_Coord x, y, vx, vy, vw, vh;
   elm_smart_scroller_child_pos_get(wd->scr, &x, &y);
   evas_object_geometry_get(wd->pan_smart, &vx, &vy, &vw, &vh);
   x = -x;
   y = -y;
   if (vw > wd->size.w) x += (((vw - wd->size.w) / 2) + vx);
   else x -= vx;
   if (vh > wd->size.h) y += (((vh - wd->size.h) / 2) + vy);
   else y -= vy;
   if (px) *px = x;
   if (py) *py = y;
 }

static void
_region_show(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;
   int x, y, w, h;

   _region_to_coord_convert(dd->wd, dd->lon, dd->lat, dd->wd->size.w, &x, &y);
   _viewport_size_get(dd->wd, &w, &h);
   x = x - (w / 2);
   y = y - (h / 2);
   elm_smart_scroller_child_region_show(dd->wd->scr, x, y, w, h);
   evas_object_smart_changed(dd->wd->pan_smart);
}

static void
_bubble_update(Marker_Bubble *bubble, Eina_List *contents)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(contents);

   Eina_List *l;
   Evas_Object *c;

   elm_box_clear(bubble->bx);
   EINA_LIST_FOREACH(contents, l, c) elm_box_pack_end(bubble->bx, c);
}

static void
_bubble_place(Marker_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   Evas_Coord x, y, w, h;
   Evas_Coord xx, yy, ww, hh;
   const char *s;

   if ((!bubble->obj) || (!bubble->pobj)) return;
   evas_object_geometry_get(bubble->pobj, &x, &y, &w, NULL);

   s = edje_object_data_get(bubble->obj, "size_w");
   if (s) ww = atoi(s);
   else ww = 0;

   edje_object_size_min_calc(bubble->obj, NULL, &hh);
   s = edje_object_data_get(bubble->obj, "size_h");
   if (s) h = atoi(s);
   else h = 0;
   if (hh < h) hh = h;

   xx = x + (w / 2) - (ww / 2);
   yy = y - hh;

   _obj_place(bubble->obj, xx, yy, ww, hh);
   evas_object_raise(bubble->obj);
}

static void
_bubble_sc_hints_changed_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Bubble *bubble = data;
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   _bubble_place(data);
}

static void
_bubble_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Bubble *bubble = data;
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   _bubble_place(bubble);
}

static void
_bubble_hide_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Bubble *bubble = data;
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   evas_object_hide(bubble->obj);
}

static void
_bubble_show_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Bubble *bubble = data;
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   _bubble_place(bubble);
}

static void
_bubble_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Marker_Bubble *bubble = data;
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   _bubble_place(bubble);
}

static void
_bubble_free(Marker_Bubble* bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   evas_object_del(bubble->bx);
   evas_object_del(bubble->sc);
   evas_object_del(bubble->obj);
   free(bubble);
}

static Marker_Bubble*
_bubble_create(Evas_Object *pobj, Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pobj, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Marker_Bubble *bubble = ELM_NEW(Marker_Bubble);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bubble, NULL);

   bubble->wd = wd;
   bubble->pobj = pobj;
   evas_object_event_callback_add(pobj, EVAS_CALLBACK_HIDE, _bubble_hide_cb,
                                  bubble);
   evas_object_event_callback_add(pobj, EVAS_CALLBACK_SHOW, _bubble_show_cb,
                                  bubble);
   evas_object_event_callback_add(pobj, EVAS_CALLBACK_MOVE, _bubble_move_cb,
                                  bubble);

   bubble->obj = edje_object_add(evas_object_evas_get(pobj));
   _elm_theme_object_set(wd->obj, bubble->obj , "map", "marker_bubble",
                         elm_widget_style_get(wd->obj));
   evas_object_event_callback_add(bubble->obj, EVAS_CALLBACK_MOUSE_UP,
                                  _bubble_mouse_up_cb, bubble);

   bubble->sc = elm_scroller_add(bubble->obj);
   elm_widget_style_set(bubble->sc, "map_bubble");
   elm_scroller_content_min_limit(bubble->sc, EINA_FALSE, EINA_TRUE);
   elm_scroller_policy_set(bubble->sc, ELM_SCROLLER_POLICY_AUTO,
                           ELM_SCROLLER_POLICY_OFF);
   elm_scroller_bounce_set(bubble->sc, _elm_config->thumbscroll_bounce_enable,
                           EINA_FALSE);
   edje_object_part_swallow(bubble->obj, "elm.swallow.content", bubble->sc);
   evas_object_event_callback_add(bubble->sc, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _bubble_sc_hints_changed_cb, bubble);

   bubble->bx = elm_box_add(bubble->sc);
   evas_object_size_hint_align_set(bubble->bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bubble->bx, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bubble->bx, EINA_TRUE);
   elm_object_content_set(bubble->sc, bubble->bx);

   return bubble;
}

static void
_marker_group_update(Marker_Group* group, Elm_Map_Group_Class *clas, Eina_List *markers)
{
   EINA_SAFETY_ON_NULL_RETURN(group);
   EINA_SAFETY_ON_NULL_RETURN(clas);
   EINA_SAFETY_ON_NULL_RETURN(markers);
   Widget_Data *wd = clas->wd;
   EINA_SAFETY_ON_NULL_RETURN(wd);

   char buf[PATH_MAX];
   Eina_List *l;
   Elm_Map_Marker *marker;
   int cnt = 0;
   int sum_x = 0, sum_y = 0;

   EINA_LIST_FOREACH(markers, l, marker)
     {
        sum_x += marker->x;
        sum_y += marker->y;
        cnt++;
     }

   group->x = sum_x / cnt;
   group->y = sum_y / cnt;
   _edj_marker_size_get(wd, &group->w, &group->h);
   group->w *=2;
   group->h *=2;
   group->clas = clas;
   group->markers = markers;

   if (clas->style) elm_layout_theme_set(group->obj, "map/marker", clas->style,
                                         elm_widget_style_get(wd->obj));
   else elm_layout_theme_set(group->obj, "map/marker", "radio",
                             elm_widget_style_get(wd->obj));


   if (clas->func.icon_get)
     {
        Evas_Object *icon = NULL;

        icon = elm_object_part_content_get(group->obj, "elm.icon");
        if (icon) evas_object_del(icon);

        icon = clas->func.icon_get(wd->obj, clas->data);
        elm_object_part_content_set(group->obj, "elm.icon", icon);
     }
   snprintf(buf, sizeof(buf), "%d", cnt);
   edje_object_part_text_set(elm_layout_edje_get(group->obj), "elm.text", buf);

   if (group->bubble)
      {
         Eina_List *contents = NULL;

         EINA_LIST_FOREACH(group->markers, l, marker)
           {
              Evas_Object *c = marker->clas->func.get(marker->wd->obj,
                                                      marker, marker->data);
              if (c) contents = eina_list_append(contents, c);
           }
         _bubble_update(group->bubble, contents);
      }
}

static void
_marker_group_bubble_open_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Marker_Group *group = data;
   Eina_List *l;
   Elm_Map_Marker *marker;
   Eina_List *contents = NULL;

   if (!group->bubble) group->bubble = _bubble_create(group->obj, group->wd);

   EINA_LIST_FOREACH(group->markers, l, marker)
     {
        if (group->wd->markers_max_num <= eina_list_count(contents)) break;
        Evas_Object *c = marker->clas->func.get(marker->wd->obj,
                                                marker, marker->data);
        if (c) contents = eina_list_append(contents, c);
     }
   _bubble_update(group->bubble, contents);
   _bubble_place(group->bubble);
}

static void
_marker_group_bringin_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);

   double lon, lat;
   Marker_Group *group = data;
   _coord_to_region_convert(group->wd, group->x, group->y, group->wd->size.w,
                         &lon, &lat);
   elm_map_region_bring_in(group->wd->obj, lon, lat);
}

static void
_marker_group_free(Marker_Group* group)
{
   EINA_SAFETY_ON_NULL_RETURN(group);

   if (group->bubble) _bubble_free(group->bubble);

   eina_list_free(group->markers);
   evas_object_del(group->obj);

   free(group);
}

static Marker_Group*
_marker_group_create(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Marker_Group *group = ELM_NEW(Marker_Group);

   group->wd = wd;
   group->obj = elm_layout_add(wd->obj);
   evas_object_smart_member_add(group->obj, wd->pan_smart);
   evas_object_stack_above(group->obj, wd->sep_maps_markers);
   elm_layout_theme_set(group->obj, "map/marker", "radio",
                        elm_widget_style_get(wd->obj));
   edje_object_signal_callback_add(elm_layout_edje_get(group->obj),
                                   "open", "elm", _marker_group_bubble_open_cb,
                                   group);
   edje_object_signal_callback_add(elm_layout_edje_get(group->obj),
                                   "bringin", "elm", _marker_group_bringin_cb,
                                   group);
   return group;
}

static void
_marker_bringin_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   Elm_Map_Marker *marker = data;
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_region_bring_in(marker->wd->obj, marker->longitude, marker->latitude);
}

static void
_marker_bubble_open_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *soure __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Elm_Map_Marker *marker = data;

   if (!marker->bubble) marker->bubble = _bubble_create(marker->obj, marker->wd);
   evas_object_smart_changed(marker->wd->pan_smart);
}

static void
_marker_update(Elm_Map_Marker *marker)
{
   EINA_SAFETY_ON_NULL_RETURN(marker);
   Elm_Map_Marker_Class *clas = marker->clas;
   EINA_SAFETY_ON_NULL_RETURN(clas);

   if (clas->style) elm_layout_theme_set(marker->obj, "map/marker", clas->style,
                                         elm_widget_style_get(marker->wd->obj));
   else elm_layout_theme_set(marker->obj, "map/marker", "radio",
                             elm_widget_style_get(marker->wd->obj));

   if (clas->func.icon_get)
     {
        Evas_Object *icon = NULL;

        icon = elm_object_part_content_get(marker->obj, "elm.icon");
        if (icon) evas_object_del(icon);

        icon = clas->func.icon_get(marker->wd->obj, marker, marker->data);
        elm_object_part_content_set(marker->obj, "elm.icon", icon);
     }
   _region_to_coord_convert(marker->wd, marker->longitude, marker->latitude,
                         marker->wd->size.w, &(marker->x), &(marker->y));

    if (marker->bubble)
      {
         if (marker->content) evas_object_del(marker->content);
         if (marker->clas->func.get)
            marker->content = marker->clas->func.get(marker->wd->obj, marker,
                                                     marker->data);
        if (marker->content)
          {
             Eina_List *contents = NULL;
             contents = eina_list_append(contents, marker->content);
             _bubble_update(marker->bubble, contents);
          }
      }
}



static void
_marker_place(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Eina_List *l;

   Elm_Map_Marker *marker;
   Elm_Map_Group_Class *group_clas;

   Evas_Coord gw, gh;
   Evas_Coord px, py;

   if (wd->paused_markers || (!eina_list_count(wd->markers))) return;

   _pan_geometry_get(wd, &px, &py);

   _edj_marker_size_get(wd, &gw, &gh);
   gw *= 2;
   gh *= 2;

   EINA_LIST_FOREACH(wd->markers, l, marker)
     {
        _marker_update(marker);
        marker->grouped = EINA_FALSE;
        marker->leader = EINA_FALSE;
     }

   EINA_LIST_FOREACH(wd->group_classes, l, group_clas)
     {
        Eina_List *ll;
        EINA_LIST_FOREACH(group_clas->markers, ll, marker)
          {
             Eina_List *lll;
             Elm_Map_Marker *mm;
             Eina_List *markers = NULL;

             if (marker->grouped) continue;
             if (group_clas->zoom_grouped < wd->zoom)
               {
                  marker->grouped = EINA_FALSE;
                  continue;
               }

             EINA_LIST_FOREACH(group_clas->markers, lll, mm)
               {
                  if (marker == mm || mm->grouped) continue;
                  if (ELM_RECTS_INTERSECT(mm->x, mm->y, mm->w, mm->h,
	                                  marker->x, marker->y, gw, gh))
	            {
	               // mm is group follower.
	               mm->leader = EINA_FALSE;
	               mm->grouped = EINA_TRUE;
	               markers = eina_list_append(markers, mm);
	            }
               }
              if (eina_list_count(markers) >= 1)
                {
                   // marker is group leader.
                   marker->leader = EINA_TRUE;
                   marker->grouped = EINA_TRUE;
                   markers = eina_list_append(markers, marker);

                   if (!marker->group) marker->group = _marker_group_create(wd);
                   _marker_group_update(marker->group, group_clas, markers);
                }
          }
     }

   EINA_LIST_FOREACH(wd->markers, l, marker)
     {

        if (marker->grouped ||
           (marker->group_clas &&
            (marker->group_clas->hide ||
             marker->group_clas->zoom_displayed > wd->zoom)))
           evas_object_hide(marker->obj);
        else
          {
             Evas_Coord x, y;
             _coord_rotate(marker->x + px, marker->y + py, wd->rotate.cx,
                           wd->rotate.cy, wd->rotate.d, &x, &y);
             _obj_place(marker->obj, x - (marker->w / 2), y - (marker->h / 2),
                         marker->w, marker->h);
          }
     }

   EINA_LIST_FOREACH(wd->markers, l, marker)
     {
        Marker_Group *group = marker->group;
        if (!group) continue;

        if (!marker->leader || (group->clas->hide) ||
            (group->clas->zoom_displayed > wd->zoom))
           evas_object_hide(group->obj);
        else
          {
             Evas_Coord x, y;
             _coord_rotate(group->x + px, group->y + py, wd->rotate.cx,
                           wd->rotate.cy, wd->rotate.d, &x, &y);
             _obj_place(group->obj, x - (group->w / 2), y - (group->h / 2),
                         group->w, group->h);
          }
     }
}

static void
_grid_item_coord_get(Grid_Item *gi, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);

   if (x) *x = gi->x * gi->wd->size.tile;
   if (y) *y = gi->y * gi->wd->size.tile;
   if (w) *w = gi->wd->size.tile;
   if (h) *h = gi->wd->size.tile;
}

static Eina_Bool
_grid_item_intersect(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(gi, EINA_FALSE);

   Evas_Coord px, py;
   Evas_Coord vw, vh;
   Evas_Coord x, y, w, h;

   _pan_geometry_get(gi->wd, &px, &py);
   _viewport_size_get(gi->wd, &vw, &vh);
   _grid_item_coord_get(gi, &x, &y, &w, &h);
   return ELM_RECTS_INTERSECT(x + px, y + py, w, h, 0, 0, vw, vh);
}

static void
_grid_item_update(Grid_Item *gi)
{
   evas_object_image_file_set(gi->img, gi->file, NULL);
   if (!gi->wd->zoom_timer && !gi->wd->scr_timer)
      evas_object_image_smooth_scale_set(gi->img, EINA_TRUE);
   else evas_object_image_smooth_scale_set(gi->img, EINA_FALSE);

   Evas_Load_Error err = evas_object_image_load_error_get(gi->img);
   if (err != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Image loading error (%s): %s", gi->file, evas_load_error_str(err));
        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
     }
   else
     {
        Evas_Coord px, py;
        Evas_Coord x, y, w, h;

        _pan_geometry_get(gi->wd, &px, &py);
        _grid_item_coord_get(gi, &x, &y, &w, &h);

        _obj_place(gi->img, x + px, y + py, w, h);
        _obj_rotate(gi->wd, gi->img);
        gi->file_have = EINA_TRUE;
     }
}

static void
_grid_item_load(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);
   if (gi->file_have) _grid_item_update(gi);
   else if (!gi->job)
     {
        gi->wd->download_list = eina_list_remove(gi->wd->download_list, gi);
        gi->wd->download_list = eina_list_append(gi->wd->download_list, gi);
     }
}

static void
_grid_item_unload(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);
   if (gi->file_have)
     {
        evas_object_hide(gi->img);
        evas_object_image_file_set(gi->img, NULL, NULL);
     }
   else if (gi->job)
     {
        ecore_file_download_abort(gi->job);
        ecore_file_remove(gi->file);
        gi->job = NULL;
        gi->wd->try_num--;
     }
   else gi->wd->download_list = eina_list_remove(gi->wd->download_list, gi);

}

static Grid_Item *
_grid_item_create(Grid *g, Evas_Coord x, Evas_Coord y)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(g, NULL);
   char buf[PATH_MAX];
   char buf2[PATH_MAX];
   char *url;
   Grid_Item *gi;

   gi = ELM_NEW(Grid_Item);
   gi->wd = g->wd;
   gi->g = g;
   gi->x = x;
   gi->y = y;

   gi->file_have = EINA_FALSE;
   gi->job = NULL;

   gi->img = evas_object_image_add(evas_object_evas_get(g->wd->obj));
   evas_object_image_smooth_scale_set(gi->img, EINA_FALSE);
   evas_object_image_scale_hint_set(gi->img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
   evas_object_image_filled_set(gi->img, 1);
   evas_object_smart_member_add(gi->img, g->wd->pan_smart);
   evas_object_pass_events_set(gi->img, EINA_TRUE);
   evas_object_stack_below(gi->img, g->wd->sep_maps_markers);

   snprintf(buf, sizeof(buf), CACHE_TILE_ROOT, g->wd->id, g->zoom, x);
   snprintf(buf2, sizeof(buf2), CACHE_TILE_PATH, buf, y);
   if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);

   eina_stringshare_replace(&gi->file, buf2);
   url = g->wd->src_tile->url_cb(g->wd->obj, x, y, g->zoom);
   if ((!url) || (!strlen(url)))
     {
        eina_stringshare_replace(&gi->url, NULL);
        ERR("Getting source url failed: %s", gi->file);
     }
   else eina_stringshare_replace(&gi->url, url);
   if (url) free(url);
   eina_matrixsparse_data_idx_set(g->grid, y, x, gi);
   return gi;
}

static void
_grid_item_free(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);
   _grid_item_unload(gi);
   if (gi->g && gi->g->grid) eina_matrixsparse_data_idx_set(gi->g->grid,
                                                            gi->y, gi->x, NULL);
   if (gi->url) eina_stringshare_del(gi->url);
   if (gi->file) eina_stringshare_del(gi->file);
   if (gi->img) evas_object_del(gi->img);
   if (gi->file_have) ecore_file_remove(gi->file);
   free(gi);
}

static void
_downloaded_cb(void *data, const char *file __UNUSED__, int status)
{
   Grid_Item *gi = data;

   if (status == 200)
     {
        DBG("Download success from %s to %s", gi->url, gi->file);
        _grid_item_update(gi);
        gi->wd->finish_num++;
        evas_object_smart_callback_call(gi->wd->obj, SIG_TILE_LOADED, NULL);
     }
   else
     {
        WRN("Download failed from %s to %s (%d) ", gi->url, gi->file, status);
        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
        evas_object_smart_callback_call(gi->wd->obj, SIG_TILE_LOADED_FAIL, NULL);
     }

   gi->job = NULL;
   gi->wd->download_num--;
   if (!gi->wd->download_num)
      edje_object_signal_emit(elm_smart_scroller_edje_object_get(gi->wd->scr),
                              "elm,state,busy,stop", "elm");
}

static Eina_Bool
_download_job(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_CANCEL);
   Widget_Data *wd = data;

   Eina_List *l, *ll;
   Grid_Item *gi;

   if (!eina_list_count(wd->download_list))
     {
        wd->download_idler = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   EINA_LIST_REVERSE_FOREACH_SAFE(wd->download_list, l, ll, gi)
     {
        if (gi->g->zoom != wd->zoom || !_grid_item_intersect(gi))
          {
             wd->download_list = eina_list_remove(wd->download_list, gi);
             continue;
          }
        if (wd->download_num >= MAX_CONCURRENT_DOWNLOAD)
           return ECORE_CALLBACK_RENEW;

        Eina_Bool ret = ecore_file_download_full(gi->url, gi->file,
                                                 _downloaded_cb, NULL,
                                                 gi, &(gi->job), wd->ua);
        if ((!ret) || (!gi->job))
           ERR("Can't start to download from %s to %s", gi->url, gi->file);
        else
          {
             wd->download_list = eina_list_remove(wd->download_list, gi);
             wd->try_num++;
             wd->download_num++;
             evas_object_smart_callback_call(gi->wd->obj, SIG_TILE_LOAD,
                                             NULL);
             if (wd->download_num == 1)
                edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                        "elm,state,busy,start", "elm");

          }
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_grid_viewport_get(Grid *g, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(g);
   int xx, yy, ww, hh;
   Evas_Coord px, py, vw, vh;

   _pan_geometry_get(g->wd, &px, &py);
   _viewport_size_get(g->wd, &vw, &vh);
   if (px > 0) px = 0;
   if (py > 0) py = 0;

   xx = (-px / g->wd->size.tile) - 1;
   if (xx < 0) xx = 0;

   yy = (-py / g->wd->size.tile) - 1;
   if (yy < 0) yy = 0;

   ww = (vw / g->wd->size.tile) + 3;
   if (xx + ww >= g->tw) ww = g->tw - xx;

   hh = (vh / g->wd->size.tile) + 3;
   if (yy + hh >= g->th) hh = g->th - yy;

   if (x) *x = xx;
   if (y) *y = yy;
   if (w) *w = ww;
   if (h) *h = hh;
}

static void
_grid_unload(Grid *g)
{
   EINA_SAFETY_ON_NULL_RETURN(g);
   Eina_Iterator *it;
   Eina_Matrixsparse_Cell *cell;
   Grid_Item *gi;

   it = eina_matrixsparse_iterator_new(g->grid);
   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);
        _grid_item_unload(gi);
     }
   eina_iterator_free(it);
}

static void
_grid_load(Grid *g)
{
   EINA_SAFETY_ON_NULL_RETURN(g);
   int x, y, xx, yy, ww, hh;
   Eina_Iterator *it;
   Eina_Matrixsparse_Cell *cell;
   Grid_Item *gi;

   it = eina_matrixsparse_iterator_new(g->grid);
   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);
        if (!_grid_item_intersect(gi)) _grid_item_unload(gi);
     }
   eina_iterator_free(it);

   _grid_viewport_get(g, &xx, &yy, &ww, &hh);
   for (y = yy; y < yy + hh; y++)
     {
        for (x = xx; x < xx + ww; x++)
          {
             gi = eina_matrixsparse_data_idx_get(g->grid, y, x);
             if (!gi) gi = _grid_item_create(g, x, y);
             _grid_item_load(gi);
          }
     }
}

static void
_grid_place(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   Eina_List *l;
   Grid *g;

   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        if (wd->zoom == g->zoom) _grid_load(g);
        else _grid_unload(g);
     }
  if (!wd->download_idler) wd->download_idler = ecore_idler_add(_download_job, wd);
}

static void
_grid_all_create(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src_tile);

   int zoom;
   for (zoom = wd->src_tile->zoom_min; zoom <= wd->src_tile->zoom_max; zoom++)
     {
        Grid *g;
        int tnum;
        g = ELM_NEW(Grid);
        g->wd = wd;
        g->zoom = zoom;
        tnum =  pow(2.0, g->zoom);
        g->tw = tnum;
        g->th = tnum;
        g->grid = eina_matrixsparse_new(g->th, g->tw, NULL, NULL);
        wd->grids = eina_list_append(wd->grids, g);
     }
}

static void
_grid_all_clear(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Grid *g;
   EINA_LIST_FREE(wd->grids, g)
     {
        Eina_Matrixsparse_Cell *cell;
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
        EINA_ITERATOR_FOREACH(it, cell)
          {
             Grid_Item *gi;
             gi = eina_matrixsparse_cell_data_get(cell);
             if (gi) _grid_item_free(gi);
          }
        eina_iterator_free(it);

        eina_matrixsparse_free(g->grid);
        free(g);
     }
}

static void
_track_place(Widget_Data *wd)
{
#ifdef ELM_EMAP
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Eina_List *l;
   Evas_Object *route;
   int xmin, xmax, ymin, ymax;
   Evas_Coord px, py, ow, oh;
   px = wd->pan_x;
   py = wd->pan_y;
   _viewport_size_get(wd, &ow, &oh);

   Evas_Coord size = wd->size.w;

   EINA_LIST_FOREACH(wd->track, l, route)
     {
        double lon_min, lon_max;
        double lat_min, lat_max;
        elm_route_longitude_min_max_get(route, &lon_min, &lon_max);
        elm_route_latitude_min_max_get(route, &lat_min, &lat_max);
        _region_to_coord_convert(wd, lon_min, lat_max, size, &xmin, &ymin);
        _region_to_coord_convert(wd, lon_max, lat_min, size, &xmax, &ymax);

        if( !(xmin < px && xmax < px) && !(xmin > px+ow && xmax > px+ow))
        {
           if( !(ymin < py && ymax < py) && !(ymin > py+oh && ymax > py+oh))
           {
              //display the route
              evas_object_move(route, xmin - px, ymin - py);
              evas_object_resize(route, xmax - xmin, ymax - ymin);

              evas_object_raise(route);
              _obj_rotate(wd, route);
              evas_object_show(route);

              continue;
           }
        }
        //the route is not display
        evas_object_hide(route);
     }
#else
   (void) wd;
#endif
}

static void
_delayed_do(Widget_Data *wd)
{
   Delayed_Data *dd;
   dd = eina_list_nth(wd->delayed_jobs, 0);
   if (dd && !dd->wd->zoom_animator)
     {
        dd->func(dd);
        wd->delayed_jobs = eina_list_remove(wd->delayed_jobs, dd);
        free(dd);
     }
}

static void
_smooth_update(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   Eina_List *l;
   Grid *g;

   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
        Eina_Matrixsparse_Cell *cell;

        EINA_ITERATOR_FOREACH(it, cell)
          {
             Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);
             if (_grid_item_intersect(gi))
                evas_object_image_smooth_scale_set(gi->img, EINA_TRUE);
          }
        eina_iterator_free(it);
     }
}

static Eina_Bool
_zoom_timeout(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_CANCEL);
   Widget_Data *wd = data;
   _smooth_update(wd);
   wd->zoom_timer = NULL;
   evas_object_smart_callback_call(wd->obj, SIG_ZOOM_STOP, NULL);
  return ECORE_CALLBACK_CANCEL;
}

static void
zoom_do(Widget_Data *wd, double zoom)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   if (zoom > wd->zoom_max) zoom = wd->zoom_max;
   else if (zoom < wd->zoom_min) zoom = wd->zoom_min;

   Evas_Coord px, py, vw, vh;
   Evas_Coord ow, oh;

   wd->zoom = ROUND(zoom);
   wd->zoom_detail = zoom;
   ow = wd->size.w;
   oh = wd->size.h;
   wd->size.tile = pow(2.0, (zoom - wd->zoom)) * wd->tsize;
   wd->size.w = pow(2.0, wd->zoom) * wd->size.tile;
   wd->size.h = wd->size.w;;

   // Fix to zooming with (viewport center px, py) as the center to prevent
   // from zooming with (0,0) as the cetner. (scroller default behavior)
   _pan_geometry_get(wd, &px, &py);
   _viewport_size_get(wd, &vw, &vh);
   if ((vw > 0) && (vh > 0) && (ow > 0) && (oh > 0))
     {
        Evas_Coord xx, yy;
        double sx, sy;
        if (vw > ow) sx = 0.5;
        else         sx = (double)(-px + (vw / 2)) / ow;
        if (vh > oh) sy = 0.5;
        else         sy = (double)(-py + (vh / 2)) / oh;

        if (sx > 1.0) sx = 1.0;
        if (sy > 1.0) sy = 1.0;

        xx = (sx * wd->size.w) - (vw / 2);
        yy = (sy * wd->size.h) - (vh / 2);
        if (xx < 0) xx = 0;
        else if (xx > (wd->size.w - vw)) xx = wd->size.w - vw;
        if (yy < 0) yy = 0;
        else if (yy > (wd->size.h - vh)) yy = wd->size.h - vh;
        elm_smart_scroller_child_region_show(wd->scr, xx, yy, vw, vh);
     }
   if (wd->zoom_timer) ecore_timer_del(wd->zoom_timer);
   else evas_object_smart_callback_call(wd->obj, SIG_ZOOM_START, NULL);
   wd->zoom_timer = ecore_timer_add(0.25, _zoom_timeout, wd);
   evas_object_smart_callback_call(wd->obj, SIG_ZOOM_CHANGE, NULL);

   evas_object_smart_callback_call(wd->pan_smart, "changed", NULL);
   evas_object_smart_changed(wd->pan_smart);
}

static Eina_Bool
_zoom_anim(void *data)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, ECORE_CALLBACK_CANCEL);

   if (wd->ani.cnt <= 0)
     {
        wd->zoom_animator = NULL;
        evas_object_smart_changed(wd->pan_smart);
        return ECORE_CALLBACK_CANCEL;
     }
   else
     {
        wd->ani.zoom += wd->ani.diff;
        wd->ani.cnt--;
        zoom_do(wd, wd->ani.zoom);
        return ECORE_CALLBACK_RENEW;
     }
}

static void
zoom_with_animation(Widget_Data *wd, double zoom, int cnt)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   if (cnt == 0) return;

   wd->ani.cnt = cnt;
   wd->ani.zoom = wd->zoom;
   wd->ani.diff = (double)(zoom - wd->zoom) / cnt;
   if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
   wd->zoom_animator = ecore_animator_add(_zoom_anim, wd);
}

static void
_sizing_eval(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Evas_Coord maxw = -1, maxh = -1;

   evas_object_size_hint_max_get(wd->scr, &maxw, &maxh);
   evas_object_size_hint_max_set(wd->obj, maxw, maxh);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static Eina_Bool
_scr_timeout(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_CANCEL);
   Widget_Data *wd = data;
   _smooth_update(wd);
   wd->scr_timer = NULL;
   evas_object_smart_callback_call(wd->obj, SIG_SCROLL_DRAG_STOP, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_scr(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;

   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   else evas_object_smart_callback_call(wd->obj, SIG_SCROLL_DRAG_START, NULL);
   wd->scr_timer = ecore_timer_add(0.25, _scr_timeout, wd);
   evas_object_smart_callback_call(wd->obj, SIG_SCROLL, NULL);
}

static void
_scr_anim_start(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   evas_object_smart_callback_call(wd->obj, SIG_SCROLL_ANIM_START, NULL);
}

static void
_scr_anim_stop(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   evas_object_smart_callback_call(wd->obj, SIG_SCROLL_ANIM_STOP, NULL);
}

static Eina_Bool
_long_press(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_CANCEL);
   Widget_Data *wd = data;

   wd->long_timer = NULL;
   evas_object_smart_callback_call(wd->obj, SIG_LONGPRESSED, &wd->ev);
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button != 1) return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(wd->obj, SIG_CLICKED_DOUBLE, ev);
   else evas_object_smart_callback_call(wd->obj, SIG_PRESS, ev);

   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   wd->ev = *ev;
   wd->long_timer = ecore_timer_add(_elm_config->longpress_timeout, _long_press, wd);
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;

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
   if (!wd->on_hold) evas_object_smart_callback_call(wd->obj, SIG_CLICKED, ev);
   wd->on_hold = EINA_FALSE;
}

static void
_mouse_wheel_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;

   if (!wd->paused)
     {
        Evas_Event_Mouse_Wheel *ev = (Evas_Event_Mouse_Wheel*) event_info;
        zoom_do(wd, wd->zoom_detail - ((double)ev->z / 10));
    }
}

static void
_region_max_min_get(Eina_List *overlays, double *max_longitude, double *min_longitude, double *max_latitude, double *min_latitude)
{
   double max_lon = -180, min_lon = 180;
   double max_lat = -90, min_lat = 90;
   Elm_Map_Overlay *overlay;
   EINA_LIST_FREE(overlays, overlay)
     {
        double lon, lat;
        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          {
             Overlay_Default *ovl = overlay->ovl;
             lon = ovl->lon;
             lat = ovl->lat;
          }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
          {
             // FIXME: class center coord is alright??
             Overlay_Class *ovl = overlay->ovl;
             double max_lo, min_lo, max_la, min_la;
             _region_max_min_get(ovl->members, &max_lo, &min_lo, &max_la,
                                 &min_la);
             lon = (max_lo + min_lo) / 2;
             lat = (max_la + min_la) / 2;
          }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
          {
             Overlay_Bubble *ovl = overlay->ovl;
             lon = ovl->lon;
             lat = ovl->lat;
          }
        else
          {
             WRN("Not supported overlay type: %d", overlay->type);
             continue;
          }
        if (lon> max_lon) max_lon = lon;
        if (lon< min_lon) min_lon = lon;
        if (lat > max_lat) max_lat = lat;
        if (lat < min_lat) min_lat = lat;
     }
   if (max_longitude) *max_longitude = max_lon;
   if (min_longitude) *min_longitude = min_lon;
   if (max_latitude) *max_latitude = max_lat;
   if (min_latitude) *min_latitude = min_lat;
}

static Evas_Object *
_icon_dup(Evas_Object *icon, Evas_Object *parent)
{
   if (!icon || !parent) return NULL;
   // Evas_Object do not support object duplication??
   const char *file = NULL, *group = NULL;
   Eina_Bool size_up, size_down;
   Evas_Object *dupp;

   dupp = elm_icon_add(parent);
   elm_icon_file_get(icon, &file, &group);
   elm_icon_file_set(dupp, file, group);
   elm_icon_animated_set(dupp, elm_icon_animated_get(icon));
   elm_icon_animated_play_set(dupp, elm_icon_animated_play_get(icon));
   elm_icon_standard_set(dupp, elm_icon_standard_get(icon));
   elm_icon_order_lookup_set(dupp, elm_icon_order_lookup_get(icon));
   elm_icon_no_scale_set(dupp, elm_icon_no_scale_get(icon));
   elm_icon_resizable_get(icon, &size_up, &size_down);
   elm_icon_resizable_set(dupp, size_up, size_down);
   elm_icon_fill_outside_set(dupp, elm_icon_fill_outside_get(icon));
   elm_icon_prescale_set(dupp, elm_icon_prescale_get(icon));
   elm_icon_aspect_fixed_set(dupp, elm_icon_aspect_fixed_get(icon));
   return dupp;
}

static Evas_Object *
_overlay_layout_new(Widget_Data *wd, const char *group)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, NULL);
   Evas_Object *obj = elm_layout_add(wd->obj);
   evas_object_smart_member_add(obj, wd->pan_smart);
   evas_object_stack_above(obj, wd->sep_maps_markers);
   elm_layout_theme_set(obj, "map/marker", group,  elm_widget_style_get(wd->obj));
   return obj;
}

static void
_overlay_layout_update(Widget_Data *wd, Evas_Object *layout, Evas_Object *icon, const char *text, const char *group)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(layout);

   Evas_Object *prev_icon = elm_object_part_content_get(layout, "elm.icon");
   if (icon && (prev_icon != icon))
     {
        elm_layout_theme_set(layout, "map/marker", "empty",
                             elm_widget_style_get(wd->obj));
        elm_object_part_content_set(layout, "elm.icon", icon);
     }
   else if (text)
     {
        if (group) elm_layout_theme_set(layout, "map/marker", group,
                                        elm_widget_style_get(wd->obj));
        edje_object_part_text_set(elm_layout_edje_get(layout), "elm.text", text);
     }
}

static void
_overlay_clicked_cb(void *data, Evas *e __UNUSED__,  Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Elm_Map_Overlay *overlay = data;

   evas_object_smart_callback_call(overlay->wd->obj, SIG_OVERLAY_CLICKED,
                                   overlay);
   if (overlay->cb) overlay->cb(overlay->cb_data, overlay->wd->obj,
                                overlay);
}

static void
_overlay_default_cb_add(Overlay_Default *ovl, Evas_Object_Event_Cb cb, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   EINA_SAFETY_ON_NULL_RETURN(data);

   // FIXME: Add icon or object event callback
   evas_object_event_callback_add(ovl->layout, EVAS_CALLBACK_MOUSE_DOWN, cb,
                                  data);
}

static void
_overlay_default_cb_del(Overlay_Default *ovl, Evas_Object_Event_Cb cb)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   // FIXME: Add icon or object event callback
   evas_object_event_callback_del(ovl->layout, EVAS_CALLBACK_MOUSE_DOWN, cb);
}

static void
_overlay_default_hide(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   if (ovl->obj) evas_object_hide(ovl->obj);
   if (ovl->layout) evas_object_hide(ovl->layout);
}

static void
_overlay_default_show(Widget_Data *wd, Overlay_Default *ovl)
{
   Evas_Object *disp;
   Evas_Coord px, py;
   Evas_Coord x, y, w, h;

   if (ovl->obj)
     {
        disp = ovl->obj;
        evas_object_geometry_get(disp, NULL, NULL, &w, &h);
        if (w <= 0 || h <= 0) evas_object_size_hint_min_get(disp, &w, &h);
     }
   else
     {
        disp = ovl->layout;
        w = ovl->w;
        h = ovl->h;
     }

   _pan_geometry_get(wd, &px, &py);
   _coord_rotate(ovl->x + px, ovl->y + py, wd->rotate.cx,  wd->rotate.cy,
                 wd->rotate.d, &x, &y);
   _obj_place(disp, x - (w / 2), y - (h / 2), w, h);
}

static void
_overlay_default_update(Widget_Data *wd, Overlay_Default *ovl, Evas_Object *obj, Evas_Object *icon, Overlay_Class *ovl_clas, const char *text, const char *group)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   _region_to_coord_convert(wd, ovl->lon, ovl->lat, wd->size.w, &ovl->x, &ovl->y);

   if (obj)
     {
        if (ovl->obj == obj) return;
        if (ovl->obj) evas_object_del(ovl->obj);
        ovl->obj = obj;
     }
   else if (!(ovl->obj) && icon)
     {
        if (ovl->icon == icon) return;
        if (ovl->icon) evas_object_del(ovl->icon);
        ovl->icon = icon;
       _overlay_layout_update(wd, ovl->layout, ovl->icon, NULL, NULL);

     }
   else if (!(ovl->obj) && !(ovl->icon) && (ovl_clas) &&
            ((ovl_clas->obj) || (ovl_clas->icon)))
     {
        // Inherit icon from group overlay's ojbect or icon

        // FIXME: It is hard to duplicate evas object :-)
        /*
        if (ovl_clas->obj && (ovl_clas->obj != ovl->clas_obj_ref))
          {
             if (ovl->clas_obj) evas_object_del(ovl->clas_obj);
             ovl->clas_obj_ref = ovl_clas->obj;
             ovl->clas_obj = _obj_dup(ovl->clas_obj_ref, ovl->layout);
             _overlay_layout_update(wd, ovl->layout, ovl->clas_obj, NULL, NULL);
          }
        */
        if (ovl_clas->icon && (ovl_clas->icon != ovl->clas_icon_ref))
          {
             if (ovl->clas_icon) evas_object_del(ovl->clas_icon);
             ovl->clas_icon_ref = ovl_clas->icon;
             ovl->clas_icon = _icon_dup(ovl->clas_icon_ref, ovl->layout);
             _overlay_layout_update(wd, ovl->layout, ovl->clas_icon, NULL, NULL);
        }
     }
   else if (!(ovl->obj) && !(ovl->icon) && !(ovl->clas_icon) && text)
     {
        _overlay_layout_update(wd, ovl->layout, NULL, text, group);
     }
}

static void
_overlay_default_free(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   if (ovl->obj) evas_object_del(ovl->obj);
   if (ovl->icon) evas_object_del(ovl->icon);
   if (ovl->clas_icon) evas_object_del(ovl->clas_icon);
   evas_object_event_callback_del(ovl->layout, EVAS_CALLBACK_MOUSE_DOWN,
                                  _overlay_clicked_cb);
   if (ovl->layout) evas_object_del(ovl->layout);
   free(ovl);
}

static Overlay_Default *
_overlay_default_new(Widget_Data *wd, double lon, double lat, const char *group)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, NULL);

   Overlay_Default *ovl = ELM_NEW(Overlay_Default);
   _edj_marker_size_get(wd, &(ovl->w), &(ovl->h));
   ovl->layout = _overlay_layout_new(wd, group);
   ovl->lon = lon;
   ovl->lat = lat;
   return ovl;
}

static void
_overlay_group_clicked_cb(void *data, Evas *e __UNUSED__,  Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Overlay_Group *ovl = data;

   evas_object_smart_callback_call(ovl->clas->wd->obj, SIG_OVERLAY_CLICKED,
                                   ovl->clas);
   if (ovl->clas->cb) ovl->clas->cb(ovl->clas->cb_data, ovl->clas->wd->obj,
                                    ovl->clas);
}

static void
_overlay_group_cb_add(Overlay_Group *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   _overlay_default_cb_add(ovl->ovl, _overlay_group_clicked_cb, ovl);
}

static void
_overlay_group_cb_del(Overlay_Group *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   _overlay_default_cb_del(ovl->ovl, _overlay_group_clicked_cb);
}

static void
_overlay_group_update(Widget_Data *wd, Overlay_Group *grp, Eina_List *members)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(grp);

   Eina_List *l;
   Elm_Map_Overlay *overlay;
   Evas_Coord sum_x = 0, sum_y = 0, cnt = 0;

   if (grp->members) eina_list_free(grp->members);
   grp->members = members;

   if (!grp->members || eina_list_count(grp->members) <= 0)
     {
        _overlay_default_hide(grp->ovl);
        return;
     }
   EINA_LIST_FOREACH(grp->members, l, overlay)
     {
        Overlay_Default *df = overlay->ovl;
        sum_x += df->x;
        sum_y += df->y;
        cnt++;
     }

   Overlay_Class *ovl_clas = grp->clas->ovl;

   char text[128];
   snprintf(text, sizeof(text), "%d", cnt);
   _overlay_default_update(wd, grp->ovl, NULL, NULL, ovl_clas, text, "radio2");

   grp->ovl->x = sum_x / cnt;
   grp->ovl->y = sum_y / cnt;
}

static void
_overlay_group_free(Overlay_Group *grp, Elm_Map_Overlay *club_owner)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   _overlay_default_cb_del(grp->ovl, _overlay_group_clicked_cb);
   _overlay_default_free(grp->ovl);
   if (grp->clas) elm_map_overlay_class_remove(grp->clas, club_owner);
   if (grp->members) eina_list_free(grp->members);
   free(grp);
}

static Overlay_Group *
_overlay_group_new(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Overlay_Group *grp = ELM_NEW(Overlay_Group);
   grp->ovl = ELM_NEW(Overlay_Default);
   grp->ovl = _overlay_default_new(wd, -1, -1, "radio2");
   grp->ovl->w *= 2;
   grp->ovl->h *= 2;
   return grp;
}

static void
_overlay_class_update(Widget_Data *wd, Overlay_Class *clas, Evas_Object *obj, Evas_Object *icon)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(clas);

   if (obj && (clas->obj != obj))
     {
        if (clas->obj) evas_object_del(clas->obj);
        clas->obj = icon;
     }
   else if (icon && (clas->icon != icon))
     {
        if (clas->icon) evas_object_del(clas->icon);
        clas->icon = icon;
     }
}

static void
_overlay_class_free(Overlay_Class *clas)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   if (clas->icon) evas_object_del(clas->icon);
   if (clas->members) eina_list_free(clas->members);
   free(clas);
}

static Overlay_Class *
_overlay_class_new(Widget_Data *wd, Elm_Map_Overlay *clas)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   Overlay_Class *ovl = ELM_NEW(Overlay_Class);
   ovl->clas = clas;
   ovl->icon = NULL;
   ovl->zoom_max = OVERLAY_CLASS_ZOOM_MAX;
   return ovl;
}

static void
_overlay_bubble_cb_add(Overlay_Bubble *ovl, Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   evas_object_event_callback_add(ovl->obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _overlay_clicked_cb, overlay);
}

static void
_overlay_bubble_cb_del(Overlay_Bubble *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   evas_object_event_callback_del(ovl->obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _overlay_clicked_cb);
}

static void
_overlay_bubble_hide(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   if (bubble->obj) evas_object_hide(bubble->obj);
}

static void
_overlay_bubble_update(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   if ((!bubble->pobj) && (bubble->lon >= 0) && (bubble->lat >= 0))
     {
        _region_to_coord_convert(bubble->wd, bubble->lon, bubble->lat,
                              bubble->wd->size.w, &bubble->x, &bubble->y);
     }
}

static void
_overlay_bubble_show(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   Evas_Coord x, y;

   if ((bubble->x < 0) || (bubble->y < 0)) return;
   Evas_Coord px, py;
   _pan_geometry_get(bubble->wd, &px, &py);
   _coord_rotate(bubble->x + px, bubble->y + py, bubble->wd->rotate.cx,
                 bubble->wd->rotate.cy, bubble->wd->rotate.d, &x, &y);
   x = x - (bubble->w / 2);
   y = y - (bubble->h / 2);
   _obj_place(bubble->obj, x, y, bubble->w, bubble->h);
   //evas_object_raise(bubble->obj);
}

static void
_overlay_bubble_chase(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(bubble->pobj);

   Evas_Coord x, y, w;
   evas_object_geometry_get(bubble->pobj, &x, &y, &w, NULL);
   x = x + (w / 2) - (bubble->w / 2);
   y = y - bubble->h;
   _obj_place(bubble->obj, x, y, bubble->w, bubble->h);
   evas_object_raise(bubble->obj);
}

static void
_overlay_bubble_hide_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   _overlay_bubble_hide(data);
}

static void
_overlay_bubble_chase_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   _overlay_bubble_chase(data);
}

static void
_overlay_bubble_free(Overlay_Bubble* bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   evas_object_del(bubble->bx);
   evas_object_del(bubble->sc);
   evas_object_del(bubble->obj);
   if (bubble->pobj)
     {
        evas_object_event_callback_del_full(bubble->pobj, EVAS_CALLBACK_HIDE,
                                            _overlay_bubble_hide_cb, bubble);
        evas_object_event_callback_del_full(bubble->pobj, EVAS_CALLBACK_SHOW,
                                            _overlay_bubble_chase_cb, bubble);
        evas_object_event_callback_del_full(bubble->pobj, EVAS_CALLBACK_MOVE,
                                            _overlay_bubble_chase_cb, bubble);
     }
   free(bubble);
}

static Overlay_Bubble*
_overlay_bubble_new(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Evas_Coord h;
   const char *s;
   Overlay_Bubble *bubble = ELM_NEW(Overlay_Bubble);
   bubble->wd = wd;

   bubble->obj = edje_object_add(evas_object_evas_get(wd->obj));
   _elm_theme_object_set(wd->obj, bubble->obj , "map", "marker_bubble",
                         elm_widget_style_get(wd->obj));
   evas_object_event_callback_add(bubble->obj, EVAS_CALLBACK_MOUSE_UP,
                                  _overlay_bubble_chase_cb, bubble);

   bubble->sc = elm_scroller_add(bubble->obj);
   elm_widget_style_set(bubble->sc, "map_bubble");
   elm_scroller_content_min_limit(bubble->sc, EINA_FALSE, EINA_TRUE);
   elm_scroller_policy_set(bubble->sc, ELM_SCROLLER_POLICY_AUTO,
                           ELM_SCROLLER_POLICY_OFF);
   elm_scroller_bounce_set(bubble->sc, _elm_config->thumbscroll_bounce_enable,
                           EINA_FALSE);
   edje_object_part_swallow(bubble->obj, "elm.swallow.content", bubble->sc);

   bubble->bx = elm_box_add(bubble->sc);
   evas_object_size_hint_align_set(bubble->bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bubble->bx, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bubble->bx, EINA_TRUE);
   elm_object_content_set(bubble->sc, bubble->bx);

   s = edje_object_data_get(bubble->obj, "size_w");
   if (s) bubble->w = atoi(s);
   else bubble->w = 0;

   edje_object_size_min_calc(bubble->obj, NULL, &(bubble->h));
   s = edje_object_data_get(bubble->obj, "size_h");
   if (s) h = atoi(s);
   else h = 0;
   if (bubble->h < h) bubble->h = h;

   bubble->lon = -1;
   bubble->lat = -1;
   bubble->x = -1;
   bubble->y = -1;
   return bubble;
}

static void
_overlay_route_hide(Overlay_Route *r)
{
   EINA_SAFETY_ON_NULL_RETURN(r);
   Eina_List *l;
   Evas_Object *p;
   EINA_LIST_FOREACH(r->paths, l, p) evas_object_hide(p);
}

static void
_overlay_route_show(Overlay_Route *r)
{
   EINA_SAFETY_ON_NULL_RETURN(r);
   EINA_SAFETY_ON_NULL_RETURN(r->wd);

   Widget_Data *wd;
   Eina_List *l;
   Evas_Object *p;
   Path_Node *n;
   int cnt;
   int x, y;
   double a;
   Evas_Coord ow, oh, px, py, size;

   wd = r->wd;
   _viewport_size_get(wd, &ow, &oh);
   px = wd->pan_x;
   py = wd->pan_y;
   size = wd->size.w;

   EINA_LIST_FOREACH(r->paths, l, p) evas_object_polygon_points_clear(p);

   cnt = eina_list_count(r->nodes);
   EINA_LIST_FOREACH(r->nodes, l, n)
     {
        if ((!wd->zoom) || ((n->idx) &&
            ((n->idx % (int)ceil((double)cnt/(double)size*100.0))))) continue;
        if (r->inbound)
          {
             _region_to_coord_convert(wd, n->pos.lon, n->pos.lat, size,
                                   &x, &y);
             if ((x >= px - ow) && (x <= (px + ow*2)) &&
                 (y >= py - oh) && (y <= (py + oh*2)))
               {
                  x = x - px;
                  y = y - py;

                  p = eina_list_nth(r->paths, n->idx);
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

                  evas_object_color_set(p, r->color.r, r->color.g, r->color.b,
                                        r->color.a);
                  evas_object_raise(p);
                  _obj_rotate(wd, p);
                  evas_object_show(p);
                  r->x = x;
                  r->y = y;
               }

             else r->inbound = EINA_FALSE;
          }
        else
          {
             _region_to_coord_convert(wd, n->pos.lon, n->pos.lat, size,
                                   &x, &y);
             if ((x >= px - ow) && (x <= (px + ow*2)) &&
                 (y >= py - oh) && (y <= (py + oh*2)))
               {
                  r->x = x - px;
                  r->y = y - py;
                  r->inbound = EINA_TRUE;
               }
             else r->inbound = EINA_FALSE;
          }
     }
   r->inbound = EINA_FALSE;
}

static void
_overlay_route_free(Overlay_Route* route)
{
   EINA_SAFETY_ON_NULL_RETURN(route);
   Evas_Object *p;
   Path_Node *n;

   EINA_LIST_FREE(route->paths, p) evas_object_del(p);
   EINA_LIST_FREE(route->nodes, n)
     {
        if (n->pos.address) eina_stringshare_del(n->pos.address);
        free(n);
     }
   free(route);
}

static Overlay_Route *
_overlay_route_new(Widget_Data *wd, const Elm_Map_Route *route)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);

   Eina_List *l;
   Path_Node *n;

   Overlay_Route *ovl = ELM_NEW(Overlay_Route);
   ovl->wd = wd;
   ovl->inbound = EINA_FALSE;
   ovl->color.r = 255;
   ovl->color.g = 0;
   ovl->color.b = 0;
   ovl->color.a = 255;

    EINA_LIST_FOREACH(route->nodes, l, n)
     {
        Evas_Object *path;
        Path_Node *node;

        node = ELM_NEW(Path_Node);
        node->idx = n->idx;
        node->pos.lon = n->pos.lon;
        node->pos.lat = n->pos.lat;
        if (n->pos.address) node->pos.address = strdup(n->pos.address);
        ovl->nodes = eina_list_append(ovl->nodes, node);

        path = evas_object_polygon_add(evas_object_evas_get(wd->obj));
        evas_object_smart_member_add(path, wd->pan_smart);
        ovl->paths = eina_list_append(ovl->paths, path);
     }
   return ovl;
}

static void
_overlay_grouping(Eina_List *members, Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN(members);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   // Currently support only basic overlay type
   EINA_SAFETY_ON_FALSE_RETURN(overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT);

   Eina_List *l;
   Elm_Map_Overlay *overlay_memb;
   Eina_List *club_members = NULL;

   // Grouping only supports overlay default
   Overlay_Default *ovl = (Overlay_Default *)overlay->ovl;
   EINA_LIST_FOREACH(members, l, overlay_memb)
     {
        Overlay_Default *ovl_memb = overlay_memb->ovl;
        if ((overlay_memb->hide) ||
            (overlay_memb->zoom_min > overlay_memb->wd->zoom)) continue;
        if (overlay == overlay_memb || overlay_memb->grp_in) continue;
        if (ELM_RECTS_INTERSECT(ovl_memb->x, ovl_memb->y, ovl_memb->w,
                                ovl_memb->h, ovl->x, ovl->y,
                                ovl->w * OVERLAY_GROUPING_SCALE,
                                ovl->h * OVERLAY_GROUPING_SCALE))
          {
             // Join group.
             overlay_memb->grp_boss = EINA_FALSE;
             overlay_memb->grp_in = EINA_TRUE;
             club_members = eina_list_append(club_members, overlay_memb);
             _overlay_group_update(overlay_memb->wd, overlay_memb->grp, NULL);
             _overlay_group_cb_del(overlay_memb->grp);
          }
     }

   if (eina_list_count(club_members) >= 1)
     {
        // Mark as boss
        overlay->grp_boss = EINA_TRUE;
        overlay->grp_in = EINA_TRUE;
        club_members = eina_list_append(club_members, overlay);
        _overlay_group_update(overlay->wd, overlay->grp, club_members);
        _overlay_group_cb_del(overlay->grp);
        _overlay_group_cb_add(overlay->grp);
     }
}

static void
_overlay_display(Widget_Data *wd, Elm_Map_Overlay *overlay)
{
   Eina_Bool hide = EINA_FALSE;

   if ((overlay->grp_in) || (overlay->hide) || (overlay->zoom_min > wd->zoom))
      hide = EINA_TRUE;
   if ((overlay->grp->clas) && ((overlay->grp->clas->hide) ||
                                (overlay->grp->clas->zoom_min > wd->zoom)))
      hide = EINA_TRUE;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        if (hide) _overlay_default_hide(overlay->ovl);
        else _overlay_default_show(wd, overlay->ovl);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        if (hide) _overlay_bubble_hide(overlay->ovl);
        else _overlay_bubble_show(overlay->ovl);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     {
       if (hide) _overlay_route_hide(overlay->ovl);
       else _overlay_route_show(overlay->ovl);
     }
}

static void
_overlay_place(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Eina_List *l, *ll;
   Elm_Map_Overlay *overlay, *grp;

   if (eina_list_count(wd->overlays) == 0) return;

   // Reset overlays coord & grp except class type
   EINA_LIST_FOREACH(wd->overlays, l, overlay)
     {

        if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS) continue;
        if (overlay->paused) continue;
        if ((overlay->grp) && (overlay->grp->clas) &&
            (overlay->grp->clas->paused)) continue;
        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          {
             Overlay_Class *ovl_grp = NULL;
             if (overlay->grp->clas) ovl_grp = overlay->grp->clas->ovl;
             _overlay_default_update(wd, overlay->ovl, NULL, NULL, ovl_grp,
                                     NULL, NULL);
           }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
           _overlay_bubble_update(overlay->ovl);
        // Reset grp flags
        overlay->grp_in = EINA_FALSE;
        overlay->grp_boss = EINA_FALSE;
        _overlay_group_update(wd, overlay->grp, NULL);
     }

   // Classify into group idol or follwer
   EINA_LIST_FOREACH(wd->overlays, l, grp)
     {
        Elm_Map_Overlay *idol;
        Overlay_Class *ovl;

        if (grp->type != ELM_MAP_OVERLAY_TYPE_CLASS) continue;
        if ((grp->hide) || (grp->zoom_min > wd->zoom)) continue;

        ovl = grp->ovl;
        if (ovl->zoom_max < wd->zoom) continue;
        EINA_LIST_FOREACH(ovl->members, ll, idol)
          {
             if (!idol->grp_in) _overlay_grouping(ovl->members, idol);
          }
     }

   // Place overlays
   EINA_LIST_FOREACH(wd->overlays, l, overlay)
      if (overlay->type != ELM_MAP_OVERLAY_TYPE_CLASS)
         _overlay_display(wd, overlay);

   // Place group overlays
   EINA_LIST_FOREACH(wd->overlays, l, overlay)
     {
        if (overlay->grp_boss) _overlay_default_show(wd, overlay->grp->ovl);
     }
}

static Evas_Object *
_overlay_obj_get(Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;
        return ovl->layout;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
}

static void
_overlays_show(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;

   int zoom;
   double max_lon, min_lon, max_lat, min_lat;
   Evas_Coord vw, vh;

   _region_max_min_get(dd->overlays, &max_lon, &min_lon, &max_lat, &min_lat);
   dd->lon = (max_lon + min_lon) / 2;
   dd->lat = (max_lat + min_lat) / 2;

   zoom = dd->wd->src_tile->zoom_min;
   _viewport_size_get(dd->wd, &vw, &vh);
   while (zoom <= dd->wd->src_tile->zoom_max)
     {
        Evas_Coord size, max_x, max_y, min_x, min_y;
        size = pow(2.0, zoom) * dd->wd->tsize;
        _region_to_coord_convert(dd->wd, min_lon, max_lat, size, &min_x, &max_y);
        _region_to_coord_convert(dd->wd, max_lon, min_lat, size, &max_x, &min_y);
        if ((max_x - min_x) > vw || (max_y - min_y) > vh) break;
        zoom++;
     }
   zoom--;

   zoom_do(dd->wd, zoom);
   _region_show(dd);
   evas_object_smart_changed(dd->wd->pan_smart);
}

static void
_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(sd);
   if ((x == sd->wd->pan_x) && (y == sd->wd->pan_y)) return;

   sd->wd->pan_x = x;
   sd->wd->pan_y = y;
   evas_object_smart_changed(obj);
}

static void
_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(sd);
   if (x) *x = sd->wd->pan_x;
   if (y) *y = sd->wd->pan_y;
}

static void
_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Pan *sd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(sd);
   Evas_Coord ow, oh;
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ow = sd->wd->size.w - ow;
   oh = sd->wd->size.h - oh;
   if (ow < 0) ow = 0;
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
   EINA_SAFETY_ON_NULL_RETURN(sd);
   if (w) *w = sd->wd->size.w;
   if (h) *h = sd->wd->size.h;
}

static void
_pan_add(Evas_Object *obj)
{
   Pan *sd;
   Evas_Object_Smart_Clipped_Data *cd;
   parent_sc.add(obj);
   cd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(cd);
   sd = ELM_NEW(Pan);
   sd->__clipped_data = *cd;
   free(cd);
   evas_object_smart_data_set(obj, sd);
}

static void
_pan_resize(Evas_Object *obj, Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__)
{
   Pan *sd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(sd);

   _sizing_eval(sd->wd);
   elm_map_zoom_mode_set(sd->wd->obj, sd->wd->mode);
   evas_object_smart_changed(obj);
}

static void
_pan_calculate(Evas_Object *obj)
{
   Pan *sd = evas_object_smart_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(sd);

   Evas_Coord w, h;
   evas_object_geometry_get(sd->wd->pan_smart, NULL, NULL, &w, &h);
   if (w <= 0 || h <= 0) return;

   _grid_place(sd->wd);
   _marker_place(sd->wd);
   _overlay_place(sd->wd);
   _track_place(sd->wd);
   _delayed_do(sd->wd);
}

static void
_pan_move(Evas_Object *obj, Evas_Coord x __UNUSED__, Evas_Coord y __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);
   evas_object_smart_changed(obj);
}

static void
_hold_on(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   elm_smart_scroller_hold_set(wd->scr, 1);
}

static void
_hold_off(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   elm_smart_scroller_hold_set(wd->scr, 0);
}

static void
_freeze_on(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   elm_smart_scroller_freeze_set(wd->scr, 1);
}

static void
_freeze_off(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Widget_Data *wd = data;
   elm_smart_scroller_freeze_set(wd->scr, 0);
}

static void
_elm_map_marker_remove(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   Widget_Data *wd = marker->wd;
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if ((marker->content) && (marker->clas->func.del))
      marker->clas->func.del(wd->obj, marker, marker->data, marker->content);

   if (marker->bubble) _bubble_free(marker->bubble);
   if (marker->group) _marker_group_free(marker->group);

   if (marker->group_clas)
      marker->group_clas->markers = eina_list_remove(marker->group_clas->markers, marker);
   wd->markers = eina_list_remove(wd->markers, marker);

   evas_object_del(marker->obj);
   free(marker);

   evas_object_smart_changed(wd->pan_smart);
#else
   (void) marker;
#endif
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
_kml_parse(Elm_Map_Route *r)
{
   EINA_SAFETY_ON_NULL_RETURN(r);
   EINA_SAFETY_ON_NULL_RETURN(r->fname);

   FILE *f;
   char **str;
   unsigned int ele, idx;
   double lon, lat;

   Route_Dump dump = {0, r->fname, 0.0, NULL, NULL};

   f = fopen(r->fname, "rb");
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
                       eina_simple_xml_parse(buf, sz, EINA_TRUE, cb_route_dump,
                                             &dump);
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
             for (idx = 0; idx < ele; idx++)
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
             for (idx = 0; idx < ele; idx++)
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
_name_parse(Elm_Map_Name *n)
{
   EINA_SAFETY_ON_NULL_RETURN(n);
   EINA_SAFETY_ON_NULL_RETURN(n->fname);

   FILE *f;

   Name_Dump dump = {0, NULL, 0.0, 0.0};

   f = fopen(n->fname, "rb");
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

static void
_route_cb(void *data, const char *file,  int status)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(file);

   Elm_Map_Route *route = data;
   Widget_Data *wd = route->wd;
   EINA_SAFETY_ON_NULL_RETURN(wd);

   route->job = NULL;
   if (status == 200)
     {
        _kml_parse(route);
        INF("Route request success from (%lf, %lf) to (%lf, %lf)",
            route->flon, route->flat, route->tlon, route->tlat);
        if (route->cb) route->cb(route->data, wd->obj, route);
        evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOADED, NULL);
     }
   else
     {
        ERR("Route request failed: %d", status);
        if (route->cb) route->cb(route->data, wd->obj, NULL);
        evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOADED_FAIL, NULL);
     }

   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,stop", "elm");
}

static void
_name_cb(void *data, const char *file,  int status)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(file);

   Elm_Map_Name *name = data;
   Widget_Data *wd = name->wd;
   EINA_SAFETY_ON_NULL_RETURN(wd);

   name->job = NULL;
   if (status == 200)
     {
        _name_parse(name);
        INF("Name request success address:%s, lon:%lf, lat:%lf",
            name->address, name->lon, name->lat);
        if (name->cb) name->cb(name->data, wd->obj, name);
        evas_object_smart_callback_call(wd->obj, SIG_NAME_LOADED, NULL);
     }
   else
     {
        ERR("Name request failed: %d", status);
        if (name->cb) name->cb(name->data, wd->obj, NULL);
        evas_object_smart_callback_call(wd->obj, SIG_NAME_LOADED_FAIL, NULL);
     }
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,stop", "elm");
}



static Elm_Map_Name *
_name_request(const Evas_Object *obj, int method, const char *address, double lon, double lat, Elm_Map_Name_Cb name_cb, void *data)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd->src_name, NULL);


   char *url;
   char fname[PATH_MAX];

   if (!ecore_file_exists(CACHE_NAME_ROOT)) ecore_file_mkpath(CACHE_NAME_ROOT);

   url = wd->src_name->url_cb(wd->obj, method, address, lon, lat);
   if (!url)
     {
        ERR("Name URL is NULL");
        return NULL;
     }

   Elm_Map_Name *name = ELM_NEW(Elm_Map_Name);
   name->wd = wd;
   snprintf(fname, sizeof(fname), CACHE_NAME_ROOT"/%d", rand());
   name->fname = strdup(fname);
   name->method = method;
   if (method == ELM_MAP_NAME_METHOD_SEARCH) name->address = strdup(address);
   else if (method == ELM_MAP_NAME_METHOD_REVERSE)
     {
        name->lon = lon;
        name->lat = lat;
     }
   name->cb = name_cb;
   name->data = data;

   if (!ecore_file_download_full(url, name->fname, _name_cb, NULL, name,
                                 &(name->job), wd->ua) || !(name->job))
     {
        ERR("Can't request Name from %s to %s", url, name->fname);
        if (name->address) free(name->address);
        free(name->fname);
        free(name);
        return NULL;
     }
   INF("Name requested from %s to %s", url, name->fname);
   free(url);

   wd->names = eina_list_append(wd->names, name);
   evas_object_smart_callback_call(wd->obj, SIG_NAME_LOAD, name);
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,start", "elm");
   return name;
}

static Evas_Event_Flags
_pinch_zoom_start_cb(void *data, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EVAS_EVENT_FLAG_NONE);
   Widget_Data *wd = data;

   wd->pinch_zoom = wd->zoom_detail;
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_zoom_cb(void *data, void *event_info)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   if (!wd->paused)
     {
        Elm_Gesture_Zoom_Info *ei = event_info;
        zoom_do(wd, wd->pinch_zoom + ei->zoom - 1);
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_rotate_cb(void *data, void *event_info)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   if (!wd->paused)
     {
        int x, y, w, h;
        Elm_Gesture_Rotate_Info *ei = event_info;
        evas_object_geometry_get(wd->obj, &x, &y, &w, &h);

        wd->rotate.d = wd->rotate.a + ei->angle - ei->base_angle;
        wd->rotate.cx = x + ((double)w * 0.5);
        wd->rotate.cy = y + ((double)h * 0.5);

        evas_object_smart_changed(wd->pan_smart);
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_rotate_end_cb(void *data, void *event_info __UNUSED__)
{
   Widget_Data *wd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EVAS_EVENT_FLAG_NONE);

   wd->rotate.a = wd->rotate.d;

   return EVAS_EVENT_FLAG_NONE;
}

static Eina_Bool
_source_tile_mod_cb(Eina_Module *m, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   Widget_Data *wd = data;
   Source_Tile *s;
   Elm_Map_Module_Source_Name_Func name_cb;
   Elm_Map_Module_Tile_Zoom_Min_Func zoom_min;
   Elm_Map_Module_Tile_Zoom_Max_Func zoom_max;
   Elm_Map_Module_Tile_Url_Func url_cb;
   Elm_Map_Module_Tile_Geo_to_Coord_Func geo_to_coord;
   Elm_Map_Module_Tile_Coord_to_Geo_Func coord_to_geo;
   const char *file;

   file = eina_module_file_get(m);
   if (!eina_module_load(m))
     {
        ERR("Could not load module \"%s\": %s", file,
            eina_error_msg_get(eina_error_get()));
        return EINA_FALSE;
     }

   name_cb = eina_module_symbol_get(m, "map_module_source_name_get");
   zoom_min = eina_module_symbol_get(m, "map_module_tile_zoom_min_get");
   zoom_max = eina_module_symbol_get(m, "map_module_tile_zoom_max_get");
   url_cb = eina_module_symbol_get(m, "map_module_tile_url_get");
   geo_to_coord = eina_module_symbol_get(m, "map_module_tile_geo_to_coord");
   coord_to_geo = eina_module_symbol_get(m, "map_module_tile_coord_to_geo");

   if ((!name_cb) || (!zoom_min) || (!zoom_max) || (!url_cb) ||
       (!geo_to_coord) || (!coord_to_geo))
     {
        WRN("Could not find map module functions from module \"%s\": %s",
            file, eina_error_msg_get(eina_error_get()));
        eina_module_unload(m);
        return EINA_FALSE;
     }
   s = ELM_NEW(Source_Tile);
   s->name = name_cb();
   s->zoom_min = zoom_min();
   s->zoom_max = zoom_max();
   s->url_cb = url_cb;
   s->geo_to_coord = geo_to_coord;
   s->coord_to_geo = coord_to_geo;
   wd->src_tiles = eina_list_append(wd->src_tiles, s);

   return EINA_TRUE;
}

static void
_source_tile_load(Widget_Data *wd)
{
   unsigned int idx;
   Eina_List *l;
   Source_Tile *s;

   // Load from hard coded data
   for (idx = 0; idx < (sizeof(src_tiles) / sizeof(Source_Tile)); idx++)
     {
        s= ELM_NEW(Source_Tile);
        s->name = src_tiles[idx].name;
        s->zoom_min = src_tiles[idx].zoom_min;
        s->zoom_max = src_tiles[idx].zoom_max;
        s->url_cb = src_tiles[idx].url_cb;
        s->geo_to_coord = src_tiles[idx].geo_to_coord;
        s->coord_to_geo = src_tiles[idx].coord_to_geo;
        wd->src_tiles = eina_list_append(wd->src_tiles, s);
     }

   // Load from modules
   wd->src_tile_mods = eina_module_list_get(wd->src_tile_mods, MODULES_PATH, 1,
                                            &_source_tile_mod_cb, wd);

   // Set default source
   wd->src_tile = eina_list_nth(wd->src_tiles, 0);

   // Make name strings
   idx = 0;
   wd->src_tile_names  = calloc((eina_list_count(wd->src_tiles) + 1),
                                sizeof(char *));
   EINA_LIST_FOREACH(wd->src_tiles, l, s)
     {
        eina_stringshare_replace(&wd->src_tile_names[idx], s->name);
        INF("source : %s", wd->src_tile_names[idx]);
        idx++;
     }
}

static void
_source_tile_unload(Widget_Data *wd)
{
   int idx = 0;
   Source_Tile *s;

   for (idx = 0; wd->src_tile_names[idx]; idx++)
      eina_stringshare_del(wd->src_tile_names[idx]);
   EINA_LIST_FREE(wd->src_tiles, s) free(s);
   eina_module_list_free(wd->src_tile_mods);
}

static void
_source_tile_set(Widget_Data *wd, const char *source_name)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(source_name);
   Source_Tile *s;
   Eina_List *l;

   if (wd->src_tile && !strcmp(wd->src_tile->name, source_name)) return;

   EINA_LIST_FOREACH(wd->src_tiles, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             wd->src_tile = s;
             break;
          }
     }
   if (!wd->src_tile)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }

   if (wd->src_tile->zoom_max < wd->zoom)
      wd->zoom = wd->src_tile->zoom_max;
   else if (wd->src_tile->zoom_min > wd->zoom)
      wd->zoom = wd->src_tile->zoom_min;

   if (wd->src_tile->zoom_max < wd->zoom_max)
      wd->zoom_max = wd->src_tile->zoom_max;
   if (wd->src_tile->zoom_min > wd->zoom_min)
      wd->zoom_min = wd->src_tile->zoom_min;

   _grid_all_clear(wd);
   _grid_all_create(wd);
   zoom_do(wd, wd->zoom);
}

static Eina_Bool
_source_route_mod_cb(Eina_Module *m, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   Widget_Data *wd = data;
   Source_Route *s;
   Elm_Map_Module_Source_Name_Func name_cb;
   Elm_Map_Module_Route_Url_Func url_cb;
   const char *file;

   file = eina_module_file_get(m);
   if (!eina_module_load(m))
     {
        ERR("Could not load module \"%s\": %s", file,
            eina_error_msg_get(eina_error_get()));
        return EINA_FALSE;
     }

   name_cb = eina_module_symbol_get(m, "map_module_source_name_get");
   url_cb = eina_module_symbol_get(m, "map_module_route_url_get");

   if ((!name_cb) || (!url_cb))
     {
        WRN("Could not find map module functions from module \"%s\": %s",
            file, eina_error_msg_get(eina_error_get()));
        eina_module_unload(m);
        return EINA_FALSE;
     }
   s = ELM_NEW(Source_Tile);
   s->name = name_cb();
   s->url_cb = url_cb;
   wd->src_routes = eina_list_append(wd->src_routes, s);

   eina_module_unload(m);
   return EINA_TRUE;
}

static void
_source_route_load(Widget_Data *wd)
{
   unsigned int idx;
   Eina_List *l;
   Source_Route *s;

   // Load from hard coded data
   for (idx = 0; idx < (sizeof(src_routes) / sizeof(Source_Route)); idx++)
     {
        s= ELM_NEW(Source_Route);
        s->name = src_routes[idx].name;
        s->url_cb = src_routes[idx].url_cb;
        wd->src_routes = eina_list_append(wd->src_routes, s);
     }

   // Load from modules
   wd->src_route_mods = eina_module_list_get(wd->src_route_mods, MODULES_PATH,
                                            1, &_source_route_mod_cb, wd);

   // Set default source
   wd->src_route = eina_list_nth(wd->src_routes, 0);

   // Make name strings
   idx = 0;
   wd->src_route_names  = calloc((eina_list_count(wd->src_routes) + 1),
                                   sizeof(char *));
   EINA_LIST_FOREACH(wd->src_routes, l, s)
     {
        eina_stringshare_replace(&wd->src_route_names[idx], s->name);
        INF("source : %s", wd->src_route_names[idx]);
        idx++;
     }
}

static void
_source_route_unload(Widget_Data *wd)
{
   int idx = 0;
   Source_Route *s;

   for (idx = 0; wd->src_route_names[idx]; idx++)
      eina_stringshare_del(wd->src_route_names[idx]);
   EINA_LIST_FREE(wd->src_routes, s) free(s);
   eina_module_list_free(wd->src_route_mods);
}

static void
_source_route_set(Widget_Data *wd, const char *source_name)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(source_name);
   Source_Route *s;
   Eina_List *l;

   if (wd->src_route && !strcmp(wd->src_route->name, source_name)) return;

   EINA_LIST_FOREACH(wd->src_routes, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             wd->src_route = s;
             break;
          }
     }
   if (!wd->src_route)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }
}

static Eina_Bool
_source_name_mod_cb(Eina_Module *m, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   Widget_Data *wd = data;
   Source_Name *s;
   Elm_Map_Module_Source_Name_Func name_cb;
   Elm_Map_Module_Name_Url_Func url_cb;
   const char *file;

   file = eina_module_file_get(m);
   if (!eina_module_load(m))
     {
        ERR("Could not load module \"%s\": %s", file,
            eina_error_msg_get(eina_error_get()));
        return EINA_FALSE;
     }

   name_cb = eina_module_symbol_get(m, "map_module_source_name_get");
   url_cb = eina_module_symbol_get(m, "map_module_name_url_get");

   if ((!name_cb) || (!url_cb))
     {
        WRN("Could not find map module functions from module \"%s\": %s",
            file, eina_error_msg_get(eina_error_get()));
        eina_module_unload(m);
        return EINA_FALSE;
     }
   s = ELM_NEW(Source_Tile);
   s->name = name_cb();
   s->url_cb = url_cb;
   wd->src_names = eina_list_append(wd->src_names, s);

   eina_module_unload(m);
   return EINA_TRUE;
}

static void
_source_name_load(Widget_Data *wd)
{
   unsigned int idx;
   Eina_List *l;
   Source_Name *s;

   // Load from hard coded data
   for (idx = 0; idx < (sizeof(src_names) / sizeof(Source_Name)); idx++)
     {
        s= ELM_NEW(Source_Name);
        s->name = src_names[idx].name;
        s->url_cb = src_names[idx].url_cb;
        wd->src_names = eina_list_append(wd->src_names, s);
     }

   // Load from modules
   wd->src_name_mods = eina_module_list_get(wd->src_name_mods, MODULES_PATH, 1,
                                            &_source_name_mod_cb, wd);

   // Set default source
   wd->src_name = eina_list_nth(wd->src_names, 0);

   // Make name strings
   idx = 0;
   wd->src_name_names  = calloc((eina_list_count(wd->src_names) + 1),
                                   sizeof(char *));
   EINA_LIST_FOREACH(wd->src_names, l, s)
     {
        eina_stringshare_replace(&wd->src_name_names[idx], s->name);
        INF("source : %s", wd->src_name_names[idx]);
        idx++;
     }
}

static void
_source_name_unload(Widget_Data *wd)
{
   int idx = 0;
   Source_Name *s;

   for (idx = 0; wd->src_name_names[idx]; idx++)
      eina_stringshare_del(wd->src_name_names[idx]);
   EINA_LIST_FREE(wd->src_names, s) free(s);
   eina_module_list_free(wd->src_name_mods);
}

static void
_source_name_set(Widget_Data *wd, const char *source_name)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(source_name);

   Source_Name *s;
   Eina_List *l;

   if (wd->src_name && !strcmp(wd->src_name->name, source_name)) return;

   EINA_LIST_FOREACH(wd->src_names, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             wd->src_name = s;
             break;
          }
     }
   if (!wd->src_name)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }
}

static void
_source_all_load(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   _source_tile_load(wd);
   _source_route_load(wd);
   _source_name_load(wd);
}

static void
_source_all_unload(Widget_Data *wd)
{
   EINA_SAFETY_ON_NULL_RETURN(wd);
   _source_tile_unload(wd);
   _source_route_unload(wd);
   _source_name_unload(wd);
}

static void
_zoom_mode_set(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;

   dd->wd->mode = dd->mode;
   if (dd->mode != ELM_MAP_ZOOM_MODE_MANUAL)
     {
        Evas_Coord w, h;
        Evas_Coord vw, vh;

        double zoom;
        double diff;

        w = dd->wd->size.w;
        h = dd->wd->size.h;
        zoom = dd->wd->zoom_detail;
        _viewport_size_get(dd->wd, &vw, &vh);

        if (dd->mode == ELM_MAP_ZOOM_MODE_AUTO_FIT)
          {
             if ((w < vw) && (h < vh))
               {
                  diff = 0.01;
                  while ((w < vw) && (h < vh))
                  {
                     zoom += diff;
                     w = pow(2.0, zoom) * dd->wd->tsize;
                     h = pow(2.0, zoom) * dd->wd->tsize;
                  }
               }
             else
               {
                  diff = -0.01;
                  while ((w > vw) || (h > vh))
                  {
                     zoom += diff;
                     w = pow(2.0, zoom) * dd->wd->tsize;
                     h = pow(2.0, zoom) * dd->wd->tsize;
                  }
               }

          }
        else if (dd->mode == ELM_MAP_ZOOM_MODE_AUTO_FILL)
          {
             if ((w < vw) || (h < vh))
               {
                  diff = 0.01;
                  while ((w < vw) || (h < vh))
                  {
                     zoom += diff;
                     w = pow(2.0, zoom) * dd->wd->tsize;
                     h = pow(2.0, zoom) * dd->wd->tsize;
                  }
               }
             else
               {
                  diff = -0.01;
                  while ((w > vw) && (h > vh))
                  {
                     zoom += diff;
                     w = pow(2.0, zoom) * dd->wd->tsize;
                     h = pow(2.0, zoom) * dd->wd->tsize;
                  }
               }
          }
       zoom_with_animation(dd->wd, zoom, 10);
     }
}

static void
_zoom_set(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;

   if (dd->wd->paused) zoom_do(dd->wd, dd->zoom);
   else zoom_with_animation(dd->wd, dd->zoom, 10);
   evas_object_smart_changed(dd->wd->pan_smart);
}

static void
_region_bring_in(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;
   int x, y, w, h;

   _region_to_coord_convert(dd->wd, dd->lon, dd->lat, dd->wd->size.w, &x, &y);
   _viewport_size_get(dd->wd, &w, &h);
   x = x - (w / 2);
   y = y - (h / 2);
   elm_smart_scroller_region_bring_in(dd->wd->scr, x, y, w, h);
   evas_object_smart_changed(dd->wd->pan_smart);
}

static void
_marker_list_show(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Delayed_Data *dd = data;
   int zoom;
   double max_lon = -180, min_lon = 180;
   double max_lat = -90, min_lat = 90;
   Evas_Coord vw, vh;
   Elm_Map_Marker *marker;

   EINA_LIST_FREE(dd->markers, marker)
     {
        if (marker->longitude > max_lon) max_lon = marker->longitude;
        if (marker->longitude < min_lon) min_lon = marker->longitude;
        if (marker->latitude > max_lat) max_lat = marker->latitude;
        if (marker->latitude < min_lat) min_lat = marker->latitude;
     }
   dd->lon = (max_lon + min_lon) / 2;
   dd->lat = (max_lat + min_lat) / 2;

   zoom = dd->wd->src_tile->zoom_min;
   _viewport_size_get(dd->wd, &vw, &vh);
   while (zoom <= dd->wd->src_tile->zoom_max)
     {
        Evas_Coord size, max_x, max_y, min_x, min_y;
        size = pow(2.0, zoom) * dd->wd->tsize;
        _region_to_coord_convert(dd->wd, min_lon, max_lat, size, &min_x, &max_y);
        _region_to_coord_convert(dd->wd, max_lon, min_lat, size, &max_x, &min_y);
        if ((max_x - min_x) > vw || (max_y - min_y) > vh) break;
        zoom++;
     }
   zoom--;

   zoom_do(dd->wd, zoom);
   _region_show(dd);
   evas_object_smart_changed(dd->wd->pan_smart);
}

static char *
_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   // ((x+y+zoom)%3)+'a' is requesting map images from distributed tile servers (eg., a, b, c)
   snprintf(buf, sizeof(buf), "http://%c.tile.openstreetmap.org/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://%c.tah.openstreetmap.org/Tiles/tile/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://%c.tile.opencyclemap.org/cycle/%d/%d/%d.png",
            (( x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_mapquest_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://otile%d.mqcdn.com/tiles/1.0.0/osm/%d/%d/%d.png",
            ((x + y + zoom) % 4) + 1, zoom, x, y);
   return strdup(buf);
}

static char *
_mapquest_aerial_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://oatile%d.mqcdn.com/naip/%d/%d/%d.png",
           ((x + y + zoom) % 4) + 1, zoom, x, y);
   return strdup(buf);
}

static char *_yours_url_cb(Evas_Object *obj __UNUSED__, const char *type_name, int method, double flon, double flat, double tlon, double tlat)
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
_nominatim_url_cb(Evas_Object *obj, int method, const char *name, double lon, double lat)
{
   ELM_CHECK_WIDTYPE(obj, widtype) strdup("");
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, strdup(""));

   char **str;
   unsigned int ele, idx;
   char search_url[PATH_MAX];
   char buf[PATH_MAX];

   if (method == ELM_MAP_NAME_METHOD_SEARCH)
     {
        search_url[0] = '\0';
        str = eina_str_split_full(name, " ", 0, &ele);
        for (idx = 0; idx < ele; idx++)
          {
             eina_strlcat(search_url, str[idx], sizeof(search_url));
             if (!(idx == (ele-1)))
                eina_strlcat(search_url, "+", sizeof(search_url));
          }
        snprintf(buf, sizeof(buf),
                 "%s/search?q=%s&format=xml&polygon=0&addressdetails=0",
                 NAME_NOMINATIM_URL, search_url);

        if (str && str[0])
          {
             free(str[0]);
             free(str);
          }
     }
   else if (method == ELM_MAP_NAME_METHOD_REVERSE)
      snprintf(buf, sizeof(buf),
               "%s/reverse?format=xml&lat=%lf&lon=%lf&zoom=%d&addressdetails=0",
               NAME_NOMINATIM_URL, lat, lon, (int)wd->zoom);
   else strcpy(buf, "");

   return strdup(buf);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

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
_del_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if (wd->map) evas_map_free(wd->map);
   free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Eina_List *l, *ll;
   Elm_Map_Route *r;
   Elm_Map_Name *na;
   Evas_Object *track;
   Elm_Map_Marker *marker;
   Elm_Map_Group_Class *group_clas;
   Elm_Map_Marker_Class *clas;
   Elm_Map_Overlay *overlay;
   Delayed_Data *dd;

   EINA_LIST_FOREACH_SAFE(wd->routes, l, ll, r) elm_map_route_del(r);
   eina_list_free(wd->routes);

   EINA_LIST_FOREACH_SAFE(wd->names, l, ll, na) elm_map_name_del(na);
   eina_list_free(wd->names);

   EINA_LIST_FOREACH_SAFE(wd->overlays, l, ll, overlay)
      elm_map_overlay_del(overlay);
   eina_list_free(wd->overlays);

   EINA_LIST_FREE(wd->track, track) evas_object_del(track);

   EINA_LIST_FOREACH_SAFE(wd->markers, l, ll, marker)
      _elm_map_marker_remove(marker);
   eina_list_free(wd->markers);

   EINA_LIST_FREE(wd->group_classes, group_clas)
     {
        eina_list_free(group_clas->markers);
        if (group_clas->style) eina_stringshare_del(group_clas->style);
        free(group_clas);
     }
   EINA_LIST_FREE(wd->marker_classes, clas)
     {
        if (clas->style) eina_stringshare_del(clas->style);
        free(clas);
     }

   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   if (wd->long_timer) ecore_timer_del(wd->long_timer);

   if (wd->delayed_jobs) EINA_LIST_FREE(wd->delayed_jobs, dd) free(dd);

   if (wd->user_agent) eina_stringshare_del(wd->user_agent);
   if (wd->ua) eina_hash_free(wd->ua);

   if (wd->zoom_timer) ecore_timer_del(wd->zoom_timer);
   if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);

   _grid_all_clear(wd);
   // Removal of download list should be after grid clear.
   if (wd->download_idler) ecore_idler_del(wd->download_idler);
   eina_list_free(wd->download_list);

   _source_all_unload(wd);

   if (!ecore_file_recursive_rm(CACHE_ROOT))
      ERR("Deletion of %s failed", CACHE_ROOT);
}

static void
_theme_hook(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   elm_smart_scroller_object_theme_set(obj, wd->scr, "map", "base", elm_widget_style_get(obj));
   _sizing_eval(wd);
}

static Eina_Bool
_event_hook(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EINA_FALSE);

   Evas_Coord x, y;
   Evas_Coord vh;
   Evas_Coord step_x, step_y, page_x, page_y;

   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;

   elm_smart_scroller_child_pos_get(wd->scr, &x, &y);
   elm_smart_scroller_step_size_get(wd->scr, &step_x, &step_y);
   elm_smart_scroller_page_size_get(wd->scr, &page_x, &page_y);
   _viewport_size_get(wd, NULL, &vh);

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
          y -= -(page_y * vh) / 100;
        else
          y -= page_y;
     }
   else if ((!strcmp(ev->keyname, "Next")) || (!strcmp(ev->keyname, "KP_Next")))
     {
        if (page_y < 0)
          y += -(page_y * vh) / 100;
        else
          y += page_y;
     }
   else if (!strcmp(ev->keyname, "KP_Add"))
     {
        zoom_with_animation(wd, wd->zoom + 1, 10);
        return EINA_TRUE;
     }
   else if (!strcmp(ev->keyname, "KP_Subtract"))
     {
        zoom_with_animation(wd, wd->zoom - 1, 10);
        return EINA_TRUE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   elm_smart_scroller_child_pos_set(wd->scr, x, y);

   return EINA_TRUE;
}
#endif

EAPI Evas_Object *
elm_map_add(Evas_Object *parent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Evas *e;
   Widget_Data *wd;
   Evas_Object *obj;
   Evas_Coord minw, minh;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);
   ELM_SET_WIDTYPE(widtype, "map");
   elm_widget_type_set(obj, "map");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_event_hook_set(obj, _event_hook);
   evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, wd);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, wd);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, wd);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _mouse_wheel_cb,wd);
   wd->obj = obj;

   wd->scr = elm_smart_scroller_add(e);
   elm_widget_sub_object_add(obj, wd->scr);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "map", "base", "default");
   elm_widget_resize_object_set(obj, wd->scr);
   elm_smart_scroller_wheel_disabled_set(wd->scr, EINA_TRUE);
   elm_smart_scroller_bounce_allow_set(wd->scr,
                                       _elm_config->thumbscroll_bounce_enable,
                                       _elm_config->thumbscroll_bounce_enable);
   evas_object_event_callback_add(wd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _changed_size_hints, wd);
   evas_object_smart_callback_add(wd->scr, "scroll", _scr, wd);
   evas_object_smart_callback_add(wd->scr, "drag", _scr, wd);
   evas_object_smart_callback_add(wd->scr, "animate,start", _scr_anim_start, wd);
   evas_object_smart_callback_add(wd->scr, "animate,stop", _scr_anim_stop, wd);

   if (!smart)
     {
        evas_object_smart_clipped_smart_set(&parent_sc);
        sc = parent_sc;
        sc.name = "elm_map_pan";
        sc.version = EVAS_SMART_CLASS_VERSION;
        sc.add = _pan_add;
        sc.resize = _pan_resize;
        sc.move = _pan_move;
        sc.calculate = _pan_calculate;
        smart = evas_smart_class_new(&sc);
     }
   if (smart)
     {
        Pan *pan;
        wd->pan_smart = evas_object_smart_add(e, smart);
        pan = evas_object_smart_data_get(wd->pan_smart);
        pan->wd = wd;
     }
   elm_widget_sub_object_add(obj, wd->pan_smart);

   elm_smart_scroller_extern_pan_set(wd->scr, wd->pan_smart,
                                     _pan_set, _pan_get, _pan_max_get,
                                     _pan_min_get, _pan_child_size_get);
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   wd->ges = elm_gesture_layer_add(obj);
   if (!wd->ges) ERR("elm_gesture_layer_add() failed");
   elm_gesture_layer_attach(wd->ges, obj);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START,
                            _pinch_zoom_start_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE,
                            _pinch_zoom_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,
                            _pinch_rotate_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_END,
                            _pinch_rotate_end_cb, wd);
   elm_gesture_layer_cb_set(wd->ges, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_ABORT,
                            _pinch_rotate_end_cb, wd);

   wd->sep_maps_markers = evas_object_rectangle_add(evas_object_evas_get(obj));
   elm_widget_sub_object_add(obj, wd->sep_maps_markers);
   evas_object_smart_member_add(wd->sep_maps_markers, wd->pan_smart);

   wd->map = evas_map_new(EVAS_MAP_POINT);

   _source_all_load(wd);
   wd->zoom_min = wd->src_tile->zoom_min;
   wd->zoom_max = wd->src_tile->zoom_max;
   // FIXME: Tile Provider is better to provide tile size!
   wd->tsize = DEFAULT_TILE_SIZE;

   srand(time(NULL));

   wd->id = ((int)getpid() << 16) | idnum;
   idnum++;
   _grid_all_create(wd);

   zoom_do(wd, 0);

   wd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
   wd->markers_max_num = MARER_MAX_NUMBER;

   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   if (!ecore_file_download_protocol_available("http://"))
      ERR("Ecore must be built with curl support for the map widget!");

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
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src_tile);

   if (wd->mode != ELM_MAP_ZOOM_MODE_MANUAL) return;
   if (zoom < 0) zoom = 0;
   if (wd->zoom == zoom) return;
   if (zoom > wd->src_tile->zoom_max) zoom = wd->src_tile->zoom_max;
   if (zoom < wd->src_tile->zoom_min) zoom = wd->src_tile->zoom_min;

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->func = _zoom_set;
   data->wd = wd;
   data->zoom = zoom;
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
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

   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, 0);
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
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if ((mode == ELM_MAP_ZOOM_MODE_MANUAL) && (wd->mode == !!mode)) return;

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->mode = mode;
   data->func = _zoom_mode_set;
   data->wd = wd;
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, ELM_MAP_ZOOM_MODE_MANUAL);

   return wd->mode;
#else
   (void) obj;
   return ELM_MAP_ZOOM_MODE_MANUAL;
#endif
}

EAPI void
elm_map_zoom_max_set(Evas_Object *obj, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src_tile);

   if ((zoom > wd->src_tile->zoom_max) || (zoom < wd->src_tile->zoom_min))
      return;
   wd->zoom_max = zoom;
#else
   (void) obj;
   (void) zoom;
#endif
}

EAPI int
elm_map_zoom_max_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) 18;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd->src_tile, -1);

   return wd->zoom_max;
#else
   (void) obj;
   return 18;
#endif
}

EAPI void
elm_map_zoom_min_set(Evas_Object *obj, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(wd->src_tile);

   if ((zoom > wd->src_tile->zoom_max) || (zoom < wd->src_tile->zoom_min))
      return;
   wd->zoom_min = zoom;
#else
   (void) obj;
   (void) zoom;
#endif
}

EAPI int
elm_map_zoom_min_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd->src_tile, -1);

   return wd->zoom_min;
#else
   (void) obj;
   return 0;
#endif
}


EAPI void
elm_map_region_bring_in(Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->func = _region_bring_in;
   data->wd = wd;
   data->lon = lon;
   data->lat = lat;
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
#else
   (void) obj;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_region_show(Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->func = _region_show;
   data->wd = wd;
   data->lon = lon;
   data->lat = lat;
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
#else
   (void) obj;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_region_get(const Evas_Object *obj, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   double tlon, tlat;
   Evas_Coord px, py, vw, vh;

   _pan_geometry_get(wd, &px, &py);
   _viewport_size_get(wd, &vw, &vh);
   _coord_to_region_convert(wd, vw/2 - px, vh/2 -py, wd->size.w, &tlon, &tlat);
   if (lon) *lon = tlon;
   if (lat) *lat = tlat;
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
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if (wd->paused == !!paused) return;
   wd->paused = !!paused;
   if (wd->paused)
     {
        if (wd->zoom_animator)
          {
             if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
             wd->zoom_animator = NULL;
             zoom_do(wd, wd->zoom);
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

EAPI Eina_Bool
elm_map_paused_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EINA_FALSE);

   return wd->paused;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_rotate_set(Evas_Object *obj, double degree, Evas_Coord cx, Evas_Coord cy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   wd->rotate.d = degree;
   wd->rotate.cx = cx;
   wd->rotate.cy = cy;

   evas_object_smart_changed(wd->pan_smart);
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
   EINA_SAFETY_ON_NULL_RETURN(wd);

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
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if ((!wd->wheel_disabled) && (disabled))
     evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, obj);
   else if ((wd->wheel_disabled) && (!disabled))
     evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, obj);
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EINA_FALSE);

   return wd->wheel_disabled;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_tile_load_status_get(const Evas_Object *obj, int *try_num, int *finish_num)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if (try_num) *try_num = wd->try_num;
   if (finish_num) *finish_num = wd->finish_num;
#else
   (void) obj;
   (void) try_num;
   (void) finish_num;
#endif
}

EAPI void
elm_map_canvas_to_region_convert(const Evas_Object *obj, Evas_Coord x, Evas_Coord y, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(lon);
   EINA_SAFETY_ON_NULL_RETURN(lat);

   Evas_Coord px, py, vw, vh;
   _pan_geometry_get(wd, &px, &py);
   _viewport_size_get(wd, &vw, &vh);
   _coord_rotate(x - px, y - py, (vw / 2) - px, (vh / 2) - py, -wd->rotate.d,
                 &x, &y);
   _coord_to_region_convert(wd, x, y, wd->size.w, lon, lat);
#else
   (void) obj;
   (void) x;
   (void) y;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_region_to_canvas_convert(const Evas_Object *obj, double lon, double lat, Evas_Coord *x, Evas_Coord *y)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(x);
   EINA_SAFETY_ON_NULL_RETURN(y);

   Evas_Coord px, py, vw, vh;
   _pan_geometry_get(wd, &px, &py);
   _viewport_size_get(wd, &vw, &vh);
   _region_to_coord_convert(wd, lon, lat, wd->size.w, x, y);
   _coord_rotate(*x, *y, (vw / 2) - px, (vh / 2) - py, wd->rotate.d,
                 x, y);
   *x += px;
   *y += py;
#else
   (void) obj;
   (void) lon;
   (void) lat;
   (void) x;
   (void) y;
#endif
}

EAPI void
elm_map_user_agent_set(Evas_Object *obj, const char *user_agent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(user_agent);

   eina_stringshare_replace(&wd->user_agent, user_agent);

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
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   return wd->user_agent;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_source_set(Evas_Object *obj, Elm_Map_Source_Type type, const char *source_name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(source_name);

   if (type == ELM_MAP_SOURCE_TYPE_TILE) _source_tile_set(wd, source_name);
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE)
      _source_route_set(wd, source_name);
   else if (type == ELM_MAP_SOURCE_TYPE_NAME) _source_name_set(wd, source_name);
   else ERR("Not supported map source type: %d", type);

#else
   (void) obj;
   (void) source_name;
#endif
}

EAPI const char *
elm_map_source_get(const Evas_Object *obj, Elm_Map_Source_Type type)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd->src_tile, NULL);

   if (type == ELM_MAP_SOURCE_TYPE_TILE) return wd->src_tile->name;
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE) return wd->src_route->name;
   else if (type == ELM_MAP_SOURCE_TYPE_NAME) return wd->src_name->name;
   else ERR("Not supported map source type: %d", type);
   return NULL;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI const char **
elm_map_sources_get(const Evas_Object *obj, Elm_Map_Source_Type type)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   if (type == ELM_MAP_SOURCE_TYPE_TILE) return wd->src_tile_names;
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE) return wd->src_route_names;
   else if (type == ELM_MAP_SOURCE_TYPE_NAME) return wd->src_tile_names;
   else ERR("Not supported map source type: %d", type);
   return NULL;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI Elm_Map_Route *
elm_map_route_add(Evas_Object *obj, Elm_Map_Route_Type type, Elm_Map_Route_Method method, double flon, double flat, double tlon, double tlat, Elm_Map_Route_Cb route_cb, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd->src_route, NULL);

   char *type_name;
   char *url;
   char fname[PATH_MAX];

   if (!ecore_file_exists(CACHE_ROUTE_ROOT))
      ecore_file_mkpath(CACHE_ROUTE_ROOT);

   if (type == ELM_MAP_ROUTE_TYPE_MOTOCAR)
      type_name = strdup(ROUTE_TYPE_MOTORCAR);
   else if (type == ELM_MAP_ROUTE_TYPE_BICYCLE)
      type_name = strdup(ROUTE_TYPE_BICYCLE);
   else if (type == ELM_MAP_ROUTE_TYPE_FOOT)
      type_name = strdup(ROUTE_TYPE_FOOT);
   else type_name = NULL;

   url = wd->src_route->url_cb(obj, type_name, method, flon, flat, tlon, tlat);
   if (!url)
     {
        ERR("Route URL is NULL");
        if (type_name) free(type_name);
        return NULL;
     }
   if (type_name) free(type_name);

   Elm_Map_Route *route = ELM_NEW(Elm_Map_Route);
   route->wd = wd;
   snprintf(fname, sizeof(fname), CACHE_ROUTE_ROOT"/%d", rand());
   route->fname = strdup(fname);
   route->type = type;
   route->method = method;
   route->flon = flon;
   route->flat = flat;
   route->tlon = tlon;
   route->tlat = tlat;
   route->cb = route_cb;
   route->data = data;

   if (!ecore_file_download_full(url, route->fname, _route_cb, NULL, route,
                                 &(route->job), wd->ua) || !(route->job))
     {
        ERR("Can't request Route from %s to %s", url, route->fname);
        free(route->fname);
        free(route);
        return NULL;
     }
   INF("Route requested from %s to %s", url, route->fname);
   free(url);

   wd->routes = eina_list_append(wd->routes, route);
   evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOAD, route);
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,start", "elm");
   return route;
#else
   (void) obj;
   (void) type;
   (void) method;
   (void) flon;
   (void) flat;
   (void) tlon;
   (void) tlat;
   (void) route_cb;
   (void) data;
   return NULL;
#endif
}


EAPI void
elm_map_route_del(Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(route);
   EINA_SAFETY_ON_NULL_RETURN(route->wd);
   ELM_CHECK_WIDTYPE(route->wd->obj, widtype);

   Path_Waypoint *w;
   Path_Node *n;

   if (route->job) ecore_file_download_abort(route->job);

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

   if (route->fname)
     {
        ecore_file_remove(route->fname);
        free(route->fname);
     }

   route->wd->routes = eina_list_remove(route->wd->routes, route);
   free(route);
#else
   (void) route;
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

EAPI Elm_Map_Name *
elm_map_name_add(const Evas_Object *obj, const char *address, double lon, double lat, Elm_Map_Name_Cb name_cb, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;

   if (address)
      return _name_request(obj, ELM_MAP_NAME_METHOD_SEARCH, address, 0, 0,
                           name_cb, data);
  else
     return _name_request(obj, ELM_MAP_NAME_METHOD_REVERSE, NULL, lon, lat,
                          name_cb, data);
#else
   (void) obj;
   (void) address;
   (void) lon;
   (void) lat;
   (void) name_cb;
   (void) data;
   return NULL;
#endif
}

EAPI void
elm_map_name_del(Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(name);
   EINA_SAFETY_ON_NULL_RETURN(name->wd);
   ELM_CHECK_WIDTYPE(name->wd->obj, widtype);

   if (name->job) ecore_file_download_abort(name->job);
   if (name->address) free(name->address);
   if (name->fname)
     {
        ecore_file_remove(name->fname);
        free(name->fname);
     }

   name->wd->names = eina_list_remove(name->wd->names, name);
   free(name);
#else
   (void) name;
#endif
}

EAPI const char *
elm_map_name_address_get(const Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name->wd, NULL);
   ELM_CHECK_WIDTYPE(name->wd->obj, widtype) NULL;

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
   EINA_SAFETY_ON_NULL_RETURN(name->wd);
   ELM_CHECK_WIDTYPE(name->wd->obj, widtype);

   if (lon) *lon = name->lon;
   if (lat) *lat = name->lat;
#else
   (void) name;
   (void) lon;
   (void) lat;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_add(Evas_Object *obj, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Elm_Map_Overlay *overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wd = wd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_DEFAULT;
   overlay->ovl = _overlay_default_new(wd, lon, lat, "radio");
   _overlay_default_cb_add(overlay->ovl, _overlay_clicked_cb, overlay);
   overlay->grp = _overlay_group_new(wd);
   wd->overlays = eina_list_append(wd->overlays, overlay);

   evas_object_smart_changed(wd->pan_smart);
   return overlay;
#else
   (void) obj;
   (void) lon;
   (void) lat;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_del(Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        _overlay_default_cb_del(overlay->ovl, _overlay_clicked_cb);
        _overlay_default_free(overlay->ovl);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        _overlay_bubble_cb_del(overlay->ovl);
        _overlay_bubble_free(overlay->ovl);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
      _overlay_class_free(overlay->ovl);

   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
      _overlay_route_free(overlay->ovl);

   if (overlay->grp)
     {
        _overlay_group_cb_del(overlay->grp);
        _overlay_group_free(overlay->grp, overlay);
     }

   overlay->wd->overlays = eina_list_remove(overlay->wd->overlays, overlay);
   evas_object_smart_changed(overlay->wd->pan_smart);

   free(overlay);
#else
   (void) overlay;
#endif
}

EAPI Elm_Map_Overlay_Type
elm_map_overlay_type_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, ELM_MAP_OVERLAY_TYPE_NONE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, ELM_MAP_OVERLAY_TYPE_NONE);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) ELM_MAP_OVERLAY_TYPE_NONE;

   return overlay->type;
#else
   (void) overlay;
   return ELM_MAP_OVERLAY_TYPE_NONE;
#endif
}

EAPI void
elm_map_overlay_data_set(Elm_Map_Overlay *overlay, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   overlay->data = data;
#else
   (void) overlay;
   (void) data;
#endif
}

EAPI void *
elm_map_overlay_data_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, NULL);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) NULL;

   return overlay->data;
#else
   (void) overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_hide_set(Elm_Map_Overlay *overlay, Eina_Bool hide)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->hide == !!hide) return;
   overlay->hide = hide;

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) hide;
#endif
}

EAPI Eina_Bool
elm_map_overlay_hide_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, EINA_FALSE);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) EINA_FALSE;

   return overlay->hide;
#else
   (void) overlay;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_overlay_displayed_zoom_min_set(Elm_Map_Overlay *overlay, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   overlay->zoom_min = zoom;
   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) zoom;
#endif
}

EAPI int
elm_map_overlay_displayed_zoom_min_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, 0);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) 0;

   return overlay->zoom_min;
#else
   (void) overlay;
   return 0;
#endif
}

EAPI void
elm_map_overlay_paused_set(Elm_Map_Overlay *overlay, Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->paused == !!paused) return;
   overlay->paused = paused;

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) paused;
#endif
}

EAPI Eina_Bool
elm_map_overlay_paused_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, EINA_FALSE);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) EINA_FALSE;

   return overlay->paused;
#else
   (void) overlay;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_overlay_show(Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;
        elm_map_region_show(overlay->wd->obj, ovl->lon, ovl->lat);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        Overlay_Bubble *ovl = overlay->ovl;
        elm_map_region_show(overlay->wd->obj, ovl->lon, ovl->lat);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        Overlay_Class *ovl = overlay->ovl;
        double lon, lat, max_lo, min_lo, max_la, min_la;
        _region_max_min_get(ovl->members, &max_lo, &min_lo, &max_la, &min_la);
        lon = (max_lo + min_lo) / 2;
        lat = (max_la + min_la) / 2;
        elm_map_region_show(overlay->wd->obj, lon, lat);
     }
   else ERR("Not supported overlay type: %d", overlay->type);

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
#endif
}

EAPI void
elm_map_overlays_show(Eina_List *overlays)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlays);
   EINA_SAFETY_ON_FALSE_RETURN(eina_list_count(overlays));

   Elm_Map_Overlay *overlay;
   overlay = eina_list_data_get(overlays);

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->func = _overlays_show;
   data->wd = overlay->wd;
   data->overlays = eina_list_clone(overlays);
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
#else
   (void) overlays;
#endif
}

EAPI void
elm_map_overlay_region_set(Elm_Map_Overlay *overlay, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;
        ovl->lon = lon;
        ovl->lat = lat;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        Overlay_Bubble *ovl = overlay->ovl;
        ovl->lon = lon;
        ovl->lat = lat;
     }
   else ERR("Not supported overlay type: %d", overlay->type);

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_overlay_region_get(const Elm_Map_Overlay *overlay, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;
        if (lon) *lon = ovl->lon;
        if (lat) *lat = ovl->lat;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        const Overlay_Bubble *ovl = overlay->ovl;
        if (lon) *lon = ovl->lon;
        if (lat) *lat = ovl->lat;
     }
   else ERR("Not supported overlay type: %d", overlay->type);
#else
   (void) overlay;
   (void) lon;
   (void) lat;
#endif
}

EAPI void
elm_map_overlay_icon_set(Elm_Map_Overlay *overlay, Evas_Object *icon)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(icon);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
      _overlay_default_update(overlay->wd, overlay->ovl, NULL, icon,
                              NULL, NULL, NULL);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
      _overlay_class_update(overlay->wd, overlay->ovl, NULL, icon);
   else ERR("Not supported overlay type: %d", overlay->type);

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) icon;
#endif
}

EAPI const Evas_Object *
elm_map_overlay_icon_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, NULL);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) NULL;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;
        return elm_object_part_content_get(ovl->layout, "elm.icon");
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        const Overlay_Class *ovl = overlay->ovl;
        return ovl->icon;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
#else
   (void) overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_content_set(Elm_Map_Overlay *overlay, Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(obj);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
      _overlay_default_update(overlay->wd, overlay->ovl, obj, NULL, NULL,
                                 NULL, NULL);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
      _overlay_class_update(overlay->wd, overlay->ovl, obj, NULL);
   else ERR("Not supported overlay type: %d", overlay->type);

   evas_object_smart_changed(overlay->wd->pan_smart);
#else
   (void) overlay;
   (void) obj;
#endif
}

EAPI const Evas_Object *
elm_map_overlay_content_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wd, NULL);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype) NULL;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;
        return elm_object_part_content_get(ovl->layout, "elm.icon");
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        const Overlay_Class *ovl = overlay->ovl;
        return ovl->icon;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
#else
   (void) overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_color_set(Elm_Map_Overlay *overlay, int r, int g , int b, int a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     {
        Overlay_Route *route = overlay->ovl;
        route->color.r = r;
        route->color.g = g;
        route->color.b = b;
        route->color.a = a;
     }
   else ERR("Not supported overlay type: %d", overlay->type);
#else
   (void) overlay;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
#endif
}

EAPI void
elm_map_overlay_color_get(const Elm_Map_Overlay *overlay, int *r, int *g , int *b, int *a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   int rr, gg, bb, aa;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     {
        Overlay_Route *route = overlay->ovl;
        rr = route->color.r;
        gg = route->color.g;
        bb = route->color.b;
        aa = route->color.a;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return;
     }
   if (r) *r = rr;
   if (g) *g = gg;
   if (b) *b = bb;
   if (a) *a = aa;
#else
   (void) overlay;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
#endif
}

EAPI void
elm_map_overlay_get_cb_set(Elm_Map_Overlay *overlay, Elm_Map_Overlay_Get_Cb get_cb, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wd);
   ELM_CHECK_WIDTYPE(overlay->wd->obj, widtype);

   overlay->cb = get_cb;
   overlay->cb_data = data;
#else
   (void) overlay;
   (void) get_cb;
   (void) data;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_class_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Elm_Map_Overlay *overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wd = wd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_CLASS;
   overlay->ovl = _overlay_class_new(wd, overlay);
   overlay->grp = NULL;
   wd->overlays = eina_list_append(wd->overlays, overlay);

   evas_object_smart_changed(wd->pan_smart);
   return overlay;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_class_append(Elm_Map_Overlay *group, Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(group);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(group->wd);
   ELM_CHECK_WIDTYPE(group->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(group->type == ELM_MAP_OVERLAY_TYPE_CLASS);

   if (overlay->type != ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        ERR("Currently group supports only default overlays");
        return;
     }

   Overlay_Class *ovl = group->ovl;
   if (eina_list_data_find(ovl->members, overlay))
     {
        ERR("Already added overlay into group");
        return;
     }
   ovl->members = eina_list_append(ovl->members, overlay);
   overlay->grp->clas = group;

  evas_object_smart_changed(group->wd->pan_smart);
#else
   (void) group;
   (void) overlay;
#endif
}

EAPI void
elm_map_overlay_class_remove(Elm_Map_Overlay *group, Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(group);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(group->wd);
   ELM_CHECK_WIDTYPE(group->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(group->type == ELM_MAP_OVERLAY_TYPE_CLASS);

    if (overlay->type != ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        ERR("Currently group supports only default overlays");
        return;
     }
   Overlay_Class *ovl = group->ovl;
   ovl->members = eina_list_remove(ovl->members, overlay);
   overlay->grp->clas = NULL;
   _overlay_group_update(group->wd, overlay->grp, NULL);

  evas_object_smart_changed(group->wd->pan_smart);
#else
   (void) group;
   (void) overlay;
#endif
}

EAPI void
elm_map_overlay_class_zoom_max_set(Elm_Map_Overlay *group, int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(group);
   EINA_SAFETY_ON_NULL_RETURN(group->wd);
   ELM_CHECK_WIDTYPE(group->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(group->type == ELM_MAP_OVERLAY_TYPE_CLASS);

   Overlay_Class *ovl = group->ovl;
   if (ovl->zoom_max == !!zoom) return;
   ovl->zoom_max = zoom;

   evas_object_smart_changed(group->wd->pan_smart);
#else
   (void) group;
   (void) zoom;
#endif
}

EAPI int
elm_map_overlay_class_zoom_max_get(const Elm_Map_Overlay *group)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, OVERLAY_CLASS_ZOOM_MAX);
   EINA_SAFETY_ON_NULL_RETURN_VAL(group->wd, OVERLAY_CLASS_ZOOM_MAX);
   ELM_CHECK_WIDTYPE(group->wd->obj, widtype) OVERLAY_CLASS_ZOOM_MAX;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(group->type == ELM_MAP_OVERLAY_TYPE_CLASS, OVERLAY_CLASS_ZOOM_MAX);

   const Overlay_Class *ovl = group->ovl;
   return ovl->zoom_max;
#else
   (void) group;
   return OVERLAY_CLASS_ZOOM_MAX;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_bubble_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Elm_Map_Overlay *overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wd = wd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_BUBBLE;
   overlay->ovl =  _overlay_bubble_new(wd);
   _overlay_bubble_cb_add(overlay->ovl, overlay);
   overlay->grp = _overlay_group_new(wd);
   wd->overlays = eina_list_append(wd->overlays, overlay);

   evas_object_smart_changed(wd->pan_smart);
   return overlay;
#else
   (void) obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_bubble_follow(Elm_Map_Overlay *bubble, Elm_Map_Overlay *parent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(parent);
   ELM_CHECK_WIDTYPE(bubble->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   Overlay_Bubble *ovl = bubble->ovl;
   Evas_Object *pobj = _overlay_obj_get(parent);
   if (!pobj) return;

   if (ovl->pobj)
     {
        evas_object_event_callback_del_full(ovl->pobj, EVAS_CALLBACK_HIDE,
                                            _overlay_bubble_hide_cb, ovl);
        evas_object_event_callback_del_full(ovl->pobj, EVAS_CALLBACK_SHOW,
                                            _overlay_bubble_chase_cb, ovl);
        evas_object_event_callback_del_full(ovl->pobj, EVAS_CALLBACK_MOVE,
                                            _overlay_bubble_chase_cb, ovl);
     }

   ovl->pobj = pobj;
   evas_object_event_callback_add(ovl->pobj, EVAS_CALLBACK_HIDE,
                                  _overlay_bubble_hide_cb, ovl);
   evas_object_event_callback_add(ovl->pobj, EVAS_CALLBACK_SHOW,
                                  _overlay_bubble_chase_cb, ovl);
   evas_object_event_callback_add(ovl->pobj, EVAS_CALLBACK_MOVE,
                                  _overlay_bubble_chase_cb, ovl);

   _overlay_bubble_chase(ovl);
   evas_object_smart_changed(bubble->wd->pan_smart);
#else
   (void) bubble;
   (void) parent;
#endif
}

EAPI void
elm_map_overlay_bubble_content_append(Elm_Map_Overlay *bubble, Evas_Object *content)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(content);
   ELM_CHECK_WIDTYPE(bubble->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   Overlay_Bubble *bb = bubble->ovl;
   elm_box_pack_end(bb->bx, content);

   evas_object_smart_changed(bubble->wd->pan_smart);
#else
   (void) bubble;
   (void) content;
#endif
}

EAPI void
elm_map_overlay_bubble_content_clear(Elm_Map_Overlay *bubble)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(bubble);
   ELM_CHECK_WIDTYPE(bubble->wd->obj, widtype);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   Overlay_Bubble *bb = bubble->ovl;
   elm_box_clear(bb->bx);

   evas_object_smart_changed(bubble->wd->pan_smart);
#else
   (void) bubble;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_route_add(Evas_Object *obj, const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(route->wd, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(obj == route->wd->obj, NULL);

   Elm_Map_Overlay *overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wd = wd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_ROUTE;
   overlay->ovl =  _overlay_route_new(wd, route);
   overlay->grp = _overlay_group_new(wd);
   wd->overlays = eina_list_append(wd->overlays, overlay);

   evas_object_smart_changed(wd->pan_smart);
   return overlay;
#else
   (void) obj;
   (void) route;
   return NULL;
#endif
}

#ifdef ELM_EMAP
EAPI Evas_Object *
elm_map_track_add(Evas_Object *obj, void *emap)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EMap_Route *emapr = emap;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EINA_FALSE);

   Evas_Object *route = elm_route_add(obj);
   elm_route_emap_set(route, emapr);
   wd->track = eina_list_append(wd->track, route);

   return route;
#else
   (void) obj;
   (void) emap;
   return NULL;
#endif
}

EAPI void
elm_map_track_remove(Evas_Object *obj, Evas_Object *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) ;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   wd->track = eina_list_remove(wd->track, route);
   evas_object_del(route);
#else
   (void) obj;
   (void) route;
#endif
}
#else
EAPI Evas_Object *
elm_map_track_add(Evas_Object *obj __UNUSED__, void *emap __UNUSED__)
{
   return NULL;
}

EAPI void
elm_map_track_remove(Evas_Object *obj __UNUSED__, Evas_Object *route __UNUSED__)
{
}
#endif

/************************* Belows are deprecated APIs *************************/
EAPI void
elm_map_source_zoom_max_set(Evas_Object *obj, int zoom)
{
   elm_map_zoom_max_set(obj, zoom);
}

EAPI int
elm_map_source_zoom_max_get(const Evas_Object *obj)
{
   return elm_map_zoom_max_get(obj);
}

EAPI void
elm_map_source_zoom_min_set(Evas_Object *obj, int zoom)
{
   elm_map_zoom_min_set(obj, zoom);
}

EAPI int
elm_map_source_zoom_min_get(const Evas_Object *obj)
{
   return elm_map_zoom_min_get(obj);
}

EAPI void
elm_map_utils_convert_coord_into_geo(const Evas_Object *obj, int x, int y, int size, double *lon, double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   _coord_to_region_convert(wd, x, y, size, lon, lat);
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
   EINA_SAFETY_ON_NULL_RETURN(wd);

   _region_to_coord_convert(wd, lon, lat, size, x, y);
#else
   (void) obj;
   (void) lon;
   (void) lat;
   (void) size;
   (void) x;
   (void) y;
#endif
}

EAPI void
elm_map_utils_rotate_coord(const Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord cx, const Evas_Coord cy, double degree, Evas_Coord *xx, Evas_Coord *yy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   _coord_rotate(x, y, cx, cy, degree, xx, yy);
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

EAPI void
elm_map_utils_downloading_status_get(const Evas_Object *obj, int *try_num, int *finish_num)
{
   elm_map_tile_load_status_get(obj, try_num, finish_num);
}

EAPI void
elm_map_geo_region_bring_in(Evas_Object *obj, double lon, double lat)
{
   elm_map_region_bring_in(obj, lon, lat);
}

EAPI void
elm_map_geo_region_show(Evas_Object *obj, double lon, double lat)
{
   elm_map_region_show(obj, lon, lat);
}

EAPI void
elm_map_geo_region_get(const Evas_Object *obj, double *lon, double *lat)
{
   elm_map_region_get(obj, lon, lat);
}

EAPI Elm_Map_Name *
elm_map_utils_convert_coord_into_name(const Evas_Object *obj, double lon, double lat)
{
   return elm_map_name_add(obj, NULL, lon, lat, NULL, NULL);
}

EAPI Elm_Map_Name *
elm_map_utils_convert_name_into_coord(const Evas_Object *obj, char *address)
{
   return elm_map_name_add(obj, address, 0, 0, NULL, NULL);
}

EAPI void
elm_map_canvas_to_geo_convert(const Evas_Object *obj, Evas_Coord x, Evas_Coord y, double *lon, double *lat)
{
   elm_map_canvas_to_region_convert(obj, x, y, lon, lat);
}

EAPI Elm_Map_Marker *
elm_map_marker_add(Evas_Object *obj, double lon, double lat, Elm_Map_Marker_Class *clas, Elm_Map_Group_Class *group_clas, void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(clas, NULL);

   Elm_Map_Marker *marker = ELM_NEW(Elm_Map_Marker);
   marker->wd = wd;
   marker->clas = clas;
   marker->group_clas = group_clas;
   marker->longitude = lon;
   marker->latitude = lat;
   marker->data = data;
   marker->x = 0;
   marker->y = 0;
   _edj_marker_size_get(wd, &marker->w, &marker->h);

   marker->obj = elm_layout_add(wd->obj);
   evas_object_smart_member_add(marker->obj, wd->pan_smart);
   evas_object_stack_above(marker->obj, wd->sep_maps_markers);

   edje_object_signal_callback_add(elm_layout_edje_get(marker->obj),
                                   "open", "elm", _marker_bubble_open_cb,
                                   marker);
   edje_object_signal_callback_add(elm_layout_edje_get(marker->obj),
                                   "bringin", "elm", _marker_bringin_cb,
                                   marker);

   wd->markers = eina_list_append(wd->markers, marker);
   if (marker->group_clas) group_clas->markers = eina_list_append(group_clas->markers,
                                                                  marker);
   evas_object_smart_changed(wd->pan_smart);
   return marker;
#else
   (void) obj;
   (void) lon;
   (void) lat;
   (void) clas;
   (void) group_clas;
   (void) data;
   return NULL;
#endif
}

EAPI void
elm_map_marker_remove(Elm_Map_Marker *marker)
{
   _elm_map_marker_remove(marker);
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
   elm_map_region_bring_in(marker->wd->obj, marker->longitude, marker->latitude);
#else
   (void) marker;
#endif
}

EAPI void
elm_map_marker_show(Elm_Map_Marker *marker)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_region_show(marker->wd->obj, marker->longitude, marker->latitude);
#else
   (void) marker;
#endif
}

EAPI void
elm_map_markers_list_show(Eina_List *markers)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(markers);
   EINA_SAFETY_ON_FALSE_RETURN(eina_list_count(markers));

   Elm_Map_Marker *marker;
   marker = eina_list_data_get(markers);

   Delayed_Data *data = ELM_NEW(Delayed_Data);
   data->func = _marker_list_show;
   data->wd = marker->wd;
   data->markers = eina_list_clone(markers);
   data->wd->delayed_jobs = eina_list_append(data->wd->delayed_jobs, data);
   evas_object_smart_changed(data->wd->pan_smart);
#else
   (void) markers;
#endif
}

EAPI void
elm_map_paused_markers_set(Evas_Object *obj, Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

   if (wd->paused_markers == !!paused) return;
   wd->paused_markers = paused;
#else
   (void) obj;
   (void) paused;
#endif
}

EAPI Eina_Bool
elm_map_paused_markers_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, EINA_FALSE);

   return wd->paused_markers;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_max_marker_per_group_set(Evas_Object *obj, int max)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   EINA_SAFETY_ON_NULL_RETURN(wd);

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
   Widget_Data *wd = marker->wd;
   EINA_SAFETY_ON_NULL_RETURN(wd);

   _marker_update(marker);
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
   EINA_SAFETY_ON_NULL_RETURN(wd);

   Eina_List *l;
   Elm_Map_Marker *marker;
   EINA_LIST_FOREACH(wd->markers, l, marker)
     {
        if (marker->bubble) _bubble_free(marker->bubble);
        marker->bubble = NULL;

        if (marker->group)
          {
             if (marker->group->bubble) _bubble_free(marker->group->bubble);
             marker->group->bubble = NULL;
          }
     }
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Elm_Map_Group_Class *clas = ELM_NEW(Elm_Map_Group_Class);
   clas->wd = wd;
   clas->zoom_displayed = 0;
   clas->zoom_grouped = 255;
   eina_stringshare_replace(&clas->style, "radio");

   wd->group_classes = eina_list_append(wd->group_classes, clas);

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
elm_map_group_class_icon_cb_set(Elm_Map_Group_Class *clas, Elm_Map_Group_Icon_Get_Func icon_get)
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
   EINA_SAFETY_ON_NULL_RETURN(wd);
   EINA_SAFETY_ON_NULL_RETURN(clas);

   clas->hide = hide;
   evas_object_smart_changed(wd->pan_smart);
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(wd, NULL);

   Elm_Map_Marker_Class *clas = ELM_NEW(Elm_Map_Marker_Class);
   eina_stringshare_replace(&clas->style, "radio");

   wd->marker_classes = eina_list_append(wd->marker_classes, clas);
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
elm_map_marker_class_icon_cb_set(Elm_Map_Marker_Class *clas, Elm_Map_Marker_Icon_Get_Func icon_get)
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
elm_map_marker_class_get_cb_set(Elm_Map_Marker_Class *clas, Elm_Map_Marker_Get_Func get)
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
elm_map_marker_class_del_cb_set(Elm_Map_Marker_Class *clas, Elm_Map_Marker_Del_Func del)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.del = del;
#else
   (void) clas;
   (void) del;
#endif
}

EAPI void
elm_map_route_color_set(Elm_Map_Route *route __UNUSED__, int r  __UNUSED__, int g  __UNUSED__, int b  __UNUSED__, int a  __UNUSED__)
{
   return;
}

EAPI void
elm_map_route_color_get(const Elm_Map_Route *route __UNUSED__, int *r  __UNUSED__, int *g  __UNUSED__, int *b  __UNUSED__, int *a  __UNUSED__)
{
   return;
}

EAPI void
elm_map_route_remove(Elm_Map_Route *route)
{
   elm_map_route_del(route);
}

EAPI void
elm_map_name_remove(Elm_Map_Name *name)
{
   elm_map_name_del(name);
}

