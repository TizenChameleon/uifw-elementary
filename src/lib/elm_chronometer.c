#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Chronometer Chronometer
 * @ingroup Elementary
 *
 * It's a widget to show chronometer. 
 *
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   int sec1, sec2;
   Evas_Object *chronometer;
   Ecore_Timer *ticker;
   struct
     {
        int hrs, min, sec;
	Eina_Bool format: 1;
	Eina_Bool run: 1;
     } cur;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static Eina_Bool _ticker(void *data);
static void _signal_chronometer_val_changed(void *data,Evas_Object *obj,const char *emission,const char *source);
static void _time_update(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->ticker) ecore_timer_del(wd->ticker);

   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   _time_update(obj);
}

static Eina_Bool
_ticker(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   double t;
   struct timeval timev;
   struct tm *tm;
   time_t tt;

   gettimeofday(&timev, NULL);
   t = ((double)(1000000 - timev.tv_usec)) / 1000000.0;
   wd->ticker = ecore_timer_add(t, _ticker, data);

   tt=(time_t)(timev.tv_sec);
   tzset();
   tm=localtime(&tt);
   if(tm)
   {
       wd->sec1=tm->tm_sec;
       _time_update(data);
   }
  return 0;
}

/*
 * FIXME:
 */
static void 
_signal_chronometer_val_changed(void *data,Evas_Object *obj,const char *emission,const char *source)
{
	Widget_Data *wd=elm_widget_data_get(data);
	
	evas_object_smart_callback_call(data,"changed",NULL);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
	Evas_Coord w, h;

	if (!wd) return;

	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->chronometer, &minw, &minh, minw, minh);   
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);

	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minw) minh = h;

	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_time_update(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	const char *style = elm_widget_style_get(obj);
	char buf1[9], buf2[7]; 

        if(wd->sec1==wd->sec2)
	  return 0;        

	wd->sec2 = wd->sec1;		

	_elm_theme_object_set(obj,wd->chronometer, "chronometer", "base", style); 

	if(wd->cur.run)
		wd->cur.sec++;

        if(wd->cur.sec==60){
		wd->cur.min++;
		if(wd->cur.min==60){
			wd->cur.hrs++;
			if(wd->cur.hrs==100)
				wd->cur.hrs=0;
			wd->cur.min=0;
		}
		wd->cur.sec=0;
	}

	if (wd->cur.format) {
		snprintf(buf1,sizeof(buf1),"%02d:%02d:%02d",wd->cur.hrs,wd->cur.min,wd->cur.sec);
	        edje_object_part_text_set(wd->chronometer,"digit",buf1);
	}
	else {
		snprintf(buf2,sizeof(buf2),"%02d:%02d",wd->cur.min,wd->cur.sec);
	        edje_object_part_text_set(wd->chronometer,"digit",buf2);
	}

}

/**
 * Add a new chronometer to the parent
 *
 * @param parent The parent object
 *
 * This function inserts a chronometer widget on a given canvas.
 *
 * @ingroup Chronometer
 */
EAPI Evas_Object *
elm_chronometer_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   elm_widget_type_set(obj, "chronometer");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->chronometer = edje_object_add(e);
   elm_widget_resize_object_set(obj, wd->chronometer);

   wd->cur.format = EINA_TRUE;
   wd->cur.run = EINA_FALSE;

   _sizing_eval(obj);

   _time_update(obj);
   _ticker(obj);

   return obj;
}

/**
 * Run the chronometer
 *
 * @param obj The chronometer object
 *
 * This function start the chronometer time 
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_start(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->cur.run = EINA_TRUE;
   _time_update(obj);
}

/**
 * Run the chronometer
 *
 * @param obj The chronometer object
 *
 * This function stop the chronometer time 
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_stop(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->cur.run = EINA_FALSE;
   _time_update(obj);
}

/**
 * Set the chronometer time
 *
 * @param obj The chronometer object
 * @param hrs The hours to set
 * @param min The minutes to set
 * @param sec The secondes to set
 *
 * This function updates the time that is showed by the chronometer widget
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_time_set(Evas_Object *obj, int hrs, int min, int sec)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->cur.hrs = hrs;
   wd->cur.min = min;
   wd->cur.sec = sec;
   _time_update(obj);
}

/**
 * Get chronometer time
 *
 * @param obj The chronometer object
 * @param hrs Pointer to the variable to get the hour of this chronometer
 * object
 * @param min Pointer to the variable to get the minute of this chronometer
 * object
 * @param sec Pointer to the variable to get the second of this chronometer
 * object
 *
 * This function gets the time set of the chronometer widget and returns it
 * on the variables passed as the arguments to function
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_time_get(const Evas_Object *obj, int *hrs, int *min, int *sec)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (hrs) *hrs = wd->cur.hrs;
   if (min) *min = wd->cur.min;
   if (sec) *sec = wd->cur.sec;
}

/**
 * Set the chronometer format 
 *
 * @param obj The chronometer object
 * @param format Bool option for the show hours
 * (1 = show hours, 0 = not show hours)
 *
 * This function sets the chronometer format.
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_format_set(Evas_Object *obj, Eina_Bool format)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   wd->cur.format = format;
   _time_update(obj);
}

/**
 * Get the chronometer format
 *
 * @param obj The chronometer object
 * @param format Bool option for the show hours
 * (1 = show hours, 0 = not show hours)
 *
 * This function gets the chronometer format
 *
 * @ingroup Chronometer
 */
EAPI void
elm_chronometer_format_get(Evas_Object *obj, Eina_Bool *format)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   *format = wd->cur.format;
}
