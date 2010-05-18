#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

/**
 * @addtogroup Colorpalette Colorpalette
 *
 * Using colorpalette, you can select a color by clicking
 * a color rectangle on the colorpalette.
 */


#define MAX_NUM_COLORS 30
#define COLOR_RECT_LENGTH 66

typedef struct _Colorpalette_Item Colorpalette_Item;
struct _Colorpalette_Item
{
	Evas_Object *parent;
	Evas_Object *lo;
	Evas_Object *cr;
	unsigned int r, g, b;
};


typedef struct _Widget_Data Widget_Data;
struct _Widget_Data
{
	Evas_Object *parent;
	Evas_Object *lay;
	Evas_Object *tab;

	Evas_Coord x, y, w, h;
	Evas_Coord tab_w, tab_h;
	Evas_Coord rect_w, rect_h;

	unsigned int row, col;
	Elm_Colorpalette_Color *color;

	Eina_List *items;

	unsigned int num;
};

static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _sub_del(void *data, Evas_Object *obj, void *event_info);


static void _colorpalette_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _colorpalette_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _colorpalette_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _colorpalette_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _color_select_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _color_release_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _color_table_delete(Evas_Object *obj);
static void _color_table_update(Evas_Object *obj, int row, int col, int color_num, Elm_Colorpalette_Color *color);

static void
_del_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return;

	_color_table_delete(obj);

	if (wd->color){
		free(wd->color);
	}

	if (wd->lay){
		evas_object_smart_member_del(wd->lay);
		evas_object_del(wd->lay);
		wd->lay = NULL;
	}
	free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	if (!wd)
		return;

	_elm_theme_set(wd->lay, "colorpalette", "bg", elm_widget_style_get(obj));
	_color_table_update(obj, wd->row, wd->col, wd->num, wd->color);
	_sizing_eval(obj);

    // FIXME : add more codes
    //

}


static void
_sub_del(void *data, Evas_Object *obj, void *event_info)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	//Evas_Object *sub = event_info;
	if (!wd)
	       	return;
}

static void
_sizing_eval(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd)
	       	return;

	DBG("[%s]\n", __func__);

	_colorpalette_object_move(obj, NULL, obj, NULL);
	_colorpalette_object_resize(obj, NULL, obj, NULL);
}


static void _colorpalette_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;
	Evas_Coord x, y;

	if(!data)
		return;

	wd = elm_widget_data_get((Evas_Object *)data);

	if(!wd)
	       	return;

	evas_object_geometry_get(wd->lay, &x, &y, NULL, NULL);

	wd->x = x;
	wd->y = y;

	evas_object_move(wd->lay, x, y);
}


static void _colorpalette_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd;
	Colorpalette_Item *item;
	Evas_Coord w, h;
	Evas_Coord tab_w, tab_h;
	double pad_x, pad_y;

	if(!data)
	       	return;

	wd = elm_widget_data_get((Evas_Object *)data);

	if(!wd)
	       	return;

	evas_object_geometry_get(wd->lay, NULL, NULL, &w, &h);
	wd->w = w;
	wd->h = h;

	evas_object_geometry_get(wd->tab, NULL, NULL, &tab_w, &tab_h);
	if (tab_w > 0 && tab_h > 0) {
		wd->tab_w = tab_w;
		wd->tab_h = tab_h;
	}

	if (wd->items)
		item = wd->items->data;

	edje_object_part_geometry_get(elm_layout_edje_get(item->lo),"focus" ,NULL, NULL, &wd->rect_w, &wd->rect_h);


	pad_x = ((double)wd->tab_w - (double)wd->rect_w * (double)wd->col) / (double)(wd->col - 1);
	pad_y = ((double)wd->tab_h - (double)wd->rect_h * (double)wd->row) / (double)(wd->row - 1);

	if (pad_x < 0.0 )
		pad_x = 0;
	if (pad_y < 0.0 )
		pad_y = 0;

	elm_table_padding_set(wd->tab, (int)pad_x , (int)pad_y);

	if(!wd->lay)
		return;

	evas_object_resize(wd->lay, w, h);
}


