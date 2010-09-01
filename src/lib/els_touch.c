/*
 *
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include <math.h>
#include "elm_priv.h"

#define SMART_NAME "elm_touch"
#define API_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;

#define PRESS_TIME          250   // ms
#define RELEASE_TIME         50   // ms
#define LONG_HOLD_TIME      500   // ms

#define N_FINGER             3
#define DEFAULT_FRAMERATE   60
#define DOUBLE_ERROR        0.00001

// for one finger
#define DBL_TAP_DISTANCE    30    // pixel
#define DRAG_THRESHOLD       3    // pixel
#define INIT_DRAG_THRESHOLD 15    // pixel
#define MOVE_HISTORY_SIZE    5
#define MAX_MOVE_DISTANCE   15
#define FLICK_THRESHOLD      5

// for two finger
#define MOVE_THRESHOLD      15
#define FINGER_DISTANCE     10

typedef struct _Mouse_Data Mouse_Data;

struct _Mouse_Data
{
   Evas_Coord x;
   Evas_Coord y;
   int time;
   int device;
};

typedef enum _Two_Drag_Mode
{
   TWO_DRAG_NONE,
   TWO_DRAG_PINCH,
   TWO_DRAG_VERTICAL,
   TWO_DRAG_HORIZONTAL,
} Two_Drag_Mode;

typedef struct _Two_Mouse_Data Two_Mouse_Data;

struct _Two_Mouse_Data
{
   Evas_Point first;
   Evas_Point second;
   Two_Drag_Mode mode;
};

typedef struct _Three_Mouse_Data Three_Mouse_Data;

struct _Three_Mouse_Data
{
   Evas_Point first;
   Evas_Point second;
   Evas_Point third;
};

typedef enum _Touch_State
{
   TOUCH_STATE_NONE,
   TOUCH_STATE_DOWN,
   TOUCH_STATE_DOWN_DURING_DRAG,
   TOUCH_STATE_DOWN_UP,
   TOUCH_STATE_DOWN_UP_DOWN,
   TOUCH_STATE_HOLD,
   TOUCH_STATE_DRAG,
   TOUCH_STATE_TWO_DOWN,
   TOUCH_STATE_TWO_DOWN_DURING_DRAG,
   TOUCH_STATE_TWO_DRAG,
   TOUCH_STATE_THREE_DOWN
} Touch_State;

typedef enum _One_Drag_Mode
{
   ONE_DRAG_NONE,
   ONE_DRAG_VERTICAL,
   ONE_DRAG_HORIZONTAL
} One_Drag_Mode;

typedef struct _Flick_Data Flick_Data;

struct _Flick_Data
{
   int flick_index;
   Evas_Coord_Point last;
   Evas_Coord_Point avg_distance;
};

typedef struct _Mouse_Diff_Data Mouse_Diff_Data;

struct _Mouse_Diff_Data
{
   Evas_Coord dx, dy;
   double time;
};

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
   Evas_Object *smart_obj;
   Evas_Object *child_obj;

   Eina_Bool running;

   int screen_angle;
   Touch_State state;
   One_Drag_Mode one_drag_mode;
   Eina_Bool is_one_drag_mode;
   Two_Drag_Mode two_drag_mode;
   int numOfTouch;

   // for flick
   int last_move_history_index;
   int move_history_count;
   Flick_Data flick_data;
   Mouse_Diff_Data move_history[MOVE_HISTORY_SIZE];

   Mouse_Data first_down[N_FINGER];
   Mouse_Data last_down[N_FINGER];
   Mouse_Data last_drag[N_FINGER];

   Ecore_Animator *animator_move;
   Ecore_Animator *animator_flick;
   Ecore_Animator *animator_two_move;

   // timers
   Ecore_Timer *press_timer;
   Ecore_Timer *long_press_timer;
   Ecore_Timer *release_timer;
   Ecore_Timer *press_release_timer;
};

/* local subsystem functions */
// mouse callbacks
static float _smart_velocity_easeinoutcubic(int index);
static void _smart_mouse_down(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _smart_mouse_up(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _smart_mouse_move(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _smart_multi_down(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _smart_multi_up(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _smart_multi_move(void *data, Evas *e, Evas_Object *obj, void *ev);
// animator callbacks
static int _smart_animation_move(void *data);
static int _smart_animation_flick(void *data);
static int _smart_animation_two_move(void *data);
// enter mode functions
static void _smart_enter_none(Smart_Data *sd);
static void _smart_enter_down(Smart_Data *sd);
static void _smart_enter_down_during_drag(Smart_Data *sd);
static void _smart_enter_down_up(Smart_Data *sd, int downTime, int time);
static void _smart_enter_down_up_down(Smart_Data *sd);
static void _smart_enter_hold(Smart_Data *sd);
static void _smart_enter_drag(Smart_Data *sd);
static void _smart_enter_two_down(Smart_Data *sd);
static void _smart_enter_two_down_during_drag(Smart_Data *sd);
static void _smart_enter_two_drag(Smart_Data *sd);
static void _smart_enter_three_down(Smart_Data *sd);
// emit functions
static void _smart_emit_press(Smart_Data *sd);
static void _smart_emit_tap(Smart_Data *sd);
static void _smart_emit_double_tap(Smart_Data *sd);
static void _smart_emit_long_hold(Smart_Data *sd);
static void _smart_emit_release(Smart_Data *sd);
static void _smart_emit_two_press(Smart_Data *sd);
static void _smart_emit_two_tap(Smart_Data *sd);
static void _smart_emit_two_move_start(Smart_Data *sd);
static void _smart_emit_two_move(Smart_Data *sd);
static void _smart_emit_two_move_end(Smart_Data *sd);
static void _smart_emit_three_press(Smart_Data *sd);
static void _smart_emit_three_tap(Smart_Data *sd);
// timer handlers
static int _smart_press_timer_handler(void *data);
static int _smart_long_press_timer_handler(void *data);
static int _smart_release_timer_handler(void *data);
static int _smart_press_release_timer_handler(void *data);

static void _smart_save_move_history(Smart_Data *sd, int x, int y, int dx, int dy);
static void _smart_start_flick(Smart_Data *sd);
static void _smart_stop_animator_move(Smart_Data *sd);
static void _smart_stop_animator_flick(Smart_Data *sd);
static void _smart_stop_animator_two_move(Smart_Data *sd);
static Two_Drag_Mode _smart_check_two_drag_mode(Smart_Data *sd);
static void _smart_set_first_down(Smart_Data *sd, int index, Mouse_Data *data);
static void _smart_set_last_down(Smart_Data *sd, int index, Mouse_Data *data);
static void _smart_set_last_drag(Smart_Data *sd, int index, Mouse_Data *data);
static void _smart_stop_all_timers(Smart_Data *sd);
static void _smart_init(void);
static void _smart_del(Evas_Object *obj);
static void _smart_add(Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;

/* externally accessible functions */
   Evas_Object *
_elm_smart_touch_add(Evas *evas)
{
   _smart_init();
   return evas_object_smart_add(evas, _smart);
}

   void
_elm_smart_touch_child_set(Evas_Object *obj, Evas_Object *child)
{
   API_ENTRY return;
   if (child == sd->child_obj) return;

   if (sd->child_obj) // delete callbacks of old object
     {
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_DOWN, _smart_mouse_down);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_UP, _smart_mouse_up);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_MOVE, _smart_mouse_move);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_DOWN, _smart_multi_down);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_UP, _smart_multi_up);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_MOVE, _smart_multi_move);
	_smart_stop_all_timers(sd);
	_smart_stop_animator_move(sd);
	_smart_stop_animator_flick(sd);
	_smart_stop_animator_two_move(sd);

	sd->child_obj = NULL;
     }

   if (child)
     {
	sd->child_obj = child;

	// add callbacks
	evas_object_event_callback_add(child, EVAS_CALLBACK_MOUSE_DOWN, _smart_mouse_down, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_MOUSE_UP, _smart_mouse_up, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_MOUSE_MOVE, _smart_mouse_move, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_MULTI_DOWN, _smart_multi_down, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_MULTI_UP, _smart_multi_up, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_MULTI_MOVE, _smart_multi_move, sd);

	_smart_enter_none(sd);

	sd->is_one_drag_mode = EINA_FALSE;
     }

   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

   void
_elm_smart_touch_start(Evas_Object *obj)
{
   API_ENTRY return;
   if (sd->running) return;

   sd->running = EINA_TRUE;
   _smart_enter_none(sd);
}

   void
_elm_smart_touch_stop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->running = EINA_FALSE;
   _smart_stop_all_timers(sd);
   _smart_stop_animator_move(sd);
   _smart_stop_animator_flick(sd);
   _smart_stop_animator_two_move(sd);
   _smart_enter_none(sd);
}

   void
