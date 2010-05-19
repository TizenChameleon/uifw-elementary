#include <Elementary.h>
#include "elm_priv.h"

/**
 * @addtogroup NavigationBar NavigationBar
 *
 * The pager is an object that allows flipping (with animation) between 1 or
 * more of objects, much like a stack of windows within the window.
 *
 * Objects can be pushed or popped from he stack or deleted as normal.
 * Pushes and pops will animate (and a pop will delete the object once the
 * animation is finished). Any object in the pager can be promoted to the top
 * (from its current stacking position) as well. Objects are pushed to the
 * top with elm_pager_content_push() and when the top item is no longer
 * wanted, simply pop it with elm_pager_content_pop() and it will also be
 * deleted. Any object you wish to promote to the top that is already in the
 * pager, simply use elm_pager_content_promote(). If an object is no longer
 * needed and is not the top item, just delete it as normal. You can query
 * which objects are the top and bottom with elm_pager_content_bottom_get()
 * and elm_pager_content_top_get().
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

static void _back_button_clicked(void *data, Evas_Object *obj, void *event_info);
static int _get_button_width(Evas_Object *obj, Eina_Bool back_btn);
static Eina_Bool _button_set(Evas_Object *obj, Evas_Object *prev_btn, Evas_Object *new_btn, Eina_Bool back_btn);
static const char *_title_string_get(Evas_Object *title);
static void _label_set(Evas_Object* label, const char* title);

static void _transition_complete_cb(void *data);
static Elm_Transit *_transition_set(Item* prev_it, Item *it, Evas_Coord y, Eina_Bool pop);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	elm_widget_sub_object_del(obj, wd->pager);

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
		elm_object_style_set(it->left_btn, "custom/darkblue");
		elm_object_style_set(it->right_btn, "custom/darkblue");
		elm_object_style_set(it->back_btn, "backkey");
	}

	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1;
	Eina_List *ll;
	Item *it;

	edje_object_size_min_calc(wd->base, &minw, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, -1, -1);

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		_item_sizing_eval(it);
	}
}

static void
_item_sizing_eval(Item *it)
{
	Widget_Data *wd = elm_widget_data_get(it->obj);
	Evas_Coord pad, height, minw;

	edje_object_size_min_calc(wd->base, &minw, NULL);
	edje_object_part_geometry_get(wd->base, "elm.rect.pad2", NULL, NULL, &pad, &height);

	if (it->left_btn) 
	{
		it->left_btn_w = _get_button_width(it->left_btn, FALSE);
		evas_object_resize(it->left_btn, it->left_btn_w , height);
		evas_object_size_hint_min_set(it->left_btn, it->left_btn_w , height);
	}
	else if (it->back_btn)
	{
		it->left_btn_w = _get_button_width(it->back_btn, TRUE);
		evas_object_resize(it->back_btn, it->left_btn_w, height);
		evas_object_size_hint_min_set(it->back_btn, it->left_btn_w, height);
	}
	
	if (it->right_btn)
	{
		it->right_btn_w = _get_button_width(it->right_btn, FALSE);
		evas_object_resize(it->right_btn, it->right_btn_w, height);
		evas_object_size_hint_min_set(it->right_btn, it->right_btn_w, height);
	}

	if (it->title) 
	{
		const char *str = NULL;

		it->title_w = minw - it->left_btn_w - it->right_btn_w - 4*pad;
		evas_object_resize(it->title, it->title_w, height);
		evas_object_size_hint_min_set(it->title, it->title_w, height);
		
		str = _title_string_get(it->title);
		if (str) 
		{
			double align;
			const char align_str[7] = {0,};

			align = 0.5 - ((it->left_btn_w-it->right_btn_w)/(double)it->title_w);
			if (align < 0.0) align = 0.0;
			sprintf(align_str, "%lf", align);
			elm_label_text_align_set(it->title, align_str);
		}
	}
}

static void
_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord x, y;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);

	/* add sub object moving!!!! */
}

