#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup NavigationBar NavigationBar
 * @ingroup [Elementary]
 *
 * The Navigationbar is an object that manages the presentation of hierarchical data in your application.
 * It covers whole content region with a bar, typically displayed at the top of the screen, 
 * containing buttons for navigating up and down a hierarchy.
 */

typedef struct _Widget_Data Widget_Data;
typedef struct _Item Item;
typedef struct _Transit_Cb_Data Transit_Cb_Data;

struct _Widget_Data
{
	Eina_List *stack;
	Evas_Object *base;
	Evas_Object *pager;
 };

struct _Item
{
	Evas_Object *obj;
 	Evas_Object *title;
	Evas_Object *title_obj;
	Eina_List *title_list;
	Evas_Object *left_btn;
	Evas_Object *right_btn;
	Evas_Object *back_btn;
	Evas_Object *content;
	int left_btn_w;
	int right_btn_w;
	int title_w;
	Eina_Bool ani;
};

struct _Transit_Cb_Data
{
	Item* prev_it;
	Item* it;
	Eina_Bool pop;
};

#define MINIMUM_WIDTH	70
#define MAXIMUM_WIDTH	200
#define EFFECT_WIDTH	160

#define CHAR_WIDTH		14
#define BACK_BTN_PADDING 10
#define ICON_PADDING	10
#define FONT_SIZE		28

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _item_sizing_eval(Item *it);
static void _delete_item(Item *it);

static void _back_button_clicked(void *data, Evas_Object *obj, void *event_info);
static int _get_button_width(Evas_Object *obj, Eina_Bool back_btn);
static Eina_Bool _button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn);
static const char *_title_string_get(Evas_Object *title);
static void _label_set(Evas_Object* label, const char* title);
static Evas_Object *_multiple_object_set(Evas_Object *obj, Evas_Object *sub_obj, Eina_List *list, int width);
static Item *_check_item_is_added(Evas_Object *obj, Evas_Object *content);

static void _transition_complete_cb(void *data);
static Elm_Transit *_transition_set(Item* prev_it, Item *it, Evas_Coord y, Eina_Bool pop);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
		_delete_item(it);

	eina_list_free(wd->stack);
	evas_object_del(wd->pager);
	evas_object_del(wd->base);

	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	_elm_theme_object_set(obj, wd->base, "navigationbar", "base", "default");

	edje_object_scale_set(wd->base, elm_widget_scale_get(obj) * _elm_config->scale);

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		//elm_object_style_set(it->left_btn, "custom/darkblue");
		elm_object_style_set(it->left_btn, "navigationbar/normal");
		//elm_object_style_set(it->right_btn, "custom/darkblue");
		elm_object_style_set(it->right_btn, "navigationbar/normal");
		//elm_object_style_set(it->back_btn, "backkey");
		elm_object_style_set(it->back_btn, "navigationbar/back");
	}

	_sizing_eval(obj);
}

static void
_delete_item(Item *it)
{
	Eina_List *ll;
	Evas_Object *list_obj;
	
	evas_object_del(it->left_btn);
	evas_object_del(it->back_btn);
	evas_object_del(it->right_btn);
	evas_object_del(it->title_obj);
	evas_object_del(it->title);	

	EINA_LIST_FOREACH(it->title_list, ll, list_obj)
		evas_object_del(list_obj);
	eina_list_free(it->title_list);

	free(it);	
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	Evas_Coord w = -1, h = -1;
	Eina_List *ll;

	edje_object_size_min_calc(wd->base, &minw, &minh);
	evas_object_size_hint_min_get(obj, &w, &h);
	if (w > minw) minw = w;
	if (h > minw) minh = h;

	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);

	ll = eina_list_last(wd->stack);
	if (ll)
	{
		Item *it = ll->data;
		_item_sizing_eval(it);
	}
}