_elm_smart_touch_reset(Evas_Object *obj)
{
   API_ENTRY return;
   _smart_stop_all_timers(sd);
   _smart_stop_animator_move(sd);
   _smart_stop_animator_flick(sd);
   _smart_stop_animator_two_move(sd);
   _smart_enter_none(sd);
}

   void
_elm_smart_touch_screen_angle_update(Evas_Object *obj, int screen_angle)
{
   API_ENTRY return;
   sd->screen_angle = screen_angle;
}

   void
_elm_smart_touch_is_one_drag_mode_enable(Evas_Object *obj, Eina_Bool is_one_drag_mode)
{
   API_ENTRY return;
   sd->is_one_drag_mode = is_one_drag_mode;
}

/* local subsystem functions */
/** reference from htsd://hosted.zeh.com.br/tweener/docs/en-us/misc/transitions.html
 * Easing equation function for a cubic (t^3) easing in/out: acceleration until halfway, then deceleration
 * @param                t   Number                Current time (in frames or seconds)
 * @param                b   Number                Starting value
 * @param                c   Number                Change needed in value
 * @param                d   Number                Expected easing duration (in frames or seconds)
 * @param                k1  Number                first sustain value
 * @param                k2  Number                second sustain value
 * @return                   Number                The correct value
 public static function easeInOutCubic (t:Number, b:Number, c:Number, d:Number, p_params:Object):Number {
 if ((t/=d/2) < 1) return c/2*t*t*t + b;
 return c/2*((t-=2)*t*t + 2) + b;
 }
 */
static float
_smart_velocity_easeinoutcubic(int index)
{
   float d = 60.0f;
   float t = d - index; // we want to get reversed value
   float c = 1.0f;
   float k1 = 0.1f;
   float k2 = 0.05f;
   float velocity;
   if ((t /= (d / 2)) < 1)
     {
	velocity = (c / 2) * t * t * t;
     }
   else
     {
	t -= 2;
	velocity = (c / 2) * (t * t * t + 2);
     }
   if (velocity < k1 && velocity > k2) velocity = 0.1;
   else if (velocity < k2) velocity = 0.05;
   return velocity;
}

/* mouse callbacks */
static void
_smart_mouse_down(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;
   Evas_Event_Mouse_Down *event;
   Mouse_Data mouse_data;

   sd = data;
   if (!sd || sd->running == EINA_FALSE) return;

   event = (Evas_Event_Mouse_Down*)ev;

   switch (sd->state)
     {
      case TOUCH_STATE_NONE:
	 mouse_data.x = event->canvas.x;
	 mouse_data.y = event->canvas.y;
	 mouse_data.time = event->timestamp;
	 mouse_data.device = -1;
	 _smart_set_first_down(sd, 0, &mouse_data);
	 _smart_set_last_down(sd, 0, &mouse_data);
	 _smart_set_last_drag(sd, 0, &mouse_data);
	 _smart_enter_down(sd);
	 break;

      case TOUCH_STATE_DRAG:
	 mouse_data.x = event->canvas.x;
	 mouse_data.y = event->canvas.y;
	 mouse_data.time = event->timestamp;
	 mouse_data.device = -1;
	 _smart_set_first_down(sd, 0, &mouse_data);
	 _smart_set_last_down(sd, 0, &mouse_data);
	 _smart_set_last_drag(sd, 0, &mouse_data);
	 if (sd->animator_move)
	   {
	      ecore_animator_del(sd->animator_move);
	      sd->animator_move = NULL;
	   }
	 if (sd->animator_flick)
	   {
	      ecore_animator_del(sd->animator_flick);
	      sd->animator_flick = NULL;
	   }
	 _smart_enter_down_during_drag(sd);
	 break;

      case TOUCH_STATE_DOWN_UP:
	 if (event->flags == EVAS_BUTTON_DOUBLE_CLICK) {
	      mouse_data.x = event->canvas.x;
	      mouse_data.y = event->canvas.y;
	      mouse_data.time = event->timestamp;
	      mouse_data.device = -1;
	      _smart_set_last_down(sd, 0, &mouse_data);
	      _smart_set_last_drag(sd, 0, &mouse_data);
	      _smart_enter_down_up_down(sd);
	 }
	 break;

      default:
	 break;
     }
}

