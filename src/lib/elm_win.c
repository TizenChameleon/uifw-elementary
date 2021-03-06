#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Elm_Win Elm_Win;

struct _Elm_Win
{
   Ecore_Evas *ee;
   Evas *evas;
   Evas_Object *parent, *win_obj, *img_obj, *frame_obj;
   Eina_List *subobjs;
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window xwin;
   Ecore_Event_Handler *client_message_handler;
   Ecore_Event_Handler *property_handler;
#endif
   Ecore_Job *deferred_resize_job;
   Ecore_Job *deferred_child_eval_job;

   Elm_Win_Type type;
   Elm_Win_Keyboard_Mode kbdmode;
   Elm_Win_Indicator_Mode indmode;
   Elm_Win_Indicator_Opacity_Mode ind_o_mode;
   struct
     {
        const char *info;
        Ecore_Timer *timer;
        int repeat_count;
        int shot_counter;
     } shot;
   int resize_location;
   int *autodel_clear, rot;
   int show_count;
   struct
     {
        int x, y;
     } screen;
   struct
     {
        Ecore_Evas *ee;
        Evas *evas;
        Evas_Object *obj, *hot_obj;
        int hot_x, hot_y;
     } pointer;
   struct
     {
        Evas_Object *top;

        struct
          {
             Evas_Object *target;
             Eina_Bool visible : 1;
             Eina_Bool handled : 1;
          } cur, prev;

        const char *style;
        Ecore_Job *reconf_job;

        Eina_Bool enabled : 1;
        Eina_Bool changed_theme : 1;
        Eina_Bool top_animate : 1;
        Eina_Bool geometry_changed : 1;
     } focus_highlight;
   struct
   {
      const char  *name;
      Ecore_Timer *timer;
      Eina_List   *names;
   } profile;

   Evas_Object *icon;
   const char *title;
   const char *icon_name;
   const char *role;

   double aspect;
   Eina_Bool urgent : 1;
   Eina_Bool modal : 1;
   Eina_Bool demand_attention : 1;
   Eina_Bool autodel : 1;
   Eina_Bool constrain : 1;
   Eina_Bool resizing : 1;
   Eina_Bool iconified : 1;
   Eina_Bool withdrawn : 1;
   Eina_Bool sticky : 1;
   Eina_Bool fullscreen : 1;
   Eina_Bool maximized : 1;
   Eina_Bool skip_focus : 1;
   Eina_Bool floating : 1;
};

static const char *widtype = NULL;
static void _elm_win_obj_callback_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _elm_win_obj_callback_img_obj_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _elm_win_obj_callback_parent_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _elm_win_obj_intercept_move(void *data, Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _elm_win_obj_intercept_show(void *data, Evas_Object *obj);
static void _elm_win_move(Ecore_Evas *ee);
static void _elm_win_resize(Ecore_Evas *ee);
static void _elm_win_delete_request(Ecore_Evas *ee);
static void _elm_win_resize_job(void *data);
#ifdef HAVE_ELEMENTARY_X
static void _elm_win_xwin_update(Elm_Win *win);
#endif
static void _elm_win_eval_subobjs(Evas_Object *obj);
static void _elm_win_subobj_callback_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _elm_win_subobj_callback_changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _elm_win_focus_highlight_init(Elm_Win *win);
static void _elm_win_focus_highlight_shutdown(Elm_Win *win);
static void _elm_win_focus_highlight_visible_set(Elm_Win *win, Eina_Bool visible);
static void _elm_win_focus_highlight_reconfigure_job_start(Elm_Win *win);
static void _elm_win_focus_highlight_reconfigure_job_stop(Elm_Win *win);
static void _elm_win_focus_highlight_anim_end(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _elm_win_focus_highlight_reconfigure(Elm_Win *win);

static void _elm_win_frame_add(Elm_Win *win, const char *style);
static void _elm_win_frame_cb_move_start(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__);
static void _elm_win_frame_cb_resize_start(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source);
static void _elm_win_frame_cb_minimize(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__);
static void _elm_win_frame_cb_maximize(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__);
static void _elm_win_frame_cb_close(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__);

//static void _elm_win_pointer_add(Elm_Win *win, const char *style);

static const char SIG_DELETE_REQUEST[] = "delete,request";
static const char SIG_FOCUS_OUT[] = "focus,out";
static const char SIG_FOCUS_IN[] = "focus,in";
static const char SIG_MOVED[] = "moved";
static const char SIG_THEME_CHANGED[] = "theme,changed";
static const char SIG_WITHDRAWN[] = "withdrawn";
static const char SIG_ICONIFIED[] = "iconified";
static const char SIG_NORMAL[] = "normal";
static const char SIG_STICK[] = "stick";
static const char SIG_UNSTICK[] = "unstick";
static const char SIG_FULLSCREEN[] = "fullscreen";
static const char SIG_UNFULLSCREEN[] = "unfullscreen";
static const char SIG_MAXIMIZED[] = "maximized";
static const char SIG_UNMAXIMIZED[] = "unmaximized";
static const char SIG_PROFILE_CHANGED[] = "profile,changed";

static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_DELETE_REQUEST, ""},
   {SIG_FOCUS_OUT, ""},
   {SIG_FOCUS_IN, ""},
   {SIG_MOVED, ""},
   {SIG_WITHDRAWN, ""},
   {SIG_ICONIFIED, ""},
   {SIG_NORMAL, ""},
   {SIG_STICK, ""},
   {SIG_UNSTICK, ""},
   {SIG_FULLSCREEN, ""},
   {SIG_UNFULLSCREEN, ""},
   {SIG_MAXIMIZED, ""},
   {SIG_UNMAXIMIZED, ""},
   {SIG_PROFILE_CHANGED, ""},
   {NULL, NULL}
};



Eina_List *_elm_win_list = NULL;
int _elm_win_deferred_free = 0;

// exmaple shot spec (wait 0.1 sec then save as my-window.png):
// ELM_ENGINE="shot:delay=0.1:file=my-window.png"

static double
_shot_delay_get(Elm_Win *win)
{
   char *p, *pd;
   char *d = strdup(win->shot.info);

   if (!d) return 0.5;
   for (p = (char *)win->shot.info; *p; p++)
     {
        if (!strncmp(p, "delay=", 6))
          {
             double v;

             for (pd = d, p += 6; (*p) && (*p != ':'); p++, pd++)
               {
                  *pd = *p;
               }
             *pd = 0;
             v = _elm_atof(d);
             free(d);
             return v;
          }
     }
   free(d);
   return 0.5;
}

static char *
_shot_file_get(Elm_Win *win)
{
   char *p;
   char *tmp = strdup(win->shot.info);
   char *repname = NULL;

   if (!tmp) return NULL;

   for (p = (char *)win->shot.info; *p; p++)
     {
        if (!strncmp(p, "file=", 5))
          {
             strcpy(tmp, p + 5);
             if (!win->shot.repeat_count) return tmp;
             else
               {
                  char *dotptr = strrchr(tmp, '.');
                  if (dotptr)
                    {
                       size_t size = sizeof(char)*(strlen(tmp) + 16);
                       repname = malloc(size);
                       strncpy(repname, tmp, dotptr - tmp);
                       snprintf(repname + (dotptr - tmp), size - (dotptr - tmp), "%03i",
                               win->shot.shot_counter + 1);
                       strcat(repname, dotptr);
                       free(tmp);
                       return repname;
                    }
               }
          }
     }
   free(tmp);
   if (!win->shot.repeat_count) return strdup("out.png");

   repname = malloc(sizeof(char) * 24);
   snprintf(repname, sizeof(char) * 24, "out%03i.png", win->shot.shot_counter + 1);
   return repname;
}

static int
_shot_repeat_count_get(Elm_Win *win)
{
   char *p, *pd;
   char *d = strdup(win->shot.info);

   if (!d) return 0;
   for (p = (char *)win->shot.info; *p; p++)
     {
        if (!strncmp(p, "repeat=", 7))
          {
             int v;

             for (pd = d, p += 7; (*p) && (*p != ':'); p++, pd++)
               {
                  *pd = *p;
               }
             *pd = 0;
             v = atoi(d);
             if (v < 0) v = 0;
             if (v > 1000) v = 999;
             free(d);
             return v;
          }
     }
   free(d);
   return 0;
}

static char *
_shot_key_get(Elm_Win *win __UNUSED__)
{
   return NULL;
}

static char *
_shot_flags_get(Elm_Win *win __UNUSED__)
{
   return NULL;
}

static void
_shot_do(Elm_Win *win)
{
   Ecore_Evas *ee;
   Evas_Object *o;
   unsigned int *pixels;
   int w, h;
   char *file, *key, *flags;

   ecore_evas_manual_render(win->ee);
   pixels = (void *)ecore_evas_buffer_pixels_get(win->ee);
   if (!pixels) return;
   ecore_evas_geometry_get(win->ee, NULL, NULL, &w, &h);
   if ((w < 1) || (h < 1)) return;
   file = _shot_file_get(win);
   if (!file) return;
   key = _shot_key_get(win);
   flags = _shot_flags_get(win);
   ee = ecore_evas_buffer_new(1, 1);
   o = evas_object_image_add(ecore_evas_get(ee));
   evas_object_image_alpha_set(o, ecore_evas_alpha_get(win->ee));
   evas_object_image_size_set(o, w, h);
   evas_object_image_data_set(o, pixels);
   if (!evas_object_image_save(o, file, key, flags))
     {
        ERR("Cannot save window to '%s' (key '%s', flags '%s')",
            file, key, flags);
     }
   free(file);
   if (key) free(key);
   if (flags) free(flags);
   ecore_evas_free(ee);
   if (win->shot.repeat_count) win->shot.shot_counter++;
}

static Eina_Bool
_shot_delay(void *data)
{
   Elm_Win *win = data;
   _shot_do(win);
   if (win->shot.repeat_count)
     {
        int remainshot = (win->shot.repeat_count - win->shot.shot_counter);
        if (remainshot > 0) return EINA_TRUE;
     }
   win->shot.timer = NULL;
   elm_exit();
   return EINA_FALSE;
}

static void
_shot_init(Elm_Win *win)
{
   if (!win->shot.info) return;
   win->shot.repeat_count = _shot_repeat_count_get(win);
   win->shot.shot_counter = 0;
}

static void
_shot_handle(Elm_Win *win)
{
   if (!win->shot.info) return;
   win->shot.timer = ecore_timer_add(_shot_delay_get(win), _shot_delay, win);
}

static void
_elm_win_move(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;
   int x, y;

   if (!obj) return;
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_geometry_get(ee, &x, &y, NULL, NULL);
   win->screen.x = x;
   win->screen.y = y;
   evas_object_smart_callback_call(win->win_obj, SIG_MOVED, NULL);
}

static void
_elm_win_resize(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;

   if (!obj) return;
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (win->deferred_resize_job) ecore_job_del(win->deferred_resize_job);
   win->deferred_resize_job = ecore_job_add(_elm_win_resize_job, win);
}

static void
_elm_win_mouse_in(Ecore_Evas *ee)
{
   Evas_Object *obj;
   Elm_Win *win;

   if (!(obj = ecore_evas_object_associate_get(ee))) return;
   if (!(win = elm_widget_data_get(obj))) return;
   if (win->resizing) win->resizing = EINA_FALSE;
}

