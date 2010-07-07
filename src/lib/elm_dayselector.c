#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Dayselector Dayselector
 * @ingroup Elementary
 *
 * Dayselector. 
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
static void _dayselector_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _dayselector_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _dayselector_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _del_hook(Evas_Object *obj);
static void _del_pre_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);


static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_SHOW, _dayselector_show, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_HIDE, _dayselector_hide, obj);
	evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_MOVE, _dayselector_move, obj);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = (Widget_Data *) elm_widget_data_get(obj);
   if (!wd) return;
	evas_object_del(wd->frame);
   free(wd);
}

static void 
_dayselector_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;
	evas_object_show(wd->frame);
}

static void 
_dayselector_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;
	
	evas_object_hide(wd->frame);
}

static void
_dayselector_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y;
	Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);
	evas_object_move(wd->frame, x, y);
}

static void
_dayselector_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord w, h;
	Widget_Data *wd = (Widget_Data *) data;
   if (!wd) return;
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);
	evas_object_resize(wd->frame, w, h);
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

}

EAPI void
elm_dayselector_title_set(Evas_Object* obj, const char* title)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);
   edje_object_part_text_set(wd->title, "elm.text", title);

}

EAPI Eina_Bool 
elm_dayselector_check_state_get(Evas_Object *obj, Elm_DaySelector_Day day)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data* wd = (Widget_Data*) elm_widget_data_get(obj);

	if(!wd) return;

	return elm_check_state_get(wd->check[day]);
}

EAPI Evas_Object *
elm_dayselector_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
	int idx;
	Evas_Coord w, h;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "dayselector");
   elm_widget_type_set(obj, "dayselector");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;

	//Base
	wd->frame = elm_frame_add(obj);
   elm_widget_sub_object_add(obj, wd->frame);
	elm_widget_resize_object_set(obj, wd->frame);

	//Content
	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "dayselector", "base", elm_widget_style_get(obj));
	elm_frame_content_set(wd->frame, wd->base);

	//Title
	wd->title = edje_object_add(e);
	_elm_theme_object_set(obj, wd->title, "label", "dayselector", "default");
	edje_object_part_swallow(wd->base, "title", wd->title); 

	//Buttons
 	for(idx=0; idx<7; ++idx) 
	{
		wd->check[idx]=elm_check_add(wd->base);
		elm_object_style_set(wd->check[idx], "dayselector");
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

   evas_object_event_callback_add(parent, EVAS_CALLBACK_SHOW, _dayselector_show, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_HIDE, _dayselector_hide, obj);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_MOVE, _dayselector_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _dayselector_show, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _dayselector_hide, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _dayselector_move, wd);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _dayselector_resize, wd);

	edje_object_size_min_get(wd->base, &w, &h);
	evas_object_resize(obj, w, h);

   return obj;
}


