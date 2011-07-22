#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Dayselector Dayselector
 * @ingroup Elementary
 *
 * DaySelector.
 *
 * Signals that you can add callbacks for are:
 *
 * dayselector,changed - This is called whenever the user changes the state of one of the check object.
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *parent;
   Evas_Object *base;
   Evas_Object *check[7];
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
//TODO:static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
//static void _sizing_eval(Evas_Object* obj);
//static void _dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _changed_size_hints(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}
/*
static void
_dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _sizing_eval(obj);
}
*/
static void
_changed_size_hints(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord w, h;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   edje_object_size_min_get(wd->base, &w, &h);
   evas_object_size_hint_min_set(obj, w * elm_scale_get(), h * elm_scale_get());
}

static void
_check_clicked(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   static Elm_DaySelector_Day day;
   Widget_Data* wd = (Widget_Data*) elm_widget_data_get(data);
   int idx;
   for (idx = 0; idx< 7; ++idx)
     {
        if (obj==wd->check[idx])
          {
             day = idx;
             evas_object_smart_callback_call(data, "dayselector,changed", (void *) &day);
             return ;
        }
     }
}

static void
_theme_hook(Evas_Object *obj)
{
   char *day_name[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat" };
   char buf[256];
   int idx;
   Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
   if (!wd) return;

   _elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));

   for ( idx = 0; idx < 7; ++idx )
     {
        sprintf(buf, "dayselector/%s_%s", elm_widget_style_get(obj), day_name[idx]);
        elm_object_style_set(wd->check[idx], buf );
     }
}

/**
 * Set the state of given check object.
 *
 * @param[in] obj  	     Dayselector
 * @param[in] day        day user want to know.
 * @param[in] checked    state of the day. Eina_True is checked.
 *
 * @ingroup Dayselector
 */
EAPI void
elm_dayselector_check_state_set(Evas_Object *obj, Elm_DaySelector_Day day, Eina_Bool checked)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
   if (!wd) return;
   elm_check_state_set(wd->check[day], checked);
}

/**
 * Get the state of given check object.
 *
 * @param[in] obj  	 	Dayselector
 * @param[in] day        day user want to know.
 *
 * @ingroup Dayselector
 */
EAPI Eina_Bool
elm_dayselector_check_state_get(Evas_Object *obj, Elm_DaySelector_Day day)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return elm_check_state_get(wd->check[day]);
}
/*
static void
_sizing_eval(Evas_Object* obj)
{
   Evas_Coord w, h;
   Evas_Coord min_w, min_h, max_h;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   edje_object_size_min_get(wd->base, &min_w, &min_h);
   edje_object_size_max_get(wd->base, NULL, &max_h);

   fprintf( stderr, "%d %d\n", w, h );
   if (w < min_w) w = min_w;
   if (h < min_h) h = min_h;
   else if (h > max_h) h = max_h;

   evas_object_resize(wd->base, w, h);
}
*/
/**
 * Add the dayselector.
 *
 * @param[in] parent 	 	Parent object.
 *
 * @ingroup Dayselector
 */
EAPI Evas_Object *
elm_dayselector_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   int idx;
   char *label[] = { "S", "M", "T", "W", "T", "F", "S" };
   char *day_name[] = { "sun", "mon", "tue", "wed", "thu", "fri", "sat" };
   char *style_name[] = { "dayselector/sun_first_sun",
                          "dayselector/sun_first_mon",
                          "dayselector/sun_first_tue",
                          "dayselector/sun_first_wed",
                          "dayselector/sun_first_thu",
                          "dayselector/sun_first_fri",
                          "dayselector/sun_first_sat",
   };
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   wd = ELM_NEW(Widget_Data);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "dayselector");
   elm_widget_type_set(obj, "dayselector");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   wd->parent = parent;
   //Base
   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));
   elm_object_style_set(wd->base, "dayselector");
   elm_widget_resize_object_set(obj, wd->base);
   //Checks
   for (idx=0; idx<7; ++idx)
     {
        wd->check[idx]=elm_check_add(wd->base);
        elm_widget_sub_object_add(obj, wd->check[idx]);
        evas_object_smart_callback_add(wd->check[idx], "changed", _check_clicked, obj);
        elm_check_label_set(wd->check[idx], label[idx] );
        edje_object_part_swallow(wd->base, day_name[idx], wd->check[idx]);
        elm_object_style_set(wd->check[idx], style_name[idx]);
     }
   //evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _dayselector_resize, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
   //_sizing_eval(obj);
   return obj;
}
