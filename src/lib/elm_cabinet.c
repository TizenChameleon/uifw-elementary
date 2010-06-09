#include <Elementary.h>
#include <math.h>
#include "elm_priv.h"

#define CELL_ITEM_NUMS (9)
#define PI (3.141592)
#define RADIAN_STEP (PI/14)
#define MOUSE_MOVE_SAMPLE (5)
#define SCALE_DOWN_FACTOR (0.25)
#define VISIBLE_CONTENT_NUMS (2)
#define DEF_ITEM_HEIGHT (74)
#define FRAME_RATE (0.03)
#define V_MIN (50)
#define ITEM_DEL_MOVE_DISTANCE (30)

//#define SCALE_DOWN_FACTOR (0.125)

/**
 * @addtogroup Cabinet Cabinet
 *
 * This is a cabinet.
 */
struct _Cabinet_Item {
	Evas_Object *cabinet;
	const char *label;
	const char *sub_info;
	void (*func)(void *data, Evas_Object *obj, void *event_info);
	void *data;
	void *priv_data;
	int btn_disabled : 1;
	int info_disabled : 1;
};

typedef struct _Widget_Data Widget_Data;
struct _Widget_Data {
	Evas_Object *base;
	Evas_Object *clip;
	Evas_Object *cover;

	Eina_List *its;
	Eina_List *cur;
	Eina_List *cells;
	Evas_Coord x, y, w, h;
	double rad_step, rad_gap, rad_tmp;
	double v;
	Evas_Coord d, item_d, item_h;
	int item_cnt;

	unsigned int prev_evt_t;
	Evas_Coord prev_evt_y;
	Evas_Coord_Point down_point;
	Elm_Animator *ani;
	Ecore_Timer *scroll_ani;

	int bounce_enable : 1;
	int bounce_flag : 1;
	int item_del_flag : 1;
	int dragged : 1;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _mouse_init(Evas_Object *obj);

static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Eina_List *_cells_add(Evas_Object *obj);
static void _cells_del(Evas_Object *obj);
static void _cells_update(Evas_Object *obj);
static double _rad_next_get(Widget_Data *wd, double rad, Evas_Coord_Rectangle *pos);
static void _items_reorder(Evas_Object *obj);

static void _ani_stop_all(Evas_Object *obj);
static void _ani_bounce_cb(void *data, Elm_Animator *animator, double frame);
static void _ani_bounce_done_cb(void *data);
static void _bounce(Evas_Object *obj);
static int _ani_scroll_cb(void *data);
static void _scroll(Evas_Object *obj);
static void _ani_item_del_cb(void *data, Elm_Animator *animator, double frame);
static void _ani_item_del_done_cb(void *data);
static void _item_del(Evas_Object *obj, Evas_Object *item);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Elm_Cabinet_Item *it;

	if (!wd) return;

	_ani_stop_all(obj);
	if (wd->its) {
		EINA_LIST_FREE(wd->its, it) {
			if (it->label) eina_stringshare_del(it->label);
			free(it);
		}
	}
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	_elm_theme_object_set(obj, wd->base, "cabinet", "base", elm_widget_style_get(obj));
	_sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
	int it_cnt;
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

	if (!wd) return;
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
	elm_coords_finger_size_adjust(1, &minw, 1, &minh);
	evas_object_size_hint_min_set(obj, minw, minh);
	evas_object_size_hint_max_set(obj, maxw, maxh);

	evas_object_geometry_get(obj, &wd->x, &wd->y, &wd->w, &wd->h);
	wd->item_h = DEF_ITEM_HEIGHT * elm_scale_get();
	if (!wd->h) return;

	wd->rad_step = asin((double)wd->item_h / wd->h);
	if (!wd->rad_step) return;
	it_cnt = (PI / 2) / wd->rad_step;
	if (wd->item_cnt != it_cnt) {
		_cells_del(obj);
		wd->item_cnt = it_cnt;
		wd->cells = _cells_add(obj);
		_cells_update(obj);
	}
	
	wd->item_d = (Evas_Coord) (2 * wd->h * (sin(wd->rad_step / 2)));
	_cells_update(obj);
}

