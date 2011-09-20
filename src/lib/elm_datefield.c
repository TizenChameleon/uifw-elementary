#include <locale.h>
#include <unicode/udat.h>
#include <unicode/ucal.h>
#include <unicode/putil.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#include <Elementary.h>
#include "elm_priv.h"
#ifdef HAVE_ELEMENTARY_X
# include <langinfo.h>
#else
# include <evil_langinfo.h>
#endif

/**
 * @defgroup Datefield Datefield
 * @ingroup Elementary
 *
 * This is a date edit field. it is used to input date and time using
 * diskselector integrated ctxpopup.
 */

typedef struct _Widget_Data Widget_Data;

#define DATEFIELD_TYPE_COUNT        6
#define BUFFER_SIZE                 64
#define MAX_FORMAT_LEN              32
#define MAX_SEPARATOR_LEN           3
#define MAX_ITEM_FORMAT_LEN         6
#define DISKSELECTOR_ITEMS_NUM_MIN  4

// Interface between EDC & C code. Item names & signal names.
// Values 0 to 6 are valid range, can be substituted for %d.
#define EDC_DATEFIELD_ENABLE_SIG_STR        "elm,state,enabled"
#define EDC_DATEFIELD_DISABLE_SIG_STR       "elm,state,disabled"
#define EDC_DATEFIELD_FOCUSIN_SIG_STR       "elm,action,focus"
#define EDC_DATEFIELD_FOCUSOUT_SIG_STR      "elm,action,unfocus"
#define EDC_PART_ITEM_STR                   "item%d"
#define EDC_PART_SEPARATOR_STR              "separator%d"
#define EDC_PART_ITEM_OVER_STR              "item%d.over"
#define EDC_PART_ITEM_ENABLE_SIG_STR        "item%d,enable"
#define EDC_PART_ITEM_DISABLE_SIG_STR       "item%d,disable"
#define EDC_PART_ITEM_FOCUSIN_SIG_STR       "item%d,focus,in"
#define EDC_PART_ITEM_FOCUSOUT_SIG_STR      "item%d,focus,out"
#define EDC_PART_ITEM_STYLE_DEFAULT_SIG_STR "item%d,style,default"
#define EDC_PART_ITEM_STYLE_AMPM_SIG_STR    "item%d,style,ampm"

typedef struct _ICU_Format
{
   Elm_Datefield_ItemType type;
   UChar fmt_char[2];
   const char *def_sep;
   UDateFormatField icu_type;
   int def_min;
   int def_max;
}ICU_Format_Map;

static const UChar icu_fmt_chars[] = {'y', 'M', 'd', 'h', 'H',
                                      'm', 's', 'a', 'z', 'E',
                                      'Q', 'L', 'v'};

//-1 denotes, get the value from ICU dynamically.
static const ICU_Format_Map mapping[DATEFIELD_TYPE_COUNT] = {
   { ELM_DATEFIELD_YEAR,   {'y',0},   "",  UDAT_YEAR_FIELD,         1970, 2037},
   { ELM_DATEFIELD_MONTH,  {'M',0},   "",  UDAT_MONTH_FIELD,        0,    11 },
   { ELM_DATEFIELD_DATE,   {'d',0},   "",  UDAT_DATE_FIELD,         1,    -1 },
   { ELM_DATEFIELD_HOUR,   {'H','h'}, ":", UDAT_HOUR_OF_DAY0_FIELD, -1,   -1 },
   { ELM_DATEFIELD_MINUTE, {'m',0},   " ", UDAT_MINUTE_FIELD,       0,    59 },
   { ELM_DATEFIELD_AMPM,   {'a',0},   " ", UDAT_AM_PM_FIELD,        0,    1 }
};

typedef struct _Datefield_Item
{
   UChar format[MAX_ITEM_FORMAT_LEN]; //format being set for the current item
   const char *content; //string to be displayed
   const char *separator;
   int location; //location of the item as per the current format
   int value;
   int min, max;
   int default_min, default_max;
   UDateFormatField type; //type of the item
   UDateFormatField sub_type; //mainly used for Time, whether 12hr/24Hr format
   Eina_Bool fmt_exist:1; //if item format is present or not
   Eina_Bool enabled:1; //if item is to be shown or not
   Eina_Bool abs_min:1;
   Eina_Bool abs_max:1;
} Datefield_Item;

struct _Widget_Data
{
   Evas_Object *base;
   Datefield_Item *item_list; //Fixed set of items, so no Eina list.
   Evas_Object *ctxpopup;
   Datefield_Item *selected_it;
   UChar format[MAX_FORMAT_LEN];
   UCalendar *calendar;
   UDateFormat* date_fmt; //ICU date format
   UDateFormatStyle date_style;
   UDateFormatStyle time_style;
   Eina_Bool user_format:1; //whether user set format or the default format.

   //////////////////////DEPRECATED//////////////////////
   unsigned int datefield_layout; // user set layout
   const char *old_style_format; // user set format
   Eina_Bool time_mode; // current time mode 12hr/24hr
   //////////////////////////////////////////////////////
};

typedef struct _DiskItem_Data
{
   Evas_Object *datefield;
   unsigned int sel_item_value;
} DiskItem_Data;

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _on_focus_hook(void *data __UNUSED__, Evas_Object *obj);
static void _disable_hook(Evas_Object *obj);
static void _mirrored_set(Evas_Object *obj, Eina_Bool rtl);
static void _sizing_eval(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj __UNUSED__,
                                   void *event_info __UNUSED__);
static void _datefield_resize_cb(void *data, Evas *e __UNUSED__,Evas_Object *obj
                                 __UNUSED__, void *event_info __UNUSED__);
static void _datefield_move_cb(void *data, Evas *e __UNUSED__,Evas_Object *obj
                                 __UNUSED__, void *event_info __UNUSED__);
static void _contextual_field_limit_get(Evas_Object * obj, Datefield_Item * it,
                 UDateFormatField udtype, int * range_min, int * range_max );
static void _update_items(Evas_Object *obj);
static void _field_value_set(Evas_Object * obj, UDateFormatField icu_type,
                             int value, Eina_Bool adjust_time);
