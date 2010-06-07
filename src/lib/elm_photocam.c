#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Photocam Photocam
 *
 * This is a widget specifically for displaying high-resolution digital
 * camera photos giving speedy feedback (fast load), low memory footprint
 * and zooming and panning as well as fitting logic. It is entirely focused
 * on jpeg images, and takes advantage of properties of the jpeg format (via
 * evas loader features in the jpeg loader).
 * 
 * Signals that you can add callbacks for are:
 *
 * clicked - This is called when a user has clicked the photo without dragging
 * around.
 * 
 * press - This is called when a user has pressed down on the photo.
 * 
 * longpressed - This is called when a user has pressed down on the photo for
 * a long time without dragging around.
 * 
 * clicked,double - This is called when a user has double-clicked the photo.
 * 
 * load - Photo load begins.
 * 
 * loaded - This is called when the image file load is complete for the first
 * view (low resolution blurry version).
 *
 * load,details - Photo detailed data load begins.
 * 
 * loaded,details - This is called when the image file load is complete for the
 * detailed image data (full resolution needed).
 *
 * zoom,start - Zoom animation started.
 * 
 * zoom,stop - Zoom animation stopped.
 * 
 * zoom,change - Zoom changed when using an auto zoom mode.
 * 
 * scroll - the content has been scrolled (moved)
 *
 * scroll,anim,start - scrolling animation has started
 *
 * scroll,anim,stop - scrolling animation has stopped
 *
 * scroll,drag,start - dragging the contents around has started
 *
 * scroll,drag,stop - dragging the contents around has stopped
 * 
 * ---
 * 
 * TODO (maybe - optional future stuff):
 * 
 * 1. wrap photo in theme edje so u can have styling around photo (like white
 *    photo bordering).
 * 2. exif handling
 * 3. rotation flags in exif handling (nasty! should have rot in evas)
 *
 */
typedef struct _Widget_Data Widget_Data;
typedef struct _Pan Pan;
typedef struct _Grid Grid;
typedef struct _Grid_Item Grid_Item;

struct _Grid_Item
{
   Widget_Data *wd;
   Evas_Object *img;
   struct 
     {
        int x, y, w, h;
     } src, out;
   Eina_Bool want : 1;
   Eina_Bool have : 1;
};

struct _Grid
{
   int tsize; // size of tile (tsize x tsize pixels)
   int zoom; // zoom level tiles want for optimal display (1, 2, 4, 8)
   int iw, ih; // size of image in pixels
   int w, h; // size of grid image in pixels (represented by grid)
   int gw, gh; // size of grid in tiles
   Grid_Item *grid; // the grid (gw * gh items)
   Eina_Bool dead : 1; // old grid. will die as soon as anim is over
};

struct _Widget_Data
{
   Evas_Object *obj;
   Evas_Object *scr;
   Evas_Object *pan_smart;
   Pan *pan;
   Evas_Coord pan_x, pan_y, minw, minh;

   double zoom;
   Elm_Photocam_Zoom_Mode mode;
   const char *file;
   
   Ecore_Job *calc_job;
   Ecore_Timer *scr_timer;
   Ecore_Timer *long_timer;
   Ecore_Animator *zoom_animator;
   double t_start, t_end;
   struct 
     {
        int imw, imh;
        int w, h;
        int ow, oh, nw, nh;
        struct 
          {
             double x, y;
          } spos;
     } size;
   struct
     {
	Eina_Bool show : 1;
	Evas_Coord x, y ,w ,h;
     } show;
   int tsize;
   Evas_Object *img; // low res version of image (scale down == 8)
   int nosmooth;
   int preload_num;
   Eina_List *grids;
   Eina_Bool main_load_pending : 1;
   Eina_Bool resized : 1;
   Eina_Bool longpressed : 1;
   Eina_Bool on_hold : 1;
   Eina_Bool paused : 1;
};

struct _Pan
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Widget_Data *wd;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
//static void _show_region_hook(void *data, Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _calc_job(void *data);
static void grid_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh);
static void grid_clear(Evas_Object *obj, Grid *g);
static Grid *grid_create(Evas_Object *obj);
static void grid_load(Evas_Object *obj, Grid *g);

static int
nearest_pow2(int num)
{
   unsigned int n = num - 1;
   n |= n >> 1;
   n |= n >> 2;
   n |= n >> 4;
   n |= n >> 8;
   n |= n >> 16;
   return n + 1;
}

static void
img_place(Evas_Object *obj, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ax, ay, gw, gh;
   if (!wd) return;
   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;
   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;
   evas_object_move(wd->img, ox + 0 - px + ax, oy + 0 - py + ay);
   evas_object_resize(wd->img, gw, gh);

   if (wd->show.show)
     {
	wd->show.show = EINA_FALSE;
	elm_smart_scroller_child_region_show(wd->scr, wd->show.x, wd->show.y, wd->show.w, wd->show.h);
     }
}