static void
_mouse_init(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, obj);
	evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, obj);
}

static void
_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Down *ev = event_info;

	if (!wd || wd->item_del_flag) return;

	if (wd->ani) {
		elm_animator_del(wd->ani);
		wd->ani = NULL;
	}

	if (wd->scroll_ani) {
		ecore_timer_del(wd->scroll_ani);
		wd->scroll_ani = NULL;
	}

	wd->d = 0;
	wd->prev_evt_t = ev->timestamp;
	wd->prev_evt_y = ev->canvas.y;

	wd->dragged = 0;
	wd->down_point.x = ev->canvas.x;
	wd->down_point.y = ev->canvas.y;
}

static void
_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	unsigned int t;
	Evas_Coord d;
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Up *ev = event_info;

	if (!wd || wd->item_del_flag) return;

	t = ev->timestamp - wd->prev_evt_t;
	d = ev->canvas.y - wd->prev_evt_y;

	if (wd->bounce_flag || t <= 0) {
		_bounce(data);
	} else {
		wd->d = -d;
		wd->v = (double)(wd->d) / (double)t * 1000;
		if (wd->v < V_MIN && wd->v > -V_MIN)
			wd->v = wd->v < 0 ? V_MIN : -V_MIN;
		_scroll(data);
	}
	wd->prev_evt_t = 0;
	wd->prev_evt_y = 0;
}

static void
_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Evas_Event_Mouse_Move *ev = event_info;
	unsigned int t;
	Evas_Coord d;
	Evas_Coord x, y, distance;
	static int sample = MOUSE_MOVE_SAMPLE;

	if (!wd) return;
	
	if (!wd->dragged) {
		x = ev->cur.canvas.x - wd->down_point.x;
		y = ev->cur.canvas.y - wd->down_point.y;
		distance = (x * x) + (y * y);
		if (distance >= (elm_finger_size_get() >> 1)) wd->dragged = 1;
	}

	if (sample) {
		sample--;
		return;
	}
	sample = MOUSE_MOVE_SAMPLE;

	if (wd->item_del_flag || !ev->timestamp) return;

	if (wd->prev_evt_t) {
		t = ev->timestamp - wd->prev_evt_t;
		d = ev->cur.canvas.y - wd->prev_evt_y;
		wd->d -= d;
		_items_reorder(data);
		wd->rad_gap = wd->rad_step * ((double)wd->d / (double)wd->item_d);
		_cells_update(data);
	}
	wd->prev_evt_t = ev->timestamp;
	wd->prev_evt_y = ev->cur.canvas.y;
}

static void
_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static void
_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_sizing_eval(data);
}

static void
_item_del_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	if (!wd || wd->item_del_flag || wd->dragged) return;
	_ani_stop_all(data);
	_item_del(data, obj);

}

static void
_item_sel_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Widget_Data *wd = elm_widget_data_get(data);
	Elm_Cabinet_Item *it;

	if (!wd) return;
	if (wd->dragged) return;

	it = evas_object_data_get(obj, "item");
	if (it && it->func) {
		it->func(it->data, it->cabinet, it);
	}
}

static Eina_List *
_cells_add(Evas_Object *obj)
{
	int i;
	Evas *e;
	Eina_List *cells = NULL;
	Evas_Object *eo;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	e = evas_object_evas_get(obj);
	for (i = 0; i < wd->item_cnt; i++) {
		eo = edje_object_add(e);
		_elm_theme_object_set(obj, eo, "cabinet/item", "base", elm_widget_style_get(obj));
		edje_object_signal_callback_add(eo, "elm,action,del", "*", _item_del_cb, obj);
		edje_object_signal_callback_add(eo, "elm,action,clicked", "*", _item_sel_cb, obj);
		cells = eina_list_append(cells, eo);
		elm_widget_sub_object_add(obj, eo);
		evas_object_smart_member_add(eo, obj);
		evas_object_clip_set(eo, wd->clip);
		evas_object_stack_below(eo, wd->base);
	}
	return cells;
}