static void _colorpalette_object_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd = NULL;

	if(data == NULL)
	       	return;

	wd = elm_widget_data_get((Evas_Object *)data);


	if(wd == NULL)
	       	return;

	if (wd->lay) {
		evas_object_show(wd->lay);
	}
}

static void _colorpalette_object_hide(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("%s", __func__);

	Widget_Data *wd = NULL;

	if(data == NULL)
	       	return;

	wd = elm_widget_data_get((Evas_Object *)data);

	if(wd == NULL)
		return;

	if (wd->lay) {
		evas_object_hide(wd->lay);
	}
}

static void _color_select_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Colorpalette_Item *item = (Colorpalette_Item *) data;
	Elm_Colorpalette_Color *color;

	color = ELM_NEW(Elm_Colorpalette_Color);

	color->r = item->r;
	color->g = item->g;
	color->b = item->b;

	evas_object_smart_callback_call(item->parent, "clicked", color);

	edje_object_signal_emit(elm_layout_edje_get(item->lo), "focus_visible", "elm");
}


static void _color_release_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Colorpalette_Item *item = (Colorpalette_Item *) data;
	edje_object_signal_emit(elm_layout_edje_get(item->lo), "focus_invisible", "elm");
}


static void _color_table_delete(Evas_Object *obj)
{
	Widget_Data *wd = NULL;
	Colorpalette_Item *item;
	wd = elm_widget_data_get(obj);

	if (!wd) return;

	if (wd->items) {
		EINA_LIST_FREE(wd->items, item) {
			if (item->lo){
				evas_object_del(item->lo);
				item->lo = NULL;
			}
			if (item->cr){
				evas_object_del(item->cr);
				item->cr = NULL;
			}
			free(item);
		}
	}

	if (wd->tab) {
		edje_object_part_unswallow(wd->lay, wd->tab);
		evas_object_del(wd->tab);
	}
}


