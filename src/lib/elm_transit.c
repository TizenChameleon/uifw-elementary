#include <Elementary.h>

/**
 *
 * @defgroup Transit Transit
 * @ingroup Elementary
 *
 * Transit is designed to set the various effects for the Evas_Object such like translation, 
 * rotation, etc. For using Effects, Create transit and insert the some of effects which are 
 * interested. Each effects has the type of Elm_Effect and those can be inserted into transit. 
 * Once effects are inserted into transit, transit will manage those effects.(ex) deleting). 
*/
struct _transit
{
   Evas_Object *parent;
   Elm_Animator *animator;
   Eina_List *effect_list;
   Evas_Object *block_rect;
   void (*completion_op) (void *data, Elm_Transit *transit);
   void *completion_arg;
   Eina_Bool reserved_del:1;
};

struct _effect
{
   void (*animation_op) (void *data, Elm_Animator *animator, double frame);
   void (*begin_op) (void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt);
   void (*end_op) (void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt);
   void (*del_op) (void* data);
   unsigned int shared_cnt;
   void *user_data;
};

static Evas_Object *_create_block_rect(Evas_Object *parent);
static void _transit_animate_cb(void *data, Elm_Animator *animator, 
				double frame);
static void _transit_fx_begin(Elm_Transit *transit);
static void _transit_fx_end(Elm_Transit *transit);
static void _transit_complete_cb(void *data);
static void _transit_fx_del(Elm_Effect *effect);
//static void _transit_parent_del(void *data); 

/*
static void
_animator_parent_del(void *data)
{
	Elm_Transit *transit = data; 
	elm_transit_del(data);
}
*/

static Evas_Object *
_create_block_rect(Evas_Object *parent)
{
   Evas_Object *rect;

   Evas_Coord w, h;

   rect = evas_object_rectangle_add(evas_object_evas_get(parent));
   evas_output_size_get(evas_object_evas_get(parent), &w, &h);
   evas_object_resize(rect, w, h);
   evas_object_color_set(rect, 0, 0, 0, 0);
   return rect;
}

static void
_transit_animate_cb(void *data, Elm_Animator *animator, double frame)
{
   Eina_List *elist;

   Elm_Effect *effect;

   Elm_Transit *transit = data;

   EINA_LIST_FOREACH(transit->effect_list, elist, effect)
   {
      effect->animation_op(effect->user_data, animator, frame);
   }
}

static void
_transit_fx_begin(Elm_Transit *transit)
{
   Eina_List *elist;

   Elm_Effect *effect;

   Eina_Bool auto_reverse;

   unsigned int repeat_cnt;

   auto_reverse = elm_animator_auto_reverse_get(transit->animator);
   repeat_cnt = elm_animator_repeat_get(transit->animator);

   EINA_LIST_FOREACH(transit->effect_list, elist, effect)
   {
      if (effect->begin_op)
	 effect->begin_op(effect->user_data, auto_reverse, repeat_cnt);
   }
}

static void
_transit_fx_end(Elm_Transit *transit)
{
   Eina_List *elist;

   Elm_Effect *effect;

   Eina_Bool auto_reverse;

   unsigned int repeat_cnt;

   auto_reverse = elm_animator_auto_reverse_get(transit->animator);
   repeat_cnt = elm_animator_repeat_get(transit->animator);

   EINA_LIST_FOREACH(transit->effect_list, elist, effect)
   {
      if (effect->end_op)
	 effect->end_op(effect->user_data, auto_reverse, repeat_cnt);
   }
}

static void
_transit_complete_cb(void *data)
{
   Elm_Transit *transit = (Elm_Transit *) data;

   evas_render(evas_object_evas_get(transit->parent));

   _transit_fx_end(transit);

   if (transit->block_rect)
      evas_object_hide(transit->block_rect);

   if (transit->completion_op)
      transit->completion_op(transit->completion_arg, transit);

   if (transit->reserved_del)
     {
	transit->reserved_del = EINA_FALSE;
	elm_transit_del(transit);
     }
}

static void
_transit_fx_del(Elm_Effect *effect)
{
   if (!effect)
      return;

   --effect->shared_cnt;

   if (effect->shared_cnt > 0)
      return;

   if(effect->del_op)
	   (*effect->del_op)(effect->user_data);

   if (effect->user_data)
      free(effect->user_data);
   free(effect);
}

/**
 * Set the event blocked when transit is operating.  
 *
 * @param transit Transit object
 * @param disabled Disable or enable
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_event_block_disabled_set(Elm_Transit *transit, Eina_Bool disabled)
{
   if (!transit)
      return;

   if (disabled)
     {
	if (transit->block_rect)
	  {
	     evas_object_del(transit->block_rect);
	     transit->block_rect = NULL;
	  }
     }
   else
     {
	if (transit->block_rect == NULL)
	   transit->block_rect = _create_block_rect(transit->parent);
     }
}

/**
 * Get the value of event blockd status.
 *
 * @param transit Transit
 * @return EINA_TRUE, when event block is disabled
 *
 * @ingroup Transit 
 */
EAPI Eina_Bool
elm_transit_event_block_disabled_get(Elm_Transit *transit)
{
   if (!transit)
      return EINA_FALSE;
   return transit->block_rect ? EINA_TRUE : EINA_FALSE;
}

/**
 * Remove effect from transit.  
 *
 * @param transit	Transit
 * @param effect Effect to be removed
 * @return EINA_TRUE, if the effect is removed
 * @warning If removed effect does not inserted in any transit, it will be deleted. 
 *
 * @ingroup Transit 
 */
EAPI Eina_Bool
elm_transit_fx_remove(Elm_Transit *transit, Elm_Effect *effect)
{
   Eina_List *elist;

   Elm_Effect *_effect;

   if (!transit)
      return EINA_FALSE;

   EINA_LIST_FOREACH(transit->effect_list, elist, _effect)
   {
      if (_effect == effect)
	{
	   transit->effect_list =
	      eina_list_remove(transit->effect_list, _effect);
	   _transit_fx_del(_effect);
	   return EINA_TRUE;
	}
   }
   return EINA_FALSE;
}

/**
 * Remove all current inserted effects. 
 *
 * @param transit	Transit 
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_fx_clear(Elm_Transit *transit)
{
   Eina_List *elist;

   Elm_Effect *effect;

   if (!transit)
      return;

   EINA_LIST_FOREACH(transit->effect_list, elist, effect)
   {
      transit->effect_list = eina_list_remove(transit->effect_list, effect);
      _transit_fx_del(effect);
   }
}

/**
 * Get the list of current inseted effects. 
 *
 * @param transit	Transit
 * @return Effect list 
 *
 * @ingroup Transit 
 */
