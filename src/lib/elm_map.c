#include <Elementary.h>
#include "Ecore_Con.h"
#include "Eina.h"
#include "elm_priv.h"

/**
 * @defgroup Map Map
 * @ingroup Elementary
 *
 * This is a widget specifically for displaying the map. It uses basically
 * OpenStreetMap provider. but it can be added custom providers.
 *
 * Signals that you can add callbacks for are:
 *
 * "clicked" - This is called when a user has clicked the map without dragging
 *             around.
 * "press" - This is called when a user has pressed down on the map.
 * "longpressed" - This is called when a user has pressed down on the map for
 *                 a long time without dragging around.
 * "clicked,double" - This is called when a user has double-clicked the photo.
 * "load,detail" - Map detailed data load begins.
 * "loaded,detail" - This is called when all parts of the map are loaded.
 * "zoom,start" - Zoom animation started.
 * "zoom,stop" - Zoom animation stopped.
 * "zoom,change" - Zoom changed when using an auto zoom mode.
 * "scroll" - the content has been scrolled (moved)
 * "scroll,anim,start" - scrolling animation has started
 * "scroll,anim,stop" - scrolling animation has stopped
 * "scroll,drag,start" - dragging the contents around has started
 * "scroll,drag,stop" - dragging the contents around has stopped
 * "downloaded" - This is called when map images are downloaded
 * "route,load" - This is called when route request begins
 * "route,loaded" - This is called when route request ends
 *
 * TODO : doxygen
 */


typedef struct _Widget_Data Widget_Data;
typedef struct _Pan Pan;
typedef struct _Grid Grid;
typedef struct _Grid_Item Grid_Item;
typedef struct _Marker_Group Marker_Group;
typedef struct _Mod_Api Mod_Api;
typedef struct _Event Event;
typedef struct _Route_Node Route_Node;
typedef struct _Route_Waypoint Route_Waypoint;
typedef struct _Url_Data Url_Data;
typedef struct _Route_Dump Route_Dump;

#define DEST_DIR_ZOOM_PATH "/tmp/elm_map/%d/%d/"
#define DEST_DIR_PATH DEST_DIR_ZOOM_PATH"%d/"
#define DEST_FILE_PATH "%s%d.png"
#define DEST_XML_FILE "/tmp/elm_map-XXXXXX.xml"

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

// Map sources
// Currently the size of a tile must be 256*256
// and the size of the map must be pow(2.0, z)*tile_size
typedef struct _Map_Sources_Tab
{
   Elm_Map_Sources source;
   const char *name;
   int zoom_min;
   int zoom_max;
   ElmMapSourceURLFunc url_cb;
   Elm_Map_Route_Sources route_source;
   ElmMapRouteSourceURLFunc route_url_cb;
} Map_Sources_Tab;

#define ZOOM_MAX 18

//Zemm min is supposed to be 0
static char *_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_maplint_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char *_module_url_cb(Evas_Object *obj, int x, int y, int zoom);
static char * _custom1_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char * _custom2_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char * _custom3_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char * _custom4_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char * _custom5_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);
static char * _custom6_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom);

static char *_yours_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
/*
static char *_monav_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
static char *_ors_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
 */
static char *_route_custom1_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_route_custom2_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_route_custom3_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_route_custom4_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_route_custom5_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
static char *_route_custom6_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
/*
static char *_route_module_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat);
 */

static Map_Sources_Tab map_sources_tab[] =
{
     {ELM_MAP_SOURCE_MAPNIK, "Mapnik", 0, 18, _mapnik_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb},
     {ELM_MAP_SOURCE_OSMARENDER, "Osmarender", 0, 17, _osmarender_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb},
     {ELM_MAP_SOURCE_CYCLEMAP, "Cycle Map", 0, 17, _cyclemap_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb},
     {ELM_MAP_SOURCE_MAPLINT, "Maplint", 12, 16, _maplint_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_1, "Custom 1", 0, 18, _custom1_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_1, _route_custom1_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_2, "Custom 2", 0, 18, _custom2_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_2, _route_custom2_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_3, "Custom 3", 0, 18, _custom3_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_3, _route_custom3_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_4, "Custom 4", 0, 18, _custom4_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_4, _route_custom4_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_5, "Custom 5", 0, 18, _custom5_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_5, _route_custom5_url_cb},
     {ELM_MAP_SOURCE_CUSTOM_6, "Custom 6", 0, 18, _custom6_url_cb, ELM_MAP_ROUTE_SOURCE_CUSTOM_6, _route_custom6_url_cb},
     {ELM_MAP_SOURCE_MODULE, "Module", 0, 18, _module_url_cb, ELM_MAP_ROUTE_SOURCE_YOURS, _yours_url_cb}
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
   Evas_Coord x[ZOOM_MAX+1], y[ZOOM_MAX+1];
   void *data;

   Marker_Group *groups[ZOOM_MAX+1];

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

   Route_Node *n;
   Route_Waypoint *w;
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

struct _Route_Node
{
   Widget_Data *wd;

   int idx;
   struct {
      double lon, lat;
      char *address;
   } pos;
};

struct _Route_Waypoint
{
   Widget_Data *wd;

   const char *point;
};

struct _Grid_Item
{
   Widget_Data *wd;
   Evas_Object *img;
   //Evas_Object *txt;
   const char *file;
   struct {
        int x, y, w, h;
   } src, out;
   Eina_Bool want : 1;
   Eina_Bool download : 1;
   Eina_Bool have : 1;
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
   Evas_Object *pan_smart;
   Evas_Object *rect;
   Evas_Object *sep_maps_markers; //map objects are below this object and marker objects are on top
   Pan *pan;
   Evas_Coord pan_x, pan_y, minw, minh;

   int id;
   int fid;
   int zoom;
   Elm_Map_Zoom_Mode mode;

   Ecore_Job *calc_job;
   Ecore_Timer *scr_timer;
   Ecore_Timer *long_timer;
   Ecore_Animator *zoom_animator;
   double t_start, t_end;
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
   int preload_num;
   Eina_List *grids;
   Eina_Bool resized : 1;
   Eina_Bool longpressed : 1;
   Eina_Bool on_hold : 1;
   Eina_Bool paused : 1;
   Eina_Bool paused_markers : 1;
   Eina_Bool pinch_zoom : 1;

   struct {
        Eina_Bool enabled;
        double lon, lat;
   } center_on;

   Ecore_Job *markers_place_job;
   Eina_Matrixsparse *markers[ZOOM_MAX+1];
   Eina_List *cells_displayed; // list of Eina_Matrixsparse_Cell
   Evas_Coord markers_max_num;
   int marker_max_w, marker_max_h;
   int marker_zoom;
   Eina_List *opened_bubbles; //opened bubbles, list of Map_Group *

   Eina_List *groups_clas; // list of Elm_Map_Group_Class*
   Eina_List *markers_clas; // list of Elm_Map_Markers_Class*

   Elm_Map_Sources source;
   Elm_Map_Route_Sources route_source;
   Mod_Api *api;
   Eina_List *s_event_list;
   int try_num;
   int finish_num;

   Eina_Hash *ua;
   const char *user_agent;
   Eina_List *route;
};

struct _Mod_Api
{
   Eina_Bool (*obj_hook) (Evas_Object *obj);
   Eina_Bool (*obj_unhook) (Evas_Object *obj);
   char * (*obj_url_request) (Evas_Object *obj, int x, int y, int zoom);
   Eina_Bool (*obj_convert_coord_into_geo) (const Evas_Object *obj, int zoom, int x, int y, int size, double *lon, double *lat);
   Eina_Bool (*obj_convert_geo_into_coord) (const Evas_Object *obj, int zoom, double lon, double lat, int size, int *x, int *y);
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
   } prev;

   Evas_Coord x, y, w, h;

   Evas_Object *object;
   Ecore_Timer *hold_timer;

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

static int dis_old = 0;
static const char *widtype = NULL;

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
       {NULL, NULL}
};

static void _pan_calculate(Evas_Object *obj);

static Eina_Bool _hold_timer_cb(void *data);
static void _rect_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _calc_job(void *data);
static Eina_Bool _event_hook(Evas_Object *obj, Evas_Object *src,
                             Evas_Callback_Type type, void *event_info);