static void
_elm_win_focus_in(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;

   if (!obj) return;
   win = elm_widget_data_get(obj);
   if (!win) return;
   _elm_widget_top_win_focused_set(win->win_obj, EINA_TRUE);
   if (!elm_widget_focus_order_get(obj))
     {
        elm_widget_focus_steal(win->win_obj);
        win->show_count++;
     }
   else
     elm_widget_focus_restore(win->win_obj);
   evas_object_smart_callback_call(win->win_obj, SIG_FOCUS_IN, NULL);
   win->focus_highlight.cur.visible = EINA_TRUE;
   _elm_win_focus_highlight_reconfigure_job_start(win);
   if (win->frame_obj)
     {
        edje_object_signal_emit(win->frame_obj, "elm,action,focus", "elm");
     }
   else if (win->img_obj)
     {
        /* do nothing */
     }
}

static void
_elm_win_focus_out(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;

   if (!obj) return;
   win = elm_widget_data_get(obj);
   if (!win) return;
   elm_object_focus_set(win->win_obj, EINA_FALSE);
   _elm_widget_top_win_focused_set(win->win_obj, EINA_FALSE);
   evas_object_smart_callback_call(win->win_obj, SIG_FOCUS_OUT, NULL);
   win->focus_highlight.cur.visible = EINA_FALSE;
   _elm_win_focus_highlight_reconfigure_job_start(win);
   if (win->frame_obj)
     {
        edje_object_signal_emit(win->frame_obj, "elm,action,unfocus", "elm");
     }
   else if (win->img_obj)
     {
        /* do nothing */
     }
}

static void
_elm_win_profile_update(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;

   if (!obj) return;
   win = elm_widget_data_get(obj);
   if (!win) return;

   if (win->profile.timer)
     ecore_timer_del(win->profile.timer);
   win->profile.timer = NULL;

   /* TODO: We need the ability to bind a profile to a specific window.
    * Elementary's configuration still has a single global profile for the app.
    */
   _elm_config_profile_set(win->profile.name);

   evas_object_smart_callback_call(win->win_obj, SIG_PROFILE_CHANGED, NULL);
}

static Eina_Bool
_elm_win_profile_change_delay(void *data)
{
   Elm_Win *win = data;
   const char *profile;
   Eina_Bool changed = EINA_FALSE;

   profile = eina_list_nth(win->profile.names, 0);
   if (profile)
     {
        if (win->profile.name)
          {
             if (strcmp(win->profile.name, profile))
               {
                  eina_stringshare_replace(&(win->profile.name), profile);
                  changed = EINA_TRUE;
               }
          }
        else
          {
             win->profile.name = eina_stringshare_add(profile);
             changed = EINA_TRUE;
          }
     }
   win->profile.timer = NULL;
   if (changed) _elm_win_profile_update(win->ee);
   return EINA_FALSE;
}

static void
_elm_win_state_change(Ecore_Evas *ee)
{
   Evas_Object *obj;
   Elm_Win *win;
   Eina_Bool ch_withdrawn = EINA_FALSE;
   Eina_Bool ch_sticky = EINA_FALSE;
   Eina_Bool ch_iconified = EINA_FALSE;
   Eina_Bool ch_fullscreen = EINA_FALSE;
   Eina_Bool ch_maximized = EINA_FALSE;
   Eina_Bool ch_profile = EINA_FALSE;
   const char *profile;

   if (!(obj = ecore_evas_object_associate_get(ee))) return;

   if (!(win = elm_widget_data_get(obj))) return;

   if (win->withdrawn != ecore_evas_withdrawn_get(win->ee))
     {
        win->withdrawn = ecore_evas_withdrawn_get(win->ee);
        ch_withdrawn = EINA_TRUE;
     }
   if (win->sticky != ecore_evas_sticky_get(win->ee))
     {
        win->sticky = ecore_evas_sticky_get(win->ee);
        ch_sticky = EINA_TRUE;
     }
   if (win->iconified != ecore_evas_iconified_get(win->ee))
     {
        win->iconified = ecore_evas_iconified_get(win->ee);
        ch_iconified = EINA_TRUE;
     }
   if (win->fullscreen != ecore_evas_fullscreen_get(win->ee))
     {
        win->fullscreen = ecore_evas_fullscreen_get(win->ee);
        ch_fullscreen = EINA_TRUE;
     }
   if (win->maximized != ecore_evas_maximized_get(win->ee))
     {
        win->maximized = ecore_evas_maximized_get(win->ee);
        ch_maximized = EINA_TRUE;
     }
   profile = ecore_evas_profile_get(win->ee);
   if ((profile) &&
       _elm_config_profile_exists(profile))
     {
        if (win->profile.name)
          {
             if (strcmp(win->profile.name, profile))
               {
                  eina_stringshare_replace(&(win->profile.name), profile);
                  ch_profile = EINA_TRUE;
               }
          }
        else
          {
             win->profile.name = eina_stringshare_add(profile);
             ch_profile = EINA_TRUE;
          }
     }
   if ((ch_withdrawn) || (ch_iconified))
     {
        if (win->withdrawn)
          evas_object_smart_callback_call(win->win_obj, SIG_WITHDRAWN, NULL);
        else if (win->iconified)
          evas_object_smart_callback_call(win->win_obj, SIG_ICONIFIED, NULL);
        else
          evas_object_smart_callback_call(win->win_obj, SIG_NORMAL, NULL);
     }
   if (ch_sticky)
     {
        if (win->sticky)
          evas_object_smart_callback_call(win->win_obj, SIG_STICK, NULL);
        else
          evas_object_smart_callback_call(win->win_obj, SIG_UNSTICK, NULL);
     }
   if (ch_fullscreen)
     {
        if (win->fullscreen)
          evas_object_smart_callback_call(win->win_obj, SIG_FULLSCREEN, NULL);
        else
          evas_object_smart_callback_call(win->win_obj, SIG_UNFULLSCREEN, NULL);
     }
   if (ch_maximized)
     {
        if (win->maximized)
          evas_object_smart_callback_call(win->win_obj, SIG_MAXIMIZED, NULL);
        else
          evas_object_smart_callback_call(win->win_obj, SIG_UNMAXIMIZED, NULL);
     }
   if (ch_profile)
     {
        _elm_win_profile_update(win->ee);
     }
}

static Eina_Bool
_elm_win_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   Elm_Win *wd = elm_widget_data_get(obj);
   const Eina_List *items;
   const Eina_List *list;
   void *(*list_data_get) (const Eina_List *list);

   if (!wd)
     return EINA_FALSE;
   list = elm_widget_sub_object_list_get(obj);

   /* Focus chain */
   if (list)
     {
        if (!(items = elm_widget_focus_custom_chain_get(obj)))
          items = list;

        list_data_get = eina_list_data_get;

        elm_widget_focus_list_next_get(obj, items, list_data_get, dir, next);

        if (*next)
          return EINA_TRUE;
     }
   *next = (Evas_Object *)obj;
   return EINA_FALSE;
}

static void
_elm_win_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Elm_Win *win = elm_widget_data_get(obj);
   if (!win) return;

   if (win->img_obj)
      evas_object_focus_set(win->img_obj, elm_widget_focus_get(obj));
   else
      evas_object_focus_set(obj, elm_widget_focus_get(obj));
}

