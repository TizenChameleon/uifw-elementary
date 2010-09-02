/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#ifdef ELM_EWEBKIT
#include <EWebKit.h>
#include <cairo.h>

#define SMART_NAME "els_webview"
#define API_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
#define EWK_VIEW_PRIV_GET_OR_RETURN(sd, ptr, ...)                   \
   Ewk_View_Private_Data* ptr = ((Ewk_View_Smart_Data*)sd)->_priv; \
   if (!ptr) \
   {                                                     \
      ERR("no private data for object %p (%s)",               \
	 ((Ewk_View_Smart_Data*)sd)->self,                           \
	 evas_object_type_get(((Ewk_View_Smart_Data*)sd)->self));    \
      return __VA_ARGS__;                                         \
   }

#define EWEBKIT_PATH "/usr/lib/libewebkit.so"
#define CAIRO_PATH "/usr/lib/libcairo.so.2"

#define MINIMAP_WIDTH 120
#define MINIMAP_HEIGHT 200

#define USE_MAX_TUC_20MB

#ifdef USE_MAX_TUC_20MB
#define MAX_TUC 1024*1024*20
#else
#define MAX_TUC 1024*1024*10
#endif
#define MAX_URI 512
#define MOBILE_DEFAULT_ZOOM_RATIO 1.5f

#define WEBVIEW_EDJ "/usr/share/edje/ewebview.edj"
#define WEBKIT_EDJ "/usr/share/edje/webkit.edj"
#define WEBVIEW_THEME_EDJ "/usr/share/edje/ewebview-theme.edj"

#define DEFAULT_LAYOUT_WIDTH 1024
#define MIN_ZOOM_RATIO 0.09f
#define MAX_ZOOM_RATIO 4.0f
#define ZOOM_OUT_BOUNCING 0.85f
#define ZOOM_IN_BOUNCING 1.25f
#define BOUNCING_DISTANCE 400

//			"<!--<body bgcolor=#4c4c4c text=white text-align=left>-->"
#define NOT_FOUND_PAGE_HEADER "<html>" \
			"<head><title>Page Not Found</title></head>" \
			"<body bgcolor=white text=black text-align=left>" \
			"<center>" \
			"<table>" \
			"<tr><td><h1>Page Not Found<br/></td></tr>" \
			"<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>" \
			"<tr><td>" \
			"<script type='text/javascript'>"\
			"var s = "

#define NOT_FOUND_PAGE_FOOTER ";" \
			"var failingUrl = s.substring(s.indexOf(\"?\"\)+1, s.lastIndexOf(\"?\"\));" \
			"document.write(\"<p><tr><td><h2>URL: \" + unescape(failingUrl) + \"</h2></td></tr>\");" \
			"var errorDesc = s.substring(s.lastIndexOf(\"?\")+1, s.length);" \
			"document.write(\"<tr><td><h2>Error: \" + unescape(errorDesc) + \"</h2></td></tr>\");" \
			"document.write(\"<tr><td><h3>Google: <form method=\'get\' action=\'http://www.google.com/custom\'><input type=text name=\'q\' size=15 maxlength=100 value=\'\"+ unescape(failingUrl)+\"\'> <input type=submit name=\'sa\' value=Search></form></h3></td></tr>\");" \
			"</script>" \
			"</td></tr>" \
			"</table>" \
			"</h1>" \
			"</body>" \
			"</html>"

#define NEED_TO_REMOVE
//#define DBG printf
//#define ERR printf

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data {
     Ewk_View_Smart_Data base; //default data

     Evas_Object* widget;
     Ecore_Job *move_calc_job;
     Ecore_Job *resize_calc_job;
     Eina_Hash* mime_func_hash;
     int locked_dx;
     int locked_dy;
     unsigned char bounce_horiz : 1;
     unsigned char bounce_vert : 1;
     unsigned char events_feed : 1;
     unsigned char auto_fitting : 1;
     unsigned char mouse_clicked : 1;

     /* ewk functions */
     void (*ewk_view_theme_set)(Evas_Object *, const char *);
     Evas_Object *(*ewk_view_frame_main_get)(const Evas_Object *);
     Eina_Bool (*ewk_view_uri_set)(Evas_Object *, const char *);
     float (*ewk_view_zoom_get)(const Evas_Object *);
     const char * (*ewk_view_uri_get)(const Evas_Object *o);
     Eina_Bool (*ewk_view_zoom_set)(Evas_Object *, float, Evas_Coord, Evas_Coord);
     Eina_Bool (*ewk_view_zoom_weak_set)(Evas_Object *, float, Evas_Coord, Evas_Coord);
     Eina_Bool (*ewk_view_zoom_text_only_set)(Evas_Object *, Eina_Bool);
     Eina_Bool (*ewk_view_zoom_cairo_scaling_get)(const Evas_Object *);
     Eina_Bool (*ewk_view_zoom_cairo_scaling_set)(Evas_Object *, Eina_Bool);
     void (*ewk_view_viewport_get)(Evas_Object *, int *, int *, float *, float *, float *, Eina_Bool *);
     void (*ewk_view_zoom_range_set)(Evas_Object *, float, float);
     void (*ewk_view_user_scalable_set)(Evas_Object *, Eina_Bool);
     Eina_Bool (*ewk_view_pre_render_region)(Evas_Object *, Evas_Coord, Evas_Coord, Evas_Coord, Evas_Coord, float);
     void (*ewk_view_pre_render_cancel)(Evas_Object *);
     Eina_Bool (*ewk_view_enable_render)(const Evas_Object *);
     Eina_Bool (*ewk_view_disable_render)(const Evas_Object *);
     void (*ewk_view_javascript_suspend)(Evas_Object *);
     void (*ewk_view_javascript_resume)(Evas_Object *);
     void (*ewk_view_fixed_layout_size_set)(Evas_Object *, Evas_Coord, Evas_Coord);
     Eina_Bool (*ewk_view_setting_enable_plugins_get)(const Evas_Object *);
     void (*ewk_view_pause_and_or_hide_plugins)(Evas_Object *, Eina_Bool, Eina_Bool);
     Eina_Bool (*ewk_view_suspend_request)(Evas_Object *);
     Eina_Bool (*ewk_view_resume_request)(Evas_Object *);
     Eina_Bool (*ewk_view_select_none)(Evas_Object *);
     Eina_Bool (*ewk_view_get_smart_zoom_rect)(Evas_Object *, int, int, const Evas_Event_Mouse_Up *, Eina_Rectangle *);
     Eina_Bool (*ewk_view_paint_contents)(Ewk_View_Private_Data *, cairo_t *, const Eina_Rectangle *);
     Eina_Bool (*ewk_view_stop)(Evas_Object *);
     Ewk_Tile_Unused_Cache *(*ewk_view_tiled_unused_cache_get)(const Evas_Object *);
     void (*ewk_view_tiled_unused_cache_set)(Evas_Object *, Ewk_Tile_Unused_Cache *);
     void (*ewk_tile_unused_cache_max_set)(Ewk_Tile_Unused_Cache *, size_t);
     size_t (*ewk_tile_unused_cache_max_get)(const Ewk_Tile_Unused_Cache *);
     size_t (*ewk_tile_unused_cache_used_get)(const Ewk_Tile_Unused_Cache *);
     size_t (*ewk_tile_unused_cache_flush)(Ewk_Tile_Unused_Cache *, size_t);
     void (*ewk_tile_unused_cache_auto_flush)(Ewk_Tile_Unused_Cache *);
     char * (*ewk_page_check_point_for_keyboard)(Evas_Object *, int, int, Eina_Bool *);
     Eina_Bool (*ewk_page_check_point)(Evas_Object *, int, int, Evas_Event_Mouse_Down *, Eina_Bool *, Eina_Bool *, char **, char **, char **);
     char ** (*ewk_page_dropdown_get_options)(Evas_Object *, int, int, int *, int *);
     Eina_Bool (*ewk_page_dropdown_set_current_index)(Evas_Object *, int, int, int);
     Eina_Bool (*ewk_frame_contents_size_get)(const Evas_Object *, Evas_Coord *, Evas_Coord *);
     Ewk_Hit_Test * (*ewk_frame_hit_test_new)(const Evas_Object *, int, int);
     Eina_Bool (*ewk_frame_feed_mouse_down)(Evas_Object *, const Evas_Event_Mouse_Down *);
     Eina_Bool (*ewk_frame_feed_mouse_up)(Evas_Object *, const Evas_Event_Mouse_Up *);
     Eina_Bool (*ewk_frame_visible_content_geometry_get)(const Evas_Object *, Eina_Bool, int *, int *, int *, int *);
     Eina_Bool (*ewk_frame_scroll_pos_get)(const Evas_Object *, int *, int *);
     void (*ewk_frame_hit_test_free)(Ewk_Hit_Test *);
     Eina_Bool (*ewk_frame_contents_set)(Evas_Object *, const char *, size_t, const char *, const char *, const char *);
     Eina_Bool (*ewk_frame_select_closest_word)(Evas_Object *, int, int, int *, int *, int *, int *, int *, int *);
     Eina_Bool (*ewk_frame_selection_handlers_get)(Evas_Object *, int *, int *, int *, int *, int *, int *);
     Eina_Bool (*ewk_frame_selection_left_set)(Evas_Object *, int, int, int *, int *, int *);
     Eina_Bool (*ewk_frame_selection_right_set)(Evas_Object *, int, int, int *, int *, int *);
     Eina_Bool (*ewk_frame_feed_focus_in)(Evas_Object *);
     Eina_Bool (*ewk_frame_scroll_add)(Evas_Object *, int, int);
     unsigned int (*ewk_view_imh_get)(Evas_Object *);
     Ecore_IMF_Context* (*ewk_view_core_imContext_get)(Evas_Object *);
     void (*ewk_set_show_geolocation_permission_dialog_callback)(ewk_show_geolocation_permission_dialog_callback);
     void (*ewk_set_geolocation_sharing_allowed)(void *, Eina_Bool);

     /* cairo functions */
     cairo_t * (*cairo_create)(cairo_surface_t *);
     void (*cairo_destroy)(cairo_t *);
     void (*cairo_paint)(cairo_t *);
     void (*cairo_stroke)(cairo_t *cr); 
     void (*cairo_scale)(cairo_t *, double, double);
     void (*cairo_rectangle)(cairo_t *, double, double, double, double);
     void (*cairo_set_source_rgb)(cairo_t *, double, double, double);
     cairo_status_t (*cairo_surface_status)(cairo_surface_t *);
     void (*cairo_surface_destroy)(cairo_surface_t *);
     void (*cairo_set_line_width)(cairo_t *, double);
     void (*cairo_set_source_surface)(cairo_t *, cairo_surface_t *, double, double);
     cairo_status_t (*cairo_surface_write_to_png)(cairo_surface_t *, const char *);
     cairo_surface_t * (*cairo_image_surface_create)(cairo_format_t, int, int);
     void (*cairo_set_antialias)(cairo_t *, cairo_antialias_t);
     cairo_surface_t * (*cairo_image_surface_create_for_data)(unsigned char *, cairo_format_t, int, int, int);

     /* add user data */
     struct {
	  Evas_Object* eo;
	  Evas_Object* content;
	  int cw, ch;
     } minimap;

     struct {
	  char** options;
	  int option_cnt;
	  int option_idx;
     } dropdown;

     struct {
	  Evas_Point basis;    // basis point of zoom
	  int finger_distance; // distance between two finger
	  int zooming_level;
	  float zooming_rate;
	  float zoom_rate_at_start;
	  float zoom_rate_to_set;
	  Evas_Point scroll_at_start;
	  Evas_Point scroll_to_set;
	  float init_zoom_rate;
	  float min_zoom_rate; //content based minimum
	  float max_zoom_rate;
	  Eina_Bool scalable;
     } zoom;

     struct {
	  int w, h;
     } content;

     struct {
	  int default_w;
	  int w, h;
     } layout;

     Ecore_Animator* smart_zoom_animator;

     Evas_Point pan_s;
     Evas_Event_Mouse_Down mouse_down_copy;
     Evas_Event_Mouse_Up mouse_up_copy;

     cairo_surface_t* thumbnail;
     float current_zoom_level;

     Eina_Bool tiled;
     Eina_Bool on_panning;
     Eina_Bool on_zooming;
     Eina_Bool is_mobile_page;

     Eina_Bool use_text_selection;
     Eina_Bool text_selection_on;
     struct {
	  Evas_Coord_Rectangle front;
	  Evas_Coord_Rectangle back;
	  Evas_Point front_handle;
	  Evas_Point back_handle;
	  Eina_Bool front_handle_moving;
	  Eina_Bool back_handle_moving;
     } text_selection;
     void* touch_obj;

     Ecore_Idler *flush_and_pre_render_idler;
     Eina_Bool use_zoom_bouncing;
};