static void
_item_sizing_eval(Item *it)
{
	Widget_Data *wd = elm_widget_data_get(it->obj);
	Evas_Coord pad, height, minw, w;
	int pad_count = 2;

	if (!it) return;

	edje_object_size_min_calc(wd->base, &minw, NULL);
	evas_object_geometry_get(wd->base, NULL, NULL, &w, NULL);
	if (w < minw) w = minw;
	
	edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
	edje_object_part_geometry_get(wd->base, "elm.swallow.title", NULL, NULL, NULL, &height);

	if (it->left_btn) 
	{
		it->left_btn_w = _get_button_width(it->left_btn, FALSE);
		evas_object_resize(it->left_btn, it->left_btn_w , height);
		evas_object_size_hint_min_set(it->left_btn, it->left_btn_w , height);
		pad_count++;
	}
	else if (it->back_btn)
	{
		it->left_btn_w = _get_button_width(it->back_btn, TRUE);
		evas_object_resize(it->back_btn, it->left_btn_w, height);
		evas_object_size_hint_min_set(it->back_btn, it->left_btn_w, height);
		pad_count++;
	}
	
	if (it->right_btn)
	{
		it->right_btn_w = _get_button_width(it->right_btn, FALSE);
		evas_object_resize(it->right_btn, it->right_btn_w, height);
		evas_object_size_hint_min_set(it->right_btn, it->right_btn_w, height);
		pad_count++;
	}

	if (it->title_list)
	{	
		it->title_w = w - it->left_btn_w - it->right_btn_w - pad_count * pad;
		it->title_obj = _multiple_object_set(it->obj, it->title_obj, it->title_list, it->title_w);
		evas_object_resize(it->title_obj, it->title_w, height);
		evas_object_size_hint_min_set(it->title_obj, it->title_w, height);
	}
	else if (it->title) 
	{
		const char *str = NULL;

		it->title_w = w - it->left_btn_w - it->right_btn_w - pad_count * pad;
		evas_object_resize(it->title, it->title_w, height);
		evas_object_size_hint_min_set(it->title, it->title_w, height);
		
		str = _title_string_get(it->title);
		if (str) 
		{
			double align;
			const char align_str[7] = {0,};
			int diff = 0;

			diff = it->left_btn_w - it->right_btn_w;
			if (it->left_btn_w) diff += pad;
			if (it->right_btn_w) diff -= pad;

			align = 0.5 - diff / (double)w;
			if (align < 0.0) align = 0.0;
			sprintf(align_str, "%lf", align);
			elm_label_text_align_set(it->title, align_str);
		}
	}
}

static void
_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(obj);
}

static void 
_transition_complete_cb(void *data)
	{
	Transit_Cb_Data *cb = data;
	if (!cb) return;

	Widget_Data *wd;
	Item *prev_it = cb->prev_it;
	Item *it = cb->it;

	if (prev_it) wd = elm_widget_data_get(prev_it->obj);
	else if (it) wd = elm_widget_data_get(it->obj);

	if (cb->pop && prev_it)
	{
		Eina_List *ll;
		ll = eina_list_last(wd->stack);
		if (ll->data == prev_it)
		{
			_delete_item(prev_it);
			wd->stack = eina_list_remove_list(wd->stack, ll);
		}
	}
	else if (prev_it)
	{
	 	evas_object_hide(prev_it->left_btn);
		evas_object_hide(prev_it->back_btn);
		evas_object_hide(prev_it->title_obj);
		evas_object_hide(prev_it->title);
		evas_object_hide(prev_it->right_btn);
	}

	if (it)
	{
		if (it->left_btn) 
		{
			edje_object_signal_emit(wd->base, "elm,state,item,add,leftpad", "elm");
			edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->left_btn);
		}
		else if (it->back_btn) 
		{
			edje_object_signal_emit(wd->base, "elm,state,item,add,leftpad", "elm");
			edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->back_btn);
		}
		else
			edje_object_signal_emit(wd->base, "elm,state,item,reset,leftpad", "elm");
			
		if (it->right_btn) 
		{
			edje_object_signal_emit(wd->base, "elm,state,item,add,rightpad", "elm");
			edje_object_part_swallow(wd->base, "elm.swallow.btn2", it->right_btn);
		}
		else
			edje_object_signal_emit(wd->base, "elm,state,item,reset,rightpad", "elm");

		if (it->title_obj) edje_object_part_swallow(wd->base, "elm.swallow.title", it->title_obj);
		else if (it->title) edje_object_part_swallow(wd->base, "elm.swallow.title", it->title);
	}

	free(cb);

	evas_object_smart_callback_call(it->obj, "updated", it->content);
}