static void grid_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);
static void grid_clear(Evas_Object *obj, Grid *g);
static Grid *grid_create(Evas_Object *obj);
static void grid_load(Evas_Object *obj, Grid *g);


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
static void _bubble_sc_hits_changed_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void _mouse_multi_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_multi_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _mouse_multi_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void route_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);

static int
get_multi_device(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Event *ev;

   EINA_LIST_FOREACH(wd->s_event_list, l, ev)
     {
        if (ev->device) return ev->device;
     }
   return 0;
}

static int
get_distance(Evas_Coord x1, Evas_Coord y1, Evas_Coord x2, Evas_Coord y2)
{
   int dx = x1 - x2;
   int dy = y1 - y2;
   return sqrt((dx * dx) + (dy * dy));
}

static Event *
create_event_object(void *data, Evas_Object *obj, int device)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Event *ev = calloc(1, sizeof(Event));

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, NULL);

   ev->object = obj;
   ev->device = device;
   evas_object_geometry_get(obj, &ev->x, &ev->y, &ev->w, &ev->h);
   wd->s_event_list = eina_list_append(wd->s_event_list, ev);
   return ev;
}

static Event*
get_event_object(void *data, int device)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Eina_List *l;
   Event *ev;

   EINA_LIST_FOREACH(wd->s_event_list, l, ev)
     {
        if (ev->device == device) break;
        ev = NULL;
     }
   return ev;
}

static void
destroy_event_object(void *data, Event *ev)
{
   Widget_Data *wd = elm_widget_data_get(data);
   EINA_SAFETY_ON_NULL_RETURN(ev);
   ev->pinch_dis = 0;
   wd->s_event_list = eina_list_remove(wd->s_event_list, ev);
   if (ev->hold_timer)
     {
        ecore_timer_del(ev->hold_timer);
        ev->hold_timer = NULL;
     }
   free(ev);
}

static Mod_Api *
module(Evas_Object *obj __UNUSED__)
{
   static Elm_Module *m = NULL;
   if (m) goto ok;
   if (!(m = _elm_module_find_as("map/api"))) return NULL;

   m->api = malloc(sizeof(Mod_Api));
   if (!m->api) return NULL;
   ((Mod_Api *)(m->api)      )->obj_hook =
      _elm_module_symbol_get(m, "obj_hook");
   ((Mod_Api *)(m->api)      )->obj_unhook =
      _elm_module_symbol_get(m, "obj_unhook");
   ((Mod_Api *)(m->api)      )->obj_url_request =
      _elm_module_symbol_get(m, "obj_url_request");
   ((Mod_Api *)(m->api)      )->obj_convert_coord_into_geo =
      _elm_module_symbol_get(m, "obj_convert_coord_into_geo");
   ((Mod_Api *)(m->api)      )->obj_convert_geo_into_coord =
      _elm_module_symbol_get(m, "obj_convert_geo_into_coord");
ok:
   return m->api;
}

static void
route_place(Evas_Object *obj, Grid *g __UNUSED__, Evas_Coord px, Evas_Coord py, Evas_Coord ox __UNUSED__, Evas_Coord oy __UNUSED__, Evas_Coord ow, Evas_Coord oh)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Eina_List *lr, *lp, *ln;
   Route_Node *n;
   Evas_Object *p;
   Elm_Map_Route *r;
   int nodes;
   int x, y, rx, ry;
   Evas_Coord size = pow(2.0, wd->zoom)*wd->tsize;

   EINA_LIST_FOREACH(wd->route, lr, r)
     {
        EINA_LIST_FOREACH(r->path, lp, p)
          {
             evas_object_hide(p);
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
                       evas_object_line_xy_set(p, r->x, r->y, x, y);
                       evas_object_color_set(p, r->color.r, r->color.g, r->color.b, r->color.a);
                       evas_object_raise(p);
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
   if (g != eina_list_data_get(wd->grids)) return;

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
   g_ww =  ow / wd->tsize + 1;
   if (g_xx + g_ww >= g->gw) g_ww = g->gw - g_xx - 1;
   g_hh =  oh / wd->tsize + 1;
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
                         }
                       if (group->update_raise)
                         {
                            group->update_raise = EINA_FALSE;
                            evas_object_raise(group->obj);
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

        /*evas_object_move(gi->txt,
                           xx - px + ax + ox,
                           yy - py + ay + oy);

          evas_object_resize(gi->txt, ww, hh);
         */
     }
   eina_iterator_free(it);
}

static void
grid_clear(Evas_Object *obj, Grid *g)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   char buf[PATH_MAX];

   if (!wd) return;
   if (!g->grid) return;

   Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
   Eina_Matrixsparse_Cell *cell;

   snprintf(buf, sizeof(buf), DEST_DIR_ZOOM_PATH, wd->id, g->zoom);
   ecore_file_recursive_rm(buf);

   EINA_ITERATOR_FOREACH(it, cell)
     {
        Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);
        evas_object_del(gi->img);
        //evas_object_del(gi->txt);

        if (gi->want)
          {
             gi->want = EINA_FALSE;
             wd->preload_num--;
             if (!wd->preload_num)
               {
                  edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                          "elm,state,busy,stop", "elm");
                  evas_object_smart_callback_call(obj, SIG_LOADED_DETAIL, NULL);
               }
          }

        if (gi->job)
          {
             DBG("DOWNLOAD abort %s", gi->file);
             ecore_file_download_abort(gi->job);
             ecore_file_remove(gi->file);
             gi->job = NULL;
             wd->try_num--;
          }
        if (gi->file)
          eina_stringshare_del(gi->file);

        free(gi);
     }
   eina_matrixsparse_free(g->grid);
   eina_iterator_free(it);
   g->grid = NULL;
   g->gw = 0;
   g->gh = 0;
}

static void
_tile_update(Grid_Item *gi)
{
   gi->want = EINA_FALSE;
   gi->download = EINA_FALSE;
   evas_object_image_file_set(gi->img, gi->file, NULL);
   if (evas_object_image_load_error_get(gi->img) != EVAS_LOAD_ERROR_NONE)
     ecore_file_remove(gi->file);

   evas_object_show(gi->img);

   //evas_object_text_text_set(gi->txt, gi->file);
   //evas_object_show(gi->txt);

   gi->have = EINA_TRUE;
   gi->wd->preload_num--;
   if (!gi->wd->preload_num)
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(gi->wd->scr),
                                "elm,state,busy,stop", "elm");
        evas_object_smart_callback_call(gi->wd->obj, SIG_LOADED_DETAIL, NULL);
     }
}


static void
_tile_downloaded(void *data, const char *file __UNUSED__, int status)
{
   Grid_Item *gi = data;

   gi->download = EINA_FALSE;
   gi->job = NULL;

   DBG("DOWNLOAD done %s", gi->file);
   if ((gi->want) && (!status)) _tile_update(gi);

   if (status)
     {
        DBG("Download failed %s (%d) ", gi->file, status);
        ecore_file_remove(gi->file);
     }
   else
     gi->wd->finish_num++;

   evas_object_smart_callback_call(gi->wd->obj, SIG_DOWNLOADED, NULL);
}

static Grid *
grid_create(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;

   g = calloc(1, sizeof(Grid));

   g->zoom = wd->zoom;
   g->tsize = wd->tsize;
   g->wd = wd;

   if (g->zoom > map_sources_tab[wd->source].zoom_max) return NULL;
   if (g->zoom < map_sources_tab[wd->source].zoom_min) return NULL;

   int size =  pow(2.0, wd->zoom);
   g->gw = size;
   g->gh = size;

   g->w = g->tsize * g->gw;
   g->h = g->tsize * g->gh;

   g->grid = eina_matrixsparse_new(g->gh, g->gw, NULL, NULL);

   return g;
}