static void _diskselector_cb(void *data, Evas_Object *obj __UNUSED__,
                             void *event_info __UNUSED__);
static void _ampm_clicked (void *data);
static void _diskselector_item_free_cb(void *data, Evas_Object *obj __UNUSED__,
                                       void *event_info __UNUSED__);
static void _load_field_options(Evas_Object * data, Evas_Object *diskselector,
                                Datefield_Item *it);
static void _datefield_clicked_cb(void *data, Evas_Object *obj __UNUSED__,
                     const char *emission __UNUSED__, const char *source);
static void _format_reload(Evas_Object *obj);
static void _item_list_init(Evas_Object *obj);

static const char SIG_CHANGED[] = "changed";
static const Evas_Smart_Cb_Description _signals[] = {
       {SIG_CHANGED, ""},
       {NULL, NULL}
};

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *tmp;
   unsigned int idx;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        tmp = wd->item_list + idx;
        if (tmp->content)  eina_stringshare_del(tmp->content);
        if (tmp->separator) eina_stringshare_del(tmp->separator);
     }
   free( wd->item_list );
   ucal_close(wd->calendar);
   wd->calendar = NULL;
   udat_close(wd->date_fmt);
   wd->date_fmt = NULL;
   evas_object_del(wd->ctxpopup);

   free(wd);
}

static void
_disable_hook(Evas_Object *obj)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->base) return;
   if (elm_widget_disabled_get(obj))
     {
        evas_object_hide(wd->ctxpopup);
        edje_object_signal_emit(wd->base, EDC_DATEFIELD_DISABLE_SIG_STR,"elm");
     }
   else
     edje_object_signal_emit(wd->base, EDC_DATEFIELD_ENABLE_SIG_STR, "elm");
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd ) return;
   if (elm_widget_focus_get(obj))
      edje_object_signal_emit(wd->base, EDC_DATEFIELD_FOCUSIN_SIG_STR, "elm");
   else
      edje_object_signal_emit(wd->base, EDC_DATEFIELD_FOCUSOUT_SIG_STR, "elm");
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
   evas_object_hide(wd->ctxpopup);
   edje_object_mirrored_set(wd->base, rtl);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd;
   Evas_Coord minw = -1, minh = -1;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->base) return;
   edje_object_size_min_calc(wd->base, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   int idx;
   //Evas_Object *diskselector;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->base) return;
   _elm_theme_object_set(obj, wd->base, "datefield", "base",
                         elm_widget_style_get(obj));
   _elm_widget_mirrored_reload(obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   snprintf(buf, sizeof(buf), "datefield/%s", elm_object_style_get(obj));
   elm_object_style_set(wd->ctxpopup, buf);
   /*//Enabled once elm_object_content_get() API comes to git.
   if (diskselector = elm_object_content_get(wd->ctxpopup))
   elm_object_style_set(diskselector, buf);*/
   edje_object_scale_set(wd->base,elm_widget_scale_get(obj)*_elm_config->scale);

   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(wd->base, EDC_DATEFIELD_DISABLE_SIG_STR,"elm");
   else
     edje_object_signal_emit(wd->base, EDC_DATEFIELD_ENABLE_SIG_STR, "elm");

   for (idx= 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        if (it->fmt_exist && it->enabled )
          {
             snprintf(buf, sizeof(buf), EDC_PART_ITEM_STR, it->location);
             edje_object_part_text_set(wd->base, buf, it->content);
             snprintf(buf, sizeof(buf), EDC_PART_SEPARATOR_STR, it->location);
             edje_object_part_text_set(wd->base, buf, it->separator);
             snprintf(buf, sizeof(buf), EDC_PART_ITEM_ENABLE_SIG_STR,
                      it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
             if (it->type == UDAT_AM_PM_FIELD)
               snprintf(buf, sizeof(buf), EDC_PART_ITEM_STYLE_AMPM_SIG_STR,
                        it->location);
             else
               snprintf(buf, sizeof(buf), EDC_PART_ITEM_STYLE_DEFAULT_SIG_STR,
                        it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
          }
        else
          {
             snprintf(buf, sizeof(buf),EDC_PART_ITEM_DISABLE_SIG_STR,
                      it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
          }
     }
   edje_object_message_signal_process(wd->base);
   _sizing_eval(obj);
}

static void
_ctxpopup_dismissed_cb(void *data, Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   Widget_Data *wd;
   Evas_Object *diskselector;
   char buf[BUFFER_SIZE];

   wd = elm_widget_data_get(data);
   if (!wd || !wd->base) return;
   diskselector = elm_ctxpopup_content_unset(wd->ctxpopup);
   if (diskselector) evas_object_del(diskselector);

   if (wd->selected_it)
     {
        snprintf(buf, sizeof(buf), EDC_PART_ITEM_FOCUSOUT_SIG_STR,
                 wd->selected_it->location);
        edje_object_signal_emit(wd->base, buf, "elm");
        wd->selected_it = NULL;
     }
}

static void
_datefield_resize_cb(void *data, Evas *e __UNUSED__,Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_hide(wd->ctxpopup);
}

static void
_datefield_move_cb(void *data, Evas *e __UNUSED__,Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Widget_Data *wd;

   wd = elm_widget_data_get(data);
   if (!wd) return;
   evas_object_hide(wd->ctxpopup);
}

static void
_contextual_field_limit_get(Evas_Object * obj, Datefield_Item * it,
                    UDateFormatField udtype, int *range_min, int *range_max)
{
   Widget_Data *wd;
   Datefield_Item * tmp;
   int idx,icu_min, icu_max;
   UErrorCode status = U_ZERO_ERROR;
   Eina_Bool min_limit = EINA_TRUE;
   Eina_Bool max_limit = EINA_TRUE;

   wd = elm_widget_data_get(obj);
   if (!wd || !it) return;
   //Top down check all the fields until current field for Min/Max limit
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++)
     {
        tmp = wd->item_list + idx;
        if (tmp->type == it->type) break;

        if (max_limit && ((int)tmp->value < tmp->max)) max_limit = EINA_FALSE;
        if (min_limit && ((int)tmp->value > tmp->min)) min_limit = EINA_FALSE;
     }

   if (it->abs_min || min_limit) (*range_min) = it->min;
   else (*range_min) = it->default_min;

   if (it->abs_max || max_limit) (*range_max) = it->max;
   else (*range_max) = it->default_max;

   icu_min = ucal_getLimit( wd->calendar, udat_toCalendarDateField(udtype),
                           UCAL_ACTUAL_MINIMUM, &status );
   icu_max = ucal_getLimit( wd->calendar, udat_toCalendarDateField(udtype),
                           UCAL_ACTUAL_MAXIMUM, &status );

   if ((*range_min == -1) || (*range_min < icu_min))
     *range_min = icu_min;
   if ((*range_max == -1) || (*range_max > icu_max))
     *range_max = icu_max;
}

static void
_update_items(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   int idx= 0;
   UDate date;
   UChar result[BUFFER_SIZE];
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->base) return;
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++)
     {
        it = wd->item_list + idx;
        ucal_set (wd->calendar, udat_toCalendarDateField(it->type), it->value);
     }
   date = ucal_getMillis (wd->calendar, &status);
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        if ( it->fmt_exist && it->enabled )
          {
             udat_applyPattern(wd->date_fmt, TRUE, it->format,
                               u_strlen(it->format));
             udat_format(wd->date_fmt, date, result, BUFFER_SIZE, NULL, &status);
             u_strToUTF8(buf, BUFFER_SIZE, 0, result, -1, &status);
             if (it->content) eina_stringshare_del(it->content);
             it->content = eina_stringshare_add(buf);
             snprintf(buf, sizeof(buf), EDC_PART_ITEM_STR, it->location);
             edje_object_part_text_set(wd->base, buf, it->content);
             snprintf(buf, sizeof(buf), EDC_PART_SEPARATOR_STR, it->location+1);
             edje_object_part_text_set(wd->base, buf, it->separator);
          }
     }
}