static Eina_Bool
_elm_win_event_cb(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   if (type == EVAS_CALLBACK_KEY_DOWN)
     {
        Evas_Event_Key_Down *ev = event_info;
        if (!strcmp(ev->keyname, "Tab"))
          {
             if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               elm_widget_focus_cycle(obj, ELM_FOCUS_PREVIOUS);
             else
               elm_widget_focus_cycle(obj, ELM_FOCUS_NEXT);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else if ((!strcmp(ev->keyname, "Left")) ||
                 ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
          {
             //TODO : woohyun jung
          }
        else if ((!strcmp(ev->keyname, "Right")) ||
                 ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
          {
             //TODO : woohyun jung
          }
        else if ((!strcmp(ev->keyname, "Up")) ||
                 ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
          {
             //TODO : woohyun jung
          }
        else if ((!strcmp(ev->keyname, "Down")) ||
                 ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
          {
             //TODO : woohyun jung
          }
     }

   return EINA_FALSE;
}

static void
_deferred_ecore_evas_free(void *data)
{
   ecore_evas_free(data);
   _elm_win_deferred_free--;
}

static void
_elm_win_obj_callback_show(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   if (!win->show_count) win->show_count++;
   if (win->shot.info) _shot_handle(win);
}

static void
_elm_win_obj_callback_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   if (win->frame_obj)
     {
        evas_object_hide(win->frame_obj);
     }
   else if (win->img_obj)
     {
        evas_object_hide(win->img_obj);
     }
   if (win->pointer.obj)
     {
        evas_object_hide(win->pointer.obj);
        ecore_evas_hide(win->pointer.ee);
     }
}

static void
_elm_win_obj_callback_img_obj_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;
   win->img_obj = NULL;
}

static void
_elm_win_obj_callback_del(void *data, Evas *e, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = data;
   Evas_Object *child, *child2 = NULL;
   const char *str;

   if (win->parent)
     {
        evas_object_event_callback_del_full(win->parent, EVAS_CALLBACK_DEL,
                                            _elm_win_obj_callback_parent_del, win);
        win->parent = NULL;
     }
   if (win->autodel_clear) *(win->autodel_clear) = -1;
   _elm_win_list = eina_list_remove(_elm_win_list, win->win_obj);
   while (win->subobjs) elm_win_resize_object_del(obj, win->subobjs->data);
   if (win->ee)
     {
        ecore_evas_callback_delete_request_set(win->ee, NULL);
        ecore_evas_callback_resize_set(win->ee, NULL);
     }
   if (win->deferred_resize_job) ecore_job_del(win->deferred_resize_job);
   if (win->deferred_child_eval_job) ecore_job_del(win->deferred_child_eval_job);
   if (win->shot.info) eina_stringshare_del(win->shot.info);
   if (win->shot.timer) ecore_timer_del(win->shot.timer);
   evas_object_event_callback_del_full(win->win_obj, EVAS_CALLBACK_DEL,
                                       _elm_win_obj_callback_del, win);
   child = evas_object_bottom_get(win->evas);
   while (child)
     {
        /* if the object we see *IS* the window object (because we are
         * faking a parent object inside the canvas), then skip it and
         * go to the next one */
        if (child == obj)
          {
             child = evas_object_above_get(child);
             if (!child) break;
          }
        /* if we are using the next object above from the previous loop */
        if (child == child2)
          {
             /* this object has refcounts from the previous loop */
             child2 = evas_object_above_get(child);
             if (child2) evas_object_ref(child2);
             evas_object_del(child);
             /* so unref from previous loop */
             evas_object_unref(child);
             child = child2;
          }
        else
          {
             /* just delete as normal (probably only first object */
             child2 = evas_object_above_get(child);
             if (child2) evas_object_ref(child2);
             evas_object_del(child);
             child = child2;
          }
     }
#ifdef HAVE_ELEMENTARY_X
   if (win->client_message_handler)
     ecore_event_handler_del(win->client_message_handler);
   if (win->property_handler)
     ecore_event_handler_del(win->property_handler);
#endif
   // FIXME: Why are we flushing edje on every window destroy ??
   //   edje_file_cache_flush();
   //   edje_collection_cache_flush();
   //   evas_image_cache_flush(win->evas);
   //   evas_font_cache_flush(win->evas);
   // FIXME: we are in the del handler for the object and delete the canvas
   // that lives under it from the handler... nasty. deferring doesn't help either

   if (win->img_obj)
     {
        evas_object_event_callback_del_full
           (win->img_obj, EVAS_CALLBACK_DEL, _elm_win_obj_callback_img_obj_del, win);
        win->img_obj = NULL;
     }
   else
     {
        if (win->ee)
          {
             ecore_job_add(_deferred_ecore_evas_free, win->ee);
             _elm_win_deferred_free++;
          }
     }

   _elm_win_focus_highlight_shutdown(win);
   eina_stringshare_del(win->focus_highlight.style);

   if (win->title) eina_stringshare_del(win->title);
   if (win->icon_name) eina_stringshare_del(win->icon_name);
   if (win->role) eina_stringshare_del(win->role);
   if (win->icon) evas_object_del(win->icon);

   EINA_LIST_FREE(win->profile.names, str) eina_stringshare_del(str);
   if (win->profile.name) eina_stringshare_del(win->profile.name);
   if (win->profile.timer) ecore_timer_del(win->profile.timer);

   free(win);

   if ((!_elm_win_list) &&
       (elm_policy_get(ELM_POLICY_QUIT) == ELM_POLICY_QUIT_LAST_WINDOW_CLOSED))
     {
        edje_file_cache_flush();
        edje_collection_cache_flush();
        evas_image_cache_flush(e);
        evas_font_cache_flush(e);
        elm_exit();
     }
}


static void
_elm_win_obj_callback_parent_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = data;
   if (obj == win->parent) win->parent = NULL;
}

static void
_elm_win_obj_intercept_move(void *data, Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Elm_Win *win = data;

   if (win->img_obj)
     {
        if ((x != win->screen.x) || (y != win->screen.y))
          {
             win->screen.x = x;
             win->screen.y = y;
             evas_object_smart_callback_call(win->win_obj, SIG_MOVED, NULL);
          }
     }
   else
     {
        evas_object_move(obj, x, y);
     }
}

static void
_elm_win_obj_intercept_show(void *data, Evas_Object *obj)
{
   Elm_Win *win = data;
   // this is called to make sure all smart containers have calculated their
   // sizes BEFORE we show the window to make sure it initially appears at
   // our desired size (ie min size is known first)
   evas_smart_objects_calculate(evas_object_evas_get(obj));
   if (win->frame_obj)
     {
        evas_object_show(win->frame_obj);
     }
   else if (win->img_obj)
     {
        evas_object_show(win->img_obj);
     }
   if (win->pointer.obj)
     {
        ecore_evas_show(win->pointer.ee);
        evas_object_show(win->pointer.obj);
        /* ecore_evas_wayland_pointer_set(win->pointer.ee, 10, 10); */
     }
   evas_object_show(obj);
}

static void
_elm_win_obj_callback_move(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   if (ecore_evas_override_get(win->ee))
     {
        Evas_Coord x, y;

        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        win->screen.x = x;
        win->screen.y = y;
        evas_object_smart_callback_call(win->win_obj, SIG_MOVED, NULL);
     }
   if (win->frame_obj)
     {
        Evas_Coord x, y;

        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        win->screen.x = x;
        win->screen.y = y;

        /* FIXME: We should update ecore_wl_window_location here !! */
     }
   else if (win->img_obj)
     {
        Evas_Coord x, y;

        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        win->screen.x = x;
        win->screen.y = y;
//        evas_object_move(win->img_obj, x, y);
     }
}

static void
_elm_win_obj_callback_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   if (win->frame_obj)
     {
     }
   else if (win->img_obj)
     {
        Evas_Coord w = 1, h = 1;

        evas_object_geometry_get(obj, NULL, NULL, &w, &h);
        if (win->constrain)
          {
             int sw, sh;
             ecore_evas_screen_geometry_get(win->ee, NULL, NULL, &sw, &sh);
             w = MIN(w, sw);
             h = MIN(h, sh);
          }
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        evas_object_image_size_set(win->img_obj, w, h);
     }
}

static void
_elm_win_delete_request(Ecore_Evas *ee)
{
   Evas_Object *obj = ecore_evas_object_associate_get(ee);
   Elm_Win *win;
   if (strcmp(elm_widget_type_get(obj), "win")) return;

   win = elm_widget_data_get(obj);
   if (!win) return;
   int autodel = win->autodel;
   win->autodel_clear = &autodel;
   evas_object_ref(win->win_obj);
   evas_object_smart_callback_call(win->win_obj, SIG_DELETE_REQUEST, NULL);
   // FIXME: if above callback deletes - then the below will be invalid
   if (autodel) evas_object_del(win->win_obj);
   else win->autodel_clear = NULL;
   evas_object_unref(win->win_obj);
}

static void
_elm_win_resize_job(void *data)
{
   Elm_Win *win = data;
   const Eina_List *l;
   Evas_Object *obj;
   int w, h;

   win->deferred_resize_job = NULL;
   ecore_evas_request_geometry_get(win->ee, NULL, NULL, &w, &h);
   if (win->constrain)
     {
        int sw, sh;
        ecore_evas_screen_geometry_get(win->ee, NULL, NULL, &sw, &sh);
        w = MIN(w, sw);
        h = MIN(h, sh);
     }
   if (win->frame_obj)
     {
        evas_object_resize(win->frame_obj, w, h);
     }
   else if (win->img_obj)
     {
     }
   evas_object_resize(win->win_obj, w, h);
   EINA_LIST_FOREACH(win->subobjs, l, obj)
     {
        evas_object_move(obj, 0, 0);
        evas_object_resize(obj, w, h);
     }
}

#ifdef HAVE_ELEMENTARY_X
static void
_elm_win_xwindow_get(Elm_Win *win)
{
   win->xwin = 0;

#define ENGINE_COMPARE(name) (!strcmp(_elm_preferred_engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_X11))
     {
       if (win->ee) win->xwin = ecore_evas_software_x11_window_get(win->ee);
     }
   else if (ENGINE_COMPARE(ELM_SOFTWARE_X11) ||
            ENGINE_COMPARE(ELM_SOFTWARE_FB) ||
            ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE) ||
            ENGINE_COMPARE(ELM_SOFTWARE_SDL) ||
            ENGINE_COMPARE(ELM_SOFTWARE_16_SDL) ||
            ENGINE_COMPARE(ELM_OPENGL_SDL) ||
	    ENGINE_COMPARE(ELM_OPENGL_COCOA))
     {
     }
   else if (ENGINE_COMPARE(ELM_SOFTWARE_16_X11))
     {
        if (win->ee) win->xwin = ecore_evas_software_x11_16_window_get(win->ee);
     }
   else if (ENGINE_COMPARE(ELM_SOFTWARE_8_X11))
     {
        if (win->ee) win->xwin = ecore_evas_software_x11_8_window_get(win->ee);
     }
/* killed
   else if (ENGINE_COMPARE(ELM_XRENDER_X11))
     {
        if (win->ee) win->xwin = ecore_evas_xrender_x11_window_get(win->ee);
     }
 */
   else if (ENGINE_COMPARE(ELM_OPENGL_X11))
     {
        if (win->ee) win->xwin = ecore_evas_gl_x11_window_get(win->ee);
     }
   else if (ENGINE_COMPARE(ELM_SOFTWARE_WIN32))
     {
        if (win->ee) win->xwin = (long)ecore_evas_win32_window_get(win->ee);
     }
#undef ENGINE_COMPARE
}
#endif

#ifdef HAVE_ELEMENTARY_X
static void
_elm_win_xwin_update(Elm_Win *win)
{
   const char *s;
   Elm_Win_Indicator_Mode mode;

   _elm_win_xwindow_get(win);
   if (win->parent)
     {
        Elm_Win *win2;

        win2 = elm_widget_data_get(win->parent);
        if (win2)
          {
             if (win->xwin)
               ecore_x_icccm_transient_for_set(win->xwin, win2->xwin);
          }
     }

   if (!win->xwin) return; /* nothing more to do */

   s = win->title;
   if (!s) s = _elm_appname;
   if (!s) s = "";
   if (win->icon_name) s = win->icon_name;
   ecore_x_icccm_icon_name_set(win->xwin, s);
   ecore_x_netwm_icon_name_set(win->xwin, s);

   s = win->role;
   if (s) ecore_x_icccm_window_role_set(win->xwin, s);

   // set window icon
   if (win->icon)
     {
        void *data;

        data = evas_object_image_data_get(win->icon, EINA_FALSE);
        if (data)
          {
             Ecore_X_Icon ic;
             int w = 0, h = 0, stride, x, y;
             unsigned char *p;
             unsigned int *p2;

             evas_object_image_size_get(win->icon, &w, &h);
             stride = evas_object_image_stride_get(win->icon);
             if ((w > 0) && (h > 0) &&
                 (stride >= (int)(w * sizeof(unsigned int))))
               {
                  ic.width = w;
                  ic.height = h;
                  ic.data = malloc(w * h * sizeof(unsigned int));

                  if (ic.data)
                    {
                       p = (unsigned char *)data;
                       p2 = (unsigned int *)ic.data;
                       for (y = 0; y < h; y++)
                         {
                            for (x = 0; x < w; x++)
                              {
                                 *p2 = *((unsigned int *)p);
                                 p += sizeof(unsigned int);
                                 p2++;
                              }
                            p += (stride - (w * sizeof(unsigned int)));
                         }
                       ecore_x_netwm_icons_set(win->xwin, &ic, 1);
                       free(ic.data);
                    }
               }
             evas_object_image_data_set(win->icon, data);
          }
     }

   switch (win->type)
     {
      case ELM_WIN_BASIC:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_NORMAL);
         break;
      case ELM_WIN_DIALOG_BASIC:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_DIALOG);
         break;
      case ELM_WIN_DESKTOP:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_DESKTOP);
         break;
      case ELM_WIN_DOCK:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_DOCK);
         break;
      case ELM_WIN_TOOLBAR:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_TOOLBAR);
         break;
      case ELM_WIN_MENU:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_MENU);
         break;
      case ELM_WIN_UTILITY:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_UTILITY);
         break;
      case ELM_WIN_SPLASH:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_SPLASH);
         break;
      case ELM_WIN_DROPDOWN_MENU:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_DROPDOWN_MENU);
         break;
      case ELM_WIN_POPUP_MENU:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_POPUP_MENU);
         break;
      case ELM_WIN_TOOLTIP:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_TOOLTIP);
         break;
      case ELM_WIN_NOTIFICATION:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
         break;
      case ELM_WIN_COMBO:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_COMBO);
         break;
      case ELM_WIN_DND:
         ecore_x_netwm_window_type_set(win->xwin, ECORE_X_WINDOW_TYPE_DND);
         break;
      default:
         break;
     }
   ecore_x_e_virtual_keyboard_state_set
      (win->xwin, (Ecore_X_Virtual_Keyboard_State)win->kbdmode);

   mode = ecore_x_e_illume_indicator_state_get(win->xwin);
   if (mode != win->indmode)
     {
        if (mode == ELM_WIN_INDICATOR_SHOW)
          ecore_x_e_illume_indicator_state_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_STATE_ON);
        else if (mode == ELM_WIN_INDICATOR_HIDE)
          ecore_x_e_illume_indicator_state_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_STATE_OFF);
     }
}
#endif

static void
_elm_win_eval_subobjs(Evas_Object *obj)
{
   const Eina_List *l;
   const Evas_Object *child;

   Elm_Win *win = elm_widget_data_get(obj);
   Evas_Coord w, h, minw = -1, minh = -1, maxw = -1, maxh = -1;
   int xx = 1, xy = 1;
   double wx, wy;

   EINA_LIST_FOREACH(win->subobjs, l, child)
     {
        evas_object_size_hint_weight_get(child, &wx, &wy);
        if (wx == 0.0) xx = 0;
        if (wy == 0.0) xy = 0;

        evas_object_size_hint_min_get(child, &w, &h);
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        if (w > minw) minw = w;
        if (h > minh) minh = h;

        evas_object_size_hint_max_get(child, &w, &h);
        if (w < 1) w = -1;
        if (h < 1) h = -1;
        if (maxw == -1) maxw = w;
        else if ((w > 0) && (w < maxw)) maxw = w;
        if (maxh == -1) maxh = h;
        else if ((h > 0) && (h < maxh)) maxh = h;
     }
   if (!xx) maxw = minw;
   else maxw = 32767;
   if (!xy) maxh = minh;
   else maxh = 32767;
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   if ((maxw >= 0) && (w > maxw)) w = maxw;
   if ((maxh >= 0) && (h > maxh)) h = maxh;
   evas_object_resize(obj, w, h);
}