static void
grid_load(Evas_Object *obj, Grid *g)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int x, y;
   int size;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh, tx, ty, gw, gh, xx, yy, ww, hh;
   Eina_Iterator *it;
   Eina_Matrixsparse_Cell *cell;
   Grid_Item *gi;

   if (!wd) return;
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
             if (gi->want)
               {
                  evas_object_hide(gi->img);
                  //evas_object_hide(gi->txt);
                  evas_object_image_file_set(gi->img, NULL, NULL);
                  gi->want = EINA_FALSE;
                  gi->have = EINA_FALSE;

                  if (gi->job)
                    {
                       DBG("DOWNLOAD abort %s", gi->file);
                       ecore_file_download_abort(gi->job);
                       ecore_file_remove(gi->file);
                       gi->job = NULL;
                       wd->try_num--;
                    }
                  gi->download = EINA_FALSE;
                  wd->preload_num--;
                  if (!wd->preload_num)
                    {
                       edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                               "elm,state,busy,stop", "elm");
                       evas_object_smart_callback_call(obj, SIG_LOADED_DETAIL,
                                                       NULL);
                    }

               }
             else if (gi->have)
               {
                  evas_object_hide(gi->img);
                  //evas_object_hide(gi->txt);
                  evas_object_image_preload(gi->img, 1);
                  evas_object_image_file_set(gi->img, NULL, NULL);
                  gi->have = EINA_FALSE;
                  gi->want = EINA_FALSE;
               }
          }
     }
   eina_iterator_free(it);

   xx = wd->pan_x / size;
   if (xx < 0) xx = 0;

   yy = wd->pan_y / size;
   if (yy < 0) yy = 0;

   ww = ow / size + 1;
   if (xx + ww >= g->gw) ww = g->gw - xx - 1;

   hh = oh / size + 1;
   if (yy + hh >= g->gh) hh = g->gh - yy - 1;

   for (y = yy; y <= yy + hh; y++)
     {
        for (x = xx; x <= xx + ww; x++)
          {
             gi = eina_matrixsparse_data_idx_get(g->grid, y, x);

             if ((!gi) && (g != eina_list_data_get(wd->grids)))
               continue;

             if (!gi)
               {
                  gi = calloc(1, sizeof(Grid_Item));
                  gi->src.x = x * g->tsize;
                  gi->src.y = y * g->tsize;
                  gi->src.w = g->tsize;
                  gi->src.h = g->tsize;

                  gi->out.x = gi->src.x;
                  gi->out.y = gi->src.y;
                  gi->out.w = gi->src.w;
                  gi->out.h = gi->src.h;

                  gi->wd = wd;

                  gi->img = evas_object_image_add(evas_object_evas_get(obj));
                  evas_object_image_scale_hint_set
                     (gi->img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
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
                  eina_matrixsparse_data_idx_set(g->grid, y, x, gi);
               }

             if ((!gi->have) && (!gi->download))
               {
                  char buf[PATH_MAX], buf2[PATH_MAX];
                  char *source;

                  gi->want = EINA_TRUE;

                  snprintf(buf, sizeof(buf), DEST_DIR_PATH, wd->id, g->zoom, x);
                  if (!ecore_file_exists(buf))
                    ecore_file_mkpath(buf);

                  snprintf(buf2, sizeof(buf2), DEST_FILE_PATH, buf, y);

                  source = map_sources_tab[wd->source].url_cb(obj, x, y, g->zoom);
                  if ((!source) || (strlen(source)==0)) continue;

                  eina_stringshare_replace(&gi->file, buf2);

                  if ((ecore_file_exists(buf2)) || (g == eina_list_data_get(wd->grids)))
                    {
                       gi->download = EINA_TRUE;
                       wd->preload_num++;
                       if (wd->preload_num == 1)
                         {
                            edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                                    "elm,state,busy,start", "elm");
                            evas_object_smart_callback_call(obj,
                                                            SIG_LOAD_DETAIL,
                                                            NULL);
                         }

                       if (ecore_file_exists(buf2))
                         _tile_update(gi);
                       else
                         {
                            DBG("DOWNLOAD %s \t in %s", source, buf2);
                            ecore_file_download_full(source, buf2, _tile_downloaded, NULL, gi, &(gi->job), wd->ua);
                            if (!gi->job)
                              DBG("Can't start to download %s", buf);
                            else
                              wd->try_num++;
                         }
                    }
                  if (source) free(source);
               }
             else if (gi->have)
               evas_object_show(gi->img);
          }
     }
}

static void
grid_clearall(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;

   if (!wd) return;
   EINA_LIST_FREE(wd->grids, g)
     {
        grid_clear(obj, g);
        free(g);
     }
}

static void
_smooth_update(Evas_Object *obj)
{
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

static void
_grid_raise(Grid *g)
{
   Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
   Eina_Matrixsparse_Cell *cell;

   g->wd->size.w = g->w;
   g->wd->size.h = g->h;

   EINA_ITERATOR_FOREACH(it, cell)
     {
        Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);
        evas_object_raise(gi->img);
        //evas_object_raise(gi->txt);
     }
   eina_iterator_free(it);
}

static Eina_Bool
_scr_timeout(void *data)
{
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

static Eina_Bool
zoom_do(Evas_Object *obj, double t)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord xx, yy, ow, oh;

   if (!wd) return 0;
   if (t > 1.0) t = 1.0;

   wd->size.w = (wd->size.ow * (1.0 - t)) + (wd->size.nw * t);
   wd->size.h = (wd->size.oh * (1.0 - t)) + (wd->size.nh * t);

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
   if (t >= 1.0)
     {
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_zoom_anim(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);
   double t;
   int go;

   if (!wd) return ECORE_CALLBACK_CANCEL;
   t = ecore_loop_time_get();
   if (t >= wd->t_end)
     t = 1.0;
   else if (wd->t_end > wd->t_start)
     t = (t - wd->t_start) / (wd->t_end - wd->t_start);
   else
     t = 1.0;
   t = 1.0 - t;
   t = 1.0 - (t * t);
   go = zoom_do(obj, t);
   if (!go)
     {
        wd->nosmooth--;
        if (!wd->nosmooth) _smooth_update(data);
        wd->zoom_animator = NULL;
        evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }
   return go;
}

static Eina_Bool
_long_press(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return ECORE_CALLBACK_CANCEL;
   wd->long_timer = NULL;
   wd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(data, SIG_LONGPRESSED, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Down *ev = event_info;
   Event *ev0;

   ev0 = get_event_object(data, 0);
   if (ev0) return;
   ev0 = create_event_object(data, obj, 0);
   if (!ev0) return;

   ev0->hold_timer = NULL;
   ev0->prev.x = ev->output.x;
   ev0->prev.y = ev->output.y;

   if (!wd) return;
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(data, SIG_CLICKED_DOUBLE, ev);
   else
     evas_object_smart_callback_call(data, SIG_PRESS, ev);
   wd->longpressed = EINA_FALSE;
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   wd->long_timer = ecore_timer_add(_elm_config->longpress_timeout, _long_press, data);
}

static void
_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Move *move = event_info;
   Event *ev0;

   if (wd->pinch_zoom) return;
   ev0 = get_event_object(data, 0);
   if (!ev0) return;
   ev0->prev.x = move->cur.output.x;
   ev0->prev.y = move->cur.output.y;
}

static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Up *ev = event_info;
   int mdevice;
   Event *ev0;
   Event *ev1;

   ev0 = get_event_object(data, 0);
   if (ev0)
     {
        mdevice = get_multi_device(data);
        if (mdevice == 0)
          {
             if (ev0->hold_timer)
               {
                  ecore_timer_del(ev0->hold_timer);
                  ev0->hold_timer = NULL;
               }
             elm_smart_scroller_hold_set(wd->scr, 0);
             elm_smart_scroller_freeze_set(wd->scr, 0);
             wd->pinch_zoom = EINA_FALSE;
          }
        else
          {
             ev1 = get_event_object(data, mdevice);
             if (ev1)
               ev1->hold_timer = ecore_timer_add(0.35, _hold_timer_cb, ev1);
          }
        destroy_event_object(data, ev0);
     }

   if (!wd) return;
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
_mouse_multi_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Event *ev;
   Evas_Event_Multi_Down *down = event_info;

   elm_smart_scroller_hold_set(wd->scr, 1);
   elm_smart_scroller_freeze_set(wd->scr, 1);

   ev = get_event_object(data, down->device);
   if (ev) goto done;

   ev = create_event_object(data, obj, down->device);
   if (!ev)
     {
        DBG("Failed : create_event_object");
        goto done;
     }

   wd->pinch_zoom = EINA_FALSE;

   ev->hold_timer = NULL;
   ev->prev.x = down->output.x;
   ev->prev.y = down->output.y;

done:
   return;
}