EAPI const Eina_List *
elm_transit_fx_get(Elm_Transit *transit)
{
   if (!transit)
      return NULL;
   return transit->effect_list;
}

/**
 * Set the user-callback function when the transit operation is done. 
 *
 * @param transit	Transit
 * @param op Callback function pointer
 * @param data Callback funtion user data
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_completion_callback_set(Elm_Transit *transit,
				    void (*op) (void *data,
						Elm_Transit *transit),
				    void *data)
{
   if (!transit)
      return;
   transit->completion_op = op;
   transit->completion_arg = data;
}

/**
 * Delete transit. 
 *
 * @param transit	Transit to be deleted
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_del(Elm_Transit *transit)
{
   if (!transit)
      return;
   if (elm_animator_operating_get(transit->animator))
     {
	transit->reserved_del = EINA_TRUE;
	return;
     }

   if (transit->block_rect)
      evas_object_del(transit->block_rect);

   elm_animator_del(transit->animator);
   elm_transit_fx_clear(transit);

//	if(transit->parent) 
//	{
//		evas_object_event_callback_del(transit->parent, EVAS_CALLBACK_DEL, _transit_parent_del);
//	}

   free(transit);
}

/**
 * Set the transit animation acceleration style. 
 *
 * @param transit	Transit
 * @param cs Curve style(Please refer elm_animator_curve_style_set)
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_curve_style_set(Elm_Transit *transit, Elm_Animator_Curve_Style cs)
{
   if (!transit)
      return;
   elm_animator_curve_style_set(transit->animator, cs);
}


/**
 * Add new transit. 
 *
 * @param parent Parent object
 * @return transit 
 *
 * @ingroup Transit 
 */
EAPI Elm_Transit *
elm_transit_add(Evas_Object *parent)
{
   Elm_Transit *transit = calloc(1, sizeof(Elm_Transit));

   if (!transit)
      return NULL;

   transit->animator = elm_animator_add(parent);

   if (!transit->animator)
     {
	free(transit);
	return NULL;
     }

   transit->parent = parent;
   elm_animator_operation_callback_set(transit->animator, _transit_animate_cb,
				       transit);
   elm_animator_completion_callback_set(transit->animator, _transit_complete_cb,
					transit);
   elm_transit_event_block_disabled_set(transit, EINA_FALSE);
/*
	if(parent)
	{
		evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL, _transit_parent_del, 
							 transit);
	}
*/
   return transit;
}

/**
 * Set reverse effect automatically.  
 *
 * @param transit Transit  
 * @param reverse EINA_TRUE is reverse.
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_auto_reverse_set(Elm_Transit *transit, Eina_Bool reverse)
{
   if (!transit)
      return;
   elm_animator_auto_reverse_set(transit->animator, reverse);
}

/**
 * Insert an effect into the transit. 
 *
 * @param transit Transit
 * @param effect Effect to be inserted
 * @return EINA_TRUE is success
 *
 * @ingroup Transit 
 */
EAPI Eina_Bool
elm_transit_fx_insert(Elm_Transit *transit, Elm_Effect *effect)
{
   Eina_List *elist;

   Elm_Effect *_effect;

   if (!transit)
      return EINA_FALSE;

   EINA_LIST_FOREACH(transit->effect_list, elist, _effect)
   {
      if (_effect == effect)
	 return EINA_FALSE;
   }

   ++effect->shared_cnt;
   transit->effect_list = eina_list_append(transit->effect_list, effect);

   return EINA_TRUE;
}

/**
 * Set the transit repeat count. Effect will be repeated by repeat count.
 *
 * @param transit Transit 
 * @param repeat Repeat count 
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_repeat_set(Elm_Transit *transit, unsigned int repeat)
{
   if (!transit)
      return;
   elm_animator_repeat_set(transit->animator, repeat);
}

/**
 * Stop the current transit, if the transit is operating. 
 *
 * @param transit Transit 
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_stop(Elm_Transit *transit)
{
   if (!transit)
      return;
   elm_animator_stop(transit->animator);
}

/**
 * Run the all the inserted effects.  
 *
 * @param transit Transit
 * @param duration Transit time in seconds
 *
 * @ingroup Transit 
 */
EAPI void
elm_transit_run(Elm_Transit *transit, double duration)
{
   if (!transit)
      return;
   _transit_fx_begin(transit);
   elm_animator_duration_set(transit->animator, duration);

   //Block to Top
   if (transit->block_rect)
      evas_object_show(transit->block_rect);

   elm_animator_animate(transit->animator);

   //If failed to animate.  
   if (!elm_animator_operating_get(transit->animator))
     {
	if (transit->block_rect)
	   evas_object_hide(transit->block_rect);
	_transit_fx_end(transit);
     }
}

/**
 * Pause the transit
 *
 * @param  transit Transit
 *
 * @ingroup Transit
 */
EAPI void
elm_transit_pause(Elm_Transit *transit)
{
	if(!transit)
		return;

	elm_animator_pause(transit->animator);
}

/**
 * Resume the transit
 *
 * @param  transit Transit
 *
 * @ingroup Transit
 */
EAPI void
elm_transit_resume(Elm_Transit *transit)
{
	if(!transit)
		return;

	elm_animator_resume(transit->animator);
}



/////////////////////////////////////////////////////////////////////////////////////
//Resizing FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _resizing Elm_Fx_Resizing;
static void _elm_fx_resizing_op(void *data, Elm_Animator *animator, 
				double frame);
static void _elm_fx_resizing_begin(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);

struct _resizing
{
   Evas_Object *obj;
   struct _size
   {
      Evas_Coord w, h;
   } from, to;
};

static void
_elm_fx_resizing_begin(void *data, Eina_Bool auto_reverse,
		       unsigned int repeat_cnt)
{
   Elm_Fx_Resizing *resizing = data;

   evas_object_show(resizing->obj);
   evas_object_resize(resizing->obj, resizing->from.w, resizing->from.h);
}

static void
_elm_fx_resizing_op(void *data, Elm_Animator *animator, double frame)
{
   Evas_Coord w, h;

   Elm_Fx_Resizing *resizing = data;

   w = resizing->from.w + (Evas_Coord) ((float)resizing->to.h * (float)frame);
   h = resizing->from.h + (Evas_Coord) ((float)resizing->to.w * (float)frame);
   evas_object_resize(resizing->obj, w, h);
}