static void
_field_value_set(Evas_Object * obj, UDateFormatField icu_type, int value, Eina_Bool adjust_time)
{
   Widget_Data *wd;
   Datefield_Item * it, *it2 = NULL;
   int idx, min, max;
   UErrorCode status = U_ZERO_ERROR;
   Eina_Bool validate = EINA_FALSE;
   Eina_Bool value_changed = EINA_FALSE;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
   //Validate & reset lower order fields
   for ( idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        if (mapping[idx].icu_type == icu_type) validate = EINA_TRUE;
        if (!validate) continue;

        it = wd->item_list + idx;
        _contextual_field_limit_get(obj, it, it->type, &min, &max);

        if (it->type == icu_type ) //Set the value now
          {
            // Updating time may alter AM/PM and viceversa. Read back from ICU.
             if (it->type == UDAT_HOUR_OF_DAY0_FIELD )
               {
                  it2 = wd->item_list + ELM_DATEFIELD_AMPM;
                  //12 hr format & PM then add 12 to value.
                  if (it->sub_type == UDAT_HOUR1_FIELD && adjust_time &&
                           it2->value && value < 12)
                     value += 12;
               }
             else if (it->type == UDAT_AM_PM_FIELD )
                  it2 = wd->item_list + ELM_DATEFIELD_HOUR;

             if (value < min) value = min;
             else if (value > max) value = max;
             if (it->value == value) continue;
             it->value = value;
             value_changed = EINA_TRUE;
             ucal_set(wd->calendar, udat_toCalendarDateField (it->type),
                     it->value);
             //Read back AM/PM or hour value. update based new 24hr format time value.
             if (it2)
               it2->value = ucal_get(wd->calendar,
                            udat_toCalendarDateField(it2->type),&status);
          }
        else
          {
             //Validate current value against context based Min/Max restriction.
             if (it->value < min)
               {
                  it->value = min;
                  value_changed = EINA_TRUE;
               }
             else if (it->value > max)
               {
                  it->value = max;
                  value_changed = EINA_TRUE;
               }
             ucal_set(wd->calendar, udat_toCalendarDateField (it->type),
                       it->value);
          }
     }
   _update_items(obj);

   if (value_changed)
     evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
}

static void
_diskselector_cb(void *data, Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   DiskItem_Data *cb_data;
   Widget_Data *wd;

   cb_data = (DiskItem_Data *)data;
   if (!cb_data) return;
   wd = elm_widget_data_get(cb_data->datefield);
   if (!wd ) return;
   _field_value_set(cb_data->datefield, wd->selected_it->type,
                    cb_data->sel_item_value, EINA_TRUE);
   evas_object_hide(wd->ctxpopup);
}

static void
_ampm_clicked (void *data)
{
   Widget_Data *wd;
   Datefield_Item * it;
   char buf[BUFFER_SIZE];
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(data);
   if (!wd || !wd->base) return;
   wd->selected_it->value = !wd->selected_it->value;
   ucal_set(wd->calendar, udat_toCalendarDateField(wd->selected_it->type),
            wd->selected_it->value );

   //Read back hour according to updated AM/PM
   it = wd->item_list + ELM_DATEFIELD_HOUR;
   it->value = ucal_get(wd->calendar, udat_toCalendarDateField(it->type),
                        &status );
   snprintf(buf, sizeof(buf), EDC_PART_ITEM_FOCUSOUT_SIG_STR,
            wd->selected_it->location);
   edje_object_signal_emit(wd->base, buf, "elm");
   wd->selected_it = NULL;
   _update_items(data);
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
}

static void
_diskselector_item_free_cb(void *data, Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   if (data) free(data);
}