static void
_elm_win_subobj_callback_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = elm_widget_data_get(data);
   win->subobjs = eina_list_remove(win->subobjs, obj);
   _elm_win_eval_subobjs(win->win_obj);
}

static void
_elm_win_subobj_callback_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _elm_win_eval_subobjs(data);
}

void
_elm_win_shutdown(void)
{
   while (_elm_win_list)
     evas_object_del(_elm_win_list->data);
}

void
_elm_win_rescale(Elm_Theme *th, Eina_Bool use_theme)
{
   const Eina_List *l;
   Evas_Object *obj;

   if (!use_theme)
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          elm_widget_theme(obj);
     }
   else
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          elm_widget_theme_specific(obj, th, EINA_FALSE);
     }
}

void
_elm_win_translate(void)
{
   const Eina_List *l;
   Evas_Object *obj;

   EINA_LIST_FOREACH(_elm_win_list, l, obj)
      elm_widget_translate(obj);
}

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool
_elm_win_client_message(void *data, int type __UNUSED__, void *event)
{
   Elm_Win *win = data;
   Ecore_X_Event_Client_Message *e = event;

   if (e->format != 32) return ECORE_CALLBACK_PASS_ON;
   if (e->message_type == ECORE_X_ATOM_E_COMP_FLUSH)
     {
        if ((unsigned)e->data.l[0] == win->xwin)
          {
             Evas *evas = evas_object_evas_get(win->win_obj);
             if (evas)
               {
                  edje_file_cache_flush();
                  edje_collection_cache_flush();
                  evas_image_cache_flush(evas);
                  evas_font_cache_flush(evas);
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_E_COMP_DUMP)
     {
        if ((unsigned)e->data.l[0] == win->xwin)
          {
             Evas *evas = evas_object_evas_get(win->win_obj);
             if (evas)
               {
                  edje_file_cache_flush();
                  edje_collection_cache_flush();
                  evas_image_cache_flush(evas);
                  evas_font_cache_flush(evas);
                  evas_render_dump(evas);
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_elm_win_property_change(void *data, int type __UNUSED__, void *event)
{
   Elm_Win *win = data;
   Ecore_X_Event_Window_Property *e = event;

   if (e->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE)
     {
        if (e->win == win->xwin)
          {
             win->indmode = ecore_x_e_illume_indicator_state_get(e->win);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}
#endif

static void
_elm_win_focus_target_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   win->focus_highlight.geometry_changed = EINA_TRUE;
   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_focus_target_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   win->focus_highlight.geometry_changed = EINA_TRUE;
   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_focus_target_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   win->focus_highlight.cur.target = NULL;

   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_focus_target_callbacks_add(Elm_Win *win)
{
   Evas_Object *obj = win->focus_highlight.cur.target;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                  _elm_win_focus_target_move, win);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                  _elm_win_focus_target_resize, win);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _elm_win_focus_target_del, win);
}

static void
_elm_win_focus_target_callbacks_del(Elm_Win *win)
{
   Evas_Object *obj = win->focus_highlight.cur.target;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE,
                                       _elm_win_focus_target_move, win);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE,
                                       _elm_win_focus_target_resize, win);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                       _elm_win_focus_target_del, win);
}

static Evas_Object *
_elm_win_focus_target_get(Evas_Object *obj)
{
   Evas_Object *o = obj;

   do
     {
        if (elm_widget_is(o))
          {
             if (!elm_widget_highlight_ignore_get(o))
               break;
             o = elm_widget_parent_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
        else
          {
             o = elm_widget_parent_widget_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
     }
   while (o);

   return o;
}

static void
_elm_win_object_focus_in(void *data, Evas *e __UNUSED__, void *event_info)
{
   Evas_Object *obj = event_info, *target;
   Elm_Win *win = data;

   if (win->focus_highlight.cur.target == obj)
     return;

   target = _elm_win_focus_target_get(obj);
   win->focus_highlight.cur.target = target;
   if (elm_widget_highlight_in_theme_get(target))
     win->focus_highlight.cur.handled = EINA_TRUE;
   else
     _elm_win_focus_target_callbacks_add(win);

   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_object_focus_out(void *data, Evas *e __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Win *win = data;

   if (!win->focus_highlight.cur.target)
     return;

   if (!win->focus_highlight.cur.handled)
     _elm_win_focus_target_callbacks_del(win);
   win->focus_highlight.cur.target = NULL;
   win->focus_highlight.cur.handled = EINA_FALSE;

   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_focus_highlight_hide(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   evas_object_hide(obj);
}

static void
_elm_win_focus_highlight_init(Elm_Win *win)
{
   evas_event_callback_add(win->evas, EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN,
                           _elm_win_object_focus_in, win);
   evas_event_callback_add(win->evas,
                           EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT,
                           _elm_win_object_focus_out, win);

   win->focus_highlight.cur.target = evas_focus_get(win->evas);

   win->focus_highlight.top = edje_object_add(win->evas);
   win->focus_highlight.changed_theme = EINA_TRUE;
   edje_object_signal_callback_add(win->focus_highlight.top,
                                   "elm,action,focus,hide,end", "",
                                   _elm_win_focus_highlight_hide, NULL);
   edje_object_signal_callback_add(win->focus_highlight.top,
                                   "elm,action,focus,anim,end", "",
                                   _elm_win_focus_highlight_anim_end, win);
   _elm_win_focus_highlight_reconfigure_job_start(win);
}

static void
_elm_win_focus_highlight_shutdown(Elm_Win *win)
{
   _elm_win_focus_highlight_reconfigure_job_stop(win);
   if (win->focus_highlight.cur.target)
     {
        _elm_win_focus_target_callbacks_del(win);
        win->focus_highlight.cur.target = NULL;
     }
   if (win->focus_highlight.top)
     {
        evas_object_del(win->focus_highlight.top);
        win->focus_highlight.top = NULL;
     }

   evas_event_callback_del_full(win->evas,
                                EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN,
                                _elm_win_object_focus_in, win);
   evas_event_callback_del_full(win->evas,
                                EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT,
                                _elm_win_object_focus_out, win);
}

static void
_elm_win_focus_highlight_visible_set(Elm_Win *win, Eina_Bool visible)
{
   Evas_Object *top;

   top = win->focus_highlight.top;
   if (visible)
     {
        if (top)
          {
             evas_object_show(top);
             edje_object_signal_emit(top, "elm,action,focus,show", "elm");
          }
     }
   else
     {
        if (top)
          edje_object_signal_emit(top, "elm,action,focus,hide", "elm");
     }
}

static void
_elm_win_focus_highlight_reconfigure_job(void *data)
{
   _elm_win_focus_highlight_reconfigure((Elm_Win *)data);
}

static void
_elm_win_focus_highlight_reconfigure_job_start(Elm_Win *win)
{
   if (win->focus_highlight.reconf_job)
     ecore_job_del(win->focus_highlight.reconf_job);
   win->focus_highlight.reconf_job = ecore_job_add(
      _elm_win_focus_highlight_reconfigure_job, win);
}

static void
_elm_win_focus_highlight_reconfigure_job_stop(Elm_Win *win)
{
   if (win->focus_highlight.reconf_job)
     ecore_job_del(win->focus_highlight.reconf_job);
   win->focus_highlight.reconf_job = NULL;
}

static void
_elm_win_focus_highlight_simple_setup(Elm_Win *win, Evas_Object *obj)
{
   Evas_Object *clip, *target = win->focus_highlight.cur.target;
   Evas_Coord x, y, w, h;

   clip = evas_object_clip_get(target);
   evas_object_geometry_get(target, &x, &y, &w, &h);

   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_clip_set(obj, clip);
}

static void
_elm_win_focus_highlight_anim_setup(Elm_Win *win, Evas_Object *obj)
{
   Evas_Coord tx, ty, tw, th;
   Evas_Coord w, h, px, py, pw, ph;
   Edje_Message_Int_Set *m;
   Evas_Object *previous = win->focus_highlight.prev.target;
   Evas_Object *target = win->focus_highlight.cur.target;

   evas_object_geometry_get(win->win_obj, NULL, NULL, &w, &h);
   evas_object_geometry_get(target, &tx, &ty, &tw, &th);
   evas_object_geometry_get(previous, &px, &py, &pw, &ph);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, tw, th);
   evas_object_clip_unset(obj);

   m = alloca(sizeof(*m) + (sizeof(int) * 8));
   m->count = 8;
   m->val[0] = px;
   m->val[1] = py;
   m->val[2] = pw;
   m->val[3] = ph;
   m->val[4] = tx;
   m->val[5] = ty;
   m->val[6] = tw;
   m->val[7] = th;
   edje_object_message_send(obj, EDJE_MESSAGE_INT_SET, 1, m);
}

static void
_elm_win_focus_highlight_anim_end(void *data, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Elm_Win *win = data;
   _elm_win_focus_highlight_simple_setup(win, obj);
}

static void
_elm_win_focus_highlight_reconfigure(Elm_Win *win)
{
   Evas_Object *target = win->focus_highlight.cur.target;
   Evas_Object *previous = win->focus_highlight.prev.target;
   Evas_Object *top = win->focus_highlight.top;
   Eina_Bool visible_changed;
   Eina_Bool common_visible;
   const char *sig = NULL;

   _elm_win_focus_highlight_reconfigure_job_stop(win);

   visible_changed = (win->focus_highlight.cur.visible !=
                      win->focus_highlight.prev.visible);

   if ((target == previous) && (!visible_changed) &&
       (!win->focus_highlight.geometry_changed))
     return;

   if ((previous) && (win->focus_highlight.prev.handled))
     elm_widget_signal_emit(previous, "elm,action,focus_highlight,hide", "elm");

   if (!target)
     common_visible = EINA_FALSE;
   else if (win->focus_highlight.cur.handled)
     {
        common_visible = EINA_FALSE;
        if (win->focus_highlight.cur.visible)
          sig = "elm,action,focus_highlight,show";
        else
          sig = "elm,action,focus_highlight,hide";
     }
   else
     common_visible = win->focus_highlight.cur.visible;

   _elm_win_focus_highlight_visible_set(win, common_visible);
   if (sig)
     elm_widget_signal_emit(target, sig, "elm");

   if ((!target) || (!common_visible) || (win->focus_highlight.cur.handled))
     goto the_end;

   if (win->focus_highlight.changed_theme)
     {
        const char *str;
        if (win->focus_highlight.style)
          str = win->focus_highlight.style;
        else
          str = "default";
        _elm_theme_object_set(win->win_obj, top, "focus_highlight", "top",
                              str);
        win->focus_highlight.changed_theme = EINA_FALSE;

        if (_elm_config->focus_highlight_animate)
          {
             str = edje_object_data_get(win->focus_highlight.top, "animate");
             win->focus_highlight.top_animate = ((str) && (!strcmp(str, "on")));
          }
     }

   if ((win->focus_highlight.top_animate) && (previous) &&
       (!win->focus_highlight.prev.handled))
     _elm_win_focus_highlight_anim_setup(win, top);
   else
     _elm_win_focus_highlight_simple_setup(win, top);
   evas_object_raise(top);

the_end:
   win->focus_highlight.geometry_changed = EINA_FALSE;
   win->focus_highlight.prev = win->focus_highlight.cur;
}

static void
_elm_win_frame_add(Elm_Win *win, const char *style)
{
   evas_output_framespace_set(win->evas, 0, 22, 0, 26);

   win->frame_obj = edje_object_add(win->evas);
   _elm_theme_set(NULL, win->frame_obj, "border", "base", style);
   evas_object_is_frame_object_set(win->frame_obj, EINA_TRUE);
   evas_object_move(win->frame_obj, 0, 0);
   evas_object_resize(win->frame_obj, 1, 1);

   edje_object_signal_callback_add(win->frame_obj, "elm,action,move,start",
                                   "elm", _elm_win_frame_cb_move_start, win);
   edje_object_signal_callback_add(win->frame_obj, "elm,action,resize,start",
                                   "*", _elm_win_frame_cb_resize_start, win);
   edje_object_signal_callback_add(win->frame_obj, "elm,action,minimize",
                                   "elm", _elm_win_frame_cb_minimize, win);
   edje_object_signal_callback_add(win->frame_obj, "elm,action,maximize",
                                   "elm", _elm_win_frame_cb_maximize, win);
   edje_object_signal_callback_add(win->frame_obj, "elm,action,close",
                                   "elm", _elm_win_frame_cb_close, win);
}

static void
_elm_win_frame_cb_move_start(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__)
{
   Elm_Win *win;

   if (!(win = data)) return;
   /* FIXME: Change mouse pointer */

   /* NB: Wayland handles moving surfaces by itself so we cannot 
    * specify a specific x/y we want. Instead, we will pass in the 
    * existing x/y values so they can be recorded as 'previous' position. 
    * The new position will get updated automatically when the move is 
    * finished */

   ecore_evas_wayland_move(win->ee, win->screen.x, win->screen.y);
}

static void
_elm_win_frame_cb_resize_start(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source)
{
   Elm_Win *win;

   if (!(win = data)) return;
   if (win->resizing) return;
   win->resizing = EINA_TRUE;

   /* FIXME: Change mouse pointer */

   if (!strcmp(source, "elm.event.resize.t"))
     win->resize_location = 1;
   else if (!strcmp(source, "elm.event.resize.b"))
     win->resize_location = 2;
   else if (!strcmp(source, "elm.event.resize.l"))
     win->resize_location = 4;
   else if (!strcmp(source, "elm.event.resize.r"))
     win->resize_location = 8;
   else if (!strcmp(source, "elm.event.resize.tl"))
     win->resize_location = 5;
   else if (!strcmp(source, "elm.event.resize.tr"))
     win->resize_location = 9;
   else if (!strcmp(source, "elm.event.resize.bl"))
     win->resize_location = 6;
   else if (!strcmp(source, "elm.event.resize.br"))
     win->resize_location = 10;
   else
     win->resize_location = 0;

   if (win->resize_location > 0)
     ecore_evas_wayland_resize(win->ee, win->resize_location);
}

static void
_elm_win_frame_cb_minimize(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__)
{
   Elm_Win *win;

   if (!(win = data)) return;
   win->iconified = EINA_TRUE;
   ecore_evas_iconified_set(win->ee, EINA_TRUE);
}

static void
_elm_win_frame_cb_maximize(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__)
{
   Elm_Win *win;

   if (!(win = data)) return;
   if (win->maximized) win->maximized = EINA_FALSE;
   else win->maximized = EINA_TRUE;
   ecore_evas_maximized_set(win->ee, win->maximized);
}

static void
_elm_win_frame_cb_close(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *source __UNUSED__)
{
   Elm_Win *win;

   if (!(win = data)) return;
   evas_object_del(win->win_obj);
}

/*
static void
_elm_win_pointer_add(Elm_Win *win, const char *style)
{
   int mw, mh;

   return;

   win->pointer.ee = ecore_evas_wayland_shm_new(NULL, 0, 0, 0, 32, 32, 0);
   ecore_evas_resize(win->pointer.ee, 32, 32);

   win->pointer.evas = ecore_evas_get(win->ee);

   win->pointer.obj = edje_object_add(win->pointer.evas);
   _elm_theme_set(NULL, win->pointer.obj, "pointer", "base", style);
   edje_object_size_min_calc(win->pointer.obj, &mw, &mh);
   evas_object_move(win->pointer.obj, 0, 0);
   evas_object_resize(win->pointer.obj, 32, 32);
   evas_object_show(win->pointer.obj);
}
*/

#ifdef ELM_DEBUG
static void
_debug_key_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     return;

   if ((strcmp(ev->keyname, "F12")) ||
       (!evas_key_modifier_is_set(ev->modifiers, "Control")))
     return;

   printf("Tree graph generated.\n");
   elm_object_tree_dot_dump(obj, "./dump.dot");
}
#endif

static void
_win_img_hide(void        *data,
              Evas        *e __UNUSED__,
              Evas_Object *obj __UNUSED__,
              void        *event_info __UNUSED__)
{
   Elm_Win *win = data;

   elm_widget_focus_hide_handle(win->win_obj);
}

static void
_win_img_mouse_up(void        *data,
                  Evas        *e __UNUSED__,
                  Evas_Object *obj __UNUSED__,
                  void        *event_info)
{
   Elm_Win *win = data;
   Evas_Event_Mouse_Up *ev = event_info;
   if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
      elm_widget_focus_mouse_up_handle(win->win_obj);
}

static void
_win_img_focus_in(void        *data,
                  Evas        *e __UNUSED__,
                  Evas_Object *obj __UNUSED__,
                  void        *event_info __UNUSED__)
{
   Elm_Win *win = data;
   elm_widget_focus_steal(win->win_obj);
}

static void
_win_img_focus_out(void        *data,
                   Evas        *e __UNUSED__,
                   Evas_Object *obj __UNUSED__,
                   void        *event_info __UNUSED__)
{
   Elm_Win *win = data;
   elm_widget_focused_object_clear(win->win_obj);
}

static void
_win_inlined_image_set(Elm_Win *win)
{
   evas_object_image_alpha_set(win->img_obj, EINA_FALSE);
   evas_object_image_filled_set(win->img_obj, EINA_TRUE);
   evas_object_event_callback_add(win->img_obj, EVAS_CALLBACK_DEL,
                                  _elm_win_obj_callback_img_obj_del, win);

   evas_object_event_callback_add(win->img_obj, EVAS_CALLBACK_HIDE,
                                  _win_img_hide, win);
   evas_object_event_callback_add(win->img_obj, EVAS_CALLBACK_MOUSE_UP,
                                  _win_img_mouse_up, win);
   evas_object_event_callback_add(win->img_obj, EVAS_CALLBACK_FOCUS_IN,
                                  _win_img_focus_in, win);
   evas_object_event_callback_add(win->img_obj, EVAS_CALLBACK_FOCUS_OUT,
                                  _win_img_focus_out, win);
}

static void
_subobj_del(Elm_Win *win, Evas_Object *obj, Evas_Object *subobj)
{
   evas_object_event_callback_del_full(subobj,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _elm_win_subobj_callback_changed_size_hints,
                                       obj);
   evas_object_event_callback_del_full(subobj, EVAS_CALLBACK_DEL,
                                       _elm_win_subobj_callback_del, obj);
   win->subobjs = eina_list_remove(win->subobjs, subobj);
   _elm_win_eval_subobjs(obj);
}

static void
_elm_win_obj_icon_callback_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win *win = data;
   if (win->icon == obj) win->icon = NULL;
}

EAPI Evas_Object *
elm_win_add(Evas_Object *parent, const char *name, Elm_Win_Type type)
{
   Elm_Win *win;
   const Eina_List *l;
   const char *fontpath;

   win = ELM_NEW(Elm_Win);

#define FALLBACK_TRY(engine)                                            \
   if (!win->ee)                                                        \
      do {                                                              \
         CRITICAL(engine " engine creation failed. Trying default.");   \
         win->ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);              \
         if (win->ee)                                                   \
            elm_config_preferred_engine_set(ecore_evas_engine_name_get(win->ee)); \
   } while (0)
#define ENGINE_COMPARE(name) (_elm_preferred_engine && !strcmp(_elm_preferred_engine, name))

   win->kbdmode = ELM_WIN_KEYBOARD_UNKNOWN;
   win->indmode = ELM_WIN_INDICATOR_UNKNOWN;

   switch (type)
     {
      case ELM_WIN_INLINED_IMAGE:
        if (!parent) break;
        {
           Evas *e = evas_object_evas_get(parent);
           Ecore_Evas *ee;
           if (!e) break;
           ee = ecore_evas_ecore_evas_get(e);
           if (!ee) break;
           win->img_obj = ecore_evas_object_image_new(ee);
           if (!win->img_obj) break;
           win->ee = ecore_evas_object_ecore_evas_get(win->img_obj);
           if (win->ee)
             {
                _win_inlined_image_set(win);
                break;
             }
           evas_object_del(win->img_obj);
           win->img_obj = NULL;
        }
        break;

      case ELM_WIN_SOCKET_IMAGE:
        win->ee = ecore_evas_extn_socket_new(1, 1);
        break;

      default:
        if (ENGINE_COMPARE(ELM_SOFTWARE_X11))
          {
             win->ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
#ifdef HAVE_ELEMENTARY_X
             win->client_message_handler = ecore_event_handler_add
                (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, win);
             win->property_handler = ecore_event_handler_add
                (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, win);
#endif
             FALLBACK_TRY("Sofware X11");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_FB))
          {
             win->ee = ecore_evas_fb_new(NULL, 0, 1, 1);
             FALLBACK_TRY("Sofware FB");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_DIRECTFB))
          {
             win->ee = ecore_evas_directfb_new(NULL, 1, 0, 0, 1, 1);
             FALLBACK_TRY("Sofware DirectFB");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_X11))
          {
             win->ee = ecore_evas_software_x11_16_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("Sofware-16");
#ifdef HAVE_ELEMENTARY_X
             win->client_message_handler = ecore_event_handler_add
                (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, win);
             win->property_handler = ecore_event_handler_add
                (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, win);
#endif
     }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_8_X11))
          {
             win->ee = ecore_evas_software_x11_8_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("Sofware-8");
#ifdef HAVE_ELEMENTARY_X
             win->client_message_handler = ecore_event_handler_add
                (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, win);
             win->property_handler = ecore_event_handler_add
                (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, win);
#endif
          }