static void
_cells_del(Evas_Object *obj)
{
	Evas_Object *eo;
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd || !wd->cells) return;
	EINA_LIST_FREE(wd->cells, eo) evas_object_del(eo);
}

static void
_cells_update(Evas_Object *obj)
{
	int color;
	double rad;
	Eina_List *l, *data;
	Evas_Object *eo;
	Elm_Cabinet_Item *it;
	Widget_Data *wd = elm_widget_data_get(obj);
	Evas_Coord_Rectangle pos = {0, };

	if (!wd || !wd->cells) return;
	data = wd->cur;
	rad = wd->rad_gap;

	EINA_LIST_REVERSE_FOREACH(wd->cells, l, eo) {
		rad = _rad_next_get(wd, rad, &pos);
		evas_object_move(eo, pos.x, pos.y);
		evas_object_resize(eo, pos.w, wd->item_h);
		
		if (wd->h - wd->item_h) color = 255 * ((double)pos.y / (wd->h - wd->item_h));
		else color = 255;
		evas_object_color_set(eo, color, color, color, 255);

		if (data) {
			it = eina_list_data_get(data);
			edje_object_part_text_set(eo, "elm.label", it->label);
			edje_object_part_text_set(eo, "elm.info_label", it->info_disabled ? NULL : it->sub_info);
			edje_object_signal_emit(eo, it->btn_disabled ? "info_disable" : "info_enable", "elm");
			data = eina_list_next(data);
			evas_object_show(eo);
			evas_object_data_set(eo, "item", it);
		} else {
			evas_object_hide(eo);
			evas_object_data_set(eo, "item", NULL);
		}
	}
}

static double
_rad_next_get(Widget_Data *wd, double rad, Evas_Coord_Rectangle *pos)
{
	if (rad >= 0) pos->w = wd->w * (cos(rad / 2));
	else pos->w = wd->w;
	rad += wd->rad_step;
	pos->y = (wd->h * (1 - sin(rad))) + wd->y;
	pos->x = wd->x + ((wd->w - pos->w) >> 1);
	return rad;
}

static void
_items_reorder(Evas_Object *obj)
{
	Eina_List *l;
	Widget_Data *wd = elm_widget_data_get(obj);

	if ((wd->d > 0 && wd->cur == wd->its)
			|| (wd->d < 0 && wd->cur == eina_list_last(wd->its))) {
		if (wd->bounce_enable)
			wd->bounce_flag = 1;
		else
			wd->d = 0;
		return;
	}
	wd->bounce_flag = 0;

	l = wd->cur;
	if (wd->d > 0) {
		do {
			if (l) l = eina_list_prev(l);
			wd->d -= wd->item_d;
		} while (wd->d > 0);
	} else if (wd->d <= -wd->item_d) {
		do {
			l = eina_list_next(l);
			if (!l) break;
			wd->d += wd->item_d;
		} while (wd->d <= -wd->item_d);
		wd->d = 0;
	}

	if (l) wd->cur = l;
	return;
}


static void
_ani_stop_all(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = elm_widget_data_get(obj);
	if (!wd) return;

	if (wd->ani) {
		elm_animator_del(wd->ani);
		wd->ani = NULL;
	}

	if (wd->scroll_ani) {
		ecore_timer_del(wd->scroll_ani);
		wd->scroll_ani = NULL;
	}
}

static void
_ani_bounce_cb(void *data, Elm_Animator *animator, double frame)
{
	Widget_Data *wd;
	wd = elm_widget_data_get(data);
	if (!wd) return;

	wd->rad_gap = wd->rad_tmp * (1 - frame);
	_cells_update(data);
}

static void
_ani_bounce_done_cb(void *data)
{
	Widget_Data *wd = elm_widget_data_get(data);
	wd->item_del_flag = 0;
	wd->rad_gap = 0;
	if (wd->ani) {
		elm_animator_del(wd->ani);
		wd->ani = NULL;
	}

	if (wd->cur)
		evas_object_smart_callback_call(data, "changed", wd->cur ? eina_list_data_get(wd->cur) : NULL);
}