static void
_load_field_options(Evas_Object * data, Evas_Object *diskselector,
                    Datefield_Item *it)
{
   Widget_Data *wd;
   Datefield_Item *it2, *it3;
   DiskItem_Data *disk_data;
   Elm_Diskselector_Item *item;
   int idx, min, max, selected_val;
   int text_len, max_len = 0;
   char item_label[BUFFER_SIZE];
   UCalendarDateFields  cfield;
   UChar result[BUFFER_SIZE];
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(data);
   if (!wd) return;
   //Apply field format to Date Format Object
   udat_applyPattern(wd->date_fmt, TRUE, it->format,
                      u_strlen(it->format));
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it3 = wd->item_list + idx;
        // To avoid month roll over based on date value, set to min value.
        if (it3->type == UDAT_DATE_FIELD)
          ucal_set (wd->calendar, udat_toCalendarDateField (it3->type), 1);
        else
          ucal_set (wd->calendar, udat_toCalendarDateField (it3->type), it3->value);
     }
   _contextual_field_limit_get(data, it, it->sub_type, &min, &max );

   cfield = udat_toCalendarDateField (it->type);
   selected_val = it->value;
   if (it->sub_type == UDAT_HOUR1_FIELD)
     {
        it2 = wd->item_list + ELM_DATEFIELD_AMPM;
        if (it2->value) selected_val -= 12;  // if 12hr format & PM, reduce 12
     }

   for (idx = min; idx <= max; idx++)
     {
        ucal_set (wd->calendar, cfield, idx);
        udat_format(wd->date_fmt, ucal_getMillis (wd->calendar, &status),
                    result, BUFFER_SIZE, NULL, &status);
        u_strToUTF8(item_label, BUFFER_SIZE, &text_len, result, -1, &status);
        if (text_len > max_len ) max_len = text_len; //Store max. label length

        if (idx == selected_val) //Selected Item, don't attach a callback to handle
          {
             item = elm_diskselector_item_append(diskselector, item_label,
                                                 NULL, NULL, NULL);
             elm_diskselector_item_selected_set(item, EINA_TRUE);
          }
        else
          {
             disk_data = (DiskItem_Data *) malloc (sizeof(DiskItem_Data));
             disk_data->datefield = data;
             disk_data->sel_item_value = idx;
             item = elm_diskselector_item_append(diskselector,
                                 item_label, NULL, _diskselector_cb, disk_data);
             elm_diskselector_item_del_cb_set(item, _diskselector_item_free_cb);
          }
     }
   elm_diskselector_side_label_length_set(diskselector, max_len);
}

static void
_datefield_clicked_cb(void *data, Evas_Object *obj __UNUSED__,
                      const char *emission __UNUSED__, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Object *diskselector;
   const Evas_Object *edj_part;
   char buf[BUFFER_SIZE];
   int idx = 0, idx1 = 0, display_item_num;
   Evas_Coord x = 0, y = 0, w = 0, h = 0;
   Evas_Coord disksel_width;

   if (!wd || !wd->base) return;
   if (elm_widget_disabled_get(data)) return;

   wd->selected_it = NULL;
   //Locate the selected Index & Selected Datefield_Item
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        snprintf(buf, sizeof(buf), EDC_PART_ITEM_OVER_STR, idx);
        if (!strncmp(buf, source, sizeof(buf)))
          {
             for (idx1 = 0; idx1 < DATEFIELD_TYPE_COUNT; idx1++ )
               {
                 if ((wd->item_list + idx1)->location == (int)idx)
                   {
                      wd->selected_it = wd->item_list + idx1;
                      break;
                   }
               }
             break;
          }
     }

   if ( !wd->selected_it || !wd->selected_it->fmt_exist
                         || !wd->selected_it->enabled ) return;
   snprintf(buf, sizeof(buf), EDC_PART_ITEM_FOCUSIN_SIG_STR,
            wd->selected_it->location);
   edje_object_signal_emit(wd->base, buf, "elm");

   if ( wd->selected_it->type == UDAT_AM_PM_FIELD )
     {
        _ampm_clicked (data);
        return;
     }

   //Recreating diskselector everytime due to diskselector behavior
   diskselector = elm_diskselector_add(elm_widget_top_get(data));
   snprintf(buf, sizeof(buf), "datefield/%s", elm_object_style_get(data));
   elm_object_style_set(diskselector, buf);

   //Load the options list
   _load_field_options(data, diskselector, wd->selected_it);

   elm_ctxpopup_content_set(wd->ctxpopup, diskselector);
   snprintf(buf,sizeof(buf), EDC_PART_ITEM_OVER_STR, wd->selected_it->location);
   edj_part = edje_object_part_object_get(wd->base, buf);
   evas_object_geometry_get(edj_part, &x, &y, &w, &h);
   evas_object_move(wd->ctxpopup, (x+w/2), (y+h));
   if (elm_ctxpopup_direction_get (wd->ctxpopup) == ELM_CTXPOPUP_DIRECTION_DOWN)
     evas_object_move(wd->ctxpopup, (x+w/2), y);
   evas_object_show(wd->ctxpopup);

   evas_object_geometry_get(diskselector, NULL, NULL, &disksel_width, NULL);
   display_item_num = disksel_width / (w +  elm_finger_size_get()/2);
   //odd number of items leads to auto selection.
   //making as event number of item to prevent auto selection.
   if (display_item_num%2) display_item_num-=1;
   if (display_item_num < DISKSELECTOR_ITEMS_NUM_MIN)
      display_item_num = DISKSELECTOR_ITEMS_NUM_MIN;

   elm_diskselector_display_item_num_set(diskselector, display_item_num);
   elm_diskselector_round_set(diskselector, EINA_TRUE);
}

static void
_parse_separator( const UChar *fmt, UChar *separator, unsigned int len )
{
   unsigned int i = 0;
   while( (*fmt) && (i < len-2))
    {
     if ( !u_strchr(icu_fmt_chars, *fmt) && !u_isspace(*fmt))
         separator[i++] = *fmt++;
      else  break;
    }
   separator[i] = 0;
}