/* killed
        else if (ENGINE_COMPARE(ELM_XRENDER_X11))
          {
             win->ee = ecore_evas_xrender_x11_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("XRender");
#ifdef HAVE_ELEMENTARY_X
             win->client_message_handler = ecore_event_handler_add
                (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, win);
             win->property_handler = ecore_event_handler_add
                (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, win);
#endif
          }
 */
        else if (ENGINE_COMPARE(ELM_OPENGL_X11))
          {
             int opt[10];
             int opt_i = 0;

             if (_elm_config->vsync)
               {
                  opt[opt_i] = ECORE_EVAS_GL_X11_OPT_VSYNC;
                  opt_i++;
                  opt[opt_i] = 1;
                  opt_i++;
               }
             if (opt_i > 0)
                win->ee = ecore_evas_gl_x11_options_new(NULL, 0, 0, 0, 1, 1, opt);
             else
                win->ee = ecore_evas_gl_x11_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("OpenGL");
#ifdef HAVE_ELEMENTARY_X
             win->client_message_handler = ecore_event_handler_add
                (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, win);
             win->property_handler = ecore_event_handler_add
                (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, win);
#endif
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_WIN32))
          {
             win->ee = ecore_evas_software_gdi_new(NULL, 0, 0, 1, 1);
             FALLBACK_TRY("Sofware Win32");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
          {
             win->ee = ecore_evas_software_wince_gdi_new(NULL, 0, 0, 1, 1);
             FALLBACK_TRY("Sofware-16-WinCE");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_PSL1GHT))
          {
             win->ee = ecore_evas_psl1ght_new(NULL, 1, 1);
             FALLBACK_TRY("PSL1GHT");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_SDL))
          {
             win->ee = ecore_evas_sdl_new(NULL, 0, 0, 0, 0, 0, 1);
             FALLBACK_TRY("Sofware SDL");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_SDL))
          {
             win->ee = ecore_evas_sdl16_new(NULL, 0, 0, 0, 0, 0, 1);
             FALLBACK_TRY("Sofware-16-SDL");
          }
        else if (ENGINE_COMPARE(ELM_OPENGL_SDL))
          {
             win->ee = ecore_evas_gl_sdl_new(NULL, 1, 1, 0, 0);
             FALLBACK_TRY("OpenGL SDL");
          }
        else if (ENGINE_COMPARE(ELM_OPENGL_COCOA))
          {
             win->ee = ecore_evas_cocoa_new(NULL, 1, 1, 0, 0);
             FALLBACK_TRY("OpenGL Cocoa");
          }
        else if (ENGINE_COMPARE(ELM_BUFFER))
          {
             win->ee = ecore_evas_buffer_new(1, 1);
          }
        else if (ENGINE_COMPARE(ELM_EWS))
          {
             win->ee = ecore_evas_ews_new(0, 0, 1, 1);
          }
        else if (ENGINE_COMPARE(ELM_WAYLAND_SHM))
          {
             win->ee = ecore_evas_wayland_shm_new(NULL, 0, 0, 0, 1, 1, 0);
	     win->evas = ecore_evas_get(win->ee);

             _elm_win_frame_add(win, "default");
//             _elm_win_pointer_add(win, "default");
          }
        else if (ENGINE_COMPARE(ELM_WAYLAND_EGL))
          {
             win->ee = ecore_evas_wayland_egl_new(NULL, 0, 0, 0, 1, 1, 0);
	     win->evas = ecore_evas_get(win->ee);

             _elm_win_frame_add(win, "default");
//             _elm_win_pointer_add(win, "default");
          }
        else if (!strncmp(_elm_preferred_engine, "shot:", 5))
          {
             win->ee = ecore_evas_buffer_new(1, 1);
             ecore_evas_manual_render_set(win->ee, EINA_TRUE);
             win->shot.info = eina_stringshare_add(_elm_preferred_engine + 5);
             _shot_init(win);
          }