static void
_bounce(Evas_Object *obj)
{
	double dur;
	Widget_Data *wd;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->d) {
		wd->item_del_flag = 0;
		return;
	}

	if (wd->item_del_flag) dur = 0.2;
	else dur = 0.5;

	wd->d = 0;
	wd->rad_tmp = wd->rad_gap;
	if (wd->ani) elm_animator_del(wd->ani);
	wd->ani = elm_animator_add(obj);
	elm_animator_curve_style_set(wd->ani, ELM_ANIMATOR_CURVE_OUT);
	elm_animator_duration_set(wd->ani, dur);
	elm_animator_operation_callback_set(wd->ani, _ani_bounce_cb, obj);
	elm_animator_completion_callback_set(wd->ani, _ani_bounce_done_cb, obj);
	elm_animator_animate(wd->ani);
}

static int
_ani_scroll_cb(void *data)
{
	Widget_Data *wd;
	Evas_Coord d;
	wd = elm_widget_data_get(data);
	if (!wd) return 0;

	d = wd->v * FRAME_RATE;
	wd->d += d;

	if (wd->v > (V_MIN >> 1) || wd->v < (-V_MIN >> 1)) {
		if (wd->cur == eina_list_last(wd->its) || wd->cur == wd->its)
			wd->v *= 0.5;
		else
			wd->v *= 0.9;
	} else {
		wd->scroll_ani = NULL;
		_bounce(data);
		return 0;
	}

	_items_reorder(data);
	wd->rad_gap = wd->rad_step * ((double)wd->d / (double)wd->item_d);
	_cells_update(data);

	return 1;
}

static void
_scroll(Evas_Object *obj)
{
	Widget_Data *wd;
	wd = elm_widget_data_get(obj);
	if (!wd) return;


	if (wd->ani) {
		elm_animator_del(wd->ani);
		wd->ani = NULL;
	}

	if (wd->scroll_ani) ecore_timer_del(wd->scroll_ani);
	wd->scroll_ani = ecore_timer_add(FRAME_RATE, _ani_scroll_cb, obj);
}

static void
_ani_item_del_cb(void *data, Elm_Animator *animator, double frame)
{
	Evas_Object *it = data;
	int color;
	Evas_Coord x, y;
	x = (Evas_Coord)evas_object_data_get(it, "X");
	y = (Evas_Coord)evas_object_data_get(it, "Y");

	if (frame == 1) {
		evas_object_color_set(it, 255, 255, 255, 255);
	} else {
		y -= ITEM_DEL_MOVE_DISTANCE * elm_scale_get() * frame;
		evas_object_move(it, x, y);
		color = 255 * (1 - frame);
		evas_object_color_set(it, color, color, color, color);
	}
}

static void
_ani_item_del_done_cb(void *data)
{
	Evas_Object *obj;
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *it, *_it;
	it = data;
	if (!it) return;
	obj = it->cabinet;
	wd = elm_widget_data_get(obj);
	if (!wd) return;
	if (wd->ani) {
		elm_animator_del(wd->ani);
		wd->ani = NULL;
	}

	EINA_LIST_FOREACH(wd->its, l, _it) {
		if (_it == it) {
			wd->its = eina_list_remove_list(wd->its, l);
			if (wd->cur == l) {
				wd->cur = eina_list_prev(l);
				if (!wd->cur) wd->cur = wd->its;
			}
			evas_object_smart_callback_call(obj, "item,deleted", _it);
			free(_it);
			break;
		}
	}

	_cells_update(obj);
	if (wd->cur == NULL) {
		evas_object_smart_callback_call(obj, "item,deleted,all", NULL);
	}
	_bounce(obj);
}

static void
_item_del(Evas_Object *obj, Evas_Object *item)
{
	Evas_Coord x, y;
	Widget_Data *wd;
	Elm_Cabinet_Item *it;
	wd = elm_widget_data_get(obj);
	if (!wd) return;

	it = evas_object_data_get(item, "item");
	if (!it) return;
	wd->item_del_flag = 1;

	evas_object_geometry_get(item, &x, &y, NULL, NULL);
	evas_object_data_set(item, "X", (void *)x);
	evas_object_data_set(item, "Y", (void *)y);

	wd->ani = elm_animator_add(obj);
	elm_animator_curve_style_set(wd->ani, ELM_ANIMATOR_CURVE_OUT);
	elm_animator_duration_set(wd->ani, 0.5);

	elm_animator_operation_callback_set(wd->ani, _ani_item_del_cb, item);
	elm_animator_completion_callback_set(wd->ani, _ani_item_del_done_cb, it);
	elm_animator_animate(wd->ani);
}