static void
_smart_mouse_up(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;
   sd = data;
   Evas_Event_Mouse_Up *event;

   if (!sd || sd->running == EINA_FALSE) return;

   event = (Evas_Event_Mouse_Up*)ev;

   switch (sd->state)
     {
      case TOUCH_STATE_DOWN:
	 _smart_stop_animator_move(sd);
	 _smart_stop_animator_flick(sd);
	 _smart_enter_down_up(sd, (event->timestamp - sd->last_down[0].time), event->timestamp);
	 break;

      case TOUCH_STATE_DOWN_DURING_DRAG:
	   {
	      Evas_Point point;
	      point.x = sd->last_drag[0].x;
	      point.y = sd->last_drag[0].y;
	      evas_object_smart_callback_call(sd->child_obj, "one,move,end", &point);
	      _smart_enter_none(sd);
	   } break;

      case TOUCH_STATE_DOWN_UP_DOWN:
	   {
	      int dx = sd->last_down[0].x - sd->first_down[0].x;
	      int dy = sd->last_down[0].y - sd->first_down[0].y;
	      if ((dx * dx + dy * dy) <= (DBL_TAP_DISTANCE * DBL_TAP_DISTANCE))
		_smart_emit_double_tap(sd);
	      _smart_stop_all_timers(sd);
	      _smart_enter_none(sd);
	   } break;

      case TOUCH_STATE_HOLD:
	 _smart_emit_release(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      case TOUCH_STATE_DRAG:
	 _smart_emit_release(sd);
	 _smart_start_flick(sd);
	 break;

      case TOUCH_STATE_TWO_DOWN:
	 _smart_emit_two_tap(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      case TOUCH_STATE_TWO_DRAG:
	 _smart_stop_animator_two_move(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      case TOUCH_STATE_THREE_DOWN:
	 _smart_emit_three_tap(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      default:
	 _smart_emit_release(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 DBG("\nERROR: wrong state in mouse_up\n\n");
	 break;
     }

}

static void
_smart_mouse_move(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;
   sd = data;
   if (!sd || sd->running == EINA_FALSE) return;

   Evas_Event_Mouse_Move *event = (Evas_Event_Mouse_Move*)ev;
   int dx = 0;
   int dy = 0;

   Mouse_Data mouse_data;
   mouse_data.x = event->cur.canvas.x;
   mouse_data.y = event->cur.canvas.y;
   mouse_data.time = event->timestamp;
   mouse_data.device = -1;

   switch (sd->state)
     {
      case TOUCH_STATE_DOWN:
      case TOUCH_STATE_DOWN_DURING_DRAG:
	 dx = mouse_data.x - sd->last_drag[0].x;
	 dy = mouse_data.y - sd->last_drag[0].y;

	 if ((abs(dx) > INIT_DRAG_THRESHOLD) || (abs(dy) > INIT_DRAG_THRESHOLD))
	   {
	      if (sd->animator_move)
		{
		   ecore_animator_del(sd->animator_move);
		   sd->animator_move = NULL;
		}
	      if (sd->animator_flick)
		{
		   ecore_animator_del(sd->animator_flick);
		   sd->animator_flick = NULL;
		}
	      _smart_set_last_drag(sd, 0, &mouse_data);
	      // Note:
	      // last_down - location where the drag starts
	      // (which is different than fisrtDown)
	      _smart_set_last_down(sd, 0, &mouse_data);
	      _smart_enter_drag(sd);
	   }
	 break;

      case TOUCH_STATE_DRAG:
	 dx = mouse_data.x - sd->last_drag[0].x;
	 dy = mouse_data.y - sd->last_drag[0].y;

	 if ((abs(dx) > DRAG_THRESHOLD) || (abs(dy) > DRAG_THRESHOLD))
	   {
	      _smart_set_last_drag(sd, 0, &mouse_data);
	      _smart_save_move_history(sd, mouse_data.x, mouse_data.y, dx, dy);
	   }
	 break;

      case TOUCH_STATE_TWO_DOWN:
	 _smart_set_last_drag(sd, 0, &mouse_data);

	 sd->two_drag_mode = _smart_check_two_drag_mode(sd);
	 if (sd->two_drag_mode != TWO_DRAG_NONE)
	   {
	      DBG("<< sd->two_drag_mode [%d] >>\n", sd->two_drag_mode);
	      _smart_enter_two_drag(sd);
	   }
	 break;

      case TOUCH_STATE_TWO_DRAG:
	 _smart_set_last_drag(sd, 0, &mouse_data);
	 break;

      case TOUCH_STATE_THREE_DOWN:
	 _smart_set_last_drag(sd, 0, &mouse_data);
	 break;

      default:
	 break;
     }
}

static void
_smart_multi_down(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;

   sd = data;
   if (!sd || sd->running == EINA_FALSE) return;

   Evas_Event_Multi_Down *event = (Evas_Event_Multi_Down*)ev;
   Mouse_Data mouse_data;

   switch (sd->state)
     {
      case TOUCH_STATE_DOWN:
	 sd->numOfTouch++;
	 if (sd->numOfTouch == 1)
	   {
	      mouse_data.x = event->output.x;
	      mouse_data.y = event->output.y;
	      mouse_data.time = event->timestamp;
	      mouse_data.device = event->device;
	      _smart_set_first_down(sd, 1, &mouse_data);
	      _smart_set_last_down(sd, 1, &mouse_data);
	      _smart_set_last_drag(sd, 1, &mouse_data);
	      _smart_stop_animator_move(sd);
	      _smart_stop_animator_flick(sd);
	      _smart_stop_animator_two_move(sd);
	      _smart_enter_two_down(sd);
	   }
	 break;

      case TOUCH_STATE_DOWN_DURING_DRAG:
      case TOUCH_STATE_DRAG:
	 sd->numOfTouch++;
	 if (sd->numOfTouch == 1)
	   {
	      mouse_data.x = event->output.x;
	      mouse_data.y = event->output.y;
	      mouse_data.time = event->timestamp;
	      mouse_data.device = event->device;
	      _smart_set_first_down(sd, 1, &mouse_data);
	      _smart_set_last_down(sd, 1, &mouse_data);
	      _smart_set_last_drag(sd, 1, &mouse_data);
	      if (sd->animator_move)
		{
		   ecore_animator_del(sd->animator_move);
		   sd->animator_move = NULL;
		}
	      if (sd->animator_flick)
		{
		   ecore_animator_del(sd->animator_flick);
		   sd->animator_flick = NULL;
		}
	      if (sd->animator_two_move)
		{
		   ecore_animator_del(sd->animator_two_move);
		   sd->animator_two_move = NULL;
		}
	      _smart_enter_two_down_during_drag(sd);
	   }
	 break;

      case TOUCH_STATE_TWO_DOWN:
      case TOUCH_STATE_TWO_DRAG:
	 sd->numOfTouch++;
	 if (sd->numOfTouch == 2)
	   {
	      mouse_data.x = event->output.x;
	      mouse_data.y = event->output.y;
	      mouse_data.time = event->timestamp;
	      mouse_data.device = event->device;
	      _smart_set_first_down(sd, 2, &mouse_data);
	      _smart_set_last_down(sd, 2, &mouse_data);
	      _smart_set_last_drag(sd, 2, &mouse_data);
	      _smart_stop_animator_move(sd);
	      _smart_stop_animator_flick(sd);
	      _smart_stop_animator_two_move(sd);
	      _smart_enter_three_down(sd);
	   }
	 break;

      default:
	 break;
     }
}

static void
_smart_multi_up(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;
   Evas_Event_Multi_Up *event;

   sd = data;
   if (!sd || sd->running == EINA_FALSE) return;

   event = (Evas_Event_Multi_Up*)ev;

   switch (sd->state)
     {
      case TOUCH_STATE_TWO_DOWN:
	 _smart_emit_two_tap(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      case TOUCH_STATE_TWO_DOWN_DURING_DRAG:
	   {
	      Evas_Point point;
	      point.x = sd->last_drag[0].x;
	      point.y = sd->last_drag[0].y;
	      evas_object_smart_callback_call(sd->child_obj, "one,move,end", &point);
	      _smart_emit_two_tap(sd);
	      _smart_stop_all_timers(sd);
	      _smart_enter_none(sd);
	   } break;

      case TOUCH_STATE_TWO_DRAG:
	 _smart_stop_animator_two_move(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      case TOUCH_STATE_THREE_DOWN:
	 _smart_emit_three_tap(sd);
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;

      default:
	 _smart_stop_all_timers(sd);
	 _smart_enter_none(sd);
	 break;
     }
}

static void
_smart_multi_move(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   Smart_Data *sd;
   Evas_Event_Multi_Move *event;
   Mouse_Data mouse_data;

   sd = data;
   if (!sd || sd->running == EINA_FALSE) return;

   event = (Evas_Event_Multi_Move*)ev;
   mouse_data.x = event->cur.output.x;
   mouse_data.y = event->cur.output.y;
   mouse_data.time = event->timestamp;
   mouse_data.device = event->device;

   switch (sd->state)
     {
      case TOUCH_STATE_TWO_DOWN:
      case TOUCH_STATE_TWO_DOWN_DURING_DRAG:
	 if (sd->first_down[1].device == event->device)
	   {
	      _smart_set_last_drag(sd, 1, &mouse_data);
	      sd->two_drag_mode = _smart_check_two_drag_mode(sd);
	      if (sd->two_drag_mode != TWO_DRAG_NONE)
		{
		   DBG("<< sd->two_drag_mode [%d] >>\n", sd->two_drag_mode);
		   _smart_enter_two_drag(sd);
		}
	   }
	 break;

      case TOUCH_STATE_TWO_DRAG:
	 if (sd->first_down[1].device == event->device)
	   {
	      _smart_set_last_drag(sd, 1, &mouse_data);
	   }
	 break;

      case TOUCH_STATE_THREE_DOWN:
	 if (sd->first_down[1].device == event->device)
	   {
	      _smart_set_last_drag(sd, 1, &mouse_data);
	   }
	 else if (sd->first_down[2].device == event->device)
	   {
	      _smart_set_last_drag(sd, 2, &mouse_data);
	   }
	 break;

      default:
	 break;
     }
}

/* animators */
static int
_smart_animation_move(void *data)
{
   Smart_Data *sd;

   sd = data;
   if (sd->child_obj)
     {
	DBG("<< animation_move >>\n");
	// get the position here instead of mouse_move event
	Evas *evas = evas_object_evas_get(sd->child_obj);
	Evas_Point point;
	evas_pointer_canvas_xy_get(evas, &point.x, &point.y);
	if (sd->is_one_drag_mode)
	  {
	     if (sd->one_drag_mode == ONE_DRAG_VERTICAL)
	       {
		  // Note:
		  // first_down - location of mouse down
		  // last_down - location where the drag started
		  point.x = sd->last_down[0].x;
	       }
	     else if (sd->one_drag_mode == ONE_DRAG_HORIZONTAL)
	       {
		  point.y = sd->last_down[0].y;
	       }
	  }
	evas_object_smart_callback_call(sd->child_obj, "one,move", &point);
	return ECORE_CALLBACK_RENEW;
     }
   else
     {
	_smart_stop_animator_move(sd);
	_smart_enter_none(sd);
	return ECORE_CALLBACK_CANCEL;
     }
}

static int
_smart_animation_flick(void *data)
{
   Smart_Data *sd;
   Flick_Data *flick_data;

   sd = data;
   flick_data = &(sd->flick_data);

   if (flick_data && sd->child_obj)
     {
	// calculate dx, dy
	float velocity = _smart_velocity_easeinoutcubic(flick_data->flick_index);
	Evas_Coord dx = flick_data->avg_distance.x * velocity;
	Evas_Coord dy = flick_data->avg_distance.y * velocity;
	flick_data->flick_index++;
	flick_data->last.x += dx;
	flick_data->last.y += dy;
	DBG("<< animation_flick |%d|%d|%f| >>\n", dx, dy, ecore_loop_time_get());

	// stop flick animator
	if (dx == 0 && dy == 0)
	  {
	     _smart_stop_animator_flick(sd);
	     if (sd->state == TOUCH_STATE_DRAG)
	       _smart_enter_none(sd);
	     return ECORE_CALLBACK_CANCEL;
	  }
	else
	  {
	     Evas_Coord_Point point;
	     point = flick_data->last;
	     if (sd->is_one_drag_mode)
	       {
		  if (sd->one_drag_mode == ONE_DRAG_VERTICAL)
		    {
		       point.x = sd->first_down[0].x;
		    }
		  else if (sd->one_drag_mode == ONE_DRAG_HORIZONTAL)
		    {
		       point.y = sd->first_down[0].y;
		    }
	       }
	     evas_object_smart_callback_call(sd->child_obj, "one,move", &point);
	     return ECORE_CALLBACK_RENEW;
	  }
     }
   else
     {
	_smart_stop_animator_flick(sd);
	_smart_enter_none(sd);
	return ECORE_CALLBACK_CANCEL;
     }
}

static int
_smart_animation_two_move(void *data)
{
   Smart_Data *sd;

   sd = data;

   if (sd->child_obj)
     {
	_smart_emit_two_move(sd);
	return ECORE_CALLBACK_RENEW;
     }
   else
     {
	_smart_stop_animator_two_move(sd);
	_smart_enter_none(sd);
	return ECORE_CALLBACK_CANCEL;
     }

}

/* state switching */
static void
_smart_enter_none(Smart_Data *sd)
{
   sd->numOfTouch = 0;
   sd->two_drag_mode = TWO_DRAG_NONE;
   sd->state = TOUCH_STATE_NONE;
   DBG("\nTOUCH_STATE_NONE\n");
}

static void
_smart_enter_down(Smart_Data *sd)
{
   // set press timer
   sd->press_timer = ecore_timer_add(((double)PRESS_TIME)/1000.0, _smart_press_timer_handler, sd);

   // set long press timer
   sd->long_press_timer = ecore_timer_add(((double)LONG_HOLD_TIME)/1000.0, _smart_long_press_timer_handler, sd);

   sd->state = TOUCH_STATE_DOWN;
   DBG("\nTOUCH_STATE_DOWN\n");
}

static void
_smart_enter_down_during_drag(Smart_Data *sd)
{
   // set press timer
   sd->press_timer = ecore_timer_add(((double)PRESS_TIME)/1000.0, _smart_press_timer_handler, sd);

   sd->state = TOUCH_STATE_DOWN_DURING_DRAG;
   DBG("\nTOUCH_STATE_DOWN_DURING_DRAG\n");
}

static void
_smart_enter_down_up(Smart_Data *sd, int downTime, int time)
{
   // remove sd->press_timer and set new timer
   int timerTime = RELEASE_TIME - (downTime - PRESS_TIME);
   if (sd->press_timer)
     {
	ecore_timer_del(sd->press_timer);
	sd->press_timer = NULL;
	sd->press_release_timer = ecore_timer_add(((double)timerTime)/1000.0, _smart_press_release_timer_handler, sd);

     }
   else
     {
	sd->release_timer = ecore_timer_add(((double)timerTime)/1000.0, _smart_release_timer_handler, sd);
     }

   if (sd->long_press_timer) // remove long press timer
     {
	ecore_timer_del(sd->long_press_timer);
	sd->long_press_timer = NULL;
     }

   sd->state = TOUCH_STATE_DOWN_UP;
   DBG("\nTOUCH_STATE_DOWN_UP\n");
}

static void
_smart_enter_down_up_down(Smart_Data *sd)
{
   if (sd->press_release_timer) // remove press_release_timer
     {
	ecore_timer_del(sd->press_release_timer);
	sd->press_release_timer = NULL;
     }

   if (sd->release_timer) // remove ReleaseTimer
     {
	ecore_timer_del(sd->release_timer);
	sd->release_timer = NULL;
     }

   sd->state = TOUCH_STATE_DOWN_UP_DOWN;
   DBG("\nTOUCH_STATE_DOWN_UP_DOWN\n");
}

static void
_smart_enter_hold(Smart_Data *sd)
{
   sd->state = TOUCH_STATE_HOLD;
   DBG("\nTOUCH_STATE_HOLD\n");
}

static void
_smart_enter_drag(Smart_Data *sd)
{
   if (sd->press_timer) // remove press_timer
     {
	ecore_timer_del(sd->press_timer);
	sd->press_timer = NULL;
     }

   if (sd->press_release_timer) // remove press_release_timer
     {
	ecore_timer_del(sd->press_release_timer);
	sd->press_release_timer = NULL;
     }

   if (sd->release_timer) // remove ReleaseTimer
     {
	ecore_timer_del(sd->release_timer);
	sd->release_timer = NULL;
     }

   if (sd->long_press_timer) // remove long press timer
     {
	ecore_timer_del(sd->long_press_timer);
	sd->long_press_timer = NULL;
     }

   if (sd->child_obj)
     {
	if (sd->is_one_drag_mode)
	  {
	     sd->one_drag_mode = ONE_DRAG_NONE;
	     int abs_dx = abs(sd->first_down[0].x - sd->last_drag[0].x);
	     int abs_dy = abs(sd->first_down[0].y - sd->last_drag[0].y);
	     abs_dx = (abs_dx == 0) ? 1 : abs_dx;
	     DBG("<< abs_dx[%d], abs_dy[%d] >>\n\n", abs_dx, abs_dy);
	     float degree = (float)abs_dy / (float)abs_dx;
	     // more than 70 degree
	     if (degree > tan(70 * M_PI / 180))
	       {
		  sd->one_drag_mode = ONE_DRAG_VERTICAL;
	       }
	     // less than 20 degree
	     else if (degree < tan(20 * M_PI / 180))
	       {
		  sd->one_drag_mode = ONE_DRAG_HORIZONTAL;
	       }
	  }
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,move,start", &point);

	// initialize flick variables
	sd->last_move_history_index = -1;
	sd->move_history_count = 0;

	sd->animator_move = ecore_animator_add(_smart_animation_move, sd);
	DBG("<< sd->animator_move >>\n");
	sd->state = TOUCH_STATE_DRAG;
	DBG("\nTOUCH_STATE_DRAG\n");
     }
   else
     {
	sd->state = TOUCH_STATE_NONE;
     }
}

static void
_smart_enter_two_down(Smart_Data *sd)
{
   _smart_stop_all_timers(sd);

   if (sd->child_obj)
     {
	DBG("<< enter two down >>\n");
	sd->state = TOUCH_STATE_TWO_DOWN;
	_smart_emit_two_press(sd);
     }
}

static void
_smart_enter_two_down_during_drag(Smart_Data *sd)
{
   _smart_stop_all_timers(sd);

   if (sd->child_obj)
     {
	DBG("<< enter two down >>\n");
	sd->state = TOUCH_STATE_TWO_DOWN_DURING_DRAG;
	_smart_emit_two_press(sd);
     }
}

static void
_smart_enter_two_drag(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< sd->animator_two_move >>\n");
	sd->state = TOUCH_STATE_TWO_DRAG;
	_smart_emit_two_move_start(sd);
	sd->animator_two_move = ecore_animator_add(_smart_animation_two_move, sd);
     }
   else
     {
	sd->state = TOUCH_STATE_NONE;
     }
}

static void
_smart_enter_three_down(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	sd->state = TOUCH_STATE_THREE_DOWN;
	_smart_emit_three_press(sd);
     }
}

/* producing output events */
static void
_smart_emit_press(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_press >>\n");
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,press", &point);
     }
}

static void
_smart_emit_tap(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_tap >>\n");
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,single,tap", &point);
     }
}

static void
_smart_emit_double_tap(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_double_tap >>\n");
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,double,tap", &point);
     }
}

static void
_smart_emit_long_hold(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_long_hold >>\n");
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,long,press", &point);
     }
}

static void
_smart_emit_release(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_release >>\n");
	Evas_Point point;
	point.x = sd->last_down[0].x;
	point.y = sd->last_down[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,release", &point);
     }
}

static void
_smart_emit_two_press(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_two_press >>\n");
	Two_Mouse_Data two_mouse_data;
	two_mouse_data.first.x = sd->last_down[0].x;
	two_mouse_data.first.y = sd->last_down[0].y;
	two_mouse_data.second.x = sd->last_down[1].x;
	two_mouse_data.second.y = sd->last_down[1].y;
	two_mouse_data.mode = sd->two_drag_mode;
	evas_object_smart_callback_call(sd->child_obj, "two,press", &two_mouse_data);
     }
}