#undef FALLBACK_TRY
        break;
     }

   if (!win->ee)
     {
        ERR("Cannot create window.");
        free(win);
        return NULL;
     }
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
#endif
   if ((_elm_config->bgpixmap) && (!_elm_config->compositing))
     ecore_evas_avoid_damage_set(win->ee, ECORE_EVAS_AVOID_DAMAGE_EXPOSE);
   // bg pixmap done by x - has other issues like can be redrawn by x before it
   // is filled/ready by app
   //     ecore_evas_avoid_damage_set(win->ee, ECORE_EVAS_AVOID_DAMAGE_BUILT_IN);

   win->type = type;
   win->parent = parent;
   if (win->parent)
     evas_object_event_callback_add(win->parent, EVAS_CALLBACK_DEL,
                                    _elm_win_obj_callback_parent_del, win);

   win->evas = ecore_evas_get(win->ee);
   win->win_obj = elm_widget_add(win->evas);
   elm_widget_type_set(win->win_obj, "win");
   ELM_SET_WIDTYPE(widtype, "win");
   elm_widget_data_set(win->win_obj, win);
   elm_widget_event_hook_set(win->win_obj, _elm_win_event_cb);
   elm_widget_on_focus_hook_set(win->win_obj, _elm_win_on_focus_hook, NULL);
   elm_widget_can_focus_set(win->win_obj, EINA_TRUE);
   elm_widget_highlight_ignore_set(win->win_obj, EINA_TRUE);
   elm_widget_focus_next_hook_set(win->win_obj, _elm_win_focus_next_hook);
   evas_object_color_set(win->win_obj, 0, 0, 0, 0);
   evas_object_move(win->win_obj, 0, 0);
   evas_object_resize(win->win_obj, 1, 1);
   evas_object_layer_set(win->win_obj, 50);
   evas_object_pass_events_set(win->win_obj, EINA_TRUE);

   if (win->frame_obj)
     {
        evas_object_clip_set(win->win_obj, win->frame_obj);
        evas_object_stack_below(win->frame_obj, win->win_obj);
     }

   if (type == ELM_WIN_INLINED_IMAGE)
     elm_widget_parent2_set(win->win_obj, parent);
   ecore_evas_object_associate(win->ee, win->win_obj,
                               ECORE_EVAS_OBJECT_ASSOCIATE_BASE |
                               ECORE_EVAS_OBJECT_ASSOCIATE_STACK |
                               ECORE_EVAS_OBJECT_ASSOCIATE_LAYER);
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_SHOW,
                                  _elm_win_obj_callback_show, win);
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_HIDE,
                                  _elm_win_obj_callback_hide, win);
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_DEL,
                                  _elm_win_obj_callback_del, win);
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_MOVE,
                                  _elm_win_obj_callback_move, win);
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_RESIZE,
                                  _elm_win_obj_callback_resize, win);
   if (win->img_obj)
     evas_object_intercept_move_callback_add(win->win_obj,
                                             _elm_win_obj_intercept_move, win);
   evas_object_intercept_show_callback_add(win->win_obj,
                                           _elm_win_obj_intercept_show, win);

   evas_object_smart_callback_add(win->win_obj, "sub-object-del", (Evas_Smart_Cb)_subobj_del, win);
   ecore_evas_name_class_set(win->ee, name, _elm_appname);
   ecore_evas_callback_delete_request_set(win->ee, _elm_win_delete_request);
   ecore_evas_callback_resize_set(win->ee, _elm_win_resize);
   ecore_evas_callback_mouse_in_set(win->ee, _elm_win_mouse_in);
   ecore_evas_callback_focus_in_set(win->ee, _elm_win_focus_in);
   ecore_evas_callback_focus_out_set(win->ee, _elm_win_focus_out);
   ecore_evas_callback_move_set(win->ee, _elm_win_move);
   ecore_evas_callback_state_change_set(win->ee, _elm_win_state_change);
   evas_image_cache_set(win->evas, (_elm_config->image_cache * 1024));
   evas_font_cache_set(win->evas, (_elm_config->font_cache * 1024));
   EINA_LIST_FOREACH(_elm_config->font_dirs, l, fontpath)
     evas_font_path_append(win->evas, fontpath);
   if (!_elm_config->font_hinting)
     evas_font_hinting_set(win->evas, EVAS_FONT_HINTING_NONE);
   else if (_elm_config->font_hinting == 1)
     evas_font_hinting_set(win->evas, EVAS_FONT_HINTING_AUTO);
   else if (_elm_config->font_hinting == 2)
     evas_font_hinting_set(win->evas, EVAS_FONT_HINTING_BYTECODE);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif

   _elm_win_list = eina_list_append(_elm_win_list, win->win_obj);

   if (ENGINE_COMPARE(ELM_SOFTWARE_FB))
     {
        ecore_evas_fullscreen_set(win->ee, 1);
     }
#undef ENGINE_COMPARE

   if (_elm_config->focus_highlight_enable)
     elm_win_focus_highlight_enabled_set(win->win_obj, EINA_TRUE);

#ifdef ELM_DEBUG
   Evas_Modifier_Mask mask = evas_key_modifier_mask_get(win->evas, "Control");
   evas_object_event_callback_add(win->win_obj, EVAS_CALLBACK_KEY_DOWN,
                                  _debug_key_down, win);

   Eina_Bool ret = evas_object_key_grab(win->win_obj, "F12", mask, 0,
                                        EINA_TRUE);
   printf("Ctrl+F12 key combination exclusive for dot tree generation\n");
#endif

   evas_object_smart_callbacks_descriptions_set(win->win_obj, _signals);

   return win->win_obj;
}

EAPI Evas_Object *
elm_win_util_standard_add(const char *name, const char *title)
{
   Evas_Object *win, *bg;

   win = elm_win_add(NULL, name, ELM_WIN_BASIC);
   if (!win) return NULL;
   elm_win_title_set(win, title);
   bg = elm_bg_add(win);
   if (!bg)
     {
        evas_object_del(win);
        return NULL;
     }
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);
   return win;
}

EAPI void
elm_win_resize_object_add(Evas_Object *obj, Evas_Object *subobj)
{
   Evas_Coord w, h;
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (eina_list_data_find(win->subobjs, subobj)) return;
   win->subobjs = eina_list_append(win->subobjs, subobj);
   elm_widget_sub_object_add(obj, subobj);
   evas_object_event_callback_add(subobj, EVAS_CALLBACK_DEL,
                                  _elm_win_subobj_callback_del, obj);
   evas_object_event_callback_add(subobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _elm_win_subobj_callback_changed_size_hints,
                                  obj);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_move(subobj, 0, 0);
   evas_object_resize(subobj, w, h);
   _elm_win_eval_subobjs(obj);
}

EAPI void
elm_win_resize_object_del(Evas_Object *obj, Evas_Object *subobj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   evas_object_event_callback_del_full(subobj,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _elm_win_subobj_callback_changed_size_hints,
                                       obj);
   evas_object_event_callback_del_full(subobj, EVAS_CALLBACK_DEL,
                                       _elm_win_subobj_callback_del, obj);
   win->subobjs = eina_list_remove(win->subobjs, subobj);
   elm_widget_sub_object_del(obj, subobj);
   _elm_win_eval_subobjs(obj);
}

EAPI void
elm_win_title_set(Evas_Object *obj, const char *title)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win || !title) return;
   eina_stringshare_replace(&(win->title), title);
   ecore_evas_title_set(win->ee, win->title);
   if (win->frame_obj)
     edje_object_part_text_escaped_set(win->frame_obj, "elm.text.title", win->title);
}

EAPI const char *
elm_win_title_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   return win->title;
}

EAPI void
elm_win_icon_name_set(Evas_Object *obj, const char *icon_name)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win || !icon_name) return;
   eina_stringshare_replace(&(win->icon_name), icon_name);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI const char *
elm_win_icon_name_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   return win->icon_name;
}

EAPI void
elm_win_role_set(Evas_Object *obj, const char *role)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win || !role) return;
   eina_stringshare_replace(&(win->role), role);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI const char *
elm_win_role_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   return win->role;
}

EAPI void
elm_win_icon_object_set(Evas_Object *obj, Evas_Object *icon)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (win->icon)
     evas_object_event_callback_del_full(win->icon, EVAS_CALLBACK_DEL,
                                         _elm_win_obj_icon_callback_del, win);
   win->icon = icon;
   if (win->icon)
     evas_object_event_callback_add(win->icon, EVAS_CALLBACK_DEL,
                                    _elm_win_obj_icon_callback_del, win);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI const Evas_Object *
elm_win_icon_object_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   return win->icon;
}

EAPI void
elm_win_autodel_set(Evas_Object *obj, Eina_Bool autodel)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->autodel = autodel;
}

EAPI Eina_Bool
elm_win_autodel_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->autodel;
}

EAPI void
elm_win_activate(Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_activate(win->ee);
}

EAPI void
elm_win_lower(Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_lower(win->ee);
}

EAPI void
elm_win_raise(Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_raise(win->ee);
}