static void _color_table_update(Evas_Object *obj, int row, int col, int color_num, Elm_Colorpalette_Color *color)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	Colorpalette_Item *item;
	Evas_Object *lo;
	Evas_Object *cr;
	Evas *e;
	int i, j, count;

	count = 0;

	if ( !wd )
		return;

	e = evas_object_evas_get(wd->parent);

	_color_table_delete(obj);

	wd->row = row;
	wd->col = col;
	wd->num = color_num;

	wd->tab = elm_table_add(obj);

	evas_object_size_hint_weight_set(wd->tab, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wd->tab, EVAS_HINT_FILL, EVAS_HINT_FILL);

	edje_object_part_swallow(wd->lay, "palette", wd->tab);
	evas_object_show(wd->tab);

	for ( i = 0 ; i < row ; i++) {
		for ( j = 0 ; j < col ; j++ ) {
			item = ELM_NEW(Colorpalette_Item);
			if (item){
				lo = elm_layout_add(obj);
				elm_layout_theme_set(lo, "colorpalette", "base", "bg");
				evas_object_size_hint_weight_set(lo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				evas_object_size_hint_align_set(lo, EVAS_HINT_FILL, EVAS_HINT_FILL);
				evas_object_show(lo);
				elm_table_pack(wd->tab, lo, j, i, 1, 1);

				item->parent = obj;
				item->lo = lo;

				if (count < color_num){
					cr =  edje_object_add(e);
					_elm_theme_set(cr, "colorpalette", "base", "color");
					evas_object_color_set(cr, color[count].r, color[count].g, color[count].b, 255);
					evas_object_size_hint_weight_set(cr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					evas_object_size_hint_align_set(cr, EVAS_HINT_FILL, EVAS_HINT_FILL);

					evas_object_event_callback_add(cr, EVAS_CALLBACK_MOUSE_DOWN, _color_select_cb, item);
					evas_object_event_callback_add(cr, EVAS_CALLBACK_MOUSE_UP, _color_release_cb, item);

					evas_object_show(cr);
					edje_object_part_swallow(elm_layout_edje_get(lo), "color_rect", cr);

					item->cr = cr;
					item->r = color[count].r;
					item->g = color[count].g;
					item->b = color[count].b;
				}

				wd->items = eina_list_append(wd->items, item);

				count ++;
			}
		}
	}
}


/**
 * Add a new colorpalette to the parent.
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Colorpalette
 */
EAPI Evas_Object *elm_colorpalette_add(Evas_Object *parent)
{
	Evas_Object *obj = NULL;
	Widget_Data *wd = NULL;
	Evas *e;
	Elm_Colorpalette_Color *color;

	wd = ELM_NEW(Widget_Data);
	e = evas_object_evas_get(parent);
	if (e == NULL) return NULL;
	obj = elm_widget_add(e);
	elm_widget_type_set(obj, "colorpalette");
	elm_widget_sub_object_add(parent, obj);
	elm_widget_data_set(obj, wd);
	elm_widget_del_hook_set(obj, _del_hook);
	elm_widget_theme_hook_set(obj, _theme_hook);

	wd->parent = parent;
	/* load background edj */
	wd->lay = edje_object_add(e);
	 _elm_theme_set(wd->lay, "colorpalette", "bg", "default");

	evas_object_size_hint_weight_set(wd->lay, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if(wd->lay == NULL) {
		printf("Cannot load bg edj\n");
		return NULL;
	}

	wd->color = (Elm_Colorpalette_Color*) calloc (10, sizeof(Elm_Colorpalette_Color));

	wd->color[0].r = 55; wd->color[0].g = 90; wd->color[0].b = 18;
	wd->color[1].r = 255; wd->color[1].g = 213; wd->color[1].b = 0;
	wd->color[2].r = 146; wd->color[2].g = 255; wd->color[2].b = 11;
	wd->color[3].r = 9; wd->color[3].g = 186; wd->color[3].b = 10;
	wd->color[4].r = 86; wd->color[4].g = 201; wd->color[4].b = 242;
	wd->color[5].r = 18; wd->color[5].g = 83; wd->color[5].b = 128;
	wd->color[6].r = 140; wd->color[6].g = 53; wd->color[6].b = 238;
	wd->color[7].r = 255; wd->color[7].g = 145; wd->color[7].b = 145;
	wd->color[8].r = 255; wd->color[8].g = 59; wd->color[8].b = 119;
	wd->color[9].r = 133; wd->color[9].g = 100; wd->color[9].b = 69;

	_color_table_update(obj, 2, 5, 10, wd->color);

	elm_widget_resize_object_set(obj, wd->lay);
//	evas_object_smart_member_add(wd->lay, obj);
	evas_object_event_callback_add(wd->lay, EVAS_CALLBACK_RESIZE, _colorpalette_object_resize, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _colorpalette_object_move, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _colorpalette_object_show, obj);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _colorpalette_object_hide, obj);

	// FIXME
	// evas_object_smart_callback_add(obj, "sub-object-del", _sub_del, obj);
	// _sizing_eval(obj);

	return obj;
}


/**
 * Set colors to the colorpalette.
 *
 * @param obj   Colorpalette object
 * @param color_num     number of the colors on the colorpalette
 * @param color     Color lists
 *
 * @ingroup Colorpalette
 */
EAPI void elm_colorpalette_color_set(Evas_Object *obj, int color_num, Elm_Colorpalette_Color *color)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int i;

	if (color_num > MAX_NUM_COLORS) return;

	if (!wd) return;

	if (wd->color) {
		free(wd->color);
		wd->color = NULL;
	}

	wd->color = (Elm_Colorpalette_Color*) calloc (color_num, sizeof(Elm_Colorpalette_Color));

	for ( i = 0 ; i < color_num ; i++) {
		wd->color[i].r = color[i].r;
		wd->color[i].g = color[i].g;
		wd->color[i].b = color[i].b;
	}

	_color_table_update(obj, wd->row, wd->col, color_num, wd->color);
	_sizing_eval(obj);
}

/**
 * Set row/column value for the colorpalette.
 *
 * @param obj   Colorpalette object
 * @param row   row value for the colorpalette
 * @param col   column value for the colorpalette
 *
 * @ingroup Colorpalette
 */
EAPI void elm_colorpalette_row_column_set(Evas_Object *obj, int row, int col)
{
	Widget_Data *wd = elm_widget_data_get(obj);

	if (!wd) return ;

	_color_table_update(obj, row, col, wd->num, wd->color);
	_sizing_eval(obj);
}