static void
_smart_emit_two_tap(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_two_tap >>\n");
	Two_Mouse_Data two_mouse_data;
	two_mouse_data.first.x = sd->last_down[0].x;
	two_mouse_data.first.y = sd->last_down[0].y;
	two_mouse_data.second.x = sd->last_down[1].x;
	two_mouse_data.second.y = sd->last_down[1].y;
	two_mouse_data.mode = sd->two_drag_mode;
	evas_object_smart_callback_call(sd->child_obj, "two,tap", &two_mouse_data);
     }
}

static void
_smart_emit_two_move_start(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_two_move_start >>\n");
	Two_Mouse_Data two_mouse_data;
	two_mouse_data.first.x = sd->last_drag[0].x;
	two_mouse_data.first.y = sd->last_drag[0].y;
	two_mouse_data.second.x = sd->last_drag[1].x;
	two_mouse_data.second.y = sd->last_drag[1].y;
	two_mouse_data.mode = sd->two_drag_mode;
	evas_object_smart_callback_call(sd->child_obj, "two,move,start", &two_mouse_data);
     }
}

static void
_smart_emit_two_move(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	Two_Mouse_Data two_mouse_data;
	two_mouse_data.first.x = sd->last_drag[0].x;
	two_mouse_data.first.y = sd->last_drag[0].y;
	two_mouse_data.second.x = sd->last_drag[1].x;
	two_mouse_data.second.y = sd->last_drag[1].y;
	two_mouse_data.mode = sd->two_drag_mode;
	evas_object_smart_callback_call(sd->child_obj, "two,move", &two_mouse_data);
     }
}