static Elm_Transit *
_transition_set(Item* prev_it, Item *it, Evas_Coord y, Eina_Bool pop)
{
	Widget_Data *wd = elm_widget_data_get(it->obj);
	Evas_Coord pad, w;
	Elm_Transit *transit;
	int num = 1;
	int pad_count;

	transit = elm_transit_add(it->obj);
	edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
	evas_object_geometry_get(wd->base, NULL, NULL, &w, NULL);

	if (pop) num = -1;

	// hide prev item
	if (prev_it)
	{
		pad_count = 1;
		if (prev_it->left_btn) 
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(prev_it->left_btn, pad, y, pad-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->left_btn, 255, 255, 255, 255, 0, 0, 0, 0));
			pad_count++;
		}
		else if (prev_it->back_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(prev_it->back_btn, pad, y, pad-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->back_btn, 255, 255, 255, 255, 0, 0, 0, 0));
			pad_count++;
		}
		if (prev_it->title_obj)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(prev_it->title_obj, prev_it->left_btn_w+pad_count*pad, y, prev_it->left_btn_w+pad_count*pad-num*EFFECT_WIDTH, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->title_obj, 255, 255, 255, 255, 0, 0, 0, 0));
		}
		else if (prev_it->title)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(prev_it->title, prev_it->left_btn_w+pad_count*pad, y, prev_it->left_btn_w+pad_count*pad-num*EFFECT_WIDTH, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->title, 255, 255, 255, 255, 0, 0, 0, 0));
		}
		if (prev_it->right_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(prev_it->right_btn, w-pad-prev_it->right_btn_w, y, w-pad-prev_it->right_btn_w-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->right_btn, 255, 255, 255, 255, 0, 0, 0, 0));
		}
	}

	// show new item
	if (it)
	{
		pad_count = 1;
		if (it->left_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(it->left_btn, pad+num*EFFECT_WIDTH/4, y, pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->left_btn, 0, 0, 0, 0, 255, 255, 255, 255));
			pad_count++;
		}
		else if (it->back_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(it->back_btn, pad+num*EFFECT_WIDTH/4, y, pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->back_btn, 0, 0, 0, 0, 255, 255, 255, 255));
			pad_count++;
		}
		if (it->title_obj)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(it->title_obj, it->left_btn_w+pad_count*pad+num*EFFECT_WIDTH, y, it->left_btn_w+pad_count*pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->title_obj, 0, 0, 0, 0, 255, 255, 255, 255));
		}
		else if (it->title)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(it->title, it->left_btn_w+pad_count*pad+num*EFFECT_WIDTH, y, it->left_btn_w+pad_count*pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->title, 0, 0, 0, 0, 255, 255, 255, 255));
		}
		if (it->right_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_translation_add(it->right_btn, w-pad-it->right_btn_w+num*EFFECT_WIDTH/4, y, w-pad-it->right_btn_w, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->right_btn, 0, 0, 0, 0, 255, 255, 255, 255));
		}
	}

	return transit;
}

static void 
_back_button_clicked(void *data, Evas_Object *obj, void *event_info)
{
	Item *it = data;
	
	elm_navigationbar_pop(it->obj, it->ani);
}


static int
_get_button_width(Evas_Object *obj, Eina_Bool back_btn)
{
	int len = 0;
	char *label;
	int btn_len = 0;

	//if (back_btn) btn_len = BACK_BTN_PADDING;

	evas_object_geometry_get(elm_button_icon_get(obj), NULL, NULL, &len, NULL);
	if (len > 0) 
	{
		btn_len += len+2*ICON_PADDING;
		return btn_len;
	}

	label = elm_button_label_get(obj);
	if (label) 	len = strlen(label);
	if (len > 0) btn_len += len*CHAR_WIDTH;	

	if (btn_len < MINIMUM_WIDTH) return MINIMUM_WIDTH;
	else if (btn_len > MAXIMUM_WIDTH) return MAXIMUM_WIDTH;
	else return btn_len;

	return 0;
}

static Eina_Bool 
_button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn)
{
	Eina_Bool changed = FALSE;
	
	if ((prev_btn) && (prev_btn != new_btn)) 
	{
		elm_widget_sub_object_del(obj, prev_btn);
		evas_object_del(prev_btn);
	}

	if ((new_btn) && (prev_btn != new_btn)) 
	{
		if (back_btn) elm_object_style_set( new_btn, "navigationbar/back" );
		else elm_object_style_set(new_btn, "navigationbar/normal");
		elm_widget_sub_object_add(obj, new_btn);
		changed = TRUE;
	}

	return changed;
}