static void
grid_place(Evas_Object *obj, Grid *g, Evas_Coord px, Evas_Coord py, Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord ax, ay, gw, gh, tx, ty;
   int x, y;
   if (!wd) return;
   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;
   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;
   for (y = 0; y < g->gh; y++)
     {
        for (x = 0; x < g->gw; x++)
          {
             int tn, xx, yy, ww, hh;
             
             tn = (y * g->gw) + x;
             xx = g->grid[tn].out.x;
             yy = g->grid[tn].out.y;
             ww = g->grid[tn].out.w;
             hh = g->grid[tn].out.h;
             if ((gw != g->w) && (g->w > 0))
               {
                  tx = xx;
                  xx = (gw * xx) / g->w;
                  ww = ((gw * (tx + ww)) / g->w) - xx;
               }
             if ((gh != g->h) && (g->h > 0))
               {
                  ty = yy;
                  yy = (gh * yy) / g->h;
                  hh = ((gh * (ty + hh)) / g->h) - yy;
               }
             evas_object_move(g->grid[tn].img,
                              ox + xx - px + ax,
                              oy + yy - py + ay);
             evas_object_resize(g->grid[tn].img, ww, hh);
          }
     }
}

static void
grid_clear(Evas_Object *obj, Grid *g)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int x, y;
   if (!wd) return;
   if (!g->grid) return;
   for (y = 0; y < g->gh; y++)
     {
        for (x = 0; x < g->gw; x++)
          {
             int tn;
             
             tn = (y * g->gw) + x;
             evas_object_del(g->grid[tn].img);
             if (g->grid[tn].want)
               {
                  wd->preload_num--;
                  if (wd->preload_num == 0)
                    {
                       edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                               "elm,state,busy,stop", "elm");
                       evas_object_smart_callback_call(obj, "loaded,detail", NULL);
                    }
               }
          }
     }
   free(g->grid);
   g->grid = NULL;
   g->gw = 0;
   g->gh = 0;
}

static void
_tile_preloaded(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__, void *event_info __UNUSED__)
{
   Grid_Item *git = data;

   if (git->want)
     {
        git->want = 0;
        evas_object_show(git->img);
        git->have = 1;
        git->wd->preload_num--;
        if (git->wd->preload_num == 0)
          {
             edje_object_signal_emit(elm_smart_scroller_edje_object_get(git->wd->scr),
                                     "elm,state,busy,stop", "elm");
             evas_object_smart_callback_call(git->wd->obj, "loaded,detail", NULL);
          }
     }
}

static int
grid_zoom_calc(double zoom)
{
   int z = zoom;
   if (z < 1) z = 1;
   return nearest_pow2(z);
}