static void
_mouse_multi_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Multi_Move *move = event_info;
   int dis_new, zoom;
   Event *ev0;
   Event *ev;

   if (wd->pinch_zoom) return;
   ev = get_event_object(data, move->device);
   if (!ev) return;

   ev->prev.x = move->cur.output.x;
   ev->prev.y = move->cur.output.y;

   ev0 = get_event_object(data, 0);
   if (!ev0) return;

   dis_new = get_distance(ev0->prev.x, ev0->prev.y, ev->prev.x, ev->prev.y);
   zoom = wd->zoom;

   if (dis_old)
     {
        if (((dis_old - dis_new) > 0) &&
            (ev->pinch_dis > elm_finger_size_get()))
          {
             wd->pinch_zoom = EINA_TRUE;
             zoom--;
             elm_map_zoom_set(data, zoom);
             ev->pinch_dis = 0;
          }
        else if (((dis_old - dis_new) < 0) &&
                 (ev->pinch_dis < -elm_finger_size_get()))
          {
             wd->pinch_zoom = EINA_TRUE;
             zoom++;
             elm_map_zoom_set(data, zoom);
             ev->pinch_dis = 0;
          }
        ev->pinch_dis += (dis_old - dis_new);
     }
   dis_old = dis_new;
}

static void
_mouse_multi_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Multi_Up *up = event_info;
   Event *ev0;
   Event *ev;

   ev = get_event_object(data, up->device);
   if (!ev)
     {
        DBG("Cannot get multi device");
        return;
     }
   dis_old = 0;

   ev0 = get_event_object(data, 0);
   if (ev0)
     ev0->hold_timer = ecore_timer_add(0.35, _hold_timer_cb, ev0);
   else
     {
        if (ev->hold_timer)
          {
             ecore_timer_del(ev->hold_timer);
             ev->hold_timer = NULL;
          }
     }
   destroy_event_object(data, ev);
}

static Evas_Smart_Class _pan_sc = EVAS_SMART_CLASS_INIT_NULL;

static Eina_Bool
_hold_timer_cb(void *data)
{
   Event *ev0 = data;

   ev0->hold_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_rect_resize_cb(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
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
   Elm_Map_Group_Class *group_clas;
   Elm_Map_Marker_Class *marker_clas;
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Event *ev;
   Evas_Object *p;
   Route_Node *n;
   Route_Waypoint *w;
   Ecore_Event_Handler *h;
   Elm_Map_Route *r;

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

   EINA_LIST_FOREACH(wd->s_event_list, l, ev)
     {
        destroy_event_object(obj, ev);
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
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   if ((wd->api) && (wd->api->obj_unhook)) wd->api->obj_unhook(obj);
   if (wd->user_agent) eina_stringshare_del(wd->user_agent);
   if (wd->ua) eina_hash_free(wd->ua);

   free(wd);
}

static void
_del_pre_hook(Evas_Object *obj)
{
   Marker_Group *group;
   Elm_Map_Marker *marker;
   int i;
   Eina_Bool free_marker = EINA_TRUE;
   Eina_List *l;
   Widget_Data *wd = elm_widget_data_get(obj);
   grid_clearall(obj);

   if (!wd) return;
   for (i = 0; i < ZOOM_MAX + 1; i++)
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
                                                           _bubble_sc_hits_changed_cb, group);
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
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->obj, "elm,action,focus", "elm");
        evas_object_focus_set(wd->obj, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(wd->obj, "elm,action,unfocus", "elm");
        evas_object_focus_set(wd->obj, EINA_FALSE);
     }
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_object_theme_set(obj, wd->scr, "map", "base", elm_widget_style_get(obj));
   //   edje_object_scale_set(wd->scr, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
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

   minw = wd->size.w;
   minh = wd->size.h;
   if (wd->resized)
     {
        wd->resized = 0;
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
   sd->wd->resized = 1;
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
   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   rect_place(sd->wd->obj, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
   EINA_LIST_FOREACH(sd->wd->grids, l, g)
     {
        grid_load(sd->wd->obj, g);
        grid_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
        marker_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
        if (!sd->wd->zoom_animator) route_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
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
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 1);
}

static void
_hold_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_hold_set(wd->scr, 0);
}

static void
_freeze_on(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_freeze_set(wd->scr, 1);
}

static void
_freeze_off(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
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
   wd->center_on.enabled = EINA_FALSE;
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scr_drag_stop(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SCROLL_DRAG_STOP, NULL);
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

        if (icon) elm_layout_content_set(group->obj, "elm.icon", icon);

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

static void _bubble_sc_hits_changed_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
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

        elm_scroller_content_set(group->sc, group->bx);

        evas_object_event_callback_add(group->sc, EVAS_CALLBACK_RESIZE,
                                       _bubble_sc_hits_changed_cb, group);
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
                                       _bubble_sc_hits_changed_cb, group);
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
   evas_object_show(group->bubble);

   evas_object_move(group->rect, xx, yy);
   evas_object_resize(group->rect, ww, hh);
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
   double zoom;
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;

   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
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
        zoom = elm_map_zoom_get(obj);
        zoom += 1;
        elm_map_zoom_mode_set(obj, ELM_MAP_ZOOM_MODE_MANUAL);
        elm_map_zoom_set(obj, zoom);
        return EINA_TRUE;
     }
   else if (!strcmp(ev->keyname, "KP_Subtract"))
     {
        zoom = elm_map_zoom_get(obj);
        zoom -= 1;
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
cb_dump_attrs(void *data __UNUSED__, const char *key __UNUSED__, const char *value __UNUSED__)
{
   return EINA_TRUE;
}


static Eina_Bool
cb_dump(void *data, Eina_Simple_XML_Type type, const char *value, unsigned offset __UNUSED__, unsigned length)
{
   Route_Dump *dump = data;

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
           else
             {
                eina_simple_xml_attributes_parse
                  (attrs, length - (attrs - value), cb_dump_attrs, dump);
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

static void
_parse_kml(void *data)
{
   Elm_Map_Route *r = (Elm_Map_Route*)data;
   if (!r && !r->ud.fname) return;

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
                       eina_simple_xml_parse(buf, sz, EINA_TRUE, cb_dump, &dump);
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
                  Route_Waypoint *wp = ELM_NEW(Route_Waypoint);
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
                  Route_Node *n = ELM_NEW(Route_Node);
                  if (n)
                    {
                       n->wd = r->wd;
                       n->pos.lon = lon;
                       n->pos.lat = lat;
                       n->idx = idx;
                       DBG("%lf:%lf", lon, lat);
                       n->pos.address = NULL;
                       r->nodes = eina_list_append(r->nodes, n);

                       path = evas_object_line_add(evas_object_evas_get(r->wd->obj));
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

static Eina_Bool
_common_complete_cb(void *data, int ev_type __UNUSED__, void *event)
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
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(wd->obj, &ox, &oy, &ow, &oh);
        route_place(wd->obj, eina_list_data_get(wd->grids), wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
   edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                           "elm,state,busy,stop", "elm");
   evas_object_smart_callback_call(wd->obj, SIG_ROUTE_LOADED, NULL);
   return EINA_TRUE;
}

static int idnum = 1;

/**
 * Add a new Map object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Map
 */
EAPI Evas_Object *
elm_map_add(Evas_Object *parent)
{
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

   evas_object_smart_callback_add(wd->scr, "animate,start", _scr_anim_start, obj);
   evas_object_smart_callback_add(wd->scr, "animate,stop", _scr_anim_stop, obj);
   evas_object_smart_callback_add(wd->scr, "drag,start", _scr_drag_start, obj);
   evas_object_smart_callback_add(wd->scr, "drag,stop", _scr_drag_stop, obj);
   evas_object_smart_callback_add(wd->scr, "scroll", _scr_scroll, obj);

   elm_smart_scroller_bounce_allow_set(wd->scr, bounce, bounce);

   wd->api = module(obj);
   if ((wd->api) && (wd->api->obj_hook)) wd->api->obj_hook(obj);

   wd->obj = obj;

   wd->markers_max_num = 30;
   wd->source = ELM_MAP_SOURCE_MAPNIK;

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
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_MOVE,
                                  _mouse_move, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MULTI_DOWN,
                                  _mouse_multi_down, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MULTI_MOVE,
                                  _mouse_multi_move, obj);
   evas_object_event_callback_add(wd->rect, EVAS_CALLBACK_MULTI_UP,
                                  _mouse_multi_up, obj);

   evas_object_smart_member_add(wd->rect, wd->pan_smart);
   elm_widget_sub_object_add(obj, wd->rect);
   evas_object_show(wd->rect);
   evas_object_color_set(wd->rect, 0, 0, 0, 0);

   wd->zoom = -1;
   wd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
   wd->fid = 0;
   wd->id = ((int)getpid() << 16) | idnum;
   idnum++;

   wd->tsize = 256;

   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr),
                             &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   wd->paused = EINA_TRUE;
   elm_map_zoom_set(obj, 0);
   wd->paused = EINA_FALSE;

   _sizing_eval(obj);

   wd->calc_job = ecore_job_add(_calc_job, wd);

   wd->sep_maps_markers = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(wd->sep_maps_markers, wd->pan_smart);

   // TODO: convert Elementary to subclassing of Evas_Smart_Class
   // TODO: and save some bytes, making descriptions per-class and not instance!
   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   if (!ecore_file_download_protocol_available("http://"))
     {
        ERR("Ecore must be built with curl support for the map widget!");
     }
   return obj;
}

/**
 * Set the zoom level of the map
 *
 * This sets the zoom level. 0 is the world map and 18 is the maximum zoom.
 *
 * @param obj The map object
 * @param zoom The zoom level to set
 *
 * @ingroup Map
 */
EAPI void
elm_map_zoom_set(Evas_Object *obj, int zoom)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l, *lr;
   Grid *g, *g_zoom = NULL;
   Evas_Coord rx, ry, rw, rh;
   Evas_Object *p;
   Elm_Map_Route *r;
   int z;
   int zoom_changed = 0, started = 0;

   if (!wd) return;
   if (zoom < 0 ) zoom = 0;
   if (zoom > map_sources_tab[wd->source].zoom_max)
     zoom = map_sources_tab[wd->source].zoom_max;
   if (zoom < map_sources_tab[wd->source].zoom_min)
     zoom = map_sources_tab[wd->source].zoom_min;
   if (zoom == wd->zoom) return;

   wd->zoom = zoom;
   wd->size.ow = wd->size.w;
   wd->size.oh = wd->size.h;
   elm_smart_scroller_child_pos_get(wd->scr, &rx, &ry);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);

   EINA_LIST_FOREACH(wd->route, lr, r)
     {
        if (r)
          {
             EINA_LIST_FOREACH(r->path, l, p)
               {
                  evas_object_hide(p);
               }
          }
     }

   if (wd->mode == ELM_MAP_ZOOM_MODE_MANUAL)
     {
        wd->size.nw = pow(2.0, wd->zoom) * wd->tsize;
        wd->size.nh = pow(2.0, wd->zoom) * wd->tsize;
     }
   else if (wd->mode == ELM_MAP_ZOOM_MODE_AUTO_FIT)
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

        if (p2w < p2h)
          z = p2w;
        else
          z = p2h;

        wd->zoom = z;
        wd->size.nw = pow(2.0, wd->zoom) * wd->tsize;
        wd->size.nh = pow(2.0, wd->zoom) * wd->tsize;
     }
   else if (wd->mode == ELM_MAP_ZOOM_MODE_AUTO_FILL)
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

        if (p2w > p2h)
          z = p2w;
        else
          z = p2h;

        wd->zoom = z;
        wd->size.nw = pow(2.0, wd->zoom) * wd->tsize;
        wd->size.nh = pow(2.0, wd->zoom) * wd->tsize;
     }

   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        if (g->zoom == wd->zoom)
          {
             wd->grids = eina_list_remove(wd->grids, g);
             wd->grids = eina_list_prepend(wd->grids, g);
             _grid_raise(g);
             goto done;
          }
     }
   g = grid_create(obj);
   if (g)
     {
        if (eina_list_count(wd->grids) > 1)
          {
             g_zoom = eina_list_last(wd->grids)->data;
             wd->grids = eina_list_remove(wd->grids, g_zoom);
             grid_clear(obj, g_zoom);
             free(g_zoom);
          }
        wd->grids = eina_list_prepend(wd->grids, g);
     }
   else
     {
        EINA_LIST_FREE(wd->grids, g)
          {
             grid_clear(obj, g);
             free(g);
          }
     }
