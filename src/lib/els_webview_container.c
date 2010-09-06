/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

#ifdef BOUNCING_SUPPORT

#define SMART_NAME "els_webview_container"
#define API_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
   Evas_Coord x, y, w, h;
   Evas_Coord bx, by;
   Evas_Object *smart_obj;
   Evas_Object *child_obj;
   Evas_Object *clip;
};

/* local subsystem functions */
static void _smart_child_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _smart_child_resize_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _smart_reconfigure(Smart_Data *sd);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_init(void);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;

/* externally accessible functions */
Evas_Object *
elm_smart_webview_container_add(Evas *evas)
{
   _smart_init();
   return evas_object_smart_add(evas, _smart);
}

void
_elm_smart_webview_container_child_set(Evas_Object *obj, Evas_Object *child)
{
   API_ENTRY return;
   if (child == sd->child_obj) return;
   if (sd->child_obj)
     {
	evas_object_clip_unset(sd->child_obj);
	evas_object_smart_member_del(sd->child_obj);
	evas_object_event_callback_del_full(sd->child_obj, EVAS_CALLBACK_FREE, _smart_child_del_hook, sd);
	evas_object_event_callback_del_full(sd->child_obj, EVAS_CALLBACK_RESIZE, _smart_child_resize_hook, sd);
	sd->child_obj = NULL;
     }
   if (child)
     {
	int r, g, b, a;
	sd->child_obj = child;
	_elm_smart_webview_container_set(child, obj);
	evas_object_clip_set(sd->child_obj, sd->clip); 
	_smart_reconfigure(sd);
     }
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

Eina_Bool
_elm_smart_webview_container_scroll_adjust(Evas_Object *obj, int *dx, int *dy)
{
   API_ENTRY return;
   Eina_Bool changed = EINA_FALSE;
   printf(" [ WCSA ] %d , %d vs %d, %d\n", *dx, *dy, sd->bx, sd->by);

   if (sd->bx != 0)
     {
	int xsum = sd->bx + *dx;
        if ((*dx < 0 && sd->bx > 0 && xsum < 0) ||
	      (*dx > 0 && sd->bx < 0 && xsum > 0))
	  {
	     sd->bx = 0;
	     *dx = xsum;
	  }
	else
	  {
	     sd->bx = xsum;
	     *dx = 0;
	  }
     }
   if (sd->by != 0)
     {
	int ysum = sd->by + *dy;
        if ((*dy < 0 && sd->by > 0 && ysum < 0) ||
	      (*dy > 0 && sd->by < 0 && ysum > 0))
	  {
	     sd->by = 0;
	     *dx = ysum;
	  }
	else
	  {
	     sd->by = ysum;
	     *dy = 0;
	  }
     }
#if 0
   if (sd->bx > 0 && *dx < 0)
     {
	sd->bx += *dx;
	if (sd->bx < 0)
	  {
	     *dx = sd->bx;
	     sd->bx = 0;
	  }
	else
	  *dx = 0;
	changed = EINA_TRUE;
     }
   else if (sd->bx < 0 && *dx > 0)
     {
	sd->bx += *dx;
	if (sd->bx > 0)
	  {
	     *dx = sd->bx;
	     sd->bx = 0;
	  }
	else
	  *dx = 0;
	changed = EINA_TRUE;
     }
   if (sd->by > 0 && *dy < 0)
     {
	sd->by += *dy;
	if (sd->by < 0)
	  {
	     *dy = sd->by;
	     sd->by = 0;
	  }
	else
	  *dy = 0;
	changed = EINA_TRUE;
     }
   else if (sd->by < 0 && *dy > 0)
     {
	sd->by += *dy;
	if (sd->by > 0)
	  {
	     *dy = sd->by;
	     sd->by = 0;
	  }
	else
	  *dy = 0;
	changed = EINA_TRUE;
     }
#endif
   printf(" [ WCSA(A) ] %d , %d vs %d, %d\n", *dx, *dy, sd->bx, sd->by);
   return changed;
}

void
_elm_smart_webview_container_bounce_add(Evas_Object *obj, int dx, int dy)
{
   API_ENTRY return;
   sd->bx += dx;
   sd->by += dy;
   if (sd->bx != 0 || sd->by != 0)
     _smart_reconfigure(sd);
}

void
_elm_smart_webview_container_mouse_up(Evas_Object *obj)
{
   API_ENTRY return;
   if (sd->bx != 0 || sd->by != 0)
     {
	sd->bx = 0;
	sd->by = 0;
	_smart_reconfigure(sd);
     }
}

void
_elm_smart_webview_container_decelerated_flick_get(Evas_Object *obj, int *dx, int *dy)
{
   API_ENTRY return;
   if (sd->bx != 0) *dx /= 2;
   if (sd->by != 0) *dy /= 2;
}

/* local subsystem functions */
static void
_smart_child_del_hook(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Smart_Data *sd;

   sd = data;
   sd->child_obj = NULL;
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_smart_child_resize_hook(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Smart_Data *sd;
   Evas_Coord w, h;

   sd = data;
   evas_object_geometry_get(sd->child_obj, NULL, NULL, &w, &h);
   /*if ((w != sd->child_w) || (h != sd->child_h))
     {
	sd->child_w = w;
	sd->child_h = h;
	_smart_reconfigure(sd);
     }
     */
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_smart_reconfigure(Smart_Data *sd)
{
   evas_object_move(sd->child_obj, sd->x - sd->bx, sd->y - sd->by);
   //evas_object_move(sd->child_obj, sd->x, sd->y);
}

static void
_smart_add(Evas_Object *obj)
{
   Smart_Data *sd;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->bx = 0;
   sd->by = 0;
   printf("#########################%s\n",__func__);
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   _elm_smart_pan_child_set(obj, NULL);
   free(sd);
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   printf("#########################%s\n",__func__);
   printf("            %d %d\n", x, y);
   evas_object_move(sd->clip, x, y);
   _smart_reconfigure(sd);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   printf("#########################%s\n",__func__);
   printf("            %d %d\n", w, h);
   evas_object_resize(sd->clip, w, h);
   _smart_reconfigure(sd);
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   printf("#########################%s(clip_show)\n",__func__);
   evas_object_show(sd->clip);
   // smart_obj? 
   evas_object_show(sd->child_obj);
}

static void
_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->child_obj);
}

static void
_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   printf("#########################%s\n",__func__);
   INTERNAL_ENTRY;
   evas_object_color_set(sd->child_obj, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   printf("#########################%s\n",__func__);
   INTERNAL_ENTRY;
   //evas_object_clip_set(sd->child_obj, clip);
}

static void
_smart_clip_unset(Evas_Object *obj)
{
   printf("#########################%s\n",__func__);
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->child_obj);
}

static void
_smart_init(void)
{
   if (_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _smart_add,
	       _smart_del,
	       _smart_move,
	       _smart_resize,
	       _smart_show,
	       _smart_hide,
	       _smart_color_set,
	       _smart_clip_set,
	       _smart_clip_unset,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
               NULL,
               NULL,
               NULL
	  };
	_smart = evas_smart_class_new(&sc);
     }
}
#endif