static Grid *
grid_create(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int x, y;
   Grid *g;
   
   if (!wd) return NULL;
   g = calloc(1, sizeof(Grid));
   
   g->zoom = grid_zoom_calc(wd->zoom);
   g->tsize = wd->tsize;
   g->iw = wd->size.imw;
   g->ih = wd->size.imh;
   
   g->w = g->iw / g->zoom;
   g->h = g->ih / g->zoom;
   if (g->zoom >= 8) return NULL;
   g->gw = (g->w + g->tsize - 1) / g->tsize;
   g->gh = (g->h + g->tsize - 1) / g->tsize;
   g->grid = calloc(1, sizeof(Grid_Item) * g->gw * g->gh);
   if (!g->grid)
     {
        g->gw = 0;
        g->gh = 0;
        return g;
     }
   for (y = 0; y < g->gh; y++)
     {
        for (x = 0; x < g->gw; x++)
          {
             int tn;
             
             tn = (y * g->gw) + x;
             g->grid[tn].src.x = x * g->tsize;
             if (x == (g->gw - 1))
               g->grid[tn].src.w = g->w - ((g->gw - 1) * g->tsize);
             else
               g->grid[tn].src.w = g->tsize;
             g->grid[tn].src.y = y * g->tsize;
             if (y == (g->gh - 1))
               g->grid[tn].src.h = g->h - ((g->gh - 1) * g->tsize);
             else
               g->grid[tn].src.h = g->tsize;
             
             g->grid[tn].out.x = g->grid[tn].src.x;
             g->grid[tn].out.y = g->grid[tn].src.y;
             g->grid[tn].out.w = g->grid[tn].src.w;
             g->grid[tn].out.h = g->grid[tn].src.h;
             
             g->grid[tn].wd = wd;
             g->grid[tn].img = 
               evas_object_image_add(evas_object_evas_get(obj));
             evas_object_image_scale_hint_set
               (g->grid[tn].img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
             evas_object_pass_events_set(g->grid[tn].img, 1);
             evas_object_smart_member_add(g->grid[tn].img, 
                                          wd->pan_smart);
             elm_widget_sub_object_add(obj, g->grid[tn].img);
             evas_object_image_filled_set(g->grid[tn].img, 1);
             evas_object_event_callback_add(g->grid[tn].img, 
                                            EVAS_CALLBACK_IMAGE_PRELOADED,
                                            _tile_preloaded, 
                                            &(g->grid[tn]));
          }
     }
   return g;
}

static void
grid_load(Evas_Object *obj, Grid *g)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   int x, y;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh, gw, gh, ax, ay, tx, ty;
   if (!wd) return;
   evas_object_geometry_get(wd->pan_smart, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(wd->obj), &cvx, &cvy, &cvw, &cvh);
   ax = 0;
   ay = 0;
   gw = wd->size.w;
   gh = wd->size.h;
   if (ow > gw) ax = (ow - gw) / 2;
   if (oh > gh) ay = (oh - gh) / 2;
   for (y = 0; y < g->gh; y++)
     {
        for (x = 0; x < g->gw; x++)
          {
             int tn, xx, yy, ww, hh;
             Eina_Bool visible = EINA_FALSE;
             
             tn = (y * g->gw) + x;
             xx = g->grid[tn].out.x;
             yy = g->grid[tn].out.y;
             ww = g->grid[tn].out.w;
             hh = g->grid[tn].out.h;
             if ((gw != g->w) && (g->w > 0))
               {
                  tx = xx;
                  xx = (gw * xx) / g->w;
                  ww = ((gw * (tx + ww)) / g->w) - xx;
               }
             if ((gh != g->h) && (g->h > 0))
               {
                  ty = yy;
                  yy = (gh * yy) / g->h;
                  hh = ((gh * (ty + hh)) / g->h) - yy;
               }
//             xx += ax;
//             yy += ay;
             if (ELM_RECTS_INTERSECT(xx - wd->pan_x + ox, 
                                     yy  - wd->pan_y + oy,
                                     ww, hh,
                                     cvx, cvy, cvw, cvh))
               visible = 1;
             if ((visible) && (!g->grid[tn].have) && (!g->grid[tn].want))
               {
                  g->grid[tn].want = 1;
                  evas_object_hide(g->grid[tn].img);
                  evas_object_image_file_set(g->grid[tn].img, NULL, NULL);
                  evas_object_image_load_scale_down_set(g->grid[tn].img, g->zoom);
                  evas_object_image_load_region_set(g->grid[tn].img,
                                                    g->grid[tn].src.x,
                                                    g->grid[tn].src.y,
                                                    g->grid[tn].src.w,
                                                    g->grid[tn].src.h);
                  evas_object_image_file_set(g->grid[tn].img, wd->file, NULL); 
                  evas_object_image_preload(g->grid[tn].img, 0);
                  wd->preload_num++;
                  if (wd->preload_num == 1)
                    {
                       edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                               "elm,state,busy,start", "elm");
                       evas_object_smart_callback_call(obj, "load,detail", NULL);
                    }
               }
             else if ((g->grid[tn].want) && (!visible))
               {
                  wd->preload_num--;
                  if (wd->preload_num == 0)
                    {
                       edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                               "elm,state,busy,stop", "elm");
                       evas_object_smart_callback_call(obj, "loaded,detail", NULL);
                    }
                  g->grid[tn].want = 0;
                  evas_object_hide(g->grid[tn].img);
                  evas_object_image_preload(g->grid[tn].img, 1);
                  evas_object_image_file_set(g->grid[tn].img, NULL, NULL);
               }
             else if ((g->grid[tn].have) && (!visible))
               {
                  g->grid[tn].have = 0;
                  evas_object_hide(g->grid[tn].img);
                  evas_object_image_preload(g->grid[tn].img, 1);
                  evas_object_image_file_set(g->grid[tn].img, NULL, NULL);
               }
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
   int x, y;
   Eina_List *l;
   Grid *g;
   if (!wd) return;
   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        for (y = 0; y < g->gh; y++)
          {
             for (x = 0; x < g->gw; x++)
               {
                  int tn;
                  
                  tn = (y * g->gw) + x;
                  evas_object_image_smooth_scale_set(g->grid[tn].img, (wd->nosmooth == 0));
               }
          }
     }
   evas_object_image_smooth_scale_set(wd->img, (wd->nosmooth == 0));
}

static void
_grid_raise(Grid *g)
{
   int x, y;
   
   for (y = 0; y < g->gh; y++)
     {
        for (x = 0; x < g->gw; x++)
          {
             int tn;
             
             tn = (y * g->gw) + x;
             evas_object_raise(g->grid[tn].img);
          }
     }
}