done:

   wd->t_start = ecore_loop_time_get();
   wd->t_end = wd->t_start + _elm_config->zoom_friction;
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
        zoom_do(obj, 1.0);
     }
   else
     {
        if (!wd->zoom_animator)
          {
             wd->zoom_animator = ecore_animator_add(_zoom_anim, obj);
             wd->nosmooth++;
             if (wd->nosmooth == 1) _smooth_update(obj);
             started = 1;
          }
     }
   if (wd->zoom_animator)
     {
        if (!_zoom_anim(obj))
          {
             ecore_animator_del(wd->zoom_animator);
             wd->zoom_animator = NULL;
          }
     }
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
   if (!wd->paused)
     {
        if (started)
          evas_object_smart_callback_call(obj, SIG_ZOOM_START, NULL);
        if (!wd->zoom_animator)
          evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }

   if (zoom_changed)
     evas_object_smart_callback_call(obj, SIG_ZOOM_CHANGE, NULL);
}

/**
 * Get the zoom level of the photo
 *
 * This returns the current zoom level of the map object. Note that if
 * you set the fill mode to other than ELM_MAP_ZOOM_MODE_MANUAL
 * (which is the default), the zoom level may be changed at any time by the
 * map object itself to account for map size and map viewpoer size
 *
 * @param obj The map object
 * @return The current zoom level
 *
 * @ingroup Map
 */
EAPI double
elm_map_zoom_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 1.0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 1.0;
   return wd->zoom;
}

/**
 * Set the zoom mode
 *
 * This sets the zoom mode to manual or one of several automatic levels.
 * Manual (ELM_MAP_ZOOM_MODE_MANUAL) means that zoom is set manually by
 * elm_map_zoom_set() and will stay at that level until changed by code
 * or until zoom mode is changed. This is the default mode.
 * The Automatic modes will allow the map object to automatically
 * adjust zoom mode based on properties. ELM_MAP_ZOOM_MODE_AUTO_FIT) will
 * adjust zoom so the photo fits inside the scroll frame with no pixels
 * outside this area. ELM_MAP_ZOOM_MODE_AUTO_FILL will be similar but
 * ensure no pixels within the frame are left unfilled. Do not forget that the valid sizes are 2^zoom, consequently the map may be smaller than the scroller view.
 *
 * @param obj The map object
 * @param mode The desired mode
 *
 * @ingroup Map
 */
EAPI void
elm_map_zoom_mode_set(Evas_Object *obj, Elm_Map_Zoom_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->mode == mode) return;
   wd->mode = mode;
     {
        double tz = wd->zoom;
        wd->zoom = 0.0;
        elm_map_zoom_set(wd->obj, tz);
     }
}

/**
 * Get the zoom mode
 *
 * This gets the current zoom mode of the map object
 *
 * @param obj The map object
 * @return The current zoom mode
 *
 * @ingroup Map
 */
EAPI Elm_Map_Zoom_Mode
elm_map_zoom_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_MAP_ZOOM_MODE_MANUAL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_MAP_ZOOM_MODE_MANUAL;
   return wd->mode;
}

/**
 * Centers the map at @p lon @p lat using an animation to scroll.
 *
 * @param obj The map object
 * @param lon Longitude to center at
 * @param lon Latitude to center at
 *
 * @ingroup Map
 */
EAPI void
elm_map_geo_region_bring_in(Evas_Object *obj, double lon, double lat)
{
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
        zoom_do(obj, 1.0);
        evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }
   elm_smart_scroller_region_bring_in(wd->scr, rx, ry, rw, rh);

   wd->center_on.enabled = EINA_TRUE;
   wd->center_on.lon = lon;
   wd->center_on.lat = lat;
}