static const char *
_title_string_get(Evas_Object *title)
{
	if (!title) return NULL;
	
	const char *str = NULL;

	str = elm_label_label_get(title);
	while (str && (*str == '<')) 
	{
		str = strstr(str, ">");
		str += 1;
	}
	return str;
}

static void
_label_set(Evas_Object* label, const char* title)
{
	if (!label) return;
	
	elm_label_label_set(label, title);
	elm_label_fontsize_set(label, FONT_SIZE);
	elm_label_text_color_set(label, 255, 255, 255, 255);
}

static Item *
_check_item_is_added(Evas_Object *obj, Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
		{
			return it;
		}
 	}
	return NULL;
}

static Evas_Object * 
_multiple_object_set(Evas_Object *obj, Evas_Object *sub_obj, Eina_List *list, int width)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Evas_Object *new_obj, *list_obj;
	Evas_Coord pad, height;
	char buf[32];
	int num = 1;
	int count;

	edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
	edje_object_part_geometry_get(wd->base, "elm.swallow.title", NULL, NULL, NULL, &height);

	if (!sub_obj)
	{
		new_obj = elm_layout_add(obj);
		elm_widget_sub_object_add(obj, new_obj);
		elm_layout_theme_set(new_obj, "navigationbar", "title", "default");
	}
	else 
		new_obj = sub_obj;

	count = eina_list_count(list);
	EINA_LIST_FOREACH(list, ll, list_obj)
	{	
		evas_object_resize(list_obj, (width-(count-1)*pad)/count, height);
		evas_object_size_hint_min_set(list_obj, (width-(count-1)*pad)/count, height);		
	
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "elm,state,item,add,%d", num);
		edje_object_signal_emit(elm_layout_edje_get(new_obj), buf, "elm");

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "elm.swallow.title%d", num++);
		elm_layout_content_set(new_obj, buf, list_obj);
	}

	return new_obj;
}

/**
 * Add a new navigatgationbar to the parent
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "navigationbar");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "navigationbar", "base", "default");
	elm_widget_resize_object_set(obj, wd->base);

	wd->pager = elm_pager_add(obj);
	elm_widget_sub_object_add(obj, wd->pager);
	edje_object_part_swallow(wd->base, "elm.swallow.content", wd->pager);	

   	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, NULL);	

	_sizing_eval(obj);
	return obj;
}

/**
 * Push an object to the top of the NavigationBar stack (and show it)
 * The object pushed becomes a child of the navigationbar and will be controlled
 * it is deleted when the navigationbar is deleted or when the content is popped.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] title The title string
 * @param[in] left_btn The left button
 * @param[in] right_btn The right button
 * @param[in] content The object to push
 * @param[in] animation transition when this content becomes the top
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_push(Evas_Object *obj, 
						const char *title,
						Evas_Object *left_btn, 
						Evas_Object *right_btn, 
						Evas_Object *content,
						Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;
	Item *prev_it = NULL;

	if (!wd) return;

	it = _check_item_is_added(obj, content);
	if (!it) it = ELM_NEW(Item); 
	if (!it) return;
	
	// add and set new items		
	_button_set(obj, NULL, left_btn, EINA_FALSE);
	_button_set(obj, NULL, right_btn, EINA_FALSE);
	
	ll = eina_list_last(wd->stack);
	while (ll) 
	{
		prev_it = ll->data;
		if (prev_it->obj) break;  //find last pushed item
		ll = ll->prev;
		prev_it = NULL;
	}

	it->obj = obj;
	it->left_btn = left_btn;
	it->right_btn = right_btn;
	it->content = content;
	it->ani = animation;

	if (!left_btn && prev_it)
	{
		char *prev_title;
		char *buf = NULL;
		int len = 0;

		it->back_btn = elm_button_add(obj);
		prev_title = _title_string_get(prev_it->title);
		if (prev_title) len = strlen(prev_title);
		if (len > 0) buf = calloc(len+3, sizeof(char));
		if (buf) 
		{
			sprintf(buf, "< %s", prev_title);
			elm_button_label_set(it->back_btn, buf);
			free(buf);
		}
		evas_object_smart_callback_add(it->back_btn, "clicked", _back_button_clicked, it); 
		_button_set(obj, NULL, it->back_btn, EINA_TRUE);
	}

	it->title = elm_label_add(obj);
	elm_widget_sub_object_add(obj, it->title);
	if (title) _label_set(it->title, title);
	
	_item_sizing_eval(it);

	// unswallow items and start transition
	if (prev_it)
	{
		Evas_Coord y;
		Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);
				
		cb->prev_it = prev_it;
		cb->it = it;
		cb->pop = EINA_FALSE;
		if (prev_it->title_obj) evas_object_geometry_get(prev_it->title_obj, NULL, &y, NULL, NULL);
		else evas_object_geometry_get(prev_it->title, NULL, &y, NULL, NULL);

		if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
		else if (prev_it->title) edje_object_part_unswallow(wd->base, prev_it->title);
 		if (prev_it->left_btn) edje_object_part_unswallow(wd->base, prev_it->left_btn);
		else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
		if (prev_it->right_btn) edje_object_part_unswallow(wd->base, prev_it->right_btn);

		/*if (it->ani) 
		{
			Elm_Transit* transit;
			transit = _transition_set(prev_it, it, y, EINA_FALSE);
			elm_transit_completion_callback_set(transit, _transition_complete_cb, cb);
			elm_transit_run(transit, 0.3);
			elm_transit_del(transit); 
		}
		else*/ 
		{
			_transition_complete_cb(cb);
		}
	}
	else  
	{
		Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);

		cb->prev_it = NULL;
		cb->it = it;
		cb->pop = EINA_FALSE;
		_transition_complete_cb(cb);
	}

	//push content to pager
	elm_pager_animation_set(wd->pager, it->ani);
	elm_pager_content_push(wd->pager, it->content);	

	//push item into the stack. it should be always the tail
	if (!_check_item_is_added(obj, content))
	wd->stack = eina_list_append(wd->stack, it);	
	else
	{
		EINA_LIST_FOREACH(wd->stack, ll, it)
		{
			if (it->content == content) 
			{
				wd->stack = eina_list_demote_list(wd->stack, ll);
				break;
			}
	 	}
	}
}