/**
 * Add Resizing effect.  
 *
 * @param obj Evas_Object that effect is applying to
 * @param from_w Object width size when effect begins
 * @param from_h Object height size when effect begins
 * @param to_w Object width size when effect ends
 * @param to_h Object height size when effect ends
 * @return Resizing effect 
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_resizing_add(Evas_Object *obj, Evas_Coord from_w, Evas_Coord from_h,
		    Evas_Coord to_w, Evas_Coord to_h)
{
   Elm_Effect *effect;

   Elm_Fx_Resizing *resizing;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   resizing = calloc(1, sizeof(Elm_Fx_Resizing));
   if (!resizing)
     {
	free(effect);
	return NULL;
     }

   resizing->obj = obj;
   resizing->from.w = from_w;
   resizing->from.h = from_h;
   resizing->to.w = to_w - from_w;
   resizing->to.h = to_h - from_h;

   effect->begin_op = _elm_fx_resizing_begin;
   effect->animation_op = _elm_fx_resizing_op;
   effect->user_data = resizing;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Translation FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _translation Elm_Fx_Translation;
static void _elm_fx_translation_op(void *data, Elm_Animator *animator, 
				double frame);
static void _elm_fx_translation_begin(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_translation_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);

struct _translation
{
   Evas_Object *obj;
   struct _point
   {
      Evas_Coord x, y;
   } from, to;
};

static void
_elm_fx_translation_begin(void *data, Eina_Bool auto_reverse,
			  unsigned int repeat_cnt)
{
   Elm_Fx_Translation *translation = data;

   evas_object_show(translation->obj);
   evas_object_move(translation->obj, translation->from.x, translation->from.y);
}

static void
_elm_fx_translation_end(void *data, Eina_Bool auto_reverse,
			unsigned int repeat_cnt)
{
   Elm_Fx_Translation *translation = data;

   evas_object_move(translation->obj, translation->from.x + translation->to.x,
		    translation->from.y + translation->to.y);
}

static void
_elm_fx_translation_op(void *data, Elm_Animator *animator, double frame)
{
   Evas_Coord x, y;

   Elm_Fx_Translation *translation = data;

   x = translation->from.x +
      (Evas_Coord) ((float)translation->to.x * (float)frame);
   y = translation->from.y +
      (Evas_Coord) ((float)translation->to.y * (float)frame);
   evas_object_move(translation->obj, x, y);
}

/**
 * Add Translation effect.  
 *
 * @param obj Evas_Object that effect is applying to
 * @param from_x Position X when effect begins
 * @param from_y Position Y when effect begins
 * @param to_x Position X when effect ends
 * @param to_y Position Y when effect ends
 * @return Translation effect 
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_translation_add(Evas_Object *obj, Evas_Coord from_x, Evas_Coord from_y,
		       Evas_Coord to_x, Evas_Coord to_y)
{
   Elm_Effect *effect;

   Elm_Fx_Translation *translation;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   translation = calloc(1, sizeof(Elm_Fx_Translation));

   if (!translation)
     {
	free(effect);
	return NULL;
     }

   translation->obj = obj;
   translation->from.x = from_x;
   translation->from.y = from_y;
   translation->to.x = to_x - from_x;
   translation->to.y = to_y - from_y;

   effect->begin_op = _elm_fx_translation_begin;
   effect->end_op = _elm_fx_translation_end;
   effect->animation_op = _elm_fx_translation_op;
   effect->user_data = translation;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Zoom FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _zoom Elm_Fx_Zoom;
static void _elm_fx_zoom_op(void *data, Elm_Animator * animator, 
				double frame);
static void _elm_fx_zoom_begin(void *data, Eina_Bool reverse, 
				unsigned int repeat);
static void _elm_fx_zoom_end(void *data, Eina_Bool reverse, 
				unsigned int repeat);

struct _zoom
{
   Evas_Object *obj;
   float from, to;
};

static void
_elm_fx_zoom_begin(void *data, Eina_Bool reverse, unsigned int repeat)
{
   Elm_Fx_Zoom *zoom = data;

   evas_object_show(zoom->obj);
   _elm_fx_zoom_op(data, NULL, 0);
}

static void
_elm_fx_zoom_end(void *data, Eina_Bool reverse, unsigned int repeat)
{
   Elm_Fx_Zoom *zoom = data;

   evas_object_map_enable_set(zoom->obj, EINA_FALSE);
}

static void
_elm_fx_zoom_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Zoom *zoom;

   Evas_Map *map;

   Evas_Coord x, y, w, h;

   map = evas_map_new(4);
   if (!map)
      return;

   zoom = data;
   evas_object_geometry_get(zoom->obj, &x, &y, &w, &h);
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, zoom->obj,
						  zoom->from +
						  (frame * zoom->to));
   evas_map_util_3d_perspective(map, x + (w / 2), y + (h / 2), 0, 10000);
   evas_object_map_set(zoom->obj, map);
   evas_object_map_enable_set(zoom->obj, EINA_TRUE);
   evas_map_free(map);
}

/**
 * Add Zoom effect.  
 *
 * @param obj Evas_Object that effect is applying to
 * @param from_rate Scale rate when effect begins (1 is current rate) 
 * @param to_rate Scale rate when effect ends
 * @return Zoom effect 
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_zoom_add(Evas_Object *obj, float from_rate, float to_rate)
{
   Elm_Effect *effect;

   Elm_Fx_Zoom *zoom;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   zoom = calloc(1, sizeof(Elm_Fx_Zoom));
   if (!zoom)
     {
	free(effect);
	return NULL;
     }

   zoom->obj = obj;
   zoom->from = (10000 - (from_rate * 10000)) * (1 / from_rate);
   zoom->to = ((10000 - (to_rate * 10000)) * (1 / to_rate)) - zoom->from;
   effect->begin_op = _elm_fx_zoom_begin;
   effect->end_op = _elm_fx_zoom_end;
   effect->animation_op = _elm_fx_zoom_op;
   effect->user_data = zoom;

   return effect;

}

/////////////////////////////////////////////////////////////////////////////////////
//Flip FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _flip Elm_Fx_Flip;
static void _elm_fx_flip_op(void *data, Elm_Animator *animator, 
				double frame);
static void _elm_fx_flip_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);

struct _flip
{
   Evas_Object *front;
   Evas_Object *back;
   Elm_Fx_Flip_Axis axis;
   Eina_Bool cw:1;
};

static void
_elm_fx_flip_end(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Flip *flip = data;

   evas_object_map_enable_set(flip->front, EINA_FALSE);
   evas_object_map_enable_set(flip->back, EINA_FALSE);
}

static void
_elm_fx_flip_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Flip *flip;

   Evas_Map *map;

   float degree;

   Evas_Object *obj;

   Evas_Coord x, y, w, h;

   map = evas_map_new(4);
   if (!map)
      return;

   flip = data;

   if (flip->cw)
      degree = (float)(frame * 180);
   else
      degree = (float)(frame * -180);

   if (degree < 90 && degree > -90)
     {
	obj = flip->front;
	evas_object_hide(flip->back);
	evas_object_show(flip->front);
     }
   else
     {
	obj = flip->back;
	evas_object_hide(flip->front);
	evas_object_show(flip->back);
     }

   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, obj, 0);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   Evas_Coord half_w = (w / 2);

   Evas_Coord half_h = (h / 2);

   if (flip->axis == ELM_FX_FLIP_AXIS_Y)
     {
	if ((degree >= 90) || (degree <= -90))
	  {
	     evas_map_point_image_uv_set(map, 0, w, 0);
	     evas_map_point_image_uv_set(map, 1, 0, 0);
	     evas_map_point_image_uv_set(map, 2, 0, h);
	     evas_map_point_image_uv_set(map, 3, w, h);
	  }
	evas_map_util_3d_rotate(map, 0, degree, 0, x + half_w, y + half_h, 0);
     }
   else
     {
	if ((degree >= 90) || (degree <= -90))
	  {
	     evas_map_point_image_uv_set(map, 0, 0, h);
	     evas_map_point_image_uv_set(map, 1, w, h);
	     evas_map_point_image_uv_set(map, 2, w, 0);
	     evas_map_point_image_uv_set(map, 3, 0, 0);
	  }
	evas_map_util_3d_rotate(map, degree, 0, 0, x + half_w, y + half_h, 0);
     }
   evas_map_util_3d_perspective(map, x + half_w, y + half_h, 0, 10000);
   evas_object_map_enable_set(flip->front, EINA_TRUE);
   evas_object_map_enable_set(flip->back, EINA_TRUE);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}

/**
 * Add Flip effect.  
 *
 * @param front Front surface object 
 * @param back Back surface object
 * @param axis Flipping Axis(X or Y)
 * @param cw Flipping Direction. EINA_TRUE is clock-wise 
 * @return Flip effect 
 * 
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_flip_add(Evas_Object *front, Evas_Object *back, Elm_Fx_Flip_Axis axis,
		Eina_Bool cw)
{
   Elm_Effect *effect;

   Elm_Fx_Flip *flip;

   if ((!front) || (!back))
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   flip = calloc(1, sizeof(Elm_Fx_Flip));

   if (!flip)
     {
	free(effect);
	return NULL;
     }

   flip->front = front;
   flip->back = back;
   flip->cw = cw;
   flip->axis = axis;
   effect->end_op = _elm_fx_flip_end;
   effect->animation_op = _elm_fx_flip_op;
   effect->user_data = flip;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//ResizableFlip FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _resizable_flip Elm_Fx_ResizableFlip;
static void _elm_fx_resizable_flip_begin(void *data, Eina_Bool reverse, 
				unsigned int repeat);
static void _elm_fx_resizable_flip_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_resizable_flip_op(void *data, Elm_Animator *animator,
				      double frame);
static void _set_image_uv_by_axis_y(Evas_Map *map, Elm_Fx_ResizableFlip *flip, 
				float degree);
static void _set_image_uv_by_axis_x(Evas_Map *map, Elm_Fx_ResizableFlip *flip, 
				float degree);

struct _resizable_flip
{
   Evas_Object *front;
   Evas_Object *back;
   Elm_Fx_Flip_Axis axis;
   struct _vector2d
   {
      float x, y;
   } from_pos, from_size, to_pos, to_size;
   Eina_Bool cw:1;
};

static void
_elm_fx_resizable_flip_begin(void *data, Eina_Bool reverse, unsigned int repeat)
{
   Elm_Fx_ResizableFlip *resizable_flip = data;

   evas_object_show(resizable_flip->front);
   _elm_fx_resizable_flip_op(data, NULL, 0);
}

static void
_elm_fx_resizable_flip_end(void *data, Eina_Bool auto_reverse,
			   unsigned int repeat_cnt)
{
   Elm_Fx_ResizableFlip *resizable_flip = data;

   evas_object_map_enable_set(resizable_flip->front, EINA_FALSE);
   evas_object_map_enable_set(resizable_flip->back, EINA_FALSE);
}

static void
_set_image_uv_by_axis_y(Evas_Map *map, Elm_Fx_ResizableFlip *flip,
			float degree)
{
   if ((degree >= 90) || (degree <= -90))
     {
	evas_map_point_image_uv_set(map, 0,
				    (flip->from_size.x * 2) + flip->to_size.x,
				    0);
	evas_map_point_image_uv_set(map, 1, 0, 0);
	evas_map_point_image_uv_set(map, 2, 0,
				    (flip->from_size.y * 2) + flip->to_size.y);
	evas_map_point_image_uv_set(map, 3,
				    (flip->from_size.x * 2) + flip->to_size.x,
				    (flip->from_size.y * 2) + flip->to_size.y);
     }
   else
     {
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, flip->from_size.x, 0);
	evas_map_point_image_uv_set(map, 2, flip->from_size.x,
				    flip->from_size.y);
	evas_map_point_image_uv_set(map, 3, 0, flip->to_size.y);
     }
}

static void
_set_image_uv_by_axis_x(Evas_Map *map, Elm_Fx_ResizableFlip *flip,
			float degree)
{
   if ((degree >= 90) || (degree <= -90))
     {
	evas_map_point_image_uv_set(map, 0, 0,
				    (flip->from_size.y * 2) + flip->to_size.y);
	evas_map_point_image_uv_set(map, 1,
				    (flip->from_size.x * 2) + flip->to_size.x,
				    (flip->from_size.y * 2) + flip->to_size.y);
	evas_map_point_image_uv_set(map, 2,
				    (flip->from_size.x * 2) + flip->to_size.x,
				    0);
	evas_map_point_image_uv_set(map, 3, 0, 0);
     }
   else
     {
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, flip->from_size.x, 0);
	evas_map_point_image_uv_set(map, 2, flip->from_size.x,
				    flip->from_size.y);
	evas_map_point_image_uv_set(map, 3, 0, flip->to_size.y);
     }
}

static void
_elm_fx_resizable_flip_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_ResizableFlip *resizable_flip;

   Evas_Map *map;

   float degree;

   Evas_Object *obj;

   float x, y, w, h;

   Evas_Coord half_w, half_h;

   map = evas_map_new(4);
   if (!map)
      return;

   resizable_flip = data;

   if (resizable_flip->cw)
      degree = (float)(frame * 180);
   else
      degree = (float)(frame * -180);

   if ((degree < 90) && (degree > -90))
     {
	obj = resizable_flip->front;
	evas_object_hide(resizable_flip->back);
	evas_object_show(resizable_flip->front);
     }
   else
     {
	obj = resizable_flip->back;
	evas_object_hide(resizable_flip->front);
	evas_object_show(resizable_flip->back);
     }

   evas_map_smooth_set(map, EINA_TRUE);

   x = resizable_flip->from_pos.x + (resizable_flip->to_pos.x * frame);
   y = resizable_flip->from_pos.y + (resizable_flip->to_pos.y * frame);
   w = resizable_flip->from_size.x + (resizable_flip->to_size.x * frame);
   h = resizable_flip->from_size.y + (resizable_flip->to_size.y * frame);
   evas_map_point_coord_set(map, 0, x, y, 0);
   evas_map_point_coord_set(map, 1, x + w, y, 0);
   evas_map_point_coord_set(map, 2, x + w, y + h, 0);
   evas_map_point_coord_set(map, 3, x, y + h, 0);

   half_w = (Evas_Coord) (w / 2);
   half_h = (Evas_Coord) (h / 2);

   if (resizable_flip->axis == ELM_FX_FLIP_AXIS_Y)
     {
	_set_image_uv_by_axis_y(map, resizable_flip, degree);
	evas_map_util_3d_rotate(map, 0, degree, 0, x + half_w, y + half_h, 0);
     }
   else
     {
	_set_image_uv_by_axis_x(map, resizable_flip, degree);
	evas_map_util_3d_rotate(map, degree, 0, 0, x + half_w, y + half_h, 0);
     }

   evas_map_util_3d_perspective(map, x + half_w, y + half_h, 0, 10000);
   evas_object_map_enable_set(resizable_flip->front, EINA_TRUE);
   evas_object_map_enable_set(resizable_flip->back, EINA_TRUE);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}

/**
 * Add ResizbleFlip effect. the size of each surface objects are interpolated automatically.
 *
 * @param front Front surface object 
 * @param back Back surface object
 * @param axis Flipping Axis.(X or Y)  
 * @param cw Flipping Direction. EINA_TRUE is clock-wise
 * @return Flip effect 
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_resizable_flip_add(Evas_Object *front, Evas_Object *back,
			  Elm_Fx_Flip_Axis axis, Eina_Bool cw)
{
   Elm_Fx_ResizableFlip *resizable_flip;

   Elm_Effect *effect;

   Evas_Coord front_x, front_y, front_w, front_h;

   Evas_Coord back_x, back_y, back_w, back_h;

   if (!front || !back)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   resizable_flip = calloc(1, sizeof(Elm_Fx_ResizableFlip));

   if (!resizable_flip)
     {
	free(effect);
	return NULL;
     }

   resizable_flip->front = front;
   resizable_flip->back = back;
   resizable_flip->cw = cw;
   resizable_flip->axis = axis;

   evas_object_geometry_get(resizable_flip->front, &front_x, &front_y, &front_w,
			    &front_h);
   evas_object_geometry_get(resizable_flip->back, &back_x, &back_y, &back_w,
			    &back_h);

   resizable_flip->from_pos.x = front_x;
   resizable_flip->from_pos.y = front_y;
   resizable_flip->to_pos.x = back_x - front_x;
   resizable_flip->to_pos.y = back_y - front_y;

   resizable_flip->from_size.x = front_w;
   resizable_flip->from_size.y = front_h;
   resizable_flip->to_size.x = back_w - front_w;
   resizable_flip->to_size.y = back_h - front_h;

   effect->begin_op = _elm_fx_resizable_flip_begin;
   effect->end_op = _elm_fx_resizable_flip_end;
   effect->animation_op = _elm_fx_resizable_flip_op;
   effect->user_data = resizable_flip;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Wipe FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _wipe Elm_Fx_Wipe;
static void _elm_fx_wipe_op(void *data, Elm_Animator *animator, 
				double frame);
static void _elm_fx_wipe_begin(void *data, Eina_Bool auto_repeat, 
				unsigned int repeat_cnt);
static void _elm_fx_wipe_end(void *data, Eina_Bool auto_repeat, 
				unsigned int repeat_cnt);
static void _elm_fx_wipe_hide(Evas_Map * map, Elm_Fx_Wipe_Dir dir, 
				float x, float y, float w, float h, float frame);
static void _elm_fx_wipe_show(Evas_Map *map, Elm_Fx_Wipe_Dir dir, 
				float x, float y, float w, float h, float frame);

struct _wipe
{
   Evas_Object *obj;
   Elm_Fx_Wipe_Type type;
   Elm_Fx_Wipe_Dir dir;
};

static void
_elm_fx_wipe_begin(void *data, Eina_Bool auto_repeat, unsigned int repeat_cnt)
{
   Elm_Fx_Wipe *wipe = data;

   evas_object_show(wipe->obj);
   _elm_fx_wipe_op(data, NULL, 0);
}

static void
_elm_fx_wipe_end(void *data, Eina_Bool auto_repeat, unsigned int repeat_cnt)
{
   Elm_Fx_Wipe *wipe = data;

   evas_object_map_enable_set(wipe->obj, EINA_FALSE);
}

static void
_elm_fx_wipe_hide(Evas_Map * map, Elm_Fx_Wipe_Dir dir, float x, float y,
		  float w, float h, float frame)
{
   float w2, h2;

   switch (dir)
     {
     case ELM_FX_WIPE_DIR_LEFT:
	w2 = w - (w * frame);
	h2 = (y + h);
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, w2, 0);
	evas_map_point_image_uv_set(map, 2, w2, h);
	evas_map_point_image_uv_set(map, 3, 0, h);
	evas_map_point_coord_set(map, 0, x, y, 0);
	evas_map_point_coord_set(map, 1, x + w2, y, 0);
	evas_map_point_coord_set(map, 2, x + w2, h2, 0);
	evas_map_point_coord_set(map, 3, x, h2, 0);
	break;
     case ELM_FX_WIPE_DIR_RIGHT:
	w2 = (w * frame);
	h2 = (y + h);
	evas_map_point_image_uv_set(map, 0, w2, 0);
	evas_map_point_image_uv_set(map, 1, w, 0);
	evas_map_point_image_uv_set(map, 2, w, h);
	evas_map_point_image_uv_set(map, 3, w2, h);
	evas_map_point_coord_set(map, 0, x + w2, y, 0);
	evas_map_point_coord_set(map, 1, x + w, y, 0);
	evas_map_point_coord_set(map, 2, x + w, h2, 0);
	evas_map_point_coord_set(map, 3, x + w2, h2, 0);
	break;
     case ELM_FX_WIPE_DIR_UP:
	w2 = (x + w);
	h2 = h - (h * frame);
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, w, 0);
	evas_map_point_image_uv_set(map, 2, w, h2);
	evas_map_point_image_uv_set(map, 3, 0, h2);
	evas_map_point_coord_set(map, 0, x, y, 0);
	evas_map_point_coord_set(map, 1, w2, y, 0);
	evas_map_point_coord_set(map, 2, w2, h2, 0);
	evas_map_point_coord_set(map, 3, x, h2, 0);
	break;
     case ELM_FX_WIPE_DIR_DOWN:
	w2 = (x + w);
	h2 = (h * frame);
	evas_map_point_image_uv_set(map, 0, 0, h2);
	evas_map_point_image_uv_set(map, 1, w, h2);
	evas_map_point_image_uv_set(map, 2, w, h);
	evas_map_point_image_uv_set(map, 3, 0, h);
	evas_map_point_coord_set(map, 0, x, y + h2, 0);
	evas_map_point_coord_set(map, 1, w2, y + h2, 0);
	evas_map_point_coord_set(map, 2, w2, y + h, 0);
	evas_map_point_coord_set(map, 3, x, y + h, 0);
	break;
     default:
	break;
     }

   evas_map_util_3d_perspective(map, x + (w / 2), y + (h / 2), 0, 10000);
}

static void
_elm_fx_wipe_show(Evas_Map *map, Elm_Fx_Wipe_Dir dir, float x, float y,
		  float w, float h, float frame)
{
   float w2, h2;

   switch (dir)
     {
     case ELM_FX_WIPE_DIR_LEFT:
	w2 = (w - (w * frame));
	h2 = (y + h);
	evas_map_point_image_uv_set(map, 0, w2, 0);
	evas_map_point_image_uv_set(map, 1, w, 0);
	evas_map_point_image_uv_set(map, 2, w, h);
	evas_map_point_image_uv_set(map, 3, w2, h);
	evas_map_point_coord_set(map, 0, x + w2, y, 0);
	evas_map_point_coord_set(map, 1, w, y, 0);
	evas_map_point_coord_set(map, 2, w, h2, 0);
	evas_map_point_coord_set(map, 3, x + w2, h2, 0);
	break;
     case ELM_FX_WIPE_DIR_RIGHT:
	w2 = (w * frame);
	h2 = (y + h);
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, w2, 0);
	evas_map_point_image_uv_set(map, 2, w2, h);
	evas_map_point_image_uv_set(map, 3, 0, h);
	evas_map_point_coord_set(map, 0, x, y, 0);
	evas_map_point_coord_set(map, 1, x + w2, y, 0);
	evas_map_point_coord_set(map, 2, x + w2, h2, 0);
	evas_map_point_coord_set(map, 3, x, h2, 0);
	break;
     case ELM_FX_WIPE_DIR_UP:
	w2 = (x + w);
	h2 = (h - (h * frame));
	evas_map_point_image_uv_set(map, 0, 0, h2);
	evas_map_point_image_uv_set(map, 1, w, h2);
	evas_map_point_image_uv_set(map, 2, w, h);
	evas_map_point_image_uv_set(map, 3, 0, h);
	evas_map_point_coord_set(map, 0, x, y + h2, 0);
	evas_map_point_coord_set(map, 1, w2, y + h2, 0);
	evas_map_point_coord_set(map, 2, w2, y + h, 0);
	evas_map_point_coord_set(map, 3, x, y + h, 0);
	break;
     case ELM_FX_WIPE_DIR_DOWN:
	w2 = (x + w);
	h2 = (h * frame);
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_image_uv_set(map, 1, w, 0);
	evas_map_point_image_uv_set(map, 2, w, h2);
	evas_map_point_image_uv_set(map, 3, 0, h2);
	evas_map_point_coord_set(map, 0, x, y, 0);
	evas_map_point_coord_set(map, 1, w2, y, 0);
	evas_map_point_coord_set(map, 2, w2, y + h2, 0);
	evas_map_point_coord_set(map, 3, x, y + h2, 0);
	break;
     default:
	break;
     }

   evas_map_util_3d_perspective(map, x + (w / 2), y + (h / 2), 0, 10000);
}

static void
_elm_fx_wipe_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Wipe *wipe;

   Evas_Map *map;

   Evas_Coord _x, _y, _w, _h;

   map = evas_map_new(4);
   if (!map)
      return;

   wipe = data;
   evas_map_smooth_set(map, EINA_TRUE);
   evas_object_geometry_get(wipe->obj, &_x, &_y, &_w, &_h);

   if (wipe->type == ELM_FX_WIPE_TYPE_SHOW)
      _elm_fx_wipe_show(map, wipe->dir, _x, _y, _w, _h, (float)frame);
   else
      _elm_fx_wipe_hide(map, wipe->dir, _x, _y, _w, _h, (float)frame);

   evas_object_map_enable_set(wipe->obj, EINA_TRUE);
   evas_object_map_set(wipe->obj, map);
   evas_map_free(map);
}

/**
 * Add Wipe effect.  
 *
 * @param obj Evas_Object that effect is applying to
 * @param type Wipe type. Hide or show
 * @param dir Wipe Direction
 * @return Wipe effect
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_wipe_add(Evas_Object *obj, Elm_Fx_Wipe_Type type, Elm_Fx_Wipe_Dir dir)
{
   Elm_Effect *effect;

   Elm_Fx_Wipe *wipe;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   wipe = calloc(1, sizeof(Elm_Fx_Wipe));
   if (!wipe)
     {
	free(effect);
	return NULL;
     }

   wipe->obj = obj;
   wipe->type = type;
   wipe->dir = dir;
   effect->begin_op = _elm_fx_wipe_begin;
   effect->end_op = _elm_fx_wipe_end;
   effect->animation_op = _elm_fx_wipe_op;
   effect->user_data = wipe;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Color FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _color Elm_Fx_Color;
static void _elm_fx_color_begin(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt);
static void _elm_fx_color_op(void *data, Elm_Animator *animator, double frame);

struct _color
{
   Evas_Object *obj;
   struct _unsigned_color
   {
      unsigned int r, g, b, a;
   } from;
   struct _signed_color
   {
      int r, g, b, a;
   } to;
};

static void
_elm_fx_color_begin(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Color *color = data;

   evas_object_show(color->obj);
}

static void
_elm_fx_color_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Color *color;

   unsigned int r, g, b, a;

   color = data;
   r = (color->from.r + (int)((float)color->to.r * frame));
   g = (color->from.g + (int)((float)color->to.g * frame));
   b = (color->from.b + (int)((float)color->to.b * frame));
   a = (color->from.a + (int)((float)color->to.a * frame));

   evas_object_color_set(color->obj, r, g, b, a);
}

/**
 * Add Color effect.  
 *
 * @param  obj           Evas_Object that effect is applying to
 * @param  from_r        RGB R when effect begins
 * @param  from_g        RGB G when effect begins 
 * @param  from_b        RGB B when effect begins
 * @param  from_a        RGB A when effect begins
 * @param  to_r          RGB R when effect ends
 * @param  to_g          RGB G when effect ends
 * @param  to_b          RGB B when effect ends
 * @param  to_a          RGB A when effect ends
 * @return               Color Effect
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_color_add(Evas_Object *obj, unsigned int from_r, unsigned int from_g,
		 unsigned int from_b, unsigned int from_a, unsigned int to_r,
		 unsigned int to_g, unsigned int to_b, unsigned int to_a)
{
   Elm_Effect *effect;

   Elm_Fx_Color *color;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   color = calloc(1, sizeof(Elm_Fx_Color));
   if (!color)
     {
	free(effect);
	return NULL;
     }

   color->obj = obj;
   color->from.r = from_r;
   color->from.g = from_g;
   color->from.b = from_b;
   color->from.a = from_a;
   color->to.r = to_r - from_r;
   color->to.g = to_g - from_g;
   color->to.b = to_b - from_b;
   color->to.a = to_a - from_a;

   effect->begin_op = _elm_fx_color_begin;
   effect->animation_op = _elm_fx_color_op;
   effect->user_data = color;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Fade FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _fade Elm_Fx_Fade;
static void _elm_fx_fade_begin(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_fade_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_fade_op(void *data, Elm_Animator *animator, 
				double frame);

struct _fade
{
   Evas_Object *before;
   Evas_Object *after;
   struct _signed_color before_color, after_color;
   int before_alpha;
   int after_alpha;
   Eina_Bool inversed:1;
};

static void
_elm_fx_fade_begin(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Fade *fade = data;

   fade->inversed = EINA_FALSE;
}

static void
_elm_fx_fade_end(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Fade *fade = data;

   evas_object_color_set(fade->before, fade->before_color.r,
			 fade->before_color.g, fade->before_color.b,
			 fade->before_color.a);
   evas_object_color_set(fade->after, fade->after_color.r, fade->after_color.g,
			 fade->after_color.b, fade->after_color.a);
}

static void
_elm_fx_fade_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Fade *fade;

   float _frame;

   fade = data;

   if (frame < 0.5)
     {
	if (!fade->inversed)
	  {
	     evas_object_hide(fade->after);
	     evas_object_show(fade->before);
	     fade->inversed = EINA_TRUE;
	  }

	_frame = (1 - (frame * 2));

	evas_object_color_set(fade->before, fade->before_color.r * _frame,
			      fade->before_color.g * _frame,
			      fade->before_color.b * _frame,
			      fade->before_color.a + fade->before_alpha * (1 -
									   _frame));
     }
   else
     {
	if (fade->inversed)
	  {
	     evas_object_hide(fade->before);
	     evas_object_show(fade->after);
	     fade->inversed = EINA_FALSE;
	  }

	_frame = ((frame - 0.5) * 2);

	evas_object_color_set(fade->after, fade->after_color.r * _frame,
			      fade->after_color.g * _frame,
			      fade->after_color.b * _frame,
			      fade->after_color.a + fade->after_alpha * (1 -
									 _frame));
     }

}

/**
 * Add Fade effect  
 *
 * @param before Evas Object before fade in 
 * @param after Evas Object after fade out 
 * @return Fade effect
 * 
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_fade_add(Evas_Object *before, Evas_Object *after)
{
   Elm_Effect *effect;

   Elm_Fx_Fade *fade;

   if ((!before) && (!after))
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   fade = calloc(1, sizeof(Elm_Fx_Fade));

   if (!fade)
     {
	free(effect);
	return NULL;
     }

   evas_object_color_get(before, &fade->before_color.r, &fade->before_color.g,
			 &fade->before_color.b, &fade->before_color.a);
   evas_object_color_get(after, &fade->after_color.r, &fade->after_color.g,
			 &fade->after_color.b, &fade->after_color.a);

   fade->before = before;
   fade->after = after;
   fade->before_alpha = (255 - fade->before_color.a);
   fade->after_alpha = (255 - fade->after_color.a);

   effect->begin_op = _elm_fx_fade_begin;
   effect->end_op = _elm_fx_fade_end;
   effect->animation_op = _elm_fx_fade_op;
   effect->user_data = fade;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Blend FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _blend Elm_Fx_Blend;
static void _elm_fx_blend_begin(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_blend_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_blend_op(void *data, Elm_Animator *animator, 
				double frame);

struct _blend
{
   Evas_Object *before;
   Evas_Object *after;
   struct _signed_color from, to;
};

static void
_elm_fx_blend_begin(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Blend *blend = data;

   evas_object_show(blend->before);
}

static void
_elm_fx_blend_end(void *data, Eina_Bool auto_reverse, unsigned int repeat_cnt)
{
   Elm_Fx_Blend *blend = data;

   evas_object_color_set(blend->before, blend->from.r, blend->from.g,
			 blend->from.b, blend->from.a);
   evas_object_color_set(blend->after, blend->to.r, blend->to.g, blend->to.b,
			 blend->to.a);
   if (!auto_reverse)
      evas_object_hide(blend->before);
   else
      evas_object_hide(blend->after);
}

static void
_elm_fx_blend_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Blend *blend = data;

   evas_object_show(blend->after);
   evas_object_color_set(blend->before, (int)(blend->from.r * (1 - frame)),
			 (int)(blend->from.g * (1 - frame)),
			 (int)(blend->from.b * (1 - frame)),
			 (int)(blend->from.a * (1 - frame)));
   evas_object_color_set(blend->after, (int)(blend->to.r * frame),
			 (int)(blend->to.g * frame), (int)(blend->to.b * frame),
			 (int)(blend->to.a * frame));
}

/**
 * Add Blend effect  
 *
 * @param before Evas Object before blending
 * @param after Evas Object after blending 
 * @return Blend effect
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_blend_add(Evas_Object *before, Evas_Object *after)
{
   Elm_Effect *effect;

   Elm_Fx_Blend *blend;

   if ((!before) && (!after))
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   blend = calloc(1, sizeof(Elm_Fx_Blend));
   if (!blend)
     {
	free(effect);
	return NULL;
     }

   blend->before = before;
   blend->after = after;
   evas_object_color_get(before, &blend->from.r, &blend->from.g, &blend->from.b,
			 &blend->from.a);
   evas_object_color_get(after, &blend->to.r, &blend->to.g, &blend->to.b,
			 &blend->to.a);

   effect->begin_op = _elm_fx_blend_begin;
   effect->end_op = _elm_fx_blend_end;
   effect->animation_op = _elm_fx_blend_op;
   effect->user_data = blend;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
//Rotation FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _rotation Elm_Fx_Rotation;
static void _elm_fx_rotation_begin(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_rotation_end(void *data, Eina_Bool auto_reverse, 
				unsigned int repeat_cnt);
static void _elm_fx_rotation_op(void *data, Elm_Animator *animator, 
				double frame);

struct _rotation
{
   Evas_Object *obj;
   Eina_Bool cw;
   float from, to;
};

static void
_elm_fx_rotation_begin(void *data, Eina_Bool auto_reverse,
		       unsigned int repeat_cnt)
{
   Elm_Fx_Rotation *rotation = data;

   evas_object_show(rotation->obj);
}

static void
_elm_fx_rotation_end(void *data, Eina_Bool auto_reverse,
		     unsigned int repeat_cnt)
{
   Elm_Fx_Rotation *rotation = data;

   evas_object_map_enable_set(rotation->obj, EINA_FALSE);
}

static void
_elm_fx_rotation_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Rotation *rotation;

   Evas_Map *map;

   Evas_Coord x, y, w, h;

   float degree;

   float half_w, half_h;

   map = evas_map_new(4);
   if (!map)
      return;

   rotation = data;

   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, rotation->obj, 0);
   degree = rotation->from + (float)(frame * rotation->to);

   if (!rotation->cw)
      degree *= -1;

   evas_object_geometry_get(rotation->obj, &x, &y, &w, &h);

   half_w = (float)w *0.5;

   half_h = (float)h *0.5;

   evas_map_util_3d_rotate(map, 0, 0, degree, x + half_w, y + half_h, 0);
   evas_map_util_3d_perspective(map, x + half_w, y + half_h, 0, 10000);
   evas_object_map_enable_set(rotation->obj, EINA_TRUE);
   evas_object_map_set(rotation->obj, map);
   evas_map_free(map);
}

/**
 * Add Rotation effect
 *
 * @param obj Evas_Object that effect is applying to 
 * @param from degree Degree when effect begins
 * @param to_degree Degree when effect is ends
 * @param cw Rotation direction. EINA_TRUE is clock wise
 * @return Rotation effect
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_rotation_add(Evas_Object *obj, float from_degree, float to_degree,
		    Eina_Bool cw)
{
   Elm_Effect *effect;

   Elm_Fx_Rotation *rotation;

   if (!obj)
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   rotation = calloc(1, sizeof(Elm_Fx_Rotation));

   if (!rotation)
     {
	free(effect);
	return NULL;
     }

   rotation->obj = obj;
   rotation->from = from_degree;
   rotation->to = to_degree - from_degree;
   rotation->cw = cw;

   effect->begin_op = _elm_fx_rotation_begin;
   effect->end_op = _elm_fx_rotation_end;
   effect->animation_op = _elm_fx_rotation_op;
   effect->user_data = rotation;

   return effect;
}

/////////////////////////////////////////////////////////////////////////////////////
// ImageAnimation FX
/////////////////////////////////////////////////////////////////////////////////////
typedef struct _image_animation Elm_Fx_Image_Animation;
static void _elm_fx_image_animation_begin(void *data, Eina_Bool auto_reverse,
		       unsigned int repeat_cnt);
static void _elm_fx_image_animation_op(void *data, Elm_Animator *animator, 
				double frame);
EAPI Elm_Effect *elm_fx_image_animation_add(Evas_Object *obj, const char **images, 
				unsigned int item_num);

struct _image_animation
{
   Evas_Object *obj;
   char **images;
   int img_cnt;
};


static void
_elm_fx_image_animation_begin(void *data, Eina_Bool auto_reverse,
		       unsigned int repeat_cnt)
{
   Elm_Fx_Image_Animation *image_animation = data;
   evas_object_show(image_animation->obj);
}


static void
_elm_fx_image_animation_op(void *data, Elm_Animator *animator, double frame)
{
   Elm_Fx_Image_Animation *image_animation = (Elm_Fx_Image_Animation *) data;
   elm_icon_file_set(image_animation->obj,
		     image_animation->images[ (int) floor(frame * image_animation->img_cnt) ], NULL);
}

static void
_elm_fx_image_animation_del(void *data)
{
	int idx;
	Elm_Fx_Image_Animation *image_animation = data;

	for(idx = 0; idx < image_animation->img_cnt; ++idx ) {
		eina_stringshare_del(image_animation->images[ idx ]);
	}

	free(image_animation->images);
}

/**
 * Add image_animation effect.  
 *
 * @param obj Icon object
 * @param images Array of image file path. 
 * @param img_cnt Count of image. 
 * @return ImageAnimation effect.
 *
 * @ingroup Transit 
 */
EAPI Elm_Effect *
elm_fx_image_animation_add(Evas_Object *obj, const char **images,
			  unsigned int img_cnt)
{
   Elm_Effect *effect;
   Elm_Fx_Image_Animation *image_animation;
   int idx;

   if ((!obj) || !images || !(*images))
      return NULL;

   effect = calloc(1, sizeof(Elm_Effect));
   if (!effect)
      return NULL;

   image_animation = calloc(1, sizeof(Elm_Fx_Image_Animation));

   if (!image_animation)
     {
	free(effect);
	return NULL;
     }

   image_animation->obj = obj;
   image_animation->images = calloc( img_cnt, sizeof(char*));
   for(idx = 0; idx < img_cnt; ++idx )
	   image_animation->images[ idx ] = eina_stringshare_add( images[ idx ] );

   image_animation->img_cnt = img_cnt;

   effect->begin_op = _elm_fx_image_animation_begin;
   effect->animation_op = _elm_fx_image_animation_op;
   effect->del_op = _elm_fx_image_animation_del;
   effect->user_data = image_animation;

   return effect;
}