/**
 * Move the map to the current coordinates.
 *
 * This move the map to the current coordinates. The map will be centered on these coordinates.
 *
 * @param obj The map object
 * @param lat The latitude.
 * @param lon The longitude.
 *
 * @ingroup Map
 */
EAPI void
elm_map_geo_region_show(Evas_Object *obj, double lon, double lat)
{
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
        zoom_do(obj, 1.0);
        evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
     }
   elm_smart_scroller_child_region_show(wd->scr, rx, ry, rw, rh);

   wd->center_on.enabled = EINA_TRUE;
   wd->center_on.lon = lon;
   wd->center_on.lat = lat;
}

/**
 * Get the current coordinates of the map.
 *
 * This gets the current coordinates of the map object.
 *
 * @param obj The map object
 * @param lat The latitude.
 * @param lon The longitude.
 *
 * @ingroup Map
 */
EAPI void
elm_map_geo_region_get(const Evas_Object *obj, double *lon, double *lat)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord sx, sy, sw, sh;

   if (!wd) return;
   elm_smart_scroller_child_pos_get(wd->scr, &sx, &sy);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &sw, &sh);
   sx += sw / 2;
   sy += sh / 2;

   elm_map_utils_convert_coord_into_geo(obj, sx, sy, wd->size.w, lon, lat);
}

/**
 * Set the paused state for map
 *
 * This sets the paused state to on (1) or off (0) for map. The default
 * is off. This will stop zooming using animation change zoom levels and
 * change instantly. This will stop any existing animations that are running.
 *
 * @param obj The map object
 * @param paused The pause state to set
 *
 * @ingroup Map
 */
EAPI void
elm_map_paused_set(Evas_Object *obj, Eina_Bool paused)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->paused == !!paused) return;
   wd->paused = paused;
   if (wd->paused)
     {
        if (wd->zoom_animator)
          {
             ecore_animator_del(wd->zoom_animator);
             wd->zoom_animator = NULL;
             zoom_do(obj, 1.0);
             evas_object_smart_callback_call(obj, SIG_ZOOM_STOP, NULL);
          }
     }
}

/**
 * Set the paused state for the markers
 *
 * This sets the paused state to on (1) or off (0) for the markers. The default
 * is off. This will stop displaying the markers during change zoom levels. Set
 * to on if you have a large number of markers.
 *
 * @param obj The map object
 * @param paused The pause state to set
 *
 * @ingroup Map
 */
EAPI void
elm_map_paused_markers_set(Evas_Object *obj, Eina_Bool paused)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->paused_markers == !!paused) return;
   wd->paused_markers = paused;
}

/**
 * Get the paused state for map
 *
 * This gets the current paused state for the map object.
 *
 * @param obj The map object
 * @return The current paused state
 *
 * @ingroup Map
 */
EAPI Eina_Bool
elm_map_paused_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->paused;
}

/**
 * Get the paused state for the markers
 *
 * This gets the current paused state for the markers object.
 *
 * @param obj The map object
 * @return The current paused state
 *
 * @ingroup Map
 */
EAPI Eina_Bool
elm_map_paused_markers_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->paused_markers;
}

/**
 * Get the information of downloading status
 *
 * This gets the current downloading status for the map object.
 *
 * @param obj The map object
 * @param try_num the number of download trying map
 * @param finish_num the number of downloaded map
 *
 * @ingroup Map
 */

EAPI void
elm_map_utils_downloading_status_get(const Evas_Object *obj, int *try_num, int *finish_num)
{
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
}
/**
 * Convert a pixel coordinate (x,y) into a geographic coordinate (longitude, latitude).
 *
 * @param obj The map object
 * @param x the coordinate
 * @param y the coordinate
 * @param size the size in pixels of the map. The map is a square and generally his size is : pow(2.0, zoom)*256.
 * @param lon the longitude correspond to x
 * @param lat the latitude correspond to y
 *
 * @ingroup Map
 */
EAPI void
elm_map_utils_convert_coord_into_geo(const Evas_Object *obj, int x, int y, int size, double *lon, double *lat)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int zoom = floor(log(size/256) / log(2));

   if (elm_map_source_get(obj) == ELM_MAP_SOURCE_MODULE)
     if ((wd->api) && (wd->api->obj_convert_coord_into_geo))
       if (wd->api->obj_convert_coord_into_geo(obj, zoom, x, y, size, lon, lat)) return;

   if (lon)
     {
        *lon = x / (double)size * 360.0 - 180;
     }
   if (lat)
     {
        double n = ELM_PI - 2.0 * ELM_PI * y / size;
        *lat = 180.0 / ELM_PI * atan(0.5 * (exp(n) - exp(-n)));
     }
}

/**
 * Convert a geographic coordinate (longitude, latitude) into a pixel coordinate (x, y).
 *
 * @param obj The map object
 * @param lon the longitude
 * @param lat the latitude
 * @param size the size in pixels of the map. The map is a square and generally his size is : pow(2.0, zoom)*256.
 * @param x the coordinate correspond to the longitude
 * @param y the coordinate correspond to the latitude
 *
 * @ingroup Map
 */
EAPI void
elm_map_utils_convert_geo_into_coord(const Evas_Object *obj, double lon, double lat, int size, int *x, int *y)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int zoom = floor(log(size/256) / log(2));

   if (elm_map_source_get(obj) == ELM_MAP_SOURCE_MODULE)
     if ((wd->api) && (wd->api->obj_convert_geo_into_coord))
       if (wd->api->obj_convert_geo_into_coord(obj, zoom, lon, lat, size, x, y)) return;

   if (x)
     *x = floor((lon + 180.0) / 360.0 * size);
   if (y)
     *y = floor((1.0 - log( tan(lat * ELM_PI/180.0) + 1.0 / cos(lat * ELM_PI/180.0)) / ELM_PI) / 2.0 * size);
}



/**
 * Add a marker on the map
 *
 * @param obj The map object
 * @param lon the longitude
 * @param lat the latitude
 * @param clas the class to use
 * @param data the data passed to the callbacks
 *
 * @return The marker object
 *
 * @ingroup Map
 */
EAPI Elm_Map_Marker *
elm_map_marker_add(Evas_Object *obj, double lon, double lat, Elm_Map_Marker_Class *clas, Elm_Map_Group_Class *clas_group, void *data)
{
   int i, j;
   Eina_List *l;
   Marker_Group *group;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
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

   for (i = clas_group->zoom_displayed; i <= ZOOM_MAX; i++)
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
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
        marker_place(obj, eina_list_data_get(wd->grids), wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }

   return marker;
}

/**
 * Remove a marker from the map
 *
 * @param marker The marker to remove
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_remove(Elm_Map_Marker *marker)
{
   int i;
   Eina_List *groups;
   Widget_Data *wd;

   EINA_SAFETY_ON_NULL_RETURN(marker);
   wd = marker->wd;
   if (!wd) return;
   for (i = marker->clas_group->zoom_displayed; i <= ZOOM_MAX; i++)
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

   free(marker);

   if (wd->grids)
     {
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(wd->obj, &ox, &oy, &ow, &oh);
        marker_place(wd->obj, eina_list_data_get(wd->grids), wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
}

/**
 * Get the current coordinates of the marker.
 *
 * @param marker marker.
 * @param lat The latitude.
 * @param lon The longitude.
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_region_get(const Elm_Map_Marker *marker, double *lon, double *lat)
{
   EINA_SAFETY_ON_NULL_RETURN(marker);
   if (lon) *lon = marker->longitude;
   if (lat) *lat = marker->latitude;
}

/**
 * Move the map to the coordinate of the marker.
 *
 * @param marker The marker where the map will be center.
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_bring_in(Elm_Map_Marker *marker)
{
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_geo_region_bring_in(marker->wd->obj, marker->longitude, marker->latitude);
}

/**
 * Move the map to the coordinate of the marker.
 *
 * @param marker The marker where the map will be center.
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_show(Elm_Map_Marker *marker)
{
   EINA_SAFETY_ON_NULL_RETURN(marker);
   elm_map_geo_region_show(marker->wd->obj, marker->longitude, marker->latitude);
}

/**
 * Move and zoom the map to display a list of markers.
 *
 * The map will be centered on the center point of the markers in the list. Then
 * the map will be zoomed in order to fit the markers using the maximum zoom which
 * allows display of all the markers.
 *
 * @param markers The list of markers (list of Elm_Map_Marker *)
 *
 * @ingroup Map
 */
