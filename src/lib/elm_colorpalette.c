#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

/**
 * @defgroup Colorpalette Colorpalette
 * @ingroup Elementary
 *
 * Using colorpalette, you can select a color by clicking
 * a color rectangle on the colorpalette.
 *
 * Smart callbacks that you can add are:
 *
 * clicked - This signal is sent when a color rectangle is clicked.
 *
 */

#define MAX_NUM_COLORS 30

typedef struct _Colorpalette_Item Colorpalette_Item;
typedef struct _Widget_Data Widget_Data;

struct _Colorpalette_Item
{
   Evas_Object *obj;
   Evas_Object *lo;
   Evas_Object *cr;
   unsigned int r, g, b;
};

struct _Widget_Data
{
   Evas_Object *base;
   Evas_Object *tab;
   Elm_Colorpalette_Color *color;
   Eina_List *items;
   unsigned int row, col;
   unsigned int num;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _colorpalette_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _color_select_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _color_release_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _color_table_delete(Evas_Object *obj);
static void _color_table_update(Evas_Object *obj, unsigned int row, unsigned int col, unsigned int color_num, Elm_Colorpalette_Color *color);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _color_table_delete(obj);
   if (wd->color)
     {
        free(wd->color);
     }
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   _elm_theme_object_set(obj, wd->base, "colorpalette", "base", elm_widget_style_get(obj));
   _color_table_update(obj, wd->row, wd->col, wd->num, wd->color);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = NULL;
   Colorpalette_Item *item = NULL;
   Evas_Coord tab_w = 0, tab_h = 0;
   Evas_Coord pad_x = 0, pad_y = 0;
   Evas_Coord rect_w = 0, rect_h = 0;
   Evas_Coord minw = -1, minh = -1;

   wd = elm_widget_data_get((Evas_Object *)obj);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(wd->base, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);

   edje_object_part_geometry_get(wd->base,"bg" ,NULL, NULL, &tab_w, &tab_h);
   if (wd->items)
     item = wd->items->data;
   edje_object_part_geometry_get(elm_layout_edje_get(item->lo),"bg" ,NULL, NULL, &rect_w, &rect_h);
   if (wd->col - 1) /*value cannot be 0 else divide by zero error will cause floating point exception*/
     pad_x = (tab_w - (rect_w * wd->col)) / (wd->col - 1);
   if (wd->row - 1) /*value cannot be 0 else divide by zero error will cause floating point exception*/
     pad_y = (tab_h - (rect_h * wd->row)) / (wd->row - 1);
   elm_table_padding_set(wd->tab, pad_x, pad_y);
   elm_table_homogeneous_set(wd->tab, EINA_TRUE);
}

static void _colorpalette_object_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void _color_select_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Colorpalette_Color *color;
   Colorpalette_Item *item = (Colorpalette_Item *) data;
   color = ELM_NEW(Elm_Colorpalette_Color);
   if (!color) return;
   color->r = item->r;
   color->g = item->g;
   color->b = item->b;
   evas_object_smart_callback_call(item->obj, "clicked", color);
   edje_object_signal_emit(elm_layout_edje_get(item->lo), "focus_visible", "elm");
}

static void _color_release_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Colorpalette_Item *item = (Colorpalette_Item *) data;
   edje_object_signal_emit(elm_layout_edje_get(item->lo), "focus_invisible", "elm");
}

static void _color_table_delete(Evas_Object *obj)
{
   Widget_Data *wd = NULL;
   Colorpalette_Item *item;
   wd = elm_widget_data_get(obj);

   if (wd->items)
     {
        EINA_LIST_FREE(wd->items, item)
          {
             if (item->lo)
               {
                  evas_object_del(item->lo);
                  item->lo = NULL;
               }
             if (item->cr)
               {
                  evas_object_del(item->cr);
                  item->cr = NULL;
               }
             free(item);
          }
     }
   if (wd->tab) evas_object_del(wd->tab);
}