static int
_scr_timeout(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return 0;
   wd->nosmooth--;
   if (wd->nosmooth == 0) _smooth_update(data);
   wd->scr_timer = NULL;
   return 0;
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

static void
_main_preloaded(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;
   if (!wd) return;
   evas_object_show(wd->img);
   wd->main_load_pending = 0;
   g = grid_create(obj);
   if (g)
     {
        wd->grids = eina_list_prepend(wd->grids, g);
        grid_load(wd->obj, g);
     }
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
   evas_object_smart_callback_call(data, "loaded", NULL);
   wd->preload_num--;
   if (wd->preload_num == 0)
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                "elm,state,busy,stop", "elm");
        evas_object_smart_callback_call(obj, "loaded,detail", NULL);
     }
}

static int
zoom_do(Evas_Object *obj, double t)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord xx, yy, ow, oh;
   if (!wd) return 0;
   wd->size.w = (wd->size.ow * (1.0 - t)) + (wd->size.nw * t);
   wd->size.h = (wd->size.oh * (1.0 - t)) + (wd->size.nh * t);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &ow, &oh);
   xx = (wd->size.spos.x * wd->size.w) - (ow / 2);
   yy = (wd->size.spos.y * wd->size.h) - (oh / 2);
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
        Eina_List *l, *l_next;
        Grid *g;
        
        EINA_LIST_FOREACH_SAFE(wd->grids, l, l_next, g)
          {
             if (g->dead)
               {
                  wd->grids = eina_list_remove_list(wd->grids, l);
                  grid_clear(obj, g);
                  free(g);
               }
          }
        return 0;
     }
   return 1;
}


static int
_zoom_anim(void *data)
{
   Evas_Object *obj = data;
   Widget_Data *wd = elm_widget_data_get(obj);
   double t;
   int go;
   if (!wd) return 0;
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
        if (wd->nosmooth == 0) _smooth_update(data);
        wd->zoom_animator = NULL;
        evas_object_smart_callback_call(obj, "zoom,stop", NULL);
     }
   return go;
}

static void
_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
//   Evas_Event_Mouse_Move *ev = event_info;
   if (!wd) return;
}

static int
_long_press(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return 0;
   wd->long_timer = NULL;
   wd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(data, "longpressed", NULL);
   return 0;
}

static void
_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Down *ev = event_info;
   if (!wd) return;
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(data, "clicked,double", NULL);
   else
     evas_object_smart_callback_call(data, "press", NULL);
   wd->longpressed = EINA_FALSE;
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   wd->long_timer = ecore_timer_add(1.0, _long_press, data);
}
   
static void
_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Event_Mouse_Up *ev = event_info;
   if (!wd) return;
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) wd->on_hold = EINA_TRUE;
   else wd->on_hold = EINA_FALSE;
   if (wd->long_timer)
     {
        ecore_timer_del(wd->long_timer);
        wd->long_timer = NULL;
     }
   if (!wd->on_hold)
     evas_object_smart_callback_call(data, "clicked", NULL);
   wd->on_hold = EINA_FALSE;
}
   
static Evas_Smart_Class _pan_sc = EVAS_SMART_CLASS_INIT_NULL;

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Grid *g;
   if (!wd) return;
   EINA_LIST_FREE(wd->grids, g)
     {
        if (g->grid) free(g->grid);
        free(g);
     }
   evas_object_del(wd->pan_smart);
   wd->pan_smart = NULL;
   if (wd->file) eina_stringshare_del(wd->file);
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   if (wd->scr_timer) ecore_timer_del(wd->scr_timer);
   if (wd->zoom_animator) ecore_animator_del(wd->zoom_animator);
   if (wd->long_timer) ecore_timer_del(wd->long_timer);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   elm_smart_scroller_object_theme_set(obj, wd->scr, "photocam", "base", elm_widget_style_get(obj));
//   edje_object_scale_set(wd->scr, elm_widget_scale_get(obj) * _elm_config->scale);
   _sizing_eval(obj);
}

/*
static void
_show_region_hook(void *data, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord x, y, w, h;
   if (!wd) return;
   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   elm_smart_scroller_child_region_show(wd->scr, x, y, w, h);
}
*/

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   if (!wd) return;
//   evas_object_size_hint_min_get(wd->scr, &minw, &minh);
   evas_object_size_hint_max_get(wd->scr, &maxw, &maxh);