EAPI void
elm_map_markers_list_show(Eina_List *markers)
{
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
   for (zoom = map_sources_tab[wd->source].zoom_max; zoom>map_sources_tab[wd->source].zoom_min; zoom--)
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
}

/**
 * Set the maximum numbers of markers display in a group.
 *
 * A group can have a long list of markers, consequently the creation of the content
 * of the bubble can be very slow. In order to avoid this, a maximum number of items
 * is displayed in a bubble. By default this number is 30.
 *
 * @param obj The map object.
 * @param max The maximum numbers of items displayed in a bubble.
 *
 * @ingroup Map
 */
EAPI void
elm_map_max_marker_per_group_set(Evas_Object *obj, int max)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->markers_max_num = max;
}

/**
 * Return the evas object getting from the ElmMapMarkerGetFunc callback
 *
 * @param marker The marker.
 * @return Return the evas object if it exists, else NULL.
 *
 * @ingroup Map
 */
EAPI Evas_Object *
elm_map_marker_object_get(const Elm_Map_Marker *marker)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(marker, NULL);
   return marker->content;
}

/**
 * Update the marker
 *
 * @param marker The marker.
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_update(Elm_Map_Marker *marker)
{
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
}

/**
 * Close all opened bubbles
 *
 * @param obj The map object
 *
 * @ingroup Map
 */
EAPI void
elm_map_bubbles_close(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Marker_Group *group;
   Eina_List *l, *l_next;
   if (!wd) return;
   EINA_LIST_FOREACH_SAFE(wd->opened_bubbles, l, l_next, group)
      _group_bubble_free(group);
}


/**
 * Create a group class.
 *
 * Each marker must be associated to a group class. Marker with the same group are grouped if they are close.
 * The group class defines the style of the marker when a marker is grouped to others markers.
 *
 * @param obj The map object
 * @return Returns the new group class
 *
 * @ingroup Map
 */
EAPI Elm_Map_Group_Class *
elm_map_group_class_new(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Elm_Map_Group_Class *clas = calloc(1, sizeof(Elm_Map_Group_Class));
   clas->zoom_grouped = ZOOM_MAX;
   wd->groups_clas = eina_list_append(wd->groups_clas, clas);
   return clas;
}

/**
 * Set the style of a group class (radio, radio2 or empty)
 *
 * @param clas the group class
 * @param style the new style
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_style_set(Elm_Map_Group_Class *clas, const char *style)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   eina_stringshare_replace(&clas->style, style);
}

/**
 * Set the icon callback of a group class.
 *
 * A custom icon can be displayed in a marker. The function @ref icon_get must return this icon.
 *
 * @param clas the group class
 * @param icon_get the callback to create the icon
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_icon_cb_set(Elm_Map_Group_Class *clas, ElmMapGroupIconGetFunc icon_get)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.icon_get = icon_get;
}

/**
 * Set the data associated to the group class (radio, radio2 or empty)
 *
 * @param clas the group class
 * @param data the new user data
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_data_set(Elm_Map_Group_Class *clas, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->data = data;
}

/**
 * Set the zoom from where the markers are displayed.
 *
 * Markers will not be displayed for a zoom less than @ref zoom
 *
 * @param clas the group class
 * @param zoom the zoom
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_zoom_displayed_set(Elm_Map_Group_Class *clas, int zoom)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->zoom_displayed = zoom;
}

/**
 * Set the zoom from where the markers are no more grouped.
 *
 * @param clas the group class
 * @param zoom the zoom
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_zoom_grouped_set(Elm_Map_Group_Class *clas, int zoom)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->zoom_grouped = zoom;
}

/**
 * Set if the markers associated to the group class @clas are hidden or not.
 * If @ref hide is true the markers will be hidden.
 *
 * @param clas the group class
 * @param hide if true the markers will be hidden, else they will be displayed.
 *
 * @ingroup Map
 */
EAPI void
elm_map_group_class_hide_set(Evas_Object *obj, Elm_Map_Group_Class *clas, Eina_Bool hide)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   EINA_SAFETY_ON_NULL_RETURN(clas);
   if (clas->hide == hide) return;
   clas->hide = hide;
   if (wd->grids)
     {
        Evas_Coord ox, oy, ow, oh;
        evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
        marker_place(obj, eina_list_data_get(wd->grids), wd->pan_x, wd->pan_y, ox, oy, ow, oh);
     }
}


/**
 * Create a marker class.
 *
 * Each marker must be associated to a class.
 * The class defines the style of the marker when a marker is displayed alone (not grouped).
 *
 * @param obj The map object
 * @return Returns the new class
 *
 * @ingroup Map
 */
EAPI Elm_Map_Marker_Class *
elm_map_marker_class_new(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   Elm_Map_Marker_Class *clas = calloc(1, sizeof(Elm_Map_Marker_Class));
   wd->markers_clas = eina_list_append(wd->markers_clas, clas);
   return clas;
}

/**
 * Set the style of a class (radio, radio2 or empty)
 *
 * @param clas the group class
 * @param style the new style
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_class_style_set(Elm_Map_Marker_Class *clas, const char *style)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   eina_stringshare_replace(&clas->style, style);
}

/**
 * Set the icon callback of a class.
 *
 * A custom icon can be displayed in a marker. The function @ref icon_get must return this icon.
 *
 * @param clas the group class
 * @param icon_get the callback to create the icon
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_class_icon_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerIconGetFunc icon_get)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.icon_get = icon_get;
}

/**
 *
 * Set the callback of the content of the bubble.
 *
 * When the user click on a marker, a bubble is displayed with a content.
 * The callback @ref get musst return this content. It can be NULL.
 *
 * @param clas the group class
 * @param get the callback to create the content
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_class_get_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerGetFunc get)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.get = get;
}

/**
 * Set the callback of the content of delete the object created by the callback "get".
 *
 * If this callback is defined the user will have to delete (or not) the object inside.
 * If the callback is not defined the object will be destroyed with evas_object_del()
 *
 * @param clas the group class
 * @param del the callback to delete the content
 *
 * @ingroup Map
 */
EAPI void
elm_map_marker_class_del_cb_set(Elm_Map_Marker_Class *clas, ElmMapMarkerDelFunc del)
{
   EINA_SAFETY_ON_NULL_RETURN(clas);
   clas->func.del = del;
}

/**
 * Set the source of the map.
 *
 * Elm_Map retrieves the image which composed the map from a web service. This web service can
 * be set with this method. A different service can return a different maps with different
 * information and it can use different zoom value.
 *
 * @param clas the group class
 * @param source the new source
 *
 * @ingroup Map
 */
EAPI void
elm_map_source_set(Evas_Object *obj, Elm_Map_Sources source)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *grid;
   int zoom;
   if (!wd) return;
   if (wd->source == source ) return;
   if (!map_sources_tab[source].url_cb) return;

   EINA_LIST_FREE(wd->grids, grid) grid_clear(obj, grid);

   wd->source = source;
   zoom = wd->zoom;
   wd->zoom = -1;

   if (map_sources_tab[wd->source].zoom_max < zoom)
     zoom = map_sources_tab[wd->source].zoom_max;
   if (map_sources_tab[wd->source].zoom_min > zoom)
     zoom = map_sources_tab[wd->source].zoom_min;

   elm_map_zoom_set(obj, zoom);
}

/**
 * Set the source of the route.
 *
 * @param clas the group class
 * @param source the new source
 *
 * @ingroup Map
 */
EAPI void
elm_map_route_source_set(Evas_Object *obj, Elm_Map_Route_Sources source __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
}

/**
 * Get the current source
 *
 * @param obj the map object
 * @return Returns the maximum zoom of the source
 *
 * @ingroup Map
 */
EAPI Elm_Map_Sources
elm_map_source_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_MAP_SOURCE_MAPNIK;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_MAP_SOURCE_MAPNIK;
   return wd->source;
}

/**
 * Get the current route source
 *
 * @param obj the map object
 * @return Returns the source of the route
 *
 * @ingroup Map
 */