/**
 * Pop the object that is on top of the NavigationBar stack (and update it)
 * This pops the object that is on top (visible) in the navigationbar, makes it disappear, then deletes the object. 
 * The object that was underneath it on the stack will become visible.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] animation transition when this content is popped
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_pop(Evas_Object *obj, 
						Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it = NULL;
	Item *prev_it = NULL;

	if (!wd->stack) return;

	//find item to be popped and to be shown
	ll = eina_list_last(wd->stack);
	if (ll)
	{
		prev_it = ll->data;
		while (ll = ll->prev) 
		{
			it = ll->data;
			if (it->obj) break;  
			it = NULL;
		}
	}

	if (prev_it && it) 
	{
		//unswallow items and start trasition
		Evas_Coord y;
		Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);

		cb->prev_it = prev_it;
		cb->it = it;
		cb->pop = EINA_TRUE;
		if (prev_it->title_obj) evas_object_geometry_get(prev_it->title_obj, NULL, &y, NULL, NULL);
		else evas_object_geometry_get(prev_it->title, NULL, &y, NULL, NULL);

		if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
		else if (prev_it->title) edje_object_part_unswallow(wd->base, prev_it->title);
		if (prev_it->left_btn) edje_object_part_unswallow(wd->base, prev_it->left_btn);
		else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
		if (prev_it->right_btn) edje_object_part_unswallow(wd->base, prev_it->right_btn);

		_item_sizing_eval(it);

		/*if (animation) 
		{
			Elm_Transit* transit;
			transit = _transition_set(prev_it, it, y, EINA_TRUE);
			elm_transit_completion_callback_set(transit, _transition_complete_cb, cb);
			elm_transit_run(transit, 0.3);
			elm_transit_del(transit); 
		}
		else*/
		{
			_transition_complete_cb(cb);
		}

		//pop content from pager
		elm_pager_animation_set(wd->pager, animation);
		elm_pager_content_pop(wd->pager);
	}
	else if (prev_it)
	{
		Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);

		cb->prev_it = prev_it;
		cb->it = NULL;
		cb->pop = EINA_TRUE;
		_transition_complete_cb(cb);

		//pop content from pager
		elm_pager_animation_set(wd->pager, EINA_FALSE);
		elm_pager_content_pop(wd->pager);
	}
}
	