EAPI void
elm_win_center(Evas_Object *obj, Eina_Bool h, Eina_Bool v)
{
   Elm_Win *win;
   int win_w, win_h, screen_w, screen_h, nx, ny;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_screen_geometry_get(win->ee, NULL, NULL, &screen_w, &screen_h);
   if ((!screen_w) || (!screen_h)) return;
   evas_object_geometry_get(obj, NULL, NULL, &win_w, &win_h);
   if ((!win_w) || (!win_h)) return;
   if (h) nx = win_w >= screen_w ? 0 : (screen_w / 2) - (win_w / 2);
   else nx = win->screen.x;
   if (v) ny = win_h >= screen_h ? 0 : (screen_h / 2) - (win_h / 2);
   else ny = win->screen.y;
   if (nx < 0) nx = 0;
   if (ny < 0) ny = 0;
   evas_object_move(obj, nx, ny);
}

EAPI void
elm_win_borderless_set(Evas_Object *obj, Eina_Bool borderless)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_borderless_set(win->ee, borderless);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_borderless_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return ecore_evas_borderless_get(win->ee);
}

EAPI void
elm_win_shaped_set(Evas_Object *obj, Eina_Bool shaped)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_shaped_set(win->ee, shaped);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_shaped_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return ecore_evas_shaped_get(win->ee);
}

EAPI void
elm_win_alpha_set(Evas_Object *obj, Eina_Bool alpha)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (win->frame_obj)
     {
     }
   else if (win->img_obj)
     {
        evas_object_image_alpha_set(win->img_obj, alpha);
        ecore_evas_alpha_set(win->ee, alpha);
     }
   else
     {
#ifdef HAVE_ELEMENTARY_X
        if (win->xwin)
          {
             if (alpha)
               {
                  if (!_elm_config->compositing)
                     elm_win_shaped_set(obj, alpha);
                  else
                     ecore_evas_alpha_set(win->ee, alpha);
               }
             else
                ecore_evas_alpha_set(win->ee, alpha);
             _elm_win_xwin_update(win);
          }
        else
#endif
           ecore_evas_alpha_set(win->ee, alpha);
     }
}

EAPI Eina_Bool
elm_win_alpha_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   if (win->frame_obj)
     {
     }
   else if (win->img_obj)
     {
        return evas_object_image_alpha_get(win->img_obj);
     }
   return ecore_evas_alpha_get(win->ee);
}

EAPI void
elm_win_override_set(Evas_Object *obj, Eina_Bool override)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_override_set(win->ee, override);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_override_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return ecore_evas_override_get(win->ee);
}

EAPI void
elm_win_fullscreen_set(Evas_Object *obj, Eina_Bool fullscreen)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   // YYY: handle if win->img_obj
#define ENGINE_COMPARE(name) (!strcmp(_elm_preferred_engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_FB) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
     {
        // these engines... can ONLY be fullscreen
        return;
     }
   else
     {
        win->fullscreen = fullscreen;
        ecore_evas_fullscreen_set(win->ee, fullscreen);
#ifdef HAVE_ELEMENTARY_X
        _elm_win_xwin_update(win);
#endif
     }
#undef ENGINE_COMPARE
}

EAPI Eina_Bool
elm_win_fullscreen_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
#define ENGINE_COMPARE(name) (!strcmp(_elm_preferred_engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_FB) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
     {
        // these engines... can ONLY be fullscreen
        return EINA_TRUE;
     }
   else
     {
        return win->fullscreen;
     }
#undef ENGINE_COMPARE
}

EAPI void
elm_win_maximized_set(Evas_Object *obj, Eina_Bool maximized)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->maximized = maximized;
   // YYY: handle if win->img_obj
   ecore_evas_maximized_set(win->ee, maximized);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_maximized_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->maximized;
}

EAPI void
elm_win_iconified_set(Evas_Object *obj, Eina_Bool iconified)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->iconified = iconified;
   ecore_evas_iconified_set(win->ee, iconified);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_iconified_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->iconified;
}

EAPI void
elm_win_withdrawn_set(Evas_Object *obj, Eina_Bool withdrawn)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->withdrawn = withdrawn;
   ecore_evas_withdrawn_set(win->ee, withdrawn);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_withdrawn_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->withdrawn;
}

EAPI void
elm_win_profiles_set(Evas_Object *obj, const char **profiles, unsigned int num_profiles)
{
   Elm_Win *win;
   char **profiles_int;
   const char *str;
   unsigned int i, num;
   Eina_List *l;

   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (!profiles) return;

   if (win->profile.timer) ecore_timer_del(win->profile.timer);
   win->profile.timer = ecore_timer_add(0.1, _elm_win_profile_change_delay, win);
   EINA_LIST_FREE(win->profile.names, str) eina_stringshare_del(str);

   for (i = 0; i < num_profiles; i++)
     {
        if ((profiles[i]) &&
            _elm_config_profile_exists(profiles[i]))
          {
             str = eina_stringshare_add(profiles[i]);
             win->profile.names = eina_list_append(win->profile.names, str);
          }
     }

   num = eina_list_count(win->profile.names);
   profiles_int = alloca(num * sizeof(char *));

   if (profiles_int)
     {
        i = 0;
        EINA_LIST_FOREACH(win->profile.names, l, str)
          {
             if (str)
               profiles_int[i] = strdup(str);
             else
               profiles_int[i] = NULL;
             i++;
          }
        ecore_evas_profiles_set(win->ee, (const char **)profiles_int, i);
        for (i = 0; i < num; i++)
          {
             if (profiles_int[i]) free(profiles_int[i]);
          }
     }
   else
     ecore_evas_profiles_set(win->ee, profiles, num_profiles);
}

EAPI const char *
elm_win_profile_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return NULL;

   return win->profile.name;
}

EAPI void
elm_win_urgent_set(Evas_Object *obj, Eina_Bool urgent)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->urgent = urgent;
   ecore_evas_urgent_set(win->ee, urgent);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_urgent_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->urgent;
}

EAPI void
elm_win_demand_attention_set(Evas_Object *obj, Eina_Bool demand_attention)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->demand_attention = demand_attention;
   ecore_evas_demand_attention_set(win->ee, demand_attention);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_demand_attention_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->demand_attention;
}

EAPI void
elm_win_modal_set(Evas_Object *obj, Eina_Bool modal)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->modal = modal;
   ecore_evas_modal_set(win->ee, modal);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_modal_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->modal;
}

EAPI void
elm_win_aspect_set(Evas_Object *obj, double aspect)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->aspect = aspect;
   ecore_evas_aspect_set(win->ee, aspect);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI double
elm_win_aspect_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->aspect;
}

EAPI void
elm_win_layer_set(Evas_Object *obj, int layer)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_layer_set(win->ee, layer);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI int
elm_win_layer_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   win = elm_widget_data_get(obj);
   if (!win) return -1;
   return ecore_evas_layer_get(win->ee);
}

EAPI void
elm_win_rotation_set(Evas_Object *obj, int rotation)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (win->rot == rotation) return;
   win->rot = rotation;
   ecore_evas_rotation_set(win->ee, rotation);
   evas_object_size_hint_min_set(obj, -1, -1);
   evas_object_size_hint_max_set(obj, -1, -1);
   _elm_win_eval_subobjs(obj);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI void
elm_win_rotation_with_resize_set(Evas_Object *obj, int rotation)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (win->rot == rotation) return;
   win->rot = rotation;
   ecore_evas_rotation_with_resize_set(win->ee, rotation);
   evas_object_size_hint_min_set(obj, -1, -1);
   evas_object_size_hint_max_set(obj, -1, -1);
   _elm_win_eval_subobjs(obj);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI int
elm_win_rotation_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   win = elm_widget_data_get(obj);
   if (!win) return -1;
   return win->rot;
}

EAPI void
elm_win_sticky_set(Evas_Object *obj, Eina_Bool sticky)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->sticky = sticky;
   ecore_evas_sticky_set(win->ee, sticky);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(win);
#endif
}

EAPI Eina_Bool
elm_win_sticky_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->sticky;
}

EAPI void
elm_win_keyboard_mode_set(Evas_Object *obj, Elm_Win_Keyboard_Mode mode)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (mode == win->kbdmode) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
#endif
   win->kbdmode = mode;
#ifdef HAVE_ELEMENTARY_X
   if (win->xwin)
     ecore_x_e_virtual_keyboard_state_set
        (win->xwin, (Ecore_X_Virtual_Keyboard_State)win->kbdmode);
#endif
}

EAPI Elm_Win_Keyboard_Mode
elm_win_keyboard_mode_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_WIN_KEYBOARD_UNKNOWN;
   win = elm_widget_data_get(obj);
   if (!win) return ELM_WIN_KEYBOARD_UNKNOWN;
   return win->kbdmode;
}

EAPI void
elm_win_keyboard_win_set(Evas_Object *obj, Eina_Bool is_keyboard)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     ecore_x_e_virtual_keyboard_set(win->xwin, is_keyboard);
#else
   (void) is_keyboard;
#endif
}

EAPI Eina_Bool
elm_win_keyboard_win_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_virtual_keyboard_get(win->xwin);
#endif
   return EINA_FALSE;
}

// WRAPPER: Temperary added.
EAPI void
elm_win_indicator_state_set(Evas_Object *obj, Elm_Win_Indicator_Mode mode)
{
   elm_win_indicator_mode_set(obj, mode);
}

EAPI void
elm_win_indicator_mode_set(Evas_Object *obj, Elm_Win_Indicator_Mode mode)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (mode == win->indmode) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
#endif
   win->indmode = mode;
#ifdef HAVE_ELEMENTARY_X
   if (win->xwin)
     {
        if (win->indmode == ELM_WIN_INDICATOR_SHOW)
          ecore_x_e_illume_indicator_state_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_STATE_ON);
        else if (win->indmode == ELM_WIN_INDICATOR_HIDE)
          ecore_x_e_illume_indicator_state_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_STATE_OFF);
     }
#endif
}

// WRAPPER: Temperary added.
EAPI Elm_Win_Indicator_Mode
elm_win_indicator_state_get(const Evas_Object *obj)
{
   return elm_win_indicator_mode_get(obj);
}

EAPI Elm_Win_Indicator_Mode
elm_win_indicator_mode_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_WIN_INDICATOR_UNKNOWN;
   win = elm_widget_data_get(obj);
   if (!win) return ELM_WIN_INDICATOR_UNKNOWN;
   return win->indmode;
}

EAPI void
elm_win_indicator_opacity_set(Evas_Object *obj, Elm_Win_Indicator_Opacity_Mode mode)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (mode == win->ind_o_mode) return;
   win->ind_o_mode = mode;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     {
        if (win->ind_o_mode == ELM_WIN_INDICATOR_OPAQUE)
          ecore_x_e_illume_indicator_opacity_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_OPAQUE);
        else if (win->ind_o_mode == ELM_WIN_INDICATOR_TRANSLUCENT)
          ecore_x_e_illume_indicator_opacity_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_TRANSLUCENT);
        else if (win->ind_o_mode == ELM_WIN_INDICATOR_TRANSPARENT)
          ecore_x_e_illume_indicator_opacity_set
          (win->xwin, ECORE_X_ILLUME_INDICATOR_TRANSPARENT);

     }
#endif
}

EAPI Elm_Win_Indicator_Opacity_Mode
elm_win_indicator_opacity_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) ELM_WIN_INDICATOR_OPACITY_UNKNOWN;
   win = elm_widget_data_get(obj);
   if (!win) return ELM_WIN_INDICATOR_OPACITY_UNKNOWN;
   return win->ind_o_mode;
}