EAPI Elm_Map_Route_Sources
elm_map_route_source_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_MAP_ROUTE_SOURCE_YOURS;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_MAP_ROUTE_SOURCE_YOURS;
   return wd->route_source;
}

/**
 * Set the API of a custom source.
 *
 * A custom web service can be associated to the source ELM_MAP_SOURCE_CUSTOM_(1..7).
 *
 * @param source the source ID (ELM_MAP_SOURCE_CUSTOM_(1..7))
 * @param name the name of the source
 * @param zoom_min the minimum zoom of the source, must be >= 0
 * @param zoom_max the maximum zoom of the source, must be <= ZOOM_MAX
 * @param url_cb the callback used to create the url from where a tile (png or jpeg file) is downloaded.
 *
 * @ingroup Map
 */
EAPI void
elm_map_source_custom_api_set(Elm_Map_Sources source, const char *name, int zoom_min, int zoom_max, ElmMapSourceURLFunc url_cb, ElmMapRouteSourceURLFunc route_url_cb)
{
   EINA_SAFETY_ON_NULL_RETURN(name);
   EINA_SAFETY_ON_NULL_RETURN(url_cb);
   map_sources_tab[source].name = name;
   map_sources_tab[source].zoom_min = zoom_min;
   map_sources_tab[source].zoom_max = zoom_max;
   map_sources_tab[source].url_cb = url_cb;
   map_sources_tab[source].route_url_cb = route_url_cb;
}

/**
 * Get the maximum zoom of the source.
 *
 * @param source the source
 * @return Returns the maximum zoom of the source
 *
 * @ingroup Map
 */
EAPI int
elm_map_source_zoom_max_get(Elm_Map_Sources source)
{
   return map_sources_tab[source].zoom_max;
}

/**
 * Get the minimum zoom of the source.
 *
 * @param source the source
 * @return Returns the minimum zoom of the source
 *
 * @ingroup Map
 */
EAPI int
elm_map_source_zoom_min_get(Elm_Map_Sources source)
{
   return map_sources_tab[source].zoom_min;
}

/**
 * Get the name of a source.
 *
 * @param source the source
 * @return Returns the name of the source
 *
 * @ingroup Map
 */
EAPI const char *
elm_map_source_name_get(Elm_Map_Sources source)
{
   return map_sources_tab[source].name;
}

/**
 * Set the user agent of the widget map.
 *
 * @param obj The map object
 * @param user_agent the user agent of the widget map
 *
 * @ingroup Map
 */
EAPI void
elm_map_user_agent_set(Evas_Object *obj, const char *user_agent)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (!wd->user_agent) wd->user_agent = eina_stringshare_add(user_agent);
   else eina_stringshare_replace(&wd->user_agent, user_agent);

   if (!wd->ua) wd->ua = eina_hash_string_small_new(NULL);
   eina_hash_set(wd->ua, "User-Agent", wd->user_agent);
}

/**
 * Get the user agent of the widget map.
 *
 * @param obj The map object
 * @return The user agent of the widget map
 *
 * @ingroup Map
 */
EAPI const char *
elm_map_user_agent_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;
   return wd->user_agent;
}

/**
 * Add a route on the map
 *
 * @param obj The map object
 * @param type the type if transport
 * @param method the routing method
 * @param flon the start longitude
 * @param flat the start latitude
 * @param tlon the destination longitude
 * @param tlat the destination latitude
 *
 * @return The Route object
 *
 * @ingroup Map
 */
EAPI Elm_Map_Route *
elm_map_route_add(Evas_Object *obj,
                  Elm_Map_Route_Type type,
                  Elm_Map_Route_Method method,
                  double flon,
                  double flat,
                  double tlon,
                  double tlat)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   char buf[PATH_MAX];
   char *source;
   char *type_name = NULL;
   int fd;
   Elm_Map_Route *route = ELM_NEW(Elm_Map_Route);
   if (!route) return NULL;

   snprintf(buf, sizeof(buf), DEST_XML_FILE);
   fd = mkstemp(buf);
   if (fd < 0)
     {
        free(route);
        return NULL;
     }
   
   route->con_url = ecore_con_url_new(NULL);
   route->ud.fname = strdup(buf);
   INF("xml file : %s", route->ud.fname);

   wd->fid++;
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
         (ECORE_CON_EVENT_URL_COMPLETE, _common_complete_cb, route));

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

   source = map_sources_tab[wd->source].route_url_cb(obj, type_name, method, flon, flat, tlon, tlat);
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
}

/**
 * Remove a route from the map
 *
 * @param route The route to remove
 *
 * @ingroup Map
 */

EAPI void
elm_map_route_remove(Elm_Map_Route *route)
{
   EINA_SAFETY_ON_NULL_RETURN(route);

   Route_Waypoint *w;
   Route_Node *n;
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
}

/**
 * Set the option used for the background color
 *
 * @param route The route object
 * @param r
 * @param g
 * @param b
 * @param a
 *
 * This sets the color used for the route
 *
 * @ingroup Map
 */
EAPI void
elm_map_route_color_set(Elm_Map_Route *route, int r, int g , int b, int a)
{
   route->color.r = r;
   route->color.g = g;
   route->color.b = b;
   route->color.a = a;
}

/**
 * Get the option used for the background color
 *
 * @param route The route object
 * @param r
 * @param g
 * @param b
 * @param a
 *
 * @ingroup Map
 */
EAPI void
elm_map_route_color_get(Elm_Map_Route *route, int *r, int *g , int *b, int *a)
{
   if (*r) *r = route->color.r;
   if (*g) *g = route->color.g;
   if (*b) *b = route->color.b;
   if (*a) *a = route->color.a;
}

/**
 * Get the information of route distance
 *
 * @param route the route object
 * @return Returns the distance of route (unit : km)
 *
 * @ingroup Map
 */
EAPI double
elm_map_route_distance_get(Elm_Map_Route *route)
{
   return route->info.distance;
}

/**
 * Get the information of route nodes
 *
 * @param route the route object
 * @return Returns the nodes of route
 *
 * @ingroup Map
 */

EAPI const char*
elm_map_route_node_get(Elm_Map_Route *route)
{
   return route->info.nodes;
}

/**
 * Get the information of route waypoint
 *
 * @param route the route object
 * @return Returns the waypoint of route
 *
 * @ingroup Map
 */

EAPI const char*
elm_map_route_waypoint_get(Elm_Map_Route *route)
{
   return route->info.waypoints;
}

static char *
_mapnik_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://tile.openstreetmap.org/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

static char *
_osmarender_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://tah.openstreetmap.org/Tiles/tile/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

static char *
_cyclemap_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://andy.sandbox.cloudmade.com/tiles/cycle/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

static char *
_maplint_url_cb(Evas_Object *obj __UNUSED__, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://tah.openstreetmap.org/Tiles/maplint/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

static char *
_custom1_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_custom2_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_custom3_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_custom4_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_custom5_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_custom6_url_cb(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, int zoom __UNUSED__)
{
   return strdup("");
}

static char *
_module_url_cb(Evas_Object *obj, int x, int y, int zoom)
{
   char *buf = NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (elm_map_source_get(obj) == ELM_MAP_SOURCE_MODULE)
     if ((wd->api) && (wd->api->obj_url_request))
       buf = wd->api->obj_url_request(obj, x, y, zoom);

   if (!buf) buf = strdup("");

   return buf;
}

static char *_yours_url_cb(Evas_Object *obj __UNUSED__, char *type_name, int method, double flon, double flat, double tlon, double tlat)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "%s?flat=%f&flon=%f&tlat=%f&tlon=%f&v=%s&fast=%d&instructions=1",
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

static char *_route_custom1_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}

static char *_route_custom2_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}

static char *_route_custom3_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}

static char *_route_custom4_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}

static char *_route_custom5_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}

static char *_route_custom6_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   return strdup("");
}
/*
static char *_route_module_url_cb(Evas_Object *obj __UNUSED__, char *type_name __UNUSED__, int method __UNUSED__, double flon __UNUSED__, double flat __UNUSED__, double tlon __UNUSED__, double tlat __UNUSED__)
{
   // TODO: make example route module
   return strdup("");
}
*/
