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
 * dayselector, changed - This is called whenever the user changes the state of one of the check object. 
 * 
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *parent;
	Evas_Object *frame;
	Evas_Object *base;
	Evas_Object *title;
	Evas_Object *check[7];
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object* obj);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   if (!wd) return;
   free(wd);
}

static void 
_check_clicked(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(data);
	int idx;
	for(idx = 0; idx< 7; ++idx) {
		if(obj==wd->check[idx]) {
			evas_object_smart_callback_call(data, "dayselector,changed", (void *)idx);
			return ;
		}
	}
}


static void
_theme_hook(Evas_Object *obj)
{
	int idx;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return;

  _elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));
  _elm_theme_object_set(obj, wd->title, "label", "dayselector", "default");

  for(idx=0; idx<7; ++idx) 
		elm_object_style_set(wd->check[idx], "dayselector");
}


/**
 * Set the title. 
 *
 * @param obj 	 	Dayselector
 * @param title 	title 
 *
 * @ingroup Dayselector
 */
EAPI void
elm_dayselector_title_set(Evas_Object* obj, const char* title)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
   edje_object_part_text_set(wd->title, "elm.text", title);
}

/**
 * Get the state of given check object.
 *
 * @param obj  	 	Dayselector
 * @param day        day user want to know. 
 *
 * @ingroup Dayselector
 */
EAPI Eina_Bool 
elm_dayselector_check_state_get(Evas_Object *obj, Elm_DaySelector_Day day)
{
	ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return EINA_FALSE;

	return elm_check_state_get(wd->check[day]);
}


/**
 * Add the dayselector.
 *
 * @param item 	 	Parent object.
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

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "dayselector");
   elm_widget_type_set(obj, "dayselector");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;

	//Frame
	wd->frame = elm_frame_add(obj);
	elm_widget_resize_object_set(obj, wd->frame);

	//Base
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));
	elm_widget_sub_object_add(obj, wd->base);
	elm_frame_content_set(wd->frame, wd->base);

	//Title
	wd->title = edje_object_add(e);
	elm_widget_sub_object_add(obj, wd->title);
	_elm_theme_object_set(obj, wd->title, "label", "dayselector", "default");
	edje_object_part_swallow(wd->base, "title", wd->title); 

	//Buttons
 	for(idx=0; idx<7; ++idx)
	{
		wd->check[idx]=elm_check_add(wd->base);
		elm_object_style_set(wd->check[idx], "dayselector");
		elm_widget_sub_object_add(obj, wd->check[idx]);
		evas_object_smart_callback_add(wd->check[idx], "changed", _check_clicked, obj);
	}

	elm_check_label_set(wd->check[ELM_DAYSELECTOR_SUN], "S");
	edje_object_part_swallow(wd->base, "sun", wd->check[0]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_MON], "M");
	edje_object_part_swallow(wd->base, "mon", wd->check[1]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_TUE], "T");
	edje_object_part_swallow(wd->base, "tue", wd->check[2]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_WED], "W");
	edje_object_part_swallow(wd->base, "wed", wd->check[3]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_THU], "T");
	edje_object_part_swallow(wd->base, "thu", wd->check[4]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_FRI], "F");
	edje_object_part_swallow(wd->base, "fri", wd->check[5]);
	elm_check_label_set(wd->check[ELM_DAYSELECTOR_SAT], "S");
	edje_object_part_swallow(wd->base, "sat", wd->check[6]);

   return obj;
}