static unsigned int
_parse_format( Evas_Object *obj )
{
   Widget_Data *wd;
   unsigned int len, idx, location = 0;
   Datefield_Item *it;
   UChar *fmt_ptr;
   UChar cur;
   UChar usep[MAX_SEPARATOR_LEN];
   char separator[2*MAX_SEPARATOR_LEN];
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(obj);
   fmt_ptr = wd->format;
   while( (cur = *fmt_ptr ) )
     {
      len = 0;
      for ( idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
       {
          if (mapping[idx].fmt_char[0] == cur ||
              mapping[idx].fmt_char[1] == cur)
            {
               while( *(fmt_ptr+len) == cur ) len++;
               fmt_ptr += len;
               break;
            }
       }

      if (!len)
        {
           fmt_ptr++;
           continue;
        }

      it = wd->item_list + idx;
      // ignore the items already have or disabled
      // valid formats, means already parsed, repeated format, ignore.
      if (!it->enabled || it->location != -1) continue;

      // In default format, Months larger than MMM are restricted.
      if ((!wd->user_format) && (it->type == UDAT_MONTH_FIELD) && (len > 3))
         len = 3;

      u_memset(it->format, cur, len);
      it->format[len] = 0; //NULL terminating

     //set the hour display format 12h/24h
      if (it->type == UDAT_HOUR_OF_DAY0_FIELD)
        {
           if (cur == 'H')  it->sub_type = UDAT_HOUR_OF_DAY0_FIELD;
           else if (cur == 'h')  it->sub_type = UDAT_HOUR1_FIELD;
        }

      it->fmt_exist = EINA_TRUE;
      it->location = location++;
      if (!strncmp(mapping[idx].def_sep,"",MAX_SEPARATOR_LEN))
        {
           _parse_separator(fmt_ptr, usep, MAX_SEPARATOR_LEN );
           u_strToUTF8(separator, BUFFER_SIZE, 0, usep, -1, &status);
           if (it->separator) eina_stringshare_del(it->separator);
           it->separator = eina_stringshare_add(separator);
       }
   }
   //Return the number of valid items parsed.
   return location;
}

static void
_format_reload(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   UChar timezone[BUFFER_SIZE];
   UErrorCode status = U_ZERO_ERROR;
   const char *locale;
   unsigned int idx, location;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
    // fetch the format from ICU, if not user format
   if (!wd->user_format )
     {
        if ( wd->date_fmt ) udat_close( wd->date_fmt );
        locale = uloc_getDefault();
        ucal_getDefaultTimeZone(timezone, BUFFER_SIZE, &status);
        wd->date_fmt = udat_open(wd->time_style, wd->date_style, locale,
                                timezone, -1, NULL, -1, &status);
        udat_toPattern(wd->date_fmt, 0, wd->format, BUFFER_SIZE, &status);
     }

   //reset all the items to disable state
   for ( idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        if (it->content) eina_stringshare_del(it->content);
        it->content = NULL;
        it->fmt_exist = EINA_FALSE;
        it->location = -1;
     }
   location = _parse_format( obj );

   //assign locations to disabled fields for uniform usage
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++)
     {
        it = wd->item_list + idx;
        if (it->location == -1) it->location = location++;

        if (it->fmt_exist && it->enabled)
          {
             snprintf(buf, sizeof(buf), EDC_PART_ITEM_ENABLE_SIG_STR,
                      it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
             if (it->type == UDAT_AM_PM_FIELD)
               snprintf(buf, sizeof(buf), EDC_PART_ITEM_STYLE_AMPM_SIG_STR,
                        it->location);
             else
               snprintf(buf, sizeof(buf), EDC_PART_ITEM_STYLE_DEFAULT_SIG_STR,
                        it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
          }
        else
          {
             snprintf(buf, sizeof(buf),EDC_PART_ITEM_DISABLE_SIG_STR,
                      it->location);
             edje_object_signal_emit(wd->base, buf, "elm");
          }
     }
   edje_object_message_signal_process(wd->base);

   wd->time_mode = ((wd->item_list + ELM_DATEFIELD_HOUR)->sub_type
                        == UDAT_HOUR1_FIELD);

   _update_items(obj);
}

static void
_item_list_init(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   UChar timezone[BUFFER_SIZE];
   UErrorCode status = U_ZERO_ERROR;
   const char *locale=NULL;
   unsigned int idx;

   wd = elm_widget_data_get(obj);
   if (!wd) return;
   ucal_getDefaultTimeZone(timezone, BUFFER_SIZE, &status);
   locale = uloc_getDefault();
   wd->calendar = ucal_open (timezone, -1, locale, UCAL_DEFAULT, &status);
   wd->date_fmt = udat_open( wd->time_style, wd->date_style, locale, timezone,
                             -1, NULL, -1, &status);

   wd->item_list = calloc(1, DATEFIELD_TYPE_COUNT * sizeof(Datefield_Item));
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++)
     {
        it = wd->item_list + idx;
        it->type = mapping[idx].icu_type;
        it->sub_type = mapping[idx].icu_type;
        it->fmt_exist = EINA_FALSE;
        it->enabled  = EINA_TRUE;
        it->min = mapping[idx].def_min;
        it->default_min = mapping[idx].def_min;
        it->max = mapping[idx].def_max;
        it->default_max = mapping[idx].def_max;
        it->separator = strdup(mapping[idx].def_sep);

        //get the default value from Calendar
        it->value = ucal_get(wd->calendar, udat_toCalendarDateField(it->type),
                             &status );

        snprintf(buf, sizeof(buf), EDC_PART_ITEM_OVER_STR, idx);
        edje_object_signal_callback_add(wd->base, "mouse,clicked,1", buf,
                                        _datefield_clicked_cb, obj);
     }
}

/**
 * @brief Add a new datefield Widget
 * The date format and strings are based on current locale
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Datefield
 */
EAPI Evas_Object *
elm_datefield_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "datefield");
   elm_widget_type_set(obj, widtype);
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
   elm_widget_disable_hook_set(obj, _disable_hook);
   elm_widget_can_focus_set(obj, EINA_TRUE);

   wd->base = edje_object_add(e);
   elm_widget_resize_object_set(obj, wd->base);
   _elm_theme_object_set(obj, wd->base, "datefield", "base", "default");
   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   wd->time_style = UDAT_LONG;
   wd->date_style = UDAT_LONG;
   _item_list_init(obj);
   _format_reload(obj);

   wd->datefield_layout = ELM_DATEFIELD_LAYOUT_DATEANDTIME;
   wd->time_mode = EINA_TRUE;
   wd->old_style_format = "mmddyy";

   wd->ctxpopup = elm_ctxpopup_add(elm_widget_top_get(obj));
   elm_object_style_set(wd->ctxpopup, "datefield/default");
   elm_ctxpopup_horizontal_set(wd->ctxpopup, EINA_TRUE);
   elm_ctxpopup_direction_priority_set(wd->ctxpopup,ELM_CTXPOPUP_DIRECTION_DOWN,
               ELM_CTXPOPUP_DIRECTION_UP,ELM_CTXPOPUP_DIRECTION_LEFT,
               ELM_CTXPOPUP_DIRECTION_RIGHT);
   evas_object_size_hint_weight_set(wd->ctxpopup, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->ctxpopup, EVAS_HINT_FILL,EVAS_HINT_FILL);
   evas_object_smart_callback_add(wd->ctxpopup, "dismissed",
                                  _ctxpopup_dismissed_cb, obj);
   evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE,
                                  _datefield_resize_cb, obj);
   evas_object_event_callback_add(wd->base, EVAS_CALLBACK_MOVE,
                                  _datefield_move_cb, obj);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   return obj;
}