static void
_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Coord w, h;

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	/* add sub object resizing!!!! */
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

		if (prev_it->left_btn) elm_widget_sub_object_del(prev_it->obj, prev_it->left_btn);
		else if (prev_it->back_btn) elm_widget_sub_object_del(prev_it->obj, prev_it->back_btn);
		if (prev_it->right_btn) elm_widget_sub_object_del(prev_it->obj, prev_it->right_btn);
		if (prev_it->title) elm_widget_sub_object_del(prev_it->obj, prev_it->title);

		evas_object_del(prev_it->left_btn);
		evas_object_del(prev_it->back_btn);
		evas_object_del(prev_it->right_btn);
		evas_object_del(prev_it->title);

		ll = eina_list_last(wd->stack);
		if (ll->data == prev_it)
		{
			wd->stack = eina_list_remove_list(wd->stack, ll);
			free(prev_it);
		}
	}
	else if (prev_it)
	{
	 	evas_object_hide(prev_it->left_btn);
		evas_object_hide(prev_it->back_btn);
		evas_object_hide(prev_it->title);
		evas_object_hide(prev_it->right_btn);
	}

	if (it)
	{
		if (it->left_btn) edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->left_btn);
		else if (it->back_btn) edje_object_part_swallow(wd->base, "elm.swallow.btn1", it->back_btn);
		if (it->right_btn) edje_object_part_swallow(wd->base, "elm.swallow.btn2", it->right_btn);
		if (it->title) edje_object_part_swallow(wd->base, "elm.swallow.title", it->title);
	}

	free(cb);
}

static Elm_Transit *
_transition_set(Item* prev_it, Item *it, Evas_Coord y, Eina_Bool pop)
{
	Widget_Data *wd = elm_widget_data_get(it->obj);
	Evas_Coord pad, minw;
	Elm_Transit *transit;
	int num = 1;

	transit = elm_transit_add(it->obj);
	edje_object_part_geometry_get(wd->base, "elm.rect.pad1", NULL, NULL, &pad, NULL);
	edje_object_size_min_calc(wd->base, &minw, NULL);

	if (pop) num = -1;

	// hide prev item
	if (prev_it)
	{
		if (prev_it->left_btn) 
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(prev_it->left_btn, pad, y, pad-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->left_btn, 255, 255, 255, 255, 0, 0, 0, 0));
		}
		else if (prev_it->back_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(prev_it->back_btn, pad, y, pad-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->back_btn, 255, 255, 255, 255, 0, 0, 0, 0));
		}
		if (prev_it->title)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(prev_it->title, prev_it->left_btn_w+2*pad, y, prev_it->left_btn_w+2*pad-num*EFFECT_WIDTH, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->title, 255, 255, 255, 255, 0, 0, 0, 0));
		}
		if (prev_it->right_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(prev_it->right_btn, minw-pad-prev_it->right_btn_w, y, minw-pad-prev_it->right_btn_w-num*EFFECT_WIDTH/4, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(prev_it->right_btn, 255, 255, 255, 255, 0, 0, 0, 0));
		}
	}

	// show new item
	if (it)
	{
		if (it->left_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(it->left_btn, pad+num*EFFECT_WIDTH/4, y, pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->left_btn, 0, 0, 0, 0, 255, 255, 255, 255));
		}
		else if (it->back_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(it->back_btn, pad+num*EFFECT_WIDTH/4, y, pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->back_btn, 0, 0, 0, 0, 255, 255, 255, 255));
		}
		if (it->title)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(it->title, it->left_btn_w+2*pad+num*EFFECT_WIDTH, y, it->left_btn_w+2*pad, y));
			elm_transit_fx_insert(transit, elm_fx_color_add(it->title, 0, 0, 0, 0, 255, 255, 255, 255));
		}
		if (it->right_btn)
		{
			elm_transit_fx_insert(transit, elm_fx_transfer_add(it->right_btn, minw-pad-it->right_btn_w+num*EFFECT_WIDTH/4, y, minw-pad-it->right_btn_w, y));
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

	if (back_btn) btn_len = BACK_BTN_PADDING;

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
		if (back_btn) elm_object_style_set( new_btn, "backkey" );
		else elm_object_style_set(new_btn, "custom/darkblue");
		elm_widget_sub_object_add(obj, new_btn);
		changed = TRUE;
	}

	return changed;
}