/**
 * Add a new cabinet to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Cabinet
 */
EAPI Evas_Object *
elm_cabinet_add(Evas_Object *parent)
{
	Evas_Object *obj;
	Evas *e;
	Widget_Data *wd;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "cabinet");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);

	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->clip = evas_object_rectangle_add(e);
	elm_widget_resize_object_set(obj, wd->clip);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move_cb, obj);

	wd->base = edje_object_add(e);
	_elm_theme_object_set(obj, wd->base, "cabinet", "base", "default");
	elm_widget_hover_object_set(obj, wd->base);
	evas_object_clip_set(wd->base, wd->clip);
	evas_object_smart_member_add(wd->base, obj);
	evas_object_show(wd->base);

	_mouse_init(obj);
	_sizing_eval(obj);
	return obj;
}

EAPI Eina_Bool
elm_cabinet_bounce_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return EINA_FALSE;
	return wd->bounce_enable ? EINA_TRUE : EINA_FALSE;
}

EAPI void
elm_cabinet_bounce_set(Evas_Object *obj, Eina_Bool bounce)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return;
	wd->bounce_enable = bounce ? 1 : 0;
}

/**
 * Select next item of cabinet
 *
 * @param obj The cabinet object
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_next(Evas_Object *obj)
{
}

/**
 * Select previous item of cabinet
 *
 * @param obj The cabinet object
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_prev(Evas_Object *obj)
{
}

/**
 * Append item to the end of cabinet
 *
 * @param obj The cabinet object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_item_append(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Cabinet_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Cabinet_Item);
	if (item) {
		if (label) item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->cabinet = obj;
		wd->its = eina_list_append(wd->its, item);
		if (!wd->cur) {
			wd->cur = wd->its;
		}
		_sizing_eval(obj);
	}
	return item;
}

/**
 * Prepend item at start of cabinet
 *
 * @param obj The cabinet object
 * @param label The label of new item
 * @param func Convenience function called when item selected
 * @param data Data passed to @p func above
 * @return A handle to the item added or NULL if not possible
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_item_prepend(Evas_Object *obj, const char *label, void (*func)(void *data, Evas_Object *obj, void *event_info), void *data)
{
	Elm_Cabinet_Item *item;
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;

	item = ELM_NEW(Elm_Cabinet_Item);
	if (item) {
		if (label) item->label = eina_stringshare_add(label);
		item->func = func;
		item->data = data;
		item->cabinet = obj;
		wd->its = eina_list_prepend(wd->its, item);
		if (!wd->cur) wd->cur = wd->its;
		_sizing_eval(obj);
	}
	return item;
}

/**
 * Get a list of items in the cabinet
 *
 * @param obj The cabinet object
 * @return The list of items, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI const Eina_List *
elm_cabinet_items_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd) return NULL;
	return wd->its;
}

/**
 * Get the first item in the cabinet
 *
 * @param obj The cabinet object
 * @return The first item, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_first_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->its) return NULL;
	return eina_list_data_get(wd->its);
}

/**
 * Get the last item in the cabinet
 *
 * @param obj The cabinet object
 * @return The last item, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_last_item_get(Evas_Object *obj)
{
	Widget_Data *wd;
	if (!obj) return NULL;
	wd = elm_widget_data_get(obj);
	if (!wd || !wd->its) return NULL;
	return eina_list_data_get(eina_list_last(wd->its));
}

/**
 * Get the selected item in the cabinet
 *
 * @param obj The cabinet object
 * @return The selected item, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_selected_item_get(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd || !wd->cur) return NULL;
	return eina_list_data_get(wd->cur);
}

/**
 * Set the selected state of an item
 *
 * @param item The item
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_item_selected_set(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->its, l, _item) {
		if (_item == item) {
			;
			//TODO: NOT YET
		}
	}
}

/**
 * Delete a given item
 *
 * @param item The item
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_item_del(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd) return;

	EINA_LIST_FOREACH(wd->its, l, _item) {
		if (_item == item) {
			wd->its = eina_list_remove_list(wd->its, l);
			if (wd->cur == l)
				wd->cur = wd->its;
			free(_item);
			break;
		}
	}
}

/**
 * Get the label of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI const char *
elm_cabinet_item_label_get(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return NULL;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item)
			return item->label;

	return NULL;
}

/**
 * Set the label of a given item
 *
 * @param item The item
 * @param label The text label string in UTF-8
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_item_label_set(Elm_Cabinet_Item *item, const char *label)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item || !label) return;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item) {
			eina_stringshare_del(item->label);
			item->label = eina_stringshare_add(label);
		}
}

/**
 * Get the sub info of a given item
 *
 * @param item The item
 * @return The label of a given item, or NULL if none
 *
 * @ingroup Cabinet
 */