/**
 * Set the format of datefield
 * Maximum allowed format length is 32 chars.
 * Default value is taken according to the system locale format.
 * The Caller has to free the memory of input format string.
 *
 * @param[in] obj The datefield object
 * @param[in] fmt The date format, ex) MMMddy hh:mm.
 *
 */
void
elm_datefield_format_set(Evas_Object *obj, const char *fmt)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (fmt)
     {
        u_strFromUTF8(wd->format, MAX_FORMAT_LEN, 0, fmt, strlen(fmt), &status);
        wd->user_format = EINA_TRUE;
     }
   else  wd->user_format = EINA_FALSE;

   _format_reload(obj);
}

/**
 * Get the format of datefield
 *
 * @param[in] obj The datefield object
 * @return date format string. ex) MMMddyhhmm
 * The Caller has to free the memory of return string.
 *
 */
char *
elm_datefield_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   int length;
   char *format;
   UErrorCode status = U_ZERO_ERROR;

   wd = elm_widget_data_get(obj);
   if (!wd ) return NULL;

   //get the length : pre flighting.
   u_strToUTF8(NULL, 0, &length, wd->format, u_strlen(wd->format), &status);
   format = malloc (length);
   u_strToUTF8(format, length, 0, wd->format, u_strlen(wd->format), &status);

   return format;
}

/**
 * @brief Set the selected value of the datefield
 * Year : default range is from 1970 to 2037.
 * Month value range is from 0 to 11
 * Date value range is from 1 to 31 according to the month value.
 * The hour value should be set according to 24hr format (0~23)
 * Minute value range is from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @param[in] value The data to be set. ex. year/month/date/hour/minute/ampm
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_item_value_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype,
                             int value)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Datefield_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   _field_value_set(obj, it->type, value, EINA_FALSE);
}

/**
 * @brief Get Current value date of the datefield
 * Year : default range is from 1970 to 2037.
 * Month value range is from 0 to 11
 * Date value range is from 1 to 31 according to the month value.
 * The hour value should be set according to 24hr format (0~23)
 * Minute value range is from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return int The value of the field.
 *
 * @ingroup Datefield
 */
EAPI int
elm_datefield_item_value_get(const Evas_Object *obj, Elm_Datefield_ItemType
                             itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return -1;

   return ((wd->item_list + itemtype)->value);
}


/**
 * @brief Enable/Disable an item of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @param[in] enable Item is Enabled or disabled.
 *
 * @ingroup Datefield
 */

EAPI void
elm_datefield_item_enable_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype,
                              Eina_Bool enable)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Datefield_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   if ( it->enabled == enable ) return;
   it->enabled = enable;
   _format_reload(obj);
}

/**
 * @brief Get whether the item is Enabled/Disabled
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return EINA_TRUE = Item is Enabled or EINA_FALSE = disabled.
 *
 * @ingroup Datefield
 */

EAPI Eina_Bool
elm_datefield_item_enable_get(Evas_Object *obj, Elm_Datefield_ItemType itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd;
   Datefield_Item *it;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return EINA_FALSE;

   it = wd->item_list + itemtype;
   return it->enabled;
}

/**
 * @brief Get lower boundary of the datefield
 * Year : default range is from 1970 to 2037.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @param[in] value The minimum value of the field that is to be set.
 * @ingroup Datefield
 */
EAPI void
elm_datefield_item_min_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype,
                           int value,  Eina_Bool abs_limit)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Datefield_Item *it;
   UDateFormatField icu_dftype;
   UErrorCode status = U_ZERO_ERROR;
   int icu_min;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   icu_dftype = it->type;
   icu_min = ucal_getLimit(wd->calendar, udat_toCalendarDateField(icu_dftype),
                           UCAL_MINIMUM, &status);
   if ( value < icu_min ) value = icu_min;
   it->min = value;
   it->abs_min = abs_limit;
   _field_value_set(obj, icu_dftype, it->value, EINA_FALSE);  // Trigger the validation
}

/**
 * @brief Get lower boundary of the datefield
 * Year : default range is from 1970 to 2037.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return int The minimum value of the field.
 *
 * @ingroup Datepicker
 */
EAPI int
elm_datefield_item_min_get(const Evas_Object *obj, Elm_Datefield_ItemType
                           itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return -1;

   return ((wd->item_list + itemtype)->min);
}

/**
 * @brief Get whether the minimum value of the item is absolute or not
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return EINA_TRUE = Minimim is absolute or EINA_FALSE = Minimum is relative.
 *
 * @ingroup Datefield
 */

EAPI Eina_Bool
elm_datefield_item_min_is_absolute(const Evas_Object *obj,
                                   Elm_Datefield_ItemType itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return EINA_FALSE;

   return ((wd->item_list + itemtype)->abs_min);
}

/**
 * @brief Set upper boundary of the datefield
 * Year : default range is from 1970 to 2037.
 * Month:default value range is from 0 to 11
 * Date : default value range is from 1 to 31 according to the month value.
 * Hour : default value will be in terms of 24 hr format (0~23)
 * Minute  : default value range will be from 0 to 59.
 * AM/PM: Value 0 for AM and 1 for PM.
 * If the value is beyond the contextual range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @param[in] value The maximum field value that is to be set.
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_item_max_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype,
                           int value, Eina_Bool abs_limit )
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Datefield_Item *it;
   UDateFormatField icu_dftype;
   UErrorCode status = U_ZERO_ERROR;
   int icu_max;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   icu_dftype = it->type;
   icu_max = ucal_getLimit(wd->calendar, udat_toCalendarDateField(icu_dftype),
                           UCAL_MAXIMUM, &status);
   if ( value > icu_max ) value = icu_max;
   it->max = value;
   it->abs_max = abs_limit;
   _field_value_set(obj, icu_dftype, it->value, EINA_FALSE);  // Trigger the validation
}


/**
 * @brief Get upper boundary of the datefield
 * Year : default range is from 1970 to 2037.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
 * ex:-If month is Feb, Max date can be 28 or 29 for leap years.
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return int The maximum value of the field.
 *
 * @ingroup Datefield
 */