//   minw = -1;
//   minh = -1;
//   if (wd->mode != ELM_LIST_LIMIT) minw = -1;
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
        wd->resized = 0;
        if (wd->mode != ELM_PHOTOCAM_ZOOM_MODE_MANUAL)
          {
             double tz = wd->zoom;
             wd->zoom = 0.0;
             elm_photocam_zoom_set(wd->obj, tz);
          }
     }
   if ((minw != wd->minw) || (minh != wd->minh))
     {
        wd->minw = minw;
        wd->minh = minh;
        evas_object_smart_callback_call(wd->pan_smart, "changed", NULL);
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
   img_place(sd->wd->obj, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
   EINA_LIST_FOREACH(sd->wd->grids, l, g)
     {
        grid_load(sd->wd->obj, g);
        grid_place(sd->wd->obj, g, sd->wd->pan_x, sd->wd->pan_y, ox, oy, ow, oh);
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
   evas_object_smart_callback_call(data, "scroll,drag,start", NULL);
}

static void
_scr_drag_stop(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, "scroll,drag,stop", NULL);
}

static void
_scr_scroll(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, "scroll", NULL);
}

/**
 * Add a new Photocam object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Photocam
 */
EAPI Evas_Object *
elm_photocam_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_Coord minw, minh;
   static Evas_Smart *smart = NULL;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "photocam");
   elm_widget_type_set(obj, "photocam");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->scr = elm_smart_scroller_add(e);
   elm_smart_scroller_widget_set(wd->scr, obj);
   elm_smart_scroller_object_theme_set(obj, wd->scr, "photocam", "base", "default");
   evas_object_smart_callback_add(wd->scr, "scroll", _scr, obj);
   evas_object_smart_callback_add(wd->scr, "drag", _scr, obj);
   elm_widget_resize_object_set(obj, wd->scr);

   evas_object_smart_callback_add(wd->scr, "animate,start", _scr_anim_start, obj);
   evas_object_smart_callback_add(wd->scr, "animate,stop", _scr_anim_stop, obj);
   evas_object_smart_callback_add(wd->scr, "drag,start", _scr_drag_start, obj);
   evas_object_smart_callback_add(wd->scr, "drag,stop", _scr_drag_stop, obj);
   evas_object_smart_callback_add(wd->scr, "scroll", _scr_scroll, obj);
   
   elm_smart_scroller_bounce_allow_set(wd->scr, 1, 1);

   wd->obj = obj;

   evas_object_smart_callback_add(obj, "scroll-hold-on", _hold_on, obj);
   evas_object_smart_callback_add(obj, "scroll-hold-off", _hold_off, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-on", _freeze_on, obj);
   evas_object_smart_callback_add(obj, "scroll-freeze-off", _freeze_off, obj);
   
   if (!smart)
     {
	static Evas_Smart_Class sc;

	evas_object_smart_clipped_smart_set(&_pan_sc);
	sc = _pan_sc;
	sc.name = "elm_photocam_pan";
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
				     _pan_set, _pan_get,
				     _pan_max_get, _pan_child_size_get);

   wd->zoom = 1;
   wd->mode = ELM_PHOTOCAM_ZOOM_MODE_MANUAL;
   
   wd->tsize = 512;
   
   wd->img = evas_object_image_add(e);
   evas_object_image_scale_hint_set(wd->img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
   evas_object_event_callback_add(wd->img, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, obj);
   evas_object_event_callback_add(wd->img, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, obj);
   evas_object_event_callback_add(wd->img, EVAS_CALLBACK_MOUSE_MOVE,
                                  _mouse_move, obj);
   evas_object_image_scale_hint_set(wd->img, EVAS_IMAGE_SCALE_HINT_STATIC);
   evas_object_smart_member_add(wd->img, wd->pan_smart);
   elm_widget_sub_object_add(obj, wd->img);
   evas_object_image_filled_set(wd->img, 1);
   evas_object_event_callback_add(wd->img, EVAS_CALLBACK_IMAGE_PRELOADED,
                                  _main_preloaded, obj);
   
   edje_object_size_min_calc(elm_smart_scroller_edje_object_get(wd->scr), 
                             &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   _sizing_eval(obj);
   return obj;
}

/**
 * Set the photo file to be shown
 *
 * This sets (and shows) the specified file (with a relative or absolute path)
 * and will return a load error (same error that
 * evas_object_image_load_error_get() will return). The image will change and
 * adjust its size at this point and begin a background load process for this
 * photo that at some time in the future will be displayed at the full quality
 * needed.
 *
 * @param obj The photocam object
 * @param file The photo file
 * @return The return error (see EVAS_LOAD_ERROR_NONE, EVAS_LOAD_ERROR_GENERIC etc.)
 *
 * @ingroup Photocam
 */
EAPI int
elm_photocam_file_set(Evas_Object *obj, const char *file)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   int w, h;
   if (!wd) return EVAS_LOAD_ERROR_GENERIC;
   if (!eina_stringshare_replace(&wd->file, file)) return EVAS_LOAD_ERROR_NONE;
   evas_object_hide(wd->img);
   evas_object_image_smooth_scale_set(wd->img, (wd->nosmooth == 0));
   evas_object_image_file_set(wd->img, NULL, NULL);
   evas_object_image_load_scale_down_set(wd->img, 0);
   evas_object_image_file_set(wd->img, wd->file, NULL);
   evas_object_image_size_get(wd->img, &w, &h);
   wd->size.imw = w;
   wd->size.imh = h;
   wd->size.w = wd->size.imw / wd->zoom;
   wd->size.h = wd->size.imh / wd->zoom;
   if (wd->zoom_animator)
     {
        wd->nosmooth--;
        if (wd->nosmooth == 0) _smooth_update(obj);
        ecore_animator_del(wd->zoom_animator);
        wd->zoom_animator = NULL;
     }
   evas_object_image_file_set(wd->img, NULL, NULL);
   evas_object_image_load_scale_down_set(wd->img, 8);
   evas_object_image_file_set(wd->img, wd->file, NULL);
   evas_object_image_preload(wd->img, 0);
   wd->main_load_pending = 1;
   grid_clearall(obj);
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
   evas_object_smart_callback_call(obj, "load", NULL);
   wd->preload_num++;
   if (wd->preload_num == 1)
     {
        edje_object_signal_emit(elm_smart_scroller_edje_object_get(wd->scr),
                                "elm,state,busy,start", "elm");
        evas_object_smart_callback_call(obj, "load,detail", NULL);
     }
     {
        double tz = wd->zoom;
        wd->zoom = 0.0;
        elm_photocam_zoom_set(wd->obj, tz);
     }
   return evas_object_image_load_error_get(wd->img);
}

/*
 * Returns the path of the current image file
 *
 * @param obj The photocam object
 * @return Returns the path 
 *
 * @ingroup Photocam
 */
EAPI const char *
elm_photocam_file_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->file;
}

