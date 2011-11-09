#include <locale.h>
#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Datefield Datefield
 * @ingroup Elementary
 *
 * This is a date edit field. it is used to input date and time using
 * diskselector integrated ctxpopup.
 *
 * Datefield Format can be like "%b %d, %Y %I : %M %p".
 * Maximum allowed format length is 32 chars.
 * Format can include separators for each individual datefield item.
 * Each separator can be a maximum of 6 UTF-8 bytes.
 * Space is also taken as a separator.
 * Following are the allowed set of format specifiers for each datefield item.
 * These specifiers can be arranged at any order as per user requirement and
 * their value will be replaced in the format as mentioned below.
 * %Y : The year as a decimal number including the century.
 * %y : The year as a decimal number without a century (range 00 to 99)
 * %m : The month as a decimal number (range 01 to 12).
 * %b : The abbreviated month name according to the current locale.
 * %B : The full month name according to the current locale.
 * %d : The day of the month as a decimal number (range 01 to 31).
 * %I : The hour as a decimal number using a 12-hour clock (range 01 to 12).
 * %H : The hour as a decimal number using a 24-hour clock (range 00 to 23).
 * %k : The hour (24-hour clock) as a decimal number (range 0 to 23). single
 *      digits are preceded by a blank.
 * %l : The hour (12-hour clock) as a decimal number (range 1 to 12); single
 *      digits are preceded by a blank.
 * %M : The minute as a decimal number (range 00 to 59).
 * %p : Either 'AM' or 'PM' according to the given time value, or the
 *      corresponding strings for the current locale. Noon is treated as 'PM'
 *      and midnight as 'AM'
 * %P : Like %p but in lowercase: 'am' or 'pm' or a corresponding string for
 *      the current locale.
 * For more reference, see the below link:
 * http://www.gnu.org/s/hello/manual/libc.html#Formatting-Calendar-Time
 * Default format is taken as per the system display language and Region format.
 *
 */

typedef struct _Widget_Data Widget_Data;

#define DATEFIELD_TYPE_COUNT        6
#define BUFFER_SIZE                 64
#define MAX_FORMAT_LEN              32
#define MAX_SEPARATOR_LEN           6
#define MAX_ITEM_FORMAT_LEN         3
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

#define DEFAULT_FORMAT "%b %d, %Y %I : %M %p"

typedef struct _Format_Map
{
   Elm_Datefield_ItemType type;
   char fmt_char[5];
   int def_min;
   int def_max;
}Format_Map;

static const Format_Map mapping[DATEFIELD_TYPE_COUNT] = {
   { ELM_DATEFIELD_YEAR,   "Yy",    70, 137 },
   { ELM_DATEFIELD_MONTH,  "mbB",   0,  11  },
   { ELM_DATEFIELD_DATE,   "d",     1,  31  },
   { ELM_DATEFIELD_HOUR,   "IHkl",  0,  23  },
   { ELM_DATEFIELD_MINUTE, "M",     0,  59  },
   { ELM_DATEFIELD_AMPM,   "pP",    0,  1   }
};

static int _days_in_month[12] = { 31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31 };

typedef enum _Elm_Datefield_HourType
  {
     ELM_DATEFIELD_HOUR_12 = 1000,
     ELM_DATEFIELD_HOUR_24,
     ELM_DATEFIELD_HOUR_NA
  } Elm_Datefield_HourType;


typedef struct _Datefield_Item
{
   char fmt[MAX_ITEM_FORMAT_LEN];
   Elm_Datefield_ItemType type;
   Elm_Datefield_HourType hour_type;
   const char *content; //string to be displayed
   const char *separator;
   int location; //location of the item as per the current format
   int *value;
   int min, max;
   int default_min, default_max;
   Eina_Bool fmt_exist:1; //if item format is present or not
   Eina_Bool enabled:1; //if item is to be shown or not
   Eina_Bool abs_min:1;
   Eina_Bool abs_max:1;
} Datefield_Item;