EAPI int
elm_datefield_item_max_get(const Evas_Object *obj, Elm_Datefield_ItemType
                           itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) -1;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return -1;

   return ((wd->item_list + itemtype)->max);
}

/**
 * @brief Get whether the max value of the item is absolute or not
 *
 * @param[in] obj The datefield object
 * @param[in] itemtype The field type of datefield. ELM_DATEFIELD_YEAR etc.
 * @return EINA_TRUE = Max is absolute or EINA_FALSE = Max is relative.
 *
 * @ingroup Datefield
 */

EAPI Eina_Bool
elm_datefield_item_max_is_absolute(const Evas_Object *obj,
                                   Elm_Datefield_ItemType itemtype)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return EINA_FALSE;

   return ((wd->item_list + itemtype)->abs_max);
}


#define DATE_FORMAT_TYPE_COUNT  6

typedef struct _format_mapper
{
   char old_fmt[BUFFER_SIZE];
   char new_fmt[BUFFER_SIZE];
}format_mapper;

format_mapper map_format[DATE_FORMAT_TYPE_COUNT] = {
   { "ddmmyy",  "ddMMy"},
   { "ddyymm",  "ddyMM"},
   { "mmddyy",  "MMddy"},
   { "mmyydd",  "MMydd"},
   { "yymmdd",  "yMMdd"},
   { "yyddmm",  "yddMM"}
};

static char*
_get_format(Evas_Object *obj, const char * format)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *strbuf;
   char * fmt;
   int i= 0;

   if (!wd) return NULL;

   strbuf =  eina_strbuf_new();

   if (wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATE ||
              wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
     {
        for (i =0; i< DATE_FORMAT_TYPE_COUNT; i++)
          {
             if (!strncmp(format, map_format[i].old_fmt, BUFFER_SIZE))
               {
                  eina_strbuf_append(strbuf, map_format[i].new_fmt);
                  break;
               }
          }
     }

   if (wd->datefield_layout == ELM_DATEFIELD_LAYOUT_TIME ||
              wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
     {
        if (wd->time_mode)
          eina_strbuf_append(strbuf, "hh:mma");
        else
          eina_strbuf_append(strbuf, "HH:mm");
     }

   eina_strbuf_append_char(strbuf, 0); // NULL terminated string
   fmt = eina_strbuf_string_steal(strbuf);
   eina_strbuf_free( strbuf );

   return fmt;
}

/**
 * @brief Set layout for the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] layout set layout for date/time/dateandtime
 * (default: ELM_DATEFIELD_LAYOUT_DATEANDTIME)
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_layout_set(Evas_Object *obj, Elm_Datefield_Layout layout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Bool date_enabled;
   Eina_Bool time_enabled;

   if (!wd || layout > ELM_DATEFIELD_LAYOUT_DATEANDTIME) return;
   if (layout == wd->datefield_layout) return;
   wd->datefield_layout = layout;
   date_enabled = ((wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATE)
                  || (wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME));

   time_enabled = ((wd->datefield_layout == ELM_DATEFIELD_LAYOUT_TIME)
                  || (wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME));

    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_YEAR, date_enabled);
    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_MONTH, date_enabled);
    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_DATE, date_enabled);
    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_HOUR, time_enabled);
    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_MINUTE, time_enabled);
    elm_datefield_item_enable_set(obj, ELM_DATEFIELD_AMPM, (time_enabled && wd->time_mode));
}

/**
 * @brief Get layout of the datefield
 *
 * @param[in] obj The datefield object
 * @return layout of the datefield
 *
 * @ingroup Datefield
 */
EAPI Elm_Datefield_Layout
elm_datefield_layout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype)-1;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return -1;

   return (wd->datefield_layout);
}

/**
 * @brief Set date format of datefield
 *
 * @param[in] obj The datefield object
 * @param[in] fmt The date format, ex) mmddyy.
 * Default value is taken according to the system locale settings.
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_format_set(Evas_Object *obj, const char *fmt)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   wd->old_style_format = strdup(fmt);
   elm_datefield_format_set(obj, _get_format(obj, fmt));
}

/**
 * @brief Get the user set format of the datefield
 *
 * @param[in] obj The datefield object
 * @return date format string. ex) mmddyy
 *
 * @ingroup Datefield
 */
EAPI const char *
elm_datefield_date_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;

   return (wd->old_style_format);
}

/**
 * @brief Set if the datefield show hours in military or am/pm mode
 *
 * @param[in] obj The datefield object
 * @param[in] mode option for the hours mode. If true, it is shown as 12h mode,
 * if false, it is shown as 24h mode. Default value is true
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_time_mode_set(Evas_Object *obj, Eina_Bool mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   wd->time_mode = mode;
   elm_datefield_format_set(obj, _get_format(obj, wd->old_style_format));
}

/**
 * @brief get time mode of the datefield
 *
 * @param[in] obj The datefield object
 * @return time mode (EINA_TRUE: 12hour mode / EINA_FALSE: 24hour mode)
 *
 * @ingroup Datefield
 */
EAPI Eina_Bool
elm_datefield_time_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;

   return (wd->time_mode);
}


/////////////////////////////////////////////////////////////////////////////////
////////////////////////// Date Field DEPRECATED APIs ///////////////////////////
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Set selected date of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The year to set
 * @param[in] month The month to set (1-12)
 * @param[in] day The day to set
 * @param[in] hour The hours to set (24hour mode - 0~23)
 * @param[in] min The minutes to set (0~59)
 *
 * @ingroup Datefield
 * @deprecated, use elm_datefield_item_value_set() instead.
 */