/**
 * Set the zoom level of the photo
 *
 * This sets the zoom level. 1 will be 1:1 pixel for pixel. 2 will be 2:1
 * (that is 2x2 photo pixels will display as 1 on-screen pixel). 4:1 will be
 * 4x4 photo pixels as 1 screen pixel, and so on. The @p zoom parameter must
 * be greater than 0. It is usggested to stick to powers of 2. (1, 2, 4, 8,
 * 16, 32, etc.). 
 *
 * @param obj The photocam object
 * @param zoom The zoom level to set
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_zoom_set(Evas_Object *obj, double zoom)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_List *l;
   Grid *g, *g_zoom = NULL;
   Evas_Coord pw, ph, rx, ry, rw, rh;
   int z;
   int zoom_changed = 0, started = 0;
   Ecore_Animator *an;
   if (!wd) return;
   if (zoom <= (1.0 / 256.0)) zoom = (1.0 / 256.0);
   if (zoom == wd->zoom) return;
   wd->zoom = zoom;
   wd->size.ow = wd->size.w;
   wd->size.oh = wd->size.h;
   elm_smart_scroller_child_pos_get(wd->scr, &rx, &ry);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &rw, &rh);
   if ((rw <= 0) || (rh <= 0)) return;

   if (wd->mode == ELM_PHOTOCAM_ZOOM_MODE_MANUAL)
     {
        wd->size.nw = (double)wd->size.imw / wd->zoom;
        wd->size.nh = (double)wd->size.imh / wd->zoom;
     }
   else if (wd->mode == ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT)
     {
        if ((wd->size.imw < 1) || (wd->size.imh < 1))
          {
             wd->size.nw = 0;
             wd->size.nw = 0;
          }
        else
          {
             ph = (wd->size.imh * rw) / wd->size.imw;
             if (ph > rh)
               {
                  pw = (wd->size.imw * rh) / wd->size.imh;
                  ph = rh;
               }
             else
               {
                  pw = rw;
               }
             if (wd->size.imw > wd->size.imh)
               z = wd->size.imw / pw;
             else
               z = wd->size.imh / ph;
             if      (z >= 8) z = 8;
             else if (z >= 4) z = 4;
             else if (z >= 2) z = 2;
             else             z = 1;
             if (z != wd->zoom) zoom_changed = 1;
             wd->zoom = z;
             wd->size.nw = pw;
             wd->size.nh = ph;
          }
     }
   else if (wd->mode == ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL)
     {
        if ((wd->size.imw < 1) || (wd->size.imh < 1))
          {
             wd->size.nw = 0;
             wd->size.nw = 0;
          }
        else
          {
             ph = (wd->size.imh * rw) / wd->size.imw;
             if (ph < rh)
               {
                  pw = (wd->size.imw * rh) / wd->size.imh;
                  ph = rh;
               }
             else
               {
                  pw = rw;
               }
             if (wd->size.imw > wd->size.imh)
               z = wd->size.imw / pw;
             else
               z = wd->size.imh / ph;
             if      (z >= 8) z = 8;
             else if (z >= 4) z = 4;
             else if (z >= 2) z = 2;
             else             z = 1;
             if (z != wd->zoom) zoom_changed = 1;
             wd->zoom = z;
             wd->size.nw = pw;
             wd->size.nh = ph;
          }
     }
   if (wd->main_load_pending)
     {
        wd->size.w = wd->size.nw;
        wd->size.h = wd->size.nh;
        goto done;
     }
   EINA_LIST_FOREACH(wd->grids, l, g)
     {
        if (g->zoom == grid_zoom_calc(wd->zoom))
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
             EINA_LIST_FOREACH(wd->grids, l, g_zoom)
               {
                  g_zoom->dead = 1;
               }
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
        wd->size.spos.x = (double)(rx + (rw / 2)) / (double)wd->size.w;
        wd->size.spos.y = (double)(ry + (rh / 2)) / (double)wd->size.h;
     }
   else
     {
        wd->size.spos.x = 0.5;
        wd->size.spos.y = 0.5;
     }
   if (rw > wd->size.w) wd->size.spos.x = 0.5;
   if (rh > wd->size.h) wd->size.spos.y = 0.5;
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
   an = wd->zoom_animator;
   if (an)
     {
        if (!_zoom_anim(obj))
          {
             ecore_animator_del(an);
             an = NULL;
          }
     }
   if (wd->calc_job) ecore_job_del(wd->calc_job);
   wd->calc_job = ecore_job_add(_calc_job, wd);
   if (!wd->paused)
     {
        if (started)
          evas_object_smart_callback_call(obj, "zoom,start", NULL);
        if (!an)
          evas_object_smart_callback_call(obj, "zoom,stop", NULL);
     }
   if (zoom_changed)
     evas_object_smart_callback_call(obj, "zoom,change", NULL);
}

/**
 * Get the zoom level of the photo
 *
 * This returns the current zoom level of the photocam object. Note that if
 * you set the fill mode to other than ELM_PHOTOCAM_ZOOM_MODE_MANUAL
 * (which is the default), the zoom level may be changed at any time by the
 * photocam object itself to account for photo size and photocam viewpoer size
 *
 * @param obj The photocam object
 * @return The current zoom level
 *
 * @ingroup Photocam
 */