EAPI void
elm_win_screen_position_get(const Evas_Object *obj, int *x, int *y)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (x) *x = win->screen.x;
   if (y) *y = win->screen.y;
}

EAPI Eina_Bool
elm_win_focus_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return ecore_evas_focus_get(win->ee);
}

EAPI void
elm_win_screen_constrain_set(Evas_Object *obj, Eina_Bool constrain)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->constrain = !!constrain;
}

EAPI Eina_Bool
elm_win_screen_constrain_get(Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   return win->constrain;
}

EAPI void
elm_win_screen_size_get(const Evas_Object *obj, int *x, int *y, int *w, int *h)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   ecore_evas_screen_geometry_get(win->ee, x, y, w, h);
}

EAPI void
elm_win_conformant_set(Evas_Object *obj, Eina_Bool conformant)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     ecore_x_e_illume_conformant_set(win->xwin, conformant);
#else
   (void) conformant;
#endif
}

EAPI Eina_Bool
elm_win_conformant_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_illume_conformant_get(win->xwin);
#endif
   return EINA_FALSE;
}

EAPI void
elm_win_quickpanel_set(Evas_Object *obj, Eina_Bool quickpanel)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     {
        ecore_x_e_illume_quickpanel_set(win->xwin, quickpanel);
        if (quickpanel)
          {
             Ecore_X_Window_State states[2];

             states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
             states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
             ecore_x_netwm_window_state_set(win->xwin, states, 2);
             ecore_x_icccm_hints_set(win->xwin, 0, 0, 0, 0, 0, 0, 0);
          }
     }
#else
   (void) quickpanel;
#endif
}

EAPI Eina_Bool
elm_win_quickpanel_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_illume_quickpanel_get(win->xwin);
#endif
   return EINA_FALSE;
}

EAPI void
elm_win_quickpanel_priority_major_set(Evas_Object *obj, int priority)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     ecore_x_e_illume_quickpanel_priority_major_set(win->xwin, priority);
#else
   (void) priority;
#endif
}

EAPI int
elm_win_quickpanel_priority_major_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   win = elm_widget_data_get(obj);
   if (!win) return -1;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_illume_quickpanel_priority_major_get(win->xwin);
#endif
   return -1;
}

EAPI void
elm_win_quickpanel_priority_minor_set(Evas_Object *obj, int priority)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     ecore_x_e_illume_quickpanel_priority_minor_set(win->xwin, priority);
#else
   (void) priority;
#endif
}

EAPI int
elm_win_quickpanel_priority_minor_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   win = elm_widget_data_get(obj);
   if (!win) return -1;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_illume_quickpanel_priority_minor_get(win->xwin);
#endif
   return -1;
}

EAPI void
elm_win_quickpanel_zone_set(Evas_Object *obj, int zone)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     ecore_x_e_illume_quickpanel_zone_set(win->xwin, zone);
#else
   (void) zone;
#endif
}

EAPI int
elm_win_quickpanel_zone_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   win = elm_widget_data_get(obj);
   if (!win) return 0;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     return ecore_x_e_illume_quickpanel_zone_get(win->xwin);
#endif
   return 0;
}

EAPI void
elm_win_prop_focus_skip_set(Evas_Object *obj, Eina_Bool skip)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   win->skip_focus = skip;
   ecore_evas_focus_skip_set(win->ee, skip);
}

EAPI void
elm_win_illume_command_send(Evas_Object *obj, Elm_Illume_Command command, void *params __UNUSED__)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
   if (win->xwin)
     {
        switch (command)
          {
           case ELM_ILLUME_COMMAND_FOCUS_BACK:
              ecore_x_e_illume_focus_back_send(win->xwin);
              break;
           case ELM_ILLUME_COMMAND_FOCUS_FORWARD:
              ecore_x_e_illume_focus_forward_send(win->xwin);
              break;
           case ELM_ILLUME_COMMAND_FOCUS_HOME:
              ecore_x_e_illume_focus_home_send(win->xwin);
              break;
           case ELM_ILLUME_COMMAND_CLOSE:
              ecore_x_e_illume_close_send(win->xwin);
              break;
           default:
              break;
          }
     }
#else
   (void) command;
#endif
}

EAPI Evas_Object *
elm_win_inlined_image_object_get(Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   return win->img_obj;
}

EAPI void
elm_win_focus_highlight_enabled_set(Evas_Object *obj, Eina_Bool enabled)
{
   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype);

   win = elm_widget_data_get(obj);
   enabled = !!enabled;
   if (win->focus_highlight.enabled == enabled)
     return;

   win->focus_highlight.enabled = enabled;

   if (win->focus_highlight.enabled)
     _elm_win_focus_highlight_init(win);
   else
     _elm_win_focus_highlight_shutdown(win);
}

EAPI Eina_Bool
elm_win_focus_highlight_enabled_get(const Evas_Object *obj)
{
   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;

   win = elm_widget_data_get(obj);
   return win->focus_highlight.enabled;
}

EAPI void
elm_win_focus_highlight_style_set(Evas_Object *obj, const char *style)
{
   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype);

   win = elm_widget_data_get(obj);
   eina_stringshare_replace(&win->focus_highlight.style, style);
   win->focus_highlight.changed_theme = EINA_TRUE;
   _elm_win_focus_highlight_reconfigure_job_start(win);
}

EAPI const char *
elm_win_focus_highlight_style_get(const Evas_Object *obj)
{
   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype) NULL;

   win = elm_widget_data_get(obj);
   return win->focus_highlight.style;
}

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *frm;
   Evas_Object *content;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);

static const char *widtype2 = NULL;

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   _elm_theme_object_set(obj, wd->frm, "win", "inwin", elm_widget_style_get(obj));
   if (wd->content)
     edje_object_part_swallow(wd->frm, "elm.swallow.content", wd->content);
   _sizing_eval(obj);

   evas_object_smart_callback_call(obj, SIG_THEME_CHANGED, NULL);
}

static Eina_Bool
_elm_inwin_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd)
     return EINA_FALSE;

   /* Try Focus cycle in subitem */
   if (wd->content)
     {
        elm_widget_focus_next_get(wd->content, dir, next);
        if (*next)
          return EINA_TRUE;
     }

   *next = (Evas_Object *)obj;
   return EINA_FALSE;
}

static void
_elm_inwin_text_set_hook(Evas_Object *obj, const char *item, const char *text)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !item) return;
   edje_object_part_text_escaped_set(wd->frm, item, text);
   _sizing_eval(obj);
}

static const char *
_elm_inwin_text_get_hook(const Evas_Object *obj, const char *item)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!item || !wd || !wd->frm) return NULL;
   return edje_object_part_text_get(wd->frm, item);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;

   evas_object_size_hint_min_get(wd->content, &minw, &minh);
   edje_object_size_min_calc(wd->frm, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *sub = event_info;
   if (sub == wd->content)
     {
        evas_object_event_callback_del_full
           (sub, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
        wd->content = NULL;
        _sizing_eval(obj);
     }
}

EAPI Evas_Object *
elm_win_inwin_add(Evas_Object *obj)
{
   Evas_Object *obj2;
   Widget_Data *wd;
   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   win = elm_widget_data_get(obj);
   if (!win) return NULL;
   wd = ELM_NEW(Widget_Data);
   obj2 = elm_widget_add(win->evas);
   elm_widget_type_set(obj2, "inwin");
   ELM_SET_WIDTYPE(widtype2, "inwin");
   elm_widget_sub_object_add(obj, obj2);
   evas_object_size_hint_weight_set(obj2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(obj, obj2);

   elm_widget_data_set(obj2, wd);
   elm_widget_del_hook_set(obj2, _del_hook);
   elm_widget_theme_hook_set(obj2, _theme_hook);
   elm_widget_focus_next_hook_set(obj2, _elm_inwin_focus_next_hook);
   elm_widget_text_set_hook_set(obj2, _elm_inwin_text_set_hook);
   elm_widget_text_get_hook_set(obj2, _elm_inwin_text_get_hook);
   elm_widget_can_focus_set(obj2, EINA_TRUE);
   elm_widget_highlight_ignore_set(obj2, EINA_TRUE);

   wd->frm = edje_object_add(win->evas);
   _elm_theme_object_set(obj, wd->frm, "win", "inwin", "default");
   elm_widget_resize_object_set(obj2, wd->frm);

   evas_object_smart_callback_add(obj2, "sub-object-del", _sub_del, obj2);

   _sizing_eval(obj2);
   return obj2;
}

EAPI void
elm_win_inwin_activate(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype2);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_raise(obj);
   evas_object_show(obj);
   edje_object_signal_emit(wd->frm, "elm,action,show", "elm");
   elm_object_focus_set(obj, EINA_TRUE);
}

EAPI void
elm_win_inwin_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype2);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content == content) return;
   if (wd->content) evas_object_del(wd->content);
   wd->content = content;
   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add(content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
        edje_object_part_swallow(wd->frm, "elm.swallow.content", content);
     }
   _sizing_eval(obj);
}

EAPI Evas_Object *
elm_win_inwin_content_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype2) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->content;
}

EAPI Evas_Object *
elm_win_inwin_content_unset(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype2) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   if (!wd->content) return NULL;
   Evas_Object *content = wd->content;
   elm_widget_sub_object_del(obj, wd->content);
   evas_object_event_callback_del_full(wd->content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints, obj);
   edje_object_part_unswallow(wd->frm, wd->content);
   wd->content = NULL;
   return content;
}

EAPI Eina_Bool
elm_win_socket_listen(Evas_Object *obj, const char *svcname, int svcnum, Eina_Bool svcsys)
{

   Elm_Win *win;

   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;
   if (!win->ee) return EINA_FALSE;

   if (!ecore_evas_extn_socket_listen(win->ee, svcname, svcnum, svcsys))
     return EINA_FALSE;

   return EINA_TRUE;
}

/* windowing specific calls - shall we do this differently? */

static Ecore_X_Window
_elm_ee_win_get(const Evas_Object *obj)
{
   if (!obj) return 0;
#ifdef HAVE_ELEMENTARY_X
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   if (ee) return (Ecore_X_Window)ecore_evas_window_get(ee);
#endif
   return 0;
}

EAPI Ecore_X_Window
elm_win_xwindow_get(const Evas_Object *obj)
{
   Elm_Win *win;
   const char *type;

   if (!obj) return 0;
   type = elm_widget_type_get(obj);
   if ((!type) || (type != widtype)) return _elm_ee_win_get(obj);
   win = elm_widget_data_get(obj);
   if (!win) return 0;
#ifdef HAVE_ELEMENTARY_X
   if (win->xwin) return win->xwin;
   if (win->parent) return elm_win_xwindow_get(win->parent);
#endif
   return 0;
}

EAPI void
elm_win_floating_mode_set(Evas_Object *obj, Eina_Bool floating)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype);
   win = elm_widget_data_get(obj);
   if (!win) return;
   if (floating == win->floating) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(win);
#endif
   win->floating = floating;
#ifdef HAVE_ELEMENTARY_X
   if (win->xwin)
     {
        if (win->floating)
          ecore_x_e_illume_window_state_set
             (win->xwin, ECORE_X_ILLUME_WINDOW_STATE_FLOATING);
        else
          ecore_x_e_illume_window_state_set
             (win->xwin, ECORE_X_ILLUME_WINDOW_STATE_NORMAL);
     }
#endif
}

EAPI Eina_Bool
elm_win_floating_mode_get(const Evas_Object *obj)
{
   Elm_Win *win;
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   win = elm_widget_data_get(obj);
   if (!win) return EINA_FALSE;

   return win->floating;
}