/**
 * Pop to the inputted content object (and update it)
 * This pops the objects that is under the inputed content on the navigationbar stack, makes them disappear, then deletes the objects. 
 * The content will become visible.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content the object to show
 * @param[in] animation transition when this content is popped
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_to_content_pop(Evas_Object *obj,
										Evas_Object *content,
										Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it = NULL;
	Item *prev_it = NULL;

	if (!wd->stack) return;

	//find item to be popped and to be shown
	ll = eina_list_last(wd->stack);
	if (ll)
	{
		prev_it = ll->data;
		while (ll = ll->prev) 
		{
			it = ll->data;
			if (it->obj && (it->content == content)) 
			{
				//delete contents between the top and the inputted content
				ll = eina_list_last(wd->stack);
				while (ll = ll->prev)
				{
					if (ll->data == it) break;
					else 
					{
						_delete_item(ll->data);
						wd->stack = eina_list_remove_list(wd->stack, ll);
					}
				}
				break;
			}
			it = NULL;
		}
	}

	if (prev_it && it) 
	{
		//unswallow items and start trasition
		Evas_Coord y;
		Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);

		cb->prev_it = prev_it;
		cb->it = it;
		cb->pop = EINA_TRUE;
		if (prev_it->title_obj) evas_object_geometry_get(prev_it->title_obj, NULL, &y, NULL, NULL);
		else evas_object_geometry_get(prev_it->title, NULL, &y, NULL, NULL);

		if (prev_it->title_obj) edje_object_part_unswallow(wd->base, prev_it->title_obj);
		else if (prev_it->title) edje_object_part_unswallow(wd->base, prev_it->title);
		if (prev_it->left_btn) edje_object_part_unswallow(wd->base, prev_it->left_btn);
		else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
		if (prev_it->right_btn) edje_object_part_unswallow(wd->base, prev_it->right_btn);

		_item_sizing_eval(it);

		/*if (animation) 
		{
			Elm_Transit* transit;
			transit = _transition_set(prev_it, it, y, EINA_TRUE);
			elm_transit_completion_callback_set(transit, _transition_complete_cb, cb);
			elm_transit_run(transit, 0.3);
			elm_transit_del(transit); 
		}
		else*/
		{
			_transition_complete_cb(cb);
		}

		//pop content from pager
		elm_pager_animation_set(wd->pager, animation);
		elm_pager_to_content_pop(wd->pager, content);
	}
	}

/**
 * Set the title string for the pushed content
 * If the content is at the top of the navigation stack, update the title string
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] title The title string
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_set(Evas_Object *obj, 
							Evas_Object *content, 
							const char *title)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return;
	
	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
		{
			_label_set(it->title, title);
			_item_sizing_eval(it);
			break;
		}
	}
}

/**
 * Return the title string of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The title string or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI const char *
elm_navigationbar_title_get(Evas_Object *obj, 
							Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return NULL;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
			return _title_string_get(it->title);
	}
	return NULL;
}

/**
 * Add a title object for the content 
 * If it is added before pushing the content, it will be shown when content is being pushed.
 * If the content is at the top of the navigation stack, update the title or title object.
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] title_obj a title object (normally button or segment_control)
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_title_object_add(Evas_Object *obj,
									Evas_Object *content,
									Evas_Object *title_obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;
	Item *last_it;

	if (!title_obj) return;
	if (!wd) return;
	
	it = _check_item_is_added(obj, content);
	if (!it) it = ELM_NEW(Item);
	if (!it) return;

	it->content = content;
	it->title_list = eina_list_append(it->title_list, title_obj);
	if (it->obj) _item_sizing_eval(it);

	if (!_check_item_is_added(obj, content))	
		wd->stack = eina_list_append(wd->stack, it);		

	//update if the content is the top item
	ll = eina_list_last(wd->stack);
	if (ll) 
	{
		last_it = ll->data;
		if (last_it->content == content) 
		{
			Evas_Object *swallow;
			swallow = edje_object_part_swallow_get(wd->base, "elm.swallow.title");
			if (swallow) {
				edje_object_part_unswallow(wd->base, swallow);
				evas_object_hide(swallow);
			}
			edje_object_part_swallow(wd->base, "elm.swallow.title", it->title_obj);
		}
	}
}

/**
 * Return the title string of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The list of title objects
 *
 * @ingroup NavigationBar
 */
EAPI Eina_List *
elm_navigationbar_title_object_list_get(Evas_Object *obj,
										Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return NULL;

	EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content)	
			return it->title_list;
	}	
	return NULL;
}