struct _Widget_Data
{
   Evas_Object *base;
   struct tm *time;
   int ampm;
   Datefield_Item *item_list; //Fixed set of items, so no Eina list.
   Evas_Object *ctxpopup;
   Datefield_Item *selected_it;
   char format[MAX_FORMAT_LEN];
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

static void _update_items(Evas_Object *obj);
static void _field_value_set(Evas_Object * obj, Elm_Datefield_ItemType type,
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
   if (wd->time) free(wd->time);
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        tmp = wd->item_list + idx;
        eina_stringshare_replace(&tmp->content, NULL);
        eina_stringshare_replace(&tmp->separator, NULL);
     }
   if (wd->item_list) free(wd->item_list);
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
   unsigned int idx;
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
             if (it->type == ELM_DATEFIELD_AMPM)
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
                Eina_Bool hr_fmt_check, int *range_min, int *range_max)
{
   Widget_Data *wd;
   Datefield_Item * tmp;
   unsigned int idx;
   int ctx_max;
   Eina_Bool min_limit = EINA_TRUE;
   Eina_Bool max_limit = EINA_TRUE;

   wd = elm_widget_data_get(obj);
   if (!wd || !it) return;

   //Top to down check for current field relative min/max limit
   if (!it->abs_min || !it->abs_max )
     {
        for (idx = ELM_DATEFIELD_YEAR; idx < it->type; idx++)
          {
             tmp = wd->item_list + idx;
             if (max_limit && (*(tmp->value) < tmp->max)) max_limit= EINA_FALSE;
             if (min_limit && (*(tmp->value) > tmp->min)) min_limit= EINA_FALSE;
          }
     }

   if (it->abs_min || min_limit) (*range_min) = it->min;
   else (*range_min) = it->default_min;

   if (it->abs_max || max_limit) (*range_max) = it->max;
   else (*range_max) = it->default_max;

   ctx_max = it->default_max;
   if (it->type == ELM_DATEFIELD_DATE )
     {
        ctx_max = _days_in_month[wd->time->tm_mon];
        // Check for Leap year Feb.
        if (__isleap((wd->time->tm_year)) && wd->time->tm_mon == 1) ctx_max= 29;
     }
   else if (it->type == ELM_DATEFIELD_HOUR  &&  hr_fmt_check &&
            it->hour_type == ELM_DATEFIELD_HOUR_12 )  ctx_max = 11;

   if (*range_max > ctx_max) *range_max = ctx_max;
}

static void
_update_items(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   unsigned int idx= 0;

   wd = elm_widget_data_get(obj);
   if (!wd || !wd->base) return;
   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        if ( it->fmt_exist && it->enabled )
          {
             strftime(buf, BUFFER_SIZE, it->fmt, wd->time);

             // FIXME: no locale string availble from Libc...
             if ((!strncmp(buf, "",1)) && (it->type == ELM_DATEFIELD_AMPM))
               {
                  if (wd->ampm) strncpy(buf, E_("PM"), BUFFER_SIZE);
                  else strncpy(buf, E_("AM"), BUFFER_SIZE);
               }
             eina_stringshare_replace(&it->content, buf);
             snprintf(buf, sizeof(buf), EDC_PART_ITEM_STR, it->location);
             edje_object_part_text_set(wd->base, buf, it->content);
          }
     }
}