static void
_smart_emit_two_move_end(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_two_move_end >>\n");
	Two_Mouse_Data two_mouse_data;
	two_mouse_data.first.x = sd->last_drag[0].x;
	two_mouse_data.first.y = sd->last_drag[0].y;
	two_mouse_data.second.x = sd->last_drag[1].x;
	two_mouse_data.second.y = sd->last_drag[1].y;
	two_mouse_data.mode = sd->two_drag_mode;
	evas_object_smart_callback_call(sd->child_obj, "two,move,end", &two_mouse_data);
     }
}

static void
_smart_emit_three_press(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_three_press >>\n");
	Three_Mouse_Data three_mouse_data;
	three_mouse_data.first.x = sd->last_drag[0].x;
	three_mouse_data.first.y = sd->last_drag[0].y;
	three_mouse_data.second.x = sd->last_drag[1].x;
	three_mouse_data.second.y = sd->last_drag[1].y;
	three_mouse_data.third.x = sd->last_drag[2].x;
	three_mouse_data.third.y = sd->last_drag[2].y;
	evas_object_smart_callback_call(sd->child_obj, "three,press", &three_mouse_data);
     }
}

static void
_smart_emit_three_tap(Smart_Data *sd)
{
   if (sd->child_obj)
     {
	DBG("<< emit_three_tap >>\n");
	Three_Mouse_Data three_mouse_data;
	three_mouse_data.first.x = sd->last_drag[0].x;
	three_mouse_data.first.y = sd->last_drag[0].y;
	three_mouse_data.second.x = sd->last_drag[1].x;
	three_mouse_data.second.y = sd->last_drag[1].y;
	three_mouse_data.third.x = sd->last_drag[2].x;
	three_mouse_data.third.y = sd->last_drag[2].y;
	evas_object_smart_callback_call(sd->child_obj, "three,tap", &three_mouse_data);
     }
}