/* local subsystem functions */
static void      _resize_calc_job(void *data);
static void      _move_calc_job(void *data);
static void      _smart_show(Evas_Object* obj);
static void      _smart_hide(Evas_Object* obj);
static void      _smart_resize(Evas_Object* obj, Evas_Coord w, Evas_Coord h);
static void      _smart_move(Evas_Object* obj, Evas_Coord x, Evas_Coord y);
#ifdef DEBUG
static void      _smart_calculate(Evas_Object* obj);
#endif
static Eina_Bool _smart_mouse_down(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Down* ev);
static Eina_Bool _smart_mouse_up(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Up* ev);
static Eina_Bool _smart_mouse_move(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Move* ev);
static void      _smart_add_console_message(Ewk_View_Smart_Data *esd, const char *message, unsigned int lineNumber, const char *sourceID);
static void      _smart_run_javascript_alert(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message);
static Eina_Bool _smart_run_javascript_confirm(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message);
static Eina_Bool _smart_run_javascript_prompt(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message, const char *defaultValue, char **value);
static Eina_Bool _smart_should_interrupt_javascript(Ewk_View_Smart_Data *esd);
static Eina_Bool _smart_run_open_panel(Ewk_View_Smart_Data *esd, Evas_Object *frame, Eina_Bool allows_multiple_files, const Eina_List *suggested_filenames, Eina_List **selected_filenames);
static Eina_Bool _smart_navigation_policy_decision(Ewk_View_Smart_Data *esd, Ewk_Frame_Resource_Request *request);
static void      _view_on_mouse_down(void* data, Evas* e, Evas_Object* o, void* event_info);
static void      _view_on_mouse_up(void* data, Evas* e, Evas_Object* o, void* event_info);
static void      _smart_load_started(void* data, Evas_Object* webview, void* error);
static void      _smart_load_finished(void* data, Evas_Object* webview, void* arg);
static void      _smart_load_error(void* data, Evas_Object* webview, void* arg);
static void      _smart_viewport_changed(void* data, Evas_Object* webview, void* arg);
static void      _smart_input_method_changed(void* data, Evas_Object* webview, void* arg);
static void      _smart_page_layout_info_set(Smart_Data *sd, float init_zoom_rate, float min_zoom_rate, float max_zoom_rate, Eina_Bool scalable);
static void      _smart_contents_size_changed(void* data, Evas_Object* frame, void* arg);
static void      _smart_load_nonemptylayout_finished(void* data, Evas_Object* frame, void* arg);
static void      _smart_cb_view_created(void* data, Evas_Object* webview, void* arg);
static void      _smart_add(Evas_Object* obj);
static void      _smart_del(Evas_Object* o);
static void      _directional_pre_render(Evas_Object* webview, int dx, int dy);
static void      _smart_cb_mouse_down(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_mouse_up(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_mouse_tap(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_pan_start(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_pan_by(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_pan_stop(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_select_closest_word(void* data, Evas_Object* webview, void* ev);
static void      _smart_cb_unselect_closest_word(void* data, Evas_Object* webview, void* ev);
static void      _suspend_all(Smart_Data *sd, Eina_Bool hidePlugin);
static void      _resume_all(Smart_Data *sd, Eina_Bool hidePlugin);
static void      _zoom_start(Smart_Data* sd, int centerX, int centerY, int distance);
static void      _zoom_move(Smart_Data* sd, int centerX, int centerY, int distance);
static void      _zoom_stop(Smart_Data* sd);
static void      _adjust_to_contents_boundary(Evas_Object* webview, int* to_x, int* to_y, int from_x, int from_y, float new_zoom_rate);
static int       _smart_zoom_animator(void* data);
static void      _smart_cb_pinch_zoom_start(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_pinch_zoom_move(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_pinch_zoom_stop(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_vertical_zoom_start(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_vertical_zoom_move(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_vertical_zoom_stop(void* data, Evas_Object* webview, void* event_info);
static void      _smart_cb_smart_zoom(void* data, Evas_Object* webview, void* event_info);
static void      _zoom_to_rect(Smart_Data *sd, int x, int y);
static void      _text_selection_init(Evas_Object* parent);
static void      _text_selection_show(void);
static void      _text_selection_hide(Smart_Data *sd);
static void      _text_selection_set_front_info(Smart_Data *sd, int x, int y, int height);
static void      _text_selection_set_back_info(Smart_Data *sd, int x, int y, int height);
static Eina_Bool _text_selection_handle_pressed(Smart_Data *sd, int x, int y);
static void      _text_selection_update_position(Smart_Data *sd, int x, int y);
static void      _text_selection_move_by(Smart_Data *sd, int dx, int dy);
static void      _minimap_update_detail(Evas_Object* minimap, Smart_Data *sd, cairo_surface_t* src, int srcW, int srcH, Eina_Rectangle* visibleRect);
static void      _minimap_update(Evas_Object* minimap, Smart_Data *sd, cairo_surface_t* src, int minimapW, int minimapH);
static cairo_surface_t* _image_clone_get(Smart_Data *sd, int* minimap_w, int* minimap_h);
static void      _unzoom_position(Evas_Object* webview, int x, int y, int* ux, int* uy);
static void      _coords_evas_to_ewk(Evas_Object* webview, int x, int y, int* ux, int* uy);
static void      _coords_ewk_to_evas(Evas_Object* webview, int x, int y, int* ux, int* uy);
static void      _update_min_zoom_rate(Evas_Object *obj);
static void      _geolocation_permission_callback(void *geolocation_obj, const char* url);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;
static Ewk_View_Smart_Class _parent_sc = EWK_VIEW_SMART_CLASS_INIT_NULL;

/* ewk functions */
static void *ewk_handle;
static void *cairo_handle;

static Ewk_Tile_Unused_Cache *ewk_tile_cache = NULL;
static ewk_tile_cache_ref_count = 0;

static Evas_Object *obj = NULL;

/* externally accessible functions */
Evas_Object*
_elm_smart_webview_add(Evas *evas, Eina_Bool tiled)
{
   Evas_Object* webview;
   int (*ewk_init)(void) = NULL;
   void (*ewk_dnet_open)(void) = NULL;
   Eina_Bool (*ewk_view_single_smart_set)(Ewk_View_Smart_Class *) = NULL;
   Eina_Bool (*ewk_view_tiled_smart_set)(Ewk_View_Smart_Class *) = NULL;

   if (!_smart)
     {
	ewk_handle = dlopen(EWEBKIT_PATH, RTLD_LAZY);
	if (ewk_handle == NULL)
	  {
	     ERR("could not initialize ewk \n");
	     return NULL;
	  }
	cairo_handle = dlopen(CAIRO_PATH, RTLD_LAZY);
	if (cairo_handle == NULL)
	  {
	     ERR("could not initialize cairo \n");
	     return NULL;
	  }

	// init ewk
	if (!ewk_init)
	  ewk_init = (int (*)())dlsym(ewk_handle, "ewk_init");
	ewk_init();

	if (!ewk_dnet_open)
	  ewk_dnet_open = (void (*)())dlsym(ewk_handle, "ewk_dnet_open");
	ewk_dnet_open();

	/* create subclass */
	static Ewk_View_Smart_Class _api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(SMART_NAME);

	if (tiled)
	  {
	     if (!ewk_view_tiled_smart_set)
	       ewk_view_tiled_smart_set = (Eina_Bool (*)(Ewk_View_Smart_Class *))dlsym(ewk_handle, "ewk_view_tiled_smart_set");
	     ewk_view_tiled_smart_set(&_api);
	     if (EINA_UNLIKELY(!_parent_sc.sc.add))
	       ewk_view_tiled_smart_set(&_parent_sc);

	  } else {
	       if (!ewk_view_single_smart_set)
		 ewk_view_single_smart_set = (Eina_Bool (*)(Ewk_View_Smart_Class *))dlsym(ewk_handle, "ewk_view_single_smart_set");
	       ewk_view_single_smart_set(&_api);
	       if (EINA_UNLIKELY(!_parent_sc.sc.add))
		 ewk_view_single_smart_set(&_parent_sc);
	  }

	_api.sc.add     = _smart_add;
	_api.sc.del     = _smart_del;
	_api.sc.show    = _smart_show;
	_api.sc.hide    = _smart_hide;
	_api.sc.resize  = _smart_resize;
	_api.sc.move  = _smart_move;
#ifdef DEBUG
	_api.sc.calculate = _smart_calculate;
#endif
	_api.mouse_down = _smart_mouse_down;
	_api.mouse_up   = _smart_mouse_up  ;
	_api.mouse_move = _smart_mouse_move;

	_api.add_console_message = _smart_add_console_message;
	_api.run_javascript_alert = _smart_run_javascript_alert;
	_api.run_javascript_confirm = _smart_run_javascript_confirm;
	_api.run_javascript_prompt = _smart_run_javascript_prompt;
	_api.should_interrupt_javascript = _smart_should_interrupt_javascript;
	_api.run_open_panel = _smart_run_open_panel;
	//_api.navigation_policy_decision = _smart_navigation_policy_decision;

	_smart = evas_smart_class_new(&_api.sc);
	elm_theme_overlay_add(NULL, WEBVIEW_THEME_EDJ);

     }

   if (!_smart)
     {
	ERR("could not create smart class\n");
	return NULL;
     }

   webview = evas_object_smart_add(evas, _smart);
   if (!webview)
     {
	ERR("could not create smart object for webview");
	return NULL;
     }
   obj = webview;

   // set tiled and unused cache 
   Smart_Data* sd = evas_object_smart_data_get(webview);
   if (sd)
     {
	sd->tiled = tiled;
	if (sd->tiled)
	  {
	     if (ewk_tile_cache_ref_count == 0)
	       {
		  if (!sd->ewk_view_tiled_unused_cache_get)
		    sd->ewk_view_tiled_unused_cache_get = (Ewk_Tile_Unused_Cache *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_get");
		  ewk_tile_cache = sd->ewk_view_tiled_unused_cache_get(webview);

		  if (!sd->ewk_tile_unused_cache_max_set)
		    sd->ewk_tile_unused_cache_max_set = (void (*)(Ewk_Tile_Unused_Cache *, size_t))dlsym(ewk_handle, "ewk_tile_unused_cache_max_set");
		  sd->ewk_tile_unused_cache_max_set(ewk_tile_cache, MAX_TUC);
	       } else {
		    if (!sd->ewk_view_tiled_unused_cache_set)
		      sd->ewk_view_tiled_unused_cache_set = (void (*)(Evas_Object *, Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_set");
		    sd->ewk_view_tiled_unused_cache_set(webview, ewk_tile_cache);
	       }
	     ++ewk_tile_cache_ref_count;
	     //size_t mem = ewk_tile_unused_cache_used_get(ewk_tile_cache);
	     //DBG("%s: Used cache: %d (%dkB)", __func__, mem, (mem/1024));
	  }
     }

   return webview;
}

void
_elm_smart_webview_events_feed_set(Evas_Object* obj, Eina_Bool feed)
{
   API_ENTRY return;
   sd->events_feed = feed;
}

Eina_Bool
_elm_smart_webview_events_feed_get(Evas_Object* obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->events_feed;
}

void
_elm_smart_webview_auto_fitting_set(Evas_Object* obj, Eina_Bool enable)
{
   API_ENTRY return;
   sd->auto_fitting = enable;
}

Eina_Bool
_elm_smart_webview_auto_fitting_get(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->auto_fitting;
}

Evas_Object *
_elm_smart_webview_minimap_get(Evas_Object* obj)
{
   DBG("%s\n", __func__);
   API_ENTRY return NULL;

   if (sd->minimap.eo != NULL) return sd->minimap.eo;

   sd->minimap.eo = edje_object_add(evas_object_evas_get(obj));
   edje_object_file_set(sd->minimap.eo, WEBVIEW_EDJ, "minimap");

   sd->minimap.content = evas_object_image_add(evas_object_evas_get(sd->minimap.eo));
   evas_object_size_hint_align_set(sd->minimap.content, 0.5, 0.5);
   evas_object_image_colorspace_set(sd->minimap.content, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(sd->minimap.content, EINA_FALSE);

   Evas_Object* box = evas_object_box_add(evas_object_evas_get(sd->minimap.eo));
   evas_object_box_append(box, sd->minimap.content);
   evas_object_show(sd->minimap.content);
   edje_object_part_swallow(sd->minimap.eo, "swallow.content", box);

   return sd->minimap.eo;
}

void
_elm_smart_webview_uri_set(Evas_Object* obj, const char* uri)
{
   API_ENTRY return;

   char full_uri[MAX_URI] = "";
   printf("<< uri [%s] >>\n", uri);

   if (uri == NULL)
     return;

   // check uri string
   int len = strlen(uri);
   if (len)
     {
	if (strstr(uri, "://") == NULL) {
	     strncpy(full_uri, "http://", 7);
	     full_uri[7] = '\0';
	     len = (len >= (MAX_URI - 7)) ? (MAX_URI - 8) : len;
	     strncat(full_uri, uri, len);
	} else {
	     len = (len >= MAX_URI) ? (MAX_URI - 1) : len;
	     strncpy(full_uri, uri, len);
	     full_uri[len] = '\0';
	}

	printf("<< full uri [%s] >>\n", full_uri);
	if (!sd->ewk_view_uri_set)
	  sd->ewk_view_uri_set = (Eina_Bool (*)(Evas_Object *, const char *))dlsym(ewk_handle, "ewk_view_uri_set");
	sd->ewk_view_uri_set(obj, full_uri);
     }
}

void
_elm_smart_webview_widget_set(Evas_Object *obj, Evas_Object *wid)
{
   API_ENTRY return;
   sd->widget = wid;
}

void
_elm_smart_webview_bounce_allow_set(Evas_Object* obj, Eina_Bool horiz, Eina_Bool vert)
{
   API_ENTRY return;
   sd->bounce_horiz = horiz;
   sd->bounce_vert = vert;
}

void
_elm_smart_webview_mime_callback_set(Evas_Object* obj, const char *mime, Elm_WebView_Mime_Cb func)
{
   API_ENTRY return;
   if (!sd->mime_func_hash)
     sd->mime_func_hash = eina_hash_pointer_new(NULL);

   if (!func)
     eina_hash_del(sd->mime_func_hash, mime, func);
   else
     eina_hash_add(sd->mime_func_hash, mime, func);
}

void
_elm_smart_webview_default_layout_width_set(Evas_Object *obj, int width)
{
   API_ENTRY return;
   sd->layout.default_w = width;
}

int
_flush_and_pre_render(void *data)
{
   Evas_Object *obj = (Evas_Object *)data;
   API_ENTRY return ECORE_CALLBACK_CANCEL;

   if (!sd->ewk_view_tiled_unused_cache_get)
     sd->ewk_view_tiled_unused_cache_get = (Ewk_Tile_Unused_Cache *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_get");
   if (!sd->ewk_tile_unused_cache_used_get)
     sd->ewk_tile_unused_cache_used_get = (size_t (*)(const Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_used_get");
   if (!sd->ewk_tile_unused_cache_flush)
     sd->ewk_tile_unused_cache_flush = (size_t (*)(Ewk_Tile_Unused_Cache *, size_t))dlsym(ewk_handle, "ewk_tile_unused_cache_flush");

   Ewk_Tile_Unused_Cache *tuc = sd->ewk_view_tiled_unused_cache_get(obj);
   sd->ewk_tile_unused_cache_flush(tuc, sd->ewk_tile_unused_cache_used_get(tuc));
   _directional_pre_render(obj, 0, 0);

   sd->flush_and_pre_render_idler = NULL;

   return ECORE_CALLBACK_CANCEL;
}

/* local subsystem functions */
static void
_smart_show(Evas_Object* obj)
{
   DBG("%s\n", __func__);
   INTERNAL_ENTRY;

   _elm_smart_touch_start(sd->touch_obj);
   _parent_sc.sc.show(obj);
}

static void
_smart_hide(Evas_Object* obj)
{
   DBG("%s\n", __func__);
   INTERNAL_ENTRY;

   _elm_smart_touch_stop(sd->touch_obj);
   _parent_sc.sc.hide(obj);
}

static void
_smart_resize(Evas_Object* obj, Evas_Coord w, Evas_Coord h)
{
   DBG("%s\n", __func__);
   INTERNAL_ENTRY;

   Evas_Coord ow, oh;
   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   if (sd->resize_calc_job) ecore_job_del(sd->resize_calc_job);
   sd->resize_calc_job = ecore_job_add(_resize_calc_job, obj);
}

static void
_resize_calc_job(void *data)
{
   Evas_Object *obj = data;
   INTERNAL_ENTRY;

   int object_w, object_h;
   evas_object_geometry_get(obj, NULL, NULL, &object_w, &object_h);
   object_w = (object_w % 10) ? (object_w / 10 * 10 + 10) : object_w;

   if (sd->is_mobile_page)
     {
	int old_layout_w = sd->layout.w;
	sd->layout.w = object_w / sd->zoom.init_zoom_rate;
	sd->layout.h = object_h / sd->zoom.init_zoom_rate;
	if (old_layout_w != sd->layout.w)
	  {
	     if (!sd->ewk_view_fixed_layout_size_set)
	       sd->ewk_view_fixed_layout_size_set = (void (*)(Evas_Object *, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_fixed_layout_size_set");
	     sd->ewk_view_fixed_layout_size_set(obj, sd->layout.w, sd->layout.h);
	  }
     }
   else
     {
	if (!sd->ewk_view_zoom_get)
	  sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
	if (!sd->ewk_view_zoom_set)
	  sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");

	_update_min_zoom_rate(obj);

	// set zoom
	if (sd->ewk_view_zoom_get(obj) < sd->zoom.min_zoom_rate)
	  sd->ewk_view_zoom_set(obj, sd->zoom.min_zoom_rate, 0, 0);
     }

   // call preRender by timer, because we can not get the correct visible_content of frame
   // when call it directly.
   if (!sd->ewk_view_uri_get)
     sd->ewk_view_uri_get = (const char * (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_uri_get");
   const char *url = sd->ewk_view_uri_get(obj);
   if (url && strcmp(url, "") != 0 && sd->flush_and_pre_render_idler == NULL)
     {
	sd->flush_and_pre_render_idler = ecore_idler_add(_flush_and_pre_render, obj);
     }

   sd->resize_calc_job = NULL;
   _parent_sc.sc.resize(obj, object_w, object_h);
}

static void
_move_calc_job(void *data)
{
   Evas_Object *obj = data;
   INTERNAL_ENTRY;
   int x, y;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   sd->move_calc_job = NULL;
   _parent_sc.sc.move(obj, x, y);
}

static void
_smart_move(Evas_Object* obj, Evas_Coord x, Evas_Coord y)
{
   DBG("%s\n", __func__);
   INTERNAL_ENTRY;

   if (sd->move_calc_job) ecore_job_del(sd->move_calc_job);
   sd->move_calc_job = ecore_job_add(_move_calc_job, obj);
}

#ifdef DEBUG
static void
_smart_calculate(Evas_Object* obj)
{
   _parent_sc.sc.calculate(obj);
}
#endif

static Eina_Bool
_smart_mouse_down(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Down* ev)
{
   DBG("[NATIVE]%s is called\n", __func__);
   Smart_Data *sd = (Smart_Data *)esd;
   sd->mouse_down_copy = *ev;

   if (sd->events_feed)
     {
	_suspend_all(sd, EINA_FALSE);
	sd->mouse_clicked = EINA_TRUE;
	return _parent_sc.mouse_down(esd, ev);
     }
   else return EINA_TRUE;
}

static Eina_Bool
_smart_mouse_up(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Up* ev)
{
   DBG("[NATIVE]%s is called\n", __func__);
   Smart_Data *sd = (Smart_Data *)esd;
   sd->mouse_up_copy = *ev;

   if (sd->events_feed)
     {
	_resume_all(sd, EINA_FALSE);
	//check if user hold touch
	if (ev && (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
	  {
	     return EINA_TRUE;
	  }

	Eina_Bool ret = _parent_sc.mouse_up(esd, ev);
	sd->mouse_clicked = EINA_FALSE;
	return ret;
     }
   else
     return EINA_TRUE;
}

static Eina_Bool
_smart_mouse_move(Ewk_View_Smart_Data *esd, const Evas_Event_Mouse_Move* ev)
{
   Smart_Data *sd = (Smart_Data *)esd;
   if (sd->events_feed) _parent_sc.mouse_move(esd, ev);
   else return EINA_TRUE;
}

static void
_smart_add_console_message(Ewk_View_Smart_Data *esd, const char *message, unsigned int lineNumber, const char *sourceID)
{
   //TODO
}

static void
_smart_run_javascript_alert(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message)
{
   Evas_Object *popup;
   popup = elm_popup_add(esd->self);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_popup_desc_set(popup, message);
   elm_popup_buttons_add(popup, 1, "Ok", ELM_POPUP_RESPONSE_OK, NULL);
   evas_object_show(popup);
}

static Eina_Bool
_smart_run_javascript_confirm(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message)
{
   Evas_Object *popup;
   popup = elm_popup_add(esd->self);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_popup_desc_set(popup, message);
   elm_popup_buttons_add(popup, 2, "Ok", ELM_POPUP_RESPONSE_OK, "Cancel", ELM_POPUP_RESPONSE_CANCEL, NULL);

   int ret = elm_popup_run(popup);
   evas_object_del(popup);
   switch (ret)
     {
      case ELM_POPUP_RESPONSE_OK:
	 return EINA_TRUE;
      case ELM_POPUP_RESPONSE_CANCEL:
	 return EINA_FALSE;
      default:
	 elm_exit();
     }
   return EINA_FALSE;
}

static Eina_Bool
_smart_run_javascript_prompt(Ewk_View_Smart_Data *esd, Evas_Object *frame, const char *message, const char *defaultValue, char **value)
{
   //FIXME: it's not work
   Evas_Object *popup;
   Evas_Object *box, *entry, *label;

   popup = elm_popup_add(esd->self);
   elm_object_style_set(popup, "customstyle");
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_popup_buttons_add(popup, 2, "Ok", ELM_POPUP_RESPONSE_OK, "Cancel", ELM_POPUP_RESPONSE_CANCEL, NULL);

   box = elm_box_add(popup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   label = elm_label_add(box);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_label_label_set(label, message);
   elm_box_pack_start(box, label);
   evas_object_show(label);

   entry = elm_entry_add(box);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_entry_set(entry, defaultValue);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);

   int ret = elm_popup_run(popup);
   *value = strdup("temp");
   evas_object_del(popup);

   return EINA_FALSE;
}

static Eina_Bool
_smart_should_interrupt_javascript(Ewk_View_Smart_Data *esd)
{
   //TODO
   return EINA_FALSE;
}

static Eina_Bool 
_smart_run_open_panel(Ewk_View_Smart_Data *esd, Evas_Object *frame, Eina_Bool allows_multiple_files, const Eina_List *suggested_filenames, Eina_List **selected_filenames)
{
   //TODO
   return EINA_FALSE;
}

static Eina_Bool
_smart_navigation_policy_decision(Ewk_View_Smart_Data *esd, Ewk_Frame_Resource_Request *request)
{
   char *protocol_hack;
   Smart_Data *sd = (Smart_Data*)esd;
   if (!sd->mime_func_hash)
     return EINA_FALSE;

   protocol_hack = strstr(request->url, ":");
   *protocol_hack = '\0';
   Elm_WebView_Mime_Cb func = (Elm_WebView_Mime_Cb) eina_hash_find(sd->mime_func_hash, request->url);
   *protocol_hack = ':';

   if (!func)
     {
	if (strncmp(request->url, "http", 4) == 0
	      || strncmp(request->url, "https", 5) == 0
	      || strncmp(request->url, "file", 4) == 0)
	  return EINA_TRUE;
	return EINA_FALSE;
     }
   else
     return func(esd->self);
}

#ifdef NEED_TO_REMOVE
// TODO: temporary mouse callback until the webkit engine can receive mouse events
static void
_view_on_mouse_down(void* data, Evas* e, Evas_Object* o, void* event_info)
{
   Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
   Ewk_View_Smart_Data* sd = (Ewk_View_Smart_Data*)data;
   EINA_SAFETY_ON_NULL_RETURN(sd->api);
   EINA_SAFETY_ON_NULL_RETURN(sd->api->mouse_down);
   sd->api->mouse_down(sd, ev);
}

static void
_view_on_mouse_up(void* data, Evas* e, Evas_Object* o, void* event_info)
{
   Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
   Ewk_View_Smart_Data* sd = (Ewk_View_Smart_Data*)data;
   EINA_SAFETY_ON_NULL_RETURN(sd->api);
   EINA_SAFETY_ON_NULL_RETURN(sd->api->mouse_up);
   sd->api->mouse_up(sd, ev);
}
#endif

static void
_smart_load_started(void* data, Evas_Object* webview, void* error)
{
   DBG("%s is called\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   if (!sd->ewk_view_user_scalable_set)
     sd->ewk_view_user_scalable_set = (void (*)(Evas_Object *, Eina_Bool))dlsym(ewk_handle, "ewk_view_user_scalable_set");

   // set default layout and zoom level
   sd->is_mobile_page = EINA_FALSE;
   sd->layout.w = -1;
   sd->layout.h = -1;
   sd->zoom.init_zoom_rate = 1.0f;
   sd->zoom.scalable = EINA_TRUE;
   sd->ewk_view_user_scalable_set(webview, EINA_TRUE);
}

static void
_smart_load_finished(void* data, Evas_Object* webview, void* arg)
{
   DBG("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   // if error, call loadNotFoundPage
   Ewk_Frame_Load_Error *error = (Ewk_Frame_Load_Error *) arg;
   int errorCode = (error)? error->code: 0;
   if ( errorCode != 0 && errorCode != -999 )
     { // 0 ok, -999 request cancelled
	DBG( "page not found:, [code: %d] [domain: %s] [description: %s] [failing_url: %s] \n",
	      error->code, error->domain, error->description, error->failing_url);
	//ecore_job_add(loadNotFoundPage, (void *)this);
	return;
     }

   if (sd->auto_fitting == EINA_TRUE)
     {
	if (!sd->ewk_view_zoom_set)
	  sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");
	sd->ewk_view_zoom_set(webview, sd->zoom.min_zoom_rate, 0, 0);
     }

   // update thumbnail and minimap
   if (sd->thumbnail != NULL)
     {
	if (!sd->cairo_surface_destroy)
	  sd->cairo_surface_destroy = (void (*)(cairo_surface_t *))dlsym(cairo_handle, "cairo_surface_destroy");
	sd->cairo_surface_destroy(sd->thumbnail);
     }
   sd->thumbnail = _image_clone_get(sd, &(sd->minimap.cw), &(sd->minimap.ch));

   if (sd->tiled)
     _directional_pre_render(sd->base.self, 0, 0);

   if (sd->minimap.eo != NULL)
     {
   _minimap_update(sd->minimap.content, sd, sd->thumbnail,
	 sd->minimap.cw, sd->minimap.ch);
     }
}

static void
_smart_load_error(void* data, Evas_Object* webview, void* arg)
{
   DBG("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   char szBuffer[2048];
   if (!sd) return;

   // if error, call loadNotFoundPage
   Ewk_Frame_Load_Error *error = (Ewk_Frame_Load_Error *) arg;
   int errorCode = (error)? error->code: 0;
   if ( errorCode != 0 && errorCode != -999 )
     { // 0 ok, -999 request cancelled
	//char szStrBuffer[1024];
	//snprintf(szStrBuffer, 1024, "page not found:, [code: %d] [domain: %s] [description: %s] [failing_url: %s] \n",
	//      error->code, error->domain, error->description, error->failing_url);
	//DBG(szStrBuffer);

	//ecore_job_add(loadNotFoundPage, (void *)this);
	if (!sd->ewk_view_stop)
	  sd->ewk_view_stop = (Eina_Bool (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_stop");
	sd->ewk_view_stop(webview);

	if (!sd->ewk_view_frame_main_get)
	  sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

	if (!sd->ewk_frame_contents_set)
	  sd->ewk_frame_contents_set = (Eina_Bool (*)(Evas_Object *, const char *, size_t, const char *, const char *, const char *))dlsym(ewk_handle, "ewk_frame_contents_set");

	snprintf(szBuffer, 2048, NOT_FOUND_PAGE_HEADER "\"?%s?%s\"" NOT_FOUND_PAGE_FOOTER, error->failing_url, error->description);
	//sd->ewk_frame_contents_set(sd->ewk_view_frame_main_get(webview), szStrBuffer, 0, NULL, NULL, NULL);
	sd->ewk_frame_contents_set(error->frame, szBuffer, 0, NULL, NULL, NULL);
	return;
     }
}

static void
_smart_viewport_changed(void* data, Evas_Object* webview, void* arg)
{
   DBG("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   // check for mobile page
   int layout_w, layout_h;
   float init_zoom_rate, max_zoom_rate, min_zoom_rate;
   Eina_Bool scalable;

   if (!sd->ewk_view_viewport_get)
     sd->ewk_view_viewport_get = (void (*)(Evas_Object *, int *, int *, float *, float *, float *, Eina_Bool *))dlsym(ewk_handle, "ewk_view_viewport_get");
   sd->ewk_view_viewport_get(webview, &layout_w, &layout_h,
	 &init_zoom_rate, &max_zoom_rate, &min_zoom_rate, &scalable);

   int object_w, object_h;
   evas_object_geometry_get(webview, NULL, NULL, &object_w, &object_h);
   object_w = (object_w % 10) ? (object_w / 10 * 10 + 10) : object_w;

   // if layout width is bigger than object width, we regard current page to not the mobile page
   // for bbc.co.uk
   if (layout_w > object_w)
     {
	sd->layout.w = layout_w;
	return;
     }

   // if there is no layout_w and url does not have mobile keyword, it is the desktop site.
   if (layout_w <= 0)
     {
	if (!sd->ewk_view_uri_get)
	  sd->ewk_view_uri_get = (const char * (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_uri_get");
	const char *url = sd->ewk_view_uri_get(webview);
	if ((url && (strstr(url, "://m.") != NULL
		    || strstr(url, "://wap.") != NULL
		    || strstr(url, ".m.") != NULL
		    || strstr(url, "/mobile/i") != NULL))) // For www.bbc.co.uk/mobile/i site
	  {
	     min_zoom_rate = MIN_ZOOM_RATIO;
	     max_zoom_rate = MAX_ZOOM_RATIO;
	     scalable = 1;
	  }
	else
	  {
	     return;
	  }
     }

   // set data for mobile page
   sd->is_mobile_page = EINA_TRUE;
   _smart_page_layout_info_set(sd, MOBILE_DEFAULT_ZOOM_RATIO, min_zoom_rate, max_zoom_rate, scalable);
}

//#ifdef PROFUSION_INPUT_PATCH
/**
 * Rotaion modes
 * @see appcore_set_rotation_cb(), appcore_get_rotation_state()
 */
enum appcore_rm {
     APPCORE_RM_UNKNOWN, /**< Unknown mode */
     APPCORE_RM_PORTRAIT_NORMAL , /**< Portrait mode */
     APPCORE_RM_PORTRAIT_REVERSE , /**< Portrait upside down mode */
     APPCORE_RM_LANDSCAPE_NORMAL , /**< Left handed landscape mode */
     APPCORE_RM_LANDSCAPE_REVERSE ,  /**< Right handed landscape mode */
};
/*
static void
updateIMFOrientation( Ecore_IMF_Context *ctx )
{
   if ( !ctx )
     return;

   enum appcore_rm current_state = APPCORE_RM_UNKNOWN;
   int ret = appcore_get_rotation_state(&current_state);

   switch (current_state)
     {
      case APPCORE_RM_PORTRAIT_NORMAL:
	ecore_imf_context_input_panel_orient_set(ctx, ECORE_IMF_INPUT_PANEL_ORIENT_NONE);
	break;
      case APPCORE_RM_PORTRAIT_REVERSE:
	ecore_imf_context_input_panel_orient_set(ctx, ECORE_IMF_INPUT_PANEL_ORIENT_180);
	break;
      case APPCORE_RM_LANDSCAPE_NORMAL:
	ecore_imf_context_input_panel_orient_set(ctx, ECORE_IMF_INPUT_PANEL_ORIENT_90_CW);
	break;
      case APPCORE_RM_LANDSCAPE_REVERSE:
	ecore_imf_context_input_panel_orient_set(ctx, ECORE_IMF_INPUT_PANEL_ORIENT_90_CCW);
	break;
     }

   // call to show needed
   if ( ecore_imf_context_input_panel_state_get(ctx) == ECORE_IMF_INPUT_PANEL_STATE_SHOW )
     ecore_imf_context_input_panel_show(ctx);
}
*/

static void
_smart_input_method_changed(void* data, Evas_Object* webview, void* arg)
{
   DBG("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   if (sd->ewk_view_core_imContext_get == NULL)
     sd->ewk_view_core_imContext_get = (Ecore_IMF_Context* (*)(Evas_Object *)) dlsym(ewk_handle, "ewk_view_core_imContext_get");

   Ecore_IMF_Context* imContext = sd->ewk_view_core_imContext_get(webview);
   Eina_Bool active = (Eina_Bool)arg;
   if (active && sd->mouse_clicked)
     {
	static unsigned int lastImh = 0;//FIXME
	if (sd->ewk_view_imh_get == NULL)
	  sd->ewk_view_imh_get = (unsigned int (*)(Evas_Object *)) dlsym(ewk_handle, "ewk_view_imh_get");
	unsigned int imh = sd->ewk_view_imh_get(webview);
	if (ecore_imf_context_input_panel_state_get(imContext) != ECORE_IMF_INPUT_PANEL_STATE_SHOW || lastImh != imh)
	  {
	     lastImh = imh;
	     //currentPage->reactToInputFieldTap(view, currentPage->getLastClickInfo().x, currentPage->getLastClickInfo().y);
	     //updateIMFOrientation( imContext );
	     ecore_imf_context_input_panel_reset (imContext);
	     switch (imh)
	       {
		case EWK_IMH_TELEPHONE: ecore_imf_context_input_panel_layout_set(imContext, ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER); break;
		case EWK_IMH_NUMBER: ecore_imf_context_input_panel_layout_set(imContext, ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER); break;
		case EWK_IMH_EMAIL: ecore_imf_context_input_panel_layout_set(imContext, ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL); break;
		case EWK_IMH_URL: ecore_imf_context_input_panel_layout_set(imContext, ECORE_IMF_INPUT_PANEL_LAYOUT_URL); break;
		default: ecore_imf_context_input_panel_layout_set(imContext, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
	       }
	     DBG("ecore_imf_context_input_panel_show");
	     ecore_imf_context_focus_in(imContext);
	     ecore_imf_context_client_canvas_set(imContext, evas_object_evas_get(sd->base.self));
	     ecore_imf_context_input_panel_show (imContext);
	  }
     }
   else
     {
	DBG("ecore_imf_context_input_panel_hide");
	ecore_imf_context_input_panel_hide (imContext);
     }
}
//#endif

static void _smart_page_layout_info_set(Smart_Data *sd, float init_zoom_rate, float min_zoom_rate, float max_zoom_rate, Eina_Bool scalable)
{
   Evas_Object* webview = sd->base.self;

   int object_w, object_h;
   evas_object_geometry_get(webview, NULL, NULL, &object_w, &object_h);
   object_w = (object_w % 10) ? (object_w / 10 * 10 + 10) : object_w;

   sd->zoom.init_zoom_rate = init_zoom_rate;
   sd->layout.w = object_w / sd->zoom.init_zoom_rate;
   sd->layout.h = object_h / sd->zoom.init_zoom_rate;
   sd->zoom.scalable = scalable;
   if (scalable)
     {
	sd->zoom.min_zoom_rate = (min_zoom_rate <= sd->zoom.init_zoom_rate) ? sd->zoom.init_zoom_rate : min_zoom_rate;
	sd->zoom.max_zoom_rate = (max_zoom_rate <= sd->zoom.init_zoom_rate) ? sd->zoom.init_zoom_rate : max_zoom_rate;
	if (sd->zoom.max_zoom_rate < sd->zoom.min_zoom_rate)
	  sd->zoom.max_zoom_rate = sd->zoom.min_zoom_rate;
     }
   else
     {
	sd->zoom.min_zoom_rate = init_zoom_rate;
	sd->zoom.max_zoom_rate = init_zoom_rate;
     }
}

static void
_smart_contents_size_changed(void* data, Evas_Object* frame, void* arg)
{
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Object* webview = sd->base.self;

   Evas_Coord* size = (Evas_Coord*)arg;
   if (!size || size[0] == 0)
     return;

   _update_min_zoom_rate(sd->base.self);
}

static void
_smart_load_nonemptylayout_finished(void* data, Evas_Object* frame, void* arg)
{
   DBG("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Object* webview = sd->base.self;

   if (!sd->ewk_view_user_scalable_set)
     sd->ewk_view_user_scalable_set = (void (*)(Evas_Object *, Eina_Bool))dlsym(ewk_handle, "ewk_view_user_scalable_set");
   if (!sd->ewk_view_zoom_range_set)
     sd->ewk_view_zoom_range_set = (void (*)(Evas_Object *, float, float))dlsym(ewk_handle, "ewk_view_zoom_range_set");
   if (!sd->ewk_view_fixed_layout_size_set)
     sd->ewk_view_fixed_layout_size_set = (void (*)(Evas_Object *, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_fixed_layout_size_set");

   if (sd->use_zoom_bouncing)
     sd->ewk_view_zoom_range_set(webview, MIN_ZOOM_RATIO, MAX_ZOOM_RATIO + ZOOM_IN_BOUNCING);
   else
     sd->ewk_view_zoom_range_set(webview, MIN_ZOOM_RATIO, MAX_ZOOM_RATIO);

   // set default layout size
   int object_w, object_h;
   evas_object_geometry_get(webview, NULL, NULL, &object_w, &object_h);
   object_w = (object_w % 10) ? (object_w / 10 * 10 + 10) : object_w;
   sd->ewk_view_fixed_layout_size_set(webview, object_w, object_h);

   sd->ewk_view_user_scalable_set(webview, EINA_TRUE);

   // set zoom and layout
   if (sd->is_mobile_page)
     {
	if (!sd->ewk_view_zoom_set)
	  sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");
	if (!sd->ewk_frame_contents_size_get)
	  sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");
	if (!sd->ewk_view_uri_get)
	  sd->ewk_view_uri_get = (const char * (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_uri_get");

	sd->ewk_view_zoom_set(webview, sd->zoom.init_zoom_rate, 0, 0);
	sd->ewk_view_fixed_layout_size_set(webview, sd->layout.w, sd->layout.h);

	int content_w;
	sd->ewk_frame_contents_size_get(frame, &content_w, NULL);

	const char *url = sd->ewk_view_uri_get(webview);
	if ((content_w > sd->layout.w && !strstr(url, "docs.google.com"))
	      || strstr(url, "maps.google.com/maps/m"))
	  {
	     // set page layout info, zoom and layout again
	     _smart_page_layout_info_set(sd, 1.0f, sd->zoom.min_zoom_rate, sd->zoom.max_zoom_rate, sd->zoom.scalable);
	     sd->ewk_view_zoom_set(webview, sd->zoom.init_zoom_rate, 0, 0);
	     sd->ewk_view_fixed_layout_size_set(webview, sd->layout.w, sd->layout.h);
	  }
	if (sd->use_zoom_bouncing)
	  {
	     float min_zoom_rate = sd->zoom.min_zoom_rate * ZOOM_OUT_BOUNCING;
	     if (min_zoom_rate <= 0) min_zoom_rate = MIN_ZOOM_RATIO;
	     float max_zoom_rate = sd->zoom.max_zoom_rate * ZOOM_IN_BOUNCING;
	     sd->ewk_view_zoom_range_set(webview, min_zoom_rate, max_zoom_rate);
	  }
	else
	  {
	     sd->ewk_view_zoom_range_set(webview, sd->zoom.min_zoom_rate, sd->zoom.max_zoom_rate);
	  }

     } else {
	  sd->zoom.min_zoom_rate = MIN_ZOOM_RATIO;
	  sd->zoom.max_zoom_rate = MAX_ZOOM_RATIO;
	  if (sd->layout.w <= 0) sd->layout.w = sd->layout.default_w;
	  sd->layout.h = object_h;

	  if (!sd->ewk_view_zoom_set)
	    sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");
	  sd->ewk_view_zoom_set(webview, sd->zoom.init_zoom_rate, 0, 0);
	  sd->ewk_view_fixed_layout_size_set(webview, sd->layout.w, sd->layout.h);

	  _update_min_zoom_rate(webview);
     }

   sd->ewk_view_user_scalable_set(webview, sd->zoom.scalable);
}

static void
_smart_cb_view_created(void* data, Evas_Object* webview, void* arg)
{
   printf("%s is called\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   *((Evas_Object**)arg) = webview;
}

static void
_smart_add(Evas_Object* obj)
{
   DBG("%s\n", __func__);
   Smart_Data* sd;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   _parent_sc.sc.add(obj);

   sd->resize_calc_job = NULL;
   sd->move_calc_job = NULL;
   sd->thumbnail = NULL;
   sd->minimap.eo = NULL;
   sd->dropdown.options = NULL;
   sd->dropdown.option_cnt = 0;
   sd->use_text_selection = EINA_FALSE;
   sd->text_selection_on = EINA_FALSE;
   sd->events_feed = EINA_FALSE;
   sd->touch_obj = _elm_smart_touch_add(evas_object_evas_get(obj));
   sd->layout.default_w = DEFAULT_LAYOUT_WIDTH;

   sd->ewk_view_theme_set = (void (*)(Evas_Object *, const char *))dlsym(ewk_handle, "ewk_view_theme_set");
   sd->ewk_view_theme_set(obj, WEBKIT_EDJ);

   // set geolocation callback
   sd->ewk_set_show_geolocation_permission_dialog_callback = (void (*)(ewk_show_geolocation_permission_dialog_callback))dlsym(ewk_handle, "ewk_set_show_geolocation_permission_dialog_callback");
   sd->ewk_set_show_geolocation_permission_dialog_callback(_geolocation_permission_callback);

   sd->ewk_view_zoom_text_only_set = (Eina_Bool (*)(Evas_Object *, Eina_Bool))dlsym(ewk_handle, "ewk_view_zoom_text_only_set");
   sd->ewk_view_zoom_text_only_set(obj, EINA_FALSE);
   sd->ewk_view_zoom_cairo_scaling_set = (Eina_Bool (*)(Evas_Object *, Eina_Bool))dlsym(ewk_handle, "ewk_view_zoom_cairo_scaling_set");
   sd->ewk_view_zoom_cairo_scaling_set(obj, EINA_TRUE);
   sd->flush_and_pre_render_idler = NULL;
   sd->use_zoom_bouncing = EINA_TRUE;

#ifdef NEED_TO_REMOVE
   // TODO: temporary add the mouse callbacks until the webkit engine can receive mouse events
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN, _view_on_mouse_down, sd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _view_on_mouse_up, sd);
#endif

   evas_object_smart_callback_add(obj, "load,started", _smart_load_started, sd);
   evas_object_smart_callback_add(obj, "load,finished", _smart_load_finished, sd);
   evas_object_smart_callback_add(obj, "load,error", _smart_load_error, sd);
   evas_object_smart_callback_add(obj, "viewport,changed", _smart_viewport_changed, sd);
   evas_object_smart_callback_add(obj, "inputmethod,changed", _smart_input_method_changed, sd);

   evas_object_smart_callback_add(obj, "webview,created", _smart_cb_view_created, sd); // I need to consider more

   if (!(sd->ewk_view_frame_main_get))
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   evas_object_smart_callback_add(sd->ewk_view_frame_main_get(obj), "contents,size,changed",
	 _smart_contents_size_changed, sd);
   evas_object_smart_callback_add(sd->ewk_view_frame_main_get(obj), "load,nonemptylayout,finished",
	 _smart_load_nonemptylayout_finished, sd);

   evas_object_smart_callback_add(obj, "one,press", _smart_cb_mouse_down, sd);
   evas_object_smart_callback_add(obj, "one,release", _smart_cb_mouse_up, sd);
   evas_object_smart_callback_add(obj, "one,single,tap", _smart_cb_mouse_tap, sd);
   evas_object_smart_callback_add(obj, "one,long,press", _smart_cb_select_closest_word, sd);
   evas_object_smart_callback_add(obj, "one,double,tap", _smart_cb_smart_zoom, sd);
   evas_object_smart_callback_add(obj, "one,move,start", _smart_cb_pan_start, sd);
   evas_object_smart_callback_add(obj, "one,move", _smart_cb_pan_by, sd);
   evas_object_smart_callback_add(obj, "one,move,end", _smart_cb_pan_stop, sd);
   evas_object_smart_callback_add(obj, "two,move,start", _smart_cb_pinch_zoom_start, sd);
   evas_object_smart_callback_add(obj, "two,move", _smart_cb_pinch_zoom_move, sd);
   evas_object_smart_callback_add(obj, "two,move,end", _smart_cb_pinch_zoom_stop, sd);

   evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   _elm_smart_touch_child_set(sd->touch_obj, obj);
   _text_selection_init(obj);
}

static void
_smart_del(Evas_Object* obj)
{
   DBG("%s\n", __func__);
   INTERNAL_ENTRY;

   if (sd->minimap.eo != NULL)
     {
	evas_object_del(sd->minimap.eo);
	sd->minimap.eo = NULL;
     }

   if (sd->minimap.content != NULL)
     {
	evas_object_del(sd->minimap.content);
	sd->minimap.content = NULL;
     }

   _parent_sc.sc.del(obj);

   if (--ewk_tile_cache_ref_count == 0)
     ewk_tile_cache = NULL;
}

static void
_directional_pre_render(Evas_Object* obj, int dx, int dy)
{
   INTERNAL_ENTRY;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_visible_content_geometry_get)
     sd->ewk_frame_visible_content_geometry_get = (Eina_Bool (*)(const Evas_Object *, Eina_Bool, int *, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_visible_content_geometry_get");
   int x, y, w, h;
   sd->ewk_frame_visible_content_geometry_get(sd->ewk_view_frame_main_get(obj), false, &x, &y, &w, &h);
   DBG("visible content: (%d, %d, %d, %d)", x, y, w, h);

   typedef enum { up, down, left, right, up_left, up_right, down_left, down_right, undefined } Directions;
   Directions direction = undefined;

#ifdef USE_MAX_TUC_20MB
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float zoom = sd->ewk_view_zoom_get(obj);
#endif

   if (dx == 0 && dy <  0) direction = down;
   if (dx >  0 && dy <  0) direction = down_left;
   if (dx >  0 && dy == 0) direction = left;
   if (dx >  0 && dy >  0) direction = up_left;
   if (dx == 0 && dy >  0) direction = up;
   if (dx <  0 && dy >  0) direction = up_right;
   if (dx <  0 && dy == 0) direction = right;
   if (dx <  0 && dy <  0) direction = down_right;

#ifdef USE_MAX_TUC_20MB
   const float DIRECTION_PLAIN_CX = 2.0/zoom;
   const float DIRECTION_CROSS_CX = 1.0/zoom;
   const float DIRECTION_UNDEFINED_CX_LEVEL_1 = 0.5/zoom;
   const float DIRECTION_UNDEFINED_CX_LEVEL_2 = 0.8/zoom;
#else
   const float DIRECTION_PLAIN_CX = 1.5;
   const float DIRECTION_CROSS_CX = 0.7;
   const float DIRECTION_UNDEFINED_CX_LEVEL_1 = 0.3;
   const float DIRECTION_UNDEFINED_CX_LEVEL_2 = 0.6;
   const float DIRECTION_UNDEFINED_CX_LEVEL_3 = 0.8;
#endif

   int p_x = x, p_y = y, p_w = w, p_h = h;

   switch (direction) {
      case up:
	 DBG("Direction: up");
	 p_y = y - h * DIRECTION_PLAIN_CX;
	 p_h = h * DIRECTION_PLAIN_CX;
	 break;
      case up_right:
	 DBG("Direction: up_right");
	 p_w = w + w * DIRECTION_CROSS_CX;
	 p_y = y - h * DIRECTION_CROSS_CX;
	 p_h = h + h * DIRECTION_CROSS_CX;
	 break;
      case right:
	 DBG("Direction: right");
	 p_x = x + w;
	 p_w = w * DIRECTION_PLAIN_CX;
	 break;
      case down_right:
	 DBG("Direction: down_right");
	 p_w = w + w * DIRECTION_CROSS_CX;
	 p_h = h + h * DIRECTION_CROSS_CX;
	 break;
      case down:
	 DBG("Direction: down");
	 p_y = y + h;
	 p_h = h * DIRECTION_PLAIN_CX;
	 break;
      case down_left:
	 DBG("Direction: down_left");
	 p_x = x - w * DIRECTION_CROSS_CX;
	 p_w = w + w * DIRECTION_CROSS_CX;
	 p_h = h + h * DIRECTION_CROSS_CX;
	 break;
      case left:
	 DBG("Direction: left");
	 p_x = x - w * DIRECTION_PLAIN_CX;
	 p_w = w * DIRECTION_PLAIN_CX;
	 break;
      case up_left:
	 DBG("Direction: left_up");
	 p_x = x - w * DIRECTION_CROSS_CX;
	 p_w = w + w * DIRECTION_CROSS_CX;
	 p_y = y - h * DIRECTION_CROSS_CX;
	 p_h = h + h * DIRECTION_CROSS_CX;
	 break;
      case undefined:
	 DBG("Direction: undefined");
	 break;
      default:
	 DBG("Shouldn't happen!!");
   }

#ifndef USE_MAX_TUC_20MB
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float zoom = sd->ewk_view_zoom_get(obj);
#endif

   // cancel the previously scheduled pre-rendering
   // This makes sense especilaly for zooming operation - when user
   // finishes zooming, and pre-render for the previous zoom was
   // not finished, it doesn't make sense to continue pre-rendering for the previous zoom
   if (!sd->ewk_view_pre_render_cancel)
     sd->ewk_view_pre_render_cancel = (void (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_pre_render_cancel");
   sd->ewk_view_pre_render_cancel(obj);

   if (!sd->ewk_view_pre_render_region)
     sd->ewk_view_pre_render_region = (Eina_Bool (*)(Evas_Object *, Evas_Coord, Evas_Coord, Evas_Coord, Evas_Coord, float))dlsym(ewk_handle, "ewk_view_pre_render_region");

   if (direction != undefined)
     {
	/* Queue tiles in the direction of the last panning */
	DBG("pre rendering - directional - content: (%d, %d, %d, %d), zoom %.3f",p_x, p_y, p_w, p_h, zoom);

	sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, p_h, zoom);
	//dbg_draw_scaled_area(obj, 0, p_x, p_y, p_w, p_h);
     }
   else
     {
	DBG("pre rendering - directional - skipped");
	//dbg_draw_scaled_area(obj, 0, 0, 0, 0, 0);
     }

#ifdef USE_MAX_TUC_20MB
   int content_w=0, content_h=0;
   int center_x=0,center_y=0;
   int tmp_h=0;

   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(obj), &content_w, &content_h);

   p_w = content_w;
   p_h = content_h;

   size_t size = (size_t)roundf(p_w * zoom * p_h * zoom * 4);
   Eina_Bool  toggle = EINA_FALSE;

   while(size > (MAX_TUC*0.8))
     {
	if(toggle)
	  {
	     p_h = p_h -32;
	  }
	else
	  {
	     p_w = p_w - 32;
	     if(p_w < w)
	       {
		  p_w = w;
		  toggle = EINA_TRUE;
	       }
	  }
	size = (size_t)roundf(p_w * zoom * p_h * zoom * 4);
     }

   center_x = (int)roundf(x + w/2);
   center_y = (int)roundf(y + h/2);

   tmp_h = p_h* DIRECTION_UNDEFINED_CX_LEVEL_1;
   p_x = center_x - (int)roundf(p_w/2);
   p_y = center_y - (int)roundf(tmp_h/2);
   if(p_x < 0) p_x = 0;
   if(p_y < 0) p_y = 0;
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, tmp_h, zoom);

   tmp_h = p_h* DIRECTION_UNDEFINED_CX_LEVEL_2;
   p_x = center_x - (int)roundf(p_w/2);
   p_y = center_y - (int)roundf(tmp_h/2);
   if(p_x < 0) p_x = 0;
   if(p_y < 0) p_y = 0;
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, tmp_h, zoom);

   p_x = center_x - (int)roundf(p_w/2);
   p_y = center_y - (int)roundf(p_h/2);
   if(p_x < 0) p_x = 0;
   if(p_y < 0) p_y = 0;
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, p_h, zoom);
#else
   /* Queue tiles in a small rectangle around the viewport */
   p_x = x - w * DIRECTION_UNDEFINED_CX_LEVEL_1;
   p_y = y - h * DIRECTION_UNDEFINED_CX_LEVEL_1;
   p_w = w + 2.0 * w * DIRECTION_UNDEFINED_CX_LEVEL_1;
   p_h = h + 2.0 * h * DIRECTION_UNDEFINED_CX_LEVEL_1;
   DBG("pre rendering - small - content: (%d, %d, %d, %d), zoom %.3f", p_x, p_y, p_w, p_h, zoom);
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, p_h, zoom);
   //dbg_draw_scaled_area(obj, 1, p_x, p_y, p_w, p_h);

   /* Queue tiles in a medium rectangle around the viewport */
   p_x = x - w * DIRECTION_UNDEFINED_CX_LEVEL_2;
   p_y = y - h * DIRECTION_UNDEFINED_CX_LEVEL_2;
   p_w = w + 2.0 * w * DIRECTION_UNDEFINED_CX_LEVEL_2;
   p_h = h + 2.0 * h * DIRECTION_UNDEFINED_CX_LEVEL_2;
   DBG("pre rendering - medium - content: (%d, %d, %d, %d), zoom %.3f", p_x, p_y, p_w, p_h, zoom);
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, p_h, zoom);
   //dbg_draw_scaled_area(obj, 2, p_x, p_y, p_w, p_h);

   /* Queue tiles in a large rectangle around the viewport */
   p_x = x - w * DIRECTION_UNDEFINED_CX_LEVEL_3;
   p_y = y - h * DIRECTION_UNDEFINED_CX_LEVEL_3;
   p_w = w + 2.0 * w * DIRECTION_UNDEFINED_CX_LEVEL_3;
   p_h = h + 2.0 * h * DIRECTION_UNDEFINED_CX_LEVEL_3;
   DBG("pre rendering - large - content: (%d, %d, %d, %d), zoom %.3f", p_x, p_y, p_w, p_h, zoom);
   sd->ewk_view_pre_render_region(obj, p_x, p_y, p_w, p_h, zoom);
   //dbg_draw_scaled_area(obj, 3, p_x, p_y, p_w, p_h);
#endif

   /* Log some statistics */
   /*
      int v_w, v_h;
      evas_object_geometry_get(obj, NULL, NULL, &v_w, &v_h);
      Ewk_Tile_Unused_Cache *tuc = ewk_view_tiled_unused_cache_get(obj);
      size_t used = ewk_tile_unused_cache_used_get(tuc);
      size_t max = ewk_tile_unused_cache_max_get(tuc);
   // Will this work for non cairo scaling?
   int est = (zoomRatio*p_w * zoomRatio*p_h - v_w * v_h) * 4; // 4 bytes per pixel
   DBG("pre rendering - Cache max = %.1fMB  Cache used = %.1fMB  Estimated size of pre-render area: %.1fMB\n", 
   max/1024.0/1024.0, used/1024.0/1024.0, est/1024.0/1024.0);
   if (est > max)
   DBG("WARNING!! estimated size of pre-render are is larger than the cache size. This will result in inefficient use of cache!");
   */
}

static void
_smart_cb_mouse_down(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   if (sd->events_feed == EINA_TRUE) return;
   //Evas_Point* point = (Evas_Point*)ev;

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE) return;

#ifdef NEED_TO_REMOVE
   evas_object_focus_set(webview, EINA_TRUE);
   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_feed_focus_in)
     sd->ewk_frame_feed_focus_in = (Eina_Bool (*)(Evas_Object *))dlsym(ewk_handle, "ewk_frame_feed_focus_in");
   sd->ewk_frame_feed_focus_in(sd->ewk_view_frame_main_get(webview));
#endif

   sd->mouse_clicked = EINA_TRUE;
   _parent_sc.mouse_down((Ewk_View_Smart_Data*)sd, &sd->mouse_down_copy);

#if 0 // comment out below code until it is completed
   if (sd->bounce_horiz)
     elm_widget_drag_lock_x_set(sd->widget, EINA_TRUE);
   if (sd->bounce_vert)
     elm_widget_drag_lock_y_set(sd->widget, EINA_TRUE);
#endif
}

static void
_smart_cb_mouse_up(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   if (sd->events_feed == EINA_TRUE) return;

   Evas_Point* point = (Evas_Point*)ev;
   DBG(" argument : (%d, %d)\n", point->x, point->y);
}

static void
_smart_cb_mouse_tap(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   if (sd->events_feed == EINA_TRUE) return;

   Evas_Point* point = (Evas_Point*)ev;
   DBG(" argument : (%d, %d)\n", point->x, point->y);

   // check for video link
   int ewk_x, ewk_y;
   _coords_evas_to_ewk(webview, point->x, point->y, &ewk_x, &ewk_y);
   Eina_Bool have_link = EINA_FALSE;
   Eina_Bool have_image = EINA_FALSE;
   char *link_url = NULL, *link_text = NULL, *image_url = NULL;
   if (!sd->ewk_page_check_point)
     sd->ewk_page_check_point = (Eina_Bool (*)(Evas_Object *, int, int, Evas_Event_Mouse_Down *, Eina_Bool *, Eina_Bool *, char **, char **, char **))dlsym(ewk_handle, "ewk_page_check_point");
   sd->ewk_page_check_point(webview, ewk_x, ewk_y, &sd->mouse_down_copy,
	 &have_link, &have_image, &link_url, &link_text, &image_url);
   if (link_url) free(link_url);
   if (link_text) free(link_text);
   if (image_url) free(image_url);

   //TODO: below code is not based on open source (need to check and refactor)
   int x = 0, y = 0;
   _unzoom_position(webview, point->x, point->y, &x, &y);

   // check for input field
   if (!sd->ewk_page_check_point_for_keyboard)
     sd->ewk_page_check_point_for_keyboard = (char * (*)(Evas_Object *, int, int, Eina_Bool *))dlsym(ewk_handle, "ewk_page_check_point_for_keyboard");
   if (!sd->ewk_page_dropdown_get_options)
     sd->ewk_page_dropdown_get_options = (char ** (*)(Evas_Object *, int, int, int *, int *))dlsym(ewk_handle, "ewk_page_dropdown_get_options");

   Eina_Bool have_input_field;
   sd->ewk_page_check_point_for_keyboard(webview, x, y, &have_input_field);
   if (have_input_field == EINA_TRUE)
     {
	_zoom_to_rect(sd, point->x, point->y);

	// check whether it is radio
     }
   else if (NULL != (sd->dropdown.options = sd->ewk_page_dropdown_get_options(webview, x, y,
	       &sd->dropdown.option_cnt, &sd->dropdown.option_idx)))
     {
	Evas* evas;
	evas = evas_object_evas_get(webview);

	// TODO: we have to show list instead of discpicker
	/* below code is deprecated
	Evas_Object* discpicker = elm_discpicker_add(webview);
	if (discpicker)
	  {
	     // set items
	     int i;
	     Elm_Discpicker_Item* item;
	     for (i = 0; i < sd->dropdown.option_cnt; i++)
	       {
		  item = elm_discpicker_item_append(discpicker, sd->dropdown.options[i], NULL, NULL);
		  if (i == sd->dropdown.option_idx)
		    {
		       elm_discpicker_item_selected_set(item);
		    }
	       }

	     // selected callback
	     void discpicker_selected_cb(void* data, Evas_Object* obj, void* event_info)
	       {
		  Smart_Data* sd = (Smart_Data *)data;
		  if (!sd) return;
		  Evas_Object* webview = sd->base.self;

		  int x = 0, y = 0;
		  Evas_Point* point = &sd->mouse_up_copy.output;
		  _unzoom_position(webview, point->x, point->y, &x, &y);

		  Elm_Discpicker_Item* item = event_info;
		  const char *selected_label = elm_discpicker_item_label_get(item);
		  int selected_index;
		  for (selected_index = 0; selected_index < sd->dropdown.option_cnt; selected_index++)
		    {
		       if (!strcmp(selected_label, sd->dropdown.options[selected_index]))
			 {
			    break;
			 }
		    }
		  printf("<< selected [%d | %s] >>\n", selected_index, selected_label);
		  if (!sd->ewk_page_dropdown_set_current_index)
		    sd->ewk_page_dropdown_set_current_index = (Eina_Bool (*)(Evas_Object *, int, int, int))dlsym(ewk_handle, "ewk_page_dropdown_set_current_index");
		  sd->ewk_page_dropdown_set_current_index(webview, x, y, selected_index);
		  //evas_object_del(obj);
	       }

	     // show discpicker
	     evas_object_smart_callback_add(discpicker, "selected", discpicker_selected_cb, sd);
	     elm_discpicker_row_height_set(discpicker, 80);
	     evas_object_resize(discpicker, 480, 400);
	     evas_object_move(discpicker, 0, 400);
	     evas_object_show(discpicker);
	  }
	  */
     }

   if (sd->use_text_selection  == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	_smart_cb_unselect_closest_word(sd, webview, NULL);
	return;
     }

   _parent_sc.mouse_up((Ewk_View_Smart_Data*)sd, &sd->mouse_up_copy);
   sd->mouse_clicked = EINA_FALSE;
}

static void
_smart_cb_pan_start(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   Evas_Point* point = (Evas_Point*)ev;

   if (sd->events_feed == EINA_TRUE) return;

   sd->pan_s = *point;
   sd->on_panning = EINA_TRUE;

   if (sd->use_text_selection  == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	if (_text_selection_handle_pressed(sd, point->x, point->y))
	  _elm_smart_touch_is_one_drag_mode_enable(sd->touch_obj, EINA_FALSE);
     }

   _suspend_all(sd, EINA_FALSE);

   sd->locked_dx = 0;
   sd->locked_dy = 0;
}

static void
_smart_cb_pan_by(void* data, Evas_Object* webview, void* ev)
{
   //DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   Evas_Point* point = (Evas_Point*)ev;

   if (sd->events_feed == EINA_TRUE)
     {
	Evas* evas = evas_object_evas_get(webview);
	Evas_Modifier *modifiers = (Evas_Modifier *)evas_key_modifier_get(evas);
	Evas_Lock *locks = (Evas_Lock *)evas_key_lock_get(evas);

	Evas_Event_Mouse_Move event_move;
	event_move.buttons = 1;
	event_move.cur.output.x = point->x;
	event_move.cur.output.y = point->y;
	event_move.cur.canvas.x = point->x;
	event_move.cur.canvas.y = point->y;
	event_move.data = NULL;
	event_move.modifiers = modifiers;
	event_move.locks = locks;
	event_move.timestamp = ecore_loop_time_get();
	event_move.event_flags = EVAS_EVENT_FLAG_NONE;
	event_move.dev = NULL;

	_parent_sc.mouse_move((Ewk_View_Smart_Data*)sd, &event_move);
	return;
     }
   if (sd->on_panning == EINA_FALSE) return;

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	if (sd->text_selection.front_handle_moving == EINA_TRUE
	      || sd->text_selection.back_handle_moving == EINA_TRUE)
	  {
	     _text_selection_update_position(sd, point->x, point->y);
	     return;
	  }
     }

   if (!sd->ewk_frame_scroll_pos_get)
     sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");

   int dx = sd->pan_s.x - point->x;
   int dy = sd->pan_s.y - point->y;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   int old_x, old_y;
   sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(webview), &old_x, &old_y);

   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");

   int content_w, content_h;
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(webview), &content_w, &content_h);
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float zoom = sd->ewk_view_zoom_get(webview);
   content_w *= zoom;
   content_h *= zoom;
   DBG("<< ========content [%d, %d] new pos [%d, %d] >>\n", content_w, content_h, old_x + dx, old_y + dy);

#if 0 // comment out below code until it is completed
   Eina_Bool locked = EINA_FALSE;
   if (!elm_widget_drag_lock_x_get(sd->widget))
     {
	if ((old_x + dx) >= 0 && (old_x + dx) <=content_w)
	  elm_widget_drag_lock_x_set(sd->widget, EINA_TRUE);
	else if ((sd->locked_dx > 0 && (sd->locked_dx + dx) <= 0)
	      || (sd->locked_dx < 0 && (sd->locked_dx + dx) >= 0))
	  {
	     elm_widget_drag_lock_x_set(sd->widget, EINA_TRUE);
	     DBG("===============<< widget x lock >>\n");
	     dx += sd->locked_dx;
	  }
	else
	  {
	     sd->locked_dx += dx;
	     locked = EINA_TRUE;
	  }
     }
   if (!elm_widget_drag_lock_y_get(sd->widget))
     {
	if ((old_y + dy) >= 0 && (old_y + dy) <= content_h)
	  elm_widget_drag_lock_y_set(sd->widget, EINA_TRUE);
	else if ((sd->locked_dy > 0 && (sd->locked_dy + dy) <= 0)
	      || (sd->locked_dy < 0 && (sd->locked_dy + dy) >= 0))
	  {
	     elm_widget_drag_lock_y_set(sd->widget, EINA_TRUE);
	     DBG("===============<< widget y lock >>\n");
	     dy += sd->locked_dy;
	  }
	else
	  {
	     sd->locked_dy += dy;
	     locked = EINA_TRUE;
	  }
     }

   if (locked) return;
#endif

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_scroll_add)
     sd->ewk_frame_scroll_add = (Eina_Bool (*)(Evas_Object *, int, int))dlsym(ewk_handle, "ewk_frame_scroll_add");
   sd->ewk_frame_scroll_add(sd->ewk_view_frame_main_get(webview), dx, dy);

   _minimap_update(sd->minimap.content, sd, sd->thumbnail,
	 sd->minimap.cw, sd->minimap.ch);
   sd->pan_s = *point;

   int new_x, new_y;
   sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(webview), &new_x, &new_y);

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     _text_selection_move_by(sd, old_x - new_x, old_y - new_y);

#if 0 // comment out below code until it is completed
   if (!sd->bounce_horiz &&
	 (dx && elm_widget_drag_lock_x_get(sd->widget) && (old_x == new_x)))
     {
	sd->locked_dx = dx - (old_x - new_x);
	elm_widget_drag_lock_x_set(sd->widget, EINA_FALSE);
	DBG("===============<< widget x unlock >>\n");
     }

   if (!sd->bounce_vert &&
	 (dy && elm_widget_drag_lock_y_get(sd->widget) && (old_y == new_y)))
     {
	sd->locked_dy = dy - (old_y - new_y);
	elm_widget_drag_lock_y_set(sd->widget, EINA_FALSE);
	DBG("===============<< widget y unlock >>\n");
     }
#endif
}

static void
_smart_cb_pan_stop(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   if (sd->events_feed == EINA_TRUE) return;

   Evas_Point* point = (Evas_Point*)ev;
   sd->on_panning = EINA_FALSE;

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	if (sd->text_selection.front_handle_moving == EINA_TRUE
	      || sd->text_selection.back_handle_moving == EINA_TRUE)
	  _elm_smart_touch_is_one_drag_mode_enable(sd->touch_obj, EINA_TRUE);
	sd->text_selection.front_handle_moving = EINA_FALSE;
	sd->text_selection.back_handle_moving = EINA_FALSE;
     }

   _resume_all(sd, EINA_FALSE);

   if (sd->tiled)
     {
	if (!sd->ewk_view_tiled_unused_cache_get)
	  sd->ewk_view_tiled_unused_cache_get = (Ewk_Tile_Unused_Cache *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_get");
	if (!sd->ewk_tile_unused_cache_used_get)
	  sd->ewk_tile_unused_cache_used_get = (size_t (*)(const Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_used_get");
	if (!sd->ewk_tile_unused_cache_max_get)
	  sd->ewk_tile_unused_cache_max_get = (size_t (*)(const Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_max_get");
	Ewk_Tile_Unused_Cache *tuc = sd->ewk_view_tiled_unused_cache_get(webview);
	size_t used = sd->ewk_tile_unused_cache_used_get(tuc);
	size_t max = sd->ewk_tile_unused_cache_max_get(tuc);
	DBG("[%s] max = %d  used = %d \n", __func__, max, used);
	if (used > max)
	  {
	     if (!sd->ewk_tile_unused_cache_auto_flush)
	       sd->ewk_tile_unused_cache_auto_flush = (void (*)(Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_auto_flush");
	     sd->ewk_tile_unused_cache_auto_flush(tuc);
	  }
	_directional_pre_render(webview,
	      (sd->mouse_down_copy.canvas.x - point->x), (sd->mouse_down_copy.canvas.y - point->y));
     }

#if 0 // comment out below code until it is completed
   if (!sd->bounce_horiz && elm_widget_drag_lock_x_get(sd->widget))
     {
	DBG("==============<< widget x unlock >>\n");
	elm_widget_drag_lock_x_set(sd->widget, EINA_FALSE);
     }

   if (!sd->bounce_vert && elm_widget_drag_lock_y_get(sd->widget))
     {
	DBG("==============<< widget y unlock >>\n");
	elm_widget_drag_lock_y_set(sd->widget, EINA_FALSE);
     }
#endif
}

static void
_smart_cb_select_closest_word(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;
   if (sd->events_feed == EINA_TRUE) return;

   Evas_Point* point = (Evas_Point*)ev;

   if (sd->use_text_selection == EINA_FALSE) return;

   int x, y;
   _coords_evas_to_ewk(webview, point->x, point->y, &x, &y);

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_select_closest_word)
     sd->ewk_frame_select_closest_word = (Eina_Bool (*)(Evas_Object *, int, int, int *, int *, int *, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_select_closest_word");
   int tx, ty, th, bx, by, bh;
   Eina_Bool ret = sd->ewk_frame_select_closest_word(sd->ewk_view_frame_main_get(webview), x, y,
	 &tx, &ty, &th, &bx, &by, &bh);
   if (ret)
     {
	_coords_ewk_to_evas(webview, tx, ty, &tx, &ty);
	_coords_ewk_to_evas(webview, bx, by, &bx, &by);
	_text_selection_show();
	_text_selection_set_front_info(sd, tx, ty, th);
	_text_selection_set_back_info(sd, bx, by, bh);
	sd->text_selection_on = EINA_TRUE;
     }
}

static void
_smart_cb_unselect_closest_word(void* data, Evas_Object* webview, void* ev)
{
   DBG("%s\n", __func__);
   Smart_Data* sd = (Smart_Data *)data;
   if (!sd) return;

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	_text_selection_hide(sd);
	if (!sd->ewk_view_select_none)
	  sd->ewk_view_select_none = (Eina_Bool (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_select_none");
	sd->ewk_view_select_none(webview);
	sd->text_selection_on = EINA_FALSE;
     }
}

// zoom
static const int ZOOM_STEP_TRESHOLD = 20;
static const float ZOOM_STEP_PER_PIXEL = 0.005f;

#define ZOOM_FRAMERATE 60
#define N_COSINE 18
static const float cosine[N_COSINE] =
{ 1.0f, 0.99f, 0.96f, 0.93f, 0.88f, 0.82f, 0.75f, 0.67f, 0.59f, 0.5f,
   0.41f, 0.33f, 0.25f, 0.18f, 0.12f, 0.07f, 0.01f, 0.0f };
static int smart_zoom_index = N_COSINE - 1;

#define INPUT_LOCATION_X 20
#define INPUT_LOCATION_Y 50
#define INPUT_ZOOM_RATIO 2.5

static void
_suspend_all(Smart_Data *sd, Eina_Bool hidePlugin)
{
   Evas_Object *webview = sd->base.self;

   // javascript suspend
   if (!sd->ewk_view_javascript_suspend)
     sd->ewk_view_javascript_suspend = (void (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_javascript_suspend");
   sd->ewk_view_javascript_suspend(webview);

   // render suspend
   if (!sd->ewk_view_disable_render)
     sd->ewk_view_disable_render = (Eina_Bool (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_disable_render");
   sd->ewk_view_disable_render(webview);

   // plugin suspend
   if (!sd->ewk_view_setting_enable_plugins_get)
     sd->ewk_view_setting_enable_plugins_get = (Eina_Bool (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_setting_enable_plugins_get");
   if (sd->ewk_view_setting_enable_plugins_get(webview))
     {
	if (!sd->ewk_view_pause_and_or_hide_plugins)
	  sd->ewk_view_pause_and_or_hide_plugins = (void (*)(Evas_Object *, Eina_Bool, Eina_Bool))dlsym(ewk_handle, "ewk_view_pause_and_or_hide_plugins");
	sd->ewk_view_pause_and_or_hide_plugins(webview, EINA_FALSE, hidePlugin);
     }

   // cancel pre-render
   if (sd->tiled)
     {
	if (!sd->ewk_view_pre_render_cancel)
	  sd->ewk_view_pre_render_cancel = (void (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_pre_render_cancel");
	sd->ewk_view_pre_render_cancel(webview);
     }

   // network suspend
   if (!sd->ewk_view_suspend_request)
     sd->ewk_view_suspend_request = (Eina_Bool (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_suspend_request");
   sd->ewk_view_suspend_request(webview); // suspend network loading

}

static void
_resume_all(Smart_Data *sd, Eina_Bool hidePlugin)
{
   Evas_Object *webview = sd->base.self;

   // js resume
   if (!sd->ewk_view_javascript_resume)
     sd->ewk_view_javascript_resume = (void (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_javascript_resume");
   sd->ewk_view_javascript_resume(webview);

   // render resume
   if (sd->tiled)
     {
	if (!sd->ewk_view_enable_render)
	  sd->ewk_view_enable_render = (Eina_Bool (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_enable_render");
	sd->ewk_view_enable_render(webview);
     }

   // plugin resume
   if (!sd->ewk_view_pause_and_or_hide_plugins)
     sd->ewk_view_pause_and_or_hide_plugins = (void (*)(Evas_Object *, Eina_Bool, Eina_Bool))dlsym(ewk_handle, "ewk_view_pause_and_or_hide_plugins");
   sd->ewk_view_pause_and_or_hide_plugins(webview, EINA_FALSE, hidePlugin);

   // network resume
   if (!sd->ewk_view_resume_request)
     sd->ewk_view_resume_request = (Eina_Bool (*)(Evas_Object *))dlsym(ewk_handle, "ewk_view_resume_request");
   sd->ewk_view_resume_request(webview);
}

static void
_zoom_start(Smart_Data* sd, int centerX, int centerY, int distance)
{
   DBG("%s\n", __func__);
   sd->zoom.basis.x = centerX;
   sd->zoom.basis.y = centerY;
   sd->zoom.finger_distance = distance;
   sd->zoom.zooming_level = 0;
   sd->on_zooming = EINA_TRUE;
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   sd->zoom.zoom_rate_at_start = sd->ewk_view_zoom_get(sd->base.self);
   sd->zoom.zooming_rate = sd->zoom.zoom_rate_at_start;

   _suspend_all(sd, EINA_TRUE);

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     _text_selection_hide(sd);
}

static void
_zoom_move(Smart_Data* sd, int centerX, int centerY, int distance)
{
   if (sd->on_zooming == EINA_FALSE) return;
   //DBG("%s\n", __func__);

   int zoom_distance = distance - sd->zoom.finger_distance;

   if (zoom_distance != sd->zoom.zooming_level)
     {
	float zoom_ratio;

	if (sd->use_zoom_bouncing)
	  {
	     float min_zoom_rate = sd->zoom.min_zoom_rate * ZOOM_OUT_BOUNCING;
	     if (min_zoom_rate <= 0) min_zoom_rate = MIN_ZOOM_RATIO;
	     float max_zoom_rate = sd->zoom.max_zoom_rate * ZOOM_IN_BOUNCING;

	     if (sd->zoom.zooming_rate < sd->zoom.min_zoom_rate)
	       {
		  float step = (sd->zoom.min_zoom_rate - min_zoom_rate) / (float)BOUNCING_DISTANCE;
		  zoom_ratio = sd->zoom.zooming_rate + (zoom_distance - sd->zoom.zooming_level) * step;
	       }
	     else if (sd->zoom.zooming_rate > sd->zoom.max_zoom_rate)
	       {
		  float step = (max_zoom_rate - sd->zoom.max_zoom_rate) / (float)BOUNCING_DISTANCE;
		  zoom_ratio = sd->zoom.zooming_rate + (zoom_distance - sd->zoom.zooming_level) * step;
	       }
	     else
	       {
		  zoom_ratio = sd->zoom.zoom_rate_at_start + zoom_distance * ZOOM_STEP_PER_PIXEL;
	       }

	     if (zoom_ratio < min_zoom_rate)
	       zoom_ratio = min_zoom_rate;
	     if (zoom_ratio > max_zoom_rate)
	       zoom_ratio = max_zoom_rate;
	  }
	else
	  {
	     zoom_ratio = sd->zoom.zoom_rate_at_start + zoom_distance * ZOOM_STEP_PER_PIXEL;
	     if (zoom_ratio < sd->zoom.min_zoom_rate)
	       zoom_ratio = sd->zoom.min_zoom_rate;
	     if (zoom_ratio > sd->zoom.max_zoom_rate)
	       zoom_ratio = sd->zoom.max_zoom_rate;
	  }
	sd->zoom.zooming_level = zoom_distance;
	sd->zoom.zooming_rate = zoom_ratio;

	//printf("new zoom : %f, (%d, %d)\n", zoom_ratio, centerX, centerY);
	if (!sd->ewk_view_zoom_weak_set)
	  sd->ewk_view_zoom_weak_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_weak_set");
	sd->ewk_view_zoom_weak_set(sd->base.self, zoom_ratio, sd->zoom.basis.x, sd->zoom.basis.y);
	DBG("<< zoom weak set [%f] >>\n", zoom_ratio);
     }
}

static void
_zoom_stop(Smart_Data* sd)
{
   sd->on_zooming = EINA_FALSE;
   DBG("%s ( %d )\n", __func__, sd->zoom.zooming_level);
   if (sd->zoom.zooming_level == 0) return;

   sd->zoom.zoom_rate_to_set = sd->zoom.zooming_rate;
   if (sd->zoom.zoom_rate_to_set < sd->zoom.min_zoom_rate)
     sd->zoom.zoom_rate_to_set = sd->zoom.min_zoom_rate;
   if (sd->zoom.zoom_rate_to_set > sd->zoom.max_zoom_rate)
     sd->zoom.zoom_rate_to_set = sd->zoom.max_zoom_rate;
   if (sd->use_zoom_bouncing
	 && (sd->zoom.zoom_rate_to_set != sd->zoom.zooming_rate))
     {
	sd->zoom.zoom_rate_at_start = sd->zoom.zooming_rate;
	smart_zoom_index = N_COSINE - 1;
	ecore_animator_frametime_set(1.0 / ZOOM_FRAMERATE);
	sd->smart_zoom_animator = ecore_animator_add(_smart_zoom_animator, sd);
     }
   else
     {
	if (!sd->ewk_view_zoom_set)
	  sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");
	sd->ewk_view_zoom_set(sd->base.self, sd->zoom.zoom_rate_to_set, sd->zoom.basis.x, sd->zoom.basis.y);
     }
   DBG("<< zoom set [%f] >>\n", sd->zoom.zooming_rate);

   _resume_all(sd, EINA_FALSE);

   if (sd->tiled)
     {
	if (!sd->ewk_view_tiled_unused_cache_get)
	  sd->ewk_view_tiled_unused_cache_get = (Ewk_Tile_Unused_Cache *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_get");
	Ewk_Tile_Unused_Cache* ewk_tile_cache = sd->ewk_view_tiled_unused_cache_get(sd->base.self);
	if (!sd->ewk_tile_unused_cache_auto_flush)
	  sd->ewk_tile_unused_cache_auto_flush = (void (*)(Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_auto_flush");
	sd->ewk_tile_unused_cache_auto_flush(ewk_tile_cache);
	_directional_pre_render(sd->base.self, 0, 0);
     }

   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     {
	if (!sd->ewk_view_frame_main_get)
	  sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
	if (!sd->ewk_frame_selection_handlers_get)
	  sd->ewk_frame_selection_handlers_get = (Eina_Bool (*)(Evas_Object *, int *, int *, int *, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_selection_handlers_get");
	int tx, ty, th, bx, by, bh;
	sd->ewk_frame_selection_handlers_get(sd->ewk_view_frame_main_get(sd->base.self), &tx, &ty, &th, &bx, &by, &bh);
	_coords_ewk_to_evas(sd->base.self, tx, ty, &tx, &ty);
	_coords_ewk_to_evas(sd->base.self, bx, by, &bx, &by);
	_text_selection_show();
	_text_selection_set_front_info(sd, tx, ty, th);
	_text_selection_set_back_info(sd, bx, by, bh);
     }
}

static void
_adjust_to_contents_boundary(Evas_Object* obj, int* to_x, int* to_y,
      int from_x, int from_y, float new_zoom_rate)
{
   INTERNAL_ENTRY;
   // get view's geometry
   int view_x, view_y, view_w, view_h;
   evas_object_geometry_get(obj, &view_x, &view_y, &view_w, &view_h);

   // get contentsSize
   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");

   int contents_w, contents_h;
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(obj), &contents_w, &contents_h);
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float current_zoom_rate = sd->ewk_view_zoom_get(obj);
   if (!sd->ewk_view_zoom_cairo_scaling_get)
     sd->ewk_view_zoom_cairo_scaling_get = (Eina_Bool (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_cairo_scaling_get");
   if (sd->ewk_view_zoom_cairo_scaling_get(obj))
     {
	contents_w *= current_zoom_rate;
	contents_h *= current_zoom_rate;
     }

   // check boundary - should not exceed the left, right, top and bottom of contents after zoom
   float zoom_step = new_zoom_rate / current_zoom_rate;
   int ewk_from_x, ewk_from_y;
   _coords_evas_to_ewk(obj, from_x, from_y, &ewk_from_x, &ewk_from_y);
   int contents_left = ewk_from_x * zoom_step; // left contents size of from
   int contents_right = contents_w * zoom_step - contents_left; // right contents size of from
   int screen_left = (*to_x) - view_x;
   int screen_right = view_w - screen_left;
   if (contents_left < screen_left)
     (*to_x) -= (screen_left - contents_left);
   else if (contents_right < screen_right)
     (*to_x) += (screen_right - contents_right);
   int contents_top = ewk_from_y * zoom_step; // top contents size of from
   int contents_bottom = contents_h * zoom_step - contents_top; // bottom contents size of from
   int screen_top = (*to_y) - view_y;
   int screen_bottom = view_h - screen_top;
   if (contents_top < screen_top)
     (*to_y) -= (screen_top - contents_top);
   else if (contents_bottom < screen_bottom)
     (*to_y) += (screen_bottom - contents_bottom);
}

static int
_smart_zoom_animator(void* data)
{
   Smart_Data* sd = (Smart_Data*)data;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   // stop
   if (smart_zoom_index < 0)
     {
	if (!sd->ewk_view_zoom_set)
	  sd->ewk_view_zoom_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_set");
	sd->ewk_view_zoom_set(sd->base.self, sd->zoom.zoom_rate_to_set,
	      sd->zoom.basis.x, sd->zoom.basis.y);
	if (sd->smart_zoom_animator)
	  {
	     ecore_animator_del(sd->smart_zoom_animator);
	     sd->smart_zoom_animator = NULL;
	  }

	_elm_smart_touch_start(sd->touch_obj);

	_resume_all(sd, EINA_FALSE);

	if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
	  {
	     if (!sd->ewk_frame_selection_handlers_get)
	       sd->ewk_frame_selection_handlers_get = (Eina_Bool (*)(Evas_Object *, int *, int *, int *, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_selection_handlers_get");
	     int tx, ty, th, bx, by, bh;
	     sd->ewk_frame_selection_handlers_get(sd->ewk_view_frame_main_get(sd->base.self),
		   &tx, &ty, &th, &bx, &by, &bh);
	     _coords_ewk_to_evas(sd->base.self, tx, ty, &tx, &ty);
	     _coords_ewk_to_evas(sd->base.self, bx, by, &bx, &by);
	     _text_selection_show();
	     _text_selection_set_front_info(sd, tx, ty, th);
	     _text_selection_set_back_info(sd, bx, by, bh);
	  }

	return ECORE_CALLBACK_CANCEL;
     }

   if (sd->zoom.zoom_rate_at_start != sd->zoom.zoom_rate_to_set)
     {
	// weak zoom
	float zoom_rate = sd->zoom.zoom_rate_at_start
	   + ((sd->zoom.zoom_rate_to_set - sd->zoom.zoom_rate_at_start) * cosine[smart_zoom_index]);
	if (!sd->ewk_view_zoom_weak_set)
	  sd->ewk_view_zoom_weak_set = (Eina_Bool (*)(Evas_Object *, float, Evas_Coord, Evas_Coord))dlsym(ewk_handle, "ewk_view_zoom_weak_set");
	if (zoom_rate <= sd->zoom.min_zoom_rate)
	  {
	     if (!sd->ewk_frame_scroll_pos_get)
	       sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");
	     if (!sd->ewk_view_zoom_get)
	       sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
	     int scroll_x, scroll_y;
	     sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(sd->base.self), &scroll_x, &scroll_y);
	     float current_zoom_rate = sd->ewk_view_zoom_get(sd->base.self);
	     int center_x = (scroll_x * sd->zoom.zoom_rate_to_set * current_zoom_rate)
		            / (current_zoom_rate - sd->zoom.zoom_rate_to_set);
	     int center_y = (scroll_y * sd->zoom.zoom_rate_to_set * current_zoom_rate)
		            / (current_zoom_rate - sd->zoom.zoom_rate_to_set);

	     int basis_x = sd->zoom.basis.x + (center_x - sd->zoom.basis.x) * cosine[smart_zoom_index];
	     int basis_y = sd->zoom.basis.y + (center_y - sd->zoom.basis.y) * cosine[smart_zoom_index];
	     sd->ewk_view_zoom_weak_set(sd->base.self, zoom_rate, basis_x, basis_y);
	     smart_zoom_index--; // in order to make zoom bouncing more faster
	  }
	if (zoom_rate >= sd->zoom.max_zoom_rate)
	  {
	     sd->ewk_view_zoom_weak_set(sd->base.self, zoom_rate, sd->zoom.basis.x, sd->zoom.basis.y);
	     smart_zoom_index--; // in order to make zoom bouncing more faster
	  }
	else
	  {
	     sd->ewk_view_zoom_weak_set(sd->base.self, zoom_rate, sd->zoom.basis.x, sd->zoom.basis.y);
	  }
     } else {
	  if (!sd->ewk_frame_scroll_pos_get)
	    sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");
	  // save old scroll positions
	  int current_scroll_x, current_scroll_y;
	  sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(sd->base.self), &current_scroll_x, &current_scroll_y);

	  // get to set position
	  int to_set_x = sd->zoom.scroll_at_start.x
	     + (sd->zoom.scroll_to_set.x - sd->zoom.scroll_at_start.x) * cosine[smart_zoom_index];
	  int to_set_y = sd->zoom.scroll_at_start.y
	     + (sd->zoom.scroll_to_set.y - sd->zoom.scroll_at_start.y) * cosine[smart_zoom_index];

	  if (!sd->ewk_frame_scroll_add)
	    sd->ewk_frame_scroll_add = (Eina_Bool (*)(Evas_Object *, int, int))dlsym(ewk_handle, "ewk_frame_scroll_add");
	  // scroll
	  sd->ewk_frame_scroll_add(sd->ewk_view_frame_main_get(sd->base.self),
		to_set_x - current_scroll_x, to_set_y - current_scroll_y);
     }
   smart_zoom_index--;

   return ECORE_CALLBACK_RENEW;
}

static void
_smart_cb_pinch_zoom_start(void* data, Evas_Object* webview, void* event_info)
{
   //DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Point* arr = (Evas_Point*) event_info;
   int centerX = (arr[0].x + arr[1].x) / 2;
   int centerY = (arr[0].y + arr[1].y) / 2;
   int dx = arr[0].x - arr[1].x;
   int dy = arr[0].y - arr[1].y;
   int distance = sqrt((double)(dx * dx + dy * dy));
   _zoom_start(sd, centerX, centerY, distance);
}

static void
_smart_cb_pinch_zoom_move(void* data, Evas_Object* webview, void* event_info)
{
   //DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Point* arr = (Evas_Point*) event_info;
   int centerX = (arr[0].x + arr[1].x) / 2;
   int centerY = (arr[0].y + arr[1].y) / 2;
   int dx = arr[0].x - arr[1].x;
   int dy = arr[0].y - arr[1].y;
   int distance = sqrt((double)(dx * dx + dy * dy));
   _zoom_move(sd, centerX, centerY, distance);
}

static void
_smart_cb_pinch_zoom_stop(void* data, Evas_Object* webview, void* event_info)
{
   //DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   _zoom_stop(sd);
   _minimap_update(sd->minimap.content, sd, sd->thumbnail, sd->minimap.cw, sd->minimap.ch);

   if (sd->tiled)
     {
	if (!sd->ewk_view_tiled_unused_cache_get)
	  sd->ewk_view_tiled_unused_cache_get = (Ewk_Tile_Unused_Cache *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_tiled_unused_cache_get");
	Ewk_Tile_Unused_Cache *tuc = sd->ewk_view_tiled_unused_cache_get(webview);
	if (!sd->ewk_tile_unused_cache_used_get)
	  sd->ewk_tile_unused_cache_used_get = (size_t (*)(const Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_used_get");
	size_t used = sd->ewk_tile_unused_cache_used_get(tuc);
	if (!sd->ewk_tile_unused_cache_max_get)
	  sd->ewk_tile_unused_cache_max_get = (size_t (*)(const Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_max_get");
	size_t max = sd->ewk_tile_unused_cache_max_get(tuc);
	DBG("[%s] max = %d  used = %d \n", __func__, max, used);
	if (used > max)
	  {
	     if (!sd->ewk_tile_unused_cache_auto_flush)
	       sd->ewk_tile_unused_cache_auto_flush = (void (*)(Ewk_Tile_Unused_Cache *))dlsym(ewk_handle, "ewk_tile_unused_cache_auto_flush");
	     sd->ewk_tile_unused_cache_auto_flush(tuc);
	  }
     }
}

static void
_smart_cb_vertical_zoom_start(void* data, Evas_Object* webview, void* event_info)
{
   DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Point* arr = (Evas_Point*) event_info;
   int centerX = (arr[0].x + arr[1].x) / 2;
   int centerY = (arr[0].y + arr[1].y) / 2;
   //int dx = arr[0].x - arr[1].x;
   //int dy = arr[0].y - arr[1].y;
   //int distance = sqrt((double)(dx * dx + dy * dy));
   _zoom_start(sd, centerX, centerY, centerY);
}

static void
_smart_cb_vertical_zoom_move(void* data, Evas_Object* webview, void* event_info)
{
   DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   Evas_Point* arr = (Evas_Point*) event_info;
   int centerX = (arr[0].x + arr[1].x) / 2;
   int centerY = (arr[0].y + arr[1].y) / 2;
   //int dx = arr[0].x - arr[1].x;
   //int dy = arr[0].y - arr[1].y;
   //int distance = centerY - sd->zoom.cy;
   _zoom_move(sd, centerX, centerY, centerY);
}

static void
_smart_cb_vertical_zoom_stop(void* data, Evas_Object* webview, void* event_info)
{
   DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;

   _zoom_stop(sd);
   _minimap_update(sd->minimap.content, sd, sd->thumbnail, sd->minimap.cw, sd->minimap.ch);
}

static void
_smart_cb_smart_zoom(void* data, Evas_Object* webview, void* event_info)
{
   DBG("%s\n", __func__);
   Smart_Data *sd = (Smart_Data *)data;
   if (!sd) return;
   Evas_Point* point = (Evas_Point*)event_info;

   if (sd->events_feed == EINA_TRUE) return;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   _elm_smart_touch_stop(sd->touch_obj);

   // get rect
   int ewk_x = 0, ewk_y = 0;
   Eina_Rectangle rect;
   _coords_evas_to_ewk(webview, point->x, point->y, &ewk_x, &ewk_y);
   if (!sd->ewk_view_get_smart_zoom_rect)
     sd->ewk_view_get_smart_zoom_rect = (Eina_Bool (*)(Evas_Object *, int, int, const Evas_Event_Mouse_Up *, Eina_Rectangle *))dlsym(ewk_handle, "ewk_view_get_smart_zoom_rect");
   sd->ewk_view_get_smart_zoom_rect(webview, ewk_x, ewk_y, &sd->mouse_up_copy, &rect);

   // calculate zoom_rate and center of rect
   int view_x, view_y, view_w, view_h;
   evas_object_geometry_get(webview, &view_x, &view_y, &view_w, &view_h);
   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float current_zoom_rate = sd->ewk_view_zoom_get(webview);
   float zoom_rate;
   int rect_center_x, rect_center_y;
   if (rect.w)
     {
	zoom_rate = current_zoom_rate * (float)view_w / (float)rect.w;
	_coords_ewk_to_evas(webview, rect.x + (rect.w >> 1), rect.y + (rect.h >> 1), &rect_center_x, &rect_center_y);
	if ((rect.h / current_zoom_rate) * zoom_rate > view_h)
	  {
	     rect_center_y = point->y;
	  }
	// check zoom rate
	if (zoom_rate < sd->zoom.min_zoom_rate)
	  zoom_rate = sd->zoom.min_zoom_rate;
	if (zoom_rate > sd->zoom.max_zoom_rate)
	  zoom_rate = sd->zoom.max_zoom_rate;
	if (zoom_rate == current_zoom_rate)
	  zoom_rate = sd->zoom.min_zoom_rate;
     } else {
	  zoom_rate = sd->zoom.min_zoom_rate;
	  rect_center_x = point->x;
	  rect_center_y = point->y;
     }

   // set zooming data
   float zoom_step = zoom_rate / current_zoom_rate;
   int center_x = view_x + (view_w >> 1);
   int center_y = view_y + (view_h >> 1);

   _adjust_to_contents_boundary(webview, &center_x, &center_y, rect_center_x, rect_center_y, zoom_rate);

   // set data for smart zoom
   sd->zoom.basis.x = (center_x - zoom_step * rect_center_x) / (1 - zoom_step);
   sd->zoom.basis.y = (center_y - zoom_step * rect_center_y) / (1 - zoom_step) - view_y;
   sd->zoom.zoom_rate_at_start = current_zoom_rate;
   sd->zoom.zoom_rate_to_set = zoom_rate;
   smart_zoom_index = N_COSINE - 1;

   _suspend_all(sd, EINA_TRUE);

   // run animator
   ecore_animator_frametime_set(1.0 / ZOOM_FRAMERATE);
   sd->smart_zoom_animator = ecore_animator_add(_smart_zoom_animator, sd);

   // hide textSelection handlers during zooming
   if (sd->use_text_selection == EINA_TRUE && sd->text_selection_on == EINA_TRUE)
     _text_selection_hide(sd);
}

static void
_zoom_to_rect(Smart_Data *sd, int x, int y)
{
   DBG("%s\n", __func__);
   Evas_Object *webview = sd->base.self;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   // performing a hit test
   _coords_evas_to_ewk(webview, x, y, &x, &y);
   if (!sd->ewk_frame_hit_test_new)
     sd->ewk_frame_hit_test_new = (Ewk_Hit_Test * (*)(const Evas_Object *, int, int))dlsym(ewk_handle, "ewk_frame_hit_test_new");
   Ewk_Hit_Test *hit_test = sd->ewk_frame_hit_test_new(sd->ewk_view_frame_main_get(webview), x, y);

   // calculate zoom_rate and center of rect
   if (hit_test->bounding_box.w && hit_test->bounding_box.h)
     {
	// set zooming data
	float zoom_rate = INPUT_ZOOM_RATIO;
	if (!sd->ewk_view_zoom_get)
	  sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
	float current_zoom_rate = sd->ewk_view_zoom_get(webview);
	float zoom_step = zoom_rate / current_zoom_rate;

	// get position to move from
	int view_x, view_y, view_w, view_h;
	evas_object_geometry_get(webview, &view_x, &view_y, &view_w, &view_h);
	int from_x, from_y;
	_coords_ewk_to_evas(webview, hit_test->bounding_box.x, hit_test->bounding_box.y, &from_x, &from_y);
	from_x = from_x + ((view_w - INPUT_LOCATION_X) / 2) / zoom_step;
	from_y = from_y + hit_test->bounding_box.h / 2;

	// get position to move to
	int to_x = view_x + INPUT_LOCATION_X + (view_w - INPUT_LOCATION_X) / 2;
	int to_y = view_y + INPUT_LOCATION_Y + (hit_test->bounding_box.h / 2) * zoom_step;

	// adjust to contents
	_adjust_to_contents_boundary(webview, &to_x, &to_y, from_x, from_y, zoom_rate);

	// set data for smart zoom
	sd->zoom.basis.x = (to_x - zoom_step * from_x) / (1 - zoom_step);
	sd->zoom.basis.y = (to_y - zoom_step * from_y) / (1 - zoom_step) - view_y;
	sd->zoom.zoom_rate_at_start = current_zoom_rate;
	sd->zoom.zoom_rate_to_set = zoom_rate;
	if (!sd->ewk_frame_scroll_pos_get)
	  sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");
	sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(webview),
	      &sd->zoom.scroll_at_start.x, &sd->zoom.scroll_at_start.y);
	sd->zoom.scroll_to_set.x = sd->zoom.scroll_at_start.x + (from_x - to_x);
	sd->zoom.scroll_to_set.y = sd->zoom.scroll_at_start.y + (from_y - to_y);
	smart_zoom_index = N_COSINE - 1;

	_suspend_all(sd, EINA_TRUE);

	// run animator
	ecore_animator_frametime_set(1.0 / ZOOM_FRAMERATE);
	sd->smart_zoom_animator = ecore_animator_add(_smart_zoom_animator, sd);
     }

   if (!sd->ewk_frame_hit_test_free)
     sd->ewk_frame_hit_test_free = (void (*)(Ewk_Hit_Test *))dlsym(ewk_handle, "ewk_frame_hit_test_free");
   sd->ewk_frame_hit_test_free(hit_test);
}

// text-selection
#define BAR_WIDTH            4
#define BAR_HEIGHT           10
#define HANDLE_WIDTH         60
#define HANDLE_HEIGHT        60
#define HANDLE_PRESS_RANGE   50
#define HANDLE_MIDDLE_LENGTH 60

static Evas_Object* front_bar_icon;
static Evas_Object* front_handle_icon;
static Evas_Object* back_bar_icon;
static Evas_Object* back_handle_icon;

static Eina_Bool initialized = EINA_FALSE;

static void
_text_selection_init(Evas_Object* parent)
{
   DBG("<< %s >>\n", __FUNCTION__);

   if (initialized)
     return;

   // front bar
   front_bar_icon = (Evas_Object*)elm_icon_add(parent);
   elm_icon_standard_set(front_bar_icon, "webview/ts_bar");
   elm_icon_scale_set(front_bar_icon, true, true);
   evas_object_pass_events_set(front_bar_icon, true);

   // front handle
   front_handle_icon = (Evas_Object*)elm_icon_add(parent);
   elm_icon_standard_set(front_handle_icon, "webview/ts_handle_front");
   elm_icon_scale_set(front_handle_icon, false, false);
   evas_object_pass_events_set(front_handle_icon, true);

   // back bar
   back_bar_icon = (Evas_Object*)elm_icon_add(parent);
   elm_icon_standard_set(back_bar_icon, "webview/ts_bar");
   elm_icon_scale_set(back_bar_icon, true, true);
   evas_object_pass_events_set(back_bar_icon, true);

   // back handle
   back_handle_icon = (Evas_Object*)elm_icon_add(parent);
   elm_icon_standard_set(back_handle_icon, "webview/ts_handle_back");
   elm_icon_scale_set(back_handle_icon, false, false);
   evas_object_pass_events_set(back_handle_icon, true);

   initialized = EINA_TRUE;
}

static void
_text_selection_show(void)
{
   evas_object_show(front_bar_icon);
   evas_object_show(front_handle_icon);
   evas_object_show(back_bar_icon);
   evas_object_show(back_handle_icon);
}

static void
_text_selection_hide(Smart_Data *sd)
{
   evas_object_hide(front_bar_icon);
   evas_object_hide(front_handle_icon);
   evas_object_hide(back_bar_icon);
   evas_object_hide(back_handle_icon);

   sd->text_selection.front.x = -1;
   sd->text_selection.front.y = -1;
   sd->text_selection.front.h = -1;
   sd->text_selection.front_handle.x = -1;
   sd->text_selection.front_handle.y = -1;
   sd->text_selection.back.x = -1;
   sd->text_selection.back.y = -1;
   sd->text_selection.back.h = -1;
   sd->text_selection.back_handle.x = -1;
   sd->text_selection.back_handle.y = -1;
}

static void
_text_selection_set_front_info(Smart_Data *sd, int x, int y, int height)
{
   Evas_Object *webview = sd->base.self;

   Evas_Coord_Rectangle* front = &(sd->text_selection.front);
   Evas_Point* front_handle = &(sd->text_selection.front_handle);

   front->h = height;
   int front_bar_height = height + HANDLE_MIDDLE_LENGTH + HANDLE_HEIGHT;

   // set size
   evas_object_resize(front_bar_icon, BAR_WIDTH, front_bar_height);
   evas_object_resize(front_handle_icon, HANDLE_WIDTH, HANDLE_HEIGHT);

   // set location
   front_handle->x = x - (HANDLE_WIDTH / 2);
   int win_y, win_height, win_bottom;
   evas_object_geometry_get(webview, NULL, &win_y, NULL, &win_height);
   win_bottom = win_y + win_height;
   if ((front_handle->y == -1 && (y + front_bar_height > win_bottom))
	 || ((front_handle->y < front->y) && (y + front->h - front_bar_height > win_y))
	 || ((front_handle->y > front->y) && (y + front_bar_height > win_bottom)))
     { // upper handle
	front_handle->y = y + front->h - front_bar_height + (HANDLE_HEIGHT / 2);
	evas_object_move(front_bar_icon, x, y + front->h - front_bar_height);
	evas_object_move(front_handle_icon, x - HANDLE_WIDTH, y + front->h - front_bar_height);

     }
   else
     { // lower handle
	front_handle->y = y + front_bar_height - (HANDLE_HEIGHT / 2);
	evas_object_move(front_bar_icon, x, y);
	evas_object_move(front_handle_icon, x - HANDLE_WIDTH, front_handle->y - (HANDLE_HEIGHT / 2));
     }

   front->x = x;
   front->y = y;
}

static void
_text_selection_set_back_info(Smart_Data *sd, int x, int y, int height)
{
   Evas_Object *webview = sd->base.self;

   Evas_Coord_Rectangle* back = &(sd->text_selection.back);
   Evas_Point* back_handle = &(sd->text_selection.back_handle);

   back->h = height;
   int back_bar_height = height + HANDLE_MIDDLE_LENGTH + HANDLE_HEIGHT;

   // set size
   evas_object_resize(back_bar_icon, BAR_WIDTH, back_bar_height);
   evas_object_resize(back_handle_icon, HANDLE_WIDTH, HANDLE_HEIGHT);

   // set location
   back_handle->x = x + (HANDLE_WIDTH / 2);
   int win_y, win_height, win_bottom;
   evas_object_geometry_get(webview, NULL, &win_y, NULL, &win_height);
   win_bottom = win_y + win_height;
   if ((back_handle->y == -1 && (y - back->h + back_bar_height > win_bottom))
	 || ((back_handle->y < back->y) && (y - back_bar_height > win_y))
	 || ((back_handle->y > back->y) && (y - back->h + back_bar_height > win_bottom))) { // upper handle
	back_handle->y = y - back->h - HANDLE_MIDDLE_LENGTH - (HANDLE_HEIGHT / 2);
	evas_object_move(back_bar_icon, x - BAR_WIDTH, y - back_bar_height);
	evas_object_move(back_handle_icon, x, back_handle->y - (HANDLE_HEIGHT / 2));

   } else {
	back_handle->y = y + HANDLE_MIDDLE_LENGTH + (HANDLE_HEIGHT / 2);
	evas_object_move(back_bar_icon, x - BAR_WIDTH, y - back->h);
	evas_object_move(back_handle_icon, x, back_handle->y - (HANDLE_HEIGHT / 2));
   }

   back->x = x;
   back->y = y;
}

static Eina_Bool
_text_selection_handle_pressed(Smart_Data *sd, int x, int y)
{
   Evas_Point front_handle = sd->text_selection.front_handle;
   Evas_Point back_handle = sd->text_selection.back_handle;

   // check front handle
   if (x > (front_handle.x - HANDLE_PRESS_RANGE) && x < (front_handle.x + HANDLE_PRESS_RANGE)
	 && y > (front_handle.y - HANDLE_PRESS_RANGE) && y < (front_handle.y + HANDLE_PRESS_RANGE))
     sd->text_selection.front_handle_moving = EINA_TRUE;

   // check back handle
   if (x > (back_handle.x - HANDLE_PRESS_RANGE) && x < (back_handle.x + HANDLE_PRESS_RANGE)
	 && y > (back_handle.y - HANDLE_PRESS_RANGE) && y < (back_handle.y + HANDLE_PRESS_RANGE))
     {
	if (sd->text_selection.front_handle_moving == EINA_TRUE)
	  {
	     if (abs(x - front_handle.x) + abs(y - front_handle.y)
		   > abs(x - back_handle.x) + abs(y - back_handle.y))
	       {
		  sd->text_selection.front_handle_moving = EINA_FALSE;
		  sd->text_selection.back_handle_moving = EINA_TRUE;
	       }
	  }
	else
	  {
	     sd->text_selection.back_handle_moving = EINA_TRUE;
	  }
     }

   return (sd->text_selection.front_handle_moving || sd->text_selection.back_handle_moving);
}

static void
_text_selection_update_position(Smart_Data *sd, int x, int y)
{
   Evas_Object *webview = sd->base.self;

   Evas_Coord_Rectangle* front = &(sd->text_selection.front);
   Evas_Coord_Rectangle* back = &(sd->text_selection.back);

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   // set selected region with front handle
   if (sd->text_selection.front_handle_moving == EINA_TRUE)
     {
	x = x + (HANDLE_WIDTH >> 1);
	if (sd->text_selection.front_handle.y < sd->text_selection.front.y)
	  y = y + (HANDLE_HEIGHT >> 1) + HANDLE_MIDDLE_LENGTH;
	else
	  y = y - front->h - HANDLE_MIDDLE_LENGTH - (HANDLE_HEIGHT >> 1);

	if (y > back->y)
	  y = back->y - back->h / 2;

	if (!sd->ewk_frame_selection_left_set)
	  sd->ewk_frame_selection_left_set = (Eina_Bool (*)(Evas_Object *, int, int, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_selection_left_set");
	int ewkX, ewkY;
	_coords_evas_to_ewk(webview, x, y, &ewkX, &ewkY);
	if (sd->ewk_frame_selection_left_set(sd->ewk_view_frame_main_get(webview), ewkX, ewkY,
		 &front->x, &front->y, &front->h)) {
	     _coords_ewk_to_evas(webview, front->x, front->y, &front->x, &front->y);
	     _text_selection_set_front_info(sd, front->x, front->y, front->h);
	}

	// set selected region with back handle
     }
   else if (sd->text_selection.back_handle_moving)
     {
	x = x - (HANDLE_WIDTH >> 1);
	if (sd->text_selection.back_handle.y < sd->text_selection.back.y)
	  y = y + (HANDLE_HEIGHT >> 1) + HANDLE_MIDDLE_LENGTH;
	else
	  y = y - back->h - HANDLE_MIDDLE_LENGTH - (HANDLE_HEIGHT >> 1);

	if (y < front->y)
	  y = front->y + front->h / 2;

	if (!sd->ewk_frame_selection_right_set)
	  sd->ewk_frame_selection_right_set = (Eina_Bool (*)(Evas_Object *, int, int, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_selection_right_set");
	int ewkX, ewkY;
	_coords_evas_to_ewk(webview, x, y, &ewkX, &ewkY);
	if (sd->ewk_frame_selection_right_set(sd->ewk_view_frame_main_get(webview), ewkX, ewkY,
		 &back->x, &back->y, &back->h)) {
	     _coords_ewk_to_evas(webview, back->x, back->y, &back->x, &back->y);
	     _text_selection_set_back_info(sd, back->x, back->y, back->h);
	}
     }
}

static void
_text_selection_move_by(Smart_Data *sd, int dx, int dy)
{
   _text_selection_set_front_info(sd, sd->text_selection.front.x + dx,
	 sd->text_selection.front.y + dy,
	 sd->text_selection.front.h);
   _text_selection_set_back_info(sd, sd->text_selection.back.x + dx,
	 sd->text_selection.back.y + dy,
	 sd->text_selection.back.h);
}
// minimap
static void
_minimap_update_detail(Evas_Object* minimap, Smart_Data *sd, cairo_surface_t* src, int srcW, int srcH, Eina_Rectangle* visibleRect)
{
   void* pixels;
   cairo_t* cr;
   cairo_surface_t* dest;
   cairo_status_t status;

   if (!sd->cairo_surface_status)
     sd->cairo_surface_status = (cairo_status_t (*)(cairo_surface_t *))dlsym(cairo_handle, "cairo_surface_status");
   if (!sd->cairo_image_surface_create_for_data)
     sd->cairo_image_surface_create_for_data = (cairo_surface_t * (*)(unsigned char *, cairo_format_t, int, int, int))dlsym(cairo_handle, "cairo_image_surface_create_for_data");

   //TODO: check which one is faster
   //      1) reuse minimap
   //      2) recreate evas_object and set pixel
   evas_object_image_size_set(minimap, srcW, srcH);
   evas_object_image_fill_set(minimap, 0, 0, srcW, srcH);
   evas_object_resize(minimap, srcW, srcH);

   pixels = evas_object_image_data_get(minimap, 1);
   dest = sd->cairo_image_surface_create_for_data(
	 (unsigned char*)pixels, CAIRO_FORMAT_RGB24, srcW, srcH, srcW * 4);
   status = sd->cairo_surface_status(dest);
   if (status != CAIRO_STATUS_SUCCESS)
     {
	printf("[%s] fail to create cairo surface\n", __func__);
	goto error_cairo_surface;
     }

   if (!sd->cairo_create)
     sd->cairo_create = (cairo_t * (*)(cairo_surface_t *))dlsym(cairo_handle, "cairo_create");
   cr = sd->cairo_create(dest);
   status = sd->cairo_surface_status(dest);
   if (status != CAIRO_STATUS_SUCCESS)
     {
	printf("[%s] fail to create cairo\n", __func__);
	goto error_cairo;
     }

   if (!sd->cairo_set_source_surface)
     sd->cairo_set_source_surface = (void (*)(cairo_t *, cairo_surface_t *, double, double))dlsym(cairo_handle, "cairo_set_source_surface");
   if (!sd->cairo_paint)
     sd->cairo_paint = (void (*)(cairo_t *))dlsym(cairo_handle, "cairo_paint");
   if (!sd->cairo_set_source_rgb)
     sd->cairo_set_source_rgb = (void (*)(cairo_t *, double, double, double))dlsym(cairo_handle, "cairo_set_source_rgb");
   if (!sd->cairo_rectangle)
     sd->cairo_rectangle = (void (*)(cairo_t *, double, double, double, double))dlsym(cairo_handle, "cairo_rectangle");
   if (!sd->cairo_set_line_width)
     sd->cairo_set_line_width = (void (*)(cairo_t *, double))dlsym(cairo_handle, "cairo_set_line_width");
   if (!sd->cairo_stroke)
     sd->cairo_stroke = (void (*)(cairo_t *cr))dlsym(cairo_handle, "cairo_stroke"); 
   if (!sd->cairo_set_antialias)
     sd->cairo_set_antialias = (void (*)(cairo_t *, cairo_antialias_t))dlsym(cairo_handle, "cairo_set_antialias");

   sd->cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
   sd->cairo_set_source_surface(cr, src, 0, 0);
   sd->cairo_paint(cr);
   sd->cairo_set_source_rgb(cr, 0, 0, 255);
   sd->cairo_set_line_width(cr, 1);
   sd->cairo_rectangle(cr,
	 visibleRect->x, visibleRect->y, visibleRect->w, visibleRect->h);
   sd->cairo_stroke(cr);

   if (!sd->cairo_destroy)
     sd->cairo_destroy = (void (*)(cairo_t *))dlsym(cairo_handle, "cairo_destroy");
   sd->cairo_destroy(cr);

error_cairo:
   if (!sd->cairo_surface_destroy)
     sd->cairo_surface_destroy = (void (*)(cairo_surface_t *))dlsym(cairo_handle, "cairo_surface_destroy");
   sd->cairo_surface_destroy(dest);
error_cairo_surface:
   evas_object_image_data_set(minimap, pixels);
   return;
}

static void
_minimap_update(Evas_Object* minimap, Smart_Data *sd, cairo_surface_t* src, int minimapW, int minimapH)
{
   if (minimap == NULL || src == NULL) return;
   Evas_Object *webview = sd->base.self;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");
   int cw, ch;
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(webview), &cw, &ch);
   if (cw == 0 || ch == 0) return;

   if (!sd->ewk_frame_visible_content_geometry_get)
     sd->ewk_frame_visible_content_geometry_get = (Eina_Bool (*)(const Evas_Object *, Eina_Bool, int *, int *, int *, int *))dlsym(ewk_handle, "ewk_frame_visible_content_geometry_get");
   int x, y, w, h;
   sd->ewk_frame_visible_content_geometry_get(
	 sd->ewk_view_frame_main_get(webview), EINA_FALSE,
	 &x, &y, &w, &h);
   DBG("visible area : %d, %d, %d, %d\n", x, y, w, h);

   Eina_Rectangle rect = {
	x * minimapW / cw, y * minimapH / ch,
	w * minimapW / cw, h * minimapH / ch};
   _minimap_update_detail(minimap, sd, src, minimapW, minimapH, &rect);
}

static cairo_surface_t*
_image_clone_get(Smart_Data *sd, int* minimap_w, int* minimap_h)
{
   DBG("%s is called\n", __func__);
   Evas_Object *webview = sd->base.self;
   EWK_VIEW_PRIV_GET_OR_RETURN(sd, priv, NULL);

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");
   int w, h;
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(webview), &w, &h);
   printf(" W : %d / H : %d\n", w, h);

   float x_scale = MINIMAP_WIDTH / (float)w;
   float y_scale = MINIMAP_HEIGHT / (float)h;
   float scale_factor;
   if (x_scale < y_scale)
     {
	scale_factor = x_scale;
	*minimap_w = MINIMAP_WIDTH;
	*minimap_h = h * scale_factor;
     }
   else
     {
	scale_factor = y_scale;
	*minimap_w = w * scale_factor;
	*minimap_h = MINIMAP_HEIGHT;
     }
   printf(" minimap w,h : (%d, %d)\n", *minimap_w, *minimap_h);

   if (!sd->ewk_view_paint_contents)
     sd->ewk_view_paint_contents = (Eina_Bool (*)(Ewk_View_Private_Data *, cairo_t *, const Eina_Rectangle *))dlsym(ewk_handle, "ewk_view_paint_contents");
   if (!sd->cairo_image_surface_create)
     sd->cairo_image_surface_create = (cairo_surface_t * (*)(cairo_format_t, int, int))dlsym(cairo_handle, "cairo_image_surface_create");
   if (!sd->cairo_create)
     sd->cairo_create = (cairo_t * (*)(cairo_surface_t *))dlsym(cairo_handle, "cairo_create");
   if (!sd->cairo_destroy)
     sd->cairo_destroy = (void (*)(cairo_t *))dlsym(cairo_handle, "cairo_destroy");
   if (!sd->cairo_scale)
     sd->cairo_scale = (void (*)(cairo_t *, double, double))dlsym(cairo_handle, "cairo_scale");
   if (!sd->cairo_surface_write_to_png)
     sd->cairo_surface_write_to_png = (cairo_status_t (*)(cairo_surface_t *, const char *))dlsym(cairo_handle, "cairo_surface_write_to_png");
   if (!sd->cairo_set_antialias)
     sd->cairo_set_antialias = (void (*)(cairo_t *, cairo_antialias_t))dlsym(cairo_handle, "cairo_set_antialias");

   cairo_surface_t* ret = sd->cairo_image_surface_create(CAIRO_FORMAT_RGB24, *minimap_w, *minimap_h);
   cairo_t* cr = sd->cairo_create(ret);
   sd->cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
   sd->cairo_scale(cr, scale_factor, scale_factor);
   Eina_Rectangle rect = {0, 0, w, h};
   sd->ewk_view_paint_contents(priv, cr, &rect);
   sd->cairo_destroy(cr);
   sd->cairo_surface_write_to_png(ret, "/home/root/test.png");

   return ret;
}

// coord
static void
_unzoom_position(Evas_Object* obj, int x, int y, int* ux, int* uy)
{
   INTERNAL_ENTRY;
   int viewY;
   evas_object_geometry_get(obj, NULL, &viewY, NULL, NULL);

   if (!sd->ewk_view_zoom_get)
     sd->ewk_view_zoom_get = (float (*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_zoom_get");
   float zoomRatio = sd->ewk_view_zoom_get(obj);
   if (zoomRatio)
     {
	*ux = x / zoomRatio;
	*uy = (y - viewY) / zoomRatio;
     }
}

static void
_coords_evas_to_ewk(Evas_Object* obj, int x, int y, int* ux, int* uy)
{
   INTERNAL_ENTRY;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   if (!sd->ewk_frame_scroll_pos_get)
     sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");

   int scrollX, scrollY, viewY;
   sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(obj), &scrollX, &scrollY);
   evas_object_geometry_get(obj, NULL, &viewY, NULL, NULL);
   *ux = x + scrollX;
   *uy = y + scrollY - viewY;
}

static void
_coords_ewk_to_evas(Evas_Object* obj, int x, int y, int* ux, int* uy)
{
   INTERNAL_ENTRY;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");

   if (!sd->ewk_frame_scroll_pos_get)
     sd->ewk_frame_scroll_pos_get = (Eina_Bool (*)(const Evas_Object *, int *, int *))dlsym(ewk_handle, "ewk_frame_scroll_pos_get");

   int scrollX, scrollY, viewY;
   sd->ewk_frame_scroll_pos_get(sd->ewk_view_frame_main_get(obj), &scrollX, &scrollY);
   evas_object_geometry_get(obj, NULL, &viewY, NULL, NULL);
   *ux = x - scrollX;
   *uy = y - scrollY + viewY;
}
static void
_update_min_zoom_rate(Evas_Object *obj)
{
   INTERNAL_ENTRY;

   if (!sd->ewk_view_frame_main_get)
     sd->ewk_view_frame_main_get = (Evas_Object *(*)(const Evas_Object *))dlsym(ewk_handle, "ewk_view_frame_main_get");
   if (!sd->ewk_frame_contents_size_get)
     sd->ewk_frame_contents_size_get = (Eina_Bool (*)(const Evas_Object *, Evas_Coord *, Evas_Coord *))dlsym(ewk_handle, "ewk_frame_contents_size_get");
   if (!sd->ewk_view_zoom_range_set)
     sd->ewk_view_zoom_range_set = (void (*)(Evas_Object *, float, float))dlsym(ewk_handle, "ewk_view_zoom_range_set");

   int content_w, object_w;
   evas_object_geometry_get(obj, NULL, NULL, &object_w, NULL);
   sd->ewk_frame_contents_size_get(sd->ewk_view_frame_main_get(obj), &content_w, NULL);
   if (content_w)
     sd->zoom.min_zoom_rate = (float)object_w / (float)content_w;

   if (sd->use_zoom_bouncing)
     {
	float min_zoom_rate = sd->zoom.min_zoom_rate * ZOOM_OUT_BOUNCING;
	if (min_zoom_rate <= 0) min_zoom_rate = MIN_ZOOM_RATIO;
	float max_zoom_rate = sd->zoom.max_zoom_rate * ZOOM_IN_BOUNCING;
	sd->ewk_view_zoom_range_set(obj, min_zoom_rate, max_zoom_rate);
     }
   else
     {
	sd->ewk_view_zoom_range_set(obj, sd->zoom.min_zoom_rate, sd->zoom.max_zoom_rate);
     }
}
static void
_geolocation_permission_callback(void *geolocation_obj, const char* url)
{
   printf("\n\n<< %s >>\n\n", __func__);
   INTERNAL_ENTRY;

   Evas_Object *popup;
   int length;
   char msg1[] = "The page at ";
   char msg2[] = "<br>wants to know where you are.<br>Do you want to share location?";
   char *msg = NULL;
   int result;

   length = strlen(msg1) + strlen(url) + strlen(msg2);
   msg = calloc(length + 1, sizeof(char));
   strncpy(msg, msg1, strlen(msg1));
   strncat(msg, url, strlen(url));
   strncat(msg, msg2, strlen(msg2));
   msg[length] = '\0';

   popup = elm_popup_add(obj);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_popup_desc_set(popup, msg);
   elm_popup_buttons_add(popup, 2, "Share", ELM_POPUP_RESPONSE_OK,
	                           "Don't Share", ELM_POPUP_RESPONSE_CANCEL, NULL);
   result = elm_popup_run(popup); // modal dialog
   switch (result)
     {
      case ELM_POPUP_RESPONSE_OK:
	 if (!sd->ewk_set_geolocation_sharing_allowed)
	   sd->ewk_set_geolocation_sharing_allowed = (void (*)(void *, Eina_Bool))dlsym(ewk_handle, "ewk_set_geolocation_sharing_allowed");
	 sd->ewk_set_geolocation_sharing_allowed(geolocation_obj, EINA_TRUE);
	 break;

      case ELM_POPUP_RESPONSE_CANCEL:
	 if (!sd->ewk_set_geolocation_sharing_allowed)
	   sd->ewk_set_geolocation_sharing_allowed = (void (*)(void *, Eina_Bool))dlsym(ewk_handle, "ewk_set_geolocation_sharing_allowed");
	 sd->ewk_set_geolocation_sharing_allowed(geolocation_obj, EINA_FALSE);
	 break;

      default:
	 break;
     }
   if (msg)
     free(msg);
}
#endif