EAPI double
elm_photocam_zoom_get(const Evas_Object *obj)
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
 * Manual (ELM_PHOTOCAM_ZOOM_MODE_MANUAL) means that zoom is set manually by
 * elm_photocam_zoom_set() and will stay at that level until changed by code
 * or until zoom mode is changed. This is the default mode.
 * The Automatic modes will allow the photocam object to automatically
 * adjust zoom mode based on properties. ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT) will
 * adjust zoom so the photo fits EXACTLY inside the scroll frame with no pixels
 * outside this area. ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL will be similar but
 * ensure no pixels within the frame are left unfilled.
 *
 * @param obj The photocam object
 * @param mode The desired mode
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_zoom_mode_set(Evas_Object *obj, Elm_Photocam_Zoom_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->mode == mode) return;
   wd->mode = mode;
     {
        double tz = wd->zoom;
        wd->zoom = 0.0;
        elm_photocam_zoom_set(wd->obj, tz);
     }
}

/**
 * Get the zoom mode
 *
 * This gets the current zoom mode of the photocam object
 *
 * @param obj The photocam object
 * @return The current zoom mode
 *
 * @ingroup Photocam
 */
EAPI Elm_Photocam_Zoom_Mode
elm_photocam_zoom_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_PHOTOCAM_ZOOM_MODE_LAST;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return ELM_PHOTOCAM_ZOOM_MODE_LAST;
   return wd->mode;
}

/**
 * Get the current image pixel width and height
 *
 * This gets the current photo pixel width and height (for the original).
 * The size will be returned in the integers @p w and @p h that are pointed
 * to.
 *
 * @param obj The photocam object
 * @param w A pointer to the width return
 * @param h A pointer to the height return
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_image_size_get(const Evas_Object *obj, int *w, int *h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (w) *w = wd->size.imw;
   if (h) *h = wd->size.imh;
}

/**
 * Get the current area of the image that is currently shown
 * 
 * This gets the region 
 * 
 */
EAPI void
elm_photocam_region_get(const Evas_Object *obj, int *x, int *y, int *w, int *h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord sx, sy, sw, sh;
   if (!wd) return;
   elm_smart_scroller_child_pos_get(wd->scr, &sx, &sy);
   elm_smart_scroller_child_viewport_size_get(wd->scr, &sw, &sh);
   if (wd->size.w > 0)
     {
        if (x)
          {
             *x = (wd->size.imw * sx) / wd->size.w;
             if (*x > wd->size.imw) *x = wd->size.imw;
             else if (*x < 0) *x = 0;
          }
        if (w)
          {
             *w = (wd->size.imw * sw) / wd->size.w;
             if (*w > wd->size.imw) *w = wd->size.imw;
             else if (*w < 0) *w = 0;
          }
     }
   else
     {
	if (x) *x = 0;
	if (w) *w = 0;
     }

   if (wd->size.h > 0)
     {
        if (y)
          {
             *y = (wd->size.imh * sy) / wd->size.h;
             if (*y > wd->size.imh) *y = wd->size.imh;
             else if (*y < 0) *y = 0;
          }
        if (h)
          {
             *h = (wd->size.imh * sh) / wd->size.h;
             if (*h > wd->size.imh) *h = wd->size.imh;
             else if (*h < 0) *h = 0;
          }
     }
   else
     {
	if (y) *y = 0;
	if (h) *h = 0;
     }
}