static const char *
_title_string_get(Evas_Object *title)
{
	if (!title) return;
	
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
	Item *it = ELM_NEW(Item);
	Item *prev_it = NULL;

	if (!it) return;
	
	// add new items	
	it->obj = obj;
	it->left_btn = left_btn;
	it->right_btn = right_btn;
	it->content = content;
	it->ani = animation;

	_button_set(obj, NULL, it->left_btn, EINA_FALSE);
	_button_set(obj, NULL, it->right_btn, EINA_FALSE);

	ll = eina_list_last(wd->stack);
	if (ll) prev_it = ll->data;
	
	if (!it->left_btn && prev_it)
	{
		it->back_btn = elm_button_add(obj);
		elm_button_label_set(it->back_btn, _title_string_get(prev_it->title));
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
		evas_object_geometry_get(prev_it->title, NULL, &y, NULL, NULL);

 		if (prev_it->left_btn) edje_object_part_unswallow(wd->base, prev_it->left_btn);
		else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
		if (prev_it->right_btn) edje_object_part_unswallow(wd->base, prev_it->right_btn);
		if (prev_it->title) edje_object_part_unswallow(wd->base, prev_it->title);

		if (it->ani) 
		{
			Elm_Transit* transit;
			transit = _transition_set(prev_it, it, y, EINA_FALSE);
			elm_transit_completion_set(transit, _transition_complete_cb, cb);
			elm_transit_run(transit, 0.3);
			elm_transit_del(transit); 
		}
		else 
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

	wd->stack = eina_list_append(wd->stack, it);	
	_sizing_eval(obj);
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

	ll = eina_list_last(wd->stack);
	if (ll)
	{
		prev_it = ll->data;
		ll = ll->prev;
	}

	if (ll) 
	{
		it = ll->data;
		// unswallow items and start trasition
		if (prev_it)
		{		
			Evas_Coord y;
			Transit_Cb_Data *cb = ELM_NEW(Transit_Cb_Data);
			
			cb->prev_it = prev_it;
			cb->it = it;
			cb->pop = EINA_TRUE;
			evas_object_geometry_get(prev_it->title, NULL, &y, NULL, NULL);

			if (prev_it->left_btn) edje_object_part_unswallow(wd->base, prev_it->left_btn);
			else if (prev_it->back_btn) edje_object_part_unswallow(wd->base, prev_it->back_btn);
			if (prev_it->right_btn) edje_object_part_unswallow(wd->base, prev_it->right_btn);
			if (prev_it->title) edje_object_part_unswallow(wd->base, prev_it->title);

			if (animation) 
			{
				Elm_Transit* transit;
				transit = _transition_set(prev_it, it, y, EINA_TRUE);
				elm_transit_completion_set(transit, _transition_complete_cb, cb);
				elm_transit_run(transit, 0.3);
				elm_transit_del(transit); 
			}
			else
			{
				_transition_complete_cb(cb);
			}
		}

		//pop content from pager
		elm_pager_animation_set(wd->pager, animation);
		elm_pager_content_pop(wd->pager);
	}
	else
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
	
	_sizing_eval(obj);
	}

/**
 * Set the title string for the content
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
 * Return the title string of the content
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

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
			return _title_string_get(it->title);
	}

	return NULL;
}

/**
 * Set the back button object for the content 
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
 * Return the back button object of the content
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

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content)	
			return it->back_btn;
	}	

	return NULL;
}

/**
 * Set the left button object for the content
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
 * Return the left button object of the content
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

	 EINA_LIST_FOREACH(wd->stack, ll, it)
	{
		if (it->content == content) 
			return it->left_btn;
	}	

	return NULL;
}

/**
 * set the right button object for the content
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
 * Return the right button of the content
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

	return elm_pager_content_top_get(wd->pager);
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

	if (hidden) edje_object_signal_emit(wd->base, "elm,state,item,moveup", "elm");
	else edje_object_signal_emit(wd->base, "elm,state,item,movedown", "elm");
}

