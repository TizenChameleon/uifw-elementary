/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Selectioninfo selectioninfo
 * @ingroup Elementary
 *
 * Provide for notifying user about the number of currently selected items
 * especially when user is on the selection mode for specific action
 * such as Move, Delete, or Share, etc.
 *
 */

#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
	Evas_Object* selectioninfo;
	Evas_Object* content;
	Evas_Object* parent;
	Eina_Bool* check_state;
	int check_count;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _changed_size_hints(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);
static void _calc(Evas_Object *obj);
static void _content_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _hide(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_del_pre_hook(Evas_Object *obj)
{
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, _resize, obj);
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE, _resize, obj);
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_SHOW, _show, obj);
	evas_object_event_callback_del_full(obj, EVAS_CALLBACK_HIDE, _hide, obj);
}

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	elm_selectioninfo_content_set(obj, NULL);
	elm_selectioninfo_parent_set(obj, NULL);
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_elm_theme_object_set(obj, wd->selectioninfo, "selectioninfo", "base", "default");
	edje_object_scale_set(wd->selectioninfo, elm_widget_scale_get(obj) *_elm_config->scale);
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord x,y,w,h;
	if (!wd) return;
	if (!wd->parent) return;
	evas_object_geometry_get(wd->parent, &x, &y, &w, &h);
	evas_object_move(obj, x, y);
	evas_object_resize(obj, w, h);
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
	if (!wd) return;
	if (event_info == wd->content) wd->content = NULL;
}

static void
_resize(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	_calc(obj);
}

static void
_content_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
	_calc(data);
}

static void
_calc(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	Evas_Coord x, y, w, h;
	if (!wd) return;
	evas_object_geometry_get(obj, &x, &y, &w, &h);
	edje_object_size_min_calc(wd->selectioninfo, &minw, &minh);

	if (wd->content)
	{
		evas_object_move(wd->selectioninfo, x, y + h - minh);
		evas_object_resize(wd->selectioninfo, w, minh);
	}
   
	_sizing_eval(obj);
}

static void
_show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	evas_object_show(wd->selectioninfo);
}

static void
_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	evas_object_hide(wd->selectioninfo);
}

static void
_parent_del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->parent = NULL;
	evas_object_hide(obj);
}

static void
_parent_hide(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->parent = NULL;
	evas_object_hide(obj);
}

/**
 * Add a new selectioninfo to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Selectioninfo
 */
EAPI Evas_Object *
elm_selectioninfo_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	ELM_SET_WIDTYPE(widtype, "selectioninfo");
	elm_widget_type_set(obj, "selectioninfo");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_pre_hook_set(obj, _del_pre_hook);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->selectioninfo = edje_object_add(e);
	_elm_theme_object_set(obj, wd->selectioninfo, "selectioninfo", "base", "default");
	_resize(obj, NULL, obj, NULL);
	
	elm_selectioninfo_parent_set(obj, parent);

	evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _hide, obj);

	_sizing_eval(obj);
	return obj;
}

/**
 * Set the selectioninfo content
 *
 * @param obj The selctioninfo object
 * @param content The content will be filled in this selectioninfo object
 *
 * @ingroup Notify
 */
EAPI void
elm_selectioninfo_content_set(Evas_Object *obj, Evas_Object *content)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->content)
	{
		evas_object_event_callback_del_full(wd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
		evas_object_event_callback_del_full(wd->content, EVAS_CALLBACK_RESIZE, _content_resize, obj);
		evas_object_del(wd->content);
		wd->content = NULL;
	}
   
	if (content)
	{
		elm_widget_sub_object_add(obj, content);
		wd->content = content;
		evas_object_event_callback_add(content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
		evas_object_event_callback_add(content, EVAS_CALLBACK_RESIZE, _content_resize, obj);
		edje_object_part_swallow(wd->selectioninfo, "elm.swallow.content", content);
        
		_sizing_eval(obj);
	}
	
	_calc(obj);
}

/**
 * Set the selectioninfo parent
 *
 * @param obj The selectioninfo object
 * @param parent The new parent
 *
 * @ingroup Selectioninfo
 */
EAPI void
elm_selectioninfo_parent_set(Evas_Object *obj, Evas_Object *parent)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->parent)
	{
		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_RESIZE, _changed_size_hints, obj);
		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_MOVE, _changed_size_hints, obj);
		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_DEL, _parent_del, obj);
		evas_object_event_callback_del_full(wd->parent, EVAS_CALLBACK_HIDE, _parent_hide, obj);
		wd->parent = NULL;
	}
   
	if (parent)
	{
		wd->parent = parent;
		evas_object_event_callback_add(parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints, obj);
		evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _changed_size_hints, obj);
		evas_object_event_callback_add(parent, EVAS_CALLBACK_MOVE, _changed_size_hints, obj);
		evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL, _parent_del, obj);
		evas_object_event_callback_add(parent, EVAS_CALLBACK_HIDE, _parent_hide, obj);
		_sizing_eval(obj);
	}
	
	_calc(obj);
}

/**
 * Set the check state to the selectioninfo
 *
 * @param obj The selectioninfo object
 * @param state The check state
 * @param count The check count
 *
 * @ingroup Selectioninfo
 */
EAPI void
elm_selectioninfo_check_state_set(Evas_Object *obj, Eina_Bool *state, int count)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->check_state = state;
	wd->check_count = count;
}

/**
 * Get the checked count
 *
 * @param obj The selectioninfo object
 *
 * @ingroup Selectioninfo
 */

EAPI int
elm_selectioninfo_checked_count_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return -1;

	int i;
	int count = 0;
	for (i=0; i<wd->check_count; i++)
	{
		if (wd->check_state[i])
		count++;
	}

	return count;
}

/**
 * Set the text to the selectioninfo
 *
 * @param obj The selectioninfo object
 * @param text The text
 *
 * @ingroup Selectioninfo
 */

EAPI void
elm_selectioninfo_text_set(Evas_Object *obj, char* text)
{
	ELM_CHECK_WIDTYPE(obj, widtype);
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	edje_object_part_text_set(_EDJ(wd->content), "elm.text", strdup(text));
}