static void _color_table_update(Evas_Object *obj, unsigned int row, unsigned int col, unsigned int color_num, Elm_Colorpalette_Color *color)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Colorpalette_Item *item;
   Evas_Object *lo;
   Evas_Object *cr;
   unsigned int i, j, count;

   _color_table_delete(obj);
   count = 0;
   wd->row = row;
   wd->col = col;
   wd->num = color_num;

   wd->tab = elm_table_add(obj);
   evas_object_size_hint_weight_set(wd->tab, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->tab, EVAS_HINT_FILL, EVAS_HINT_FILL);
   edje_object_part_swallow(wd->base, "palette", wd->tab);

   for ( i = 0 ; i < row ; i++)
     {
        for ( j = 0 ; j < col ; j++ )
          {
             item = ELM_NEW(Colorpalette_Item);
             if (item)
               {
                  lo = elm_layout_add(obj);
                  elm_layout_theme_set(lo, "colorpalette", "base", "bg");
                  evas_object_size_hint_weight_set(lo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                  evas_object_size_hint_align_set(lo, EVAS_HINT_FILL, EVAS_HINT_FILL);
                  evas_object_show(lo);
                  elm_table_pack(wd->tab, lo, (int)j, (int)i, 1, 1);
                  item->obj = obj;
                  item->lo = lo;
                  if (count < color_num)
                    {
                       cr =  edje_object_add(evas_object_evas_get(obj));
                       _elm_theme_object_set(obj, cr, "colorpalette", "base", "color");
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

static int
_hex_string_get(char ch)
{
   if ((ch >= '0') && (ch <= '9')) return (ch - '0');
   else if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
   else if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
   return 0;
}

static void
_color_parse(const char *str, unsigned char *r, unsigned char *g, unsigned char *b)
{
   int slen;

   slen = strlen(str);
   *r = *g = *b = 0;

   if (slen == 7) /* #RRGGBB */
     {
        *r = (_hex_string_get(str[1]) << 4) | (_hex_string_get(str[2]));
        *g = (_hex_string_get(str[3]) << 4) | (_hex_string_get(str[4]));
        *b = (_hex_string_get(str[5]) << 4) | (_hex_string_get(str[6]));
     }
   *r = (*r * 0xff) / 255;
   *g = (*g * 0xff) / 255;
   *b = (*b * 0xff) / 255;
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
   Eina_List *colors;
   const Eina_List *l;
   const char *color_code;
   const char *rowstr, *colstr, *color_numstr;
   int color_num = 10;
   int row = 2;
   int col = 5;
   int index = 0;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "colorpalette");
   elm_widget_type_set(obj, "colorpalette");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);

   wd->base = edje_object_add(e);
   _elm_theme_object_set(obj, wd->base, "colorpalette", "base", "default");
   elm_widget_resize_object_set(obj, wd->base);

   color_numstr = edje_object_data_get(wd->base, "color_num");
   if (color_numstr) color_num = atoi(color_numstr); /* Need to get this value
   from edc inorder to allocate memory in advance: Think of a better way*/
   rowstr = edje_object_data_get(wd->base, "row");
   if (rowstr) row = atoi(rowstr);
   colstr = edje_object_data_get(wd->base, "col");
   if (colstr) col = atoi(colstr);
   wd->color = (Elm_Colorpalette_Color*) calloc (color_num, sizeof(Elm_Colorpalette_Color));
   colors = elm_widget_stringlist_get(edje_object_data_get(wd->base, "colors"));
   EINA_LIST_FOREACH(colors, l, color_code)
     {
        unsigned char r, g, b;
        /*Optimized color parsing algorithm*/
        _color_parse(color_code, &r, &g, &b);
        /*TODO: Make color storing structure and item also take unsigned char*/
        wd->color[index].r = (unsigned int)r;
        wd->color[index].g = (unsigned int)g;
        wd->color[index].b = (unsigned int)b;
        index++;
     }
   elm_widget_stringlist_free(colors);
   _color_table_update(obj, row, col, color_num, wd->color);
   evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE, _colorpalette_object_resize, obj);
   _sizing_eval(obj);
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
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int i;

   if (color_num > MAX_NUM_COLORS) return;
   if (!wd) return;

   if (wd->color)
     {
        free(wd->color);
        wd->color = NULL;
     }
   wd->color = (Elm_Colorpalette_Color*) calloc (color_num, sizeof(Elm_Colorpalette_Color));
   for (i = 0; i < color_num; i++)
     {
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
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   /*Row can be incomplete with atleast one color*/
   if(((row-1)*col) > MAX_NUM_COLORS) return;
   _color_table_update(obj, row, col, wd->num, wd->color);
   _sizing_eval(obj);
}