/**
 * Set the back button object for the pushed content
 * If the content is at the top of the navigation stack, update the back button
 * If the left button is already set, back button is not created
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] button The back button
 * @param[in] animation transition when this button becomes the top item
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_back_button_set(Evas_Object *obj, 
									Evas_Object *content, 
									Evas_Object *button, 
									Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;
	Eina_Bool changed;

	if (!wd) return;
	
	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
		{
			changed = _button_set(obj, it->back_btn, button, EINA_TRUE);
			it->back_btn = button;
			_item_sizing_eval(it);
			break;
		}
	}

	//update if the content is the top item
	ll = eina_list_last(wd->stack);
	if (ll) 
	{
		it = ll->data;
		if (changed && (it->content == content) && (!it->left_btn)) 
		{
			edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->back_btn);	
			evas_object_smart_callback_add(it->back_btn, "clicked", _back_button_clicked, it); 
		}
	}
}

/**
 * Return the back button object of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The back button object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_back_button_get(Evas_Object *obj, 
									Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return NULL;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content)	
			return it->back_btn;
	}	
	return NULL;
}

/**
 * Set the left button object for the pushed content
 * If the content is at the top of the navigation stack, update the left button
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] button The left button
 * @param[in] animation transition when this button becomes the top item 
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_left_button_set(Evas_Object *obj, 
									Evas_Object *content, 
									Evas_Object *button, 
									Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;
	Eina_Bool changed;

	if (!wd) return;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
		{
			changed = _button_set(obj, it->left_btn, button, EINA_FALSE);
			it->left_btn = button;
			_item_sizing_eval(it);
			break;
		}
	 }

	//update if the content is the top item
	ll = eina_list_last(wd->stack);
	if (ll) 
	{
		it = ll->data;
		if (changed && (it->content == content)) 
		{
			if (edje_object_part_swallow_get(wd->base, "elm.swallow.btn1") == it->back_btn)
			{
				edje_object_part_unswallow(wd->base, it->back_btn);
				evas_object_hide(it->back_btn);
			}
			edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->left_btn);
		}
	}
}

/**
 * Return the left button object of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The left button object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_left_button_get(Evas_Object *obj, 	
									Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return NULL;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
			return it->left_btn;
	}	
	return NULL;
}

/**
 * set the right button object for the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @param[in] button The right button
 * @param[in] animation transition when this button becomes the top item
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_right_button_set(Evas_Object *obj, 
										Evas_Object *content, 
										Evas_Object *button, 
										Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;
	Eina_Bool changed;

	if (!wd) return;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
		{
			changed = _button_set(obj, it->right_btn, button, EINA_FALSE);
			it->right_btn = button;
			_item_sizing_eval(it);
			break;
		}
	 }	

	//update if the content is the top item
	ll = eina_list_last(wd->stack);
	if (ll) 
	{
		it = ll->data;
		if (changed && (it->content == content)) 
			edje_object_part_swallow(wd->base, "elm.swallow.btn2", it->right_btn);
	}
}

/**
 * Return the right button of the pushed content
 *
 * @param[in] obj The NavigationBar object
 * @param[in] content The object to push or pushed
 * @return The right button object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_right_button_get(Evas_Object *obj, 
										Evas_Object *content)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Eina_List *ll;
	Item *it;

	if (!wd) return NULL;

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
			return it->right_btn;
	}
	return NULL;
}

/**
 * Return the content object at the top of the pager stack
 *
 * @param[in] obj The NavigationBar object
 * @return The top content object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_content_top_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return elm_pager_content_top_get(wd->pager);
}

/**
 * Return the content object at the top of the pager stack
 *
 * @param[in] obj The NavigationBar object
 * @return The root content object or NULL if none
 *
 * @ingroup NavigationBar
 */
EAPI Evas_Object *
elm_navigationbar_content_root_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	return elm_pager_content_bottom_get(wd->pager);
}

/**
 * set navigationbar hidden state and update
 *
 * @param[in] obj The NavigationBar object
 * @param[in] hidden hidden state (default value : TRUE)
 * @param[in] animation transition when this button becomes the top item (default value : TRUE)
 *
 * @ingroup NavigationBar
 */
EAPI void
elm_navigationbar_hidden_set(Evas_Object *obj, 
								Eina_Bool hidden, 
								Eina_Bool animation)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (hidden) edje_object_signal_emit(wd->base, "elm,state,item,moveup", "elm");
	else edje_object_signal_emit(wd->base, "elm,state,item,movedown", "elm");
}