EAPI const char *
elm_cabinet_item_sub_info_get(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return NULL;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item)
			return item->sub_info;

	return NULL;
}

/**
 * Set the sub info of a given item
 *
 * @param item The item
 * @param label The text label string in UTF-8
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_item_sub_info_set(Elm_Cabinet_Item *item, const char *sub_info)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item || !sub_info) return;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item) {
			eina_stringshare_del(item->sub_info);
			item->sub_info = eina_stringshare_add(sub_info);
		}
}

/**
 * Get the previous item in the cabinet
 *
 * @param item The item
 * @return The item before the item @p item
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_item_prev(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return NULL;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item) {
			l = eina_list_prev(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}

/**
 * Get the next item in the cabinet
 *
 * @param item The item
 * @return The item after the item @p item
 *
 * @ingroup Cabinet
 */
EAPI Elm_Cabinet_Item *
elm_cabinet_item_next(Elm_Cabinet_Item *item)
{
	Widget_Data *wd;
	Eina_List *l;
	Elm_Cabinet_Item *_item;

	if (!item) return NULL;
	wd = elm_widget_data_get(item->cabinet);
	if (!wd || !wd->its) return NULL;

	EINA_LIST_FOREACH(wd->its, l, _item)
		if (_item == item) {
			l = eina_list_next(l);
			if (!l) return NULL;
			return eina_list_data_get(l);
		}
	return NULL;
}

/**
 * Get private data of item
 *
 * @param item The item
 * @return Private data of the item @p item
 *
 * @ingroup Cabinet
 */
EAPI void *
elm_cabinet_item_data_get(Elm_Cabinet_Item *item)
{
	if (!item) return NULL;
	return item->priv_data;
}

/**
 * Set private data of item
 *
 * @param item The item
 * @param data data
 *
 * @ingroup Cabinet
 */
EAPI void
elm_cabinet_item_data_set(Elm_Cabinet_Item *item, void *data)
{
	if (!item) return;
	item->priv_data = data;
}

EAPI Eina_Bool
elm_cabinet_item_del_btn_disabled_get(Elm_Cabinet_Item *item)
{
	if (!item) return EINA_FALSE;
	return item->btn_disabled ? EINA_TRUE : EINA_FALSE;
}

EAPI void
elm_cabinet_item_del_btn_disabled_set(Elm_Cabinet_Item *item, Eina_Bool disabled)
{
	if (!item) return;

	if (item->btn_disabled == !!disabled) return;
	item->btn_disabled = !!disabled;
	_cells_update(item->cabinet);
}

EAPI Eina_Bool
elm_cabinet_item_sub_info_disabled_get(Elm_Cabinet_Item *item)
{
	if (!item) return EINA_FALSE;
	return item->info_disabled ? EINA_TRUE : EINA_FALSE;
}

EAPI void
elm_cabinet_item_sub_info_disabled_set(Elm_Cabinet_Item *item, Eina_Bool disabled)
{
	if (!item) return;
	
	if (item->info_disabled == !!disabled) return;
	item->info_disabled = !!disabled;
	_cells_update(item->cabinet);
}