/* timer event handling */
static int
_smart_press_timer_handler(void *data)
{
   Smart_Data *sd;

   sd = data;
   _smart_emit_press(sd);
   sd->press_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int
_smart_long_press_timer_handler(void *data)
{
   Smart_Data *sd;

   sd = data;
   _smart_emit_long_hold(sd);
   _smart_enter_hold(sd);
   sd->long_press_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int
_smart_release_timer_handler(void *data)
{
   Smart_Data *sd;

   sd = data;
   _smart_emit_tap(sd);
   _smart_stop_all_timers(sd);
   _smart_enter_none(sd);
   sd->release_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int
_smart_press_release_timer_handler(void *data)
{
   static int prevent_handler = 0;
   if (prevent_handler != 0) return ECORE_CALLBACK_CANCEL;
   prevent_handler = 1;
   Smart_Data *sd;

   sd = data;
   _smart_emit_press(sd);
   _smart_emit_tap(sd);
   _smart_stop_all_timers(sd);
   _smart_enter_none(sd);
   sd->press_release_timer = NULL;
   prevent_handler = 0;
   return ECORE_CALLBACK_CANCEL;
}

/* other functions */
static void
_smart_save_move_history(Smart_Data *sd, int x, int y, int dx, int dy)
{
   // save pan information to the pan history
   int index = (sd->last_move_history_index + 1) % MOVE_HISTORY_SIZE;
   sd->last_move_history_index = index;
   sd->move_history[index].dx = dx;
   sd->move_history[index].dy = dy;
   sd->move_history[index].time = ecore_time_get();
   sd->move_history_count++;
}

static void
_smart_start_flick(Smart_Data *sd)
{
   if (sd->animator_move)
     {
	ecore_animator_del(sd->animator_move);
	DBG("<< stop_animator_move >>\n");
	sd->animator_move = NULL;

	// start flick
	// accumulate sd->move_history data
	int nSamples = 0;
	int totalDx = 0;
	int totalDy = 0;
	int index = sd->last_move_history_index;
	int todo = sd->move_history_count > MOVE_HISTORY_SIZE ? MOVE_HISTORY_SIZE : sd->move_history_count;
	Mouse_Diff_Data *p;
	double endTime = ecore_time_get();
	double startTime = endTime;
	for( ; todo > 0; todo--) {
	     p = sd->move_history + index; // get one sd->move_history

	     // get values
	     startTime = p->time;
	     totalDx += p->dx;
	     totalDy += p->dy;
	     nSamples++;

	     if ((endTime - startTime) > 0.2 && nSamples > 0)
	       break;

	     index = (index > 0) ? (index - 1) : (MOVE_HISTORY_SIZE - 1); // set index
	}
	double totalTime = endTime - startTime;
	if (totalTime < DOUBLE_ERROR)
	  totalTime = 0.001;

	// calculate average pan_dx and pan_dy (per 1 / DEFAULT_FRAMERATE ms)
	double temp = totalTime * DEFAULT_FRAMERATE;
	if (temp <= 0)
	  temp = 1;

	Flick_Data *flick_data = &sd->flick_data;
	flick_data->avg_distance.x = totalDx / temp;
	flick_data->avg_distance.y = totalDy / temp;

	// set max value for pan_dx and pan_dy
	int abs_pan_dx = abs(flick_data->avg_distance.x);
	int abs_pan_dy = abs(flick_data->avg_distance.y);
	if ((abs_pan_dx > MAX_MOVE_DISTANCE) && (abs_pan_dx > abs_pan_dy))
	  {
	     flick_data->avg_distance.x = (flick_data->avg_distance.x > 0) ? MAX_MOVE_DISTANCE : -MAX_MOVE_DISTANCE;
	     flick_data->avg_distance.y = flick_data->avg_distance.y * MAX_MOVE_DISTANCE / abs_pan_dx;

	  }
	else if ((abs_pan_dy > MAX_MOVE_DISTANCE) && (abs_pan_dy > abs_pan_dx))
	  {
	     flick_data->avg_distance.y = (flick_data->avg_distance.y > 0) ? MAX_MOVE_DISTANCE : -MAX_MOVE_DISTANCE;
	     flick_data->avg_distance.x = flick_data->avg_distance.x * MAX_MOVE_DISTANCE / abs_pan_dy;
	  }

	if (abs_pan_dx > FLICK_THRESHOLD || abs_pan_dy > FLICK_THRESHOLD)
	  {
	     // set flick_data and start flick
	     flick_data->last.x = sd->last_drag[0].x;
	     flick_data->last.y = sd->last_drag[0].y;
	     flick_data->flick_index = 0;
	     sd->animator_flick = ecore_animator_add(_smart_animation_flick, sd);
	     DBG("<< sd->animator_flick >>\n");
	  }
	else
	  {
	     Evas_Point point;
	     point.x = sd->last_drag[0].x;
	     point.y = sd->last_drag[0].y;
	     evas_object_smart_callback_call(sd->child_obj, "one,move,end", &point);
	     _smart_enter_none(sd);
	  }
     }
   else
     {
	_smart_emit_release(sd);
	_smart_stop_all_timers(sd);
	_smart_enter_none(sd);
     }
}

static void
_smart_stop_animator_move(Smart_Data *sd)
{
   if (sd->animator_move)
     {
	ecore_animator_del(sd->animator_move);
	DBG("<< stop_animator_move >>\n");
	sd->animator_move = NULL;
	Evas_Point point;
	point.x = sd->last_drag[0].x;
	point.y = sd->last_drag[0].y;
	evas_object_smart_callback_call(sd->child_obj, "one,move,end", &point);
     }
}

static void
_smart_stop_animator_flick(Smart_Data *sd)
{
   if (sd->animator_flick)
     {
	ecore_animator_del(sd->animator_flick);
	DBG("<< stop_animator_flick >>\n");
	sd->animator_flick = NULL;
	Evas_Coord_Point point;
	point = sd->flick_data.last;
	evas_object_smart_callback_call(sd->child_obj, "one,move,end", &point);
     }
}

static void
_smart_stop_animator_two_move(Smart_Data *sd)
{
   if (sd->animator_two_move)
     {
	ecore_animator_del(sd->animator_two_move);
	DBG("<< stop_animator_two_move >>\n");
	sd->animator_two_move = NULL;
	_smart_emit_two_move_end(sd);
     }
}

static Two_Drag_Mode
_smart_check_two_drag_mode(Smart_Data *sd)
{
   // get distance from press to current position
   int dx0 = sd->last_drag[0].x - sd->first_down[0].x;
   int dy0 = sd->last_drag[0].y - sd->first_down[0].y;
   int dx1 = sd->last_drag[1].x - sd->first_down[1].x;
   int dy1 = sd->last_drag[1].y - sd->first_down[1].y;
   int dx = 0;
   int dy = 0;

   // select dx and dy
   if ((abs(dx1) >= MOVE_THRESHOLD) || (abs(dy1) >= MOVE_THRESHOLD))
     {
	dx = dx1;
	dy = dy1;
     }
   else if ((abs(dx0) >= MOVE_THRESHOLD) || (abs(dy0) >= MOVE_THRESHOLD))
     {
	dx = dx0;
	dy = dy0;
     }
   else
     {
	return TWO_DRAG_NONE;
     }

   // same x direction
   if ((abs(dx) > abs(dy)) && ((dx0 > 0 && dx1 > 0) || (dx0 < 0 && dx1 < 0)))
     {
	dy = (dy == 0) ? 1 : dy;
	// less than 30 degree (1024/root(3) = 591)
	if (((abs(dy) << 10) / abs(dx)) < 591)
	  {
	     return TWO_DRAG_HORIZONTAL;
	  }
     }

   // same y direction
   if ((abs(dy) > abs(dx)) && ((dy0 > 0 && dy1 > 0) || (dy0 < 0 && dy1 < 0)))
     {
	dx = (dx == 0) ? 1 : dx;
	// more than 60 degree (1024 * root(3)/1 = 1773)
	if (((abs(dy) << 10) / abs(dx)) > 1773)
	  {
	     return TWO_DRAG_VERTICAL;
	  }
     }

   // pinch direction
   int distanceX = abs(abs(sd->first_down[0].x - sd->first_down[1].x)
	 - abs(sd->last_drag[0].x - sd->last_drag[1].x));
   int distanceY = abs(abs(sd->first_down[0].y - sd->first_down[1].y)
	 - abs(sd->last_drag[0].y - sd->last_drag[1].y));
   if ((distanceX > FINGER_DISTANCE) || (distanceY > FINGER_DISTANCE))
     {
	return TWO_DRAG_PINCH;
     }

   return TWO_DRAG_NONE;
}

static void
_smart_set_first_down(Smart_Data *sd, int index, Mouse_Data *data)
{
   if (index > N_FINGER)
     {
	return;
     }

   if ((sd->screen_angle == 270) && (index > 0))
     {
	sd->first_down[index].x = data->y;
	sd->first_down[index].y = data->x;
	sd->first_down[index].time = data->time;
	sd->first_down[index].device = data->device;
     }
   else
     {
	sd->first_down[index].x = data->x;
	sd->first_down[index].y = data->y;
	sd->first_down[index].time = data->time;
	sd->first_down[index].device = data->device;
     }
}

static void
_smart_set_last_down(Smart_Data *sd, int index, Mouse_Data *data)
{
   if (index > N_FINGER)
     {
	return;
     }

   if ((sd->screen_angle == 270) && (index > 0))
     {
	sd->last_down[index].x = data->y;
	sd->last_down[index].y = data->x;
	sd->last_down[index].time = data->time;
	sd->last_down[index].device = data->device;
     }
   else
     {
	sd->last_down[index].x = data->x;
	sd->last_down[index].y = data->y;
	sd->last_down[index].time = data->time;
	sd->last_down[index].device = data->device;
     }
}

static void
_smart_set_last_drag(Smart_Data *sd, int index, Mouse_Data *data)
{
   if (index > N_FINGER)
     {
	return;
     }

   if ((sd->screen_angle == 270) && (index > 0))
     {
	sd->last_drag[index].x = data->y;
	sd->last_drag[index].y = data->x;
	sd->last_drag[index].time = data->time;
	sd->last_drag[index].device = data->device;
     }
   else
     {
	sd->last_drag[index].x = data->x;
	sd->last_drag[index].y = data->y;
	sd->last_drag[index].time = data->time;
	sd->last_drag[index].device = data->device;
     }
}

static void
_smart_stop_all_timers(Smart_Data *sd)
{
   if (sd->press_timer) // remove sd->press_timer
     {
	ecore_timer_del(sd->press_timer);
	sd->press_timer = NULL;
     }

   if (sd->long_press_timer) // remove long press timer
     {
	ecore_timer_del(sd->long_press_timer);
	sd->long_press_timer = NULL;
     }

   if (sd->release_timer) // remove release timer
     {
	ecore_timer_del(sd->release_timer);
	sd->release_timer = NULL;
     }

   if (sd->press_release_timer) // remove pressRelease timer
     {
	ecore_timer_del(sd->press_release_timer);
	sd->press_release_timer = NULL;
     }
}

static void
_smart_add(Evas_Object *obj)
{
   Smart_Data *sd;

   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   memset((void *)sd, 0x00, sizeof(Smart_Data));

   sd->smart_obj = obj;

   // set default framerate
   ecore_animator_frametime_set(1.0 / DEFAULT_FRAMERATE);

   evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   if (sd)
     {
	if (sd->press_timer)
	  ecore_timer_del(sd->press_timer);

	if (sd->long_press_timer)
	  ecore_timer_del(sd->long_press_timer);

	if (sd->release_timer)
	  ecore_timer_del(sd->release_timer);

	if (sd->press_release_timer)
	  ecore_timer_del(sd->press_release_timer);

	if (sd->animator_move)
	  ecore_animator_del(sd->animator_move);

	if (sd->animator_flick)
	  ecore_animator_del(sd->animator_flick);

	if (sd->animator_two_move)
	  ecore_animator_del(sd->animator_two_move);

	if (sd->child_obj)
	  {
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_DOWN, _smart_mouse_down);
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_UP, _smart_mouse_up);
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MOUSE_MOVE, _smart_mouse_move);
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_DOWN, _smart_multi_down);
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_UP, _smart_multi_up);
	     evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_MULTI_MOVE, _smart_multi_move);
	  }

	free(sd);
     }
}

static void
_smart_init(void)
{
   if (_smart) return;

   static const Evas_Smart_Class sc = 
     {
	SMART_NAME,
	EVAS_SMART_CLASS_VERSION,
	_smart_add,
	_smart_del,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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