/**
 * Set the viewed portion of the image
 *
 * This sets the region of the image to be viewed
 *
 * @param obj The photocam object
 * @param x X-coordinate of region in image original pixels
 * @param y Y-coordinate of region in image original pixels
 * @param w Width of region in image original pixels
 * @param h Height of region in image original pixels
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_image_region_show(Evas_Object *obj, int x, int y, int w, int h __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int rx, ry, rw, rh;
   if (!wd) return;
   if ((wd->size.imw < 1) || (wd->size.imh < 1)) return;
   rx = (x * wd->size.w) / wd->size.imw;
   ry = (y * wd->size.h) / wd->size.imh;
   rw = (w * wd->size.w) / wd->size.imw;
   rh = (w * wd->size.h) / wd->size.imh;
   if (rw < 1) rw = 1;
   if (rh < 1) rh = 1;
   if ((rx + rw) > wd->size.w) rx = wd->size.w - rw; 
   if ((ry + rh) > wd->size.h) ry = wd->size.h - rh;
   if (wd->zoom_animator)
     {
        wd->nosmooth--;
        ecore_animator_del(wd->zoom_animator);
        wd->zoom_animator = NULL;
        zoom_do(obj, 1.0);
        evas_object_smart_callback_call(obj, "zoom,stop", NULL);
     }
   elm_smart_scroller_child_region_show(wd->scr, rx, ry, rw, rh);
}

/**
 * Bring in the viewed portion of the image
 *
 * This brings in the region of the image over time
 *
 * @param obj The photocam object
 * @param x X-coordinate of region in image original pixels
 * @param y Y-coordinate of region in image original pixels
 * @param w Width of region in image original pixels
 * @param h Height of region in image original pixels
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_image_region_bring_in(Evas_Object *obj, int x, int y, int w, int h __UNUSED__)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int rx, ry, rw, rh;
   if (!wd) return;
   if ((wd->size.imw < 1) || (wd->size.imh < 1)) return;
   rx = (x * wd->size.w) / wd->size.imw;
   ry = (y * wd->size.h) / wd->size.imh;
   rw = (w * wd->size.w) / wd->size.imw;
   rh = (w * wd->size.h) / wd->size.imh;
   if (rw < 1) rw = 1;
   if (rh < 1) rh = 1;
   if ((rx + rw) > wd->size.w) rx = wd->size.w - rw; 
   if ((ry + rh) > wd->size.h) ry = wd->size.h - rh;
   if (wd->zoom_animator)
     {
        wd->nosmooth--;
        if (wd->nosmooth == 0) _smooth_update(obj);
        ecore_animator_del(wd->zoom_animator);
        wd->zoom_animator = NULL;
        zoom_do(obj, 1.0);
        evas_object_smart_callback_call(obj, "zoom,stop", NULL);
    }
   elm_smart_scroller_region_bring_in(wd->scr, rx, ry, rw, rh);
}

/**
 * Set the paused state for photocam
 * 
 * This sets the paused state to on (1) or off (0) for photocam. The default
 * is on. This will stop zooming using animation ch change zoom levels and
 * change instantly. This will stop any existing animations that are running.
 * 
 * @param obj The photocam object
 * @param paused The pause state to set
 *
 * @ingroup Photocam
 */
EAPI void
elm_photocam_paused_set(Evas_Object *obj, Eina_Bool paused)
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
             evas_object_smart_callback_call(obj, "zoom,stop", NULL);
          }
     }
}

/**
 * Get the paused state for photocam
 * 
 * This gets the current paused state for the photocam object.
 * 
 * @param obj The photocam object
 * @return The current paused state
 *
 * @ingroup Photocam
 */
EAPI Eina_Bool
elm_photocam_paused_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->paused;
}

/**
 * Get ther internal low-res image used for photocam
 * 
 * This gets the internal image object inside photocam. Do not modify it. It
 * is for inspection only, and hooking callbacks to. Nothing else. It may be
 * deleted at any time as well.
 *
 * @param obj The photocam object
 * @return The internal image object handle, or NULL if none exists
 *
 * @ingroup Photocam
 */
EAPI Evas_Object *
elm_photocam_internal_image_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->img;
}