static void
_field_value_set(Evas_Object * obj, Elm_Datefield_ItemType item_type, int value,
                 Eina_Bool adjust_time)
{
   Widget_Data *wd;
   Datefield_Item * it;
   unsigned int idx;
   int min, max;
   Eina_Bool value_changed = EINA_FALSE;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (item_type == ELM_DATEFIELD_AMPM)
     {
        if ( value == wd->ampm ) return;
        item_type = ELM_DATEFIELD_HOUR;
        value = (wd->time->tm_hour + 12) % 24;
        adjust_time =  EINA_FALSE;
     }

   it = wd->item_list + item_type;
   _contextual_field_limit_get(obj, it, EINA_FALSE, &min, &max);

   //12 hr format & PM then add 12 to value.
   if (adjust_time && it->type == ELM_DATEFIELD_HOUR &&
       it->hour_type == ELM_DATEFIELD_HOUR_12 && wd->ampm && value < 12)
      value += 12;

   if (value < min) value = min;
   else if (value > max) value = max;
   if ( *(it->value) == value) return;
   *(it->value) = value;
   value_changed = EINA_TRUE;

   //Validate & reset lower order fields
   for ( idx = item_type+1; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        _contextual_field_limit_get(obj, it, EINA_FALSE, &min, &max);
        //Validate current value against context based Min/Max restriction.
        if (*it->value < min)
          {
             *it->value = min;
             value_changed = EINA_TRUE;
          }
        else if (*it->value > max)
          {
             *it->value = max;
             value_changed = EINA_TRUE;
          }
     }
   //update AM/PM state
   wd->ampm = (wd->time->tm_hour > 11 );
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
   char buf[BUFFER_SIZE];

   wd = elm_widget_data_get(data);
   if (!wd || !wd->base) return;

   _field_value_set( data, ELM_DATEFIELD_AMPM, !wd->ampm, EINA_FALSE );

   snprintf(buf, sizeof(buf), EDC_PART_ITEM_FOCUSOUT_SIG_STR,
            wd->selected_it->location);
   edje_object_signal_emit(wd->base, buf, "elm");
   wd->selected_it = NULL;
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
   DiskItem_Data *disk_data;
   Elm_Diskselector_Item *item;
   int idx, min, max, selected_val;
   int text_len, max_len = 0;
   char item_label[BUFFER_SIZE];
   int cur_val, date_val;

   wd = elm_widget_data_get(data);
   if (!wd) return;

   cur_val = *(it->value);
   date_val = wd->time->tm_mday;
   _contextual_field_limit_get(data, it, EINA_TRUE, &min, &max );

   selected_val = *(it->value);
   wd->time->tm_mday = 1;
   // If 12hr format & PM, reduce 12
   if (it->hour_type == ELM_DATEFIELD_HOUR_12 && wd->ampm) selected_val -= 12;

   for (idx = min; idx <= max; idx++)
     {
        *(it->value) = idx;
        strftime(item_label, BUFFER_SIZE, it->fmt, wd->time );
        text_len = strlen(item_label);
        if (text_len > max_len ) max_len = text_len; //Store max. label length

        if (idx == selected_val) //Selected Item, dont attach a callback handler
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
   *(it->value) = cur_val;
   wd->time->tm_mday = date_val;
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
   unsigned int idx = 0, idx1 = 0, display_item_num;
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

   if ( wd->selected_it->type == ELM_DATEFIELD_AMPM )
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

   elm_ctxpopup_direction_priority_set(wd->ctxpopup, ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_UP, -1, -1);
   elm_object_content_set(wd->ctxpopup, diskselector);
   snprintf(buf,sizeof(buf), EDC_PART_ITEM_STR, wd->selected_it->location);
   edj_part = edje_object_part_object_get(wd->base, buf);
   evas_object_geometry_get(edj_part, &x, &y, &w, &h);
   evas_object_move(wd->ctxpopup, (x+w/2), (y+h));

   //If the direction of Ctxpopup is upwards, move it to the top of datefield
   if (elm_ctxpopup_direction_get (wd->ctxpopup) == ELM_CTXPOPUP_DIRECTION_UP)
     {
        elm_ctxpopup_direction_priority_set(wd->ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
                                            ELM_CTXPOPUP_DIRECTION_DOWN, -1, -1);
        evas_object_move(wd->ctxpopup, (x+w/2), y);
     }
   evas_object_show(wd->ctxpopup);

   evas_object_geometry_get(diskselector, NULL, NULL, &disksel_width, NULL);
   display_item_num = disksel_width / (w +  elm_finger_size_get());
   //odd number of items leads to auto selection.
   //making as event number of item to prevent auto selection.
   if (display_item_num%2) display_item_num-=1;
   if (display_item_num < DISKSELECTOR_ITEMS_NUM_MIN)
     display_item_num = DISKSELECTOR_ITEMS_NUM_MIN;

   elm_diskselector_display_item_num_set(diskselector, display_item_num);
   elm_diskselector_round_set(diskselector, EINA_TRUE);
}

static unsigned int
_parse_format( Evas_Object *obj )
{
   Widget_Data *wd;
   Datefield_Item *it = NULL;
   unsigned int len = 0, idx, location = 0;
   char separator[MAX_SEPARATOR_LEN];
   char *fmt_ptr;
   char cur;
   Eina_Bool fmt_parsing = EINA_FALSE, sep_parsing = EINA_FALSE,
             sep_lookup = EINA_FALSE;

   wd = elm_widget_data_get(obj);
   fmt_ptr = wd->format;

   while ( (cur = *fmt_ptr ) )
     {
        if (fmt_parsing)
          {
             for ( idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
               {
                  if ( strchr( mapping[idx].fmt_char, cur ) )
                    {
                       it = wd->item_list + idx;
                       //Ignore the items already have or disabled
                       //valid formats, means already parsed & repeated, ignore.
                       if (!it->enabled || it->location != -1) break;
                       it->fmt[1] = cur;

                      //set the hour display format 12h/24h
                       if (it->type == ELM_DATEFIELD_HOUR)
                         {
                            if (cur == 'H' || cur == 'k' )
                              it->hour_type = ELM_DATEFIELD_HOUR_24;
                            else if (cur == 'I' || cur == 'l' )
                              it->hour_type = ELM_DATEFIELD_HOUR_12;
                         }
                       else it->hour_type = ELM_DATEFIELD_HOUR_NA;

                       it->fmt_exist = EINA_TRUE;
                       it->location = location++;
                       fmt_parsing = EINA_FALSE;
                       sep_lookup = EINA_TRUE;
                       len = 0;
                       break;
                    }
               }
          }

        if (cur == '%')
          {
             fmt_parsing = EINA_TRUE;
             sep_parsing = EINA_FALSE;
             // Set the separator to previous Item
             separator[len] = 0;
             if (it) eina_stringshare_replace(&it->separator, separator);
          }
        if (sep_parsing && (len < MAX_SEPARATOR_LEN-1)) separator[len++] = cur;
        if (sep_lookup) sep_parsing = EINA_TRUE;
        sep_lookup = EINA_FALSE;
        fmt_ptr++;
   }
   // Return the number of valid items parsed.
   return location;
}

static void
_format_reload(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   unsigned int idx, location;
   char *def_fmt;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

    // fetch the format from locale specific po file.
   if (!wd->user_format )
     {
        def_fmt = E_("DateTimeFormat");
        if (!strncmp(def_fmt, "DateTimeFormat", sizeof("DateTimeFormat")))
          strncpy(wd->format, DEFAULT_FORMAT, MAX_FORMAT_LEN );
        else
          strncpy(wd->format, def_fmt, MAX_FORMAT_LEN );
     }

   //reset all the items to disable state
   for ( idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++ )
     {
        it = wd->item_list + idx;
        eina_stringshare_replace(&it->content, NULL);
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
             if (it->type == ELM_DATEFIELD_AMPM)
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
        snprintf(buf, sizeof(buf), EDC_PART_SEPARATOR_STR, it->location+1);
        edje_object_part_text_set(wd->base, buf, it->separator);
     }
   edje_object_message_signal_process(wd->base);

   wd->time_mode = ((wd->item_list + ELM_DATEFIELD_HOUR)->hour_type
                        == ELM_DATEFIELD_HOUR_12);
   _update_items(obj);
}

static void
_item_list_init(Evas_Object *obj)
{
   Widget_Data *wd;
   Datefield_Item *it;
   char buf[BUFFER_SIZE];
   unsigned int idx;
   time_t t;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->item_list = calloc(1, DATEFIELD_TYPE_COUNT * sizeof(Datefield_Item));
   wd->time = calloc(1, sizeof(struct tm));
   t = time(NULL);
   localtime_r(&t, wd->time);

   (wd->item_list + ELM_DATEFIELD_YEAR)->value = &wd->time->tm_year;
   (wd->item_list + ELM_DATEFIELD_MONTH)->value = &wd->time->tm_mon;
   (wd->item_list + ELM_DATEFIELD_DATE)->value = &wd->time->tm_mday;
   (wd->item_list + ELM_DATEFIELD_HOUR)->value = &wd->time->tm_hour;
   (wd->item_list + ELM_DATEFIELD_MINUTE)->value = &wd->time->tm_min;
   (wd->item_list + ELM_DATEFIELD_AMPM)->value = &wd->ampm;
    wd->ampm = (wd->time->tm_hour > 11 );

   for (idx = 0; idx < DATEFIELD_TYPE_COUNT; idx++)
     {
        it = wd->item_list + idx;
        it->type = ELM_DATEFIELD_YEAR + idx;
        it->fmt[0] = '%';
        it->fmt_exist = EINA_FALSE;
        it->enabled  = EINA_TRUE;
        it->min = mapping[idx].def_min;
        it->default_min = mapping[idx].def_min;
        it->max = mapping[idx].def_max;
        it->default_max = mapping[idx].def_max;
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

   _item_list_init(obj);
   _format_reload(obj);

   wd->datefield_layout = ELM_DATEFIELD_LAYOUT_DATEANDTIME;
   wd->time_mode = EINA_TRUE;
   wd->old_style_format = "mmddyy";

   wd->ctxpopup = elm_ctxpopup_add(elm_widget_top_get(obj));
   elm_object_style_set(wd->ctxpopup, "datefield/default");
   elm_ctxpopup_horizontal_set(wd->ctxpopup, EINA_TRUE);
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
 * Set the format of datefield. Formats can be like "%b %d, %Y %I : %M %p".
 * Maximum allowed format length is 32 chars.
 * Format can include separators for each individual datefield item.
 * Each separator can be a maximum of 6 UTF-8 bytes.
 * Space is also taken as a separator.
 * Following are the allowed set of format specifiers for each datefield item.
 * These specifiers can be arranged at any order as per user requirement and
 * their value will be replaced in the format as mentioned below.
 * %Y : The year as a decimal number including the century.
 * %y : The year as a decimal number without a century (range 00 to 99)
 * %m : The month as a decimal number (range 01 to 12).
 * %b : The abbreviated month name according to the current locale.
 * %B : The full month name according to the current locale.
 * %d : The day of the month as a decimal number (range 01 to 31).
 * %I : The hour as a decimal number using a 12-hour clock (range 01 to 12).
 * %H : The hour as a decimal number using a 24-hour clock (range 00 to 23).
 * %k : The hour (24-hour clock) as a decimal number (range 0 to 23). single
 *      digits are preceded by a blank.
 * %l : The hour (12-hour clock) as a decimal number (range 1 to 12); single
 *      digits are preceded by a blank.
 * %M : The minute as a decimal number (range 00 to 59).
 * %p : Either 'AM' or 'PM' according to the given time value, or the
 *      corresponding strings for the current locale. Noon is treated as 'PM'
 *      and midnight as 'AM'
 * %P : Like %p but in lowercase: 'am' or 'pm' or a corresponding string for
 *      the current locale.
 * Default format is taken as per the system display language and Region format.
 *
 * @param[in] obj The datefield object
 * @param[in] fmt The date format
 *
 */
EAPI void
elm_datefield_format_set(Evas_Object *obj, const char *fmt)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;

   wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (fmt)
     {
        strncpy( wd->format, fmt, MAX_FORMAT_LEN );
        wd->user_format = EINA_TRUE;
     }
   else  wd->user_format = EINA_FALSE;

   _format_reload(obj);
}

/**
 * Get the format of datefield. Formats can be like "%b %d, %Y %I : %M %p".
 * Maximum allowed format length is 32 chars.
 * Format can include separators for each individual datefield item.
 * Each separator can be a maximum of 6 UTF-8 bytes.
 * Space is also taken as a separator.
 * Following are the allowed set of format specifiers for each datefield item.
 * These specifiers can be arranged at any order as per user requirement and
 * their value will be replaced in the format as mentioned below.
 * %Y : The year as a decimal number including the century.
 * %y : The year as a decimal number without a century (range 00 to 99)
 * %m : The month as a decimal number (range 01 to 12).
 * %b : The abbreviated month name according to the current locale.
 * %B : The full month name according to the current locale.
 * %d : The day of the month as a decimal number (range 01 to 31).
 * %I : The hour as a decimal number using a 12-hour clock (range 01 to 12).
 * %H : The hour as a decimal number using a 24-hour clock (range 00 to 23).
 * %k : The hour (24-hour clock) as a decimal number (range 0 to 23). single
 *      digits are preceded by a blank.
 * %l : The hour (12-hour clock) as a decimal number (range 1 to 12); single
 *      digits are preceded by a blank.
 * %M : The minute as a decimal number (range 00 to 59).
 * %p : Either 'AM' or 'PM' according to the given time value, or the
 *      corresponding strings for the current locale. Noon is treated as 'PM'
 *      and midnight as 'AM'
 * %P : Like %p but in lowercase: 'am' or 'pm' or a corresponding string for
 *      the current locale.
 * Default format is taken as per the system display language and Region format.
 *
 * @param[in] obj The datefield object
 * @return date format string. ex) %b %d, %Y %I : %M %p
 *
 */
EAPI char *
elm_datefield_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd ) return NULL;
   return strdup(wd->format);
}

/**
 * @brief Set the selected value of the datefield
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Month value range is from 0 to 11
 * Date value range is from 1 to 31 according to the month value.
 * The hour value should be set according to 24hr format (0~23)
 * Minute value range is from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Month value range is from 0 to 11
 * Date value range is from 1 to 31 according to the month value.
 * The hour value should be set according to 24hr format (0~23)
 * Minute value range is from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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

   return (*(wd->item_list + itemtype)->value);
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
elm_datefield_item_enabled_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype,
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
elm_datefield_item_enabled_get(const Evas_Object *obj, Elm_Datefield_ItemType itemtype)
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
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   if ( it->type != ELM_DATEFIELD_YEAR && value < it->default_min )
      it->min = it->default_min;
   else
      it->min = value;

   if(it->min > it->max ) it->max = it->min;
   it->abs_min = abs_limit;
   _field_value_set(obj, it->type, *(it->value), EINA_FALSE);  // Trigger the validation
}

/**
 * @brief Get lower boundary of the datefield
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Year default range is from 70 to 137.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Month:default value range is from 0 to 11
 * Date : default value range is from 1 to 31 according to the month value.
 * Hour : default value will be in terms of 24 hr format (0~23)
 * Minute  : default value range will be from 0 to 59.
 * AM/PM: Value 0 for AM and 1 for PM.
 * If the value is beyond the contextual range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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

   wd = elm_widget_data_get(obj);
   if (!wd || itemtype > ELM_DATEFIELD_AMPM ) return;

   it = wd->item_list + itemtype;
   if (it->type != ELM_DATEFIELD_YEAR && value > it->default_max )
      it->max = it->default_max;
   else
      it->max = value;

   if(it->max < it->min) it->min = it->max;
   it->abs_max = abs_limit;

   _field_value_set(obj, it->type, *(it->value), EINA_FALSE);  // Trigger the validation
}

/**
 * @brief Get upper boundary of the datefield
 * Year : years since 1900. Negative value represents year below 1900. (
 * year value -30 represents 1870). Year default range is from 70 to 137.
 * Month default value range is from 0 to 11
 * Date default value range is from 1 to 31 according to the month value.
 * Hour default value will be in terms of 24 hr format (0~23)
 * Minute default value range will be from 0 to 59.
 * AM/PM. Value 0 for AM and 1 for PM.
 * If the value is beyond the range,
 * a) Value is less than Min range value, then Min range value is set.
 * b) Greater than Max range value, then Max Range value is set.
 * Both Min and Max range of individual fields are bound to the current context.
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


/////////////////////////////////////////////////////////////////////////////////
////////////////////////// Date Field DEPRECATED APIs ///////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define DATE_FORMAT_TYPE_COUNT 6

typedef struct _format_mapper
{
   char old_fmt[BUFFER_SIZE];
   char new_fmt[BUFFER_SIZE];
}format_mapper;

format_mapper map_format[DATE_FORMAT_TYPE_COUNT] = {
   { "ddmmyy",  "%d %b %Y" },
   { "ddyymm",  "%d %Y %b" },
   { "mmddyy",  "%b %d %Y" },
   { "mmyydd",  "%b %Y %d" },
   { "yymmdd",  "%Y %b %d" },
   { "yyddmm",  "%Y %d %b" }
};

static char*
_get_format(Evas_Object *obj, const char * format)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *strbuf;
   char * fmt;
   unsigned int i= 0;

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
          eina_strbuf_append(strbuf, "%I : %M %p");
        else
          eina_strbuf_append(strbuf, "%H : %M");
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
EINA_DEPRECATED EAPI void
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
                  ||(wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME));

   time_enabled = ((wd->datefield_layout == ELM_DATEFIELD_LAYOUT_TIME)
                  ||(wd->datefield_layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME));

   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_YEAR, date_enabled);
   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_MONTH, date_enabled);
   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_DATE, date_enabled);
   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_HOUR, time_enabled);
   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_MINUTE, time_enabled);
   elm_datefield_item_enabled_set(obj, ELM_DATEFIELD_AMPM,
                                 (time_enabled && wd->time_mode));
}

/**
 * @brief Get layout of the datefield
 *
 * @param[in] obj The datefield object
 * @return layout of the datefield
 *
 * @ingroup Datefield
 */
EINA_DEPRECATED EAPI Elm_Datefield_Layout
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
EINA_DEPRECATED EAPI void
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
EINA_DEPRECATED EAPI const char *
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
EINA_DEPRECATED EAPI void
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
EINA_DEPRECATED EAPI Eina_Bool
elm_datefield_time_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;

   return (wd->time_mode);
}

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
   year -= 1900;  // backward compatibility
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
     {
        *year = elm_datefield_item_value_get(obj, ELM_DATEFIELD_YEAR);
        *year = (*year+1900);  // backward compatibility
     }
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
   year -= 1900;  // backward compatibility
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
     {
        *year = elm_datefield_item_min_get(obj, ELM_DATEFIELD_YEAR);
        *year = (*year) + 1900;  // backward compatibility
     }
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
   year -= 1900;  // backward compatibility
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
   {
     *year = elm_datefield_item_max_get(obj, ELM_DATEFIELD_YEAR);
     *year = (*year) + 1900;  // backward compatibility
   }
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
elm_datefield_input_panel_state_callback_add(Evas_Object *obj __UNUSED__,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value)
          __UNUSED__, void *data __UNUSED__)
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
elm_datefield_input_panel_state_callback_del(Evas_Object *obj __UNUSED__,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value)
          __UNUSED__)
{
   //Empty implementation
}
/////////////////////////////////////////////////////////////////////////////////
//////////////////////////Date Field DEPRECATED APIs  END////////////////////////
////////////////////////////////////////////////////////////////////////////////