EINA_DEPRECATED EAPI void
elm_datefield_date_set(Evas_Object *obj, int year, int month, int day, int hour,
                       int min)
{
   elm_datefield_item_value_set(obj, ELM_DATEFIELD_YEAR, year);
   month -= 1;  // backward compatibility
   elm_datefield_item_value_set(obj, ELM_DATEFIELD_MONTH, month);
   elm_datefield_item_value_set(obj, ELM_DATEFIELD_DATE, day);
   elm_datefield_item_value_set(obj, ELM_DATEFIELD_HOUR, hour);
   elm_datefield_item_value_set(obj, ELM_DATEFIELD_MINUTE, min);
}

/**
 * @brief Get selected date of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The pointer to the variable get the selected year
 * @param[in] month The pointer to the variable get the selected month (1-12)
 * @param[in] day The pointer to the variable get the selected day
 * @param[in] hour The pointer to the variable get the selected hour(24hour mode)
 * @param[in] hour The pointer to the variable get the selected min
 *
 * @ingroup Datefield
 * @deprecated, use elm_datefield_item_value_get() instead.
 */
EINA_DEPRECATED EAPI void
elm_datefield_date_get(const Evas_Object *obj, int *year, int *month, int *day,
                       int *hour, int *min)
{
   if (year)
     *year = elm_datefield_item_value_get(obj, ELM_DATEFIELD_YEAR);
   if (month)
     {
        *month = elm_datefield_item_value_get(obj, ELM_DATEFIELD_MONTH);
        (*month)+=1;  // backward compatibility
     }
   if (day)
     *day = elm_datefield_item_value_get(obj, ELM_DATEFIELD_DATE);
   if (hour)
     *hour = elm_datefield_item_value_get(obj, ELM_DATEFIELD_HOUR);
   if (min)
     *min = elm_datefield_item_value_get(obj, ELM_DATEFIELD_MINUTE);
}

/**
 * @brief Set lower boundary of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The year to set
 * @param[in] month The month to set (1-12)
 * @param[in] day The day to set
 * @return TRUE/FALSE
 *
 * @ingroup Datepicker
 * @deprecated, use elm_datefield_item_min_set() instead.
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_datefield_date_min_set(Evas_Object *obj, int year, int month, int day)
{
   elm_datefield_item_min_set(obj, ELM_DATEFIELD_YEAR, year, EINA_FALSE);
   month -= 1;  // backward compatibility
   elm_datefield_item_min_set(obj, ELM_DATEFIELD_MONTH, month, EINA_FALSE);
   elm_datefield_item_min_set(obj, ELM_DATEFIELD_DATE, day, EINA_FALSE);
   return EINA_TRUE;
}

/**
 * @brief Get lower boundary of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The pointer to the variable get the maximum year
 * @param[in] month The pointer to the variable get the maximum month (1-12)
 * @param[in] day The pointer to the variable get the maximum day
 *
 * @ingroup Datefield
 * @deprecated, use elm_datefield_item_min_get() instead.
 */
EINA_DEPRECATED EAPI void
elm_datefield_date_min_get(const Evas_Object *obj, int *year, int *month,
                           int *day)
{
   if (year)
     *year = elm_datefield_item_min_get(obj, ELM_DATEFIELD_YEAR);
   if (month)
     {
        *month = elm_datefield_item_min_get(obj, ELM_DATEFIELD_MONTH);
        (*month)+=1;  // backward compatibility
     }
   if (day)
     *day = elm_datefield_item_min_get(obj, ELM_DATEFIELD_DATE);
}

/**
 * @brief Set upper boundary of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The year to set
 * @param[in] month The month to set (1-12)
 * @param[in] day The day to set
 * @return TRUE/FALSE
 *
 * @ingroup Datefield
 * @deprecated, use elm_datefield_item_max_set() instead.
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_datefield_date_max_set(Evas_Object *obj, int year, int month, int day)
{
   elm_datefield_item_max_set(obj, ELM_DATEFIELD_YEAR, year, EINA_FALSE);
   month -= 1;  // backward compatibility
   elm_datefield_item_max_set(obj, ELM_DATEFIELD_MONTH, month, EINA_FALSE);
   elm_datefield_item_max_set(obj, ELM_DATEFIELD_DATE, day, EINA_FALSE);
   return EINA_TRUE;
}

/**
 * @brief Get upper boundary of the datefield
 *
 * @param[in] obj The datefield object
 * @param[in] year The pointer to the variable get the maximum year
 * @param[in] month The pointer to the variable get the maximum month (1-12)
 * @param[in] day The pointer to the variable get the maximum day
 *
 * @ingroup Datefield
 * @deprecated, use elm_datefield_item_max_get() instead.
 */
EINA_DEPRECATED EAPI void
elm_datefield_date_max_get(const Evas_Object *obj, int *year, int *month,
                           int *day)
{
   if (year)
     *year = elm_datefield_item_max_get(obj, ELM_DATEFIELD_YEAR);
   if (month)
     {
       *month = elm_datefield_item_max_get(obj, ELM_DATEFIELD_MONTH);
       (*month)+=1;  // backward compatibility
     }
   if (day)
     *day = elm_datefield_item_max_get(obj, ELM_DATEFIELD_DATE);
}

/**
 * @brief Add a callback function for input panel state
 *
 * @param[in] obj The datefield object
 * @param[in] func The function to be called when the event is triggered
 * (value will be the Ecore_IMF_Input_Panel_State)
 * @param[in] data The data pointer to be passed to @p func
 *
 * @ingroup Datefield
 * @deprecated and will no longer be in use.
 */
EINA_DEPRECATED EAPI void
elm_datefield_input_panel_state_callback_add(Evas_Object *obj,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value),
          void *data)
{
   //Empty implementation
}

/**
 * @brief Delete a callback function for input panel state
 *
 * @param[in] obj The datefield object
 * @param[in] func The function to be called when the event is triggered
 *
 * @ingroup Datefield
 * @deprecated and will no longer be in use.
 */
EINA_DEPRECATED EAPI void
elm_datefield_input_panel_state_callback_del(Evas_Object *obj,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value))
{
   //Empty implementation
}
/////////////////////////////////////////////////////////////////////////////////
//////////////////////////Date Field DEPRECATED APIs  END////////////////////////
////////////////////////////////////////////////////////////////////////////////

